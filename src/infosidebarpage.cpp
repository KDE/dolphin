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

#include "infosidebarpage.h"

#include <config-kmetadata.h>

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

#include <QEvent>
#include <QInputDialog>
#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>

#include "dolphinsettings.h"
#include "metadatawidget.h"
#include "pixmapviewer.h"

InfoSidebarPage::InfoSidebarPage(QWidget* parent) :
    SidebarPage(parent),
    m_pendingPreview(false),
    m_timer(0),
    m_preview(0),
    m_nameLabel(0),
    m_infoLabel(0),
    m_metadataWidget(0)
{
    const int spacing = KDialog::spacingHint();

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(slotTimeout()));

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setSpacing(spacing);

    // preview
    m_preview = new PixmapViewer(this);
    m_preview->setMinimumWidth(K3Icon::SizeEnormous);
    m_preview->setFixedHeight(K3Icon::SizeEnormous);

    // name
    m_nameLabel = new QLabel(this);
    m_nameLabel->setTextFormat(Qt::RichText);
    m_nameLabel->setAlignment(m_nameLabel->alignment() | Qt::AlignHCenter);
    m_nameLabel->setWordWrap(true);

    // general information
    m_infoLabel = new QLabel(this);
    m_infoLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_infoLabel->setTextFormat(Qt::RichText);

    if (MetaDataWidget::metaDataAvailable()) {
        m_metadataWidget = new MetaDataWidget(this);
    }

    layout->addItem(new QSpacerItem(spacing, spacing, QSizePolicy::Preferred, QSizePolicy::Fixed));
    layout->addWidget(m_preview);
    layout->addWidget(m_nameLabel);
    layout->addWidget(new KSeparator(this));
    layout->addWidget(m_infoLabel);
    layout->addWidget(new KSeparator(this));
    if (m_metadataWidget) {
        layout->addWidget(m_metadataWidget);
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
    if (url.isValid() && !m_shownUrl.equals(url, KUrl::CompareWithoutTrailingSlash)) {
        cancelRequest();
        m_shownUrl = url;
        showItemInfo();
    }
}

void InfoSidebarPage::setSelection(const KFileItemList& selection)
{
    SidebarPage::setSelection(selection);
    m_timer->start(TimerDelay);
}

void InfoSidebarPage::requestDelayedItemInfo(const KUrl& url)
{
    cancelRequest();

    if (!url.isEmpty() && (selection().size() <= 1)) {
        m_urlCandidate = url;
        m_timer->start(TimerDelay);
    }
}

void InfoSidebarPage::showEvent(QShowEvent* event)
{
    SidebarPage::showEvent(event);
    if (event->spontaneous()) {
        return;
    }
    showItemInfo();
}

void InfoSidebarPage::resizeEvent(QResizeEvent* event)
{
    // If the text inside the name label or the info label cannot
    // get wrapped, then the maximum width of the label is increased
    // so that the width of the information sidebar gets increased.
    // To prevent this, the maximum width is adjusted to
    // the current width of the sidebar.
    const int maxWidth = event->size().width() - KDialog::spacingHint() * 4;
    m_nameLabel->setMaximumWidth(maxWidth);
    m_infoLabel->setMaximumWidth(maxWidth);
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
    const int itemCount = selectedItems.count();
    if (selectedItems.count() == 0) {
        file = m_shownUrl;
    } else {
        file = selectedItems[0]->url();
    }
    if (itemCount > 1) {
        KIconLoader iconLoader;
        QPixmap icon = iconLoader.loadIcon("exec",
                                           K3Icon::NoGroup,
                                           K3Icon::SizeEnormous);
        m_preview->setPixmap(icon);
        m_nameLabel->setText(i18n("%1 items selected", selectedItems.count()));
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
        m_nameLabel->setText(text);
    }

    showMetaInfo();
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
            m_nameLabel->setText(text);

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

void InfoSidebarPage::showMetaInfo()
{
    QString text;

    const KFileItemList& selectedItems = selection();
    if (selectedItems.size() <= 1) {
        KFileItem fileItem(S_IFDIR, KFileItem::Unknown, m_shownUrl);
        fileItem.refresh();

        if (fileItem.isDir()) {
            addInfoLine(text, i18n("Type:"), i18n("Folder"));
        } else {
            addInfoLine(text, i18n("Type:"), fileItem.mimeComment());

            QString sizeText(KIO::convertSize(fileItem.size()));
            addInfoLine(text, i18n("Size:"), sizeText);
            addInfoLine(text, i18n("Modified:"), fileItem.timeString());

            // TODO: deactivate showing additional meta information, as the system
            // hangs when retrieving the meta information of a zipped file
            /*const KFileMetaInfo metaInfo(fileItem.url());
            if (metaInfo.isValid()) {
                const QHash<QString, KFileMetaInfoItem>& items = metaInfo.items();
                QHash<QString, KFileMetaInfoItem>::const_iterator it = items.constBegin();
                const QHash<QString, KFileMetaInfoItem>::const_iterator end = items.constEnd();
                QString labelText;
                while (it != end) {
                    const KFileMetaInfoItem& metaInfo = it.value();
                    const QVariant& value = metaInfo.value();
                    if (value.isValid() && convertMetaInfo(metaInfo.name(), labelText)) {
                        addInfoLine(text, labelText, value.toString());
                    }
                    ++it;
                }
            }*/
        }

        if (MetaDataWidget::metaDataAvailable()) {
            m_metadataWidget->setFile(fileItem.url());
        }
    } else {
        if (MetaDataWidget::metaDataAvailable()) {
            m_metadataWidget->setFiles(selectedItems.urlList());
        }

        unsigned long int totalSize = 0;
        foreach (KFileItem* item, selectedItems) {
            // TODO: what to do with directories (same with the one-item-selected-code)?,
            // item->size() does not return the size of the content : not very instinctive for users
            totalSize += item->size();
        }
        addInfoLine(text, i18n("Total size:"), KIO::convertSize(totalSize));
    }
    m_infoLabel->setText(text);
}

void InfoSidebarPage::addInfoLine(QString& text,
                                  const QString& labelText,
                                  const QString& infoText)
{
    if (!infoText.isEmpty()) {
        text += "<br/>";
    }
    text += QString("<b>%1</b> %2").arg(labelText).arg(infoText);
}

bool InfoSidebarPage::convertMetaInfo(const QString& key, QString& text) const
{
    // TODO: This code prevents that interesting meta information might be hidden
    // and only bypasses the current problem that not all the meta information should
    // be shown to the user. Check whether it's possible with Nepomuk to show
    // all "user relevant" information in a readable way...

    struct MetaKey {
        const char* key;
        const char* text;
    };

    // sorted list of keys, where its data should be shown
    static const MetaKey keys[] = {
        { "audio.album", "Album:" },
        { "audio.artist", "Artist:" },
        { "audio.title", "Title:" },
    };

    // do a binary search for the key...
    int top = 0;
    int bottom = sizeof(keys) / sizeof(MetaKey) - 1;
    while (top <= bottom) {
        const int middle = (top + bottom) / 2;
        const int result = key.compare(keys[middle].key);
        if (result < 0) {
            bottom = middle - 1;
        } else if (result > 0) {
            top = middle + 1;
        } else {
            text = keys[middle].text;
            return true;
        }
    }

    return false;
}

#include "infosidebarpage.moc"
