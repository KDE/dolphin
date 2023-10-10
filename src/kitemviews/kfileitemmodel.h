/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KFILEITEMMODEL_H
#define KFILEITEMMODEL_H

#include "dolphin_export.h"
#include "kitemviews/kitemmodelbase.h"
#include "kitemviews/private/kfileitemmodelfilter.h"

#include <KFileItem>
#include <KLazyLocalizedString>

#include <QCollator>
#include <QHash>
#include <QSet>
#include <QUrl>

#include <functional>

class KDirLister;

class QTimer;

namespace KIO
{
class Job;
}

/**
 * @brief KItemModelBase implementation for KFileItems.
 *
 * Allows to load items of a directory. Sorting and grouping of
 * items are supported. Roles that are not part of KFileItem can
 * be added with KFileItemModel::setData().
 *
 * Recursive expansion of sub-directories is supported by
 * KFileItemModel::setExpanded().
 */
class DOLPHIN_EXPORT KFileItemModel : public KItemModelBase
{
    Q_OBJECT

public:
    explicit KFileItemModel(QObject *parent = nullptr);
    ~KFileItemModel() override;

    /**
     * Loads the directory specified by \a url. The signals
     * directoryLoadingStarted(), directoryLoadingProgress() and directoryLoadingCompleted()
     * indicate the current state of the loading process. The items
     * of the directory are added after the loading has been completed.
     */
    void loadDirectory(const QUrl &url);

    /**
     * Throws away all currently loaded items and refreshes the directory
     * by reloading all items again.
     */
    void refreshDirectory(const QUrl &url);

    /**
     * @return Parent directory of the items that are shown. In case
     *         if a directory tree is shown, KFileItemModel::dir() returns
     *         the root-parent of all items.
     * @see rootItem()
     */
    QUrl directory() const override;

    /**
     * Cancels the loading of a directory which has been started by either
     * loadDirectory() or refreshDirectory().
     */
    void cancelDirectoryLoading();

    int count() const override;
    QHash<QByteArray, QVariant> data(int index) const override;
    bool setData(int index, const QHash<QByteArray, QVariant> &values) override;

    /**
     * Sets a separate sorting with directories first (true) or a mixed
     * sorting of files and directories (false).
     */
    void setSortDirectoriesFirst(bool dirsFirst);
    bool sortDirectoriesFirst() const;

    /**
     * Sets a separate sorting with hidden files and folders last (true) or not (false).
     */
    void setSortHiddenLast(bool hiddenLast);
    bool sortHiddenLast() const;

    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const;

    /**
     * If set to true, only directories are shown as items of the model. Files
     * are ignored.
     */
    void setShowDirectoriesOnly(bool enabled);
    bool showDirectoriesOnly() const;

    QMimeData *createMimeData(const KItemSet &indexes) const override;

    int indexForKeyboardSearch(const QString &text, int startFromIndex = 0) const override;

    bool supportsDropping(int index) const override;

    QString roleDescription(const QByteArray &role) const override;

    QList<QPair<int, QVariant>> groups() const override;

    /**
     * @return The file-item for the index \a index. If the index is in a valid
     *         range it is assured that the file-item is not null. The runtime
     *         complexity of this call is O(1).
     */
    KFileItem fileItem(int index) const;

    /**
     * @return The file-item for the url \a url. If no file-item with the given
     *         URL is found KFileItem::isNull() will be true for the returned
     *         file-item. The runtime complexity of this call is O(1).
     */
    KFileItem fileItem(const QUrl &url) const;

    /**
     * @return The index for the file-item \a item. -1 is returned if no file-item
     *         is found or if the file-item is null. The amortized runtime
     *         complexity of this call is O(1).
     */
    int index(const KFileItem &item) const;

    /**
     * @return The index for the URL \a url. -1 is returned if no file-item
     *         is found. The amortized runtime complexity of this call is O(1).
     */
    int index(const QUrl &url) const;

    /**
     * @return Root item of all items representing the item
     *         for KFileItemModel::dir().
     */
    KFileItem rootItem() const;

    /**
     * Clears all items of the model.
     */
    void clear();

    /**
     * Sets the roles that should be shown for each item.
     */
    void setRoles(const QSet<QByteArray> &roles);
    QSet<QByteArray> roles() const;

    bool setExpanded(int index, bool expanded) override;
    bool isExpanded(int index) const override;
    bool isExpandable(int index) const override;
    int expandedParentsCount(int index) const override;

    QSet<QUrl> expandedDirectories() const;

    /**
     * Marks the URLs in \a urls as sub-directories which were expanded previously.
     * After calling loadDirectory() or refreshDirectory() the marked sub-directories
     * will be expanded step-by-step.
     */
    void restoreExpandedDirectories(const QSet<QUrl> &urls);

    /**
     * Expands all parent-directories of the item \a url.
     */
    void expandParentDirectories(const QUrl &url);

    void setNameFilter(const QString &nameFilter);
    QString nameFilter() const;

    void setMimeTypeFilters(const QStringList &filters);
    QStringList mimeTypeFilters() const;

    void setExcludeMimeTypeFilter(const QStringList &filters);
    QStringList excludeMimeTypeFilter() const;

    struct RoleInfo {
        QByteArray role;
        QString translation;
        QString group;
        QString tooltip;
        bool requiresBaloo;
        bool requiresIndexer;
    };

    /**
     * @return Provides static information for all available roles that
     *         are supported by KFileItemModel. Some roles can only be
     *         determined if Baloo is enabled and/or the Baloo
     *         indexing is enabled.
     */
    static QList<RoleInfo> rolesInformation();

    /** set to true to hide application/x-trash files */
    void setShowTrashMime(bool show);

Q_SIGNALS:
    /**
     * Is emitted if the loading of a directory has been started. It is
     * assured that a signal directoryLoadingCompleted() will be send after
     * the loading has been finished. For tracking the loading progress
     * the signal directoryLoadingProgress() gets emitted in between.
     */
    void directoryLoadingStarted();

    /**
     * Is emitted after the loading of a directory has been completed or new
     * items have been inserted to an already loaded directory. Usually
     * one or more itemsInserted() signals are emitted before loadingCompleted()
     * (the only exception is loading an empty directory, where only a
     * loadingCompleted() signal gets emitted).
     */
    void directoryLoadingCompleted();

    /**
     * Is emitted when the model is being refreshed (F5 key press)
     */
    void directoryRefreshing();

    /**
     * Is emitted after the loading of a directory has been canceled.
     */
    void directoryLoadingCanceled();

    /**
     * Informs about the progress in percent when loading a directory. It is assured
     * that the signal directoryLoadingStarted() has been emitted before.
     */
    void directoryLoadingProgress(int percent);

    /**
     * Is emitted if the sort-role gets resolved asynchronously and provides
     * the progress-information of the sorting in percent. It is assured
     * that the last sortProgress-signal contains 100 as value.
     */
    void directorySortingProgress(int percent);

    /**
     * Is emitted if an information message (e.g. "Connecting to host...")
     * should be shown.
     */
    void infoMessage(const QString &message);

    /**
     * Is emitted if an error message (e.g. "Unknown location")
     * should be shown.
     */
    void errorMessage(const QString &message);

    /**
     * Is emitted if a redirection from the current URL \a oldUrl
     * to the new URL \a newUrl has been done.
     */
    void directoryRedirection(const QUrl &oldUrl, const QUrl &newUrl);

    /**
     * Is emitted when the URL passed by KFileItemModel::setUrl() represents a file.
     * In this case no signal errorMessage() will be emitted.
     */
    void urlIsFileError(const QUrl &url);

    /**
     * It is emitted for files when they change and
     * for dirs when files are added or removed.
     */
    void fileItemsChanged(const KFileItemList &changedFileItems);

    /**
     * It is emitted when the parent directory was removed.
     */
    void currentDirectoryRemoved();

protected:
    void onGroupedSortingChanged(bool current) override;
    void onSortRoleChanged(const QByteArray &current, const QByteArray &previous, bool resortItems = true) override;
    void onSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous) override;

private Q_SLOTS:
    /**
     * Resorts all items dependent on the set sortRole(), sortOrder()
     * and foldersFirst() settings.
     */
    void resortAllItems();

    void slotCompleted();
    void slotCanceled();
    void slotItemsAdded(const QUrl &directoryUrl, const KFileItemList &items);
    void slotItemsDeleted(const KFileItemList &items);
    void slotRefreshItems(const QList<QPair<KFileItem, KFileItem>> &items);
    void slotClear();
    void slotSortingChoiceChanged();
    void slotListerError(KIO::Job *job);

    void dispatchPendingItemsToInsert();

private:
    enum RoleType {
        // User visible roles:
        NoRole,
        NameRole,
        SizeRole,
        ModificationTimeRole,
        ModificationTimeDayWiseRole,
        CreationTimeRole,
        CreationTimeDayWiseRole,
        AccessTimeRole,
        AccessTimeDayWiseRole,
        PermissionsRole,
        OwnerRole,
        GroupRole,
        TypeRole,
        ExtensionRole,
        DestinationRole,
        PathRole,
        DeletionTimeRole,
        DeletionTimeDayWiseRole,
        // User visible roles available with Baloo:
        CommentRole,
        TagsRole,
        RatingRole,
        DimensionsRole,
        WidthRole,
        HeightRole,
        ImageDateTimeRole,
        OrientationRole,
        PublisherRole,
        PageCountRole,
        WordCountRole,
        TitleRole,
        AuthorRole,
        LineCountRole,
        ArtistRole,
        GenreRole,
        AlbumRole,
        DurationRole,
        TrackRole,
        ReleaseYearRole,
        BitrateRole,
        OriginUrlRole,
        AspectRatioRole,
        FrameRateRole,
        // Non-visible roles:
        IsDirRole,
        IsLinkRole,
        IsHiddenRole,
        IsExpandedRole,
        IsExpandableRole,
        ExpandedParentsCountRole,
        // Mandatory last entry:
        RolesCount
    };

    struct ItemData {
        KFileItem item;
        QHash<QByteArray, QVariant> values;
        ItemData *parent;
    };

    enum RemoveItemsBehavior { KeepItemData, DeleteItemData, DeleteItemDataIfUnfiltered };

    void insertItems(QList<ItemData *> &items);
    void removeItems(const KItemRangeList &itemRanges, RemoveItemsBehavior behavior);

    /**
     * Helper method for insertItems() and removeItems(): Creates
     * a list of ItemData elements based on the given items.
     * Note that the ItemData instances are created dynamically and
     * must be deleted by the caller.
     */
    QList<ItemData *> createItemDataList(const QUrl &parentUrl, const KFileItemList &items) const;

    /**
     * Prepares the items for sorting. Normally, the hash 'values' in ItemData is filled
     * lazily to save time and memory, but for some sort roles, it is expected that the
     * sort role data is stored in 'values'.
     */
    void prepareItemsForSorting(QList<ItemData *> &itemDataList);

    static int expandedParentsCount(const ItemData *data);

    void removeExpandedItems();

    /**
     * This function is called by setData() and slotRefreshItems(). It emits
     * the itemsChanged() signal, checks if the sort order is still correct,
     * and starts m_resortAllItemsTimer if that is not the case.
     */
    void emitItemsChangedAndTriggerResorting(const KItemRangeList &itemRanges, const QSet<QByteArray> &changedRoles);

    /**
     * Resets all values from m_requestRole to false.
     */
    void resetRoles();

    /**
     * @return Role-type for the given role.
     *         Runtime complexity is O(1).
     */
    RoleType typeForRole(const QByteArray &role) const;

    /**
     * @return Role-byte-array for the given role-type.
     *         Runtime complexity is O(1).
     */
    QByteArray roleForType(RoleType roleType) const;

    QHash<QByteArray, QVariant> retrieveData(const KFileItem &item, const ItemData *parent) const;

    /**
     * @return True if role values benefit from natural or case insensitive sorting.
     */
    static bool isRoleValueNatural(const RoleType roleType);

    /**
     * @return True if \a a has a KFileItem whose text is 'less than' the one
     *         of \a b according to QString::operator<(const QString&).
     */
    static bool nameLessThan(const ItemData *a, const ItemData *b);

    /**
     * @return True if the item-data \a a should be ordered before the item-data
     *         \b. The item-data may have different parent-items.
     */
    bool lessThan(const ItemData *a, const ItemData *b, const QCollator &collator) const;

    /**
     * Sorts the items between \a begin and \a end using the comparison
     * function lessThan().
     */
    void sort(const QList<ItemData *>::iterator &begin, const QList<ItemData *>::iterator &end) const;

    /**
     * Helper method for lessThan() and expandedParentsCountCompare(): Compares
     * the passed item-data using m_sortRole as criteria. Both items must
     * have the same parent item, otherwise the comparison will be wrong.
     */
    int sortRoleCompare(const ItemData *a, const ItemData *b, const QCollator &collator) const;

    int stringCompare(const QString &a, const QString &b, const QCollator &collator) const;

    QList<QPair<int, QVariant>> nameRoleGroups() const;
    QList<QPair<int, QVariant>> sizeRoleGroups() const;
    QList<QPair<int, QVariant>> timeRoleGroups(const std::function<QDateTime(const ItemData *)> &fileTimeCb, bool daywise = false) const;
    QList<QPair<int, QVariant>> permissionRoleGroups() const;
    QList<QPair<int, QVariant>> ratingRoleGroups() const;
    QList<QPair<int, QVariant>> genericStringRoleGroups(const QByteArray &typeForRole) const;

    /**
     * Helper method for all xxxRoleGroups() methods to check whether the
     * item with the given index is a child-item. A child-item is defined
     * as item having an expansion-level > 0. All xxxRoleGroups() methods
     * should skip the grouping if the item is a child-item (although
     * KItemListView would be capable to show sub-groups in groups this
     * results in visual clutter for most usecases).
     */
    bool isChildItem(int index) const;

    /**
     * Is invoked by KFileItemModelRolesUpdater and results in emitting the
     * sortProgress signal with a percent-value of the progress.
     */
    void emitSortProgress(int resolvedCount);

    /**
     * Applies the filters set through @ref setNameFilter and @ref setMimeTypeFilters.
     */
    void applyFilters();

    /**
     * Removes filtered items whose expanded parents have been deleted
     * or collapsed via setExpanded(parentIndex, false).
     */
    void removeFilteredChildren(const KItemRangeList &parents);

    /**
     * Loads the selected choice of sorting method from Dolphin General Settings
     */
    void loadSortingSettings();

    /**
     * Maps the QByteArray-roles to RoleTypes and provides translation- and
     * group-contexts.
     */
    struct RoleInfoMap {
        const char *const role;
        const RoleType roleType;
        const KLazyLocalizedString roleTranslation;
        const KLazyLocalizedString groupTranslation;
        const KLazyLocalizedString tooltipTranslation;
        const bool requiresBaloo;
        const bool requiresIndexer;
    };

    /**
     * @return Map of user visible roles that are accessible by KFileItemModel::rolesInformation().
     */
    static const RoleInfoMap *rolesInfoMap(int &count);

    /**
     * Determines the MIME-types of all items that can be done within
     * the given timeout.
     */
    static void determineMimeTypes(const KFileItemList &items, int timeout);

    /**
     * @return Returns a copy of \a value that is implicitly shared
     * with other users to save memory.
     */
    static QByteArray sharedValue(const QByteArray &value);

    /**
     * Checks if the model's internal data structures are consistent.
     */
    bool isConsistent() const;

    /**
     * Filters out the expanded folders that don't pass the filter themselves and don't have any filter-passing children.
     * Will update the removedItemRanges arguments to include the parents that have been filtered.
     * @returns the number of parents that have been filtered.
     * @param removedItemRanges The ranges of items being deleted/filtered, will get updated
     * @param parentsToEnsureVisible Parents that must be visible no matter what due to being ancestors of newly visible items
     */
    int filterChildlessParents(KItemRangeList &removedItemRanges, const QSet<ItemData *> &parentsToEnsureVisible = QSet<ItemData *>());

private:
    KDirLister *m_dirLister = nullptr;

    QCollator m_collator;
    bool m_naturalSorting;
    bool m_sortDirsFirst;
    bool m_sortHiddenLast;

    RoleType m_sortRole;
    int m_sortingProgressPercent; // Value of directorySortingProgress() signal
    QSet<QByteArray> m_roles;

    QList<ItemData *> m_itemData;

    // m_items is a cache for the method index(const QUrl&). If it contains N
    // entries, it is guaranteed that these correspond to the first N items in
    // the model, i.e., that (for every i between 0 and N - 1)
    // m_items.value(fileItem(i).url()) == i
    mutable QHash<QUrl, int> m_items;

    KFileItemModelFilter m_filter;
    QHash<KFileItem, ItemData *> m_filteredItems; // Items that got hidden by KFileItemModel::setNameFilter()

    bool m_requestRole[RolesCount];

    QTimer *m_maximumUpdateIntervalTimer;
    QTimer *m_resortAllItemsTimer;
    QList<ItemData *> m_pendingItemsToInsert;

    // Cache for KFileItemModel::groups()
    mutable QList<QPair<int, QVariant>> m_groups;

    // Stores the URLs (key: target url, value: url) of the expanded directories.
    QHash<QUrl, QUrl> m_expandedDirs;

    // URLs that must be expanded. The expanding is initially triggered in setExpanded()
    // and done step after step in slotCompleted().
    QSet<QUrl> m_urlsToExpand;

    friend class KFileItemModelRolesUpdater; // Accesses emitSortProgress() method
    friend class KFileItemModelTest; // For unit testing
    friend class KFileItemModelBenchmark; // For unit testing
    friend class KFileItemListViewTest; // For unit testing
    friend class DolphinPart; // Accesses m_dirLister
};

inline bool KFileItemModel::isRoleValueNatural(RoleType roleType)
{
    return (roleType == TypeRole || roleType == ExtensionRole || roleType == TagsRole || roleType == CommentRole || roleType == TitleRole
            || roleType == ArtistRole || roleType == GenreRole || roleType == AlbumRole || roleType == PathRole || roleType == DestinationRole
            || roleType == OriginUrlRole || roleType == OwnerRole || roleType == GroupRole);
}

inline bool KFileItemModel::nameLessThan(const ItemData *a, const ItemData *b)
{
    return a->item.text() < b->item.text();
}

inline bool KFileItemModel::isChildItem(int index) const
{
    if (m_itemData.at(index)->parent) {
        return true;
    } else {
        return false;
    }
}

#endif
