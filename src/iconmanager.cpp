/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "iconmanager.h"

#include "dolphinmodel.h"
#include "dolphinsortfilterproxymodel.h"

#include <kiconeffect.h>
#include <kio/previewjob.h>
#include <kdirlister.h>
#include <konqmimedata.h>

#include <QApplication>
#include <QAbstractItemView>
#include <QClipboard>
#include <QColor>
#include <QPainter>
#include <QIcon>

IconManager::IconManager(QAbstractItemView* parent, DolphinSortFilterProxyModel* model) :
    QObject(parent),
    m_showPreview(false),
    m_view(parent),
    m_previewTimer(0),
    m_previewJobs(),
    m_dolphinModel(0),
    m_proxyModel(model),
    m_cutItemsCache(),
    m_previews()
{
    Q_ASSERT(m_view->iconSize().isValid());  // each view must provide its current icon size

    m_dolphinModel = static_cast<DolphinModel*>(m_proxyModel->sourceModel());
    connect(m_dolphinModel->dirLister(), SIGNAL(newItems(const KFileItemList&)),
            this, SLOT(generatePreviews(const KFileItemList&)));

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updateCutItems()));

    m_previewTimer = new QTimer(this);
    connect(m_previewTimer, SIGNAL(timeout()), this, SLOT(dispatchPreviewQueue()));
}

IconManager::~IconManager()
{
    killJobs();
}


void IconManager::setShowPreview(bool show)
{
    if (m_showPreview != show) {
        m_showPreview = show;
        m_cutItemsCache.clear();
        updateCutItems();
        if (show) {
            updatePreviews();
        }
    }
}

void IconManager::updatePreviews()
{
    if (!m_showPreview) {
        return;
    }

    killJobs();
    KFileItemList itemList;

    const int rowCount = m_dolphinModel->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        const QModelIndex index = m_dolphinModel->index(row, 0);
        KFileItem item = m_dolphinModel->itemForIndex(index);
        itemList.append(item);
    }

    generatePreviews(itemList);
}

void IconManager::generatePreviews(const KFileItemList &items)
{
    if (!m_showPreview) {
        return;
    }

    const QRect visibleArea = m_view->viewport()->rect();

    // Order the items in a way that the preview for the visible items
    // is generated first, as this improves the feeled performance a lot.
    KFileItemList orderedItems;
    foreach (KFileItem item, items) {
        const QModelIndex dirIndex = m_dolphinModel->indexForItem(item);
        const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
        const QRect itemRect = m_view->visualRect(proxyIndex);
        if (itemRect.intersects(visibleArea)) {
            orderedItems.insert(0, item);
        } else {
            orderedItems.append(item);
        }
    }

    const QSize size = m_view->iconSize();
    KIO::PreviewJob* job = KIO::filePreview(orderedItems, 128, 128);
    connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
            this, SLOT(addToPreviewQueue(const KFileItem&, const QPixmap&)));
    connect(job, SIGNAL(finished(KJob*)),
            this, SLOT(slotPreviewJobFinished(KJob*)));

    m_previewJobs.append(job);
    m_previewTimer->start(200);
}

void IconManager::addToPreviewQueue(const KFileItem& item, const QPixmap& pixmap)
{
    Preview preview;
    preview.item = item;
    preview.pixmap = pixmap;
    m_previews.append(preview);
}

void IconManager::slotPreviewJobFinished(KJob* job)
{
    const int index = m_previewJobs.indexOf(job);
    m_previewJobs.removeAt(index);
}

void IconManager::updateCutItems()
{
    // restore the icons of all previously selected items to the
    // original state...
    foreach (CutItem cutItem, m_cutItemsCache) {
        const QModelIndex index = m_dolphinModel->indexForUrl(cutItem.url);
        if (index.isValid()) {
            m_dolphinModel->setData(index, QIcon(cutItem.pixmap), Qt::DecorationRole);
        }
    }
    m_cutItemsCache.clear();

    // ... and apply an item effect to all currently cut items
    applyCutItemEffect();
}

void IconManager::dispatchPreviewQueue()
{
    int previewsCount = m_previews.count();
    if (previewsCount > 0) {
        // Applying the previews to the model must be done step by step
        // in larger blocks: Applying a preview immediately when getting the signal
        // 'gotPreview()' from the PreviewJob is too expensive, as a relayout
        // of the view would be triggered for each single preview.

        int dispatchCount = 30;
        if (dispatchCount > m_previews.count()) {
            dispatchCount = m_previews.count();
        }

        for (int i = 0; i < dispatchCount; ++i) {
            const Preview& preview = m_previews.first();
            replaceIcon(preview.item, preview.pixmap);
            m_previews.pop_front();
        }

        previewsCount = m_previews.count();
    }

    const bool workingPreviewJobs = (m_previewJobs.count() > 0);
    if (workingPreviewJobs) {
        // poll for previews as long as not all preview jobs are finished
        m_previewTimer->start(200);
    } else if (previewsCount > 0) {
        // all preview jobs are finished but there are still pending previews
        // in the queue -> poll more aggressively
        m_previewTimer->start(10);
    }
}

void IconManager::replaceIcon(const KFileItem& item, const QPixmap& pixmap)
{
    Q_ASSERT(!item.isNull());
    if (!m_showPreview) {
        // the preview has been canceled in the meantime
        return;
    }

    // check whether the item is part of the directory lister (it is possible
    // that a preview from an old directory lister is received)
    KDirLister* dirLister = m_dolphinModel->dirLister();
    bool isOldPreview = true;
    const KUrl::List dirs = dirLister->directories();
    const QString itemDir = item.url().directory();
    foreach (KUrl url, dirs) {
        if (url.path() == itemDir) {
            isOldPreview = false;
            break;
        }
    }
    if (isOldPreview) {
        return;
    }

    const QModelIndex idx = m_dolphinModel->indexForItem(item);
    if (idx.isValid() && (idx.column() == 0)) {
        QPixmap icon = pixmap;

        const QString mimeType = item.mimetype();
        const QString mimeTypeGroup = mimeType.left(mimeType.indexOf('/'));
        if ((mimeTypeGroup != "image") || !applyImageFrame(icon)) {
            limitToSize(icon, m_view->iconSize());
        }

        const QMimeData* mimeData = QApplication::clipboard()->mimeData();
        if (KonqMimeData::decodeIsCutSelection(mimeData) && isCutItem(item)) {
            KIconEffect iconEffect;
            icon = iconEffect.apply(icon, KIconLoader::Desktop, KIconLoader::DisabledState);
            m_dolphinModel->setData(idx, QIcon(icon), Qt::DecorationRole);
        } else {
            m_dolphinModel->setData(idx, QIcon(icon), Qt::DecorationRole);
        }
    }
}

bool IconManager::isCutItem(const KFileItem& item) const
{
    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    const KUrl::List cutUrls = KUrl::List::fromMimeData(mimeData);

    const KUrl& itemUrl = item.url();
    foreach (KUrl url, cutUrls) {
        if (url == itemUrl) {
            return true;
        }
    }

    return false;
}

void IconManager::applyCutItemEffect()
{
    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    if (!KonqMimeData::decodeIsCutSelection(mimeData)) {
        return;
    }

    KFileItemList items;
    KDirLister* dirLister = m_dolphinModel->dirLister();
    const KUrl::List dirs = dirLister->directories();
    foreach (KUrl url, dirs) {
        items << dirLister->itemsForDir(url);
    }

    foreach (KFileItem item, items) {
        if (isCutItem(item)) {
            const QModelIndex index = m_dolphinModel->indexForItem(item);
            const QVariant value = m_dolphinModel->data(index, Qt::DecorationRole);
            if (value.type() == QVariant::Icon) {
                const QIcon icon(qvariant_cast<QIcon>(value));
                QPixmap pixmap = icon.pixmap(m_view->iconSize());

                // remember current pixmap for the item to be able
                // to restore it when other items get cut
                CutItem cutItem;
                cutItem.url = item.url();
                cutItem.pixmap = pixmap;
                m_cutItemsCache.append(cutItem);

                // apply icon effect to the cut item
                KIconEffect iconEffect;
                pixmap = iconEffect.apply(pixmap, KIconLoader::Desktop, KIconLoader::DisabledState);
                m_dolphinModel->setData(index, QIcon(pixmap), Qt::DecorationRole);
            }
        }
    }
}

bool IconManager::applyImageFrame(QPixmap& icon)
{
    const QSize maxSize = m_view->iconSize();
    const bool applyFrame = (maxSize.width()  > KIconLoader::SizeSmallMedium) &&
                            (maxSize.height() > KIconLoader::SizeSmallMedium) &&
                            ((icon.width()  > KIconLoader::SizeLarge) ||
                             (icon.height() > KIconLoader::SizeLarge));
    if (!applyFrame) {
        // the maximum size or the image itself is too small for a frame
        return false;
    }

    const int frame = 4;
    const int doubleFrame = frame * 2;

    // resize the icon to the maximum size minus the space required for the frame
    limitToSize(icon, QSize(maxSize.width() - doubleFrame, maxSize.height() - doubleFrame));

    QPainter painter;
    const QPalette palette = m_view->palette();
    QPixmap framedIcon(icon.size().width() + doubleFrame, icon.size().height() + doubleFrame);
    framedIcon.fill(palette.color(QPalette::Normal, QPalette::Base));
    const int width = framedIcon.width() - 1;
    const int height = framedIcon.height() - 1;

    painter.begin(&framedIcon);
    painter.drawPixmap(frame, frame, icon);

    // add a border
    painter.setPen(palette.color(QPalette::Text));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(0, 0, width, height);
    painter.drawRect(1, 1, width - 2, height - 2);

    // dim image frame by 12.5 %
    painter.setPen(QColor(0, 0, 0, 32));
    painter.drawRect(frame, frame, width - doubleFrame, height - doubleFrame);
    painter.end();

    icon = framedIcon;

    // provide an alpha channel for the border
    QPixmap alphaChannel(icon.size());
    alphaChannel.fill();

    QPainter alphaPainter(&alphaChannel);
    alphaPainter.setBrush(Qt::NoBrush);
    alphaPainter.setPen(QColor(32, 32, 32));
    alphaPainter.drawRect(0, 0, width, height);
    alphaPainter.setPen(QColor(64, 64, 64));
    alphaPainter.drawRect(1, 1, width - 2, height - 2);

    icon.setAlphaChannel(alphaChannel);
    return true;
}

void IconManager::limitToSize(QPixmap& icon, const QSize& maxSize)
{
    if ((icon.width() > maxSize.width()) || (icon.height() > maxSize.height())) {
        icon = icon.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
}

void IconManager::killJobs()
{
    foreach (KJob* job, m_previewJobs) {
        Q_ASSERT(job != 0);
        job->kill();
    }
    m_previewJobs.clear();
}

#include "iconmanager.moc"
