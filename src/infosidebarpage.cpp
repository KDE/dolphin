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

#include <config-nepomuk.h>

#include <kfileplacesmodel.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/previewjob.h>
#include <kfileitem.h>
#include <kdialog.h>
#include <kglobalsettings.h>
#include <kfilemetainfo.h>
#include <kseparator.h>
#include <kiconloader.h>

#include <QEvent>
#include <QInputDialog>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QStyleOptionMenuItem>
#include <QTimer>
#include <QVBoxLayout>

#include "dolphinsettings.h"
#include "metadatawidget.h"
#include "metatextlabel.h"
#include "pixmapviewer.h"

class InfoSeparator : public QWidget
{
public:
    InfoSeparator(QWidget* parent);
    virtual ~InfoSeparator();

protected:
    virtual void paintEvent(QPaintEvent* event);
};

InfoSeparator::InfoSeparator(QWidget* parent) :
    QWidget(parent)
{
    setMinimumSize(0, 8);
}

InfoSeparator::~InfoSeparator()
{
}

void InfoSeparator::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    QStyleOptionMenuItem option;
    option.initFrom(this);
    option.menuItemType = QStyleOptionMenuItem::Separator;
    style()->drawControl(QStyle::CE_MenuItem, &option, &painter, this);
}

InfoSidebarPage::InfoSidebarPage(QWidget* parent) :
    SidebarPage(parent),
    m_pendingPreview(false),
    m_shownUrl(),
    m_urlCandidate(),
    m_fileItem(),
    m_nameLabel(0),
    m_preview(0),
    m_metaDataWidget(0),
    m_metaTextLabel(0)
{
    const int spacing = KDialog::spacingHint();

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(slotTimeout()));

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setSpacing(spacing);

    // name
    m_nameLabel = new QLabel(this);
    QFont font = m_nameLabel->font();
    font.setBold(true);
    m_nameLabel->setFont(font);
    m_nameLabel->setAlignment(Qt::AlignHCenter);
    m_nameLabel->setWordWrap(true);

    // preview
    m_preview = new PixmapViewer(this);
    m_preview->setMinimumWidth(KIconLoader::SizeEnormous + KIconLoader::SizeMedium);
    m_preview->setMinimumHeight(KIconLoader::SizeEnormous);

    if (MetaDataWidget::metaDataAvailable()) {
        // rating, comment and tags
        m_metaDataWidget = new MetaDataWidget(this);
    }

    // general meta text information
    m_metaTextLabel = new MetaTextLabel(this);
    m_metaTextLabel->setMinimumWidth(spacing);

    layout->addWidget(m_nameLabel);
    layout->addWidget(new InfoSeparator(this));
    layout->addWidget(m_preview);
    layout->addWidget(new InfoSeparator(this));
    if (m_metaDataWidget != 0) {
        layout->addWidget(m_metaDataWidget);
        layout->addWidget(new InfoSeparator(this));
    }
    layout->addWidget(m_metaTextLabel);

    // ensure that widgets in the information side bar are aligned towards the top
    layout->addStretch(1);
    setLayout(layout);
}

InfoSidebarPage::~InfoSidebarPage()
{
}

QSize InfoSidebarPage::sizeHint() const
{
    QSize size = SidebarPage::sizeHint();
    size.setWidth(minimumSizeHint().width());
    return size;
}

void InfoSidebarPage::setUrl(const KUrl& url)
{
    SidebarPage::setUrl(url);
    if (url.isValid() && !m_shownUrl.equals(url, KUrl::CompareWithoutTrailingSlash)) {
        cancelRequest();
        m_shownUrl = url;
        showItemInfo();
    }
}

void InfoSidebarPage::setSelection(const KFileItemList& selection)
{
    SidebarPage::setSelection(selection);

    const int count = selection.count();
    if (count == 0) {
        m_shownUrl = url();
        showItemInfo();
    } else {
        if ((count == 1) && !selection.first().url().isEmpty()) {
            m_urlCandidate = selection.first().url();
        }
        m_timer->start(TimerDelay);
    }
}

void InfoSidebarPage::requestDelayedItemInfo(const KFileItem& item)
{
    cancelRequest();

    m_fileItem = KFileItem();
    if (item.isNull()) {
        // The cursor is above the viewport. If files are selected,
        // show information regarding the selection.
        if (selection().size() > 0) {
            m_timer->start(TimerDelay);
        }
    } else if (!item.url().isEmpty()) {
        m_urlCandidate = item.url();
        m_fileItem = item;
        m_timer->start(TimerDelay);
    }
}

void InfoSidebarPage::showEvent(QShowEvent* event)
{
    SidebarPage::showEvent(event);
    if (!event->spontaneous()) {
        showItemInfo();
    }
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

    // try to increase the preview as large as possible
    m_preview->setSizeHint(QSize(maxWidth, maxWidth));
    m_urlCandidate = m_shownUrl; // reset the URL candidate if a resizing is done
    m_timer->start(TimerDelay);

    SidebarPage::resizeEvent(event);
}

void InfoSidebarPage::showItemInfo()
{
    if (!isVisible()) {
        return;
    }

    cancelRequest();

    const KFileItemList& selectedItems = selection();
    const KUrl file = (!m_fileItem.isNull() || selectedItems.isEmpty()) ? m_shownUrl : selectedItems[0].url();
    if (!file.isValid()) {
        return;
    }

    const int selectionCount = selectedItems.count();
    if (m_fileItem.isNull() && (selectionCount > 1)) {
        KIconLoader iconLoader;
        QPixmap icon = iconLoader.loadIcon("dialog-information",
                                           KIconLoader::NoGroup,
                                           KIconLoader::SizeEnormous);
        m_preview->setPixmap(icon);
        m_nameLabel->setText(i18ncp("@info", "%1 item selected", "%1 items selected", selectionCount));
    } else if (!applyPlace(file)) {
        // try to get a preview pixmap from the item...
        KUrl::List list;
        list.append(file);

        m_pendingPreview = true;
        m_preview->setPixmap(QPixmap());

        KIO::PreviewJob* job = KIO::filePreview(list,
                                                m_preview->width(),
                                                m_preview->height(),
                                                0,
                                                0,
                                                true,
                                                false);
        job->setIgnoreMaximumSize(true);

        connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
                this, SLOT(showPreview(const KFileItem&, const QPixmap&)));
        connect(job, SIGNAL(failed(const KFileItem&)),
                this, SLOT(showIcon(const KFileItem&)));

        m_nameLabel->setText( !m_fileItem.isNull() && selectedItems.isEmpty() ? m_fileItem.text() : selectedItems[0].text() );
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
    if (!applyPlace(item.url())) {
        m_preview->setPixmap(item.pixmap(KIconLoader::SizeEnormous));
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

bool InfoSidebarPage::applyPlace(const KUrl& url)
{
    KFilePlacesModel* placesModel = DolphinSettings::instance().placesModel();
    int count = placesModel->rowCount();

    for (int i = 0; i < count; ++i) {
        QModelIndex index = placesModel->index(i, 0);

        if (url.equals(placesModel->url(index), KUrl::CompareWithoutTrailingSlash)) {
            m_nameLabel->setText(placesModel->text(index));
            m_preview->setPixmap(placesModel->icon(index).pixmap(128, 128));
            return true;
        }
    }

    return false;
}

void InfoSidebarPage::cancelRequest()
{
    m_timer->stop();
}

void InfoSidebarPage::showMetaInfo()
{
    m_metaTextLabel->clear();

    const KFileItemList& selectedItems = selection();
    if ((selectedItems.size() <= 1) || !m_fileItem.isNull()) {
        KFileItem fileItem;
        if (m_fileItem.isNull()) {
            // no pending request is ongoing
            const KUrl url = (selectedItems.size() == 1) ? selectedItems.first().url() : m_shownUrl;
            fileItem = KFileItem(KFileItem::Unknown, KFileItem::Unknown, url);
            fileItem.refresh();
        } else {
            fileItem = m_fileItem;
        }

        if (fileItem.isDir()) {
            m_metaTextLabel->add(i18nc("@label", "Type:"), i18nc("@label", "Folder"));
            m_metaTextLabel->add(i18nc("@label", "Modified:"), fileItem.timeString());
        } else {
            m_metaTextLabel->add(i18nc("@label", "Type:"), fileItem.mimeComment());

            m_metaTextLabel->add(i18nc("@label", "Size:"), KIO::convertSize(fileItem.size()));
            m_metaTextLabel->add(i18nc("@label", "Modified:"), fileItem.timeString());

            // TODO: See convertMetaInfo below, find a way to display only interesting information
            // in a readable way
            const KFileMetaInfo::WhatFlags flags = KFileMetaInfo::Fastest |
                                                   KFileMetaInfo::TechnicalInfo |
                                                   KFileMetaInfo::ContentInfo |
                                                   KFileMetaInfo::Thumbnail;
            const QString path = fileItem.url().url();
            const KFileMetaInfo fileMetaInfo(path, QString(), flags);
            if (fileMetaInfo.isValid()) {
                const QHash<QString, KFileMetaInfoItem>& items = fileMetaInfo.items();
                QHash<QString, KFileMetaInfoItem>::const_iterator it = items.constBegin();
                const QHash<QString, KFileMetaInfoItem>::const_iterator end = items.constEnd();
                QString labelText;
                while (it != end) {
                    const KFileMetaInfoItem& metaInfoItem = it.value();
                    const QVariant& value = metaInfoItem.value();
                    if (value.isValid() && convertMetaInfo(metaInfoItem.name(), labelText)) {
                        m_metaTextLabel->add(labelText, value.toString());
                    }
                    ++it;
                }
            }
        }

        if (m_metaDataWidget != 0) {
            m_metaDataWidget->setFile(fileItem.targetUrl());
        }
    } else {
        if (m_metaDataWidget != 0) {
            KUrl::List urls;
            foreach (const KFileItem& item, selectedItems) {
                urls.append(item.targetUrl());
            }
            m_metaDataWidget->setFiles(urls);
        }

        unsigned long int totalSize = 0;
        foreach (const KFileItem& item, selectedItems) {
            // Only count the size of files, not dirs to match what
            // DolphinViewContainer::selectionStatusBarText() does.
            if (!item.isDir() && !item.isLink()) {
                totalSize += item.size();
            }
        }
        m_metaTextLabel->add(i18nc("@label", "Total size:"), KIO::convertSize(totalSize));
    }
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
