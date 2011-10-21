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

#include <libdolphin_export.h>
#include <KFileItemList>
#include <KUrl>
#include <kitemviews/kitemmodelbase.h>

#include <QHash>

class KDirLister;
class QTimer;

/**
 * @brief KItemModelBase implementation for KFileItems.
 *
 * KFileItemModel is connected with one KDirLister. Each time the KDirLister
 * emits new items, removes items or changes items the model gets synchronized.
 *
 * KFileItemModel supports sorting and grouping of items. Additional roles that
 * are not part of KFileItem can be added with KFileItemModel::setData().
 *
 * Also the recursive expansion of sub-directories is supported by
 * KFileItemModel::setExpanded().
 */
class LIBDOLPHINPRIVATE_EXPORT KFileItemModel : public KItemModelBase
{
    Q_OBJECT

public:
    explicit KFileItemModel(KDirLister* dirLister, QObject* parent = 0);
    virtual ~KFileItemModel();

    virtual int count() const;
    virtual QHash<QByteArray, QVariant> data(int index) const;
    virtual bool setData(int index, const QHash<QByteArray, QVariant>& values);

    /**
     * Sets a separate sorting with folders first (true) or a mixed sorting of files and folders (false).
     */
    void setSortFoldersFirst(bool foldersFirst);
    bool sortFoldersFirst() const;

    /** @reimp */
    virtual QMimeData* createMimeData(const QSet<int>& indexes) const;

    /** @reimp */
    virtual int indexForKeyboardSearch(const QString& text, int startFromIndex = 0) const;

    /** @reimp */
    virtual bool supportsDropping(int index) const;

    /** @reimp */
    virtual QString roleDescription(const QByteArray& role) const;

    /** @reimp */
    virtual QList<QPair<int, QVariant> > groups() const;

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
    KFileItem fileItem(const KUrl& url) const;

    /**
     * @return The index for the file-item \a item. -1 is returned if no file-item
     *         is found or if the file-item is null. The runtime
     *         complexity of this call is O(1).
     */
    int index(const KFileItem& item) const;

    /**
     * @return The index for the URL \a url. -1 is returned if no file-item
     *         is found. The runtime complexity of this call is O(1).
     */
    int index(const KUrl& url) const;

    /**
     * @return Root item of all items.
     */
    KFileItem rootItem() const;

    /**
     * Clears all items of the model.
     */
    void clear();

    // TODO: "name" + "isDir" is default in ctor
    void setRoles(const QSet<QByteArray>& roles);
    QSet<QByteArray> roles() const;

    bool setExpanded(int index, bool expanded);
    bool isExpanded(int index) const;
    bool isExpandable(int index) const;
    QSet<KUrl> expandedUrls() const;
    void restoreExpandedUrls(const QSet<KUrl>& urls);

signals:
    void loadingCompleted();

protected:
    virtual void onGroupedSortingChanged(bool current);
    virtual void onSortRoleChanged(const QByteArray& current, const QByteArray& previous);
    virtual void onSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous);

private slots:
    void slotCompleted();
    void slotCanceled();
    void slotNewItems(const KFileItemList& items);
    void slotItemsDeleted(const KFileItemList& items);
    void slotRefreshItems(const QList<QPair<KFileItem, KFileItem> >& items);
    void slotClear();
    void slotClear(const KUrl& url);

    void dispatchPendingItemsToInsert();

private:
    void insertItems(const KFileItemList& items);
    void removeItems(const KFileItemList& items);

    void resortAllItems();

    void removeExpandedItems();

    enum Role {
        NoRole,
        NameRole,
        SizeRole,
        DateRole,
        PermissionsRole,
        OwnerRole,
        GroupRole,
        TypeRole,
        DestinationRole,
        PathRole,
        IsDirRole,
        IsExpandedRole,
        ExpansionLevelRole,
        RolesCount // Mandatory last entry
    };

    void resetRoles();

    Role roleIndex(const QByteArray& role) const;

    QHash<QByteArray, QVariant> retrieveData(const KFileItem& item) const;

    bool lessThan(const KFileItem& a, const KFileItem& b) const;
    void sort(const KFileItemList::iterator& start, const KFileItemList::iterator& end);
    int stringCompare(const QString& a, const QString& b) const;

    /**
     * Compares the expansion level of both items. The "expansion level" is defined
     * by the number of parent directories. However simply comparing just the numbers
     * is not sufficient, it is also important to check the hierarchy for having
     * a correct order like shown in a tree.
     */
    int expansionLevelsCompare(const KFileItem& a, const KFileItem& b) const;

    /**
     * Helper method for expansionLevelCompare().
     */
    QString subPath(const KFileItem& item,
                    const QString& itemPath,
                    int start,
                    bool* isDir) const;

    bool useMaximumUpdateInterval() const;

    QList<QPair<int, QVariant> > nameRoleGroups() const;
    QList<QPair<int, QVariant> > sizeRoleGroups() const;
    QList<QPair<int, QVariant> > dateRoleGroups() const;
    QList<QPair<int, QVariant> > permissionRoleGroups() const;
    QList<QPair<int, QVariant> > ownerRoleGroups() const;
    QList<QPair<int, QVariant> > groupRoleGroups() const;
    QList<QPair<int, QVariant> > typeRoleGroups() const;
    QList<QPair<int, QVariant> > destinationRoleGroups() const;
    QList<QPair<int, QVariant> > pathRoleGroups() const;

private:
    QWeakPointer<KDirLister> m_dirLister;

    bool m_naturalSorting;
    bool m_sortFoldersFirst;

    Role m_sortRole;
    Qt::CaseSensitivity m_caseSensitivity;

    KFileItemList m_sortedItems;   // Allows O(1) access for KFileItemModel::fileItem(int index)
    QHash<KUrl, int> m_items;      // Allows O(1) access for KFileItemModel::index(const KFileItem& item)
    QList<QHash<QByteArray, QVariant> > m_data;

    bool m_requestRole[RolesCount];

    QTimer* m_minimumUpdateIntervalTimer;
    QTimer* m_maximumUpdateIntervalTimer;
    KFileItemList m_pendingItemsToInsert;
    bool m_pendingEmitLoadingCompleted;

    // Cache for KFileItemModel::groups()
    mutable QList<QPair<int, QVariant> > m_groups;

    // Stores the smallest expansion level of the root-URL. Is required to calculate
    // the "expansionLevel" role in an efficient way. A value < 0 indicates that
    // it has not been initialized yet.
    mutable int m_rootExpansionLevel;

    // Stores the URLs of the expanded folders.
    QSet<KUrl> m_expandedUrls;

    // Stores the URLs which have to be expanded in order to restore a previous state of the model.
    QSet<KUrl> m_restoredExpandedUrls;

    friend class KFileItemModelTest; // For unit testing
};

#endif


