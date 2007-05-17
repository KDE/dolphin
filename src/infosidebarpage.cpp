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

#include <QDir>
#include <QEvent>
#include <QFontMetrics>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>

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
    m_multipleSelection(false), //TODO: check if I'm needed
    m_pendingPreview(false),
    m_timer(0),
    m_preview(0),
    m_name(0),
    m_infos(0),
    m_metadataWidget(0)
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
    m_name->setWordWrap(true);

    // general information
    m_infos = new QLabel(this);
    m_infos->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_infos->setTextFormat(Qt::RichText);

    if (MetaDataWidget::metaDataAvailable()) {
        m_metadataWidget = new MetaDataWidget(this);
    }

    layout->addItem(new QSpacerItem(spacing, spacing, QSizePolicy::Preferred, QSizePolicy::Fixed));
    layout->addWidget(m_preview);
    layout->addWidget(m_name);
    layout->addWidget(new KSeparator(this));
    layout->addWidget(m_infos);
    layout->addWidget(new KSeparator(this));
    if (m_metadataWidget) {
        layout->addWidget(m_metadataWidget);
        layout->addWidget(new KSeparator(this));
    }
    // ensure that widgets in the information side bar are aligned towards the top
    layout->addStretch(1);
    setLayout(layout);
}

InfoSidebarPage::~InfoSidebarPage()
{
}

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
    cancelRequest();
    SidebarPage::setSelection(selection);
    m_multipleSelection = (selection.size() > 1);
    showItemInfo();
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

void InfoSidebarPage::showEvent(QShowEvent* event)
{
    SidebarPage::showEvent(event);
    showItemInfo();
}

void InfoSidebarPage::resizeEvent(QResizeEvent* event)
{
    // If the item name cannot get wrapped, the maximum width of
    // the label is increased, so that the width of the information sidebar
    // gets increased. To prevent this, the maximum width is adjusted to
    // the current width of the sidebar.
    m_name->setMaximumWidth(event->size().width() - KDialog::spacingHint() * 4);
    SidebarPage::resizeEvent(event);
}

void InfoSidebarPage::showItemInfo()
{
    if (!isVisible()) {
        return;
    }

    cancelRequest();

    const KFileItemList& selectedItems = selection();

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
                this, SLOT(showPreview(const KFileItem&, const QPixmap&)));
        connect(job, SIGNAL(failed(const KFileItem&)),
                this, SLOT(showIcon(const KFileItem&)));

        QString text("<b>");
        text.append(file.fileName());
        text.append("</b>");
        m_name->setText(text);
    }

    createMetaInfo();
}

void InfoSidebarPage::slotTimeout()
{
    m_shownUrl = m_urlCandidate;
    showItemInfo();
}

void InfoSidebarPage::showIcon(const KFileItem& item)
{
    m_pendingPreview = false;
    if (!applyBookmark(item.url())) {
        m_preview->setPixmap(item.pixmap(K3Icon::SizeEnormous));
    }
}

void InfoSidebarPage::showPreview(const KFileItem& item,
                                  const QPixmap& pixmap)
{
    Q_UNUSED(item);
    if (m_pendingPreview) {
        m_preview->setPixmap(pixmap);
        m_pendingPreview = false;
    }
}

bool InfoSidebarPage::applyBookmark(const KUrl& url)
{
    KFilePlacesModel* placesModel = DolphinSettings::instance().placesModel();
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
    const KFileItemList& selectedItems = selection();
    if (selectedItems.size() == 0) {
        KFileItem fileItem(S_IFDIR, KFileItem::Unknown, m_shownUrl);
        fileItem.refresh();

        if (fileItem.isDir()) {
            addInfoLine(i18n("Type:"), i18n("Directory"));
        }
        if (MetaDataWidget::metaDataAvailable()) {
            m_metadataWidget->setFile(fileItem.url());
        }
    } else if (selectedItems.count() == 1) {
        KFileItem* fileItem = selectedItems.at(0);
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
        if (MetaDataWidget::metaDataAvailable()) {
            m_metadataWidget->setFile(fileItem->url());
        }
    } else {
        if (MetaDataWidget::metaDataAvailable()) {
            m_metadataWidget->setFiles(selectedItems.urlList());
        }

        unsigned long int totSize = 0;
        foreach(KFileItem* item, selectedItems) {
            totSize += item->size(); //FIXME what to do with directories ? (same with the one-item-selected-code), item->size() does not return the size of the content : not very instinctive for users
        }
        addInfoLine(i18n("Total size:"), KIO::convertSize(totSize));
    }
    endInfoLines();
}

void InfoSidebarPage::beginInfoLines()
{
    m_infoLines = QString();
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
    if (!m_infoLines.isEmpty()) {
        m_infoLines += "<br/>";
    }
    m_infoLines += QString("<b>%1</b> %2").arg(labelText).arg(infoText);
}

#include "infosidebarpage.moc"
