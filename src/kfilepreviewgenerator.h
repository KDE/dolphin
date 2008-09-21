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

#ifndef KFILEPREVIEWGENERATOR_H
#define KFILEPREVIEWGENERATOR_H

#include <kfileitem.h>
#include <kurl.h>

#include <QList>
#include <QObject>
#include <QPixmap>

class KDirModel;
class KDirSortFilterProxyModel;
class KJob;
class KMimeTypeResolver;
class QAbstractItemView;

/**
 * @brief Manages the icon state of a directory model.
 *
 * Per default a preview is generated for each item.
 * Additionally the clipboard is checked for cut items.
 * The icon state for cut items gets dimmed automatically.
 *
 * The following strategy is used when creating previews:
 * - The previews for currently visible items are created before
 *   the previews for invisible items.
 * - If the user changes the visible area by using the scrollbars,
 *   all pending previews get paused. As soon as the user stays
 *   on the same position for a short delay, the previews are
 *   resumed. Also in this case the previews for the visible items
 *   are generated first.
 */
class KFilePreviewGenerator : public QObject
{
    Q_OBJECT

public:
    KFilePreviewGenerator(QAbstractItemView* parent, KDirSortFilterProxyModel* model);
    virtual ~KFilePreviewGenerator();
    void setShowPreview(bool show);
    bool showPreview() const;

    /**
     * Updates the previews for all already available items. It is only necessary
     * to invoke this method when the icon size of the abstract item view has
     * been changed.
     */
    void updatePreviews();

    /**
     * Cancels all pending previews. Should be invoked when the URL of the item
     * view has been changed.
     */
    void cancelPreviews();

private slots:
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

private:
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

private:
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
};

inline bool KFilePreviewGenerator::showPreview() const
{
    return m_showPreview;
}

#endif
