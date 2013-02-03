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

#include <KDirModel>
#include <KGlobalSettings>
#include <KLocale>
#include <KStringHandler>
#include <KDebug>

#include "private/kfileitemmodelsortalgorithm.h"
#include "private/kfileitemmodeldirlister.h"

#include <QApplication>
#include <QMimeData>
#include <QTimer>
#include <QWidget>

// #define KFILEITEMMODEL_DEBUG

KFileItemModel::KFileItemModel(QObject* parent) :
    KItemModelBase("text", parent),
    m_dirLister(0),
    m_naturalSorting(KGlobalSettings::naturalSorting()),
    m_sortDirsFirst(true),
    m_sortRole(NameRole),
    m_sortingProgressPercent(-1),
    m_roles(),
    m_caseSensitivity(Qt::CaseInsensitive),
    m_itemData(),
    m_items(),
    m_filter(),
    m_filteredItems(),
    m_requestRole(),
    m_maximumUpdateIntervalTimer(0),
    m_resortAllItemsTimer(0),
    m_pendingItemsToInsert(),
    m_groups(),
    m_expandedParentsCountRoot(UninitializedExpandedParentsCountRoot),
    m_expandedDirs(),
    m_urlsToExpand()
{
    m_dirLister = new KFileItemModelDirLister(this);
    m_dirLister->setDelayedMimeTypes(true);

    const QWidget* parentWidget = qobject_cast<QWidget*>(parent);
    if (parentWidget) {
        m_dirLister->setMainWindow(parentWidget->window());
    }

    connect(m_dirLister, SIGNAL(started(KUrl)), this, SIGNAL(directoryLoadingStarted()));
    connect(m_dirLister, SIGNAL(canceled()), this, SLOT(slotCanceled()));
    connect(m_dirLister, SIGNAL(completed(KUrl)), this, SLOT(slotCompleted()));
    connect(m_dirLister, SIGNAL(newItems(KFileItemList)), this, SLOT(slotNewItems(KFileItemList)));
    connect(m_dirLister, SIGNAL(itemsDeleted(KFileItemList)), this, SLOT(slotItemsDeleted(KFileItemList)));
    connect(m_dirLister, SIGNAL(refreshItems(QList<QPair<KFileItem,KFileItem> >)), this, SLOT(slotRefreshItems(QList<QPair<KFileItem,KFileItem> >)));
    connect(m_dirLister, SIGNAL(clear()), this, SLOT(slotClear()));
    connect(m_dirLister, SIGNAL(clear(KUrl)), this, SLOT(slotClear(KUrl)));
    connect(m_dirLister, SIGNAL(infoMessage(QString)), this, SIGNAL(infoMessage(QString)));
    connect(m_dirLister, SIGNAL(errorMessage(QString)), this, SIGNAL(errorMessage(QString)));
    connect(m_dirLister, SIGNAL(redirection(KUrl,KUrl)), this, SIGNAL(directoryRedirection(KUrl,KUrl)));
    connect(m_dirLister, SIGNAL(urlIsFileError(KUrl)), this, SIGNAL(urlIsFileError(KUrl)));

    // Apply default roles that should be determined
    resetRoles();
    m_requestRole[NameRole] = true;
    m_requestRole[IsDirRole] = true;
    m_requestRole[IsLinkRole] = true;
    m_roles.insert("text");
    m_roles.insert("isDir");
    m_roles.insert("isLink");

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

    connect(KGlobalSettings::self(), SIGNAL(naturalSortingChanged()), this, SLOT(slotNaturalSortingChanged()));
}

KFileItemModel::~KFileItemModel()
{
    qDeleteAll(m_itemData);
    m_itemData.clear();
}

void KFileItemModel::loadDirectory(const KUrl& url)
{
    m_dirLister->openUrl(url);
}

void KFileItemModel::refreshDirectory(const KUrl& url)
{
    m_dirLister->openUrl(url, KDirLister::Reload);
}

KUrl KFileItemModel::directory() const
{
    return m_dirLister->url();
}

void KFileItemModel::cancelDirectoryLoading()
{
    m_dirLister->stop();
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
    if (changedRoles.contains("text")) {
        KUrl url = m_itemData[index]->item.url();
        url.setFileName(currentValues["text"].toString());
        m_itemData[index]->item.setUrl(url);
    }

    emit itemsChanged(KItemRangeList() << KItemRange(index, 1), changedRoles);

    if (changedRoles.contains(sortRole())) {
        m_resortAllItemsTimer->start();
    }

    return true;
}

void KFileItemModel::setSortDirectoriesFirst(bool dirsFirst)
{
    if (dirsFirst != m_sortDirsFirst) {
        m_sortDirsFirst = dirsFirst;
        resortAllItems();
    }
}

bool KFileItemModel::sortDirectoriesFirst() const
{
    return m_sortDirsFirst;
}

void KFileItemModel::setShowHiddenFiles(bool show)
{
    m_dirLister->setShowingDotFiles(show);
    m_dirLister->emitChanges();
    if (show) {
        slotCompleted();
    }
}

bool KFileItemModel::showHiddenFiles() const
{
    return m_dirLister->showingDotFiles();
}

void KFileItemModel::setShowDirectoriesOnly(bool enabled)
{
    m_dirLister->setDirOnlyMode(enabled);
}

bool KFileItemModel::showDirectoriesOnly() const
{
    return m_dirLister->dirOnlyMode();
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
        if (data(i)["text"].toString().startsWith(text, Qt::CaseInsensitive)) {
            return i;
        }
    }
    for (int i = 0; i < startFromIndex; ++i) {
        if (data(i)["text"].toString().startsWith(text, Qt::CaseInsensitive)) {
            return i;
        }
    }
    return -1;
}

bool KFileItemModel::supportsDropping(int index) const
{
    const KFileItem item = fileItem(index);
    return !item.isNull() && (item.isDir() || item.isDesktopFile());
}

QString KFileItemModel::roleDescription(const QByteArray& role) const
{
    static QHash<QByteArray, QString> description;
    if (description.isEmpty()) {
        int count = 0;
        const RoleInfoMap* map = rolesInfoMap(count);
        for (int i = 0; i < count; ++i) {
            description.insert(map[i].role, i18nc(map[i].roleTranslationContext, map[i].roleTranslation));
        }
    }

    return description.value(role);
}

QList<QPair<int, QVariant> > KFileItemModel::groups() const
{
    if (!m_itemData.isEmpty() && m_groups.isEmpty()) {
#ifdef KFILEITEMMODEL_DEBUG
        QElapsedTimer timer;
        timer.start();
#endif
        switch (typeForRole(sortRole())) {
        case NameRole:        m_groups = nameRoleGroups(); break;
        case SizeRole:        m_groups = sizeRoleGroups(); break;
        case DateRole:        m_groups = dateRoleGroups(); break;
        case PermissionsRole: m_groups = permissionRoleGroups(); break;
        case RatingRole:      m_groups = ratingRoleGroups(); break;
        default:              m_groups = genericStringRoleGroups(sortRole()); break;
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
    return m_dirLister->rootItem();
}

void KFileItemModel::clear()
{
    slotClear();
}

void KFileItemModel::setRoles(const QSet<QByteArray>& roles)
{
    if (m_roles == roles) {
        return;
    }
    m_roles = roles;

    if (count() > 0) {
        const bool supportedExpanding = m_requestRole[ExpandedParentsCountRole];
        const bool willSupportExpanding = roles.contains("expandedParentsCount");
        if (supportedExpanding && !willSupportExpanding) {
            // No expanding is supported anymore. Take care to delete all items that have an expansion level
            // that is not 0 (and hence are part of an expanded item).
            removeExpandedItems();
        }
    }

    m_groups.clear();
    resetRoles();

    QSetIterator<QByteArray> it(roles);
    while (it.hasNext()) {
        const QByteArray& role = it.next();
        m_requestRole[typeForRole(role)] = true;
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
    if (!isExpandable(index) || isExpanded(index) == expanded) {
        return false;
    }

    QHash<QByteArray, QVariant> values;
    values.insert("isExpanded", expanded);
    if (!setData(index, values)) {
        return false;
    }

    const KUrl url = m_itemData.at(index)->item.url();
    if (expanded) {
        m_expandedDirs.insert(url);
        m_dirLister->openUrl(url, KDirLister::Keep);
    } else {
        m_expandedDirs.remove(url);
        m_dirLister->stop(url);


        KFileItemList itemsToRemove;
        const int expandedParentsCount = data(index)["expandedParentsCount"].toInt();
        ++index;
        while (index < count() && data(index)["expandedParentsCount"].toInt() > expandedParentsCount) {
            itemsToRemove.append(m_itemData.at(index)->item);
            ++index;
        }
        removeItems(itemsToRemove);
    }

    return true;
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
        return m_itemData.at(index)->values.value("isExpandable").toBool();
    }
    return false;
}

int KFileItemModel::expandedParentsCount(int index) const
{
    if (index >= 0 && index < count()) {
        const int parentsCount = m_itemData.at(index)->values.value("expandedParentsCount").toInt();
        if (parentsCount > 0) {
            return parentsCount;
        }
    }
    return 0;
}

QSet<KUrl> KFileItemModel::expandedDirectories() const
{
    return m_expandedDirs;
}

void KFileItemModel::restoreExpandedDirectories(const QSet<KUrl>& urls)
{
    m_urlsToExpand = urls;
}

void KFileItemModel::expandParentDirectories(const KUrl& url)
{
    const int pos = m_dirLister->url().path().length();

    // Assure that each sub-path of the URL that should be
    // expanded is added to m_urlsToExpand. KDirLister
    // does not care whether the parent-URL has already been
    // expanded.
    KUrl urlToExpand = m_dirLister->url();
    const QStringList subDirs = url.path().mid(pos).split(QDir::separator());
    for (int i = 0; i < subDirs.count() - 1; ++i) {
        urlToExpand.addPath(subDirs.at(i));
        m_urlsToExpand.insert(urlToExpand);
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

void KFileItemModel::setNameFilter(const QString& nameFilter)
{
    if (m_filter.pattern() != nameFilter) {
        dispatchPendingItemsToInsert();
        m_filter.setPattern(nameFilter);
        applyFilters();
    }
}

QString KFileItemModel::nameFilter() const
{
    return m_filter.pattern();
}

void KFileItemModel::setMimeTypeFilters(const QStringList& filters)
{
    if (m_filter.mimeTypes() != filters) {
        dispatchPendingItemsToInsert();
        m_filter.setMimeTypes(filters);
        applyFilters();
    }
}

QStringList KFileItemModel::mimeTypeFilters() const
{
    return m_filter.mimeTypes();
}


void KFileItemModel::applyFilters()
{
    // Check which shown items from m_itemData must get
    // hidden and hence moved to m_filteredItems.
    KFileItemList newFilteredItems;

    foreach (ItemData* itemData, m_itemData) {
        // Only filter non-expanded items as child items may never
        // exist without a parent item
        if (!itemData->values.value("isExpanded").toBool()) {
            if (!m_filter.matches(itemData->item)) {
                newFilteredItems.append(itemData->item);
                m_filteredItems.insert(itemData->item);
            }
        }
    }

    removeItems(newFilteredItems);

    // Check which hidden items from m_filteredItems should
    // get visible again and hence removed from m_filteredItems.
    KFileItemList newVisibleItems;

    QMutableSetIterator<KFileItem> it(m_filteredItems);
    while (it.hasNext()) {
        const KFileItem item = it.next();
        if (m_filter.matches(item)) {
            newVisibleItems.append(item);
            it.remove();
        }
    }

    insertItems(newVisibleItems);
}

QList<KFileItemModel::RoleInfo> KFileItemModel::rolesInformation()
{
    static QList<RoleInfo> rolesInfo;
    if (rolesInfo.isEmpty()) {
        int count = 0;
        const RoleInfoMap* map = rolesInfoMap(count);
        for (int i = 0; i < count; ++i) {
            if (map[i].roleType != NoRole) {
                RoleInfo info;
                info.role = map[i].role;
                info.translation = i18nc(map[i].roleTranslationContext, map[i].roleTranslation);
                if (map[i].groupTranslation) {
                    info.group = i18nc(map[i].groupTranslationContext, map[i].groupTranslation);
                } else {
                    // For top level roles, groupTranslation is 0. We must make sure that
                    // info.group is an empty string then because the code that generates
                    // menus tries to put the actions into sub menus otherwise.
                    info.group = QString();
                }
                info.requiresNepomuk = map[i].requiresNepomuk;
                info.requiresIndexer = map[i].requiresIndexer;
                rolesInfo.append(info);
            }
        }
    }

    return rolesInfo;
}

void KFileItemModel::onGroupedSortingChanged(bool current)
{
    Q_UNUSED(current);
    m_groups.clear();
}

void KFileItemModel::onSortRoleChanged(const QByteArray& current, const QByteArray& previous)
{
    Q_UNUSED(previous);
    m_sortRole = typeForRole(current);

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
    QList<int> movedToIndexes;
    movedToIndexes.reserve(itemCount);
    for (int i = 0; i < itemCount; i++) {
        const int newIndex = m_items.value(oldUrls.at(i).url());
        movedToIndexes.append(newIndex);
    }

    // Don't check whether items have really been moved and always emit a
    // itemsMoved() signal after resorting: In case of grouped items
    // the groups might change even if the items themselves don't change their
    // position. Let the receiver of the signal decide whether a check for moved
    // items makes sense.
    emit itemsMoved(KItemRange(0, itemCount), movedToIndexes);

#ifdef KFILEITEMMODEL_DEBUG
    kDebug() << "[TIME] Resorting of" << itemCount << "items:" << timer.elapsed();
#endif
}

void KFileItemModel::slotCompleted()
{
    dispatchPendingItemsToInsert();

    if (!m_urlsToExpand.isEmpty()) {
        // Try to find a URL that can be expanded.
        // Note that the parent folder must be expanded before any of its subfolders become visible.
        // Therefore, some URLs in m_restoredExpandedUrls might not be visible yet
        // -> we expand the first visible URL we find in m_restoredExpandedUrls.
        foreach (const KUrl& url, m_urlsToExpand) {
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

    emit directoryLoadingCompleted();
}

void KFileItemModel::slotCanceled()
{
    m_maximumUpdateIntervalTimer->stop();
    dispatchPendingItemsToInsert();

    emit directoryLoadingCanceled();
}

void KFileItemModel::slotNewItems(const KFileItemList& items)
{
    Q_ASSERT(!items.isEmpty());

    if (m_requestRole[ExpandedParentsCountRole] && m_expandedParentsCountRoot >= 0) {
        // To be able to compare whether the new items may be inserted as children
        // of a parent item the pending items must be added to the model first.
        dispatchPendingItemsToInsert();

        KFileItem item = items.first();

        // If the expanding of items is enabled, the call
        // dirLister->openUrl(url, KDirLister::Keep) in KFileItemModel::setExpanded()
        // might result in emitting the same items twice due to the Keep-parameter.
        // This case happens if an item gets expanded, collapsed and expanded again
        // before the items could be loaded for the first expansion.
        const int index = m_items.value(item.url(), -1);
        if (index >= 0) {
            // The items are already part of the model.
            return;
        }

        // KDirLister keeps the children of items that got expanded once even if
        // they got collapsed again with KFileItemModel::setExpanded(false). So it must be
        // checked whether the parent for new items is still expanded.
        KUrl parentUrl = item.url().upUrl();
        parentUrl.adjustPath(KUrl::RemoveTrailingSlash);
        const int parentIndex = m_items.value(parentUrl, -1);
        if (parentIndex >= 0 && !m_itemData[parentIndex]->values.value("isExpanded").toBool()) {
            // The parent is not expanded.
            return;
        }
    }

    if (!m_filter.hasSetFilters()) {
        m_pendingItemsToInsert.append(items);
    } else {
        // The name or type filter is active. Hide filtered items
        // before inserting them into the model and remember
        // the filtered items in m_filteredItems.
        KFileItemList filteredItems;
        foreach (const KFileItem& item, items) {
            if (m_filter.matches(item)) {
                filteredItems.append(item);
            } else {
                m_filteredItems.insert(item);
            }
        }

        m_pendingItemsToInsert.append(filteredItems);
    }

    if (useMaximumUpdateInterval() && !m_maximumUpdateIntervalTimer->isActive()) {
        // Assure that items get dispatched if no completed() or canceled() signal is
        // emitted during the maximum update interval.
        m_maximumUpdateIntervalTimer->start();
    }
}

void KFileItemModel::slotItemsDeleted(const KFileItemList& items)
{
    dispatchPendingItemsToInsert();

    KFileItemList itemsToRemove = items;
    if (m_requestRole[ExpandedParentsCountRole] && m_expandedParentsCountRoot >= 0) {
        // Assure that removing a parent item also results in removing all children
        foreach (const KFileItem& item, items) {
            itemsToRemove.append(childItems(item));
        }
    }

    if (!m_filteredItems.isEmpty()) {
        foreach (const KFileItem& item, itemsToRemove) {
            m_filteredItems.remove(item);
        }
    }

    removeItems(itemsToRemove);
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
        const KFileItem& oldItem = itemPair.first;
        const KFileItem& newItem = itemPair.second;
        const int index = m_items.value(oldItem.url(), -1);
        if (index >= 0) {
            m_itemData[index]->item = newItem;

            // Keep old values as long as possible if they could not retrieved synchronously yet.
            // The update of the values will be done asynchronously by KFileItemModelRolesUpdater.
            QHashIterator<QByteArray, QVariant> it(retrieveData(newItem));
            while (it.hasNext()) {
                it.next();
                m_itemData[index]->values.insert(it.key(), it.value());
            }

            m_items.remove(oldItem.url());
            m_items.insert(newItem.url(), index);
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
    int previousIndex = indexes.at(0);
    int rangeIndex = previousIndex;
    int rangeCount = 1;

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

    emit itemsChanged(itemRangeList, m_roles);

    resortAllItems();
}

void KFileItemModel::slotClear()
{
#ifdef KFILEITEMMODEL_DEBUG
    kDebug() << "Clearing all items";
#endif

    m_filteredItems.clear();
    m_groups.clear();

    m_maximumUpdateIntervalTimer->stop();
    m_resortAllItemsTimer->stop();
    m_pendingItemsToInsert.clear();

    m_expandedParentsCountRoot = UninitializedExpandedParentsCountRoot;

    const int removedCount = m_itemData.count();
    if (removedCount > 0) {
        qDeleteAll(m_itemData);
        m_itemData.clear();
        m_items.clear();
        emit itemsRemoved(KItemRangeList() << KItemRange(0, removedCount));
    }

    m_expandedDirs.clear();
}

void KFileItemModel::slotClear(const KUrl& url)
{
    Q_UNUSED(url);
}

void KFileItemModel::slotNaturalSortingChanged()
{
    m_naturalSorting = KGlobalSettings::naturalSorting();
    resortAllItems();
}

void KFileItemModel::dispatchPendingItemsToInsert()
{
    if (!m_pendingItemsToInsert.isEmpty()) {
        insertItems(m_pendingItemsToInsert);
        m_pendingItemsToInsert.clear();
    }
}

void KFileItemModel::insertItems(const KFileItemList& items)
{
    if (items.isEmpty()) {
        return;
    }

    if (m_sortRole == TypeRole) {
        // Try to resolve the MIME-types synchronously to prevent a reordering of
        // the items when sorting by type (per default MIME-types are resolved
        // asynchronously by KFileItemModelRolesUpdater).
        determineMimeTypes(items, 200);
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

        // Insert item at the position targetIndex by transferring
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

static KItemRangeList sortedIndexesToKItemRangeList(const QList<int>& sortedNumbers)
{
    if (sortedNumbers.empty()) {
        return KItemRangeList();
    }

    KItemRangeList result;

    QList<int>::const_iterator it = sortedNumbers.begin();
    int index = *it;
    int count = 1;

    ++it;

    QList<int>::const_iterator end = sortedNumbers.end();
    while (it != end) {
        if (*it == index + count) {
            ++count;
        } else {
            result << KItemRange(index, count);
            index = *it;
            count = 1;
        }
        ++it;
    }

    result << KItemRange(index, count);
    return result;
}

void KFileItemModel::removeItems(const KFileItemList& items)
{
#ifdef KFILEITEMMODEL_DEBUG
    kDebug() << "Removing " << items.count() << "items";
#endif

    m_groups.clear();

    // Step 1: Determine the indexes of the removed items, remove them from
    //         the hash m_items, and free the ItemData.
    QList<int> indexesToRemove;
    indexesToRemove.reserve(items.count());
    foreach (const KFileItem& item, items) {
        const KUrl url = item.url();
        const int index = m_items.value(url, -1);
        if (index >= 0) {
            indexesToRemove.append(index);

            // Prevent repeated expensive rehashing by using QHash::erase(),
            // rather than QHash::remove().
            QHash<KUrl, int>::iterator it = m_items.find(url);
            m_items.erase(it);

            ItemData* data = m_itemData.at(index);
            delete data;
            m_itemData[index] = 0;
        }
    }

    if (indexesToRemove.isEmpty()) {
        return;
    }

    std::sort(indexesToRemove.begin(), indexesToRemove.end());

    // Step 2: Remove the ItemData pointers from the list m_itemData.
    const KItemRangeList itemRanges = sortedIndexesToKItemRangeList(indexesToRemove);
    int target = itemRanges.at(0).index;
    int source = itemRanges.at(0).index + itemRanges.at(0).count;
    int nextRange = 1;

    const int oldItemDataCount = m_itemData.count();
    while (source < oldItemDataCount) {
        m_itemData[target] = m_itemData[source];
        ++target;
        ++source;

        if (nextRange < itemRanges.count() && source == itemRanges.at(nextRange).index) {
            // Skip the items in the next removed range.
            source += itemRanges.at(nextRange).count;
            ++nextRange;
        }
    }

    m_itemData.erase(m_itemData.end() - indexesToRemove.count(), m_itemData.end());

    // Step 3: Adjust indexes in the hash m_items. Note that all indexes
    //         might have been changed by the removal of the items.
    const int newItemDataCount = m_itemData.count();
    for (int i = 0; i < newItemDataCount; ++i) {
        m_items.insert(m_itemData.at(i)->item.url(), i);
    }

    if (count() <= 0) {
        m_expandedParentsCountRoot = UninitializedExpandedParentsCountRoot;
    }

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
        itemData->parent = 0;

        const bool determineParent = m_requestRole[ExpandedParentsCountRole]
                                     && itemData->values["expandedParentsCount"].toInt() > 0;
        if (determineParent) {
            KUrl parentUrl = item.url().upUrl();
            parentUrl.adjustPath(KUrl::RemoveTrailingSlash);
            const int parentIndex = m_items.value(parentUrl, -1);
            if (parentIndex >= 0) {
                itemData->parent = m_itemData.at(parentIndex);
            } else {
                kWarning() << "Parent item not found for" << item.url();
            }
        }

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
        if (itemData->values.value("expandedParentsCount").toInt() > 0) {
            expandedItems.append(itemData->item);
        }
    }

    // The m_expandedParentsCountRoot may not get reset before all items with
    // a bigger count have been removed.
    removeItems(expandedItems);

    m_expandedParentsCountRoot = UninitializedExpandedParentsCountRoot;
    m_expandedDirs.clear();
}

void KFileItemModel::resetRoles()
{
    for (int i = 0; i < RolesCount; ++i) {
        m_requestRole[i] = false;
    }
}

KFileItemModel::RoleType KFileItemModel::typeForRole(const QByteArray& role) const
{
    static QHash<QByteArray, RoleType> roles;
    if (roles.isEmpty()) {
        // Insert user visible roles that can be accessed with
        // KFileItemModel::roleInformation()
        int count = 0;
        const RoleInfoMap* map = rolesInfoMap(count);
        for (int i = 0; i < count; ++i) {
            roles.insert(map[i].role, map[i].roleType);
        }

        // Insert internal roles (take care to synchronize the implementation
        // with KFileItemModel::roleForType() in case if a change is done).
        roles.insert("isDir", IsDirRole);
        roles.insert("isLink", IsLinkRole);
        roles.insert("isExpanded", IsExpandedRole);
        roles.insert("isExpandable", IsExpandableRole);
        roles.insert("expandedParentsCount", ExpandedParentsCountRole);

        Q_ASSERT(roles.count() == RolesCount);
    }

    return roles.value(role, NoRole);
}

QByteArray KFileItemModel::roleForType(RoleType roleType) const
{
    static QHash<RoleType, QByteArray> roles;
    if (roles.isEmpty()) {
        // Insert user visible roles that can be accessed with
        // KFileItemModel::roleInformation()
        int count = 0;
        const RoleInfoMap* map = rolesInfoMap(count);
        for (int i = 0; i < count; ++i) {
            roles.insert(map[i].roleType, map[i].role);
        }

        // Insert internal roles (take care to synchronize the implementation
        // with KFileItemModel::typeForRole() in case if a change is done).
        roles.insert(IsDirRole, "isDir");
        roles.insert(IsLinkRole, "isLink");
        roles.insert(IsExpandedRole, "isExpanded");
        roles.insert(IsExpandableRole, "isExpandable");
        roles.insert(ExpandedParentsCountRole, "expandedParentsCount");

        Q_ASSERT(roles.count() == RolesCount);
    };

    return roles.value(roleType);
}

QHash<QByteArray, QVariant> KFileItemModel::retrieveData(const KFileItem& item) const
{
    // It is important to insert only roles that are fast to retrieve. E.g.
    // KFileItem::iconName() can be very expensive if the MIME-type is unknown
    // and hence will be retrieved asynchronously by KFileItemModelRolesUpdater.
    QHash<QByteArray, QVariant> data;
    data.insert("url", item.url());

    const bool isDir = item.isDir();
    if (m_requestRole[IsDirRole]) {
        data.insert("isDir", isDir);
    }

    if (m_requestRole[IsLinkRole]) {
        const bool isLink = item.isLink();
        data.insert("isLink", isLink);
    }

    if (m_requestRole[NameRole]) {
        data.insert("text", item.text());
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
            destination = QLatin1String("-");
        }
        data.insert("destination", destination);
    }

    if (m_requestRole[PathRole]) {
        QString path;
        if (item.url().protocol() == QLatin1String("trash")) {
            path = item.entry().stringValue(KIO::UDSEntry::UDS_EXTRA);
        } else {
            // For performance reasons cache the home-path in a static QString
            // (see QDir::homePath() for more details)
            static QString homePath;
            if (homePath.isEmpty()) {
                homePath = QDir::homePath();
            }

            path = item.localPath();
            if (path.startsWith(homePath)) {
                path.replace(0, homePath.length(), QLatin1Char('~'));
            }
        }

        const int index = path.lastIndexOf(item.text());
        path = path.mid(0, index - 1);
        data.insert("path", path);
    }

    if (m_requestRole[IsExpandedRole]) {
        data.insert("isExpanded", false);
    }

    if (m_requestRole[IsExpandableRole]) {
        data.insert("isExpandable", item.isDir() && item.url() == item.targetUrl());
    }

    if (m_requestRole[ExpandedParentsCountRole]) {
        if (m_expandedParentsCountRoot == UninitializedExpandedParentsCountRoot) {
            const KUrl rootUrl = m_dirLister->url();
            const QString protocol = rootUrl.protocol();
            const bool forceExpandedParentsCountRoot = (protocol == QLatin1String("trash") ||
                                                        protocol == QLatin1String("nepomuk") ||
                                                        protocol == QLatin1String("remote") ||
                                                        protocol.contains(QLatin1String("search")));
            if (forceExpandedParentsCountRoot) {
                m_expandedParentsCountRoot = ForceExpandedParentsCountRoot;
            } else {
                const QString rootDir = rootUrl.path(KUrl::AddTrailingSlash);
                m_expandedParentsCountRoot = rootDir.count('/');
            }
        }

        if (m_expandedParentsCountRoot == ForceExpandedParentsCountRoot) {
            data.insert("expandedParentsCount", -1);
        } else {
            const QString dir = item.url().directory(KUrl::AppendTrailingSlash);
            const int level = dir.count('/') - m_expandedParentsCountRoot;
            data.insert("expandedParentsCount", level);
        }
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
    int result = 0;

    if (m_expandedParentsCountRoot >= 0) {
        result = expandedParentsCountCompare(a, b);
        if (result != 0) {
            // The items have parents with different expansion levels
            return (sortOrder() == Qt::AscendingOrder) ? result < 0 : result > 0;
        }
    }

    if (m_sortDirsFirst || m_sortRole == SizeRole) {
        const bool isDirA = a->item.isDir();
        const bool isDirB = b->item.isDir();
        if (isDirA && !isDirB) {
            return true;
        } else if (!isDirA && isDirB) {
            return false;
        }
    }

    result = sortRoleCompare(a, b);

    return (sortOrder() == Qt::AscendingOrder) ? result < 0 : result > 0;
}

/**
 * Helper class for KFileItemModel::sort().
 */
class KFileItemModelLessThan
{
public:
    KFileItemModelLessThan(const KFileItemModel* model) :
        m_model(model)
    {
    }

    bool operator()(const KFileItemModel::ItemData* a, const KFileItemModel::ItemData* b) const
    {
        return m_model->lessThan(a, b);
    }

private:
    const KFileItemModel* m_model;
};

void KFileItemModel::sort(QList<KFileItemModel::ItemData*>::iterator begin,
                          QList<KFileItemModel::ItemData*>::iterator end) const
{
    KFileItemModelLessThan lessThan(this);

    if (m_sortRole == NameRole) {
        // Sorting by name can be expensive, in particular if natural sorting is
        // enabled. Use all CPU cores to speed up the sorting process.
        static const int numberOfThreads = QThread::idealThreadCount();
        parallelMergeSort(begin, end, lessThan, numberOfThreads);
    } else {
        // Sorting by other roles is quite fast. Use only one thread to prevent
        // problems caused by non-reentrant comparison functions, see
        // https://bugs.kde.org/show_bug.cgi?id=312679
        mergeSort(begin, end, lessThan);
    }
}

int KFileItemModel::sortRoleCompare(const ItemData* a, const ItemData* b) const
{
    const KFileItem& itemA = a->item;
    const KFileItem& itemB = b->item;

    int result = 0;

    switch (m_sortRole) {
    case NameRole:
        // The name role is handled as default fallback after the switch
        break;

    case SizeRole: {
        if (itemA.isDir()) {
            // See "if (m_sortFoldersFirst || m_sortRole == SizeRole)" in KFileItemModel::lessThan():
            Q_ASSERT(itemB.isDir());

            const QVariant valueA = a->values.value("size");
            const QVariant valueB = b->values.value("size");
            if (valueA.isNull() && valueB.isNull()) {
                result = 0;
            } else if (valueA.isNull()) {
                result = -1;
            } else if (valueB.isNull()) {
                result = +1;
            } else {
                result = valueA.toInt() - valueB.toInt();
            }
        } else {
            // See "if (m_sortFoldersFirst || m_sortRole == SizeRole)" in KFileItemModel::lessThan():
            Q_ASSERT(!itemB.isDir());
            const KIO::filesize_t sizeA = itemA.size();
            const KIO::filesize_t sizeB = itemB.size();
            if (sizeA > sizeB) {
                result = +1;
            } else if (sizeA < sizeB) {
                result = -1;
            } else {
                result = 0;
            }
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

    case RatingRole: {
        result = a->values.value("rating").toInt() - b->values.value("rating").toInt();
        break;
    }

    case ImageSizeRole: {
        // Alway use a natural comparing to interpret the numbers of a string like
        // "1600 x 1200" for having a correct sorting.
        result = KStringHandler::naturalCompare(a->values.value("imageSize").toString(),
                                                b->values.value("imageSize").toString(),
                                                Qt::CaseSensitive);
        break;
    }

    default: {
        const QByteArray role = roleForType(m_sortRole);
        result = QString::compare(a->values.value(role).toString(),
                                  b->values.value(role).toString());
        break;
    }

    }

    if (result != 0) {
        // The current sort role was sufficient to define an order
        return result;
    }

    // Fallback #1: Compare the text of the items
    result = stringCompare(itemA.text(), itemB.text());
    if (result != 0) {
        return result;
    }

    // Fallback #2: KFileItem::text() may not be unique in case UDS_DISPLAY_NAME is used
    result = stringCompare(itemA.name(m_caseSensitivity == Qt::CaseInsensitive),
                           itemB.name(m_caseSensitivity == Qt::CaseInsensitive));
    if (result != 0) {
        return result;
    }

    // Fallback #3: It must be assured that the sort order is always unique even if two values have been
    // equal. In this case a comparison of the URL is done which is unique in all cases
    // within KDirLister.
    return QString::compare(itemA.url().url(), itemB.url().url(), Qt::CaseSensitive);
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

int KFileItemModel::expandedParentsCountCompare(const ItemData* a, const ItemData* b) const
{
    const KUrl urlA = a->item.url();
    const KUrl urlB = b->item.url();
    if (urlA.directory() == urlB.directory()) {
        // Both items have the same directory as parent
        return 0;
    }

    // Check whether one item is the parent of the other item
    if (urlA.isParentOf(urlB)) {
        return (sortOrder() == Qt::AscendingOrder) ? -1 : +1;
    } else if (urlB.isParentOf(urlA)) {
        return (sortOrder() == Qt::AscendingOrder) ? +1 : -1;
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
    while (index > 0 && (pathA.at(index) != QLatin1Char('/') || pathB.at(index) != QLatin1Char('/'))) {
        --index;
    }

    // Determine the first sub-path after the common path and
    // check whether it represents a directory or already a file
    bool isDirA = true;
    const QString subPathA = subPath(a->item, pathA, index, &isDirA);
    bool isDirB = true;
    const QString subPathB = subPath(b->item, pathB, index, &isDirB);

    if (m_sortDirsFirst || m_sortRole == SizeRole) {
        if (isDirA && !isDirB) {
            return (sortOrder() == Qt::AscendingOrder) ? -1 : +1;
        } else if (!isDirA && isDirB) {
            return (sortOrder() == Qt::AscendingOrder) ? +1 : -1;
        }
    }

    // Compare the items of the parents that represent the first
    // different path after the common path.
    const QString parentPathA = pathA.left(index) + subPathA;
    const QString parentPathB = pathB.left(index) + subPathB;

    const ItemData* parentA = a;
    while (parentA && parentA->item.url().path() != parentPathA) {
        parentA = parentA->parent;
    }

    const ItemData* parentB = b;
    while (parentB && parentB->item.url().path() != parentPathB) {
        parentB = parentB->parent;
    }

    if (parentA && parentB) {
        return sortRoleCompare(parentA, parentB);
    }

    kWarning() << "Child items without parent detected:" << a->item.url() << b->item.url();
    return QString::compare(urlA.url(), urlB.url(), Qt::CaseSensitive);
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
    return !m_dirLister->url().isLocalFile();
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

        const QString name = m_itemData.at(i)->values.value("text").toString();

        // Use the first character of the name as group indication
        QChar newFirstChar = name.at(0).toUpper();
        if (newFirstChar == QLatin1Char('~') && name.length() > 1) {
            newFirstChar = name.at(1).toUpper();
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

    int groupValue = -1;
    for (int i = 0; i <= maxIndex; ++i) {
        if (isChildItem(i)) {
            continue;
        }
        const int newGroupValue = m_itemData.at(i)->values.value("rating", 0).toInt();
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

    bool isFirstGroupValue = true;
    QString groupValue;
    for (int i = 0; i <= maxIndex; ++i) {
        if (isChildItem(i)) {
            continue;
        }
        const QString newGroupValue = m_itemData.at(i)->values.value(role).toString();
        if (newGroupValue != groupValue || isFirstGroupValue) {
            groupValue = newGroupValue;
            groups.append(QPair<int, QVariant>(i, newGroupValue));
            isFirstGroupValue = false;
        }
    }

    return groups;
}

KFileItemList KFileItemModel::childItems(const KFileItem& item) const
{
    KFileItemList items;

    int index = m_items.value(item.url(), -1);
    if (index >= 0) {
        const int parentLevel = m_itemData.at(index)->values.value("expandedParentsCount").toInt();
        ++index;
        while (index < m_itemData.count() && m_itemData.at(index)->values.value("expandedParentsCount").toInt() > parentLevel) {
            items.append(m_itemData.at(index)->item);
            ++index;
        }
    }

    return items;
}

void KFileItemModel::emitSortProgress(int resolvedCount)
{
    // Be tolerant against a resolvedCount with a wrong range.
    // Although there should not be a case where KFileItemModelRolesUpdater
    // (= caller) provides a wrong range, it is important to emit
    // a useful progress information even if there is an unexpected
    // implementation issue.

    const int itemCount = count();
    if (resolvedCount >= itemCount) {
        m_sortingProgressPercent = -1;
        if (m_resortAllItemsTimer->isActive()) {
            m_resortAllItemsTimer->stop();
            resortAllItems();
        }

        emit directorySortingProgress(100);
    } else if (itemCount > 0) {
        resolvedCount = qBound(0, resolvedCount, itemCount);

        const int progress = resolvedCount * 100 / itemCount;
        if (m_sortingProgressPercent != progress) {
            m_sortingProgressPercent = progress;
            emit directorySortingProgress(progress);
        }
    }
}

const KFileItemModel::RoleInfoMap* KFileItemModel::rolesInfoMap(int& count)
{
    static const RoleInfoMap rolesInfoMap[] = {
    //  | role         | roleType       | role translation                                | group translation           | requires Nepomuk | requires indexer
        { 0,             NoRole,          0, 0,                                             0, 0,                                     false, false },
        { "text",        NameRole,        I18N_NOOP2_NOSTRIP("@label", "Name"),             0, 0,                                     false, false },
        { "size",        SizeRole,        I18N_NOOP2_NOSTRIP("@label", "Size"),             0, 0,                                     false, false },
        { "date",        DateRole,        I18N_NOOP2_NOSTRIP("@label", "Date"),             0, 0,                                     false, false },
        { "type",        TypeRole,        I18N_NOOP2_NOSTRIP("@label", "Type"),             0, 0,                                     false, false },
        { "rating",      RatingRole,      I18N_NOOP2_NOSTRIP("@label", "Rating"),           0, 0,                                     true,  false },
        { "tags",        TagsRole,        I18N_NOOP2_NOSTRIP("@label", "Tags"),             0, 0,                                     true,  false },
        { "comment",     CommentRole,     I18N_NOOP2_NOSTRIP("@label", "Comment"),          0, 0,                                     true,  false },
        { "wordCount",   WordCountRole,   I18N_NOOP2_NOSTRIP("@label", "Word Count"),       I18N_NOOP2_NOSTRIP("@label", "Document"), true,  true  },
        { "lineCount",   LineCountRole,   I18N_NOOP2_NOSTRIP("@label", "Line Count"),       I18N_NOOP2_NOSTRIP("@label", "Document"), true,  true  },
        { "imageSize",   ImageSizeRole,   I18N_NOOP2_NOSTRIP("@label", "Image Size"),       I18N_NOOP2_NOSTRIP("@label", "Image"),    true,  true  },
        { "orientation", OrientationRole, I18N_NOOP2_NOSTRIP("@label", "Orientation"),      I18N_NOOP2_NOSTRIP("@label", "Image"),    true,  true  },
        { "artist",      ArtistRole,      I18N_NOOP2_NOSTRIP("@label", "Artist"),           I18N_NOOP2_NOSTRIP("@label", "Audio"),    true,  true  },
        { "album",       AlbumRole,       I18N_NOOP2_NOSTRIP("@label", "Album"),            I18N_NOOP2_NOSTRIP("@label", "Audio"),    true,  true  },
        { "duration",    DurationRole,    I18N_NOOP2_NOSTRIP("@label", "Duration"),         I18N_NOOP2_NOSTRIP("@label", "Audio"),    true,  true  },
        { "track",       TrackRole,       I18N_NOOP2_NOSTRIP("@label", "Track"),            I18N_NOOP2_NOSTRIP("@label", "Audio"),    true,  true  },
        { "path",        PathRole,        I18N_NOOP2_NOSTRIP("@label", "Path"),             I18N_NOOP2_NOSTRIP("@label", "Other"),    false, false },
        { "destination", DestinationRole, I18N_NOOP2_NOSTRIP("@label", "Link Destination"), I18N_NOOP2_NOSTRIP("@label", "Other"),    false, false },
        { "copiedFrom",  CopiedFromRole,  I18N_NOOP2_NOSTRIP("@label", "Copied From"),      I18N_NOOP2_NOSTRIP("@label", "Other"),    true,  false },
        { "permissions", PermissionsRole, I18N_NOOP2_NOSTRIP("@label", "Permissions"),      I18N_NOOP2_NOSTRIP("@label", "Other"),    false, false },
        { "owner",       OwnerRole,       I18N_NOOP2_NOSTRIP("@label", "Owner"),            I18N_NOOP2_NOSTRIP("@label", "Other"),    false, false },
        { "group",       GroupRole,       I18N_NOOP2_NOSTRIP("@label", "User Group"),       I18N_NOOP2_NOSTRIP("@label", "Other"),    false, false },
    };

    count = sizeof(rolesInfoMap) / sizeof(RoleInfoMap);
    return rolesInfoMap;
}

void KFileItemModel::determineMimeTypes(const KFileItemList& items, int timeout)
{
    QElapsedTimer timer;
    timer.start();
    foreach (KFileItem item, items) { // krazy:exclude=foreach
        item.determineMimeType();
        if (timer.elapsed() > timeout) {
            // Don't block the user interface, let the remaining items
            // be resolved asynchronously.
            return;
        }
    }
}

bool KFileItemModel::isConsistent() const
{
    // Check that m_items and m_itemData are consistent, and that the items are sorted.
    if (m_items.count() != m_itemData.count()) {
        return false;
    }

    for (int i = 0; i < count(); ++i) {
        const KFileItem item = fileItem(i);
        if (item.isNull()) {
            qWarning() << "Item" << i << "is null";
            return false;
        }

        const int itemIndex = index(item);
        if (itemIndex != i) {
            qWarning() << "Item" << i << "has a wrong index:" << itemIndex;
            return false;
        }

        if (i > 0 && !lessThan(m_itemData.at(i - 1), m_itemData.at(i))) {
            qWarning() << "The order of items" << i - 1 << "and" << i << "is wrong:"
                << fileItem(i - 1) << fileItem(i);
            return false;
        }
    }

    return true;
}

#include "kfileitemmodel.moc"
