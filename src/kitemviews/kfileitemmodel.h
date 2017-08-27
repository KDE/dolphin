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

#ifndef KFILEITEMMODEL_H
#define KFILEITEMMODEL_H

#include "dolphin_export.h"
#include <KFileItem>
#include <QUrl>
#include <kitemviews/kitemmodelbase.h>
#include <kitemviews/private/kfileitemmodelfilter.h>

#include <QCollator>
#include <QHash>
#include <QSet>

#include <functional>

class KFileItemModelDirLister;
class QTimer;

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
    explicit KFileItemModel(QObject* parent = 0);
    virtual ~KFileItemModel();

    /**
     * Loads the directory specified by \a url. The signals
     * directoryLoadingStarted(), directoryLoadingProgress() and directoryLoadingCompleted()
     * indicate the current state of the loading process. The items
     * of the directory are added after the loading has been completed.
     */
    void loadDirectory(const QUrl& url);

    /**
     * Throws away all currently loaded items and refreshes the directory
     * by reloading all items again.
     */
    void refreshDirectory(const QUrl& url);

    /**
     * @return Parent directory of the items that are shown. In case
     *         if a directory tree is shown, KFileItemModel::dir() returns
     *         the root-parent of all items.
     * @see rootItem()
     */
    QUrl directory() const;

    /**
     * Cancels the loading of a directory which has been started by either
     * loadDirectory() or refreshDirectory().
     */
    void cancelDirectoryLoading();

    virtual int count() const Q_DECL_OVERRIDE;
    virtual QHash<QByteArray, QVariant> data(int index) const Q_DECL_OVERRIDE;
    virtual bool setData(int index, const QHash<QByteArray, QVariant>& values) Q_DECL_OVERRIDE;

    /**
     * Sets a separate sorting with directories first (true) or a mixed
     * sorting of files and directories (false).
     */
    void setSortDirectoriesFirst(bool dirsFirst);
    bool sortDirectoriesFirst() const;

    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const;

    /**
     * If set to true, only directories are shown as items of the model. Files
     * are ignored.
     */
    void setShowDirectoriesOnly(bool enabled);
    bool showDirectoriesOnly() const;

    virtual QMimeData* createMimeData(const KItemSet& indexes) const Q_DECL_OVERRIDE;

    virtual int indexForKeyboardSearch(const QString& text, int startFromIndex = 0) const Q_DECL_OVERRIDE;

    virtual bool supportsDropping(int index) const Q_DECL_OVERRIDE;

    virtual QString roleDescription(const QByteArray& role) const Q_DECL_OVERRIDE;

    virtual QList<QPair<int, QVariant> > groups() const Q_DECL_OVERRIDE;

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
    KFileItem fileItem(const QUrl& url) const;

    /**
     * @return The index for the file-item \a item. -1 is returned if no file-item
     *         is found or if the file-item is null. The amortized runtime
     *         complexity of this call is O(1).
     */
    int index(const KFileItem& item) const;

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
    void setRoles(const QSet<QByteArray>& roles);
    QSet<QByteArray> roles() const;

    virtual bool setExpanded(int index, bool expanded) Q_DECL_OVERRIDE;
    virtual bool isExpanded(int index) const Q_DECL_OVERRIDE;
    virtual bool isExpandable(int index) const Q_DECL_OVERRIDE;
    virtual int expandedParentsCount(int index) const Q_DECL_OVERRIDE;

    QSet<QUrl> expandedDirectories() const;

    /**
     * Marks the URLs in \a urls as sub-directories which were expanded previously.
     * After calling loadDirectory() or refreshDirectory() the marked sub-directories
     * will be expanded step-by-step.
     */
    void restoreExpandedDirectories(const QSet<QUrl>& urls);

    /**
     * Expands all parent-directories of the item \a url.
     */
    void expandParentDirectories(const QUrl& url);

    void setNameFilter(const QString& nameFilter);
    QString nameFilter() const;

    void setMimeTypeFilters(const QStringList& filters);
    QStringList mimeTypeFilters() const;

    struct RoleInfo
    {   QByteArray role;
        QString translation;
        QString group;
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

signals:
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
    void infoMessage(const QString& message);

    /**
     * Is emitted if an error message (e.g. "Unknown location")
     * should be shown.
     */
    void errorMessage(const QString& message);

    /**
     * Is emitted if a redirection from the current URL \a oldUrl
     * to the new URL \a newUrl has been done.
     */
    void directoryRedirection(const QUrl& oldUrl, const QUrl& newUrl);

    /**
     * Is emitted when the URL passed by KFileItemModel::setUrl() represents a file.
     * In this case no signal errorMessage() will be emitted.
     */
    void urlIsFileError(const QUrl& url);

protected:
    virtual void onGroupedSortingChanged(bool current) Q_DECL_OVERRIDE;
    virtual void onSortRoleChanged(const QByteArray& current, const QByteArray& previous) Q_DECL_OVERRIDE;
    virtual void onSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous) Q_DECL_OVERRIDE;

private slots:
    /**
     * Resorts all items dependent on the set sortRole(), sortOrder()
     * and foldersFirst() settings.
     */
    void resortAllItems();

    void slotCompleted();
    void slotCanceled();
    void slotItemsAdded(const QUrl& directoryUrl, const KFileItemList& items);
    void slotItemsDeleted(const KFileItemList& items);
    void slotRefreshItems(const QList<QPair<KFileItem, KFileItem> >& items);
    void slotClear();
    void slotSortingChoiceChanged();

    void dispatchPendingItemsToInsert();

private:
    enum RoleType {
        // User visible roles:
        NoRole, NameRole, SizeRole, ModificationTimeRole, CreationTimeRole, AccessTimeRole, PermissionsRole, OwnerRole,
        GroupRole, TypeRole, DestinationRole, PathRole, DeletionTimeRole,
        // User visible roles available with Baloo:
        CommentRole, TagsRole, RatingRole, ImageSizeRole, OrientationRole,
        WordCountRole, TitleRole, LineCountRole, ArtistRole, GenreRole, AlbumRole, DurationRole, TrackRole,
        OriginUrlRole,
        // Non-visible roles:
        IsDirRole, IsLinkRole, IsHiddenRole, IsExpandedRole, IsExpandableRole, ExpandedParentsCountRole,
        // Mandatory last entry:
        RolesCount
    };

    struct ItemData
    {
        KFileItem item;
        QHash<QByteArray, QVariant> values;
        ItemData* parent;
    };

    enum RemoveItemsBehavior {
        KeepItemData,
        DeleteItemData
    };

    void insertItems(QList<ItemData*>& items);
    void removeItems(const KItemRangeList& itemRanges, RemoveItemsBehavior behavior);

    /**
     * Helper method for insertItems() and removeItems(): Creates
     * a list of ItemData elements based on the given items.
     * Note that the ItemData instances are created dynamically and
     * must be deleted by the caller.
     */
    QList<ItemData*> createItemDataList(const QUrl& parentUrl, const KFileItemList& items) const;

    /**
     * Prepares the items for sorting. Normally, the hash 'values' in ItemData is filled
     * lazily to save time and memory, but for some sort roles, it is expected that the
     * sort role data is stored in 'values'.
     */
    void prepareItemsForSorting(QList<ItemData*>& itemDataList);

    static int expandedParentsCount(const ItemData* data);

    void removeExpandedItems();

    /**
     * This function is called by setData() and slotRefreshItems(). It emits
     * the itemsChanged() signal, checks if the sort order is still correct,
     * and starts m_resortAllItemsTimer if that is not the case.
     */
    void emitItemsChangedAndTriggerResorting(const KItemRangeList& itemRanges, const QSet<QByteArray>& changedRoles);

    /**
     * Resets all values from m_requestRole to false.
     */
    void resetRoles();

    /**
     * @return Role-type for the given role.
     *         Runtime complexity is O(1).
     */
    RoleType typeForRole(const QByteArray& role) const;

    /**
     * @return Role-byte-array for the given role-type.
     *         Runtime complexity is O(1).
     */
    QByteArray roleForType(RoleType roleType) const;

    QHash<QByteArray, QVariant> retrieveData(const KFileItem& item, const ItemData* parent) const;

    /**
     * @return True if \a a has a KFileItem whose text is 'less than' the one
     *         of \a b according to QString::operator<(const QString&).
     */
    static bool nameLessThan(const ItemData* a, const ItemData* b);

    /**
     * @return True if the item-data \a a should be ordered before the item-data
     *         \b. The item-data may have different parent-items.
     */
    bool lessThan(const ItemData* a, const ItemData* b, const QCollator& collator) const;

    /**
     * Sorts the items between \a begin and \a end using the comparison
     * function lessThan().
     */
    void sort(QList<ItemData*>::iterator begin, QList<ItemData*>::iterator end) const;

    /**
     * Helper method for lessThan() and expandedParentsCountCompare(): Compares
     * the passed item-data using m_sortRole as criteria. Both items must
     * have the same parent item, otherwise the comparison will be wrong.
     */
    int sortRoleCompare(const ItemData* a, const ItemData* b, const QCollator& collator) const;

    int stringCompare(const QString& a, const QString& b, const QCollator& collator) const;

    bool useMaximumUpdateInterval() const;

    QList<QPair<int, QVariant> > nameRoleGroups() const;
    QList<QPair<int, QVariant> > sizeRoleGroups() const;
    QList<QPair<int, QVariant> > timeRoleGroups(std::function<QDateTime(const ItemData *)> fileTimeCb) const;
    QList<QPair<int, QVariant> > permissionRoleGroups() const;
    QList<QPair<int, QVariant> > ratingRoleGroups() const;
    QList<QPair<int, QVariant> > genericStringRoleGroups(const QByteArray& typeForRole) const;

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
    void removeFilteredChildren(const KItemRangeList& parents);

    /**
     * Loads the selected choice of sorting method from Dolphin General Settings
     */
    void loadSortingSettings();

    /**
     * Maps the QByteArray-roles to RoleTypes and provides translation- and
     * group-contexts.
     */
    struct RoleInfoMap
    {
        const char* const role;
        const RoleType roleType;
        const char* const roleTranslationContext;
        const char* const roleTranslation;
        const char* const groupTranslationContext;
        const char* const groupTranslation;
        const bool requiresBaloo;
        const bool requiresIndexer;
    };

    /**
     * @return Map of user visible roles that are accessible by KFileItemModel::rolesInformation().
     */
    static const RoleInfoMap* rolesInfoMap(int& count);

    /**
     * Determines the MIME-types of all items that can be done within
     * the given timeout.
     */
    static void determineMimeTypes(const KFileItemList& items, int timeout);

    /**
     * @return Returns a copy of \a value that is implicitly shared
     * with other users to save memory.
     */
    static QByteArray sharedValue(const QByteArray& value);

    /**
     * Checks if the model's internal data structures are consistent.
     */
    bool isConsistent() const;

private:
    KFileItemModelDirLister* m_dirLister;

    QCollator m_collator;
    bool m_naturalSorting;
    bool m_sortDirsFirst;

    RoleType m_sortRole;
    int m_sortingProgressPercent; // Value of directorySortingProgress() signal
    QSet<QByteArray> m_roles;

    QList<ItemData*> m_itemData;

    // m_items is a cache for the method index(const QUrl&). If it contains N
    // entries, it is guaranteed that these correspond to the first N items in
    // the model, i.e., that (for every i between 0 and N - 1)
    // m_items.value(fileItem(i).url()) == i
    mutable QHash<QUrl, int> m_items;

    KFileItemModelFilter m_filter;
    QHash<KFileItem, ItemData*> m_filteredItems; // Items that got hidden by KFileItemModel::setNameFilter()

    bool m_requestRole[RolesCount];

    QTimer* m_maximumUpdateIntervalTimer;
    QTimer* m_resortAllItemsTimer;
    QList<ItemData*> m_pendingItemsToInsert;

    // Cache for KFileItemModel::groups()
    mutable QList<QPair<int, QVariant> > m_groups;

    // Stores the URLs (key: target url, value: url) of the expanded directories.
    QHash<QUrl, QUrl> m_expandedDirs;

    // URLs that must be expanded. The expanding is initially triggered in setExpanded()
    // and done step after step in slotCompleted().
    QSet<QUrl> m_urlsToExpand;

    friend class KFileItemModelLessThan;       // Accesses lessThan() method
    friend class KFileItemModelRolesUpdater;   // Accesses emitSortProgress() method
    friend class KFileItemModelTest;           // For unit testing
    friend class KFileItemModelBenchmark;      // For unit testing
    friend class KFileItemListViewTest;        // For unit testing
    friend class DolphinPart;                  // Accesses m_dirLister
};

inline bool KFileItemModel::nameLessThan(const ItemData* a, const ItemData* b)
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


