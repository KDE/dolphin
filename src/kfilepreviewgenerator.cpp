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

#include "kfilepreviewgenerator.h"

#include <kfileitem.h>
#include <kiconeffect.h>
#include <kio/previewjob.h>
#include <kdirlister.h>
#include <kdirmodel.h>
#include <kdirsortfilterproxymodel.h>
#include <kmimetyperesolver.h>
#include <konqmimedata.h>

#include <QApplication>
#include <QAbstractItemView>
#include <QClipboard>
#include <QColor>
#include <QList>
#include <QListView>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QIcon>

#ifdef Q_WS_X11
#  include <QX11Info>
#  include <X11/Xlib.h>
#  include <X11/extensions/Xrender.h>
#endif

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
 * it again before the event loop is entered, otherwise QListView
 * would not get the correct size hints after dispatching the layoutChanged()
 * signal.
 */
class LayoutBlocker {
public:
    LayoutBlocker(QAbstractItemView* view) :
        m_uniformSizes(false),
        m_view(qobject_cast<QListView*>(view))
    {
        if (m_view != 0) {
            m_uniformSizes = m_view->uniformItemSizes();
            m_view->setUniformItemSizes(true);
        }
    }

    ~LayoutBlocker()
    {
        if (m_view != 0) {
            m_view->setUniformItemSizes(m_uniformSizes);
        }
    }

private:
    bool m_uniformSizes;
    QListView* m_view;
};

class KFilePreviewGenerator::Private
{
public:
    Private(KFilePreviewGenerator* parent,
            QAbstractItemView* view,
            KDirSortFilterProxyModel* model);
    ~Private();
    
    /**
     * Generates previews for the items \a items asynchronously.
     */
    void generatePreviews(const KFileItemList& items);

    /**
     * Adds the preview \a pixmap for the item \a item to the preview
     * queue and starts a timer which will dispatch the preview queue
     * later.
     */
    void addToPreviewQueue(const KFileItem& item, const QPixmap& pixmap);

    /**
     * Is invoked when the preview job has been finished and
     * removes the job from the m_previewJobs list.
     */
    void slotPreviewJobFinished(KJob* job);

    /** Synchronizes the item icon with the clipboard of cut items. */
    void updateCutItems();

    /**
     * Dispatches the preview queue  block by block within
     * time slices.
     */
    void dispatchPreviewQueue();

    /**
     * Pauses all preview jobs and invokes KFilePreviewGenerator::resumePreviews()
     * after a short delay. Is invoked as soon as the user has moved
     * a scrollbar.
     */
    void pausePreviews();

    /**
     * Resumes the previews that have been paused after moving the
     * scrollbar. The previews for the current visible area are
     * generated first.
     */
    void resumePreviews();
    
    /**
     * Returns true, if the item \a item has been cut into
     * the clipboard.
     */
    bool isCutItem(const KFileItem& item) const;

    /** Applies an item effect to all cut items. */
    void applyCutItemEffect();

    /**
     * Applies a frame around the icon. False is returned if
     * no frame has been added because the icon is too small.
     */
    bool applyImageFrame(QPixmap& icon);

    /**
     * Resizes the icon to \a maxSize if the icon size does not
     * fit into the maximum size. The aspect ratio of the icon
     * is kept.
     */
    void limitToSize(QPixmap& icon, const QSize& maxSize);

    /**
     * Starts a new preview job for the items \a to m_previewJobs
     * and triggers the preview timer.
     */
    void startPreviewJob(const KFileItemList& items);

    /** Kills all ongoing preview jobs. */
    void killPreviewJobs();

    /**
     * Orders the items \a items in a way that the visible items
     * are moved to the front of the list. When passing this
     * list to a preview job, the visible items will get generated
     * first.
     */
    void orderItems(KFileItemList& items);
    
    /** Remembers the pixmap for an item specified by an URL. */
    struct ItemInfo
    {
        KUrl url;
        QPixmap pixmap;
    };

    bool m_showPreview;

    /**
     * True, if m_pendingItems and m_dispatchedItems should be
     * cleared when the preview jobs have been finished.
     */
    bool m_clearItemQueues;

    /**
     * True if a selection has been done which should cut items.
     */
    bool m_hasCutSelection;

    int m_pendingVisiblePreviews;

    QAbstractItemView* m_view;
    QTimer* m_previewTimer;
    QTimer* m_scrollAreaTimer;
    QList<KJob*> m_previewJobs;
    KDirModel* m_dirModel;
    KDirSortFilterProxyModel* m_proxyModel;

    KMimeTypeResolver* m_mimeTypeResolver;

    QList<ItemInfo> m_cutItemsCache;
    QList<ItemInfo> m_previews;

    /**
     * Contains all items where a preview must be generated, but
     * where the preview job has not dispatched the items yet.
     */
    KFileItemList m_pendingItems;

    /**
     * Contains all items, where a preview has already been
     * generated by the preview jobs.
     */
    KFileItemList m_dispatchedItems;
    
private:
    KFilePreviewGenerator* const q;

};

KFilePreviewGenerator::Private::Private(KFilePreviewGenerator* parent,
                                        QAbstractItemView* view,
                                        KDirSortFilterProxyModel* model) :
    m_showPreview(true),
    m_clearItemQueues(true),
    m_hasCutSelection(false),
    m_pendingVisiblePreviews(0),
    m_view(view),
    m_previewTimer(0),
    m_scrollAreaTimer(0),
    m_previewJobs(),
    m_dirModel(0),
    m_proxyModel(model),
    m_mimeTypeResolver(0),
    m_cutItemsCache(),
    m_previews(),
    m_pendingItems(),
    m_dispatchedItems(),
    q(parent)
{
    if (!m_view->iconSize().isValid()) {
        m_showPreview = false;
    }

    m_dirModel = static_cast<KDirModel*>(m_proxyModel->sourceModel());
    connect(m_dirModel->dirLister(), SIGNAL(newItems(const KFileItemList&)),
            q, SLOT(generatePreviews(const KFileItemList&)));

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            q, SLOT(updateCutItems()));

    m_previewTimer = new QTimer(q);
    m_previewTimer->setSingleShot(true);
    connect(m_previewTimer, SIGNAL(timeout()), q, SLOT(dispatchPreviewQueue()));

    // Whenever the scrollbar values have been changed, the pending previews should
    // be reordered in a way that the previews for the visible items are generated
    // first. The reordering is done with a small delay, so that during moving the
    // scrollbars the CPU load is kept low.
    m_scrollAreaTimer = new QTimer(q);
    m_scrollAreaTimer->setSingleShot(true);
    m_scrollAreaTimer->setInterval(200);
    connect(m_scrollAreaTimer, SIGNAL(timeout()),
            q, SLOT(resumePreviews()));
    connect(m_view->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            q, SLOT(pausePreviews()));
    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)),
            q, SLOT(pausePreviews()));
}

KFilePreviewGenerator::Private::~Private()
{        
    killPreviewJobs();
    m_pendingItems.clear();
    m_dispatchedItems.clear();
    if (m_mimeTypeResolver != 0) {
        m_mimeTypeResolver->deleteLater();
        m_mimeTypeResolver = 0;
    }
}

void KFilePreviewGenerator::Private::generatePreviews(const KFileItemList& items)
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

void KFilePreviewGenerator::Private::addToPreviewQueue(const KFileItem& item, const QPixmap& pixmap)
{
    if (!m_showPreview) {
        // the preview has been canceled in the meantime
        return;
    }
    const KUrl url = item.url();

    // check whether the item is part of the directory lister (it is possible
    // that a preview from an old directory lister is received)
    KDirLister* dirLister = m_dirModel->dirLister();
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

    QPixmap icon = pixmap;

    const QString mimeType = item.mimetype();
    const QString mimeTypeGroup = mimeType.left(mimeType.indexOf('/'));
    if ((mimeTypeGroup != "image") || !applyImageFrame(icon)) {
        limitToSize(icon, m_view->iconSize());
    }

    if (m_hasCutSelection && isCutItem(item)) {
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
    }

    // remember the preview and URL, so that it can be applied to the model
    // in KFilePreviewGenerator::dispatchPreviewQueue()
    ItemInfo preview;
    preview.url = url;
    preview.pixmap = icon;
    m_previews.append(preview);

    m_dispatchedItems.append(item);
}

void KFilePreviewGenerator::Private::slotPreviewJobFinished(KJob* job)
{
    const int index = m_previewJobs.indexOf(job);
    m_previewJobs.removeAt(index);

    if ((m_previewJobs.count() == 0) && m_clearItemQueues) {
        m_pendingItems.clear();
        m_dispatchedItems.clear();
        m_pendingVisiblePreviews = 0;
        QMetaObject::invokeMethod(q, "dispatchPreviewQueue", Qt::QueuedConnection);
    }
}

void KFilePreviewGenerator::Private::updateCutItems()
{
    // restore the icons of all previously selected items to the
    // original state...
    foreach (const ItemInfo& cutItem, m_cutItemsCache) {
        const QModelIndex index = m_dirModel->indexForUrl(cutItem.url);
        if (index.isValid()) {
            m_dirModel->setData(index, QIcon(cutItem.pixmap), Qt::DecorationRole);
        }
    }
    m_cutItemsCache.clear();

    // ... and apply an item effect to all currently cut items
    applyCutItemEffect();
}

void KFilePreviewGenerator::Private::dispatchPreviewQueue()
{
    const int previewsCount = m_previews.count();
    if (previewsCount > 0) {
        // Applying the previews to the model must be done step by step
        // in larger blocks: Applying a preview immediately when getting the signal
        // 'gotPreview()' from the PreviewJob is too expensive, as a relayout
        // of the view would be triggered for each single preview.
        LayoutBlocker blocker(m_view);
        for (int i = 0; i < previewsCount; ++i) {
            const ItemInfo& preview = m_previews.first();

            const QModelIndex idx = m_dirModel->indexForUrl(preview.url);
            if (idx.isValid() && (idx.column() == 0)) {
                m_dirModel->setData(idx, QIcon(preview.pixmap), Qt::DecorationRole);
            }

            m_previews.pop_front();
            if (m_pendingVisiblePreviews > 0) {
                --m_pendingVisiblePreviews;
            }
        }
    }

    if (m_pendingVisiblePreviews > 0) {
        // As long as there are pending previews for visible items, poll
        // the preview queue each 200 ms. If there are no pending previews,
        // the queue is dispatched in slotPreviewJobFinished().
        m_previewTimer->start(200);
    }
}

void KFilePreviewGenerator::Private::pausePreviews()
{
    foreach (KJob* job, m_previewJobs) {
        Q_ASSERT(job != 0);
        job->suspend();
    }
    m_scrollAreaTimer->start();
}

void KFilePreviewGenerator::Private::resumePreviews()
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

    m_pendingVisiblePreviews = 0;
    dispatchPreviewQueue();

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

bool KFilePreviewGenerator::Private::isCutItem(const KFileItem& item) const
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

void KFilePreviewGenerator::Private::applyCutItemEffect()
{
    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    m_hasCutSelection = KonqMimeData::decodeIsCutSelection(mimeData);
    if (!m_hasCutSelection) {
        return;
    }

    KFileItemList items;
    KDirLister* dirLister = m_dirModel->dirLister();
    const KUrl::List dirs = dirLister->directories();
    foreach (const KUrl& url, dirs) {
        items << dirLister->itemsForDir(url);
    }

    foreach (const KFileItem& item, items) {
        if (isCutItem(item)) {
            const QModelIndex index = m_dirModel->indexForItem(item);
            const QVariant value = m_dirModel->data(index, Qt::DecorationRole);
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
                m_dirModel->setData(index, QIcon(pixmap), Qt::DecorationRole);
            }
        }
    }
}

bool KFilePreviewGenerator::Private::applyImageFrame(QPixmap& icon)
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
    painter.drawRect(0, 0, width, height);
    painter.drawRect(1, 1, width - 2, height - 2);
     
    painter.setCompositionMode(QPainter::CompositionMode_Plus);
    QColor blendColor = palette.color(QPalette::Normal, QPalette::Base);
    
    blendColor.setAlpha(255 - 32);
    painter.setPen(blendColor);
    painter.drawRect(0, 0, width, height);
    
    blendColor.setAlpha(255 - 64);
    painter.setPen(blendColor);
    painter.drawRect(1, 1, width - 2, height - 2);
    painter.end();

    icon = framedIcon;

    return true;
}

void KFilePreviewGenerator::Private::limitToSize(QPixmap& icon, const QSize& maxSize)
{
    if ((icon.width() > maxSize.width()) || (icon.height() > maxSize.height())) {
#ifdef Q_WS_X11
        // Assume that the texture size limit is 2048x2048
        if ((icon.width() <= 2048) && (icon.height() <= 2048)) {
            QSize size = icon.size();
            size.scale(maxSize, Qt::KeepAspectRatio);

            const qreal factor = size.width() / qreal(icon.width());

            XTransform xform = {{
                { XDoubleToFixed(1 / factor), 0, 0 },
                { 0, XDoubleToFixed(1 / factor), 0 },
                { 0, 0, XDoubleToFixed(1) }
            }};

            QPixmap pixmap(size);
            pixmap.fill(Qt::transparent);

            Display *dpy = QX11Info::display();
            XRenderSetPictureFilter(dpy, icon.x11PictureHandle(), FilterBilinear, 0, 0);
            XRenderSetPictureTransform(dpy, icon.x11PictureHandle(), &xform);
            XRenderComposite(dpy, PictOpOver, icon.x11PictureHandle(), None, pixmap.x11PictureHandle(),
                             0, 0, 0, 0, 0, 0, pixmap.width(), pixmap.height());
            icon = pixmap;
        } else {
            icon = icon.scaled(maxSize, Qt::KeepAspectRatio, Qt::FastTransformation);
        }
#else
        icon = icon.scaled(maxSize, Qt::KeepAspectRatio, Qt::FastTransformation);
#endif
    }
}

void KFilePreviewGenerator::Private::startPreviewJob(const KFileItemList& items)
{
    if (items.count() == 0) {
        return;
    }

    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    m_hasCutSelection = KonqMimeData::decodeIsCutSelection(mimeData);

    const QSize size = m_view->iconSize();

    // PreviewJob internally caches items always with the size of
    // 128 x 128 pixels or 256 x 256 pixels. A downscaling is done 
    // by PreviewJob if a smaller size is requested. As the KFilePreviewGenerator must
    // do a downscaling anyhow because of the frame, only the provided
    // cache sizes are requested.
    const int cacheSize = (size.width() > 128) || (size.height() > 128) ? 256 : 128;
    KIO::PreviewJob* job = KIO::filePreview(items, cacheSize, cacheSize);
    connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
            q, SLOT(addToPreviewQueue(const KFileItem&, const QPixmap&)));
    connect(job, SIGNAL(finished(KJob*)),
            q, SLOT(slotPreviewJobFinished(KJob*)));

    m_previewJobs.append(job);
    m_previewTimer->start(200);
}

void KFilePreviewGenerator::Private::killPreviewJobs()
{
    foreach (KJob* job, m_previewJobs) {
        Q_ASSERT(job != 0);
        job->kill();
    }
    m_previewJobs.clear();
}

void KFilePreviewGenerator::Private::orderItems(KFileItemList& items)
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

    int insertPos = 0;
    if (itemCount * 10 > rowCount) {
        // Algorithm 1: The number of items is > 10 % of the row count. Parse all rows
        // and check whether the received row is part of the item list.
        for (int row = 0; row < rowCount; ++row) {
            const QModelIndex proxyIndex = m_proxyModel->index(row, 0);
            const QRect itemRect = m_view->visualRect(proxyIndex);
            const QModelIndex dirIndex = m_proxyModel->mapToSource(proxyIndex);

            KFileItem item = m_dirModel->itemForIndex(dirIndex);  // O(1)
            const KUrl url = item.url();

            // check whether the item is part of the item list 'items'
            int index = -1;
            for (int i = 0; i < itemCount; ++i) {
                if (items.at(i).url() == url) {
                    index = i;
                    break;
                }
            }

            if ((index > 0) && itemRect.intersects(visibleArea)) {
                // The current item is (at least partly) visible. Move it
                // to the front of the list, so that the preview is
                // generated earlier.
                items.removeAt(index);
                items.insert(insertPos, item);
                ++insertPos;
                ++m_pendingVisiblePreviews;
            }
        }
    } else {
        // Algorithm 2: The number of items is <= 10 % of the row count. In this case iterate
        // all items and receive the corresponding row from the item.
        for (int i = 0; i < itemCount; ++i) {
            const QModelIndex dirIndex = m_dirModel->indexForItem(items.at(i)); // O(n) (n = number of rows)
            const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
            const QRect itemRect = m_view->visualRect(proxyIndex);

            if (itemRect.intersects(visibleArea)) {
                // The current item is (at least partly) visible. Move it
                // to the front of the list, so that the preview is
                // generated earlier.
                items.insert(insertPos, items.at(i));
                items.removeAt(i + 1);
                ++insertPos;
                ++m_pendingVisiblePreviews;
            }
        }
    }
}

KFilePreviewGenerator::KFilePreviewGenerator(QAbstractItemView* parent, KDirSortFilterProxyModel* model) :
    QObject(parent),
    d(new Private(this, parent, model))
{
}

KFilePreviewGenerator::~KFilePreviewGenerator()
{
    delete d;
}

void KFilePreviewGenerator::setShowPreview(bool show)
{
    if (show && !d->m_view->iconSize().isValid()) {
        // the view must provide an icon size, otherwise the showing
        // off previews will get ignored
        return;
    }
    
    if (d->m_showPreview != show) {
        d->m_showPreview = show;
        d->m_cutItemsCache.clear();
        d->updateCutItems();
        if (show) {
            updatePreviews();
        }
    }

    if (show && (d->m_mimeTypeResolver != 0)) {
        // don't resolve the MIME types if the preview is turned on
        d->m_mimeTypeResolver->deleteLater();
        d->m_mimeTypeResolver = 0;
    } else if (!show && (d->m_mimeTypeResolver == 0)) {
        // the preview is turned off: resolve the MIME-types so that
        // the icons gets updated
        d->m_mimeTypeResolver = new KMimeTypeResolver(d->m_view, d->m_dirModel);
    }
}

bool KFilePreviewGenerator::showPreview() const
{
    return d->m_showPreview;
}

void KFilePreviewGenerator::updatePreviews()
{
    if (!d->m_showPreview) {
        return;
    }

    d->killPreviewJobs();
    d->m_cutItemsCache.clear();
    d->m_pendingItems.clear();
    d->m_dispatchedItems.clear();

    KFileItemList itemList;
    const int rowCount = d->m_dirModel->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        const QModelIndex index = d->m_dirModel->index(row, 0);
        KFileItem item = d->m_dirModel->itemForIndex(index);
        itemList.append(item);
    }

    d->generatePreviews(itemList);
    d->updateCutItems();
}

void KFilePreviewGenerator::cancelPreviews()
{
    d->killPreviewJobs();
    d->m_cutItemsCache.clear();
    d->m_pendingItems.clear();
    d->m_dispatchedItems.clear();
}

#include "kfilepreviewgenerator.moc"
