/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2013 Frank Reininghaus <frank78ac@googlemail.com>
 * SPDX-FileCopyrightText: 2013 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kfileitemmodel.h"

#include "dolphin_contentdisplaysettings.h"
#include "dolphin_generalsettings.h"
#include "dolphindebug.h"
#include "private/kfileitemmodelsortalgorithm.h"
#include "views/draganddrophelper.h"

#include <KDirLister>
#include <KIO/Job>
#include <KIO/ListJob>
#include <KLocalizedString>
#include <KUrlMimeData>

#ifndef QT_NO_ACCESSIBILITY
#include <QAccessible>
#endif
#include <QElapsedTimer>
#include <QIcon>
#include <QMimeData>
#include <QMimeDatabase>
#include <QRecursiveMutex>
#include <QTimer>
#include <QWidget>
#include <klazylocalizedstring.h>

Q_GLOBAL_STATIC(QRecursiveMutex, s_collatorMutex)

// #define KFILEITEMMODEL_DEBUG

KFileItemModel::KFileItemModel(QObject *parent)
    : KItemModelBase("text", parent)
    , m_dirLister(nullptr)
    , m_sortDirsFirst(true)
    , m_sortHiddenLast(false)
    , m_sortRole(NameRole)
    , m_sortingProgressPercent(-1)
    , m_roles()
    , m_itemData()
    , m_items()
    , m_filter()
    , m_filteredItems()
    , m_requestRole()
    , m_maximumUpdateIntervalTimer(nullptr)
    , m_resortAllItemsTimer(nullptr)
    , m_pendingItemsToInsert()
    , m_groups()
    , m_expandedDirs()
    , m_urlsToExpand()
{
    m_collator.setNumericMode(true);

    loadSortingSettings();

    m_dirLister = new KDirLister(this);
    m_dirLister->setAutoErrorHandlingEnabled(false);
    m_dirLister->setDelayedMimeTypes(true);

    const QWidget *parentWidget = qobject_cast<QWidget *>(parent);
    if (parentWidget) {
        m_dirLister->setMainWindow(parentWidget->window());
    }

    connect(m_dirLister, &KCoreDirLister::started, this, &KFileItemModel::directoryLoadingStarted);
    connect(m_dirLister, &KCoreDirLister::canceled, this, &KFileItemModel::slotCanceled);
    connect(m_dirLister, &KCoreDirLister::itemsAdded, this, &KFileItemModel::slotItemsAdded);
    connect(m_dirLister, &KCoreDirLister::itemsDeleted, this, &KFileItemModel::slotItemsDeleted);
    connect(m_dirLister, &KCoreDirLister::refreshItems, this, &KFileItemModel::slotRefreshItems);
    connect(m_dirLister, &KCoreDirLister::clear, this, &KFileItemModel::slotClear);
    connect(m_dirLister, &KCoreDirLister::infoMessage, this, &KFileItemModel::infoMessage);
    connect(m_dirLister, &KCoreDirLister::jobError, this, &KFileItemModel::slotListerError);
    connect(m_dirLister, &KCoreDirLister::percent, this, &KFileItemModel::directoryLoadingProgress);
    connect(m_dirLister, &KCoreDirLister::redirection, this, &KFileItemModel::directoryRedirection);
    connect(m_dirLister, &KCoreDirLister::listingDirCompleted, this, &KFileItemModel::slotCompleted);

    // Apply default roles that should be determined
    resetRoles();
    m_requestRole[NameRole] = true;
    m_requestRole[IsDirRole] = true;
    m_requestRole[IsLinkRole] = true;
    m_roles.insert("text");
    m_roles.insert("isDir");
    m_roles.insert("isLink");
    m_roles.insert("isHidden");

    // For slow KIO-slaves like used for searching it makes sense to show results periodically even
    // before the completed() or canceled() signal has been emitted.
    m_maximumUpdateIntervalTimer = new QTimer(this);
    m_maximumUpdateIntervalTimer->setInterval(2000);
    m_maximumUpdateIntervalTimer->setSingleShot(true);
    connect(m_maximumUpdateIntervalTimer, &QTimer::timeout, this, &KFileItemModel::dispatchPendingItemsToInsert);

    // When changing the value of an item which represents the sort-role a resorting must be
    // triggered. Especially in combination with KFileItemModelRolesUpdater this might be done
    // for a lot of items within a quite small timeslot. To prevent expensive resortings the
    // resorting is postponed until the timer has been exceeded.
    m_resortAllItemsTimer = new QTimer(this);
    m_resortAllItemsTimer->setInterval(100); // 100 is a middle ground between sorting too frequently which makes the view unreadable
                                             // and sorting too infrequently which leads to users seeing an outdated sort order.
    m_resortAllItemsTimer->setSingleShot(true);
    connect(m_resortAllItemsTimer, &QTimer::timeout, this, &KFileItemModel::resortAllItems);

    connect(GeneralSettings::self(), &GeneralSettings::sortingChoiceChanged, this, &KFileItemModel::slotSortingChoiceChanged);

    setShowTrashMime(m_dirLister->showHiddenFiles() || !GeneralSettings::hideXTrashFile());
}

KFileItemModel::~KFileItemModel()
{
    qDeleteAll(m_itemData);
    qDeleteAll(m_filteredItems);
    qDeleteAll(m_pendingItemsToInsert);
}

void KFileItemModel::loadDirectory(const QUrl &url)
{
    m_dirLister->openUrl(url);
}

void KFileItemModel::refreshDirectory(const QUrl &url)
{
    // Refresh all expanded directories first (Bug 295300)
    QHashIterator<QUrl, QUrl> expandedDirs(m_expandedDirs);
    while (expandedDirs.hasNext()) {
        expandedDirs.next();
        m_dirLister->openUrl(expandedDirs.value(), KDirLister::Reload);
    }

    m_dirLister->openUrl(url, KDirLister::Reload);

    Q_EMIT directoryRefreshing();
}

QUrl KFileItemModel::directory() const
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
        ItemData *data = m_itemData.at(index);
        if (data->values.isEmpty()) {
            data->values = retrieveData(data->item, data->parent);
        } else if (data->values.count() <= 2 && data->values.value("isExpanded").toBool()) {
            // Special case dealt by slotRefreshItems(), avoid losing the "isExpanded" and "expandedParentsCount" state when refreshing
            // slotRefreshItems() makes sure folders keep the "isExpanded" and "expandedParentsCount" while clearing the remaining values
            // so this special request of different behavior can be identified here.
            bool hasExpandedParentsCount = false;
            const int expandedParentsCount = data->values.value("expandedParentsCount").toInt(&hasExpandedParentsCount);

            data->values = retrieveData(data->item, data->parent);
            data->values.insert("isExpanded", true);
            if (hasExpandedParentsCount) {
                data->values.insert("expandedParentsCount", expandedParentsCount);
            }
        }

        return data->values;
    }
    return QHash<QByteArray, QVariant>();
}

bool KFileItemModel::setData(int index, const QHash<QByteArray, QVariant> &values)
{
    if (index < 0 || index >= count()) {
        return false;
    }

    QHash<QByteArray, QVariant> currentValues = data(index);

    // Determine which roles have been changed
    QSet<QByteArray> changedRoles;
    QHashIterator<QByteArray, QVariant> it(values);
    while (it.hasNext()) {
        it.next();
        const QByteArray role = sharedValue(it.key());
        const QVariant value = it.value();

        if (currentValues[role] != value) {
            currentValues[role] = value;
            changedRoles.insert(role);
        }
    }

    if (changedRoles.isEmpty()) {
        return false;
    }

    if (changedRoles.contains("text")) {
        QUrl url = m_itemData[index]->item.url();
        m_items.remove(url);
        url = url.adjusted(QUrl::RemoveFilename);
        url.setPath(url.path() + currentValues["text"].toString());
        m_itemData[index]->item.setUrl(url);
        m_items.insert(url, index);

        if (!changedRoles.contains("url")) {
            changedRoles.insert("url");
            currentValues["url"] = url;
        }
    }
    m_itemData[index]->values = currentValues;

    emitItemsChangedAndTriggerResorting(KItemRangeList() << KItemRange(index, 1), changedRoles);

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

void KFileItemModel::setSortHiddenLast(bool hiddenLast)
{
    if (hiddenLast != m_sortHiddenLast) {
        m_sortHiddenLast = hiddenLast;
        resortAllItems();
    }
}

bool KFileItemModel::sortHiddenLast() const
{
    return m_sortHiddenLast;
}

void KFileItemModel::setShowTrashMime(bool showTrashMime)
{
    const auto trashMime = QStringLiteral("application/x-trash");
    QStringList excludeFilter = m_filter.excludeMimeTypes();

    if (showTrashMime) {
        excludeFilter.removeAll(trashMime);
    } else if (!excludeFilter.contains(trashMime)) {
        excludeFilter.append(trashMime);
    }

    setExcludeMimeTypeFilter(excludeFilter);
}

void KFileItemModel::scheduleResortAllItems()
{
    if (!m_resortAllItemsTimer->isActive()) {
        m_resortAllItemsTimer->start();
    }
}

void KFileItemModel::setShowHiddenFiles(bool show)
{
    m_dirLister->setShowHiddenFiles(show);
    setShowTrashMime(show || !GeneralSettings::hideXTrashFile());
    m_dirLister->emitChanges();
    if (show) {
        dispatchPendingItemsToInsert();
    }
}

bool KFileItemModel::showHiddenFiles() const
{
    return m_dirLister->showHiddenFiles();
}

void KFileItemModel::setShowDirectoriesOnly(bool enabled)
{
    m_dirLister->setDirOnlyMode(enabled);
}

bool KFileItemModel::showDirectoriesOnly() const
{
    return m_dirLister->dirOnlyMode();
}

QMimeData *KFileItemModel::createMimeData(const KItemSet &indexes) const
{
    QMimeData *data = new QMimeData();

    // The following code has been taken from KDirModel::mimeData()
    // (kdelibs/kio/kio/kdirmodel.cpp)
    // SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>
    QList<QUrl> urls;
    QList<QUrl> mostLocalUrls;
    const ItemData *lastAddedItem = nullptr;

    for (int index : indexes) {
        const ItemData *itemData = m_itemData.at(index);
        const ItemData *parent = itemData->parent;

        while (parent && parent != lastAddedItem) {
            parent = parent->parent;
        }

        if (parent && parent == lastAddedItem) {
            // A parent of 'itemData' has been added already.
            continue;
        }

        lastAddedItem = itemData;
        const KFileItem &item = itemData->item;
        if (!item.isNull()) {
            urls << item.url();

            bool isLocal;
            mostLocalUrls << item.mostLocalUrl(&isLocal);
        }
    }

    KUrlMimeData::setUrls(urls, mostLocalUrls, data);
    return data;
}

int KFileItemModel::indexForKeyboardSearch(const QString &text, int startFromIndex) const
{
    startFromIndex = qMax(0, startFromIndex);
    for (int i = startFromIndex; i < count(); ++i) {
        if (fileItem(i).text().startsWith(text, Qt::CaseInsensitive)) {
            return i;
        }
    }
    for (int i = 0; i < startFromIndex; ++i) {
        if (fileItem(i).text().startsWith(text, Qt::CaseInsensitive)) {
            return i;
        }
    }
    return -1;
}

bool KFileItemModel::supportsDropping(int index) const
{
    KFileItem item;
    if (index == -1) {
        item = rootItem();
    } else {
        item = fileItem(index);
    }
    return !item.isNull() && DragAndDropHelper::supportsDropping(item);
}

bool KFileItemModel::canEnterOnHover(int index) const
{
    KFileItem item;
    if (index == -1) {
        item = rootItem();
    } else {
        item = fileItem(index);
    }
    return !item.isNull() && (item.isDir() || item.isDesktopFile());
}

QString KFileItemModel::roleDescription(const QByteArray &role) const
{
    static QHash<QByteArray, QString> description;
    if (description.isEmpty()) {
        int count = 0;
        const RoleInfoMap *map = rolesInfoMap(count);
        for (int i = 0; i < count; ++i) {
            if (map[i].roleTranslation.isEmpty()) {
                continue;
            }
            description.insert(map[i].role, map[i].roleTranslation.toString());
        }
    }

    return description.value(role);
}

QList<QPair<int, QVariant>> KFileItemModel::groups() const
{
    if (!m_itemData.isEmpty() && m_groups.isEmpty()) {
#ifdef KFILEITEMMODEL_DEBUG
        QElapsedTimer timer;
        timer.start();
#endif
        switch (typeForRole(sortRole())) {
        case NameRole:
            m_groups = nameRoleGroups();
            break;
        case SizeRole:
            m_groups = sizeRoleGroups();
            break;
        case ModificationTimeRole:
            m_groups = timeRoleGroups([](const ItemData *item) {
                return item->item.time(KFileItem::ModificationTime);
            });
            break;
        case CreationTimeRole:
            m_groups = timeRoleGroups([](const ItemData *item) {
                return item->item.time(KFileItem::CreationTime);
            });
            break;
        case AccessTimeRole:
            m_groups = timeRoleGroups([](const ItemData *item) {
                return item->item.time(KFileItem::AccessTime);
            });
            break;
        case DeletionTimeRole:
            m_groups = timeRoleGroups([](const ItemData *item) {
                return item->values.value("deletiontime").toDateTime();
            });
            break;
        case PermissionsRole:
            m_groups = permissionRoleGroups();
            break;
        case RatingRole:
            m_groups = ratingRoleGroups();
            break;
        default:
            m_groups = genericStringRoleGroups(sortRole());
            break;
        }

#ifdef KFILEITEMMODEL_DEBUG
        qCDebug(DolphinDebug) << "[TIME] Calculating groups for" << count() << "items:" << timer.elapsed();
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

KFileItem KFileItemModel::fileItem(const QUrl &url) const
{
    const int indexForUrl = index(url);
    if (indexForUrl >= 0) {
        return m_itemData.at(indexForUrl)->item;
    }
    return KFileItem();
}

int KFileItemModel::index(const KFileItem &item) const
{
    return index(item.url());
}

int KFileItemModel::index(const QUrl &url) const
{
    const QUrl urlToFind = url.adjusted(QUrl::StripTrailingSlash);

    const int itemCount = m_itemData.count();
    int itemsInHash = m_items.count();

    int index = m_items.value(urlToFind, -1);
    while (index < 0 && itemsInHash < itemCount) {
        // Not all URLs are stored yet in m_items. We grow m_items until either
        // urlToFind is found, or all URLs have been stored in m_items.
        // Note that we do not add the URLs to m_items one by one, but in
        // larger blocks. After each block, we check if urlToFind is in
        // m_items. We could in principle compare urlToFind with each URL while
        // we are going through m_itemData, but comparing two QUrls will,
        // unlike calling qHash for the URLs, trigger a parsing of the URLs
        // which costs both CPU cycles and memory.
        const int blockSize = 1000;
        const int currentBlockEnd = qMin(itemsInHash + blockSize, itemCount);
        for (int i = itemsInHash; i < currentBlockEnd; ++i) {
            const QUrl nextUrl = m_itemData.at(i)->item.url();
            m_items.insert(nextUrl, i);
        }

        itemsInHash = currentBlockEnd;
        index = m_items.value(urlToFind, -1);
    }

    if (index < 0) {
        // The item could not be found, even though all items from m_itemData
        // should be in m_items now. We print some diagnostic information which
        // might help to find the cause of the problem, but only once. This
        // prevents that obtaining and printing the debugging information
        // wastes CPU cycles and floods the shell or .xsession-errors.
        static bool printDebugInfo = true;

        if (m_items.count() != m_itemData.count() && printDebugInfo) {
            printDebugInfo = false;

            qCWarning(DolphinDebug) << "The model is in an inconsistent state.";
            qCWarning(DolphinDebug) << "m_items.count()    ==" << m_items.count();
            qCWarning(DolphinDebug) << "m_itemData.count() ==" << m_itemData.count();

            // Check if there are multiple items with the same URL.
            QMultiHash<QUrl, int> indexesForUrl;
            for (int i = 0; i < m_itemData.count(); ++i) {
                indexesForUrl.insert(m_itemData.at(i)->item.url(), i);
            }

            const auto uniqueKeys = indexesForUrl.uniqueKeys();
            for (const QUrl &url : uniqueKeys) {
                if (indexesForUrl.count(url) > 1) {
                    qCWarning(DolphinDebug) << "Multiple items found with the URL" << url;

                    auto it = indexesForUrl.find(url);
                    while (it != indexesForUrl.end() && it.key() == url) {
                        const ItemData *data = m_itemData.at(it.value());
                        qCWarning(DolphinDebug) << "index" << it.value() << ":" << data->item;
                        if (data->parent) {
                            qCWarning(DolphinDebug) << "parent" << data->parent->item;
                        }
                        ++it;
                    }
                }
            }
        }
    }

    return index;
}

KFileItem KFileItemModel::rootItem() const
{
    return m_dirLister->rootItem();
}

void KFileItemModel::clear()
{
    slotClear();
}

void KFileItemModel::setRoles(const QSet<QByteArray> &roles)
{
    if (m_roles == roles) {
        return;
    }

    const QSet<QByteArray> changedRoles = (roles - m_roles) + (m_roles - roles);
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
        const QByteArray &role = it.next();
        m_requestRole[typeForRole(role)] = true;
    }

    if (count() > 0) {
        // Update m_data with the changed requested roles
        const int maxIndex = count() - 1;
        for (int i = 0; i <= maxIndex; ++i) {
            m_itemData[i]->values = retrieveData(m_itemData.at(i)->item, m_itemData.at(i)->parent);
        }

        Q_EMIT itemsChanged(KItemRangeList() << KItemRange(0, count()), changedRoles);
    }

    // Clear the 'values' of all filtered items. They will be re-populated with the
    // correct roles the next time 'values' will be accessed via data(int).
    QHash<KFileItem, ItemData *>::iterator filteredIt = m_filteredItems.begin();
    const QHash<KFileItem, ItemData *>::iterator filteredEnd = m_filteredItems.end();
    while (filteredIt != filteredEnd) {
        (*filteredIt)->values.clear();
        ++filteredIt;
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
    values.insert(sharedValue("isExpanded"), expanded);
    if (!setData(index, values)) {
        return false;
    }

    const KFileItem item = m_itemData.at(index)->item;
    const QUrl url = item.url();
    const QUrl targetUrl = item.targetUrl();
    if (expanded) {
        m_expandedDirs.insert(targetUrl, url);
        m_dirLister->openUrl(url, KDirLister::Keep);

        const QVariantList previouslyExpandedChildren = m_itemData.at(index)->values.value("previouslyExpandedChildren").value<QVariantList>();
        for (const QVariant &var : previouslyExpandedChildren) {
            m_urlsToExpand.insert(var.toUrl());
        }
    } else {
        // Note that there might be (indirect) children of the folder which is to be collapsed in
        // m_pendingItemsToInsert. To prevent that they will be inserted into the model later,
        // possibly without a parent, which might result in a crash, we insert all pending items
        // right now. All new items which would be without a parent will then be removed.
        dispatchPendingItemsToInsert();

        // Check if the index of the collapsed folder has changed. If that is the case, then items
        // were inserted before the collapsed folder, and its index needs to be updated.
        if (m_itemData.at(index)->item != item) {
            index = this->index(item);
        }

        m_expandedDirs.remove(targetUrl);
        m_dirLister->stop(url);
        m_dirLister->forgetDirs(url);

        const int parentLevel = expandedParentsCount(index);
        const int itemCount = m_itemData.count();
        const int firstChildIndex = index + 1;

        QVariantList expandedChildren;

        int childIndex = firstChildIndex;
        while (childIndex < itemCount && expandedParentsCount(childIndex) > parentLevel) {
            ItemData *itemData = m_itemData.at(childIndex);
            if (itemData->values.value("isExpanded").toBool()) {
                const QUrl targetUrl = itemData->item.targetUrl();
                const QUrl url = itemData->item.url();
                m_expandedDirs.remove(targetUrl);
                m_dirLister->stop(url); // TODO: try to unit-test this, see https://bugs.kde.org/show_bug.cgi?id=332102#c11
                m_dirLister->forgetDirs(url);
                expandedChildren.append(targetUrl);
            }
            ++childIndex;
        }
        const int childrenCount = childIndex - firstChildIndex;

        removeFilteredChildren(KItemRangeList() << KItemRange(index, 1 + childrenCount));
        removeItems(KItemRangeList() << KItemRange(firstChildIndex, childrenCount), DeleteItemData);

        m_itemData.at(index)->values.insert("previouslyExpandedChildren", expandedChildren);
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
        // Call data (instead of accessing m_itemData directly)
        // to ensure that the value is initialized.
        return data(index).value("isExpandable").toBool();
    }
    return false;
}

int KFileItemModel::expandedParentsCount(int index) const
{
    if (index >= 0 && index < count()) {
        return expandedParentsCount(m_itemData.at(index));
    }
    return 0;
}

QSet<QUrl> KFileItemModel::expandedDirectories() const
{
    QSet<QUrl> result;
    const auto dirs = m_expandedDirs;
    for (const auto &dir : dirs) {
        result.insert(dir);
    }
    return result;
}

void KFileItemModel::restoreExpandedDirectories(const QSet<QUrl> &urls)
{
    m_urlsToExpand = urls;
}

void KFileItemModel::expandParentDirectories(const QUrl &url)
{
    // Assure that each sub-path of the URL that should be
    // expanded is added to m_urlsToExpand. KDirLister
    // does not care whether the parent-URL has already been
    // expanded.
    QUrl urlToExpand = m_dirLister->url();
    const int pos = urlToExpand.path().length();

    // first subdir can be empty, if m_dirLister->url().path() does not end with '/'
    // this happens if baseUrl is not root but a home directory, see FoldersPanel,
    // so using QString::SkipEmptyParts
    const QStringList subDirs = url.path().mid(pos).split(QDir::separator(), Qt::SkipEmptyParts);
    for (int i = 0; i < subDirs.count() - 1; ++i) {
        QString path = urlToExpand.path();
        if (!path.endsWith(QLatin1Char('/'))) {
            path.append(QLatin1Char('/'));
        }
        urlToExpand.setPath(path + subDirs.at(i));
        m_urlsToExpand.insert(urlToExpand);
    }

    // KDirLister::open() must called at least once to trigger an initial
    // loading. The pending URLs that must be restored are handled
    // in slotCompleted().
    QSetIterator<QUrl> it2(m_urlsToExpand);
    while (it2.hasNext()) {
        const int idx = index(it2.next());
        if (idx >= 0 && !isExpanded(idx)) {
            setExpanded(idx, true);
            break;
        }
    }
}

void KFileItemModel::setNameFilter(const QString &nameFilter)
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

void KFileItemModel::setMimeTypeFilters(const QStringList &filters)
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

void KFileItemModel::setExcludeMimeTypeFilter(const QStringList &filters)
{
    if (m_filter.excludeMimeTypes() != filters) {
        dispatchPendingItemsToInsert();
        m_filter.setExcludeMimeTypes(filters);
        applyFilters();
    }
}

QStringList KFileItemModel::excludeMimeTypeFilter() const
{
    return m_filter.excludeMimeTypes();
}

void KFileItemModel::applyFilters()
{
    // ===STEP 1===
    // Check which previously shown items from m_itemData must now get
    // hidden and hence moved from m_itemData into m_filteredItems.

    QList<int> newFilteredIndexes; // This structure is good for prepending. We will want an ascending sorted Container at the end, this will do fine.

    // This pointer will refer to the next confirmed shown item from the point of
    // view of the current "itemData" in the upcoming "for" loop.
    ItemData *itemShownBelow = nullptr;

    // We will iterate backwards because it's convenient to know beforehand if the item just below is its child or not.
    for (int index = m_itemData.count() - 1; index >= 0; --index) {
        ItemData *itemData = m_itemData.at(index);

        if (m_filter.matches(itemData->item) || (itemShownBelow && itemShownBelow->parent == itemData)) {
            // We could've entered here for two reasons:
            // 1. This item passes the filter itself
            // 2. This is an expanded folder that doesn't pass the filter but sees a filter-passing child just below

            // So this item must remain shown.
            // Lets register this item as the next shown item from the point of view of the next iteration of this for loop
            itemShownBelow = itemData;
        } else {
            // We hide this item for now, however, for expanded folders this is not final:
            // if after the next "for" loop we discover that its children must now be shown with the newly applied fliter, we shall re-insert it
            newFilteredIndexes.prepend(index);
            m_filteredItems.insert(itemData->item, itemData);
            // indexShownBelow doesn't get updated since this item will be hidden
        }
    }

    // This will remove the newly filtered items from m_itemData
    removeItems(KItemRangeList::fromSortedContainer(newFilteredIndexes), KeepItemData);

    // ===STEP 2===
    // Check which hidden items from m_filteredItems should
    // become visible again and hence moved from m_filteredItems back into m_itemData.

    QList<ItemData *> newVisibleItems;

    QHash<KFileItem, ItemData *> ancestorsOfNewVisibleItems; // We will make sure these also become visible in step 3.

    QHash<KFileItem, ItemData *>::iterator it = m_filteredItems.begin();
    while (it != m_filteredItems.end()) {
        if (m_filter.matches(it.key())) {
            newVisibleItems.append(it.value());

            // If this is a child of an expanded folder, we must make sure that its whole parental chain will also be shown.
            // We will go up through its parental chain until we either:
            // 1 - reach the "root item" of the current view, i.e the currently opened folder on Dolphin. Their children have their ItemData::parent set to
            // nullptr. or 2 - we reach an unfiltered parent or a previously discovered ancestor.
            for (ItemData *parent = it.value()->parent; parent && !ancestorsOfNewVisibleItems.contains(parent->item) && m_filteredItems.contains(parent->item);
                 parent = parent->parent) {
                // We wish we could remove this parent from m_filteredItems right now, but we are iterating over it
                // and it would mess up the iteration. We will mark it to be removed in step 3.
                ancestorsOfNewVisibleItems.insert(parent->item, parent);
            }

            it = m_filteredItems.erase(it);
        } else {
            // Item remains filtered for now
            // However, for expanded folders this is not final, we may discover later that it has unfiltered descendants.
            ++it;
        }
    }

    // ===STEP 3===
    // Handles the ancestorsOfNewVisibleItems.
    // Now that we are done iterating through m_filteredItems we can safely move the ancestorsOfNewVisibleItems from m_filteredItems to newVisibleItems.
    for (it = ancestorsOfNewVisibleItems.begin(); it != ancestorsOfNewVisibleItems.end(); it++) {
        if (m_filteredItems.remove(it.key())) {
            // m_filteredItems still contained this ancestor until now so we can be sure that we aren't adding a duplicate ancestor to newVisibleItems.
            newVisibleItems.append(it.value());
        }
    }

    // This will insert the newly discovered unfiltered items into m_itemData
    insertItems(newVisibleItems);
}

void KFileItemModel::removeFilteredChildren(const KItemRangeList &itemRanges)
{
    if (m_filteredItems.isEmpty() || !m_requestRole[ExpandedParentsCountRole]) {
        // There are either no filtered items, or it is not possible to expand
        // folders -> there cannot be any filtered children.
        return;
    }

    QSet<ItemData *> parents;
    for (const KItemRange &range : itemRanges) {
        for (int index = range.index; index < range.index + range.count; ++index) {
            parents.insert(m_itemData.at(index));
        }
    }

    QHash<KFileItem, ItemData *>::iterator it = m_filteredItems.begin();
    while (it != m_filteredItems.end()) {
        if (parents.contains(it.value()->parent)) {
            delete it.value();
            it = m_filteredItems.erase(it);
        } else {
            ++it;
        }
    }
}

QList<KFileItemModel::RoleInfo> KFileItemModel::rolesInformation()
{
    static QList<RoleInfo> rolesInfo;
    if (rolesInfo.isEmpty()) {
        int count = 0;
        const RoleInfoMap *map = rolesInfoMap(count);
        for (int i = 0; i < count; ++i) {
            if (map[i].roleType != NoRole) {
                RoleInfo info;
                info.role = map[i].role;
                info.translation = map[i].roleTranslation.toString();
                if (!map[i].groupTranslation.isEmpty()) {
                    info.group = map[i].groupTranslation.toString();
                } else {
                    // For top level roles, groupTranslation is 0. We must make sure that
                    // info.group is an empty string then because the code that generates
                    // menus tries to put the actions into sub menus otherwise.
                    info.group = QString();
                }
                info.requiresBaloo = map[i].requiresBaloo;
                info.requiresIndexer = map[i].requiresIndexer;
                if (!map[i].tooltipTranslation.isEmpty()) {
                    info.tooltip = map[i].tooltipTranslation.toString();
                } else {
                    info.tooltip = QString();
                }
                rolesInfo.append(info);
            }
        }
    }

    return rolesInfo;
}

void KFileItemModel::onGroupedSortingChanged(bool current)
{
    Q_UNUSED(current)
    m_groups.clear();
}

void KFileItemModel::onSortRoleChanged(const QByteArray &current, const QByteArray &previous, bool resortItems)
{
    Q_UNUSED(previous)
    m_sortRole = typeForRole(current);

    if (!m_requestRole[m_sortRole]) {
        QSet<QByteArray> newRoles = m_roles;
        newRoles << current;
        setRoles(newRoles);
    }

    if (resortItems) {
        resortAllItems();
    }
}

void KFileItemModel::onSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    resortAllItems();
}

void KFileItemModel::loadSortingSettings()
{
    using Choice = GeneralSettings::EnumSortingChoice;
    switch (GeneralSettings::sortingChoice()) {
    case Choice::NaturalSorting:
        m_naturalSorting = true;
        m_collator.setCaseSensitivity(Qt::CaseInsensitive);
        break;
    case Choice::CaseSensitiveSorting:
        m_naturalSorting = false;
        m_collator.setCaseSensitivity(Qt::CaseSensitive);
        break;
    case Choice::CaseInsensitiveSorting:
        m_naturalSorting = false;
        m_collator.setCaseSensitivity(Qt::CaseInsensitive);
        break;
    default:
        Q_UNREACHABLE();
    }
    // Workaround for bug https://bugreports.qt.io/browse/QTBUG-69361
    // Force the clean state of QCollator in single thread to avoid thread safety problems in sort
    m_collator.compare(QString(), QString());
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
    qCDebug(DolphinDebug) << "===========================================================";
    qCDebug(DolphinDebug) << "Resorting" << itemCount << "items";
#endif

    // Remember the order of the current URLs so
    // that it can be determined which indexes have
    // been moved because of the resorting.
    QList<QUrl> oldUrls;
    oldUrls.reserve(itemCount);
    for (const ItemData *itemData : std::as_const(m_itemData)) {
        oldUrls.append(itemData->item.url());
    }

    m_items.clear();
    m_items.reserve(itemCount);

    // Resort the items
    sort(m_itemData.begin(), m_itemData.end());
    for (int i = 0; i < itemCount; ++i) {
        m_items.insert(m_itemData.at(i)->item.url(), i);
    }

    // Determine the first index that has been moved.
    int firstMovedIndex = 0;
    while (firstMovedIndex < itemCount && firstMovedIndex == m_items.value(oldUrls.at(firstMovedIndex))) {
        ++firstMovedIndex;
    }

    const bool itemsHaveMoved = firstMovedIndex < itemCount;
    if (itemsHaveMoved) {
        m_groups.clear();

        int lastMovedIndex = itemCount - 1;
        while (lastMovedIndex > firstMovedIndex && lastMovedIndex == m_items.value(oldUrls.at(lastMovedIndex))) {
            --lastMovedIndex;
        }

        Q_ASSERT(firstMovedIndex <= lastMovedIndex);

        // Create a list movedToIndexes, which has the property that
        // movedToIndexes[i] is the new index of the item with the old index
        // firstMovedIndex + i.
        const int movedItemsCount = lastMovedIndex - firstMovedIndex + 1;
        QList<int> movedToIndexes;
        movedToIndexes.reserve(movedItemsCount);
        for (int i = firstMovedIndex; i <= lastMovedIndex; ++i) {
            const int newIndex = m_items.value(oldUrls.at(i));
            movedToIndexes.append(newIndex);
        }

        Q_EMIT itemsMoved(KItemRange(firstMovedIndex, movedItemsCount), movedToIndexes);
    } else if (groupedSorting()) {
        // The groups might have changed even if the order of the items has not.
        const QList<QPair<int, QVariant>> oldGroups = m_groups;
        m_groups.clear();
        if (groups() != oldGroups) {
            Q_EMIT groupsChanged();
        }
    }

#ifdef KFILEITEMMODEL_DEBUG
    qCDebug(DolphinDebug) << "[TIME] Resorting of" << itemCount << "items:" << timer.elapsed();
#endif
}

void KFileItemModel::slotCompleted()
{
    m_maximumUpdateIntervalTimer->stop();
    dispatchPendingItemsToInsert();

    if (!m_urlsToExpand.isEmpty()) {
        // Try to find a URL that can be expanded.
        // Note that the parent folder must be expanded before any of its subfolders become visible.
        // Therefore, some URLs in m_restoredExpandedUrls might not be visible yet
        // -> we expand the first visible URL we find in m_restoredExpandedUrls.
        // Iterate over a const copy because items are deleted and inserted within the loop
        const auto urlsToExpand = m_urlsToExpand;
        for (const QUrl &url : urlsToExpand) {
            const int indexForUrl = index(url);
            if (indexForUrl >= 0) {
                m_urlsToExpand.remove(url);
                if (setExpanded(indexForUrl, true)) {
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

    Q_EMIT directoryLoadingCompleted();
}

void KFileItemModel::slotCanceled()
{
    m_maximumUpdateIntervalTimer->stop();
    dispatchPendingItemsToInsert();

    Q_EMIT directoryLoadingCanceled();
}

void KFileItemModel::slotItemsAdded(const QUrl &directoryUrl, const KFileItemList &items)
{
    Q_ASSERT(!items.isEmpty());

    const QUrl parentUrl = m_expandedDirs.value(directoryUrl, directoryUrl.adjusted(QUrl::StripTrailingSlash));

    if (m_requestRole[ExpandedParentsCountRole]) {
        // If the expanding of items is enabled, the call
        // dirLister->openUrl(url, KDirLister::Keep) in KFileItemModel::setExpanded()
        // might result in emitting the same items twice due to the Keep-parameter.
        // This case happens if an item gets expanded, collapsed and expanded again
        // before the items could be loaded for the first expansion.
        if (index(items.first().url()) >= 0) {
            // The items are already part of the model.
            return;
        }

        if (directoryUrl != directory()) {
            // To be able to compare whether the new items may be inserted as children
            // of a parent item the pending items must be added to the model first.
            dispatchPendingItemsToInsert();
        }

        // KDirLister keeps the children of items that got expanded once even if
        // they got collapsed again with KFileItemModel::setExpanded(false). So it must be
        // checked whether the parent for new items is still expanded.
        const int parentIndex = index(parentUrl);
        if (parentIndex >= 0 && !m_itemData[parentIndex]->values.value("isExpanded").toBool()) {
            // The parent is not expanded.
            return;
        }
    }

    const QList<ItemData *> itemDataList = createItemDataList(parentUrl, items);

    if (!m_filter.hasSetFilters()) {
        m_pendingItemsToInsert.append(itemDataList);
    } else {
        QSet<ItemData *> parentsToEnsureVisible;

        // The name or type filter is active. Hide filtered items
        // before inserting them into the model and remember
        // the filtered items in m_filteredItems.
        for (ItemData *itemData : itemDataList) {
            if (m_filter.matches(itemData->item)) {
                m_pendingItemsToInsert.append(itemData);
                if (itemData->parent) {
                    parentsToEnsureVisible.insert(itemData->parent);
                }
            } else {
                m_filteredItems.insert(itemData->item, itemData);
            }
        }

        // Entire parental chains must be shown
        for (ItemData *parent : parentsToEnsureVisible) {
            for (; parent && m_filteredItems.remove(parent->item); parent = parent->parent) {
                m_pendingItemsToInsert.append(parent);
            }
        }
    }

    if (!m_maximumUpdateIntervalTimer->isActive()) {
        // Assure that items get dispatched if no completed() or canceled() signal is
        // emitted during the maximum update interval.
        m_maximumUpdateIntervalTimer->start();
    }

    Q_EMIT fileItemsChanged({KFileItem(directoryUrl)});
}

int KFileItemModel::filterChildlessParents(KItemRangeList &removedItemRanges, const QSet<ItemData *> &parentsToEnsureVisible)
{
    int filteredParentsCount = 0;
    // The childless parents not yet removed will always be right above the start of a removed range.
    // We iterate backwards to ensure the deepest folders are processed before their parents
    for (int i = removedItemRanges.size() - 1; i >= 0; i--) {
        KItemRange itemRange = removedItemRanges.at(i);
        const ItemData *const firstInRange = m_itemData.at(itemRange.index);
        ItemData *itemAbove = itemRange.index - 1 >= 0 ? m_itemData.at(itemRange.index - 1) : nullptr;
        const ItemData *const itemBelow = itemRange.index + itemRange.count < m_itemData.count() ? m_itemData.at(itemRange.index + itemRange.count) : nullptr;

        if (itemAbove && firstInRange->parent == itemAbove && !m_filter.matches(itemAbove->item) && (!itemBelow || itemBelow->parent != itemAbove)
            && !parentsToEnsureVisible.contains(itemAbove)) {
            // The item above exists, is the parent, doesn't pass the filter, does not belong to parentsToEnsureVisible
            // and this deleted range covers all of its descendents, so none will be left.
            m_filteredItems.insert(itemAbove->item, itemAbove);
            // This range's starting index will be extended to include the parent above:
            --itemRange.index;
            ++itemRange.count;
            ++filteredParentsCount;
            KItemRange previousRange = i > 0 ? removedItemRanges.at(i - 1) : KItemRange();
            // We must check if this caused the range to touch the previous range, if that's the case they shall be merged
            if (i > 0 && previousRange.index + previousRange.count == itemRange.index) {
                previousRange.count += itemRange.count;
                removedItemRanges.replace(i - 1, previousRange);
                removedItemRanges.removeAt(i);
            } else {
                removedItemRanges.replace(i, itemRange);
                // We must revisit this range in the next iteration since its starting index changed
                ++i;
            }
        }
    }
    return filteredParentsCount;
}

void KFileItemModel::slotItemsDeleted(const KFileItemList &items)
{
    dispatchPendingItemsToInsert();

    QVector<int> indexesToRemove;
    indexesToRemove.reserve(items.count());
    KFileItemList dirsChanged;

    const auto currentDir = directory();

    for (const KFileItem &item : items) {
        if (item.url() == currentDir) {
            Q_EMIT currentDirectoryRemoved();
            return;
        }

        const int indexForItem = index(item);
        if (indexForItem >= 0) {
            indexesToRemove.append(indexForItem);
        } else {
            // Probably the item has been filtered.
            QHash<KFileItem, ItemData *>::iterator it = m_filteredItems.find(item);
            if (it != m_filteredItems.end()) {
                delete it.value();
                m_filteredItems.erase(it);
            }
        }

        QUrl parentUrl = item.url().adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash);
        if (dirsChanged.findByUrl(parentUrl).isNull()) {
            dirsChanged << KFileItem(parentUrl);
        }
    }

    std::sort(indexesToRemove.begin(), indexesToRemove.end());

    if (m_requestRole[ExpandedParentsCountRole] && !m_expandedDirs.isEmpty()) {
        // Assure that removing a parent item also results in removing all children
        QVector<int> indexesToRemoveWithChildren;
        indexesToRemoveWithChildren.reserve(m_itemData.count());

        const int itemCount = m_itemData.count();
        for (int index : std::as_const(indexesToRemove)) {
            indexesToRemoveWithChildren.append(index);

            const int parentLevel = expandedParentsCount(index);
            int childIndex = index + 1;
            while (childIndex < itemCount && expandedParentsCount(childIndex) > parentLevel) {
                indexesToRemoveWithChildren.append(childIndex);
                ++childIndex;
            }
        }

        indexesToRemove = indexesToRemoveWithChildren;
    }

    KItemRangeList itemRanges = KItemRangeList::fromSortedContainer(indexesToRemove);
    removeFilteredChildren(itemRanges);

    // This call will update itemRanges to include the childless parents that have been filtered.
    const int filteredParentsCount = filterChildlessParents(itemRanges);

    // If any childless parents were filtered, then itemRanges got updated and now contains items that were really deleted
    // mixed with expanded folders that are just being filtered out.
    // If that's the case, we pass 'DeleteItemDataIfUnfiltered' as a hint
    // so removeItems() will check m_filteredItems to differentiate which is which.
    removeItems(itemRanges, filteredParentsCount > 0 ? DeleteItemDataIfUnfiltered : DeleteItemData);

    Q_EMIT fileItemsChanged(dirsChanged);
}

void KFileItemModel::slotRefreshItems(const QList<QPair<KFileItem, KFileItem>> &items)
{
    Q_ASSERT(!items.isEmpty());
#ifdef KFILEITEMMODEL_DEBUG
    qCDebug(DolphinDebug) << "Refreshing" << items.count() << "items";
#endif

    // Get the indexes of all items that have been refreshed
    QList<int> indexes;
    indexes.reserve(items.count());

    QSet<QByteArray> changedRoles;
    KFileItemList changedFiles;

    // Contains the indexes of the currently visible items
    // that should get hidden and hence moved to m_filteredItems.
    QVector<int> newFilteredIndexes;

    // Contains currently hidden items that should
    // get visible and hence removed from m_filteredItems
    QList<ItemData *> newVisibleItems;

    QListIterator<QPair<KFileItem, KFileItem>> it(items);

    while (it.hasNext()) {
        const QPair<KFileItem, KFileItem> &itemPair = it.next();
        const KFileItem &oldItem = itemPair.first;
        const KFileItem &newItem = itemPair.second;
        const int indexForItem = index(oldItem);
        const bool newItemMatchesFilter = m_filter.matches(newItem);
        if (indexForItem >= 0) {
            m_itemData[indexForItem]->item = newItem;

            // Keep old values as long as possible if they could not retrieved synchronously yet.
            // The update of the values will be done asynchronously by KFileItemModelRolesUpdater.
            ItemData *const itemData = m_itemData.at(indexForItem);
            QHashIterator<QByteArray, QVariant> it(retrieveData(newItem, itemData->parent));
            while (it.hasNext()) {
                it.next();
                const QByteArray &role = it.key();
                if (itemData->values.value(role) != it.value()) {
                    itemData->values.insert(role, it.value());
                    changedRoles.insert(role);
                }
            }

            m_items.remove(oldItem.url());
            // We must maintain m_items consistent with m_itemData for now, this very loop is using it.
            // We leave it to be cleared by removeItems() later, when m_itemData actually gets updated.
            m_items.insert(newItem.url(), indexForItem);
            if (newItemMatchesFilter
                || (itemData->values.value("isExpanded").toBool()
                    && (indexForItem + 1 < m_itemData.count() && m_itemData.at(indexForItem + 1)->parent == itemData))) {
                // We are lenient with expanded folders that originally had visible children.
                // If they become childless now they will be caught by filterChildlessParents()
                changedFiles.append(newItem);
                indexes.append(indexForItem);
            } else {
                newFilteredIndexes.append(indexForItem);
                m_filteredItems.insert(newItem, itemData);
            }
        } else {
            // Check if 'oldItem' is one of the filtered items.
            QHash<KFileItem, ItemData *>::iterator it = m_filteredItems.find(oldItem);
            if (it != m_filteredItems.end()) {
                ItemData *const itemData = it.value();
                itemData->item = newItem;

                // The data stored in 'values' might have changed. Therefore, we clear
                // 'values' and re-populate it the next time it is requested via data(int).
                // Before clearing, we must remember if it was expanded and the expanded parents count,
                // otherwise these states would be lost. The data() method will deal with this special case.
                const bool isExpanded = itemData->values.value("isExpanded").toBool();
                bool hasExpandedParentsCount = false;
                const int expandedParentsCount = itemData->values.value("expandedParentsCount").toInt(&hasExpandedParentsCount);
                itemData->values.clear();
                if (isExpanded) {
                    itemData->values.insert("isExpanded", true);
                    if (hasExpandedParentsCount) {
                        itemData->values.insert("expandedParentsCount", expandedParentsCount);
                    }
                }

                m_filteredItems.erase(it);
                if (newItemMatchesFilter) {
                    newVisibleItems.append(itemData);
                } else {
                    m_filteredItems.insert(newItem, itemData);
                }
            }
        }
    }

    std::sort(newFilteredIndexes.begin(), newFilteredIndexes.end());

    // We must keep track of parents of new visible items since they must be shown no matter what
    // They will be considered "immune" to filterChildlessParents()
    QSet<ItemData *> parentsToEnsureVisible;

    for (ItemData *item : newVisibleItems) {
        for (ItemData *parent = item->parent; parent && !parentsToEnsureVisible.contains(parent); parent = parent->parent) {
            parentsToEnsureVisible.insert(parent);
        }
    }
    for (ItemData *parent : parentsToEnsureVisible) {
        // We make sure they are all unfiltered.
        if (m_filteredItems.remove(parent->item)) {
            // If it is being unfiltered now, we mark it to be inserted by appending it to newVisibleItems
            newVisibleItems.append(parent);
            // It could be in newFilteredIndexes, we must remove it if it's there:
            const int parentIndex = index(parent->item);
            if (parentIndex >= 0) {
                QVector<int>::iterator it = std::lower_bound(newFilteredIndexes.begin(), newFilteredIndexes.end(), parentIndex);
                if (it != newFilteredIndexes.end() && *it == parentIndex) {
                    newFilteredIndexes.erase(it);
                }
            }
        }
    }

    KItemRangeList removedRanges = KItemRangeList::fromSortedContainer(newFilteredIndexes);

    // This call will update itemRanges to include the childless parents that have been filtered.
    filterChildlessParents(removedRanges, parentsToEnsureVisible);

    removeItems(removedRanges, KeepItemData);

    // Show previously hidden items that should get visible
    insertItems(newVisibleItems);

    // Final step: we will emit 'itemsChanged' and 'fileItemsChanged' signals and trigger the asynchronous re-sorting logic.

    // If the changed items have been created recently, they might not be in m_items yet.
    // In that case, the list 'indexes' might be empty.
    if (indexes.isEmpty()) {
        return;
    }

    if (newVisibleItems.count() > 0 || removedRanges.count() > 0) {
        // The original indexes have changed and are now worthless since items were removed and/or inserted.
        indexes.clear();
        // m_items is not yet rebuilt at this point, so we use our own means to resolve the new indexes.
        const QSet<const KFileItem> changedFilesSet(changedFiles.cbegin(), changedFiles.cend());
        for (int i = 0; i < m_itemData.count(); i++) {
            if (changedFilesSet.contains(m_itemData.at(i)->item)) {
                indexes.append(i);
            }
        }
    } else {
        std::sort(indexes.begin(), indexes.end());
    }

    // Extract the item-ranges out of the changed indexes
    const KItemRangeList itemRangeList = KItemRangeList::fromSortedContainer(indexes);
    emitItemsChangedAndTriggerResorting(itemRangeList, changedRoles);

    Q_EMIT fileItemsChanged(changedFiles);
}

void KFileItemModel::slotClear()
{
#ifdef KFILEITEMMODEL_DEBUG
    qCDebug(DolphinDebug) << "Clearing all items";
#endif

    qDeleteAll(m_filteredItems);
    m_filteredItems.clear();
    m_groups.clear();

    m_maximumUpdateIntervalTimer->stop();
    m_resortAllItemsTimer->stop();

    qDeleteAll(m_pendingItemsToInsert);
    m_pendingItemsToInsert.clear();

    const int removedCount = m_itemData.count();
    if (removedCount > 0) {
        qDeleteAll(m_itemData);
        m_itemData.clear();
        m_items.clear();
        Q_EMIT itemsRemoved(KItemRangeList() << KItemRange(0, removedCount));
    }

    m_expandedDirs.clear();
}

void KFileItemModel::slotSortingChoiceChanged()
{
    loadSortingSettings();
    resortAllItems();
}

void KFileItemModel::dispatchPendingItemsToInsert()
{
    if (!m_pendingItemsToInsert.isEmpty()) {
        insertItems(m_pendingItemsToInsert);
        m_pendingItemsToInsert.clear();
    }
}

void KFileItemModel::insertItems(QList<ItemData *> &newItems)
{
    if (newItems.isEmpty()) {
        return;
    }

#ifdef KFILEITEMMODEL_DEBUG
    QElapsedTimer timer;
    timer.start();
    qCDebug(DolphinDebug) << "===========================================================";
    qCDebug(DolphinDebug) << "Inserting" << newItems.count() << "items";
#endif

    m_groups.clear();
    prepareItemsForSorting(newItems);

    // Natural sorting of items can be very slow. However, it becomes much faster
    // if the input sequence is already mostly sorted. Therefore, we first sort
    // 'newItems' according to the QStrings using QString::operator<(), which is quite fast.
    if (m_naturalSorting) {
        if (m_sortRole == NameRole) {
            parallelMergeSort(newItems.begin(), newItems.end(), nameLessThan, QThread::idealThreadCount());
        } else if (isRoleValueNatural(m_sortRole)) {
            auto lambdaLessThan = [&](const KFileItemModel::ItemData *a, const KFileItemModel::ItemData *b) {
                const QByteArray role = roleForType(m_sortRole);
                return a->values.value(role).toString() < b->values.value(role).toString();
            };
            parallelMergeSort(newItems.begin(), newItems.end(), lambdaLessThan, QThread::idealThreadCount());
        }
    }

    sort(newItems.begin(), newItems.end());

#ifdef KFILEITEMMODEL_DEBUG
    qCDebug(DolphinDebug) << "[TIME] Sorting:" << timer.elapsed();
#endif

    KItemRangeList itemRanges;
    const int existingItemCount = m_itemData.count();
    const int newItemCount = newItems.count();
    const int totalItemCount = existingItemCount + newItemCount;

    if (existingItemCount == 0) {
        // Optimization for the common special case that there are no
        // items in the model yet. Happens, e.g., when entering a folder.
        m_itemData = newItems;
        itemRanges << KItemRange(0, newItemCount);
    } else {
        m_itemData.reserve(totalItemCount);
        for (int i = existingItemCount; i < totalItemCount; ++i) {
            m_itemData.append(nullptr);
        }

        // We build the new list m_itemData in reverse order to minimize
        // the number of moves and guarantee O(N) complexity.
        int targetIndex = totalItemCount - 1;
        int sourceIndexExistingItems = existingItemCount - 1;
        int sourceIndexNewItems = newItemCount - 1;

        int rangeCount = 0;

        while (sourceIndexNewItems >= 0) {
            ItemData *newItem = newItems.at(sourceIndexNewItems);
            if (sourceIndexExistingItems >= 0 && lessThan(newItem, m_itemData.at(sourceIndexExistingItems), m_collator)) {
                // Move an existing item to its new position. If any new items
                // are behind it, push the item range to itemRanges.
                if (rangeCount > 0) {
                    itemRanges << KItemRange(sourceIndexExistingItems + 1, rangeCount);
                    rangeCount = 0;
                }

                m_itemData[targetIndex] = m_itemData.at(sourceIndexExistingItems);
                --sourceIndexExistingItems;
            } else {
                // Insert a new item into the list.
                ++rangeCount;
                m_itemData[targetIndex] = newItem;
                --sourceIndexNewItems;
            }
            --targetIndex;
        }

        // Push the final item range to itemRanges.
        if (rangeCount > 0) {
            itemRanges << KItemRange(sourceIndexExistingItems + 1, rangeCount);
        }

        // Note that itemRanges is still sorted in reverse order.
        std::reverse(itemRanges.begin(), itemRanges.end());
    }

    // The indexes in m_items are not correct anymore. Therefore, we clear m_items.
    // It will be re-populated with the updated indices if index(const QUrl&) is called.
    m_items.clear();

    Q_EMIT itemsInserted(itemRanges);

#ifdef KFILEITEMMODEL_DEBUG
    qCDebug(DolphinDebug) << "[TIME] Inserting of" << newItems.count() << "items:" << timer.elapsed();
#endif
}

void KFileItemModel::removeItems(const KItemRangeList &itemRanges, RemoveItemsBehavior behavior)
{
    if (itemRanges.isEmpty()) {
        return;
    }

    m_groups.clear();

    // Step 1: Remove the items from m_itemData, and free the ItemData.
    int removedItemsCount = 0;
    for (const KItemRange &range : itemRanges) {
        removedItemsCount += range.count;

        for (int index = range.index; index < range.index + range.count; ++index) {
            if (behavior == DeleteItemData || (behavior == DeleteItemDataIfUnfiltered && !m_filteredItems.contains(m_itemData.at(index)->item))) {
                delete m_itemData.at(index);
            }

            m_itemData[index] = nullptr;
        }
    }

    // Step 2: Remove the ItemData pointers from the list m_itemData.
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

    m_itemData.erase(m_itemData.end() - removedItemsCount, m_itemData.end());

    // The indexes in m_items are not correct anymore. Therefore, we clear m_items.
    // It will be re-populated with the updated indices if index(const QUrl&) is called.
    m_items.clear();

    Q_EMIT itemsRemoved(itemRanges);
}

QList<KFileItemModel::ItemData *> KFileItemModel::createItemDataList(const QUrl &parentUrl, const KFileItemList &items) const
{
    if (m_sortRole == TypeRole) {
        // Try to resolve the MIME-types synchronously to prevent a reordering of
        // the items when sorting by type (per default MIME-types are resolved
        // asynchronously by KFileItemModelRolesUpdater).
        determineMimeTypes(items, 200);
    }

    // We search for the parent in m_itemData and then in m_filteredItems if necessary
    const int parentIndex = index(parentUrl);
    ItemData *parentItem = parentIndex < 0 ? m_filteredItems.value(KFileItem(parentUrl), nullptr) : m_itemData.at(parentIndex);

    QList<ItemData *> itemDataList;
    itemDataList.reserve(items.count());

    for (const KFileItem &item : items) {
        ItemData *itemData = new ItemData();
        itemData->item = item;
        itemData->parent = parentItem;
        itemDataList.append(itemData);
    }

    return itemDataList;
}

void KFileItemModel::prepareItemsForSorting(QList<ItemData *> &itemDataList)
{
    switch (m_sortRole) {
    case ExtensionRole:
    case PermissionsRole:
    case OwnerRole:
    case GroupRole:
    case DestinationRole:
    case PathRole:
    case DeletionTimeRole:
        // These roles can be determined with retrieveData, and they have to be stored
        // in the QHash "values" for the sorting.
        for (ItemData *itemData : std::as_const(itemDataList)) {
            if (itemData->values.isEmpty()) {
                itemData->values = retrieveData(itemData->item, itemData->parent);
            }
        }
        break;

    case TypeRole:
        // At least store the data including the file type for items with known MIME type.
        for (ItemData *itemData : std::as_const(itemDataList)) {
            if (itemData->values.isEmpty()) {
                const KFileItem item = itemData->item;
                if (item.isDir() || item.isMimeTypeKnown()) {
                    itemData->values = retrieveData(itemData->item, itemData->parent);
                }
            }
        }
        break;

    default:
        // The other roles are either resolved by KFileItemModelRolesUpdater
        // (this includes the SizeRole for directories), or they do not need
        // to be stored in the QHash "values" for sorting because the data can
        // be retrieved directly from the KFileItem (NameRole, SizeRole for files,
        // DateRole).
        break;
    }
}

int KFileItemModel::expandedParentsCount(const ItemData *data)
{
    // The hash 'values' is only guaranteed to contain the key "expandedParentsCount"
    // if the corresponding item is expanded, and it is not a top-level item.
    const ItemData *parent = data->parent;
    if (parent) {
        if (parent->parent) {
            Q_ASSERT(parent->values.contains("expandedParentsCount"));
            return parent->values.value("expandedParentsCount").toInt() + 1;
        } else {
            return 1;
        }
    } else {
        return 0;
    }
}

void KFileItemModel::removeExpandedItems()
{
    QVector<int> indexesToRemove;

    const int maxIndex = m_itemData.count() - 1;
    for (int i = 0; i <= maxIndex; ++i) {
        const ItemData *itemData = m_itemData.at(i);
        if (itemData->parent) {
            indexesToRemove.append(i);
        }
    }

    removeItems(KItemRangeList::fromSortedContainer(indexesToRemove), DeleteItemData);
    m_expandedDirs.clear();

    // Also remove all filtered items which have a parent.
    QHash<KFileItem, ItemData *>::iterator it = m_filteredItems.begin();
    const QHash<KFileItem, ItemData *>::iterator end = m_filteredItems.end();

    while (it != end) {
        if (it.value()->parent) {
            delete it.value();
            it = m_filteredItems.erase(it);
        } else {
            ++it;
        }
    }
}

void KFileItemModel::emitItemsChangedAndTriggerResorting(const KItemRangeList &itemRanges, const QSet<QByteArray> &changedRoles)
{
    Q_EMIT itemsChanged(itemRanges, changedRoles);

    // Trigger a resorting if necessary. Note that this can happen even if the sort
    // role has not changed at all because the file name can be used as a fallback.
    if (changedRoles.contains(sortRole()) || changedRoles.contains(roleForType(NameRole))
        || (changedRoles.contains("count") && sortRole() == "size")) { // "count" is used in the "size" sort role, so this might require a resorting.
        for (const KItemRange &range : itemRanges) {
            bool needsResorting = false;

            const int first = range.index;
            const int last = range.index + range.count - 1;

            // Resorting the model is necessary if
            // (a)  The first item in the range is "lessThan" its predecessor,
            // (b)  the successor of the last item is "lessThan" the last item, or
            // (c)  the internal order of the items in the range is incorrect.
            if (first > 0 && lessThan(m_itemData.at(first), m_itemData.at(first - 1), m_collator)) {
                needsResorting = true;
            } else if (last < count() - 1 && lessThan(m_itemData.at(last + 1), m_itemData.at(last), m_collator)) {
                needsResorting = true;
            } else {
                for (int index = first; index < last; ++index) {
                    if (lessThan(m_itemData.at(index + 1), m_itemData.at(index), m_collator)) {
                        needsResorting = true;
                        break;
                    }
                }
            }

            if (needsResorting) {
                scheduleResortAllItems();
                return;
            }
        }
    }

    if (groupedSorting() && changedRoles.contains(sortRole())) {
        // The position is still correct, but the groups might have changed
        // if the changed item is either the first or the last item in a
        // group.
        // In principle, we could try to find out if the item really is the
        // first or last one in its group and then update the groups
        // (possibly with a delayed timer to make sure that we don't
        // re-calculate the groups very often if items are updated one by
        // one), but starting m_resortAllItemsTimer is easier.
        m_resortAllItemsTimer->start();
    }
}

void KFileItemModel::resetRoles()
{
    for (int i = 0; i < RolesCount; ++i) {
        m_requestRole[i] = false;
    }
}

KFileItemModel::RoleType KFileItemModel::typeForRole(const QByteArray &role) const
{
    static QHash<QByteArray, RoleType> roles;
    if (roles.isEmpty()) {
        // Insert user visible roles that can be accessed with
        // KFileItemModel::roleInformation()
        int count = 0;
        const RoleInfoMap *map = rolesInfoMap(count);
        for (int i = 0; i < count; ++i) {
            roles.insert(map[i].role, map[i].roleType);
        }

        // Insert internal roles (take care to synchronize the implementation
        // with KFileItemModel::roleForType() in case if a change is done).
        roles.insert("isDir", IsDirRole);
        roles.insert("isLink", IsLinkRole);
        roles.insert("isHidden", IsHiddenRole);
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
        const RoleInfoMap *map = rolesInfoMap(count);
        for (int i = 0; i < count; ++i) {
            roles.insert(map[i].roleType, map[i].role);
        }

        // Insert internal roles (take care to synchronize the implementation
        // with KFileItemModel::typeForRole() in case if a change is done).
        roles.insert(IsDirRole, "isDir");
        roles.insert(IsLinkRole, "isLink");
        roles.insert(IsHiddenRole, "isHidden");
        roles.insert(IsExpandedRole, "isExpanded");
        roles.insert(IsExpandableRole, "isExpandable");
        roles.insert(ExpandedParentsCountRole, "expandedParentsCount");

        Q_ASSERT(roles.count() == RolesCount);
    };

    return roles.value(roleType);
}

QHash<QByteArray, QVariant> KFileItemModel::retrieveData(const KFileItem &item, const ItemData *parent) const
{
    // It is important to insert only roles that are fast to retrieve. E.g.
    // KFileItem::iconName() can be very expensive if the MIME-type is unknown
    // and hence will be retrieved asynchronously by KFileItemModelRolesUpdater.
    QHash<QByteArray, QVariant> data;
    data.insert(sharedValue("url"), item.url());

    const bool isDir = item.isDir();
    if (m_requestRole[IsDirRole] && isDir) {
        data.insert(sharedValue("isDir"), true);
    }

    if (m_requestRole[IsLinkRole] && item.isLink()) {
        data.insert(sharedValue("isLink"), true);
    }

    if (m_requestRole[IsHiddenRole]) {
        data.insert(sharedValue("isHidden"), item.isHidden() || item.mimetype() == QStringLiteral("application/x-trash"));
    }

    if (m_requestRole[NameRole]) {
        data.insert(sharedValue("text"), item.text());
    }

    if (m_requestRole[ExtensionRole] && !isDir) {
        // TODO KF6 use KFileItem::suffix 464722
        data.insert(sharedValue("extension"), QFileInfo(item.name()).suffix());
    }

    if (m_requestRole[SizeRole] && !isDir) {
        data.insert(sharedValue("size"), item.size());
    }

    if (m_requestRole[ModificationTimeRole]) {
        // Don't use KFileItem::timeString() or KFileItem::time() as this is too expensive when
        // having several thousands of items. Instead read the raw number from UDSEntry directly
        // and the formatting of the date-time will be done on-demand by the view when the date will be shown.
        const long long dateTime = item.entry().numberValue(KIO::UDSEntry::UDS_MODIFICATION_TIME, -1);
        data.insert(sharedValue("modificationtime"), dateTime);
    }

    if (m_requestRole[CreationTimeRole]) {
        // Don't use KFileItem::timeString() or KFileItem::time() as this is too expensive when
        // having several thousands of items. Instead read the raw number from UDSEntry directly
        // and the formatting of the date-time will be done on-demand by the view when the date will be shown.
        const long long dateTime = item.entry().numberValue(KIO::UDSEntry::UDS_CREATION_TIME, -1);
        data.insert(sharedValue("creationtime"), dateTime);
    }

    if (m_requestRole[AccessTimeRole]) {
        // Don't use KFileItem::timeString() or KFileItem::time() as this is too expensive when
        // having several thousands of items. Instead read the raw number from UDSEntry directly
        // and the formatting of the date-time will be done on-demand by the view when the date will be shown.
        const long long dateTime = item.entry().numberValue(KIO::UDSEntry::UDS_ACCESS_TIME, -1);
        data.insert(sharedValue("accesstime"), dateTime);
    }

    if (m_requestRole[PermissionsRole]) {
        data.insert(sharedValue("permissions"), QVariantList() << item.permissionsString() << item.permissions());
    }

    if (m_requestRole[OwnerRole]) {
        data.insert(sharedValue("owner"), item.user());
    }

    if (m_requestRole[GroupRole]) {
        data.insert(sharedValue("group"), item.group());
    }

    if (m_requestRole[DestinationRole]) {
        QString destination = item.linkDest();
        if (destination.isEmpty()) {
            destination = QLatin1Char('-');
        }
        data.insert(sharedValue("destination"), destination);
    }

    if (m_requestRole[PathRole]) {
        QString path;
        if (item.url().scheme() == QLatin1String("trash")) {
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
        data.insert(sharedValue("path"), path);
    }

    if (m_requestRole[DeletionTimeRole]) {
        QDateTime deletionTime;
        if (item.url().scheme() == QLatin1String("trash")) {
            deletionTime = QDateTime::fromString(item.entry().stringValue(KIO::UDSEntry::UDS_EXTRA + 1), Qt::ISODate);
        }
        data.insert(sharedValue("deletiontime"), deletionTime);
    }

    if (m_requestRole[IsExpandableRole] && isDir) {
        data.insert(sharedValue("isExpandable"), true);
    }

    if (m_requestRole[ExpandedParentsCountRole]) {
        if (parent) {
            const int level = expandedParentsCount(parent) + 1;
            data.insert(sharedValue("expandedParentsCount"), level);
        }
    }

    if (item.isMimeTypeKnown()) {
        QString iconName = item.iconName();
        if (!QIcon::hasThemeIcon(iconName)) {
            QMimeType mimeType = QMimeDatabase().mimeTypeForName(item.mimetype());
            iconName = mimeType.genericIconName();
        }

        data.insert(sharedValue("iconName"), iconName);

        if (m_requestRole[TypeRole]) {
            data.insert(sharedValue("type"), item.mimeComment());
        }
    } else if (m_requestRole[TypeRole] && isDir) {
        static const QString folderMimeType = item.mimeComment();
        data.insert(sharedValue("type"), folderMimeType);
    }

    return data;
}

bool KFileItemModel::lessThan(const ItemData *a, const ItemData *b, const QCollator &collator) const
{
    int result = 0;

    if (a->parent != b->parent) {
        const int expansionLevelA = expandedParentsCount(a);
        const int expansionLevelB = expandedParentsCount(b);

        // If b has a higher expansion level than a, check if a is a parent
        // of b, and make sure that both expansion levels are equal otherwise.
        for (int i = expansionLevelB; i > expansionLevelA; --i) {
            if (b->parent == a) {
                return true;
            }
            b = b->parent;
        }

        // If a has a higher expansion level than a, check if b is a parent
        // of a, and make sure that both expansion levels are equal otherwise.
        for (int i = expansionLevelA; i > expansionLevelB; --i) {
            if (a->parent == b) {
                return false;
            }
            a = a->parent;
        }

        Q_ASSERT(expandedParentsCount(a) == expandedParentsCount(b));

        // Compare the last parents of a and b which are different.
        while (a->parent != b->parent) {
            a = a->parent;
            b = b->parent;
        }
    }

    // Show hidden files and folders last
    if (m_sortHiddenLast) {
        const bool isHiddenA = a->item.isHidden();
        const bool isHiddenB = b->item.isHidden();
        if (isHiddenA && !isHiddenB) {
            return false;
        } else if (!isHiddenA && isHiddenB) {
            return true;
        }
    }

    if (m_sortDirsFirst
        || (ContentDisplaySettings::directorySizeMode() == ContentDisplaySettings::EnumDirectorySizeMode::ContentCount && m_sortRole == SizeRole)) {
        const bool isDirA = a->item.isDir();
        const bool isDirB = b->item.isDir();
        if (isDirA && !isDirB) {
            return true;
        } else if (!isDirA && isDirB) {
            return false;
        }
    }

    result = sortRoleCompare(a, b, collator);

    return (sortOrder() == Qt::AscendingOrder) ? result < 0 : result > 0;
}

void KFileItemModel::sort(const QList<KFileItemModel::ItemData *>::iterator &begin, const QList<KFileItemModel::ItemData *>::iterator &end) const
{
    auto lambdaLessThan = [&](const KFileItemModel::ItemData *a, const KFileItemModel::ItemData *b) {
        return lessThan(a, b, m_collator);
    };

    if (m_sortRole == NameRole || isRoleValueNatural(m_sortRole)) {
        // Sorting by string can be expensive, in particular if natural sorting is
        // enabled. Use all CPU cores to speed up the sorting process.
        static const int numberOfThreads = QThread::idealThreadCount();
        parallelMergeSort(begin, end, lambdaLessThan, numberOfThreads);
    } else {
        // Sorting by other roles is quite fast. Use only one thread to prevent
        // problems caused by non-reentrant comparison functions, see
        // https://bugs.kde.org/show_bug.cgi?id=312679
        mergeSort(begin, end, lambdaLessThan);
    }
}

int KFileItemModel::sortRoleCompare(const ItemData *a, const ItemData *b, const QCollator &collator) const
{
    // This function must never return 0, because that would break stable
    // sorting, which leads to all kinds of bugs.
    // See: https://bugs.kde.org/show_bug.cgi?id=433247
    // If two items have equal sort values, let the fallbacks at the bottom of
    // the function handle it.
    const KFileItem &itemA = a->item;
    const KFileItem &itemB = b->item;

    int result = 0;

    switch (m_sortRole) {
    case NameRole:
        // The name role is handled as default fallback after the switch
        break;

    case SizeRole: {
        if (ContentDisplaySettings::directorySizeMode() == ContentDisplaySettings::EnumDirectorySizeMode::ContentCount && itemA.isDir()) {
            // folders first then
            // items A and B are folders thanks to lessThan checks
            auto valueA = a->values.value("count");
            auto valueB = b->values.value("count");
            if (valueA.isNull()) {
                if (!valueB.isNull()) {
                    return -1;
                }
            } else if (valueB.isNull()) {
                return +1;
            } else {
                if (valueA.toLongLong() < valueB.toLongLong()) {
                    return -1;
                } else if (valueA.toLongLong() > valueB.toLongLong()) {
                    return +1;
                }
            }
            break;
        }

        KIO::filesize_t sizeA = 0;
        if (itemA.isDir()) {
            sizeA = a->values.value("size").toULongLong();
        } else {
            sizeA = itemA.size();
        }
        KIO::filesize_t sizeB = 0;
        if (itemB.isDir()) {
            sizeB = b->values.value("size").toULongLong();
        } else {
            sizeB = itemB.size();
        }
        if (sizeA < sizeB) {
            return -1;
        } else if (sizeA > sizeB) {
            return +1;
        }
        break;
    }

    case ModificationTimeRole: {
        const long long dateTimeA = itemA.entry().numberValue(KIO::UDSEntry::UDS_MODIFICATION_TIME, -1);
        const long long dateTimeB = itemB.entry().numberValue(KIO::UDSEntry::UDS_MODIFICATION_TIME, -1);
        if (dateTimeA < dateTimeB) {
            return -1;
        } else if (dateTimeA > dateTimeB) {
            return +1;
        }
        break;
    }

    case AccessTimeRole: {
        const long long dateTimeA = itemA.entry().numberValue(KIO::UDSEntry::UDS_ACCESS_TIME, -1);
        const long long dateTimeB = itemB.entry().numberValue(KIO::UDSEntry::UDS_ACCESS_TIME, -1);
        if (dateTimeA < dateTimeB) {
            return -1;
        } else if (dateTimeA > dateTimeB) {
            return +1;
        }
        break;
    }

    case CreationTimeRole: {
        const long long dateTimeA = itemA.entry().numberValue(KIO::UDSEntry::UDS_CREATION_TIME, -1);
        const long long dateTimeB = itemB.entry().numberValue(KIO::UDSEntry::UDS_CREATION_TIME, -1);
        if (dateTimeA < dateTimeB) {
            return -1;
        } else if (dateTimeA > dateTimeB) {
            return +1;
        }
        break;
    }

    case DeletionTimeRole: {
        const QDateTime dateTimeA = a->values.value("deletiontime").toDateTime();
        const QDateTime dateTimeB = b->values.value("deletiontime").toDateTime();
        if (dateTimeA < dateTimeB) {
            return -1;
        } else if (dateTimeA > dateTimeB) {
            return +1;
        }
        break;
    }

    case RatingRole:
    case WidthRole:
    case HeightRole:
    case PublisherRole:
    case PageCountRole:
    case WordCountRole:
    case LineCountRole:
    case TrackRole:
    case ReleaseYearRole: {
        result = a->values.value(roleForType(m_sortRole)).toInt() - b->values.value(roleForType(m_sortRole)).toInt();
        break;
    }

    case DimensionsRole: {
        const QByteArray role = roleForType(m_sortRole);
        const QSize dimensionsA = a->values.value(role).toSize();
        const QSize dimensionsB = b->values.value(role).toSize();

        if (dimensionsA.width() == dimensionsB.width()) {
            result = dimensionsA.height() - dimensionsB.height();
        } else {
            result = dimensionsA.width() - dimensionsB.width();
        }
        break;
    }

    default: {
        const QByteArray role = roleForType(m_sortRole);
        const QString roleValueA = a->values.value(role).toString();
        const QString roleValueB = b->values.value(role).toString();
        if (!roleValueA.isEmpty() && roleValueB.isEmpty()) {
            return -1;
        } else if (roleValueA.isEmpty() && !roleValueB.isEmpty()) {
            return +1;
        } else if (isRoleValueNatural(m_sortRole)) {
            result = stringCompare(roleValueA, roleValueB, collator);
        } else {
            result = QString::compare(roleValueA, roleValueB);
        }
        break;
    }
    }

    if (result != 0) {
        // The current sort role was sufficient to define an order
        return result;
    }

    // Fallback #1: Compare the text of the items
    result = stringCompare(itemA.text(), itemB.text(), collator);
    if (result != 0) {
        return result;
    }

    // Fallback #2: KFileItem::text() may not be unique in case UDS_DISPLAY_NAME is used
    result = stringCompare(itemA.name(), itemB.name(), collator);
    if (result != 0) {
        return result;
    }

    // Fallback #3: It must be assured that the sort order is always unique even if two values have been
    // equal. In this case a comparison of the URL is done which is unique in all cases
    // within KDirLister.
    return QString::compare(itemA.url().url(), itemB.url().url(), Qt::CaseSensitive);
}

int KFileItemModel::stringCompare(const QString &a, const QString &b, const QCollator &collator) const
{
    QMutexLocker collatorLock(s_collatorMutex());

    if (m_naturalSorting) {
        // Split extension, taking into account it can be empty
        constexpr QString::SectionFlags flags = QString::SectionSkipEmpty | QString::SectionIncludeLeadingSep;

        // Sort by baseName first
        const QString aBaseName = a.section('.', 0, 0, flags);
        const QString bBaseName = b.section('.', 0, 0, flags);

        const int res = collator.compare(aBaseName, bBaseName);
        if (res != 0 || (aBaseName.length() == a.length() && bBaseName.length() == b.length())) {
            return res;
        }

        // sliced() has undefined behavior when pos < 0 or pos > size().
        Q_ASSERT(aBaseName.length() <= a.length() && aBaseName.length() >= 0);
        Q_ASSERT(bBaseName.length() <= b.length() && bBaseName.length() >= 0);

        // baseNames were equal, sort by extension
        return collator.compare(a.sliced(aBaseName.length()), b.sliced(bBaseName.length()));
    }

    const int result = QString::compare(a, b, collator.caseSensitivity());
    if (result != 0 || collator.caseSensitivity() == Qt::CaseSensitive) {
        // Only return the result, if the strings are not equal. If they are equal by a case insensitive
        // comparison, still a deterministic sort order is required. A case sensitive
        // comparison is done as fallback.
        return result;
    }

    return QString::compare(a, b, Qt::CaseSensitive);
}

QList<QPair<int, QVariant>> KFileItemModel::nameRoleGroups() const
{
    Q_ASSERT(!m_itemData.isEmpty());

    const int maxIndex = count() - 1;
    QList<QPair<int, QVariant>> groups;

    QString groupValue;
    QChar firstChar;
    for (int i = 0; i <= maxIndex; ++i) {
        if (isChildItem(i)) {
            continue;
        }

        const QString name = m_itemData.at(i)->item.text();

        // Use the first character of the name as group indication
        QChar newFirstChar = name.at(0).toUpper();
        if (newFirstChar == QLatin1Char('~') && name.length() > 1) {
            newFirstChar = name.at(1).toUpper();
        }

        if (firstChar != newFirstChar) {
            QString newGroupValue;
            if (newFirstChar.isLetter()) {
                if (m_collator.compare(newFirstChar, QChar(QLatin1Char('A'))) >= 0 && m_collator.compare(newFirstChar, QChar(QLatin1Char('Z'))) <= 0) {
                    // WARNING! Symbols based on latin 'Z' like 'Z' with acute are treated wrong as non Latin and put in a new group.

                    // Try to find a matching group in the range 'A' to 'Z'.
                    static std::vector<QChar> lettersAtoZ;
                    lettersAtoZ.reserve('Z' - 'A' + 1);
                    if (lettersAtoZ.empty()) {
                        for (char c = 'A'; c <= 'Z'; ++c) {
                            lettersAtoZ.push_back(QLatin1Char(c));
                        }
                    }

                    auto localeAwareLessThan = [this](QChar c1, QChar c2) -> bool {
                        return m_collator.compare(c1, c2) < 0;
                    };

                    std::vector<QChar>::iterator it = std::lower_bound(lettersAtoZ.begin(), lettersAtoZ.end(), newFirstChar, localeAwareLessThan);
                    if (it != lettersAtoZ.end()) {
                        if (localeAwareLessThan(newFirstChar, *it)) {
                            // newFirstChar belongs to the group preceding *it.
                            // Example: for an umlaut 'A' in the German locale, *it would be 'B' now.
                            --it;
                        }
                        newGroupValue = *it;
                    }

                } else {
                    // Symbols from non Latin-based scripts
                    newGroupValue = newFirstChar;
                }
            } else if (newFirstChar >= QLatin1Char('0') && newFirstChar <= QLatin1Char('9')) {
                // Apply group '0 - 9' for any name that starts with a digit
                newGroupValue = i18nc("@title:group Groups that start with a digit", "0 - 9");
            } else {
                newGroupValue = i18nc("@title:group", "Others");
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

QList<QPair<int, QVariant>> KFileItemModel::sizeRoleGroups() const
{
    Q_ASSERT(!m_itemData.isEmpty());

    const int maxIndex = count() - 1;
    QList<QPair<int, QVariant>> groups;

    QString groupValue;
    for (int i = 0; i <= maxIndex; ++i) {
        if (isChildItem(i)) {
            continue;
        }

        const KFileItem &item = m_itemData.at(i)->item;
        KIO::filesize_t fileSize = !item.isNull() ? item.size() : ~0U;
        QString newGroupValue;
        if (!item.isNull() && item.isDir()) {
            if (ContentDisplaySettings::directorySizeMode() == ContentDisplaySettings::EnumDirectorySizeMode::ContentCount || m_sortDirsFirst) {
                newGroupValue = i18nc("@title:group Size", "Folders");
            } else {
                fileSize = m_itemData.at(i)->values.value("size").toULongLong();
            }
        }

        if (newGroupValue.isEmpty()) {
            if (fileSize < 5 * 1024 * 1024) { // < 5 MB
                newGroupValue = i18nc("@title:group Size", "Small");
            } else if (fileSize < 10 * 1024 * 1024) { // < 10 MB
                newGroupValue = i18nc("@title:group Size", "Medium");
            } else {
                newGroupValue = i18nc("@title:group Size", "Big");
            }
        }

        if (newGroupValue != groupValue) {
            groupValue = newGroupValue;
            groups.append(QPair<int, QVariant>(i, newGroupValue));
        }
    }

    return groups;
}

QList<QPair<int, QVariant>> KFileItemModel::timeRoleGroups(const std::function<QDateTime(const ItemData *)> &fileTimeCb) const
{
    Q_ASSERT(!m_itemData.isEmpty());

    const int maxIndex = count() - 1;
    QList<QPair<int, QVariant>> groups;

    const QDate currentDate = QDate::currentDate();

    QDate previousFileDate;
    QString groupValue;
    for (int i = 0; i <= maxIndex; ++i) {
        if (isChildItem(i)) {
            continue;
        }

        const QLocale locale;
        const QDateTime fileTime = fileTimeCb(m_itemData.at(i));
        const QDate fileDate = fileTime.date();
        if (fileDate == previousFileDate) {
            // The current item is in the same group as the previous item
            continue;
        }
        previousFileDate = fileDate;

        const int daysDistance = fileDate.daysTo(currentDate);

        QString newGroupValue;
        if (currentDate.year() == fileDate.year() && currentDate.month() == fileDate.month()) {
            switch (daysDistance / 7) {
            case 0:
                switch (daysDistance) {
                case 0:
                    newGroupValue = i18nc("@title:group Date", "Today");
                    break;
                case 1:
                    newGroupValue = i18nc("@title:group Date", "Yesterday");
                    break;
                default:
                    newGroupValue = locale.toString(fileTime, i18nc("@title:group Date: The week day name: dddd", "dddd"));
                    newGroupValue = i18nc(
                        "Can be used to script translation of \"dddd\""
                        "with context @title:group Date",
                        "%1",
                        newGroupValue);
                }
                break;
            case 1:
                newGroupValue = i18nc("@title:group Date", "One Week Ago");
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
            if (lastMonthDate.year() == fileDate.year() && lastMonthDate.month() == fileDate.month()) {
                if (daysDistance == 1) {
                    const KLocalizedString format = ki18nc(
                        "@title:group Date: "
                        "MMMM is full month name in current locale, and yyyy is "
                        "full year number. You must keep the ' don't use any fancy \" or « or similar. The ' is not shown to the user, it's there to mark a "
                        "part of the text that should not be formatted as a date",
                        "'Yesterday' (MMMM, yyyy)");
                    const QString translatedFormat = format.toString();
                    if (translatedFormat.count(QLatin1Char('\'')) == 2) {
                        newGroupValue = locale.toString(fileTime, translatedFormat);
                        newGroupValue = i18nc(
                            "Can be used to script translation of "
                            "\"'Yesterday' (MMMM, yyyy)\" with context @title:group Date",
                            "%1",
                            newGroupValue);
                    } else {
                        qCWarning(DolphinDebug).nospace()
                            << "A wrong translation was found: " << translatedFormat << ". Please file a bug report at bugs.kde.org";
                        const QString untranslatedFormat = format.toString({QLatin1String("en_US")});
                        newGroupValue = locale.toString(fileTime, untranslatedFormat);
                    }
                } else if (daysDistance <= 7) {
                    newGroupValue = locale.toString(fileTime,
                                                    i18nc("@title:group Date: "
                                                          "The week day name: dddd, MMMM is full month name "
                                                          "in current locale, and yyyy is full year number.",
                                                          "dddd (MMMM, yyyy)"));
                    newGroupValue = i18nc(
                        "Can be used to script translation of "
                        "\"dddd (MMMM, yyyy)\" with context @title:group Date",
                        "%1",
                        newGroupValue);
                } else if (daysDistance <= 7 * 2) {
                    const KLocalizedString format = ki18nc(
                        "@title:group Date: "
                        "MMMM is full month name in current locale, and yyyy is "
                        "full year number. You must keep the ' don't use any fancy \" or « or similar. The ' is not shown to the user, it's there to mark a "
                        "part of the text that should not be formatted as a date",
                        "'One Week Ago' (MMMM, yyyy)");
                    const QString translatedFormat = format.toString();
                    if (translatedFormat.count(QLatin1Char('\'')) == 2) {
                        newGroupValue = locale.toString(fileTime, translatedFormat);
                        newGroupValue = i18nc(
                            "Can be used to script translation of "
                            "\"'One Week Ago' (MMMM, yyyy)\" with context @title:group Date",
                            "%1",
                            newGroupValue);
                    } else {
                        qCWarning(DolphinDebug).nospace()
                            << "A wrong translation was found: " << translatedFormat << ". Please file a bug report at bugs.kde.org";
                        const QString untranslatedFormat = format.toString({QLatin1String("en_US")});
                        newGroupValue = locale.toString(fileTime, untranslatedFormat);
                    }
                } else if (daysDistance <= 7 * 3) {
                    const KLocalizedString format = ki18nc(
                        "@title:group Date: "
                        "MMMM is full month name in current locale, and yyyy is "
                        "full year number. You must keep the ' don't use any fancy \" or « or similar. The ' is not shown to the user, it's there to mark a "
                        "part of the text that should not be formatted as a date",
                        "'Two Weeks Ago' (MMMM, yyyy)");
                    const QString translatedFormat = format.toString();
                    if (translatedFormat.count(QLatin1Char('\'')) == 2) {
                        newGroupValue = locale.toString(fileTime, translatedFormat);
                        newGroupValue = i18nc(
                            "Can be used to script translation of "
                            "\"'Two Weeks Ago' (MMMM, yyyy)\" with context @title:group Date",
                            "%1",
                            newGroupValue);
                    } else {
                        qCWarning(DolphinDebug).nospace()
                            << "A wrong translation was found: " << translatedFormat << ". Please file a bug report at bugs.kde.org";
                        const QString untranslatedFormat = format.toString({QLatin1String("en_US")});
                        newGroupValue = locale.toString(fileTime, untranslatedFormat);
                    }
                } else if (daysDistance <= 7 * 4) {
                    const KLocalizedString format = ki18nc(
                        "@title:group Date: "
                        "MMMM is full month name in current locale, and yyyy is "
                        "full year number. You must keep the ' don't use any fancy \" or « or similar. The ' is not shown to the user, it's there to mark a "
                        "part of the text that should not be formatted as a date",
                        "'Three Weeks Ago' (MMMM, yyyy)");
                    const QString translatedFormat = format.toString();
                    if (translatedFormat.count(QLatin1Char('\'')) == 2) {
                        newGroupValue = locale.toString(fileTime, translatedFormat);
                        newGroupValue = i18nc(
                            "Can be used to script translation of "
                            "\"'Three Weeks Ago' (MMMM, yyyy)\" with context @title:group Date",
                            "%1",
                            newGroupValue);
                    } else {
                        qCWarning(DolphinDebug).nospace()
                            << "A wrong translation was found: " << translatedFormat << ". Please file a bug report at bugs.kde.org";
                        const QString untranslatedFormat = format.toString({QLatin1String("en_US")});
                        newGroupValue = locale.toString(fileTime, untranslatedFormat);
                    }
                } else {
                    const KLocalizedString format = ki18nc(
                        "@title:group Date: "
                        "MMMM is full month name in current locale, and yyyy is "
                        "full year number. You must keep the ' don't use any fancy \" or « or similar. The ' is not shown to the user, it's there to mark a "
                        "part of the text that should not be formatted as a date",
                        "'Earlier on' MMMM, yyyy");
                    const QString translatedFormat = format.toString();
                    if (translatedFormat.count(QLatin1Char('\'')) == 2) {
                        newGroupValue = locale.toString(fileTime, translatedFormat);
                        newGroupValue = i18nc(
                            "Can be used to script translation of "
                            "\"'Earlier on' MMMM, yyyy\" with context @title:group Date",
                            "%1",
                            newGroupValue);
                    } else {
                        qCWarning(DolphinDebug).nospace()
                            << "A wrong translation was found: " << translatedFormat << ". Please file a bug report at bugs.kde.org";
                        const QString untranslatedFormat = format.toString({QLatin1String("en_US")});
                        newGroupValue = locale.toString(fileTime, untranslatedFormat);
                    }
                }
            } else {
                newGroupValue = locale.toString(fileTime,
                                                i18nc("@title:group "
                                                      "The month and year: MMMM is full month name in current locale, "
                                                      "and yyyy is full year number",
                                                      "MMMM, yyyy"));
                newGroupValue = i18nc(
                    "Can be used to script translation of "
                    "\"MMMM, yyyy\" with context @title:group Date",
                    "%1",
                    newGroupValue);
            }
        }

        if (newGroupValue != groupValue) {
            groupValue = newGroupValue;
            groups.append(QPair<int, QVariant>(i, newGroupValue));
        }
    }

    return groups;
}

QList<QPair<int, QVariant>> KFileItemModel::permissionRoleGroups() const
{
    Q_ASSERT(!m_itemData.isEmpty());

    const int maxIndex = count() - 1;
    QList<QPair<int, QVariant>> groups;

    QString permissionsString;
    QString groupValue;
    for (int i = 0; i <= maxIndex; ++i) {
        if (isChildItem(i)) {
            continue;
        }

        const ItemData *itemData = m_itemData.at(i);
        const QString newPermissionsString = itemData->values.value("permissions").toString();
        if (newPermissionsString == permissionsString) {
            continue;
        }
        permissionsString = newPermissionsString;

        const QFileInfo info(itemData->item.url().toLocalFile());

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
        user = user.isEmpty() ? i18nc("@item:intext Access permission, concatenated", "Forbidden") : user.mid(0, user.length() - 2);

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
        group = group.isEmpty() ? i18nc("@item:intext Access permission, concatenated", "Forbidden") : group.mid(0, group.length() - 2);

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
        others = others.isEmpty() ? i18nc("@item:intext Access permission, concatenated", "Forbidden") : others.mid(0, others.length() - 2);

        const QString newGroupValue = i18nc("@title:group Files and folders by permissions", "User: %1 | Group: %2 | Others: %3", user, group, others);
        if (newGroupValue != groupValue) {
            groupValue = newGroupValue;
            groups.append(QPair<int, QVariant>(i, newGroupValue));
        }
    }

    return groups;
}

QList<QPair<int, QVariant>> KFileItemModel::ratingRoleGroups() const
{
    Q_ASSERT(!m_itemData.isEmpty());

    const int maxIndex = count() - 1;
    QList<QPair<int, QVariant>> groups;

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

QList<QPair<int, QVariant>> KFileItemModel::genericStringRoleGroups(const QByteArray &role) const
{
    Q_ASSERT(!m_itemData.isEmpty());

    const int maxIndex = count() - 1;
    QList<QPair<int, QVariant>> groups;

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

        Q_EMIT directorySortingProgress(100);
    } else if (itemCount > 0) {
        resolvedCount = qBound(0, resolvedCount, itemCount);

        const int progress = resolvedCount * 100 / itemCount;
        if (m_sortingProgressPercent != progress) {
            m_sortingProgressPercent = progress;
            Q_EMIT directorySortingProgress(progress);
        }
    }
}

const KFileItemModel::RoleInfoMap *KFileItemModel::rolesInfoMap(int &count)
{
    static const RoleInfoMap rolesInfoMap[] = {
        // clang-format off
    //  |         role           |        roleType        |                role translation          |         group translation                                        | requires Baloo | requires indexer
        { nullptr,               NoRole,                  KLazyLocalizedString(),                    KLazyLocalizedString(),        KLazyLocalizedString(),                    false,           false },
        { "text",                NameRole,                kli18nc("@label", "Name"),                 KLazyLocalizedString(),        KLazyLocalizedString(),                    false,           false },
        { "size",                SizeRole,                kli18nc("@label", "Size"),                 KLazyLocalizedString(),        KLazyLocalizedString(),                    false,           false },
        { "modificationtime",    ModificationTimeRole,    kli18nc("@label", "Modified"),             KLazyLocalizedString(),        kli18nc("@tooltip", "The date format can be selected in settings."),                    false,           false },
        { "creationtime",        CreationTimeRole,        kli18nc("@label", "Created"),              KLazyLocalizedString(),        kli18nc("@tooltip", "The date format can be selected in settings."),                    false,           false },
        { "accesstime",          AccessTimeRole,          kli18nc("@label", "Accessed"),             KLazyLocalizedString(),        kli18nc("@tooltip", "The date format can be selected in settings."),                    false,           false },
        { "type",                TypeRole,                kli18nc("@label", "Type"),                 KLazyLocalizedString(),        KLazyLocalizedString(),                    false,           false },
        { "rating",              RatingRole,              kli18nc("@label", "Rating"),               KLazyLocalizedString(),        KLazyLocalizedString(),                    true,            false },
        { "tags",                TagsRole,                kli18nc("@label", "Tags"),                 KLazyLocalizedString(),        KLazyLocalizedString(),                    true,            false },
        { "comment",             CommentRole,             kli18nc("@label", "Comment"),              KLazyLocalizedString(),        KLazyLocalizedString(),                    true,            false },
        { "title",               TitleRole,               kli18nc("@label", "Title"),                kli18nc("@label", "Document"), KLazyLocalizedString(),                    true,            true  },
        { "author",              AuthorRole,              kli18nc("@label", "Author"),               kli18nc("@label", "Document"), KLazyLocalizedString(),                    true,            true  },
        { "publisher",           PublisherRole,           kli18nc("@label", "Publisher"),            kli18nc("@label", "Document"), KLazyLocalizedString(),                    true,            true  },
        { "pageCount",           PageCountRole,           kli18nc("@label", "Page Count"),           kli18nc("@label", "Document"), KLazyLocalizedString(),                    true,            true  },
        { "wordCount",           WordCountRole,           kli18nc("@label", "Word Count"),           kli18nc("@label", "Document"), KLazyLocalizedString(),                    true,            true  },
        { "lineCount",           LineCountRole,           kli18nc("@label", "Line Count"),           kli18nc("@label", "Document"), KLazyLocalizedString(),                    true,            true  },
        { "imageDateTime",       ImageDateTimeRole,       kli18nc("@label", "Date Photographed"),    kli18nc("@label", "Image"),    KLazyLocalizedString(),                    true,            true  },
        { "dimensions",          DimensionsRole,          kli18nc("@label width x height", "Dimensions"), kli18nc("@label", "Image"), KLazyLocalizedString(),                  true,            true  },
        { "width",               WidthRole,               kli18nc("@label", "Width"),                kli18nc("@label", "Image"),    KLazyLocalizedString(),                    true,            true  },
        { "height",              HeightRole,              kli18nc("@label", "Height"),               kli18nc("@label", "Image"),    KLazyLocalizedString(),                    true,            true  },
        { "orientation",         OrientationRole,         kli18nc("@label", "Orientation"),          kli18nc("@label", "Image"),    KLazyLocalizedString(),                    true,            true  },
        { "artist",              ArtistRole,              kli18nc("@label", "Artist"),               kli18nc("@label", "Audio"),    KLazyLocalizedString(),                    true,            true  },
        { "genre",               GenreRole,               kli18nc("@label", "Genre"),                kli18nc("@label", "Audio"),    KLazyLocalizedString(),                    true,            true  },
        { "album",               AlbumRole,               kli18nc("@label", "Album"),                kli18nc("@label", "Audio"),    KLazyLocalizedString(),                    true,            true  },
        { "duration",            DurationRole,            kli18nc("@label", "Duration"),             kli18nc("@label", "Audio"),    KLazyLocalizedString(),                    true,            true  },
        { "bitrate",             BitrateRole,             kli18nc("@label", "Bitrate"),              kli18nc("@label", "Audio"),    KLazyLocalizedString(),                    true,            true  },
        { "track",               TrackRole,               kli18nc("@label", "Track"),                kli18nc("@label", "Audio"),    KLazyLocalizedString(),                    true,            true  },
        { "releaseYear",         ReleaseYearRole,         kli18nc("@label", "Release Year"),         kli18nc("@label", "Audio"),    KLazyLocalizedString(),                    true,            true  },
        { "aspectRatio",         AspectRatioRole,         kli18nc("@label", "Aspect Ratio"),         kli18nc("@label", "Video"),    KLazyLocalizedString(),                    true,            true  },
        { "frameRate",           FrameRateRole,           kli18nc("@label", "Frame Rate"),           kli18nc("@label", "Video"),    KLazyLocalizedString(),                    true,            true  },
        { "duration",            DurationRole,            kli18nc("@label", "Duration"),             kli18nc("@label", "Video"),    KLazyLocalizedString(),                    true,            true  },
        { "path",                PathRole,                kli18nc("@label", "Path"),                 kli18nc("@label", "Other"),    KLazyLocalizedString(),                    false,           false },
        { "extension",           ExtensionRole,           kli18nc("@label", "File Extension"),       kli18nc("@label", "Other"),    KLazyLocalizedString(),                    false,           false },
        { "deletiontime",        DeletionTimeRole,        kli18nc("@label", "Deletion Time"),        kli18nc("@label", "Other"),    KLazyLocalizedString(),                    false,           false },
        { "destination",         DestinationRole,         kli18nc("@label", "Link Destination"),     kli18nc("@label", "Other"),    KLazyLocalizedString(),                    false,           false },
        { "originUrl",           OriginUrlRole,           kli18nc("@label", "Downloaded From"),      kli18nc("@label", "Other"),    KLazyLocalizedString(),                    true,            false },
        { "permissions",         PermissionsRole,         kli18nc("@label", "Permissions"),          kli18nc("@label", "Other"),    kli18nc("@tooltip", "The permission format can be changed in settings. Options are Symbolic, Numeric (Octal) or Combined formats"),        false,           false },
        { "owner",               OwnerRole,               kli18nc("@label", "Owner"),                kli18nc("@label", "Other"),    KLazyLocalizedString(),                    false,           false },
        { "group",               GroupRole,               kli18nc("@label", "User Group"),           kli18nc("@label", "Other"),    KLazyLocalizedString(),                    false,           false },
    };
    // clang-format on

    count = sizeof(rolesInfoMap) / sizeof(RoleInfoMap);
    return rolesInfoMap;
}

void KFileItemModel::determineMimeTypes(const KFileItemList &items, int timeout)
{
    QElapsedTimer timer;
    timer.start();
    for (const KFileItem &item : items) {
        // Only determine mime types for files here. For directories,
        // KFileItem::determineMimeType() reads the .directory file inside to
        // load the icon, but this is not necessary at all if we just need the
        // type. Some special code for setting the correct mime type for
        // directories is in retrieveData().
        if (!item.isDir()) {
            item.determineMimeType();
        }

        if (timer.elapsed() > timeout) {
            // Don't block the user interface, let the remaining items
            // be resolved asynchronously.
            return;
        }
    }
}

QByteArray KFileItemModel::sharedValue(const QByteArray &value)
{
    static QSet<QByteArray> pool;
    const QSet<QByteArray>::const_iterator it = pool.constFind(value);

    if (it != pool.constEnd()) {
        return *it;
    } else {
        pool.insert(value);
        return value;
    }
}

bool KFileItemModel::isConsistent() const
{
    // m_items may contain less items than m_itemData because m_items
    // is populated lazily, see KFileItemModel::index(const QUrl& url).
    if (m_items.count() > m_itemData.count()) {
        return false;
    }

    for (int i = 0, iMax = count(); i < iMax; ++i) {
        // Check if m_items and m_itemData are consistent.
        const KFileItem item = fileItem(i);
        if (item.isNull()) {
            qCWarning(DolphinDebug) << "Item" << i << "is null";
            return false;
        }

        const int itemIndex = index(item);
        if (itemIndex != i) {
            qCWarning(DolphinDebug) << "Item" << i << "has a wrong index:" << itemIndex;
            return false;
        }

        // Check if the items are sorted correctly.
        if (i > 0 && !lessThan(m_itemData.at(i - 1), m_itemData.at(i), m_collator)) {
            qCWarning(DolphinDebug) << "The order of items" << i - 1 << "and" << i << "is wrong:" << fileItem(i - 1) << fileItem(i);
            return false;
        }

        // Check if all parent-child relationships are consistent.
        const ItemData *data = m_itemData.at(i);
        const ItemData *parent = data->parent;
        if (parent) {
            if (expandedParentsCount(data) != expandedParentsCount(parent) + 1) {
                qCWarning(DolphinDebug) << "expandedParentsCount is inconsistent for parent" << parent->item << "and child" << data->item;
                return false;
            }

            const int parentIndex = index(parent->item);
            if (parentIndex >= i) {
                qCWarning(DolphinDebug) << "Index" << parentIndex << "of parent" << parent->item << "is not smaller than index" << i << "of child"
                                        << data->item;
                return false;
            }
        }
    }

    return true;
}

void KFileItemModel::slotListerError(KIO::Job *job)
{
    const int jobError = job->error();
    if (jobError == KIO::ERR_IS_FILE) {
        if (auto *listJob = qobject_cast<KIO::ListJob *>(job)) {
            Q_EMIT urlIsFileError(listJob->url());
        }
    } else {
        const QString errorString = job->errorString();
        Q_EMIT errorMessage(!errorString.isEmpty() ? errorString : i18nc("@info:status", "Unknown error."), jobError);
    }
}

#include "moc_kfileitemmodel.cpp"
