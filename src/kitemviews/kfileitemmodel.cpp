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
    KItemModelBase(QByteArray(), "name", parent),
    m_dirLister(dirLister),
    m_naturalSorting(true),
    m_sortFoldersFirst(true),
    m_groupRole(NoRole),
    m_sortRole(NameRole),
    m_caseSensitivity(Qt::CaseInsensitive),
    m_sortedItems(),
    m_items(),
    m_data(),
    m_requestRole(),
    m_minimumUpdateIntervalTimer(0),
    m_maximumUpdateIntervalTimer(0),
    m_pendingItemsToInsert(),
    m_pendingItemsToDelete(),
    m_rootExpansionLevel(-1)
{
    resetRoles();
    m_requestRole[NameRole] = true;
    m_requestRole[IsDirRole] = true;

    Q_ASSERT(dirLister);

    connect(dirLister, SIGNAL(canceled()), this, SLOT(slotCanceled()));
    connect(dirLister, SIGNAL(completed()), this, SLOT(slotCompleted()));
    connect(dirLister, SIGNAL(newItems(KFileItemList)), this, SLOT(slotNewItems(KFileItemList)));
    connect(dirLister, SIGNAL(itemsDeleted(KFileItemList)), this, SLOT(slotItemsDeleted(KFileItemList)));
    connect(dirLister, SIGNAL(clear()), this, SLOT(slotClear()));
    connect(dirLister, SIGNAL(clear(KUrl)), this, SLOT(slotClear(KUrl)));

    // Although the layout engine of KItemListView is fast it is very inefficient to e.g.
    // emit 50 itemsInserted()-signals each 100 ms. m_minimumUpdateIntervalTimer assures that updates
    // are done in 1 second intervals for equal operations.
    m_minimumUpdateIntervalTimer = new QTimer(this);
    m_minimumUpdateIntervalTimer->setInterval(1000);
    m_minimumUpdateIntervalTimer->setSingleShot(true);
    connect(m_minimumUpdateIntervalTimer, SIGNAL(timeout()), this, SLOT(dispatchPendingItems()));

    // For slow KIO-slaves like used for searching it makes sense to show results periodically even
    // before the completed() or canceled() signal has been emitted.
    m_maximumUpdateIntervalTimer = new QTimer(this);
    m_maximumUpdateIntervalTimer->setInterval(2000);
    m_maximumUpdateIntervalTimer->setSingleShot(true);
    connect(m_maximumUpdateIntervalTimer, SIGNAL(timeout()), this, SLOT(dispatchPendingItems()));

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

int KFileItemModel::indexForKeyboardSearch(const QString& text, int startFromIndex) const
{
    startFromIndex = qMax(0, startFromIndex);
    for (int i = startFromIndex; i < count(); i++) {
        if (data(i)["name"].toString().startsWith(text, Qt::CaseInsensitive)) {
            kDebug() << data(i)["name"].toString();
            return i;
        }
    }
    for (int i = 0; i < startFromIndex; i++) {
        if (data(i)["name"].toString().startsWith(text, Qt::CaseInsensitive)) {
            kDebug() << data(i)["name"].toString();
            return i;
        }
    }
    return -1;
}

bool KFileItemModel::supportsGrouping() const
{
    return true;
}

bool KFileItemModel::supportsSorting() const
{
    return true;
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

KFileItem KFileItemModel::fileItem(int index) const
{
    if (index >= 0 && index < count()) {
        return m_sortedItems.at(index);
    }

    return KFileItem();
}

int KFileItemModel::index(const KFileItem& item) const
{
    if (item.isNull()) {
        return -1;
    }

    return m_items.value(item, -1);
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

    if (expanded) {
        const KUrl url = m_sortedItems.at(index).url();
        KDirLister* dirLister = m_dirLister.data();
        if (dirLister) {
            dirLister->openUrl(url, KDirLister::Keep);
            return true;
        }
    } else {
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

void KFileItemModel::onGroupRoleChanged(const QByteArray& current, const QByteArray& previous)
{
    Q_UNUSED(previous);
    m_groupRole = roleIndex(current);
}

void KFileItemModel::onSortRoleChanged(const QByteArray& current, const QByteArray& previous)
{
    Q_UNUSED(previous);
    const int itemCount = count();
    if (itemCount <= 0) {
        return;
    }

    m_sortRole = roleIndex(current);

    KFileItemList sortedItems = m_sortedItems;
    m_sortedItems.clear();
    m_items.clear();
    m_data.clear();
    emit itemsRemoved(KItemRangeList() << KItemRange(0, itemCount));

    sort(sortedItems.begin(), sortedItems.end());
    int index = 0;
    foreach (const KFileItem& item, sortedItems) {
        m_sortedItems.append(item);
        m_items.insert(item, index);
        m_data.append(retrieveData(item));

        ++index;
    }

    emit itemsInserted(KItemRangeList() << KItemRange(0, itemCount));
}

void KFileItemModel::slotCompleted()
{
    if (m_minimumUpdateIntervalTimer->isActive()) {
        // dispatchPendingItems() will be called when the timer
        // has been expired.
        return;
    }

    dispatchPendingItems();
    m_minimumUpdateIntervalTimer->start();
}

void KFileItemModel::slotCanceled()
{
    m_minimumUpdateIntervalTimer->stop();
    m_maximumUpdateIntervalTimer->stop();
    dispatchPendingItems();
}

void KFileItemModel::slotNewItems(const KFileItemList& items)
{
    if (!m_pendingItemsToDelete.isEmpty()) {
        removeItems(m_pendingItemsToDelete);
        m_pendingItemsToDelete.clear();
    }
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
    m_pendingItemsToDelete.append(items);
}

void KFileItemModel::slotClear()
{
#ifdef KFILEITEMMODEL_DEBUG
    kDebug() << "Clearing all items";
#endif

    m_minimumUpdateIntervalTimer->stop();
    m_maximumUpdateIntervalTimer->stop();
    m_pendingItemsToInsert.clear();
    m_pendingItemsToDelete.clear();

    m_rootExpansionLevel = -1;

    const int removedCount = m_data.count();
    if (removedCount > 0) {
        m_sortedItems.clear();
        m_items.clear();
        m_data.clear();
        emit itemsRemoved(KItemRangeList() << KItemRange(0, removedCount));
    }
}

void KFileItemModel::slotClear(const KUrl& url)
{
    Q_UNUSED(url);
}

void KFileItemModel::dispatchPendingItems()
{
    if (!m_pendingItemsToInsert.isEmpty()) {
        Q_ASSERT(m_pendingItemsToDelete.isEmpty());
        insertItems(m_pendingItemsToInsert);
        m_pendingItemsToInsert.clear();
    } else if (!m_pendingItemsToDelete.isEmpty()) {
        Q_ASSERT(m_pendingItemsToInsert.isEmpty());
        removeItems(m_pendingItemsToDelete);
        m_pendingItemsToDelete.clear();
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
        m_items.insert(m_sortedItems.at(i), i);
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
        m_items.remove(m_sortedItems.at(indexToRemove));
        m_sortedItems.removeAt(indexToRemove);
        m_data.removeAt(indexToRemove);
    }

    // The indexes of all m_items must be adjusted, not only the index
    // of the removed items
    for (int i = 0; i < m_sortedItems.count(); ++i) {
        m_items.insert(m_sortedItems.at(i), i);
    }

    if (count() <= 0) {
        m_rootExpansionLevel = -1;
    }

    itemRanges << KItemRange(removedAtIndex, removedCount);
    emit itemsRemoved(itemRanges);
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
            return result < 0;
        }
    }

    if (m_sortFoldersFirst) {
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

    default:
        break;
    }

    if (result == 0) {
        // It must be assured that the sort order is always unique even if two values have been
        // equal. In this case a comparison of the URL is done which is unique in all cases
        // within KDirLister.
        result = QString::compare(a.url().url(), b.url().url(), Qt::CaseSensitive);
    }

    return result < 0;
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

#include "kfileitemmodel.moc"
