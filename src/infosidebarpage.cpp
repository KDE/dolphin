/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include <config-kmetadata.h>

#include "infosidebarpage.h"

#include <QLayout>
#include <QPixmap>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QMenu>
#include <QPainter>
#include <QFontMetrics>
#include <QEvent>
#include <QInputDialog>
#include <QDir>

#include <kfileplacesmodel.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/previewjob.h>
#include <kfileitem.h>
#include <kdialog.h>
#include <kglobalsettings.h>
#include <kfilemetainfo.h>
#include <kvbox.h>
#include <kseparator.h>
#include <kiconloader.h>

#include "pixmapviewer.h"
#include "dolphinsettings.h"
#include "metadatawidget.h"

InfoSidebarPage::InfoSidebarPage(QWidget* parent) :
        SidebarPage(parent),
        m_multipleSelection(false), //TODO:check if I'm needed
        m_pendingPreview(false),
        m_timer(0),
        m_currentSelection(KFileItemList()),
        m_preview(0),
        m_name(0),
        m_infos(0)
{
    const int spacing = KDialog::spacingHint();

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(slotTimeout()));

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setSpacing(spacing);

    // preview
    m_preview = new PixmapViewer(this);
    m_preview->setMinimumWidth(K3Icon::SizeEnormous);
    m_preview->setFixedHeight(K3Icon::SizeEnormous);

    // name
    m_name = new QLabel(this);
    m_name->setTextFormat(Qt::RichText);
    m_name->setAlignment(m_name->alignment() | Qt::AlignHCenter);
    QFontMetrics fontMetrics(m_name->font());
    m_name->setMinimumHeight(fontMetrics.height() * 3);
    m_name->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

    KSeparator* sep1 = new KSeparator(this);

    // general information
    m_infos = new QLabel(this);
    m_infos->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_infos->setTextFormat(Qt::RichText);

    KSeparator* sep2 = new KSeparator(this);

    if (MetaDataWidget::metaDataAvailable())
        m_metadataWidget = new MetaDataWidget(this);
    else
        m_metadataWidget = 0;

    // actions
    m_actionBox = new KVBox(this);
    m_actionBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    layout->addItem(new QSpacerItem(spacing, spacing, QSizePolicy::Preferred, QSizePolicy::Fixed));
    layout->addWidget(m_preview);
    layout->addWidget(m_name);
    layout->addWidget(sep1);
    layout->addWidget(m_infos);
    layout->addWidget(sep2);
    if (m_metadataWidget) {
        layout->addWidget(m_metadataWidget);
        layout->addWidget(new KSeparator(this));
    }
    layout->addWidget(m_actionBox);
    // ensure that widgets in the information side bar are aligned towards the top
    layout->addStretch(1);
    setLayout(layout);
}

InfoSidebarPage::~InfoSidebarPage()
{}

void InfoSidebarPage::setUrl(const KUrl& url)
{
    if (!m_shownUrl.equals(url, KUrl::CompareWithoutTrailingSlash)) {
        cancelRequest();
        m_shownUrl = url;
        showItemInfo();
    }
}

void InfoSidebarPage::setSelection(const KFileItemList& selection)
{
    // TODO: deactivated the following code, as it has side effects. To
    // reproduce start Dolphin and open a folder -> the URL navigator gets
    // reset. First guess: it seems that a setUrl signal is emitted
    // by the following code

    Q_UNUSED(selection);
    /*cancelRequest();
    m_currentSelection = selection;
    m_multipleSelection = (m_currentSelection.size() > 1);
    showItemInfo();*/
}

void InfoSidebarPage::requestDelayedItemInfo(const KUrl& url)
{
    cancelRequest();

    if (!url.isEmpty() && !m_multipleSelection) {
        m_urlCandidate = url;
        m_timer->setSingleShot(true);
        m_timer->start(300);
    }
}

void InfoSidebarPage::showItemInfo()
{
    cancelRequest();

    KFileItemList selectedItems = m_currentSelection;
    KUrl file;
    if (selectedItems.count() == 0) {
        file = m_shownUrl;
    } else {
        file = selectedItems[0]->url();
    }
    if (m_multipleSelection) {
        KIconLoader iconLoader;
        QPixmap icon = iconLoader.loadIcon("exec",
                                           K3Icon::NoGroup,
                                           K3Icon::SizeEnormous);
        m_preview->setPixmap(icon);
        m_name->setText(i18n("%1 items selected", selectedItems.count()));
    } else if (!applyBookmark(file)) {
        // try to get a preview pixmap from the item...
        KUrl::List list;
        list.append(file);

        m_pendingPreview = true;
        m_preview->setPixmap(QPixmap());

        KIO::PreviewJob* job = KIO::filePreview(list,
                                                m_preview->width(),
                                                K3Icon::SizeEnormous,
                                                0,
                                                0,
                                                true,
                                                false);
        job->setIgnoreMaximumSize(true);

        connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
                this, SLOT(gotPreview(const KFileItem&, const QPixmap&)));
        connect(job, SIGNAL(failed(const KFileItem&)),
                this, SLOT(slotPreviewFailed(const KFileItem&)));

        QString text("<b>");
        text.append(file.fileName());
        text.append("</b>");
        m_name->setText(text);
    }

    createMetaInfo();
    insertActions();
}

void InfoSidebarPage::slotTimeout()
{
    m_shownUrl = m_urlCandidate;
    showItemInfo();
}

void InfoSidebarPage::slotPreviewFailed(const KFileItem& item)
{
    m_pendingPreview = false;
    if (!applyBookmark(item.url())) {
        m_preview->setPixmap(item.pixmap(K3Icon::SizeEnormous));
    }
}

void InfoSidebarPage::gotPreview(const KFileItem& item,
                                 const QPixmap& pixmap)
{
    Q_UNUSED(item);
    if (m_pendingPreview) {
        m_preview->setPixmap(pixmap);
        m_pendingPreview = false;
    }
}

void InfoSidebarPage::startService(int index)
{
    if (m_currentSelection.count() > 0) {
        // TODO: Use "at()" as soon as executeService is fixed to take a const param (BIC)
        KDesktopFileActions::executeService(m_currentSelection.urlList(), m_actionsVector[index]);
    } else {
        // TODO: likewise
        KDesktopFileActions::executeService(m_shownUrl, m_actionsVector[index]);
    }
}

bool InfoSidebarPage::applyBookmark(const KUrl& url)
{
    KFilePlacesModel *placesModel = DolphinSettings::instance().placesModel();
    int count = placesModel->rowCount();

    for (int i = 0; i < count; ++i) {
        QModelIndex index = placesModel->index(i, 0);

        if (url.equals(placesModel->url(index), KUrl::CompareWithoutTrailingSlash)) {
            QString text("<b>");
            text.append(placesModel->text(index));
            text.append("</b>");
            m_name->setText(text);

            m_preview->setPixmap(placesModel->icon(index).pixmap(128, 128));
            return true;
        }
    }

    return false;
}

void InfoSidebarPage::cancelRequest()
{
    m_timer->stop();
    m_pendingPreview = false;
}

void InfoSidebarPage::createMetaInfo()
{
    beginInfoLines();
    if (m_currentSelection.size() == 0) {
        KFileItem fileItem(S_IFDIR, KFileItem::Unknown, m_shownUrl);
        fileItem.refresh();

        if (fileItem.isDir()) {
            addInfoLine(i18n("Type:"), i18n("Directory"));
        }
        if (MetaDataWidget::metaDataAvailable())
            m_metadataWidget->setFile(fileItem.url());
    } else if (m_currentSelection.count() == 1) {
        KFileItem* fileItem = m_currentSelection.at(0);
        addInfoLine(i18n("Type:"), fileItem->mimeComment());

        QString sizeText(KIO::convertSize(fileItem->size()));
        addInfoLine(i18n("Size:"), sizeText);
        addInfoLine(i18n("Modified:"), fileItem->timeString());

        const KFileMetaInfo& metaInfo = fileItem->metaInfo();
        if (metaInfo.isValid()) {
            QStringList keys = metaInfo.supportedKeys();
            for (QStringList::Iterator it = keys.begin(); it != keys.end(); ++it) {
                if (showMetaInfo(*it)) {
                    KFileMetaInfoItem metaInfoItem = metaInfo.item(*it);
                    addInfoLine(*it, metaInfoItem.value().toString());
                }
            }
        }
        if (MetaDataWidget::metaDataAvailable())
            m_metadataWidget->setFile(fileItem->url());
    } else {
        if (MetaDataWidget::metaDataAvailable())
            m_metadataWidget->setFiles(m_currentSelection.urlList());
        unsigned long int totSize = 0;
        foreach(KFileItem* item, m_currentSelection) {
            totSize += item->size(); //FIXME what to do with directories ? (same with the one-item-selected-code), item->size() does not return the size of the content : not very instinctive for users
        }
        addInfoLine(i18n("Total size:"), KIO::convertSize(totSize));
    }
    endInfoLines();
}

void InfoSidebarPage::beginInfoLines()
{
    m_infoLines = QString("");
}

void InfoSidebarPage::endInfoLines()
{
    m_infos->setText(m_infoLines);
}

bool InfoSidebarPage::showMetaInfo(const QString& key) const
{
    // sorted list of keys, where it's data should be shown
    static const char* keys[] = {
                                    "Album",
                                    "Artist",
                                    "Author",
                                    "Bitrate",
                                    "Date",
                                    "Dimensions",
                                    "Genre",
                                    "Length",
                                    "Lines",
                                    "Pages",
                                    "Title",
                                    "Words"
                                };

    // do a binary search for the key...
    int top = 0;
    int bottom = sizeof(keys) / sizeof(char*) - 1;
    while (top < bottom) {
        const int middle = (top + bottom) / 2;
        const int result = key.compare(keys[middle]);
        if (result < 0) {
            bottom = middle - 1;
        } else if (result > 0) {
            top = middle + 1;
        } else {
            return true;
        }
    }

    return false;
}

void InfoSidebarPage::addInfoLine(const QString& labelText, const QString& infoText)
{
    if (!m_infoLines.isEmpty())
        m_infoLines += "<br/>";
    m_infoLines += QString("<b>%1</b> %2").arg(labelText).arg(infoText);
}

void InfoSidebarPage::insertActions()
{
    QListIterator<QPushButton*> deleteIter(m_actionBox->findChildren<QPushButton*>());
    QWidget* widget = 0;
    while (deleteIter.hasNext()) {
        widget = deleteIter.next();
        widget->close();
        widget->deleteLater();
    }

    m_actionsVector.clear();

    int actionsIndex = 0;

    // The algorithm for searching the available actions works on a list
    // of KFileItems. If no selection is given, a temporary KFileItem
    // by the given Url 'url' is created and added to the list.
    KFileItem fileItem(S_IFDIR, KFileItem::Unknown, m_shownUrl);
    KFileItemList itemList = m_currentSelection;
    if (itemList.isEmpty()) {
        fileItem.refresh();
        itemList.append(&fileItem);
    }

    // 'itemList' contains now all KFileItems, where an item information should be shown.
    // TODO: the following algorithm is quite equal to DolphinContextMenu::insertActionItems().
    // It's open yet whether they should be merged or whether they have to work slightly different.
    QStringList dirs = KGlobal::dirs()->findDirs("data", "dolphin/servicemenus/");
    for (QStringList::ConstIterator dirIt = dirs.begin(); dirIt != dirs.end(); ++dirIt) {
        QDir dir(*dirIt);
        QStringList entries = dir.entryList(QStringList("*.desktop"), QDir::Files);

        for (QStringList::ConstIterator entryIt = entries.begin(); entryIt != entries.end(); ++entryIt) {
            KConfigGroup cfg(KSharedConfig::openConfig(*dirIt + *entryIt, KConfig::OnlyLocal), "Desktop Entry");
            if ((cfg.hasKey("Actions") || cfg.hasKey("X-KDE-GetActionMenu")) && cfg.hasKey("ServiceTypes")) {
                const QStringList types = cfg.readEntry("ServiceTypes", QStringList(), ',');
                for (QStringList::ConstIterator it = types.begin(); it != types.end(); ++it) {
                    // check whether the mime type is equal or whether the
                    // mimegroup (e. g. image/*) is supported

                    bool insert = false;
                    if ((*it) == "all/allfiles") {
                        // The service type is valid for all files, but not for directories.
                        // Check whether the selected items only consist of files...
                        QListIterator<KFileItem*> mimeIt(itemList);
                        insert = true;
                        while (insert && mimeIt.hasNext()) {
                            KFileItem* item = mimeIt.next();
                            insert = !item->isDir();
                        }
                    }

                    if (!insert) {
                        // Check whether the MIME types of all selected files match
                        // to the mimetype of the service action. As soon as one MIME
                        // type does not match, no service menu is shown at all.
                        QListIterator<KFileItem*> mimeIt(itemList);
                        insert = true;
                        while (insert && mimeIt.hasNext()) {
                            KFileItem* item = mimeIt.next();
                            const QString mimeType(item->mimetype());
                            const QString mimeGroup(mimeType.left(mimeType.indexOf('/')));

                            insert  = (*it == mimeType) ||
                                      ((*it).right(1) == "*") &&
                                      ((*it).left((*it).indexOf('/')) == mimeGroup);
                        }
                    }

                    if (insert) {
                        const QString submenuName = cfg.readEntry("X-KDE-Submenu");
                        QMenu* popup = 0;
                        if (!submenuName.isEmpty()) {
                            // create a sub menu containing all actions
                            popup = new QMenu();
                            connect(popup, SIGNAL(activated(int)),
                                    this, SLOT(startService(int)));

                            QPushButton* button = new QPushButton(submenuName, m_actionBox);
                            button->setFlat(true);
                            button->setMenu(popup);
                            button->show();
                        }

                        QList<KDesktopFileActions::Service> userServices =
                            KDesktopFileActions::userDefinedServices(*dirIt + *entryIt, true);

                        // iterate through all actions and add them to a widget
                        QList<KDesktopFileActions::Service>::Iterator serviceIt;
                        for (serviceIt = userServices.begin(); serviceIt != userServices.end(); ++serviceIt) {
                            KDesktopFileActions::Service service = (*serviceIt);
                            if (popup == 0) {
                                ServiceButton* button = new ServiceButton(KIcon(service.m_strIcon),
                                                        service.m_strName,
                                                        m_actionBox,
                                                        actionsIndex);
                                connect(button, SIGNAL(requestServiceStart(int)),
                                        this, SLOT(startService(int)));
                                button->show();
                            } else {
                                popup->insertItem(KIcon(service.m_strIcon), service.m_strName, actionsIndex);
                            }

                            m_actionsVector.append(service);
                            ++actionsIndex;
                        }
                    }
                }
            }
        }
    }
}



ServiceButton::ServiceButton(const QIcon& icon,
                             const QString& text,
                             QWidget* parent,
                             int index) :
        QPushButton(icon, text, parent),
        m_hover(false),
        m_index(index)
{
    setEraseColor(palette().brush(QPalette::Background).color());
    setFocusPolicy(Qt::NoFocus);
    connect(this, SIGNAL(released()),
            this, SLOT(slotReleased()));
}

ServiceButton::~ServiceButton()
{}

void ServiceButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    const int buttonWidth  = width();
    const int buttonHeight = height();

    QColor backgroundColor;
    QColor foregroundColor;
    if (m_hover) {
        backgroundColor = KGlobalSettings::highlightColor();
        foregroundColor = KGlobalSettings::highlightedTextColor();
    } else {
        backgroundColor = palette().brush(QPalette::Background).color();
        foregroundColor = KGlobalSettings::buttonTextColor();
    }

    // draw button background
    painter.setPen(Qt::NoPen);
    painter.setBrush(backgroundColor);
    painter.drawRect(0, 0, buttonWidth, buttonHeight);

    const int spacing = KDialog::spacingHint();

    // draw icon
    int x = spacing;
    const int y = (buttonHeight - K3Icon::SizeSmall) / 2;
    const QIcon &set = icon();
    if (!set.isNull()) {
        painter.drawPixmap(x, y, set.pixmap(QIcon::Small, QIcon::Normal));
    }
    x += K3Icon::SizeSmall + spacing;

    // draw text
    painter.setPen(foregroundColor);

    const int textWidth = buttonWidth - x;
    QFontMetrics fontMetrics(font());
    const bool clipped = fontMetrics.width(text()) >= textWidth;
    //const int align = clipped ? Qt::AlignVCenter : Qt::AlignCenter;
    painter.drawText(QRect(x, 0, textWidth, buttonHeight), Qt::AlignVCenter, text());

    if (clipped) {
        // Blend the right area of the text with the background, as the
        // text is clipped.
        // TODO #1: use alpha blending in Qt4 instead of drawing the text that often
        // TODO #2: same code as in UrlNavigatorButton::drawButton() -> provide helper class?
        const int blendSteps = 16;

        QColor blendColor(backgroundColor);
        const int redInc   = (foregroundColor.red()   - backgroundColor.red())   / blendSteps;
        const int greenInc = (foregroundColor.green() - backgroundColor.green()) / blendSteps;
        const int blueInc  = (foregroundColor.blue()  - backgroundColor.blue())  / blendSteps;
        for (int i = 0; i < blendSteps; ++i) {
            painter.setClipRect(QRect(x + textWidth - i, 0, 1, buttonHeight));
            painter.setPen(blendColor);
            painter.drawText(QRect(x, 0, textWidth, buttonHeight), Qt::AlignVCenter, text());

            blendColor.setRgb(blendColor.red()   + redInc,
                              blendColor.green() + greenInc,
                              blendColor.blue()  + blueInc);
        }
    }
}

void ServiceButton::enterEvent(QEvent* event)
{
    QPushButton::enterEvent(event);
    m_hover = true;
    update();
}

void ServiceButton::leaveEvent(QEvent* event)
{
    QPushButton::leaveEvent(event);
    m_hover = false;
    update();
}

void ServiceButton::slotReleased()
{
    emit requestServiceStart(m_index);
}

#include "infosidebarpage.moc"
