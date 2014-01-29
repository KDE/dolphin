/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef KFILEITEMMODELROLESUPDATER_H
#define KFILEITEMMODELROLESUPDATER_H

#include <config-baloo.h>

#include <KFileItem>
#include <kitemviews/kitemmodelbase.h>

#include <libdolphin_export.h>

#include <QObject>
#include <QSet>
#include <QSize>
#include <QStringList>

class KDirectoryContentsCounter;
class KFileItemModel;
class KJob;
class QPixmap;
class QTimer;

#ifdef HAVE_BALOO
    namespace Baloo
    {
        class FileMonitor;
    }
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
class LIBDOLPHINPRIVATE_EXPORT KFileItemModelRolesUpdater : public QObject
{
    Q_OBJECT

public:
    explicit KFileItemModelRolesUpdater(KFileItemModel* model, QObject* parent = 0);
    virtual ~KFileItemModelRolesUpdater();

    void setIconSize(const QSize& size);
    QSize iconSize() const;

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
    void setRoles(const QSet<QByteArray>& roles);
    QSet<QByteArray> roles() const;

    /**
     * Sets the list of enabled thumbnail plugins that are used for previews.
     * Per default all plugins enabled in the KConfigGroup "PreviewSettings"
     * are used.
     *
     * For a list of available plugins, call KServiceTypeTrader::self()->query("ThumbCreator").
     *
     * @see enabledPlugins
     */
    void setEnabledPlugins(const QStringList& list);

    /**
     * Returns the list of enabled thumbnail plugins.
     * @see setEnabledPlugins
     */
    QStringList enabledPlugins() const;

private slots:
    void slotItemsInserted(const KItemRangeList& itemRanges);
    void slotItemsRemoved(const KItemRangeList& itemRanges);
    void slotItemsMoved(const KItemRange& itemRange, QList<int> movedToIndexes);
    void slotItemsChanged(const KItemRangeList& itemRanges,
                          const QSet<QByteArray>& roles);
    void slotSortRoleChanged(const QByteArray& current,
                             const QByteArray& previous);

    /**
     * Is invoked after a preview has been received successfully.
     * @see startPreviewJob()
     */
    void slotGotPreview(const KFileItem& item, const QPixmap& pixmap);

    /**
     * Is invoked after generating a preview has failed.
     * @see startPreviewJob()
     */
    void slotPreviewFailed(const KFileItem& item);

    /**
     * Is invoked when the preview job has been finished. Starts a new preview
     * job if there are any interesting items without previews left, or updates
     * the changed items otherwise.     *
     * @see startPreviewJob()
     */
    void slotPreviewJobFinished();

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

    void applyChangedBalooRoles(const QString& file);

    void slotDirectoryContentsCountReceived(const QString& path, int count);

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
     * Ensures that icons, previews, and other roles are determined for any
     * items that have been changed.
     */
    void updateChangedItems();

    /**
     * Resolves the sort role of the item and applies it to the model.
     */
    void applySortRole(int index);

    void applySortProgressToModel();

    enum ResolveHint {
        ResolveFast,
        ResolveAll
    };
    bool applyResolvedRoles(int index, ResolveHint hint);
    QHash<QByteArray, QVariant> rolesData(const KFileItem& item);

    /**
     * @return The number of items of the path \a path.
     */
    int subItemsCount(const QString& path) const;

    /**
     * Must be invoked if a property has been changed that affects
     * the look of the preview. Takes care to update all previews.
     */
    void updateAllPreviews();

    void killPreviewJob();

    QList<int> indexesToResolve() const;

private:
    enum State {
        Idle,
        Paused,
        ResolvingSortRole,
        ResolvingAllRoles,
        PreviewJobRunning
    };

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

    KFileItemModel* m_model;
    QSize m_iconSize;
    int m_firstVisibleIndex;
    int m_lastVisibleIndex;
    int m_maximumVisibleItems;
    QSet<QByteArray> m_roles;
    QSet<QByteArray> m_resolvableRoles;
    QStringList m_enabledPlugins;

    // Items for which the sort role still has to be determined.
    QSet<KFileItem> m_pendingSortRoleItems;

    // Indexes of items which still have to be handled by
    // resolveNextPendingRoles().
    QList<int> m_pendingIndexes;

    // Items which have been left over from the last call of startPreviewJob().
    // A new preview job will be started from them once the first one finishes.
    KFileItemList m_pendingPreviewItems;

    KJob* m_previewJob;

    // When downloading or copying large files, the slot slotItemsChanged()
    // will be called periodically within a quite short delay. To prevent
    // a high CPU-load by generating e.g. previews for each notification, the update
    // will be postponed until no file change has been done within a longer period
    // of time.
    QTimer* m_recentlyChangedItemsTimer;
    QSet<KFileItem> m_recentlyChangedItems;

    // Items which have not been changed repeatedly recently.
    QSet<KFileItem> m_changedItems;

    KDirectoryContentsCounter* m_directoryContentsCounter;

#ifdef HAVE_BALOO
    Baloo::FileMonitor* m_balooFileMonitor;
#endif
};

#endif


