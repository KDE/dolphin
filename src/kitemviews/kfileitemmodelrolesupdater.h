/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KFILEITEMMODELROLESUPDATER_H
#define KFILEITEMMODELROLESUPDATER_H

#include "dolphin_export.h"
#include "kitemviews/kitemmodelbase.h"

#include <list>

#include "config-dolphin.h"
#include <KFileItem>

#include <QObject>
#include <QSet>
#include <QSize>
#include <QStringList>

class KDirectoryContentsCounter;
class KFileItemModel;
class QPixmap;
class QTimer;
class KOverlayIconPlugin;

namespace KIO
{
class PreviewJob;
}

#if HAVE_BALOO
namespace Baloo
{
class FileMonitor;
}
#include <Baloo/IndexerConfig>
#endif

/**
 * @brief Resolves expensive roles asynchronously and applies them to the KFileItemModel.
 *
 * KFileItemModel only resolves roles that are inexpensive like e.g. the file name or
 * the permissions. Creating previews or determining the MIME-type can be quite expensive
 * and KFileItemModelRolesUpdater takes care to update such roles asynchronously.
 *
 * To prevent a huge CPU and I/O load, these roles are not updated for all
 * items, but only for the visible items, some items around the visible area,
 * and the items on the first and last pages of the view. This is a compromise
 * that aims to minimize the risk that the user sees items with unknown icons
 * in the view when scrolling or pressing Home or End.
 *
 * Determining the roles is done in several phases:
 *
 * 1.   If the sort role is "slow", it is determined for all items. If this
 *      cannot be finished synchronously in 200 ms, the remaining items are
 *      handled asynchronously by \a resolveNextSortRole().
 *
 * 2.   The function startUpdating(), which is called if either the sort role
 *      has been successfully determined for all items, or items are inserted
 *      in the view, or the visible items might have changed because items
 *      were removed or moved, tries to determine the icons for all visible
 *      items synchronously for 200 ms. Then:
 *
 *      (a) If previews are disabled, icons and all other roles are determined
 *          asynchronously for the interesting items. This is done by the
 *          function \a resolveNextPendingRoles().
 *
 *      (b) If previews are enabled, a \a KIO::PreviewJob is started that loads
 *          the previews for the interesting items. At the same time, the icons
 *          for these items are determined asynchronously as fast as possible
 *          by \a resolveNextPendingRoles(). This minimizes the risk that the
 *          user sees "unknown" icons when scrolling before the previews have
 *          arrived.
 *
 * 3.   Finally, the entire process is repeated for any items that might have
 *      changed in the mean time.
 */
class DOLPHIN_EXPORT KFileItemModelRolesUpdater : public QObject
{
    Q_OBJECT

public:
    explicit KFileItemModelRolesUpdater(KFileItemModel *model, QObject *parent = nullptr);
    ~KFileItemModelRolesUpdater() override;

    void setIconSize(const QSize &size);
    QSize iconSize() const;

    void setDevicePixelRatio(qreal devicePixelRatio);
    qreal devicePixelRatio() const;

    /**
     * Sets the range of items that are visible currently. The roles
     * of visible items are resolved first.
     */
    void setVisibleIndexRange(int index, int count);

    void setMaximumVisibleItems(int count);

    /**
     * If \a show is set to true, the "iconPixmap" role will be filled with a preview
     * of the file. If \a show is false the MIME type icon will be used for the "iconPixmap"
     * role.
     */
    void setPreviewsShown(bool show);
    bool previewsShown() const;

    /**
     * If enabled a small preview gets upscaled to the icon size in case where
     * the icon size is larger than the preview. Per default enlarging is
     * enabled.
     */
    void setEnlargeSmallPreviews(bool enlarge);
    bool enlargeSmallPreviews() const;

    /**
     * If \a paused is set to true the asynchronous resolving of roles will be paused.
     * State changes during pauses like changing the icon size or the preview-shown
     * will be remembered and handled after unpausing.
     */
    void setPaused(bool paused);
    bool isPaused() const;

    /**
     * Sets the roles that should be resolved asynchronously.
     */
    void setRoles(const QSet<QByteArray> &roles);
    QSet<QByteArray> roles() const;

    /**
     * Sets the list of enabled thumbnail plugins that are used for previews.
     * Per default all plugins enabled in the KConfigGroup "PreviewSettings"
     * are used.
     *
     * For a list of available plugins, call KIO::PreviewJob::availableThumbnailerPlugins().
     *
     * @see enabledPlugins
     */
    void setEnabledPlugins(const QStringList &list);

    /**
     * Returns the list of enabled thumbnail plugins.
     * @see setEnabledPlugins
     */
    QStringList enabledPlugins() const;

    /**
     * Sets the maximum file size of local files for which
     * previews will be generated (if enabled). A value of 0
     * indicates no file size limit.
     * Per default the value from KConfigGroup "PreviewSettings"
     * MaximumSize is used, 0 otherwise.
     * @param size
     */
    void setLocalFileSizePreviewLimit(qlonglong size);
    qlonglong localFileSizePreviewLimit() const;

    /**
     * If set to true, directories contents are scanned to determine their size
     * Default true
     */
    void setScanDirectories(bool enabled);
    bool scanDirectories() const;

    /**
     * Notifies the updater of a change in the hover state on an item.
     *
     * This will trigger asynchronous loading of the next few thumb sequence images
     * using a PreviewJob.
     *
     * @param item URL of the item that is hovered, or an empty URL if no item is hovered.
     * @param seqIdx The current hover sequence index. While an item is hovered,
     *               this method will be called repeatedly with increasing values
     *               for this parameter.
     */
    void setHoverSequenceState(const QUrl &itemUrl, int seqIdx);

private Q_SLOTS:
    void slotItemsInserted(const KItemRangeList &itemRanges);
    void slotItemsRemoved(const KItemRangeList &itemRanges);
    void slotItemsMoved(KItemRange itemRange, const QList<int> &movedToIndexes);
    void slotItemsChanged(const KItemRangeList &itemRanges, const QSet<QByteArray> &roles);
    void slotSortRoleChanged(const QByteArray &current, const QByteArray &previous);

    /**
     * Is invoked after a preview has been received successfully.
     *
     * Note that this is not called for hover sequence previews.
     *
     * @see startPreviewJob()
     */
    void slotGotPreview(const KFileItem &item, const QPixmap &pixmap);

    /**
     * Is invoked after generating a preview has failed.
     *
     * Note that this is not called for hover sequence previews.
     *
     * @see startPreviewJob()
     */
    void slotPreviewFailed(const KFileItem &item);

    /**
     * Is invoked when the preview job has been finished. Starts a new preview
     * job if there are any interesting items without previews left, or updates
     * the changed items otherwise.
     *
     * Note that this is not called for hover sequence previews.
     *
     * @see startPreviewJob()
     */
    void slotPreviewJobFinished();

    /**
     * Is invoked after a hover sequence preview has been received successfully.
     */
    void slotHoverSequenceGotPreview(const KFileItem &item, const QPixmap &pixmap);

    /**
     * Is invoked after generating a hover sequence preview has failed.
     */
    void slotHoverSequencePreviewFailed(const KFileItem &item);

    /**
     * Is invoked when a hover sequence preview job is finished. May start another
     * job for the next sequence index right away by calling
     * \a loadNextHoverSequencePreview().
     *
     * Note that a PreviewJob will only ever generate a single sequence image, due
     * to limitations of the PreviewJob API.
     */
    void slotHoverSequencePreviewJobFinished();

    /**
     * Is invoked when one of the KOverlayIconPlugin emit the signal that an overlay has changed
     */
    void slotOverlaysChanged(const QUrl &url, const QStringList &);

    /**
     * Resolves the sort role of the next item in m_pendingSortRole, applies it
     * to the model, and invokes itself if there are any pending items left. If
     * that is not the case, \a startUpdating() is called.
     */
    void resolveNextSortRole();

    /**
     * Resolves the icon name and (if previews are disabled) all other roles
     * for the next interesting item. If there are no pending items left, any
     * changed items are updated.
     */
    void resolveNextPendingRoles();

    /**
     * Resolves items that have not been resolved yet after the change has been
     * notified by slotItemsChanged(). Is invoked if the m_changedItemsTimer
     * expires.
     */
    void resolveRecentlyChangedItems();

    void applyChangedBalooRoles(const QString &file);
    void applyChangedBalooRolesForItem(const KFileItem &file);

    void slotDirectoryContentsCountReceived(const QString &path, int count, long long size);

private:
    /**
     * Starts the updating of all roles. The visible items are handled first.
     */
    void startUpdating();

    /**
     * Loads the icons for the visible items. After 200 ms, the function
     * stops determining mime types and only loads preliminary icons.
     * This is a compromise that prevents that
     * (a) the GUI is blocked for more than 200 ms, and
     * (b) "unknown" icons could be shown in the view.
     */
    void updateVisibleIcons();

    /**
     * Creates previews for the items starting from the first item in
     * m_pendingPreviewItems.
     * @see slotGotPreview()
     * @see slotPreviewFailed()
     * @see slotPreviewJobFinished()
     */
    void startPreviewJob();

    /**
     * Transforms a raw preview image, applying scale and frame.
     *
     * @param pixmap A raw preview image from a PreviewJob.
     * @return The scaled and decorated preview image.
     */
    QPixmap transformPreviewPixmap(const QPixmap &pixmap);

    /**
     * Starts a PreviewJob for loading the next hover sequence image.
     */
    void loadNextHoverSequencePreview();

    /**
     * Aborts the currently running hover sequence PreviewJob (if any).
     */
    void killHoverSequencePreviewJob();

    /**
     * Ensures that icons, previews, and other roles are determined for any
     * items that have been changed.
     */
    void updateChangedItems();

    /**
     * Resolves the sort role of the item and applies it to the model.
     */
    void applySortRole(int index);

    void applySortProgressToModel();

    enum ResolveHint { ResolveFast, ResolveAll };
    bool applyResolvedRoles(int index, ResolveHint hint);
    QHash<QByteArray, QVariant> rolesData(const KFileItem &item, int index);

    /**
     * Must be invoked if a property has been changed that affects
     * the look of the preview. Takes care to update all previews.
     */
    void updateAllPreviews();

    void killPreviewJob();

    QList<int> indexesToResolve() const;

    void trimHoverSequenceLoadedItems();

private:
    /**
     * enqueue directory size counting for KFileItem item at index
     */
    void startDirectorySizeCounting(const KFileItem &item, int index);

    enum State { Idle, Paused, ResolvingSortRole, ResolvingAllRoles, PreviewJobRunning };

    State m_state;

    // Property changes during pausing must be remembered to be able
    // to react when unpausing again:
    bool m_previewChangedDuringPausing;
    bool m_iconSizeChangedDuringPausing;
    bool m_rolesChangedDuringPausing;

    // Property for setPreviewsShown()/previewsShown().
    bool m_previewShown;

    // Property for setEnlargeSmallPreviews()/enlargeSmallPreviews()
    bool m_enlargeSmallPreviews;

    // True if the role "iconPixmap" should be cleared when resolving the next
    // role with resolveRole(). Is necessary if the preview gets disabled
    // during the roles-updater has been paused by setPaused().
    bool m_clearPreviews;

    // Remembers which items have been handled already, to prevent that
    // previews and other expensive roles are determined again.
    QSet<KFileItem> m_finishedItems;

    KFileItemModel *m_model;
    QSize m_iconSize;
    qreal m_devicePixelRatio;
    int m_firstVisibleIndex;
    int m_lastVisibleIndex;
    int m_maximumVisibleItems;
    QSet<QByteArray> m_roles;
    QSet<QByteArray> m_resolvableRoles;
    QStringList m_enabledPlugins;
    qulonglong m_localFileSizePreviewLimit;

    // Items for which the sort role still has to be determined.
    QSet<KFileItem> m_pendingSortRoleItems;

    // Indexes of items which still have to be handled by
    // resolveNextPendingRoles().
    QList<int> m_pendingIndexes;

    // Items which have been left over from the last call of startPreviewJob().
    // A new preview job will be started from them once the first one finishes.
    KFileItemList m_pendingPreviewItems;

    KIO::PreviewJob *m_previewJob;

    // Info about the item that the user currently hovers, and the current sequence
    // index for thumb generation.
    KFileItem m_hoverSequenceItem;
    int m_hoverSequenceIndex;
    KIO::PreviewJob *m_hoverSequencePreviewJob;
    int m_hoverSequenceNumSuccessiveFailures;
    std::list<KFileItem> m_hoverSequenceLoadedItems;

    // When downloading or copying large files, the slot slotItemsChanged()
    // will be called periodically within a quite short delay. To prevent
    // a high CPU-load by generating e.g. previews for each notification, the update
    // will be postponed until no file change has been done within a longer period
    // of time.
    QTimer *m_recentlyChangedItemsTimer;
    QSet<KFileItem> m_recentlyChangedItems;

    // Items which have not been changed repeatedly recently.
    QSet<KFileItem> m_changedItems;

    KDirectoryContentsCounter *m_directoryContentsCounter;

    QList<KOverlayIconPlugin *> m_overlayIconsPlugin;

#if HAVE_BALOO
    Baloo::FileMonitor *m_balooFileMonitor;
    Baloo::IndexerConfig m_balooConfig;
#endif
};

#endif
