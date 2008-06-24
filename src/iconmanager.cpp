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
#include <kmimetyperesolver.h>
#include <konqmimedata.h>

#include <QApplication>
#include <QAbstractItemView>
#include <QClipboard>
#include <QColor>
#include <QListView>
#include <QPainter>
#include <QScrollBar>
#include <QIcon>

/**
 * If the passed item view is an instance of QListView, expensive
 * layout operations are blocked in the constructor and are unblocked
 * again in the destructor.
 *
 * This helper class is a workaround for the following huge performance
 * problem when having directories with several 1000 items:
 * - each change of an icon emits a dataChanged() signal from the model
 * - QListView iterates through all items on each dataChanged() signal
 *   and invokes QItemDelegate::sizeHint()
 * - the sizeHint() implementation of KFileItemDelegate is quite complex,
 *   invoking it 1000 times for each icon change might block the UI
 *
 * QListView does not invoke QItemDelegate::sizeHint() when the
 * uniformItemSize property has been set to true, so this property is
 * set before exchanging a block of icons. It is important to reset
 * it to false again before the event loop is entered, otherwise QListView
 * would not get the correct size hints after dispatching the layoutChanged()
 * signal.
 */
class LayoutBlocker {
public:
    LayoutBlocker(QAbstractItemView* view) : m_view(0)
    {
        if (view->inherits("QListView")) {
            m_view = qobject_cast<QListView*>(view);
            m_view->setUniformItemSizes(true);
        }
    }

    ~LayoutBlocker()
    {
        if (m_view != 0) {
            m_view->setUniformItemSizes(false);
        }
    }

private:
    QListView* m_view;
};

IconManager::IconManager(QAbstractItemView* parent, DolphinSortFilterProxyModel* model) :
    QObject(parent),
    m_showPreview(false),
    m_clearItemQueues(true),
    m_view(parent),
    m_previewTimer(0),
    m_scrollAreaTimer(0),
    m_previewJobs(),
    m_dolphinModel(0),
    m_proxyModel(model),
    m_mimeTypeResolver(0),
    m_cutItemsCache(),
    m_previews(),
    m_pendingItems(),
    m_dispatchedItems()
{
    Q_ASSERT(m_view->iconSize().isValid());  // each view must provide its current icon size

    m_dolphinModel = static_cast<DolphinModel*>(m_proxyModel->sourceModel());
    connect(m_dolphinModel->dirLister(), SIGNAL(newItems(const KFileItemList&)),
            this, SLOT(generatePreviews(const KFileItemList&)));

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updateCutItems()));

    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    connect(m_previewTimer, SIGNAL(timeout()), this, SLOT(dispatchPreviewQueue()));

    // Whenever the scrollbar values have been changed, the pending previews should
    // be reordered in a way that the previews for the visible items are generated
    // first. The reordering is done with a small delay, so that during moving the
    // scrollbars the CPU load is kept low.
    m_scrollAreaTimer = new QTimer(this);
    m_scrollAreaTimer->setSingleShot(true);
    m_scrollAreaTimer->setInterval(200);
    connect(m_scrollAreaTimer, SIGNAL(timeout()),
            this, SLOT(resumePreviews()));
    connect(m_view->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(pausePreviews()));
    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(pausePreviews()));
}

IconManager::~IconManager()
{
    killPreviewJobs();
    m_pendingItems.clear();
    m_dispatchedItems.clear();
    if (m_mimeTypeResolver != 0) {
        m_mimeTypeResolver->deleteLater();
        m_mimeTypeResolver = 0;
    }
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

    if (show && (m_mimeTypeResolver != 0)) {
        // don't resolve the MIME types if the preview is turned on
        m_mimeTypeResolver->deleteLater();
        m_mimeTypeResolver = 0;
    } else if (!show && (m_mimeTypeResolver == 0)) {
        // the preview is turned off: resolve the MIME-types so that
        // the icons gets updated
        m_mimeTypeResolver = new KMimeTypeResolver(m_view, m_dolphinModel);
    }
}

void IconManager::updatePreviews()
{
    if (!m_showPreview) {
        return;
    }

    killPreviewJobs();
    m_cutItemsCache.clear();
    m_pendingItems.clear();
    m_dispatchedItems.clear();

    KFileItemList itemList;
    const int rowCount = m_dolphinModel->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        const QModelIndex index = m_dolphinModel->index(row, 0);
        KFileItem item = m_dolphinModel->itemForIndex(index);
        itemList.append(item);
    }

    generatePreviews(itemList);
    updateCutItems();
}

void IconManager::cancelPreviews()
{
    killPreviewJobs();
    m_cutItemsCache.clear();
    m_pendingItems.clear();
    m_dispatchedItems.clear();
}

void IconManager::generatePreviews(const KFileItemList& items)
{
    applyCutItemEffect();

    if (!m_showPreview) {
        return;
    }

    KFileItemList orderedItems = items;
    orderItems(orderedItems);

    foreach (const KFileItem& item, orderedItems) {
        m_pendingItems.append(item);
    }

    startPreviewJob(orderedItems);
}

void IconManager::addToPreviewQueue(const KFileItem& item, const QPixmap& pixmap)
{
    ItemInfo preview;
    preview.url = item.url();
    preview.pixmap = pixmap;
    m_previews.append(preview);

    m_dispatchedItems.append(item);
}

void IconManager::slotPreviewJobFinished(KJob* job)
{
    const int index = m_previewJobs.indexOf(job);
    m_previewJobs.removeAt(index);

    if ((m_previewJobs.count() == 0) && m_clearItemQueues) {
        m_pendingItems.clear();
        m_dispatchedItems.clear();
    }
}

void IconManager::updateCutItems()
{
    // restore the icons of all previously selected items to the
    // original state...
    foreach (const ItemInfo& cutItem, m_cutItemsCache) {
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
        if (dispatchCount > previewsCount) {
            dispatchCount = previewsCount;
        }

        LayoutBlocker blocker(m_view);
        for (int i = 0; i < dispatchCount; ++i) {
            const ItemInfo& preview = m_previews.first();
            replaceIcon(preview.url, preview.pixmap);
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

void IconManager::pausePreviews()
{
    foreach (KJob* job, m_previewJobs) {
        Q_ASSERT(job != 0);
        job->suspend();
    }
    m_scrollAreaTimer->start();
}

void IconManager::resumePreviews()
{
    // Before creating new preview jobs the m_pendingItems queue must be
    // cleaned up by removing the already dispatched items. Implementation
    // note: The order of the m_dispatchedItems queue and the m_pendingItems
    // queue is usually equal. So even when having a lot of elements the
    // nested loop is no performance bottle neck, as the inner loop is only
    // entered once in most cases.
    foreach (const KFileItem& item, m_dispatchedItems) {
        KFileItemList::iterator begin = m_pendingItems.begin();
        KFileItemList::iterator end   = m_pendingItems.end();
        for (KFileItemList::iterator it = begin; it != end; ++it) {
            if ((*it).url() == item.url()) {
                m_pendingItems.erase(it);
                break;
            }
        }
    }
    m_dispatchedItems.clear();

    KFileItemList orderedItems = m_pendingItems;
    orderItems(orderedItems);

    // Kill all suspended preview jobs. Usually when a preview job
    // has been finished, slotPreviewJobFinished() clears all item queues.
    // This is not wanted in this case, as a new job is created afterwards
    // for m_pendingItems.
    m_clearItemQueues = false;
    killPreviewJobs();
    m_clearItemQueues = true;

    startPreviewJob(orderedItems);
}

void IconManager::replaceIcon(const KUrl& url, const QPixmap& pixmap)
{
    Q_ASSERT(url.isValid());
    if (!m_showPreview) {
        // the preview has been canceled in the meantime
        return;
    }

    // check whether the item is part of the directory lister (it is possible
    // that a preview from an old directory lister is received)
    KDirLister* dirLister = m_dolphinModel->dirLister();
    bool isOldPreview = true;
    const KUrl::List dirs = dirLister->directories();
    const QString itemDir = url.directory();
    foreach (const KUrl& url, dirs) {
        if (url.path() == itemDir) {
            isOldPreview = false;
            break;
        }
    }
    if (isOldPreview) {
        return;
    }

    const QModelIndex idx = m_dolphinModel->indexForUrl(url);
    if (idx.isValid() && (idx.column() == 0)) {
        QPixmap icon = pixmap;

        const KFileItem item = m_dolphinModel->itemForIndex(idx);
        const QString mimeType = item.mimetype();
        const QString mimeTypeGroup = mimeType.left(mimeType.indexOf('/'));
        if ((mimeTypeGroup != "image") || !applyImageFrame(icon)) {
            limitToSize(icon, m_view->iconSize());
        }

        const QMimeData* mimeData = QApplication::clipboard()->mimeData();
        if (KonqMimeData::decodeIsCutSelection(mimeData) && isCutItem(item)) {
            // Remember the current icon in the cache for cut items before
            // the disabled effect is applied. This makes it possible restoring
            // the uncut version again when cutting other items.
            QList<ItemInfo>::iterator begin = m_cutItemsCache.begin();
            QList<ItemInfo>::iterator end   = m_cutItemsCache.end();
            for (QList<ItemInfo>::iterator it = begin; it != end; ++it) {
                if ((*it).url == item.url()) {
                    (*it).pixmap = icon;
                    break;
                }
            }

            // apply the disabled effect to the icon for marking it as "cut item"
            // and apply the icon to the item
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

    const KUrl itemUrl = item.url();
    foreach (const KUrl& url, cutUrls) {
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
    foreach (const KUrl& url, dirs) {
        items << dirLister->itemsForDir(url);
    }

    foreach (const KFileItem& item, items) {
        if (isCutItem(item)) {
            const QModelIndex index = m_dolphinModel->indexForItem(item);
            const QVariant value = m_dolphinModel->data(index, Qt::DecorationRole);
            if (value.type() == QVariant::Icon) {
                const QIcon icon(qvariant_cast<QIcon>(value));
                const QSize actualSize = icon.actualSize(m_view->iconSize());
                QPixmap pixmap = icon.pixmap(actualSize);

                // remember current pixmap for the item to be able
                // to restore it when other items get cut
                ItemInfo cutItem;
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

void IconManager::startPreviewJob(const KFileItemList& items)
{
    if (items.count() == 0) {
        return;
    }

    const QSize size = m_view->iconSize();
    KIO::PreviewJob* job = KIO::filePreview(items, 128, 128);
    connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
            this, SLOT(addToPreviewQueue(const KFileItem&, const QPixmap&)));
    connect(job, SIGNAL(finished(KJob*)),
            this, SLOT(slotPreviewJobFinished(KJob*)));

    m_previewJobs.append(job);
    m_previewTimer->start(200);
}

void IconManager::killPreviewJobs()
{
    foreach (KJob* job, m_previewJobs) {
        Q_ASSERT(job != 0);
        job->kill();
    }
    m_previewJobs.clear();
}

void IconManager::orderItems(KFileItemList& items)
{
    // Order the items in a way that the preview for the visible items
    // is generated first, as this improves the feeled performance a lot.
    //
    // Implementation note: 2 different algorithms are used for the sorting.
    // Algorithm 1 is faster when having a lot of items in comparison
    // to the number of rows in the model. Algorithm 2 is faster
    // when having quite less items in comparison to the number of rows in
    // the model. Choosing the right algorithm is important when having directories
    // with several hundreds or thousands of items.

    const int itemCount = items.count();
    const int rowCount = m_proxyModel->rowCount();
    const QRect visibleArea = m_view->viewport()->rect();

    if (itemCount * 10 > rowCount) {
        // Algorithm 1: The number of items is > 10 % of the row count. Parse all rows
        // and check whether the received row is part of the item list.
        for (int row = 0; row < rowCount; ++row) {
            const QModelIndex proxyIndex = m_proxyModel->index(row, 0);
            const QRect itemRect = m_view->visualRect(proxyIndex);
            const QModelIndex dirIndex = m_proxyModel->mapToSource(proxyIndex);

            KFileItem item = m_dolphinModel->itemForIndex(dirIndex);  // O(1)
            const KUrl url = item.url();

            // check whether the item is part of the item list 'items'
            int index = -1;
            for (int i = 0; i < itemCount; ++i) {
                if (items[i].url() == url) {
                    index = i;
                    break;
                }
            }

            if ((index > 0) && itemRect.intersects(visibleArea)) {
                // The current item is (at least partly) visible. Move it
                // to the front of the list, so that the preview is
                // generated earlier.
                items.removeAt(index);
                items.insert(0, item);
            }
        }
    } else {
        // Algorithm 2: The number of items is <= 10 % of the row count. In this case iterate
        // all items and receive the corresponding row from the item.
        for (int i = 0; i < itemCount; ++i) {
            const QModelIndex dirIndex = m_dolphinModel->indexForItem(items[i]); // O(n) (n = number of rows)
            const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
            const QRect itemRect = m_view->visualRect(proxyIndex);

            if (itemRect.intersects(visibleArea)) {
                // The current item is (at least partly) visible. Move it
                // to the front of the list, so that the preview is
                // generated earlier.
                items.insert(0, items[i]);
                items.removeAt(i + 1);
            }
        }
    }
}

#include "iconmanager.moc"
