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

#include <kdialog.h>
#include <kdirnotify.h>
#include <kfileplacesmodel.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/previewjob.h>
#include <kfileitem.h>
#include <kglobalsettings.h>
#include <kfilemetainfo.h>
#include <kiconeffect.h>
#include <kseparator.h>
#include <kiconloader.h>

#include <QEvent>
#include <QInputDialog>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>

#include "dolphinsettings.h"
#include "metadatawidget.h"
#include "metatextlabel.h"
#include "pixmapviewer.h"

InfoSidebarPage::InfoSidebarPage(QWidget* parent) :
    SidebarPage(parent),
    m_initialized(false),
    m_pendingPreview(false),
    m_infoTimer(0),
    m_outdatedPreviewTimer(0),
    m_shownUrl(),
    m_urlCandidate(),
    m_fileItem(),
    m_selection(),
    m_nameLabel(0),
    m_preview(0),
    m_metaDataWidget(0),
    m_metaTextLabel(0)
{
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
    if (url.isValid() && !isEqualToShownUrl(url)) {
        if (m_initialized) {
            cancelRequest();
            m_shownUrl = url;
            showItemInfo();
        } else {
            m_shownUrl = url;
        }
    }
}

void InfoSidebarPage::setSelection(const KFileItemList& selection)
{
    if (!m_initialized) {
        return;
    }

    if ((selection.count() == 0) && (m_selection.count() == 0)) {
        // The selection has not really changed, only the current index.
        // QItemSelectionModel emits a signal in this case and it is less
        // expensive doing the check this way instead of patching
        // DolphinView::emitSelectionChanged().
        return;
    }

    m_selection = selection;

    const int count = selection.count();
    if (count == 0) {
        if (!isEqualToShownUrl(url())) {
            m_shownUrl = url();
            showItemInfo();
        }
    } else {
        if ((count == 1) && !selection.first().url().isEmpty()) {
            m_urlCandidate = selection.first().url();
        }
        m_infoTimer->start();
    }
}

void InfoSidebarPage::requestDelayedItemInfo(const KFileItem& item)
{
    if (!m_initialized) {
        return;
    }

    cancelRequest();

    m_fileItem = KFileItem();
    if (item.isNull()) {
        // The cursor is above the viewport. If files are selected,
        // show information regarding the selection.
        if (m_selection.size() > 0) {
            m_pendingPreview = false;
            m_infoTimer->start();
        }
    } else {
        const KUrl url = item.url();
        if (url.isValid() && !isEqualToShownUrl(url)) {
            m_urlCandidate = item.url();
            m_fileItem = item;
            m_infoTimer->start();
        }    
    }
}

void InfoSidebarPage::showEvent(QShowEvent* event)
{
    SidebarPage::showEvent(event);
    if (!event->spontaneous()) {
        if (!m_initialized) {
            // do a delayed initialization so that no performance
            // penalty is given when Dolphin is started with a closed
            // Information Panel
            init();
        }
        showItemInfo();
    }
}

void InfoSidebarPage::resizeEvent(QResizeEvent* event)
{
    if (m_initialized) {
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
        m_infoTimer->start();
    }

    SidebarPage::resizeEvent(event);
}

void InfoSidebarPage::showItemInfo()
{
    if (!isVisible()) {
        return;
    }

    cancelRequest();

    if (showMultipleSelectionInfo()) {
        KIconLoader iconLoader;
        QPixmap icon = iconLoader.loadIcon("dialog-information",
                                           KIconLoader::NoGroup,
                                           KIconLoader::SizeEnormous);
        m_preview->setPixmap(icon);
        m_nameLabel->setText(i18ncp("@info", "%1 item selected", "%1 items selected",  m_selection.count()));
        m_shownUrl = KUrl();
    } else {
        const KFileItem item = fileItem();
        const KUrl itemUrl = item.url();
        if (!applyPlace(itemUrl)) {
            // try to get a preview pixmap from the item...
            m_pendingPreview = true;

            // Mark the currently shown preview as outdated. This is done
            // with a small delay to prevent a flickering when the next preview
            // can be shown within a short timeframe.
            m_outdatedPreviewTimer->start();

            KIO::PreviewJob* job = KIO::filePreview(KUrl::List() << itemUrl,
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

            m_nameLabel->setText(itemUrl.fileName());
        }
    }

    showMetaInfo();
}

void InfoSidebarPage::slotInfoTimeout()
{
    m_shownUrl = m_urlCandidate;
    showItemInfo();
}

void InfoSidebarPage::markOutdatedPreview()
{
    KIconEffect iconEffect;
    QPixmap disabledPixmap = iconEffect.apply(m_preview->pixmap(),
                                              KIconLoader::Desktop,
                                              KIconLoader::DisabledState);
    m_preview->setPixmap(disabledPixmap);
}

void InfoSidebarPage::showIcon(const KFileItem& item)
{
    m_outdatedPreviewTimer->stop();
    m_pendingPreview = false;
    if (!applyPlace(item.url())) {
        m_preview->setPixmap(item.pixmap(KIconLoader::SizeEnormous));
    }
}

void InfoSidebarPage::showPreview(const KFileItem& item,
                                  const QPixmap& pixmap)
{
    m_outdatedPreviewTimer->stop();

    Q_UNUSED(item);
    if (m_pendingPreview) {
        m_preview->setPixmap(pixmap);
        m_pendingPreview = false;
    }
}

void InfoSidebarPage::slotFileRenamed(const QString& source, const QString& dest)
{
    if (m_shownUrl == KUrl(source)) {
        // the currently shown file has been renamed, hence update the item information
        // for the renamed file
        KFileItem item(KFileItem::Unknown, KFileItem::Unknown, KUrl(dest));
        requestDelayedItemInfo(item);
    }
}

void InfoSidebarPage::slotFilesAdded(const QString& directory)
{
    if (m_shownUrl == KUrl(directory)) {
        // If the 'trash' icon changes because the trash has been emptied or got filled,
        // the signal filesAdded("trash:/") will be emitted.
        KFileItem item(KFileItem::Unknown, KFileItem::Unknown, KUrl(directory));
        requestDelayedItemInfo(item);
    }
}

void InfoSidebarPage::slotFilesChanged(const QStringList& files)
{
    foreach (const QString& fileName, files) {
        if (m_shownUrl == KUrl(fileName)) {
            showItemInfo();
            break;
        }
    }
}

void InfoSidebarPage::slotFilesRemoved(const QStringList& files)
{
    foreach (const QString& fileName, files) {
        if (m_shownUrl == KUrl(fileName)) {
            // the currently shown item has been removed, show
            // the parent directory as fallback
            m_shownUrl = url();
            showItemInfo();
            break;
        }
    }
}

void InfoSidebarPage::slotEnteredDirectory(const QString& directory)
{
    if (m_shownUrl == KUrl(directory)) {
        KFileItem item(KFileItem::Unknown, KFileItem::Unknown, KUrl(directory));
        requestDelayedItemInfo(item);
    }
}

void InfoSidebarPage::slotLeftDirectory(const QString& directory)
{
    if (m_shownUrl == KUrl(directory)) {
        // The signal 'leftDirectory' is also emitted when a media
        // has been unmounted. In this case no directory change will be
        // done in Dolphin, but the Information Panel must be updated to
        // indicate an invalid directory.
        m_shownUrl = url();
        showItemInfo();
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
    m_infoTimer->stop();
}

void InfoSidebarPage::showMetaInfo()
{
    m_metaTextLabel->clear();

    if (showMultipleSelectionInfo()) {
        if (m_metaDataWidget != 0) {
            KUrl::List urls;
            foreach (const KFileItem& item, m_selection) {
                urls.append(item.targetUrl());
            }
            m_metaDataWidget->setFiles(urls);
        }

        quint64 totalSize = 0;
        foreach (const KFileItem& item, m_selection) {
            // Only count the size of files, not dirs to match what
            // DolphinViewContainer::selectionStatusBarText() does.
            if (!item.isDir() && !item.isLink()) {
                totalSize += item.size();
            }
        }
        m_metaTextLabel->add(i18nc("@label", "Total size:"), KIO::convertSize(totalSize));
    } else {
        const KFileItem item = fileItem();
        if (item.isDir()) {
            m_metaTextLabel->add(i18nc("@label", "Type:"), i18nc("@label", "Folder"));
            m_metaTextLabel->add(i18nc("@label", "Modified:"), item.timeString());
        } else {
            m_metaTextLabel->add(i18nc("@label", "Type:"), item.mimeComment());

            m_metaTextLabel->add(i18nc("@label", "Size:"), KIO::convertSize(item.size()));
            m_metaTextLabel->add(i18nc("@label", "Modified:"), item.timeString());

            if (item.isLocalFile()) {
                // TODO: See convertMetaInfo below, find a way to display only interesting information
                // in a readable way
                const KFileMetaInfo::WhatFlags flags = KFileMetaInfo::Fastest |
                                                       KFileMetaInfo::TechnicalInfo |
                                                       KFileMetaInfo::ContentInfo;
                const QString path = item.url().path();
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
        }

        if (m_metaDataWidget != 0) {
            m_metaDataWidget->setFile(item.targetUrl());
        }
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

KFileItem InfoSidebarPage::fileItem() const
{
    if (!m_fileItem.isNull()) {
        return m_fileItem;
    }

    if (!m_selection.isEmpty()) {
        Q_ASSERT(m_selection.count() == 1);
        return m_selection.first();
    }

    // no item is hovered and no selection has been done: provide
    // an item for the directory represented by m_shownUrl
    KFileItem item(KFileItem::Unknown, KFileItem::Unknown, m_shownUrl);
    item.refresh();
    return item;
}

bool InfoSidebarPage::showMultipleSelectionInfo() const
{
    return m_fileItem.isNull() && (m_selection.count() > 1);
}

bool InfoSidebarPage::isEqualToShownUrl(const KUrl& url) const
{
    return m_shownUrl.equals(url, KUrl::CompareWithoutTrailingSlash);
}

void InfoSidebarPage::init()
{
    const int spacing = KDialog::spacingHint();

    m_infoTimer = new QTimer(this);
    m_infoTimer->setInterval(300);
    m_infoTimer->setSingleShot(true);
    connect(m_infoTimer, SIGNAL(timeout()),
            this, SLOT(slotInfoTimeout()));

    // Initialize timer for disabling an outdated preview with a small
    // delay. This prevents flickering if the new preview can be generated
    // within a very small timeframe.
    m_outdatedPreviewTimer = new QTimer(this);
    m_outdatedPreviewTimer->setInterval(300);
    m_outdatedPreviewTimer->setSingleShot(true);
    connect(m_outdatedPreviewTimer, SIGNAL(timeout()),
            this, SLOT(markOutdatedPreview()));

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setSpacing(spacing);

    // name
    m_nameLabel = new QLabel(this);
    QFont font = m_nameLabel->font();
    font.setBold(true);
    m_nameLabel->setFont(font);
    m_nameLabel->setAlignment(Qt::AlignHCenter);
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // preview
    m_preview = new PixmapViewer(this);
    m_preview->setMinimumWidth(KIconLoader::SizeEnormous + KIconLoader::SizeMedium);
    m_preview->setMinimumHeight(KIconLoader::SizeEnormous);

    if (MetaDataWidget::metaDataAvailable()) {
        // rating, comment and tags
        m_metaDataWidget = new MetaDataWidget(this);
        m_metaDataWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

    // general meta text information
    m_metaTextLabel = new MetaTextLabel(this);
    m_metaTextLabel->setMinimumWidth(spacing);
    m_metaTextLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    layout->addWidget(m_nameLabel);
    layout->addWidget(new KSeparator(this));
    layout->addWidget(m_preview);
    layout->addWidget(new KSeparator(this));
    if (m_metaDataWidget != 0) {
        layout->addWidget(m_metaDataWidget);
        layout->addWidget(new KSeparator(this));
    }
    layout->addWidget(m_metaTextLabel);

    // ensure that widgets in the information side bar are aligned towards the top
    layout->addStretch(1);
    setLayout(layout);

    org::kde::KDirNotify* dirNotify = new org::kde::KDirNotify(QString(), QString(),
                                                               QDBusConnection::sessionBus(), this);
    connect(dirNotify, SIGNAL(FileRenamed(QString, QString)), SLOT(slotFileRenamed(QString, QString)));
    connect(dirNotify, SIGNAL(FilesAdded(QString)), SLOT(slotFilesAdded(QString)));
    connect(dirNotify, SIGNAL(FilesChanged(QStringList)), SLOT(slotFilesChanged(QStringList)));
    connect(dirNotify, SIGNAL(FilesRemoved(QStringList)), SLOT(slotFilesRemoved(QStringList)));
    connect(dirNotify, SIGNAL(enteredDirectory(QString)), SLOT(slotEnteredDirectory(QString)));
    connect(dirNotify, SIGNAL(leftDirectory(QString)), SLOT(slotLeftDirectory(QString)));

    m_initialized = true;
}

#include "infosidebarpage.moc"
