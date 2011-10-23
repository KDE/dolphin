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

#include "kfileitemmodel.h"

#include <KDirLister>
#include <KDirModel>
#include <KLocale>
#include <KStringHandler>
#include <KDebug>

#include <QMimeData>
#include <QTimer>

#define KFILEITEMMODEL_DEBUG

KFileItemModel::KFileItemModel(KDirLister* dirLister, QObject* parent) :
    KItemModelBase("name", parent),
    m_dirLister(dirLister),
    m_naturalSorting(true),
    m_sortFoldersFirst(true),
    m_sortRole(NameRole),
    m_caseSensitivity(Qt::CaseInsensitive),
    m_sortedItems(),
    m_items(),
    m_data(),
    m_requestRole(),
    m_minimumUpdateIntervalTimer(0),
    m_maximumUpdateIntervalTimer(0),
    m_pendingItemsToInsert(),
    m_pendingEmitLoadingCompleted(false),
    m_groups(),
    m_rootExpansionLevel(-1),
    m_expandedUrls(),
    m_restoredExpandedUrls()
{
    resetRoles();
    m_requestRole[NameRole] = true;
    m_requestRole[IsDirRole] = true;

    Q_ASSERT(dirLister);

    connect(dirLister, SIGNAL(canceled()), this, SLOT(slotCanceled()));
    connect(dirLister, SIGNAL(completed()), this, SLOT(slotCompleted()));
    connect(dirLister, SIGNAL(newItems(KFileItemList)), this, SLOT(slotNewItems(KFileItemList)));
    connect(dirLister, SIGNAL(itemsDeleted(KFileItemList)), this, SLOT(slotItemsDeleted(KFileItemList)));
    connect(dirLister, SIGNAL(refreshItems(QList<QPair<KFileItem,KFileItem> >)), this, SLOT(slotRefreshItems(QList<QPair<KFileItem,KFileItem> >)));
    connect(dirLister, SIGNAL(clear()), this, SLOT(slotClear()));
    connect(dirLister, SIGNAL(clear(KUrl)), this, SLOT(slotClear(KUrl)));

    // Although the layout engine of KItemListView is fast it is very inefficient to e.g.
    // emit 50 itemsInserted()-signals each 100 ms. m_minimumUpdateIntervalTimer assures that updates
    // are done in 1 second intervals for equal operations.
    m_minimumUpdateIntervalTimer = new QTimer(this);
    m_minimumUpdateIntervalTimer->setInterval(1000);
    m_minimumUpdateIntervalTimer->setSingleShot(true);
    connect(m_minimumUpdateIntervalTimer, SIGNAL(timeout()), this, SLOT(dispatchPendingItemsToInsert()));

    // For slow KIO-slaves like used for searching it makes sense to show results periodically even
    // before the completed() or canceled() signal has been emitted.
    m_maximumUpdateIntervalTimer = new QTimer(this);
    m_maximumUpdateIntervalTimer->setInterval(2000);
    m_maximumUpdateIntervalTimer->setSingleShot(true);
    connect(m_maximumUpdateIntervalTimer, SIGNAL(timeout()), this, SLOT(dispatchPendingItemsToInsert()));

    Q_ASSERT(m_minimumUpdateIntervalTimer->interval() <= m_maximumUpdateIntervalTimer->interval());
}

KFileItemModel::~KFileItemModel()
{
}

int KFileItemModel::count() const
{
    return m_data.count();
}

QHash<QByteArray, QVariant> KFileItemModel::data(int index) const
{
    if (index >= 0 && index < count()) {
        return m_data.at(index);
    }
    return QHash<QByteArray, QVariant>();
}

bool KFileItemModel::setData(int index, const QHash<QByteArray, QVariant>& values)
{
    if (index >= 0 && index < count()) {
        QHash<QByteArray, QVariant> currentValue = m_data.at(index);

        QSet<QByteArray> changedRoles;
        QHashIterator<QByteArray, QVariant> it(values);
        while (it.hasNext()) {
            it.next();
            const QByteArray role = it.key();
            const QVariant value = it.value();

            if (currentValue[role] != value) {
                currentValue[role] = value;
                changedRoles.insert(role);
            }
        }

        if (!changedRoles.isEmpty()) {
            m_data[index] = currentValue;
            emit itemsChanged(KItemRangeList() << KItemRange(index, 1), changedRoles);
        }

        return true;
    }
    return false;
}

void KFileItemModel::setSortFoldersFirst(bool foldersFirst)
{
    if (foldersFirst != m_sortFoldersFirst) {
        m_sortFoldersFirst = foldersFirst;
        resortAllItems();
    }
}

bool KFileItemModel::sortFoldersFirst() const
{
    return m_sortFoldersFirst;
}

QMimeData* KFileItemModel::createMimeData(const QSet<int>& indexes) const
{
    QMimeData* data = new QMimeData();

    // The following code has been taken from KDirModel::mimeData()
    // (kdelibs/kio/kio/kdirmodel.cpp)
    // Copyright (C) 2006 David Faure <faure@kde.org>
    KUrl::List urls;
    KUrl::List mostLocalUrls;
    bool canUseMostLocalUrls = true;

    QSetIterator<int> it(indexes);
    while (it.hasNext()) {
        const int index = it.next();
        const KFileItem item = fileItem(index);
        if (!item.isNull()) {
            urls << item.url();

            bool isLocal;
            mostLocalUrls << item.mostLocalUrl(isLocal);
            if (!isLocal) {
                canUseMostLocalUrls = false;
            }
        }
    }

    const bool different = canUseMostLocalUrls && mostLocalUrls != urls;
    urls = KDirModel::simplifiedUrlList(urls); // TODO: Check if we still need KDirModel for this in KDE 5.0
    if (different) {
        mostLocalUrls = KDirModel::simplifiedUrlList(mostLocalUrls);
        urls.populateMimeData(mostLocalUrls, data);
    } else {
        urls.populateMimeData(data);
    }

    return data;
}

int KFileItemModel::indexForKeyboardSearch(const QString& text, int startFromIndex) const
{
    startFromIndex = qMax(0, startFromIndex);
    for (int i = startFromIndex; i < count(); ++i) {
        if (data(i)["name"].toString().startsWith(text, Qt::CaseInsensitive)) {
            return i;
        }
    }
    for (int i = 0; i < startFromIndex; ++i) {
        if (data(i)["name"].toString().startsWith(text, Qt::CaseInsensitive)) {
            return i;
        }
    }
    return -1;
}

bool KFileItemModel::supportsDropping(int index) const
{
    const KFileItem item = fileItem(index);
    return item.isNull() ? false : item.isDir();
}

QString KFileItemModel::roleDescription(const QByteArray& role) const
{
    QString descr;

    switch (roleIndex(role)) {
    case NameRole:           descr = i18nc("@item:intable", "Name"); break;
    case SizeRole:           descr = i18nc("@item:intable", "Size"); break;
    case DateRole:           descr = i18nc("@item:intable", "Date"); break;
    case PermissionsRole:    descr = i18nc("@item:intable", "Permissions"); break;
    case OwnerRole:          descr = i18nc("@item:intable", "Owner"); break;
    case GroupRole:          descr = i18nc("@item:intable", "Group"); break;
    case TypeRole:           descr = i18nc("@item:intable", "Type"); break;
    case DestinationRole:    descr = i18nc("@item:intable", "Destination"); break;
    case PathRole:           descr = i18nc("@item:intable", "Path"); break;
    case NoRole:             break;
    case IsDirRole:          break;
    case IsExpandedRole:     break;
    case ExpansionLevelRole: break;
    default:                 Q_ASSERT(false); break;
    }

    return descr;
}

QList<QPair<int, QVariant> > KFileItemModel::groups() const
{
    if (!m_data.isEmpty() && m_groups.isEmpty()) {
#ifdef KFILEITEMMODEL_DEBUG
        QElapsedTimer timer;
        timer.start();
#endif
        switch (roleIndex(sortRole())) {
        case NameRole:           m_groups = nameRoleGroups(); break;
        case SizeRole:           m_groups = sizeRoleGroups(); break;
        case DateRole:           m_groups = dateRoleGroups(); break;
        case PermissionsRole:    m_groups = permissionRoleGroups(); break;
        case OwnerRole:          m_groups = ownerRoleGroups(); break;
        case GroupRole:          m_groups = groupRoleGroups(); break;
        case TypeRole:           m_groups = typeRoleGroups(); break;
        case DestinationRole:    m_groups = destinationRoleGroups(); break;
        case PathRole:           m_groups = pathRoleGroups(); break;
        case NoRole:             break;
        case IsDirRole:          break;
        case IsExpandedRole:     break;
        case ExpansionLevelRole: break;
        default:                 Q_ASSERT(false); break;
        }

#ifdef KFILEITEMMODEL_DEBUG
        kDebug() << "[TIME] Calculating groups for" << count() << "items:" << timer.elapsed();
#endif
    }

    return m_groups;
}

KFileItem KFileItemModel::fileItem(int index) const
{
    if (index >= 0 && index < count()) {
        return m_sortedItems.at(index);
    }

    return KFileItem();
}

KFileItem KFileItemModel::fileItem(const KUrl& url) const
{
    const int index = m_items.value(url, -1);
    if (index >= 0) {
        return m_sortedItems.at(index);
    }
    return KFileItem();
}

int KFileItemModel::index(const KFileItem& item) const
{
    if (item.isNull()) {
        return -1;
    }

    return m_items.value(item.url(), -1);
}

int KFileItemModel::index(const KUrl& url) const
{
    KUrl urlToFind = url;
    urlToFind.adjustPath(KUrl::RemoveTrailingSlash);
    return m_items.value(urlToFind, -1);
}

KFileItem KFileItemModel::rootItem() const
{
    const KDirLister* dirLister = m_dirLister.data();
    if (dirLister) {
        return dirLister->rootItem();
    }
    return KFileItem();
}

void KFileItemModel::clear()
{
    slotClear();
}

void KFileItemModel::setRoles(const QSet<QByteArray>& roles)
{
    if (count() > 0) {
        const bool supportedExpanding = m_requestRole[IsExpandedRole] && m_requestRole[ExpansionLevelRole];
        const bool willSupportExpanding = roles.contains("isExpanded") && roles.contains("expansionLevel");
        if (supportedExpanding && !willSupportExpanding) {
            // No expanding is supported anymore. Take care to delete all items that have an expansion level
            // that is not 0 (and hence are part of an expanded item).
            removeExpandedItems();
        }
    }

    resetRoles();
    QSetIterator<QByteArray> it(roles);
    while (it.hasNext()) {
        const QByteArray& role = it.next();
        m_requestRole[roleIndex(role)] = true;
    }

    if (count() > 0) {
        // Update m_data with the changed requested roles
        const int maxIndex = count() - 1;
        for (int i = 0; i <= maxIndex; ++i) {
            m_data[i] = retrieveData(m_sortedItems.at(i));
        }

        kWarning() << "TODO: Emitting itemsChanged() with no information what has changed!";
        emit itemsChanged(KItemRangeList() << KItemRange(0, count()), QSet<QByteArray>());
    }
}

QSet<QByteArray> KFileItemModel::roles() const
{
    QSet<QByteArray> roles;
    for (int i = 0; i < RolesCount; ++i) {
        if (m_requestRole[i]) {
            switch (i) {
            case NoRole:             break;
            case NameRole:           roles.insert("name"); break;
            case SizeRole:           roles.insert("size"); break;
            case DateRole:           roles.insert("date"); break;
            case PermissionsRole:    roles.insert("permissions"); break;
            case OwnerRole:          roles.insert("owner"); break;
            case GroupRole:          roles.insert("group"); break;
            case TypeRole:           roles.insert("type"); break;
            case DestinationRole:    roles.insert("destination"); break;
            case PathRole:           roles.insert("path"); break;
            case IsDirRole:          roles.insert("isDir"); break;
            case IsExpandedRole:     roles.insert("isExpanded"); break;
            case ExpansionLevelRole: roles.insert("expansionLevel"); break;
            default:                 Q_ASSERT(false); break;
            }
        }
    }
    return roles;
}

bool KFileItemModel::setExpanded(int index, bool expanded)
{
    if (isExpanded(index) == expanded || index < 0 || index >= count()) {
        return false;
    }

    QHash<QByteArray, QVariant> values;
    values.insert("isExpanded", expanded);
    if (!setData(index, values)) {
        return false;
    }

    const KUrl url = m_sortedItems.at(index).url();
    if (expanded) {
        m_expandedUrls.insert(url);

        KDirLister* dirLister = m_dirLister.data();
        if (dirLister) {
            dirLister->openUrl(url, KDirLister::Keep);
            return true;
        }
    } else {
        m_expandedUrls.remove(url);

        KFileItemList itemsToRemove;
        const int expansionLevel = data(index)["expansionLevel"].toInt();
        ++index;
        while (index < count() && data(index)["expansionLevel"].toInt() > expansionLevel) {
            itemsToRemove.append(m_sortedItems.at(index));
            ++index;
        }
        removeItems(itemsToRemove);
        return true;
    }

    return false;
}

bool KFileItemModel::isExpanded(int index) const
{
    if (index >= 0 && index < count()) {
        return m_data.at(index).value("isExpanded").toBool();
    }
    return false;
}

bool KFileItemModel::isExpandable(int index) const
{
    if (index >= 0 && index < count()) {
        return m_sortedItems.at(index).isDir();
    }
    return false;
}

QSet<KUrl> KFileItemModel::expandedUrls() const
{
    return m_expandedUrls;
}

void KFileItemModel::restoreExpandedUrls(const QSet<KUrl>& urls)
{
    m_restoredExpandedUrls = urls;
}

void KFileItemModel::onGroupedSortingChanged(bool current)
{
    Q_UNUSED(current);
    m_groups.clear();
}

void KFileItemModel::onSortRoleChanged(const QByteArray& current, const QByteArray& previous)
{
    Q_UNUSED(previous);
    m_sortRole = roleIndex(current);
    resortAllItems();
}

void KFileItemModel::onSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    resortAllItems();
}

void KFileItemModel::slotCompleted()
{
    if (m_restoredExpandedUrls.isEmpty() && m_minimumUpdateIntervalTimer->isActive()) {
        // dispatchPendingItems() will be called when the timer
        // has been expired.
        m_pendingEmitLoadingCompleted = true;
        return;
    }

    m_pendingEmitLoadingCompleted = false;
    dispatchPendingItemsToInsert();

    if (!m_restoredExpandedUrls.isEmpty()) {
        // Try to find a URL that can be expanded.
        // Note that the parent folder must be expanded before any of its subfolders become visible.
        // Therefore, some URLs in m_restoredExpandedUrls might not be visible yet
        // -> we expand the first visible URL we find in m_restoredExpandedUrls.
        foreach(const KUrl& url, m_restoredExpandedUrls) {
            const int index = m_items.value(url, -1);
            if (index >= 0) {
                // We have found an expandable URL. Expand it and return - when
                // the dir lister has finished, this slot will be called again.
                m_restoredExpandedUrls.remove(url);
                setExpanded(index, true);
                return;
            }
        }

        // None of the URLs in m_restoredExpandedUrls could be found in the model. This can happen
        // if these URLs have been deleted in the meantime.
        m_restoredExpandedUrls.clear();
    }

    emit loadingCompleted();
    m_minimumUpdateIntervalTimer->start();
}

void KFileItemModel::slotCanceled()
{
    m_minimumUpdateIntervalTimer->stop();
    m_maximumUpdateIntervalTimer->stop();
    dispatchPendingItemsToInsert();
}

void KFileItemModel::slotNewItems(const KFileItemList& items)
{
    m_pendingItemsToInsert.append(items);

    if (useMaximumUpdateInterval() && !m_maximumUpdateIntervalTimer->isActive()) {
        // Assure that items get dispatched if no completed() or canceled() signal is
        // emitted during the maximum update interval.
        m_maximumUpdateIntervalTimer->start();
    }
}

void KFileItemModel::slotItemsDeleted(const KFileItemList& items)
{
    if (!m_pendingItemsToInsert.isEmpty()) {
        insertItems(m_pendingItemsToInsert);
        m_pendingItemsToInsert.clear();
    }
    removeItems(items);
}

void KFileItemModel::slotRefreshItems(const QList<QPair<KFileItem, KFileItem> >& items)
{
    Q_ASSERT(!items.isEmpty());
#ifdef KFILEITEMMODEL_DEBUG
    kDebug() << "Refreshing" << items.count() << "items";
#endif

    m_groups.clear();

    // Get the indexes of all items that have been refreshed
    QList<int> indexes;
    indexes.reserve(items.count());

    QListIterator<QPair<KFileItem, KFileItem> > it(items);
    while (it.hasNext()) {
        const QPair<KFileItem, KFileItem>& itemPair = it.next();
        const int index = m_items.value(itemPair.second.url(), -1);
        if (index >= 0) {
            indexes.append(index);
        }
    }

    // If the changed items have been created recently, they might not be in m_items yet.
    // In that case, the list 'indexes' might be empty.
    if (indexes.isEmpty()) {
        return;
    }

    // Extract the item-ranges out of the changed indexes
    qSort(indexes);

    KItemRangeList itemRangeList;
    int rangeIndex = 0;
    int rangeCount = 1;
    int previousIndex = indexes.at(0);

    const int maxIndex = indexes.count() - 1;
    for (int i = 1; i <= maxIndex; ++i) {
        const int currentIndex = indexes.at(i);
        if (currentIndex == previousIndex + 1) {
            ++rangeCount;
        } else {
            itemRangeList.append(KItemRange(rangeIndex, rangeCount));

            rangeIndex = currentIndex;
            rangeCount = 1;
        }
        previousIndex = currentIndex;
    }

    if (rangeCount > 0) {
        itemRangeList.append(KItemRange(rangeIndex, rangeCount));
    }

    emit itemsChanged(itemRangeList, QSet<QByteArray>());
}

void KFileItemModel::slotClear()
{
#ifdef KFILEITEMMODEL_DEBUG
    kDebug() << "Clearing all items";
#endif

    m_groups.clear();

    m_minimumUpdateIntervalTimer->stop();
    m_maximumUpdateIntervalTimer->stop();
    m_pendingItemsToInsert.clear();

    m_rootExpansionLevel = -1;

    const int removedCount = m_data.count();
    if (removedCount > 0) {
        m_sortedItems.clear();
        m_items.clear();
        m_data.clear();
        emit itemsRemoved(KItemRangeList() << KItemRange(0, removedCount));
    }

    m_expandedUrls.clear();
}

void KFileItemModel::slotClear(const KUrl& url)
{
    Q_UNUSED(url);
}

void KFileItemModel::dispatchPendingItemsToInsert()
{
    if (!m_pendingItemsToInsert.isEmpty()) {
        insertItems(m_pendingItemsToInsert);
        m_pendingItemsToInsert.clear();
    }

    if (m_pendingEmitLoadingCompleted) {
        emit loadingCompleted();
    }
}

void KFileItemModel::insertItems(const KFileItemList& items)
{
    if (items.isEmpty()) {
        return;
    }

#ifdef KFILEITEMMODEL_DEBUG
    QElapsedTimer timer;
    timer.start();
    kDebug() << "===========================================================";
    kDebug() << "Inserting" << items.count() << "items";
#endif

    m_groups.clear();

    KFileItemList sortedItems = items;
    sort(sortedItems.begin(), sortedItems.end());

#ifdef KFILEITEMMODEL_DEBUG
    kDebug() << "[TIME] Sorting:" << timer.elapsed();
#endif

    KItemRangeList itemRanges;
    int targetIndex = 0;
    int sourceIndex = 0;
    int insertedAtIndex = -1;         // Index for the current item-range
    int insertedCount = 0;            // Count for the current item-range
    int previouslyInsertedCount = 0;  // Sum of previously inserted items for all ranges
    while (sourceIndex < sortedItems.count()) {
        // Find target index from m_items to insert the current item
        // in a sorted order
        const int previousTargetIndex = targetIndex;
        while (targetIndex < m_sortedItems.count()) {
            if (!lessThan(m_sortedItems.at(targetIndex), sortedItems.at(sourceIndex))) {
                break;
            }
            ++targetIndex;
        }

        if (targetIndex - previousTargetIndex > 0 && insertedAtIndex >= 0) {
            itemRanges << KItemRange(insertedAtIndex, insertedCount);
            previouslyInsertedCount += insertedCount;
            insertedAtIndex = targetIndex - previouslyInsertedCount;
            insertedCount = 0;
        }

        // Insert item at the position targetIndex
        const KFileItem item = sortedItems.at(sourceIndex);
        m_sortedItems.insert(targetIndex, item);
        m_data.insert(targetIndex, retrieveData(item));
        // m_items will be inserted after the loop (see comment below)
        ++insertedCount;

        if (insertedAtIndex < 0) {
            insertedAtIndex = targetIndex;
            Q_ASSERT(previouslyInsertedCount == 0);
        }
        ++targetIndex;
        ++sourceIndex;
    }

    // The indexes of all m_items must be adjusted, not only the index
    // of the new items
    for (int i = 0; i < m_sortedItems.count(); ++i) {
        m_items.insert(m_sortedItems.at(i).url(), i);
    }

    itemRanges << KItemRange(insertedAtIndex, insertedCount);
    emit itemsInserted(itemRanges);

#ifdef KFILEITEMMODEL_DEBUG
    kDebug() << "[TIME] Inserting of" << items.count() << "items:" << timer.elapsed();
#endif
}

void KFileItemModel::removeItems(const KFileItemList& items)
{
    if (items.isEmpty()) {
        return;
    }

#ifdef KFILEITEMMODEL_DEBUG
    kDebug() << "Removing " << items.count() << "items";
#endif

    m_groups.clear();

    KFileItemList sortedItems = items;
    sort(sortedItems.begin(), sortedItems.end());

    QList<int> indexesToRemove;
    indexesToRemove.reserve(items.count());

    // Calculate the item ranges that will get deleted
    KItemRangeList itemRanges;
    int removedAtIndex = -1;
    int removedCount = 0;
    int targetIndex = 0;
    foreach (const KFileItem& itemToRemove, sortedItems) {
        const int previousTargetIndex = targetIndex;
        while (targetIndex < m_sortedItems.count()) {
            if (m_sortedItems.at(targetIndex).url() == itemToRemove.url()) {
                break;
            }
            ++targetIndex;
        }
        if (targetIndex >= m_sortedItems.count()) {
            kWarning() << "Item that should be deleted has not been found!";
            return;
        }

        if (targetIndex - previousTargetIndex > 0 && removedAtIndex >= 0) {
            itemRanges << KItemRange(removedAtIndex, removedCount);
            removedAtIndex = targetIndex;
            removedCount = 0;
        }

        indexesToRemove.append(targetIndex);
        if (removedAtIndex < 0) {
            removedAtIndex = targetIndex;
        }
        ++removedCount;
        ++targetIndex;
    }

    // Delete the items
    for (int i = indexesToRemove.count() - 1; i >= 0; --i) {
        const int indexToRemove = indexesToRemove.at(i);
        m_items.remove(m_sortedItems.at(indexToRemove).url());
        m_sortedItems.removeAt(indexToRemove);
        m_data.removeAt(indexToRemove);
    }

    // The indexes of all m_items must be adjusted, not only the index
    // of the removed items
    for (int i = 0; i < m_sortedItems.count(); ++i) {
        m_items.insert(m_sortedItems.at(i).url(), i);
    }

    if (count() <= 0) {
        m_rootExpansionLevel = -1;
    }

    itemRanges << KItemRange(removedAtIndex, removedCount);
    emit itemsRemoved(itemRanges);
}

void KFileItemModel::resortAllItems()
{
    const int itemCount = count();
    if (itemCount <= 0) {
        return;
    }

    m_groups.clear();

    const KFileItemList oldSortedItems = m_sortedItems;
    const QHash<KUrl, int> oldItems = m_items;
    const QList<QHash<QByteArray, QVariant> > oldData = m_data;

    m_items.clear();
    m_data.clear();

    sort(m_sortedItems.begin(), m_sortedItems.end());
    int index = 0;
    foreach (const KFileItem& item, m_sortedItems) {
        m_items.insert(item.url(), index);

        const int oldItemIndex = oldItems.value(item.url());
        m_data.append(oldData.at(oldItemIndex));

        ++index;
    }

    bool emitItemsMoved = false;
    QList<int> movedToIndexes;
    movedToIndexes.reserve(m_sortedItems.count());
    for (int i = 0; i < itemCount; i++) {
        const int newIndex = m_items.value(oldSortedItems.at(i).url());
        movedToIndexes.append(newIndex);
        if (!emitItemsMoved && newIndex != i) {
            emitItemsMoved = true;
        }
    }

    if (emitItemsMoved) {
        emit itemsMoved(KItemRange(0, itemCount), movedToIndexes);
    }
}

void KFileItemModel::removeExpandedItems()
{
    KFileItemList expandedItems;

    const int maxIndex = m_data.count() - 1;
    for (int i = 0; i <= maxIndex; ++i) {
        if (m_data.at(i).value("expansionLevel").toInt() > 0) {
            const KFileItem fileItem = m_sortedItems.at(i);
            expandedItems.append(fileItem);
        }
    }

    // The m_rootExpansionLevel may not get reset before all items with
    // a bigger expansionLevel have been removed.
    Q_ASSERT(m_rootExpansionLevel >= 0);
    removeItems(expandedItems);

    m_rootExpansionLevel = -1;
    m_expandedUrls.clear();
}

void KFileItemModel::resetRoles()
{
    for (int i = 0; i < RolesCount; ++i) {
        m_requestRole[i] = false;
    }
}

KFileItemModel::Role KFileItemModel::roleIndex(const QByteArray& role) const
{
    static QHash<QByteArray, Role> rolesHash;
    if (rolesHash.isEmpty()) {
        rolesHash.insert("name", NameRole);
        rolesHash.insert("size", SizeRole);
        rolesHash.insert("date", DateRole);
        rolesHash.insert("permissions", PermissionsRole);
        rolesHash.insert("owner", OwnerRole);
        rolesHash.insert("group", GroupRole);
        rolesHash.insert("type", TypeRole);
        rolesHash.insert("destination", DestinationRole);
        rolesHash.insert("path", PathRole);
        rolesHash.insert("isDir", IsDirRole);
        rolesHash.insert("isExpanded", IsExpandedRole);
        rolesHash.insert("expansionLevel", ExpansionLevelRole);
    }
    return rolesHash.value(role, NoRole);
}

QHash<QByteArray, QVariant> KFileItemModel::retrieveData(const KFileItem& item) const
{
    // It is important to insert only roles that are fast to retrieve. E.g.
    // KFileItem::iconName() can be very expensive if the MIME-type is unknown
    // and hence will be retrieved asynchronously by KFileItemModelRolesUpdater.
    QHash<QByteArray, QVariant> data;
    data.insert("iconPixmap", QPixmap());

    const bool isDir = item.isDir();
    if (m_requestRole[IsDirRole]) {
        data.insert("isDir", isDir);
    }

    if (m_requestRole[NameRole]) {
        data.insert("name", item.name());
    }

    if (m_requestRole[SizeRole]) {
        if (isDir) {
            data.insert("size", QVariant());
        } else {
            data.insert("size", item.size());
        }
    }

    if (m_requestRole[DateRole]) {
        // Don't use KFileItem::timeString() as this is too expensive when
        // having several thousands of items. Instead the formatting of the
        // date-time will be done on-demand by the view when the date will be shown.
        const KDateTime dateTime = item.time(KFileItem::ModificationTime);
        data.insert("date", dateTime.dateTime());
    }

    if (m_requestRole[PermissionsRole]) {
        data.insert("permissions", item.permissionsString());
    }

    if (m_requestRole[OwnerRole]) {
        data.insert("owner", item.user());
    }

    if (m_requestRole[GroupRole]) {
        data.insert("group", item.group());
    }

    if (m_requestRole[DestinationRole]) {
        QString destination = item.linkDest();
        if (destination.isEmpty()) {
            destination = i18nc("@item:intable", "No destination");
        }
        data.insert("destination", destination);
    }

    if (m_requestRole[PathRole]) {
        data.insert("path", item.localPath());
    }

    if (m_requestRole[IsExpandedRole]) {
        data.insert("isExpanded", false);
    }

    if (m_requestRole[ExpansionLevelRole]) {
        if (m_rootExpansionLevel < 0) {
            KDirLister* dirLister = m_dirLister.data();
            if (dirLister) {
                const QString rootDir = dirLister->url().directory(KUrl::AppendTrailingSlash);
                m_rootExpansionLevel = rootDir.count('/');
            }
        }
        const QString dir = item.url().directory(KUrl::AppendTrailingSlash);
        const int level = dir.count('/') - m_rootExpansionLevel - 1;
        data.insert("expansionLevel", level);
    }

    if (item.isMimeTypeKnown()) {
        data.insert("iconName", item.iconName());

        if (m_requestRole[TypeRole]) {
            data.insert("type", item.mimeComment());
        }
    }

    return data;
}

bool KFileItemModel::lessThan(const KFileItem& a, const KFileItem& b) const
{
    int result = 0;

    if (m_rootExpansionLevel >= 0) {
        result = expansionLevelsCompare(a, b);
        if (result != 0) {
            // The items have parents with different expansion levels
            return (sortOrder() == Qt::AscendingOrder) ? result < 0 : result > 0;
        }
    }

    if (m_sortFoldersFirst || m_sortRole == SizeRole) {
        const bool isDirA = a.isDir();
        const bool isDirB = b.isDir();
        if (isDirA && !isDirB) {
            return true;
        } else if (!isDirA && isDirB) {
            return false;
        }
    }

    switch (m_sortRole) {
    case NameRole: {
        result = stringCompare(a.text(), b.text());
        if (result == 0) {
            // KFileItem::text() may not be unique in case UDS_DISPLAY_NAME is used
            result = stringCompare(a.name(m_caseSensitivity == Qt::CaseInsensitive),
                                   b.name(m_caseSensitivity == Qt::CaseInsensitive));
        }
        break;
    }

    case DateRole: {
        const KDateTime dateTimeA = a.time(KFileItem::ModificationTime);
        const KDateTime dateTimeB = b.time(KFileItem::ModificationTime);
        if (dateTimeA < dateTimeB) {
            result = -1;
        } else if (dateTimeA > dateTimeB) {
            result = +1;
        }
        break;
    }

    case SizeRole: {
        // TODO: Implement sorting folders by the number of items inside.
        // This is more tricky to get right because this number is retrieved
        // asynchronously by KFileItemModelRolesUpdater.
        const KIO::filesize_t sizeA = a.size();
        const KIO::filesize_t sizeB = b.size();
        if (sizeA < sizeB) {
            result = -1;
        } else if (sizeA > sizeB) {
            result = +1;
        }
        break;
    }

    default:
        break;
    }

    if (result == 0) {
        // It must be assured that the sort order is always unique even if two values have been
        // equal. In this case a comparison of the URL is done which is unique in all cases
        // within KDirLister.
        result = QString::compare(a.url().url(), b.url().url(), Qt::CaseSensitive);
    }

    return (sortOrder() == Qt::AscendingOrder) ? result < 0 : result > 0;
}

void KFileItemModel::sort(const KFileItemList::iterator& startIterator, const KFileItemList::iterator& endIterator)
{
    KFileItemList::iterator start = startIterator;
    KFileItemList::iterator end = endIterator;

    // The implementation is based on qSortHelper() from qalgorithms.h
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
    // In opposite to qSort() it allows to use a member-function for the comparison of elements.
    while (1) {
        int span = int(end - start);
        if (span < 2) {
            return;
        }

        --end;
        KFileItemList::iterator low = start, high = end - 1;
        KFileItemList::iterator pivot = start + span / 2;

        if (lessThan(*end, *start)) {
            qSwap(*end, *start);
        }
        if (span == 2) {
            return;
        }

        if (lessThan(*pivot, *start)) {
            qSwap(*pivot, *start);
        }
        if (lessThan(*end, *pivot)) {
            qSwap(*end, *pivot);
        }
        if (span == 3) {
            return;
        }

        qSwap(*pivot, *end);

        while (low < high) {
            while (low < high && lessThan(*low, *end)) {
                ++low;
            }

            while (high > low && lessThan(*end, *high)) {
                --high;
            }
            if (low < high) {
                qSwap(*low, *high);
                ++low;
                --high;
            } else {
                break;
            }
        }

        if (lessThan(*low, *end)) {
            ++low;
        }

        qSwap(*end, *low);
        sort(start, low);

        start = low + 1;
        ++end;
    }
}

int KFileItemModel::stringCompare(const QString& a, const QString& b) const
{
    // Taken from KDirSortFilterProxyModel (kdelibs/kfile/kdirsortfilterproxymodel.*)
    // Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>
    // Copyright (C) 2006 by Dominic Battre <dominic@battre.de>
    // Copyright (C) 2006 by Martin Pool <mbp@canonical.com>

    if (m_caseSensitivity == Qt::CaseInsensitive) {
        const int result = m_naturalSorting ? KStringHandler::naturalCompare(a, b, Qt::CaseInsensitive)
                                            : QString::compare(a, b, Qt::CaseInsensitive);
        if (result != 0) {
            // Only return the result, if the strings are not equal. If they are equal by a case insensitive
            // comparison, still a deterministic sort order is required. A case sensitive
            // comparison is done as fallback.
            return result;
        }
    }

    return m_naturalSorting ? KStringHandler::naturalCompare(a, b, Qt::CaseSensitive)
                            : QString::compare(a, b, Qt::CaseSensitive);
}

int KFileItemModel::expansionLevelsCompare(const KFileItem& a, const KFileItem& b) const
{
    const KUrl urlA = a.url();
    const KUrl urlB = b.url();
    if (urlA.directory() == urlB.directory()) {
        // Both items have the same directory as parent
        return 0;
    }

    // Check whether one item is the parent of the other item
    if (urlA.isParentOf(urlB)) {
        return -1;
    } else if (urlB.isParentOf(urlA)) {
        return +1;
    }

    // Determine the maximum common path of both items and
    // remember the index in 'index'
    const QString pathA = urlA.path();
    const QString pathB = urlB.path();

    const int maxIndex = qMin(pathA.length(), pathB.length()) - 1;
    int index = 0;
    while (index <= maxIndex && pathA.at(index) == pathB.at(index)) {
        ++index;
    }
    if (index > maxIndex) {
        index = maxIndex;
    }
    while ((pathA.at(index) != QLatin1Char('/') || pathB.at(index) != QLatin1Char('/')) && index > 0) {
        --index;
    }

    // Determine the first sub-path after the common path and
    // check whether it represents a directory or already a file
    bool isDirA = true;
    const QString subPathA = subPath(a, pathA, index, &isDirA);
    bool isDirB = true;
    const QString subPathB = subPath(b, pathB, index, &isDirB);

    if (isDirA && !isDirB) {
        return -1;
    } else if (!isDirA && isDirB) {
        return +1;
    }

    return stringCompare(subPathA, subPathB);
}

QString KFileItemModel::subPath(const KFileItem& item,
                                const QString& itemPath,
                                int start,
                                bool* isDir) const
{
    Q_ASSERT(isDir);
    const int pathIndex = itemPath.indexOf('/', start + 1);
    *isDir = (pathIndex > 0) || item.isDir();
    return itemPath.mid(start, pathIndex - start);
}

bool KFileItemModel::useMaximumUpdateInterval() const
{
    const KDirLister* dirLister = m_dirLister.data();
    return dirLister && !dirLister->url().isLocalFile();
}

QList<QPair<int, QVariant> > KFileItemModel::nameRoleGroups() const
{
    Q_ASSERT(!m_data.isEmpty());

    const int maxIndex = count() - 1;
    QList<QPair<int, QVariant> > groups;

    QString groupValue;
    QChar firstChar;
    bool isLetter = false;
    for (int i = 0; i <= maxIndex; ++i) {
        if (m_requestRole[ExpansionLevelRole] && m_data.at(i).value("expansionLevel").toInt() > 0) {
            // KItemListView would be capable to show sub-groups in groups but
            // in typical usecases this results in visual clutter, hence we
            // just ignore sub-groups.
            continue;
        }

        const QString name = m_data.at(i).value("name").toString();

        // Use the first character of the name as group indication
        QChar newFirstChar = name.at(0).toUpper();
        if (newFirstChar == QLatin1Char('~') && name.length() > 1) {
            newFirstChar = name.at(1);
        }

        if (firstChar != newFirstChar) {
            QString newGroupValue;
            if (newFirstChar >= QLatin1Char('A') && newFirstChar <= QLatin1Char('Z')) {
                // Apply group 'A' - 'Z'
                newGroupValue = newFirstChar;
                isLetter = true;
            } else if (newFirstChar >= QLatin1Char('0') && newFirstChar <= QLatin1Char('9')) {
                // Apply group 'Numerics' for any name that starts with a digit
                newGroupValue = i18nc("@title:group", "Numerics");
                isLetter = false;
            } else {
                if (isLetter) {
                    // If the current group is 'A' - 'Z' check whether a locale character
                    // fits into the existing group.
                    const QChar prevChar(firstChar.unicode() - ushort(1));
                    const QChar nextChar(firstChar.unicode() + ushort(1));
                    const QString currChar(newFirstChar);
                    const bool partOfCurrentGroup = currChar.localeAwareCompare(prevChar) > 0 &&
                                                    currChar.localeAwareCompare(nextChar) < 0;
                    if (partOfCurrentGroup) {
                        continue;
                    }
                }
                newGroupValue = i18nc("@title:group", "Others");
                isLetter = false;
            }

            if (newGroupValue != groupValue) {
                groupValue = newGroupValue;
                groups.append(QPair<int, QVariant>(i, newGroupValue));
            }

            firstChar = newFirstChar;
        }
    }
    return groups;
}

QList<QPair<int, QVariant> > KFileItemModel::sizeRoleGroups() const
{
    Q_ASSERT(!m_data.isEmpty());

    return QList<QPair<int, QVariant> >();
}

QList<QPair<int, QVariant> > KFileItemModel::dateRoleGroups() const
{
    Q_ASSERT(!m_data.isEmpty());
    return QList<QPair<int, QVariant> >();
}

QList<QPair<int, QVariant> > KFileItemModel::permissionRoleGroups() const
{
    Q_ASSERT(!m_data.isEmpty());
    return QList<QPair<int, QVariant> >();
}

QList<QPair<int, QVariant> > KFileItemModel::ownerRoleGroups() const
{
    Q_ASSERT(!m_data.isEmpty());
    return QList<QPair<int, QVariant> >();
}

QList<QPair<int, QVariant> > KFileItemModel::groupRoleGroups() const
{
    Q_ASSERT(!m_data.isEmpty());
    return QList<QPair<int, QVariant> >();
}

QList<QPair<int, QVariant> > KFileItemModel::typeRoleGroups() const
{
    Q_ASSERT(!m_data.isEmpty());
    return QList<QPair<int, QVariant> >();
}

QList<QPair<int, QVariant> > KFileItemModel::destinationRoleGroups() const
{
    Q_ASSERT(!m_data.isEmpty());
    return QList<QPair<int, QVariant> >();
}

QList<QPair<int, QVariant> > KFileItemModel::pathRoleGroups() const
{
    Q_ASSERT(!m_data.isEmpty());
    return QList<QPair<int, QVariant> >();
}

#include "kfileitemmodel.moc"
