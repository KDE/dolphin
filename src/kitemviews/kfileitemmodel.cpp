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
    m_roles(),
    m_caseSensitivity(Qt::CaseInsensitive),
    m_itemData(),
    m_items(),
    m_requestRole(),
    m_minimumUpdateIntervalTimer(0),
    m_maximumUpdateIntervalTimer(0),
    m_resortAllItemsTimer(0),
    m_pendingItemsToInsert(),
    m_pendingEmitLoadingCompleted(false),
    m_groups(),
    m_rootExpansionLevel(-1),
    m_expandedUrls(),
    m_urlsToExpand()
{
    // Apply default roles that should be determined
    resetRoles();
    m_requestRole[NameRole] = true;
    m_requestRole[IsDirRole] = true;
    m_roles.insert("name");
    m_roles.insert("isDir");

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
    
    // When changing the value of an item which represents the sort-role a resorting must be
    // triggered. Especially in combination with KFileItemModelRolesUpdater this might be done
    // for a lot of items within a quite small timeslot. To prevent expensive resortings the
    // resorting is postponed until the timer has been exceeded.
    m_resortAllItemsTimer = new QTimer(this);
    m_resortAllItemsTimer->setInterval(500);
    m_resortAllItemsTimer->setSingleShot(true);
    connect(m_resortAllItemsTimer, SIGNAL(timeout()), this, SLOT(resortAllItems()));

    Q_ASSERT(m_minimumUpdateIntervalTimer->interval() <= m_maximumUpdateIntervalTimer->interval());
}

KFileItemModel::~KFileItemModel()
{
    qDeleteAll(m_itemData);
    m_itemData.clear();
}

int KFileItemModel::count() const
{
    return m_itemData.count();
}

QHash<QByteArray, QVariant> KFileItemModel::data(int index) const
{
    if (index >= 0 && index < count()) {
        return m_itemData.at(index)->values;
    }
    return QHash<QByteArray, QVariant>();
}

bool KFileItemModel::setData(int index, const QHash<QByteArray, QVariant>& values)
{
    if (index < 0 || index >= count()) {
        return false;
    }

    QHash<QByteArray, QVariant> currentValues = m_itemData.at(index)->values;

    // Determine which roles have been changed
    QSet<QByteArray> changedRoles;
    QHashIterator<QByteArray, QVariant> it(values);
    while (it.hasNext()) {
        it.next();
        const QByteArray role = it.key();
        const QVariant value = it.value();

        if (currentValues[role] != value) {
            currentValues[role] = value;
            changedRoles.insert(role);
        }
    }

    if (changedRoles.isEmpty()) {
        return false;
    }

    m_itemData[index]->values = currentValues;
    emit itemsChanged(KItemRangeList() << KItemRange(index, 1), changedRoles);

    if (changedRoles.contains(sortRole())) {
        m_resortAllItemsTimer->start();
    }
        
    return true;
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
    case CommentRole:        descr = i18nc("@item:intable", "Comment"); break;
    case TagsRole:           descr = i18nc("@item:intable", "Tags"); break;
    case RatingRole:         descr = i18nc("@item:intable", "Rating"); break;
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
    if (!m_itemData.isEmpty() && m_groups.isEmpty()) {
#ifdef KFILEITEMMODEL_DEBUG
        QElapsedTimer timer;
        timer.start();
#endif
        switch (roleIndex(sortRole())) {
        case NameRole:           m_groups = nameRoleGroups(); break;
        case SizeRole:           m_groups = sizeRoleGroups(); break;
        case DateRole:           m_groups = dateRoleGroups(); break;
        case PermissionsRole:    m_groups = permissionRoleGroups(); break;
        case OwnerRole:          m_groups = genericStringRoleGroups("owner"); break;
        case GroupRole:          m_groups = genericStringRoleGroups("group"); break;
        case TypeRole:           m_groups = genericStringRoleGroups("type"); break;
        case DestinationRole:    m_groups = genericStringRoleGroups("destination"); break;
        case PathRole:           m_groups = genericStringRoleGroups("path"); break;
        case CommentRole:        m_groups = genericStringRoleGroups("comment"); break;
        case TagsRole:           m_groups = genericStringRoleGroups("tags"); break;
        case RatingRole:         m_groups = ratingRoleGroups(); break;
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
        return m_itemData.at(index)->item;
    }

    return KFileItem();
}

KFileItem KFileItemModel::fileItem(const KUrl& url) const
{
    const int index = m_items.value(url, -1);
    if (index >= 0) {
        return m_itemData.at(index)->item;
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
    m_roles = roles;

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
            m_itemData[i]->values = retrieveData(m_itemData.at(i)->item);
        }

        kWarning() << "TODO: Emitting itemsChanged() with no information what has changed!";
        emit itemsChanged(KItemRangeList() << KItemRange(0, count()), QSet<QByteArray>());
    }
}

QSet<QByteArray> KFileItemModel::roles() const
{
    return m_roles;
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

    const KUrl url = m_itemData.at(index)->item.url();
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
            itemsToRemove.append(m_itemData.at(index)->item);
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
        return m_itemData.at(index)->values.value("isExpanded").toBool();
    }
    return false;
}

bool KFileItemModel::isExpandable(int index) const
{
    if (index >= 0 && index < count()) {
        return m_itemData.at(index)->item.isDir();
    }
    return false;
}

QSet<KUrl> KFileItemModel::expandedUrls() const
{
    return m_expandedUrls;
}

void KFileItemModel::restoreExpandedUrls(const QSet<KUrl>& urls)
{
    m_urlsToExpand = urls;
}

void KFileItemModel::setExpanded(const QSet<KUrl>& urls)
{

    const KDirLister* dirLister = m_dirLister.data();
    if (!dirLister) {
        return;
    }

    const int pos = dirLister->url().url().length();

    // Assure that each sub-path of the URLs that should be
    // expanded is added to m_urlsToExpand too. KDirLister
    // does not care whether the parent-URL has already been
    // expanded.
    QSetIterator<KUrl> it1(urls);
    while (it1.hasNext()) {
        const KUrl& url = it1.next();

        KUrl urlToExpand = dirLister->url();
        const QStringList subDirs = url.url().mid(pos).split(QDir::separator());
        for (int i = 0; i < subDirs.count(); ++i) {
            urlToExpand.addPath(subDirs.at(i));
            m_urlsToExpand.insert(urlToExpand);
        }
    }

    // KDirLister::open() must called at least once to trigger an initial
    // loading. The pending URLs that must be restored are handled
    // in slotCompleted().
    QSetIterator<KUrl> it2(m_urlsToExpand);
    while (it2.hasNext()) {
        const int idx = index(it2.next());
        if (idx >= 0 && !isExpanded(idx)) {
            setExpanded(idx, true);
            break;
        }
    }
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

#ifdef KFILEITEMMODEL_DEBUG
    if (!m_requestRole[m_sortRole]) {
        kWarning() << "The sort-role has been changed to a role that has not been received yet";
    }
#endif

    resortAllItems();
}

void KFileItemModel::onSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    resortAllItems();    
}

void KFileItemModel::resortAllItems()
{
    m_resortAllItemsTimer->stop();
    
    const int itemCount = count();
    if (itemCount <= 0) {
        return;
    }

#ifdef KFILEITEMMODEL_DEBUG
    QElapsedTimer timer;
    timer.start();
    kDebug() << "===========================================================";
    kDebug() << "Resorting" << itemCount << "items";
#endif

    // Remember the order of the current URLs so
    // that it can be determined which indexes have
    // been moved because of the resorting.
    QList<KUrl> oldUrls;
    oldUrls.reserve(itemCount);
    foreach (const ItemData* itemData, m_itemData) {
        oldUrls.append(itemData->item.url());
    }
   
    m_groups.clear();
    m_items.clear();
    
    // Resort the items
    sort(m_itemData.begin(), m_itemData.end());    
    for (int i = 0; i < itemCount; ++i) {
        m_items.insert(m_itemData.at(i)->item.url(), i);
    }
    
    // Determine the indexes that have been moved
    bool emitItemsMoved = false;
    QList<int> movedToIndexes;
    movedToIndexes.reserve(itemCount);
    for (int i = 0; i < itemCount; i++) {
        const int newIndex = m_items.value(oldUrls.at(i).url());
        movedToIndexes.append(newIndex);
        if (!emitItemsMoved && newIndex != i) {
            emitItemsMoved = true;
        }
    }   

    if (emitItemsMoved) {
        emit itemsMoved(KItemRange(0, itemCount), movedToIndexes);
    }
    
#ifdef KFILEITEMMODEL_DEBUG
    kDebug() << "[TIME] Resorting of" << itemCount << "items:" << timer.elapsed();
#endif    
}

void KFileItemModel::slotCompleted()
{
    if (m_urlsToExpand.isEmpty() && m_minimumUpdateIntervalTimer->isActive()) {
        // dispatchPendingItems() will be called when the timer
        // has been expired.
        m_pendingEmitLoadingCompleted = true;
        return;
    }

    m_pendingEmitLoadingCompleted = false;
    dispatchPendingItemsToInsert();

    if (!m_urlsToExpand.isEmpty()) {
        // Try to find a URL that can be expanded.
        // Note that the parent folder must be expanded before any of its subfolders become visible.
        // Therefore, some URLs in m_restoredExpandedUrls might not be visible yet
        // -> we expand the first visible URL we find in m_restoredExpandedUrls.
        foreach(const KUrl& url, m_urlsToExpand) {
            const int index = m_items.value(url, -1);
            if (index >= 0) {
                m_urlsToExpand.remove(url);
                if (setExpanded(index, true)) {
                    // The dir lister has been triggered. This slot will be called
                    // again after the directory has been expanded.
                    return;
                }
            }
        }

        // None of the URLs in m_restoredExpandedUrls could be found in the model. This can happen
        // if these URLs have been deleted in the meantime.
        m_urlsToExpand.clear();
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
    m_resortAllItemsTimer->stop();
    m_pendingItemsToInsert.clear();

    m_rootExpansionLevel = -1;

    const int removedCount = m_itemData.count();
    if (removedCount > 0) {
        qDeleteAll(m_itemData);
        m_itemData.clear();
        m_items.clear();
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

    QList<ItemData*> sortedItems = createItemDataList(items);
    sort(sortedItems.begin(), sortedItems.end());

#ifdef KFILEITEMMODEL_DEBUG
    kDebug() << "[TIME] Sorting:" << timer.elapsed();
#endif

    KItemRangeList itemRanges;
    int targetIndex = 0;
    int sourceIndex = 0;
    int insertedAtIndex = -1;        // Index for the current item-range
    int insertedCount = 0;           // Count for the current item-range
    int previouslyInsertedCount = 0; // Sum of previously inserted items for all ranges
    while (sourceIndex < sortedItems.count()) {
        // Find target index from m_items to insert the current item
        // in a sorted order
        const int previousTargetIndex = targetIndex;
        while (targetIndex < m_itemData.count()) {
            if (!lessThan(m_itemData.at(targetIndex), sortedItems.at(sourceIndex))) {
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

        // Insert item at the position targetIndex by transfering
        // the ownership of the item-data from sortedItems to m_itemData.
        // m_items will be inserted after the loop (see comment below)
        m_itemData.insert(targetIndex, sortedItems.at(sourceIndex));        
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
    const int itemDataCount = m_itemData.count();
    for (int i = 0; i < itemDataCount; ++i) {
        m_items.insert(m_itemData.at(i)->item.url(), i);
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

    QList<ItemData*> sortedItems = createItemDataList(items);
    sort(sortedItems.begin(), sortedItems.end());

    QList<int> indexesToRemove;
    indexesToRemove.reserve(items.count());

    // Calculate the item ranges that will get deleted
    KItemRangeList itemRanges;
    int removedAtIndex = -1;
    int removedCount = 0;
    int targetIndex = 0;
    foreach (const ItemData* itemData, sortedItems) {
        const KFileItem& itemToRemove = itemData->item;
        
        const int previousTargetIndex = targetIndex;
        while (targetIndex < m_itemData.count()) {
            if (m_itemData.at(targetIndex)->item.url() == itemToRemove.url()) {
                break;
            }
            ++targetIndex;
        }
        if (targetIndex >= m_itemData.count()) {
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
    qDeleteAll(sortedItems);
    sortedItems.clear();

    // Delete the items
    for (int i = indexesToRemove.count() - 1; i >= 0; --i) {
        const int indexToRemove = indexesToRemove.at(i);
        delete m_itemData.at(indexToRemove);
        m_itemData.removeAt(indexToRemove);
    }

    // The indexes of all m_items must be adjusted, not only the index
    // of the removed items
    const int itemDataCount = m_itemData.count();
    for (int i = 0; i < itemDataCount; ++i) {
        m_items.insert(m_itemData.at(i)->item.url(), i);
    }

    if (count() <= 0) {
        m_rootExpansionLevel = -1;
    }

    itemRanges << KItemRange(removedAtIndex, removedCount);
    emit itemsRemoved(itemRanges);
}

QList<KFileItemModel::ItemData*> KFileItemModel::createItemDataList(const KFileItemList& items) const
{
    QList<ItemData*> itemDataList;
    itemDataList.reserve(items.count());

    foreach (const KFileItem& item, items) {
        ItemData* itemData = new ItemData();
        itemData->item = item;
        itemData->values = retrieveData(item);
        itemDataList.append(itemData);
    }
 
    return itemDataList;
}

void KFileItemModel::removeExpandedItems()
{
    KFileItemList expandedItems;

    const int maxIndex = m_itemData.count() - 1;
    for (int i = 0; i <= maxIndex; ++i) {
        const ItemData* itemData = m_itemData.at(i);
        if (itemData->values.value("expansionLevel").toInt() > 0) {
            expandedItems.append(itemData->item);
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
        rolesHash.insert("comment", CommentRole);
        rolesHash.insert("tags", TagsRole);
        rolesHash.insert("rating", RatingRole);
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
        if (m_rootExpansionLevel < 0 && m_dirLister.data()) {
            const QString rootDir = m_dirLister.data()->url().directory(KUrl::AppendTrailingSlash);
            m_rootExpansionLevel = rootDir.count('/');
            if (m_rootExpansionLevel == 1) {
                // Special case: The root is already reached and no parent is available
                --m_rootExpansionLevel;
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

bool KFileItemModel::lessThan(const ItemData* a, const ItemData* b) const
{
    const KFileItem& itemA = a->item;
    const KFileItem& itemB = b->item;
    
    int result = 0;

    if (m_rootExpansionLevel >= 0) {
        result = expansionLevelsCompare(itemA, itemB);
        if (result != 0) {
            // The items have parents with different expansion levels
            return (sortOrder() == Qt::AscendingOrder) ? result < 0 : result > 0;
        }
    }

    if (m_sortFoldersFirst || m_sortRole == SizeRole) {
        const bool isDirA = itemA.isDir();
        const bool isDirB = itemB.isDir();
        if (isDirA && !isDirB) {
            return true;
        } else if (!isDirA && isDirB) {
            return false;
        }
    }

    switch (m_sortRole) {
    case NameRole: {
        result = stringCompare(itemA.text(), itemB.text());
        if (result == 0) {
            // KFileItem::text() may not be unique in case UDS_DISPLAY_NAME is used
            result = stringCompare(itemA.name(m_caseSensitivity == Qt::CaseInsensitive),
                                   itemB.name(m_caseSensitivity == Qt::CaseInsensitive));
        }
        break;
    }

    case DateRole: {
        const KDateTime dateTimeA = itemA.time(KFileItem::ModificationTime);
        const KDateTime dateTimeB = itemB.time(KFileItem::ModificationTime);
        if (dateTimeA < dateTimeB) {
            result = -1;
        } else if (dateTimeA > dateTimeB) {
            result = +1;
        }
        break;
    }

    case SizeRole: {
        if (itemA.isDir()) {
            Q_ASSERT(itemB.isDir()); // see "if (m_sortFoldersFirst || m_sortRole == SizeRole)" above

            const QVariant valueA = a->values.value("size");
            const QVariant valueB = b->values.value("size");

            if (valueA.isNull()) {
                result = -1;
            } else if (valueB.isNull()) {
                result = +1;
            } else {
                result = valueA.value<KIO::filesize_t>() - valueB.value<KIO::filesize_t>();
            }
        } else {
            Q_ASSERT(!itemB.isDir()); // see "if (m_sortFoldersFirst || m_sortRole == SizeRole)" above
            result = itemA.size() - itemB.size();
        }
        break;
    }

    case TypeRole: {
        result = QString::compare(a->values.value("type").toString(),
                                  b->values.value("type").toString());
        break;
    }

    case CommentRole: {
        result = QString::compare(a->values.value("comment").toString(),
                                  b->values.value("comment").toString());
        break;
    }    

    case TagsRole: {
        result = QString::compare(a->values.value("tags").toString(),
                                  b->values.value("tags").toString());
        break;
    }    
    
    case RatingRole: {
        result = a->values.value("rating").toInt() - b->values.value("rating").toInt();
        break;
    }

    default:
        break;
    }

    if (result == 0) {
        // It must be assured that the sort order is always unique even if two values have been
        // equal. In this case a comparison of the URL is done which is unique in all cases
        // within KDirLister.
        result = QString::compare(itemA.url().url(), itemB.url().url(), Qt::CaseSensitive);
    }

    return (sortOrder() == Qt::AscendingOrder) ? result < 0 : result > 0;
}

void KFileItemModel::sort(QList<ItemData*>::iterator begin,
                          QList<ItemData*>::iterator end)
{   
    // The implementation is based on qStableSortHelper() from qalgorithms.h
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
    // In opposite to qStableSort() it allows to use a member-function for the comparison of elements.
    
    const int span = end - begin;
    if (span < 2) {
        return;
    }
    
    const QList<ItemData*>::iterator middle = begin + span / 2;
    sort(begin, middle);
    sort(middle, end);
    merge(begin, middle, end);
}

void KFileItemModel::merge(QList<ItemData*>::iterator begin,
                           QList<ItemData*>::iterator pivot,
                           QList<ItemData*>::iterator end)
{
    // The implementation is based on qMerge() from qalgorithms.h
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
    
    const int len1 = pivot - begin;
    const int len2 = end - pivot;
    
    if (len1 == 0 || len2 == 0) {
        return;
    }
    
    if (len1 + len2 == 2) {
        if (lessThan(*(begin + 1), *(begin))) {
            qSwap(*begin, *(begin + 1));
        }
        return;
    }
    
    QList<ItemData*>::iterator firstCut;
    QList<ItemData*>::iterator secondCut;
    int len2Half;
    if (len1 > len2) {
        const int len1Half = len1 / 2;
        firstCut = begin + len1Half;
        secondCut = lowerBound(pivot, end, *firstCut);
        len2Half = secondCut - pivot;
    } else {
        len2Half = len2 / 2;
        secondCut = pivot + len2Half;
        firstCut = upperBound(begin, pivot, *secondCut);
    }
    
    reverse(firstCut, pivot);
    reverse(pivot, secondCut);
    reverse(firstCut, secondCut);
    
    const QList<ItemData*>::iterator newPivot = firstCut + len2Half;
    merge(begin, firstCut, newPivot);
    merge(newPivot, secondCut, end);
}

QList<KFileItemModel::ItemData*>::iterator KFileItemModel::lowerBound(QList<ItemData*>::iterator begin,
                                                                      QList<ItemData*>::iterator end,
                                                                      const ItemData* value)
{
    // The implementation is based on qLowerBound() from qalgorithms.h
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
    
    QList<ItemData*>::iterator middle;
    int n = int(end - begin);
    int half;

    while (n > 0) {
        half = n >> 1;
        middle = begin + half;
        if (lessThan(*middle, value)) {
            begin = middle + 1;
            n -= half + 1;
        } else {
            n = half;
        }
    }
    return begin;
}

QList<KFileItemModel::ItemData*>::iterator KFileItemModel::upperBound(QList<ItemData*>::iterator begin,
                                                                      QList<ItemData*>::iterator end,
                                                                      const ItemData* value)
{
    // The implementation is based on qUpperBound() from qalgorithms.h
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
    
    QList<ItemData*>::iterator middle;
    int n = end - begin;
    int half;

    while (n > 0) {
        half = n >> 1;
        middle = begin + half;
        if (lessThan(value, *middle)) {
            n = half;
        } else {
            begin = middle + 1;
            n -= half + 1;
        }
    }
    return begin;
}

void KFileItemModel::reverse(QList<ItemData*>::iterator begin,
                             QList<ItemData*>::iterator end)
{
    // The implementation is based on qReverse() from qalgorithms.h
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
    
    --end;
    while (begin < end) {
        qSwap(*begin++, *end--);
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
    Q_ASSERT(!m_itemData.isEmpty());

    const int maxIndex = count() - 1;
    QList<QPair<int, QVariant> > groups;

    QString groupValue;
    QChar firstChar;
    bool isLetter = false;
    for (int i = 0; i <= maxIndex; ++i) {
        if (isChildItem(i)) {
            continue;
        }

        const QString name = m_itemData.at(i)->values.value("name").toString();

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
                // Apply group '0 - 9' for any name that starts with a digit
                newGroupValue = i18nc("@title:group Groups that start with a digit", "0 - 9");
                isLetter = false;
            } else {
                if (isLetter) {
                    // If the current group is 'A' - 'Z' check whether a locale character
                    // fits into the existing group.
                    // TODO: This does not work in the case if e.g. the group 'O' starts with
                    // an umlaut 'O' -> provide unit-test to document this known issue
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
    Q_ASSERT(!m_itemData.isEmpty());

    const int maxIndex = count() - 1;
    QList<QPair<int, QVariant> > groups;

    QString groupValue;
    for (int i = 0; i <= maxIndex; ++i) {
        if (isChildItem(i)) {
            continue;
        }

        const KFileItem& item = m_itemData.at(i)->item;
        const KIO::filesize_t fileSize = !item.isNull() ? item.size() : ~0U;
        QString newGroupValue;
        if (!item.isNull() && item.isDir()) {
            newGroupValue = i18nc("@title:group Size", "Folders");
        } else if (fileSize < 5 * 1024 * 1024) {
            newGroupValue = i18nc("@title:group Size", "Small");
        } else if (fileSize < 10 * 1024 * 1024) {
            newGroupValue = i18nc("@title:group Size", "Medium");
        } else {
            newGroupValue = i18nc("@title:group Size", "Big");
        }

        if (newGroupValue != groupValue) {
            groupValue = newGroupValue;
            groups.append(QPair<int, QVariant>(i, newGroupValue));
        }
    }

    return groups;
}

QList<QPair<int, QVariant> > KFileItemModel::dateRoleGroups() const
{
    Q_ASSERT(!m_itemData.isEmpty());

    const int maxIndex = count() - 1;
    QList<QPair<int, QVariant> > groups;

    const QDate currentDate = KDateTime::currentLocalDateTime().date();

    int yearForCurrentWeek = 0;
    int currentWeek = currentDate.weekNumber(&yearForCurrentWeek);
    if (yearForCurrentWeek == currentDate.year() + 1) {
        currentWeek = 53;
    }

    QDate previousModifiedDate;
    QString groupValue;
    for (int i = 0; i <= maxIndex; ++i) {
        if (isChildItem(i)) {
            continue;
        }

        const KDateTime modifiedTime = m_itemData.at(i)->item.time(KFileItem::ModificationTime);
        const QDate modifiedDate = modifiedTime.date();
        if (modifiedDate == previousModifiedDate) {
            // The current item is in the same group as the previous item
            continue;
        }
        previousModifiedDate = modifiedDate;

        const int daysDistance = modifiedDate.daysTo(currentDate);

        int yearForModifiedWeek = 0;
        int modifiedWeek = modifiedDate.weekNumber(&yearForModifiedWeek);
        if (yearForModifiedWeek == modifiedDate.year() + 1) {
            modifiedWeek = 53;
        }

        QString newGroupValue;
        if (currentDate.year() == modifiedDate.year() && currentDate.month() == modifiedDate.month()) {
            if (modifiedWeek > currentWeek) {
                // Usecase: modified date = 2010-01-01, current date = 2010-01-22
                //          modified week = 53,         current week = 3
                modifiedWeek = 0;
            }
            switch (currentWeek - modifiedWeek) {
            case 0:
                switch (daysDistance) {
                case 0:  newGroupValue = i18nc("@title:group Date", "Today"); break;
                case 1:  newGroupValue = i18nc("@title:group Date", "Yesterday"); break;
                default: newGroupValue = modifiedTime.toString(i18nc("@title:group The week day name: %A", "%A"));
                }
                break;
            case 1:
                newGroupValue = i18nc("@title:group Date", "Last Week");
                break;
            case 2:
                newGroupValue = i18nc("@title:group Date", "Two Weeks Ago");
                break;
            case 3:
                newGroupValue = i18nc("@title:group Date", "Three Weeks Ago");
                break;
            case 4:
            case 5:
                newGroupValue = i18nc("@title:group Date", "Earlier this Month");
                break;
            default:
                Q_ASSERT(false);
            }
        } else {
            const QDate lastMonthDate = currentDate.addMonths(-1);
            if  (lastMonthDate.year() == modifiedDate.year() && lastMonthDate.month() == modifiedDate.month()) {
                if (daysDistance == 1) {
                    newGroupValue = modifiedTime.toString(i18nc("@title:group Date: %B is full month name in current locale, and %Y is full year number", "Yesterday (%B, %Y)"));
                } else if (daysDistance <= 7) {
                    newGroupValue = modifiedTime.toString(i18nc("@title:group The week day name: %A, %B is full month name in current locale, and %Y is full year number", "%A (%B, %Y)"));
                } else if (daysDistance <= 7 * 2) {
                    newGroupValue = modifiedTime.toString(i18nc("@title:group Date: %B is full month name in current locale, and %Y is full year number", "Last Week (%B, %Y)"));
                } else if (daysDistance <= 7 * 3) {
                    newGroupValue = modifiedTime.toString(i18nc("@title:group Date: %B is full month name in current locale, and %Y is full year number", "Two Weeks Ago (%B, %Y)"));
                } else if (daysDistance <= 7 * 4) {
                    newGroupValue = modifiedTime.toString(i18nc("@title:group Date: %B is full month name in current locale, and %Y is full year number", "Three Weeks Ago (%B, %Y)"));
                } else {
                    newGroupValue = modifiedTime.toString(i18nc("@title:group Date: %B is full month name in current locale, and %Y is full year number", "Earlier on %B, %Y"));
                }
            } else {
                newGroupValue = modifiedTime.toString(i18nc("@title:group The month and year: %B is full month name in current locale, and %Y is full year number", "%B, %Y"));
            }
        }

        if (newGroupValue != groupValue) {
            groupValue = newGroupValue;
            groups.append(QPair<int, QVariant>(i, newGroupValue));
        }
    }

    return groups;
}

QList<QPair<int, QVariant> > KFileItemModel::permissionRoleGroups() const
{
    Q_ASSERT(!m_itemData.isEmpty());

    const int maxIndex = count() - 1;
    QList<QPair<int, QVariant> > groups;

    QString permissionsString;
    QString groupValue;
    for (int i = 0; i <= maxIndex; ++i) {
        if (isChildItem(i)) {
            continue;
        }

        const ItemData* itemData = m_itemData.at(i);
        const QString newPermissionsString = itemData->values.value("permissions").toString();
        if (newPermissionsString == permissionsString) {
            continue;
        }
        permissionsString = newPermissionsString;

        const QFileInfo info(itemData->item.url().pathOrUrl());

        // Set user string
        QString user;
        if (info.permission(QFile::ReadUser)) {
            user = i18nc("@item:intext Access permission, concatenated", "Read, ");
        }
        if (info.permission(QFile::WriteUser)) {
            user += i18nc("@item:intext Access permission, concatenated", "Write, ");
        }
        if (info.permission(QFile::ExeUser)) {
            user += i18nc("@item:intext Access permission, concatenated", "Execute, ");
        }
        user = user.isEmpty() ? i18nc("@item:intext Access permission, concatenated", "Forbidden") : user.mid(0, user.count() - 2);

        // Set group string
        QString group;
        if (info.permission(QFile::ReadGroup)) {
            group = i18nc("@item:intext Access permission, concatenated", "Read, ");
        }
        if (info.permission(QFile::WriteGroup)) {
            group += i18nc("@item:intext Access permission, concatenated", "Write, ");
        }
        if (info.permission(QFile::ExeGroup)) {
            group += i18nc("@item:intext Access permission, concatenated", "Execute, ");
        }
        group = group.isEmpty() ? i18nc("@item:intext Access permission, concatenated", "Forbidden") : group.mid(0, group.count() - 2);

        // Set others string
        QString others;
        if (info.permission(QFile::ReadOther)) {
            others = i18nc("@item:intext Access permission, concatenated", "Read, ");
        }
        if (info.permission(QFile::WriteOther)) {
            others += i18nc("@item:intext Access permission, concatenated", "Write, ");
        }
        if (info.permission(QFile::ExeOther)) {
            others += i18nc("@item:intext Access permission, concatenated", "Execute, ");
        }
        others = others.isEmpty() ? i18nc("@item:intext Access permission, concatenated", "Forbidden") : others.mid(0, others.count() - 2);

        const QString newGroupValue = i18nc("@title:group Files and folders by permissions", "User: %1 | Group: %2 | Others: %3", user, group, others);
        if (newGroupValue != groupValue) {
            groupValue = newGroupValue;
            groups.append(QPair<int, QVariant>(i, newGroupValue));
        }
    }

    return groups;
}

QList<QPair<int, QVariant> > KFileItemModel::ratingRoleGroups() const
{
    Q_ASSERT(!m_itemData.isEmpty());

    const int maxIndex = count() - 1;
    QList<QPair<int, QVariant> > groups;

    int groupValue;
    for (int i = 0; i <= maxIndex; ++i) {
        if (isChildItem(i)) {
            continue;
        }
        const int newGroupValue = m_itemData.at(i)->values.value("rating").toInt();
        if (newGroupValue != groupValue) {
            groupValue = newGroupValue;
            groups.append(QPair<int, QVariant>(i, newGroupValue));
        }
    }

    return groups;
}

QList<QPair<int, QVariant> > KFileItemModel::genericStringRoleGroups(const QByteArray& role) const
{
    Q_ASSERT(!m_itemData.isEmpty());

    const int maxIndex = count() - 1;
    QList<QPair<int, QVariant> > groups;

    QString groupValue;
    for (int i = 0; i <= maxIndex; ++i) {
        if (isChildItem(i)) {
            continue;
        }
        const QString newGroupValue = m_itemData.at(i)->values.value(role).toString();
        if (newGroupValue != groupValue) {
            groupValue = newGroupValue;
            groups.append(QPair<int, QVariant>(i, newGroupValue));
        }
    }

    return groups;
}

#include "kfileitemmodel.moc"
