/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on KFilePlacesModel from kdelibs:                               *
 *   Copyright (C) 2007 Kevin Ottens <ervin@kde.org>                       *
 *   Copyright (C) 2007 David Faure <faure@kde.org>                        *
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

#include "placesitemmodel.h"
#include "placesitemsignalhandler.h"

#include "dolphin_generalsettings.h"

#include <KBookmark>
#include <KBookmarkManager>
#include "dolphindebug.h"
#include <QIcon>
#include <KProtocolInfo>
#include <KLocalizedString>
#include <QStandardPaths>
#include <KAboutData>
#include "placesitem.h"
#include <QAction>
#include <QDate>
#include <QMimeData>
#include <QTimer>
#include <KUrlMimeData>

#include <Solid/Device>
#include <Solid/DeviceNotifier>
#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>
#include <Solid/StorageAccess>
#include <Solid/StorageDrive>

#include <views/dolphinview.h>
#include <views/viewproperties.h>

#ifdef HAVE_BALOO
    #include <Baloo/Query>
    #include <Baloo/IndexerConfig>
#endif

namespace {
    // As long as KFilePlacesView from kdelibs is available in parallel, the
    // system-bookmarks for "Recently Saved" and "Search For" should be
    // shown only inside the Places Panel. This is necessary as the stored
    // URLs needs to get translated to a Baloo-search-URL on-the-fly to
    // be independent from changes in the Baloo-search-URL-syntax.
    // Hence a prefix to the application-name of the stored bookmarks is
    // added, which is only read by PlacesItemModel.
    const char AppNamePrefix[] = "-places-panel";
}

PlacesItemModel::PlacesItemModel(QObject* parent) :
    KStandardItemModel(parent),
    m_fileIndexingEnabled(false),
    m_hiddenItemsShown(false),
    m_availableDevices(),
    m_predicate(),
    m_bookmarkManager(0),
    m_systemBookmarks(),
    m_systemBookmarksIndexes(),
    m_bookmarkedItems(),
    m_hiddenItemToRemove(-1),
    m_deviceToTearDown(0),
    m_updateBookmarksTimer(0),
    m_storageSetupInProgress()
{
#ifdef HAVE_BALOO
    Baloo::IndexerConfig config;
    m_fileIndexingEnabled = config.fileIndexingEnabled();
#endif
    const QString file = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/user-places.xbel";
    m_bookmarkManager = KBookmarkManager::managerForExternalFile(file);

    createSystemBookmarks();
    initializeAvailableDevices();
    loadBookmarks();

    const int syncBookmarksTimeout = 100;

    m_updateBookmarksTimer = new QTimer(this);
    m_updateBookmarksTimer->setInterval(syncBookmarksTimeout);
    m_updateBookmarksTimer->setSingleShot(true);
    connect(m_updateBookmarksTimer, &QTimer::timeout, this, &PlacesItemModel::updateBookmarks);

    connect(m_bookmarkManager, &KBookmarkManager::changed,
            m_updateBookmarksTimer, static_cast<void(QTimer::*)()>(&QTimer::start));
}

PlacesItemModel::~PlacesItemModel()
{
    qDeleteAll(m_bookmarkedItems);
    m_bookmarkedItems.clear();
}

PlacesItem* PlacesItemModel::createPlacesItem(const QString& text,
                                              const QUrl& url,
                                              const QString& iconName)
{
    const KBookmark bookmark = PlacesItem::createBookmark(m_bookmarkManager, text, url, iconName);
    return new PlacesItem(bookmark);
}

PlacesItem* PlacesItemModel::placesItem(int index) const
{
    return dynamic_cast<PlacesItem*>(item(index));
}

int PlacesItemModel::hiddenCount() const
{
    int modelIndex = 0;
    int hiddenItemCount = 0;
    foreach (const PlacesItem* item, m_bookmarkedItems) {
        if (item) {
            ++hiddenItemCount;
        } else {
            if (placesItem(modelIndex)->isHidden()) {
                ++hiddenItemCount;
            }
            ++modelIndex;
        }
    }

    return hiddenItemCount;
}

void PlacesItemModel::setHiddenItemsShown(bool show)
{
    if (m_hiddenItemsShown == show) {
        return;
    }

    m_hiddenItemsShown = show;

    if (show) {
        // Move all items that are part of m_bookmarkedItems to the model.
        QList<PlacesItem*> itemsToInsert;
        QList<int> insertPos;
        int modelIndex = 0;
        for (int i = 0; i < m_bookmarkedItems.count(); ++i) {
            if (m_bookmarkedItems[i]) {
                itemsToInsert.append(m_bookmarkedItems[i]);
                m_bookmarkedItems[i] = 0;
                insertPos.append(modelIndex);
            }
            ++modelIndex;
        }

        // Inserting the items will automatically insert an item
        // to m_bookmarkedItems in PlacesItemModel::onItemsInserted().
        // The items are temporary saved in itemsToInsert, so
        // m_bookmarkedItems can be shrinked now.
        m_bookmarkedItems.erase(m_bookmarkedItems.begin(),
                                m_bookmarkedItems.begin() + itemsToInsert.count());

        for (int i = 0; i < itemsToInsert.count(); ++i) {
            insertItem(insertPos[i], itemsToInsert[i]);
        }

        Q_ASSERT(m_bookmarkedItems.count() == count());
    } else {
        // Move all items of the model, where the "isHidden" property is true, to
        // m_bookmarkedItems.
        Q_ASSERT(m_bookmarkedItems.count() == count());
        for (int i = count() - 1; i >= 0; --i) {
            if (placesItem(i)->isHidden()) {
                hideItem(i);
            }
        }
    }

#ifdef PLACESITEMMODEL_DEBUG
        qCDebug(DolphinDebug) << "Changed visibility of hidden items";
        showModelState();
#endif
}

bool PlacesItemModel::hiddenItemsShown() const
{
    return m_hiddenItemsShown;
}

int PlacesItemModel::closestItem(const QUrl& url) const
{
    int foundIndex = -1;
    int maxLength = 0;

    for (int i = 0; i < count(); ++i) {
        const QUrl itemUrl = placesItem(i)->url();
        if (url == itemUrl) {
            // We can't find a closer one, so stop here.
            foundIndex = i;
            break;
        } else if (itemUrl.isParentOf(url)) {
            const int length = itemUrl.path().length();
            if (length > maxLength) {
                foundIndex = i;
                maxLength = length;
            }
        }
    }

    return foundIndex;
}

void PlacesItemModel::appendItemToGroup(PlacesItem* item)
{
    if (!item) {
        return;
    }

    int i = 0;
    while (i < count() && placesItem(i)->group() != item->group()) {
        ++i;
    }

    bool inserted = false;
    while (!inserted && i < count()) {
        if (placesItem(i)->group() != item->group()) {
            insertItem(i, item);
            inserted = true;
        }
        ++i;
    }

    if (!inserted) {
        appendItem(item);
    }
}


QAction* PlacesItemModel::ejectAction(int index) const
{
    const PlacesItem* item = placesItem(index);
    if (item && item->device().is<Solid::OpticalDisc>()) {
        return new QAction(QIcon::fromTheme(QStringLiteral("media-eject")), i18nc("@item", "Eject"), 0);
    }

    return 0;
}

QAction* PlacesItemModel::teardownAction(int index) const
{
    const PlacesItem* item = placesItem(index);
    if (!item) {
        return 0;
    }

    Solid::Device device = item->device();
    const bool providesTearDown = device.is<Solid::StorageAccess>() &&
                                  device.as<Solid::StorageAccess>()->isAccessible();
    if (!providesTearDown) {
        return 0;
    }

    Solid::StorageDrive* drive = device.as<Solid::StorageDrive>();
    if (!drive) {
        drive = device.parent().as<Solid::StorageDrive>();
    }

    bool hotPluggable = false;
    bool removable = false;
    if (drive) {
        hotPluggable = drive->isHotpluggable();
        removable = drive->isRemovable();
    }

    QString iconName;
    QString text;
    if (device.is<Solid::OpticalDisc>()) {
        text = i18nc("@item", "Release");
    } else if (removable || hotPluggable) {
        text = i18nc("@item", "Safely Remove");
        iconName = QStringLiteral("media-eject");
    } else {
        text = i18nc("@item", "Unmount");
        iconName = QStringLiteral("media-eject");
    }

    if (iconName.isEmpty()) {
        return new QAction(text, 0);
    }

    return new QAction(QIcon::fromTheme(iconName), text, 0);
}

void PlacesItemModel::requestEject(int index)
{
    const PlacesItem* item = placesItem(index);
    if (item) {
        Solid::OpticalDrive* drive = item->device().parent().as<Solid::OpticalDrive>();
        if (drive) {
            connect(drive, &Solid::OpticalDrive::ejectDone,
                    this, &PlacesItemModel::slotStorageTearDownDone);
            drive->eject();
        } else {
            const QString label = item->text();
            const QString message = i18nc("@info", "The device '%1' is not a disk and cannot be ejected.", label);
            emit errorMessage(message);
        }
    }
}

void PlacesItemModel::requestTearDown(int index)
{
    const PlacesItem* item = placesItem(index);
    if (item) {
        Solid::StorageAccess *tmp = item->device().as<Solid::StorageAccess>();
        if (tmp) {
            m_deviceToTearDown = tmp;
            // disconnect the Solid::StorageAccess::teardownRequested
            // to prevent emitting PlacesItemModel::storageTearDownExternallyRequested
            // after we have emitted PlacesItemModel::storageTearDownRequested
            disconnect(tmp, &Solid::StorageAccess::teardownRequested,
                       item->signalHandler(), &PlacesItemSignalHandler::onTearDownRequested);
            emit storageTearDownRequested(tmp->filePath());
        }
    }
}

bool PlacesItemModel::storageSetupNeeded(int index) const
{
    const PlacesItem* item = placesItem(index);
    return item ? item->storageSetupNeeded() : false;
}

void PlacesItemModel::requestStorageSetup(int index)
{
    const PlacesItem* item = placesItem(index);
    if (!item) {
        return;
    }

    Solid::Device device = item->device();
    const bool setup = device.is<Solid::StorageAccess>()
                       && !m_storageSetupInProgress.contains(device.as<Solid::StorageAccess>())
                       && !device.as<Solid::StorageAccess>()->isAccessible();
    if (setup) {
        Solid::StorageAccess* access = device.as<Solid::StorageAccess>();

        m_storageSetupInProgress[access] = index;

        connect(access, &Solid::StorageAccess::setupDone,
                this, &PlacesItemModel::slotStorageSetupDone);

        access->setup();
    }
}

QMimeData* PlacesItemModel::createMimeData(const KItemSet& indexes) const
{
    QList<QUrl> urls;
    QByteArray itemData;

    QDataStream stream(&itemData, QIODevice::WriteOnly);

    for (int index : indexes) {
        const QUrl itemUrl = placesItem(index)->url();
        if (itemUrl.isValid()) {
            urls << itemUrl;
        }
        stream << index;
    }

    QMimeData* mimeData = new QMimeData();
    if (!urls.isEmpty()) {
        mimeData->setUrls(urls);
    } else {
        // #378954: prevent itemDropEvent() drops if there isn't a source url.
        mimeData->setData(blacklistItemDropEventMimeType(), QByteArrayLiteral("true"));
    }
    mimeData->setData(internalMimeType(), itemData);

    return mimeData;
}

bool PlacesItemModel::supportsDropping(int index) const
{
    return index >= 0 && index < count();
}

void PlacesItemModel::dropMimeDataBefore(int index, const QMimeData* mimeData)
{
    if (mimeData->hasFormat(internalMimeType())) {
        // The item has been moved inside the view
        QByteArray itemData = mimeData->data(internalMimeType());
        QDataStream stream(&itemData, QIODevice::ReadOnly);
        int oldIndex;
        stream >> oldIndex;
        if (oldIndex == index || oldIndex == index - 1) {
            // No moving has been done
            return;
        }

        PlacesItem* oldItem = placesItem(oldIndex);
        if (!oldItem) {
            return;
        }

        PlacesItem* newItem = new PlacesItem(oldItem->bookmark());
        removeItem(oldIndex);

        if (oldIndex < index) {
            --index;
        }

        const int dropIndex = groupedDropIndex(index, newItem);
        insertItem(dropIndex, newItem);
    } else if (mimeData->hasFormat(QStringLiteral("text/uri-list"))) {
        // One or more items must be added to the model
        const QList<QUrl> urls = KUrlMimeData::urlsFromMimeData(mimeData);
        for (int i = urls.count() - 1; i >= 0; --i) {
            const QUrl& url = urls[i];

            QString text = url.fileName();
            if (text.isEmpty()) {
                text = url.host();
            }

            if ((url.isLocalFile() && !QFileInfo(url.toLocalFile()).isDir())
                    || url.scheme() == QLatin1String("trash")) {
                // Only directories outside the trash are allowed
                continue;
            }

            PlacesItem* newItem = createPlacesItem(text, url);
            const int dropIndex = groupedDropIndex(index, newItem);
            insertItem(dropIndex, newItem);
        }
    }
}

QUrl PlacesItemModel::convertedUrl(const QUrl& url)
{
    QUrl newUrl = url;
    if (url.scheme() == QLatin1String("timeline")) {
        newUrl = createTimelineUrl(url);
    } else if (url.scheme() == QLatin1String("search")) {
        newUrl = createSearchUrl(url);
    }

    return newUrl;
}

void PlacesItemModel::onItemInserted(int index)
{
    const PlacesItem* insertedItem = placesItem(index);
    if (insertedItem) {
        // Take care to apply the PlacesItemModel-order of the inserted item
        // also to the bookmark-manager.
        const KBookmark insertedBookmark = insertedItem->bookmark();

        const PlacesItem* previousItem = placesItem(index - 1);
        KBookmark previousBookmark;
        if (previousItem) {
            previousBookmark = previousItem->bookmark();
        }

        m_bookmarkManager->root().moveBookmark(insertedBookmark, previousBookmark);
    }

    if (index == count() - 1) {
        // The item has been appended as last item to the list. In this
        // case assure that it is also appended after the hidden items and
        // not before (like done otherwise).
        m_bookmarkedItems.append(0);
    } else {

        int modelIndex = -1;
        int bookmarkIndex = 0;
        while (bookmarkIndex < m_bookmarkedItems.count()) {
            if (!m_bookmarkedItems[bookmarkIndex]) {
                ++modelIndex;
                if (modelIndex + 1 == index) {
                    break;
                }
            }
            ++bookmarkIndex;
        }
        m_bookmarkedItems.insert(bookmarkIndex, 0);
    }

#ifdef PLACESITEMMODEL_DEBUG
    qCDebug(DolphinDebug) << "Inserted item" << index;
    showModelState();
#endif
}

void PlacesItemModel::onItemRemoved(int index, KStandardItem* removedItem)
{
    PlacesItem* placesItem = dynamic_cast<PlacesItem*>(removedItem);
    if (placesItem) {
        const KBookmark bookmark = placesItem->bookmark();
        m_bookmarkManager->root().deleteBookmark(bookmark);
    }

    const int boomarkIndex = bookmarkIndex(index);
    Q_ASSERT(!m_bookmarkedItems[boomarkIndex]);
    m_bookmarkedItems.removeAt(boomarkIndex);

#ifdef PLACESITEMMODEL_DEBUG
    qCDebug(DolphinDebug) << "Removed item" << index;
    showModelState();
#endif
}

void PlacesItemModel::onItemChanged(int index, const QSet<QByteArray>& changedRoles)
{
    const PlacesItem* changedItem = placesItem(index);
    if (changedItem) {
        // Take care to apply the PlacesItemModel-order of the changed item
        // also to the bookmark-manager.
        const KBookmark insertedBookmark = changedItem->bookmark();

        const PlacesItem* previousItem = placesItem(index - 1);
        KBookmark previousBookmark;
        if (previousItem) {
            previousBookmark = previousItem->bookmark();
        }

        m_bookmarkManager->root().moveBookmark(insertedBookmark, previousBookmark);
    }

    if (changedRoles.contains("isHidden")) {
        if (!m_hiddenItemsShown && changedItem->isHidden()) {
            m_hiddenItemToRemove = index;
            QTimer::singleShot(0, this, static_cast<void (PlacesItemModel::*)()>(&PlacesItemModel::hideItem));
        }
    }
}

void PlacesItemModel::slotDeviceAdded(const QString& udi)
{
    const Solid::Device device(udi);

    if (!m_predicate.matches(device)) {
        return;
    }

    m_availableDevices << udi;
    const KBookmark bookmark = PlacesItem::createDeviceBookmark(m_bookmarkManager, udi);

    PlacesItem *item = new PlacesItem(bookmark);
    appendItem(item);
    connect(item->signalHandler(), &PlacesItemSignalHandler::tearDownExternallyRequested,
            this, &PlacesItemModel::storageTearDownExternallyRequested);
}

void PlacesItemModel::slotDeviceRemoved(const QString& udi)
{
    if (!m_availableDevices.contains(udi)) {
        return;
    }

    for (int i = 0; i < m_bookmarkedItems.count(); ++i) {
        PlacesItem* item = m_bookmarkedItems[i];
        if (item && item->udi() == udi) {
            m_bookmarkedItems.removeAt(i);
            delete item;
            return;
        }
    }

    for (int i = 0; i < count(); ++i) {
        if (placesItem(i)->udi() == udi) {
            removeItem(i);
            return;
        }
    }
}

void PlacesItemModel::slotStorageTearDownDone(Solid::ErrorType error, const QVariant& errorData)
{
    if (error && errorData.isValid()) {
        emit errorMessage(errorData.toString());
    }
    m_deviceToTearDown->disconnect();
    m_deviceToTearDown = nullptr;
}

void PlacesItemModel::slotStorageSetupDone(Solid::ErrorType error,
                                           const QVariant& errorData,
                                           const QString& udi)
{
    Q_UNUSED(udi);

    const int index = m_storageSetupInProgress.take(sender());
    const PlacesItem*  item = placesItem(index);
    if (!item) {
        return;
    }

    if (error != Solid::NoError) {
        if (errorData.isValid()) {
            emit errorMessage(i18nc("@info", "An error occurred while accessing '%1', the system responded: %2",
                                    item->text(),
                                    errorData.toString()));
        } else {
            emit errorMessage(i18nc("@info", "An error occurred while accessing '%1'",
                                    item->text()));
        }
        emit storageSetupDone(index, false);
    } else {
        emit storageSetupDone(index, true);
    }
}

void PlacesItemModel::hideItem()
{
    hideItem(m_hiddenItemToRemove);
    m_hiddenItemToRemove = -1;
}

void PlacesItemModel::updateBookmarks()
{
    // Verify whether new bookmarks have been added or existing
    // bookmarks have been changed.
    KBookmarkGroup root = m_bookmarkManager->root();
    KBookmark newBookmark = root.first();
    while (!newBookmark.isNull()) {
        if (acceptBookmark(newBookmark, m_availableDevices)) {
            bool found = false;
            int modelIndex = 0;
            for (int i = 0; i < m_bookmarkedItems.count(); ++i) {
                PlacesItem* item = m_bookmarkedItems[i];
                if (!item) {
                    item = placesItem(modelIndex);
                    ++modelIndex;
                }

                const KBookmark oldBookmark = item->bookmark();
                if (equalBookmarkIdentifiers(newBookmark, oldBookmark)) {
                    // The bookmark has been found in the model or as
                    // a hidden item. The content of the bookmark might
                    // have been changed, so an update is done.
                    found = true;
                    if (newBookmark.metaDataItem(QStringLiteral("UDI")).isEmpty()) {
                        item->setBookmark(newBookmark);
                        item->setText(i18nc("KFile System Bookmarks", newBookmark.text().toUtf8().constData()));
                    }
                    break;
                }
            }

            if (!found) {
                const QString udi = newBookmark.metaDataItem(QStringLiteral("UDI"));

                /*
                 * See Bug 304878
                 * Only add a new places item, if the item text is not empty
                 * and if the device is available. Fixes the strange behaviour -
                 * add a places item without text in the Places section - when you
                 * remove a device (e.g. a usb stick) without unmounting.
                 */
                if (udi.isEmpty() || Solid::Device(udi).isValid()) {
                    PlacesItem* item = new PlacesItem(newBookmark);
                    if (item->isHidden() && !m_hiddenItemsShown) {
                        m_bookmarkedItems.append(item);
                    } else {
                        appendItemToGroup(item);
                    }
                }
            }
        }

        newBookmark = root.next(newBookmark);
    }

    // Remove items that are not part of the bookmark-manager anymore
    int modelIndex = 0;
    for (int i = m_bookmarkedItems.count() - 1; i >= 0; --i) {
        PlacesItem* item = m_bookmarkedItems[i];
        const bool itemIsPartOfModel = (item == 0);
        if (itemIsPartOfModel) {
            item = placesItem(modelIndex);
        }

        bool hasBeenRemoved = true;
        const KBookmark oldBookmark = item->bookmark();
        KBookmark newBookmark = root.first();
        while (!newBookmark.isNull()) {
            if (equalBookmarkIdentifiers(newBookmark, oldBookmark)) {
                hasBeenRemoved = false;
                break;
            }
            newBookmark = root.next(newBookmark);
        }

        if (hasBeenRemoved) {
            if (m_bookmarkedItems[i]) {
                delete m_bookmarkedItems[i];
                m_bookmarkedItems.removeAt(i);
            } else {
                removeItem(modelIndex);
                --modelIndex;
            }
        }

        if (itemIsPartOfModel) {
            ++modelIndex;
        }
    }
}

void PlacesItemModel::saveBookmarks()
{
    m_bookmarkManager->emitChanged(m_bookmarkManager->root());
}

void PlacesItemModel::loadBookmarks()
{
    KBookmarkGroup root = m_bookmarkManager->root();
    KBookmark bookmark = root.first();
    QSet<QString> devices = m_availableDevices;

    QSet<QUrl> missingSystemBookmarks;
    foreach (const SystemBookmarkData& data, m_systemBookmarks) {
        missingSystemBookmarks.insert(data.url);
    }

    // The bookmarks might have a mixed order of places, devices and search-groups due
    // to the compatibility with the KFilePlacesPanel. In Dolphin's places panel the
    // items should always be collected in one group so the items are collected first
    // in separate lists before inserting them.
    QList<PlacesItem*> placesItems;
    QList<PlacesItem*> recentlySavedItems;
    QList<PlacesItem*> searchForItems;
    QList<PlacesItem*> devicesItems;

    while (!bookmark.isNull()) {
        if (acceptBookmark(bookmark, devices)) {
            PlacesItem* item = new PlacesItem(bookmark);
            if (item->groupType() == PlacesItem::DevicesType) {
                devices.remove(item->udi());
                devicesItems.append(item);
            } else {
                const QUrl url = bookmark.url();
                if (missingSystemBookmarks.contains(url)) {
                    missingSystemBookmarks.remove(url);

                    // Try to retranslate the text of system bookmarks to have translated
                    // items when changing the language. In case if the user has applied a custom
                    // text, the retranslation will fail and the users custom text is still used.
                    // It is important to use "KFile System Bookmarks" as context (see
                    // createSystemBookmarks()).
                    item->setText(i18nc("KFile System Bookmarks", bookmark.text().toUtf8().constData()));
                    item->setSystemItem(true);
                }

                switch (item->groupType()) {
                case PlacesItem::PlacesType:        placesItems.append(item); break;
                case PlacesItem::RecentlySavedType: recentlySavedItems.append(item); break;
                case PlacesItem::SearchForType:     searchForItems.append(item); break;
                case PlacesItem::DevicesType:
                default:                            Q_ASSERT(false); break;
                }
            }
        }

        bookmark = root.next(bookmark);
    }

    if (!missingSystemBookmarks.isEmpty()) {
        // The current bookmarks don't contain all system-bookmarks. Add the missing
        // bookmarks.
        foreach (const SystemBookmarkData& data, m_systemBookmarks) {
            if (missingSystemBookmarks.contains(data.url)) {
                PlacesItem* item = createSystemPlacesItem(data);
                switch (item->groupType()) {
                case PlacesItem::PlacesType:        placesItems.append(item); break;
                case PlacesItem::RecentlySavedType: recentlySavedItems.append(item); break;
                case PlacesItem::SearchForType:     searchForItems.append(item); break;
                case PlacesItem::DevicesType:
                default:                            Q_ASSERT(false); break;
                }
            }
        }
    }

    // Create items for devices that have not been stored as bookmark yet
    devicesItems.reserve(devicesItems.count() + devices.count());
    foreach (const QString& udi, devices) {
        const KBookmark bookmark = PlacesItem::createDeviceBookmark(m_bookmarkManager, udi);
        PlacesItem *item = new PlacesItem(bookmark);
        devicesItems.append(item);
        connect(item->signalHandler(), &PlacesItemSignalHandler::tearDownExternallyRequested,
                this, &PlacesItemModel::storageTearDownExternallyRequested);
    }

    QList<PlacesItem*> items;
    items.append(placesItems);
    items.append(recentlySavedItems);
    items.append(searchForItems);
    items.append(devicesItems);

    foreach (PlacesItem* item, items) {
        if (!m_hiddenItemsShown && item->isHidden()) {
            m_bookmarkedItems.append(item);
        } else {
            appendItem(item);
        }
    }

#ifdef PLACESITEMMODEL_DEBUG
    qCDebug(DolphinDebug) << "Loaded bookmarks";
    showModelState();
#endif
}

bool PlacesItemModel::acceptBookmark(const KBookmark& bookmark,
                                     const QSet<QString>& availableDevices) const
{
    const QString udi = bookmark.metaDataItem(QStringLiteral("UDI"));
    const QUrl url = bookmark.url();
    const QString appName = bookmark.metaDataItem(QStringLiteral("OnlyInApp"));
    const bool deviceAvailable = availableDevices.contains(udi);

    const bool allowedHere = (appName.isEmpty()
                              || appName == KAboutData::applicationData().componentName()
                              || appName == KAboutData::applicationData().componentName() + AppNamePrefix)
                             && (m_fileIndexingEnabled || (url.scheme() != QLatin1String("timeline") &&
                                                           url.scheme() != QLatin1String("search")));

    return (udi.isEmpty() && allowedHere) || deviceAvailable;
}

PlacesItem* PlacesItemModel::createSystemPlacesItem(const SystemBookmarkData& data)
{
    KBookmark bookmark = PlacesItem::createBookmark(m_bookmarkManager,
                                                    data.text,
                                                    data.url,
                                                    data.icon);

    const QString protocol = data.url.scheme();
    if (protocol == QLatin1String("timeline") || protocol == QLatin1String("search")) {
        // As long as the KFilePlacesView from kdelibs is available, the system-bookmarks
        // for "Recently Saved" and "Search For" should be a setting available only
        // in the Places Panel (see description of AppNamePrefix for more details).
        const QString appName = KAboutData::applicationData().componentName() + AppNamePrefix;
        bookmark.setMetaDataItem(QStringLiteral("OnlyInApp"), appName);
    }

    PlacesItem* item = new PlacesItem(bookmark);
    item->setSystemItem(true);

    // Create default view-properties for all "Search For" and "Recently Saved" bookmarks
    // in case if the user has not already created custom view-properties for a corresponding
    // query yet.
    const bool createDefaultViewProperties = (item->groupType() == PlacesItem::SearchForType ||
                                              item->groupType() == PlacesItem::RecentlySavedType) &&
                                              !GeneralSettings::self()->globalViewProps();
    if (createDefaultViewProperties) {
        ViewProperties props(convertedUrl(data.url));
        if (!props.exist()) {
            const QString path = data.url.path();
            if (path == QLatin1String("/documents")) {
                props.setViewMode(DolphinView::DetailsView);
                props.setPreviewsShown(false);
                props.setVisibleRoles({"text", "path"});
            } else if (path == QLatin1String("/images")) {
                props.setViewMode(DolphinView::IconsView);
                props.setPreviewsShown(true);
                props.setVisibleRoles({"text", "imageSize"});
            } else if (path == QLatin1String("/audio")) {
                props.setViewMode(DolphinView::DetailsView);
                props.setPreviewsShown(false);
                props.setVisibleRoles({"text", "artist", "album"});
            } else if (path == QLatin1String("/videos")) {
                props.setViewMode(DolphinView::IconsView);
                props.setPreviewsShown(true);
                props.setVisibleRoles({"text"});
            } else if (data.url.scheme() == QLatin1String("timeline")) {
                props.setViewMode(DolphinView::DetailsView);
                props.setVisibleRoles({"text", "modificationtime"});
            }
        }
    }

    return item;
}

void PlacesItemModel::createSystemBookmarks()
{
    Q_ASSERT(m_systemBookmarks.isEmpty());
    Q_ASSERT(m_systemBookmarksIndexes.isEmpty());

    // Note: The context of the I18N_NOOP2 must be "KFile System Bookmarks". The real
    // i18nc call is done after reading the bookmark. The reason why the i18nc call is not
    // done here is because otherwise switching the language would not result in retranslating the
    // bookmarks.
    m_systemBookmarks.append(SystemBookmarkData(QUrl::fromLocalFile(QDir::homePath()),
                                                QStringLiteral("user-home"),
                                                I18N_NOOP2("KFile System Bookmarks", "Home")));
    m_systemBookmarks.append(SystemBookmarkData(QUrl(QStringLiteral("remote:/")),
                                                QStringLiteral("network-workgroup"),
                                                I18N_NOOP2("KFile System Bookmarks", "Network")));
    m_systemBookmarks.append(SystemBookmarkData(QUrl::fromLocalFile(QStringLiteral("/")),
                                                QStringLiteral("folder-red"),
                                                I18N_NOOP2("KFile System Bookmarks", "Root")));
    m_systemBookmarks.append(SystemBookmarkData(QUrl(QStringLiteral("trash:/")),
                                                QStringLiteral("user-trash"),
                                                I18N_NOOP2("KFile System Bookmarks", "Trash")));

    if (m_fileIndexingEnabled) {
        m_systemBookmarks.append(SystemBookmarkData(QUrl(QStringLiteral("timeline:/today")),
                                                    QStringLiteral("go-jump-today"),
                                                    I18N_NOOP2("KFile System Bookmarks", "Today")));
        m_systemBookmarks.append(SystemBookmarkData(QUrl(QStringLiteral("timeline:/yesterday")),
                                                    QStringLiteral("view-calendar-day"),
                                                    I18N_NOOP2("KFile System Bookmarks", "Yesterday")));
        m_systemBookmarks.append(SystemBookmarkData(QUrl(QStringLiteral("timeline:/thismonth")),
                                                    QStringLiteral("view-calendar-month"),
                                                    I18N_NOOP2("KFile System Bookmarks", "This Month")));
        m_systemBookmarks.append(SystemBookmarkData(QUrl(QStringLiteral("timeline:/lastmonth")),
                                                    QStringLiteral("view-calendar-month"),
                                                    I18N_NOOP2("KFile System Bookmarks", "Last Month")));
        m_systemBookmarks.append(SystemBookmarkData(QUrl(QStringLiteral("search:/documents")),
                                                    QStringLiteral("folder-text"),
                                                    I18N_NOOP2("KFile System Bookmarks", "Documents")));
        m_systemBookmarks.append(SystemBookmarkData(QUrl(QStringLiteral("search:/images")),
                                                    QStringLiteral("folder-images"),
                                                    I18N_NOOP2("KFile System Bookmarks", "Images")));
        m_systemBookmarks.append(SystemBookmarkData(QUrl(QStringLiteral("search:/audio")),
                                                    QStringLiteral("folder-sound"),
                                                    I18N_NOOP2("KFile System Bookmarks", "Audio Files")));
        m_systemBookmarks.append(SystemBookmarkData(QUrl(QStringLiteral("search:/videos")),
                                                    QStringLiteral("folder-videos"),
                                                    I18N_NOOP2("KFile System Bookmarks", "Videos")));
    }

    for (int i = 0; i < m_systemBookmarks.count(); ++i) {
        m_systemBookmarksIndexes.insert(m_systemBookmarks[i].url, i);
    }
}

void PlacesItemModel::clear() {
    m_bookmarkedItems.clear();
    KStandardItemModel::clear();
}

void PlacesItemModel::proceedWithTearDown()
{
    Q_ASSERT(m_deviceToTearDown);

    connect(m_deviceToTearDown, &Solid::StorageAccess::teardownDone,
            this, &PlacesItemModel::slotStorageTearDownDone);
    m_deviceToTearDown->teardown();
}

void PlacesItemModel::initializeAvailableDevices()
{
    QString predicate(QStringLiteral("[[[[ StorageVolume.ignored == false AND [ StorageVolume.usage == 'FileSystem' OR StorageVolume.usage == 'Encrypted' ]]"
        " OR "
        "[ IS StorageAccess AND StorageDrive.driveType == 'Floppy' ]]"
        " OR "
        "OpticalDisc.availableContent & 'Audio' ]"
        " OR "
        "StorageAccess.ignored == false ]"));


    if (KProtocolInfo::isKnownProtocol(QStringLiteral("mtp"))) {
        predicate.prepend("[");
        predicate.append(" OR PortableMediaPlayer.supportedProtocols == 'mtp']");
    }

    m_predicate = Solid::Predicate::fromString(predicate);
    Q_ASSERT(m_predicate.isValid());

    Solid::DeviceNotifier* notifier = Solid::DeviceNotifier::instance();
    connect(notifier, &Solid::DeviceNotifier::deviceAdded,   this, &PlacesItemModel::slotDeviceAdded);
    connect(notifier, &Solid::DeviceNotifier::deviceRemoved, this, &PlacesItemModel::slotDeviceRemoved);

    const QList<Solid::Device>& deviceList = Solid::Device::listFromQuery(m_predicate);
    foreach (const Solid::Device& device, deviceList) {
        m_availableDevices << device.udi();
    }
}

int PlacesItemModel::bookmarkIndex(int index) const
{
    int bookmarkIndex = 0;
    int modelIndex = 0;
    while (bookmarkIndex < m_bookmarkedItems.count()) {
        if (!m_bookmarkedItems[bookmarkIndex]) {
            if (modelIndex == index) {
                break;
            }
            ++modelIndex;
        }
        ++bookmarkIndex;
    }

    return bookmarkIndex >= m_bookmarkedItems.count() ? -1 : bookmarkIndex;
}

void PlacesItemModel::hideItem(int index)
{
    PlacesItem* shownItem = placesItem(index);
    if (!shownItem) {
        return;
    }

    shownItem->setHidden(true);
    if (m_hiddenItemsShown) {
        // Removing items from the model is not allowed if all hidden
        // items should be shown.
        return;
    }

    const int newIndex = bookmarkIndex(index);
    if (newIndex >= 0) {
        const KBookmark hiddenBookmark = shownItem->bookmark();
        PlacesItem* hiddenItem = new PlacesItem(hiddenBookmark);

        const PlacesItem* previousItem = placesItem(index - 1);
        KBookmark previousBookmark;
        if (previousItem) {
            previousBookmark = previousItem->bookmark();
        }

        const bool updateBookmark = (m_bookmarkManager->root().indexOf(hiddenBookmark) >= 0);
        removeItem(index);

        if (updateBookmark) {
            // removeItem() also removed the bookmark from m_bookmarkManager in
            // PlacesItemModel::onItemRemoved(). However for hidden items the
            // bookmark should still be remembered, so readd it again:
            m_bookmarkManager->root().addBookmark(hiddenBookmark);
            m_bookmarkManager->root().moveBookmark(hiddenBookmark, previousBookmark);
        }

        m_bookmarkedItems.insert(newIndex, hiddenItem);
    }
}

QString PlacesItemModel::internalMimeType() const
{
    return "application/x-dolphinplacesmodel-" +
            QString::number((qptrdiff)this);
}

int PlacesItemModel::groupedDropIndex(int index, const PlacesItem* item) const
{
    Q_ASSERT(item);

    int dropIndex = index;
    const PlacesItem::GroupType type = item->groupType();

    const int itemCount = count();
    if (index < 0) {
        dropIndex = itemCount;
    }

    // Search nearest previous item with the same group
    int previousIndex = -1;
    for (int i = dropIndex - 1; i >= 0; --i) {
        if (placesItem(i)->groupType() == type) {
            previousIndex = i;
            break;
        }
    }

    // Search nearest next item with the same group
    int nextIndex = -1;
    for (int i = dropIndex; i < count(); ++i) {
        if (placesItem(i)->groupType() == type) {
            nextIndex = i;
            break;
        }
    }

    // Adjust the drop-index to be inserted to the
    // nearest item with the same group.
    if (previousIndex >= 0 && nextIndex >= 0) {
        dropIndex = (dropIndex - previousIndex < nextIndex - dropIndex) ?
                    previousIndex + 1 : nextIndex;
    } else if (previousIndex >= 0) {
        dropIndex = previousIndex + 1;
    } else if (nextIndex >= 0) {
        dropIndex = nextIndex;
    }

    return dropIndex;
}

bool PlacesItemModel::equalBookmarkIdentifiers(const KBookmark& b1, const KBookmark& b2)
{
    const QString udi1 = b1.metaDataItem(QStringLiteral("UDI"));
    const QString udi2 = b2.metaDataItem(QStringLiteral("UDI"));
    if (!udi1.isEmpty() && !udi2.isEmpty()) {
        return udi1 == udi2;
    } else {
        return b1.metaDataItem(QStringLiteral("ID")) == b2.metaDataItem(QStringLiteral("ID"));
    }
}

QUrl PlacesItemModel::createTimelineUrl(const QUrl& url)
{
    // TODO: Clarify with the Baloo-team whether it makes sense
    // provide default-timeline-URLs like 'yesterday', 'this month'
    // and 'last month'.
    QUrl timelineUrl;

    const QString path = url.toDisplayString(QUrl::PreferLocalFile);
    if (path.endsWith(QLatin1String("yesterday"))) {
        const QDate date = QDate::currentDate().addDays(-1);
        const int year = date.year();
        const int month = date.month();
        const int day = date.day();
        timelineUrl = QUrl("timeline:/" + timelineDateString(year, month) +
              '/' + timelineDateString(year, month, day));
    } else if (path.endsWith(QLatin1String("thismonth"))) {
        const QDate date = QDate::currentDate();
        timelineUrl = QUrl("timeline:/" + timelineDateString(date.year(), date.month()));
    } else if (path.endsWith(QLatin1String("lastmonth"))) {
        const QDate date = QDate::currentDate().addMonths(-1);
        timelineUrl = QUrl("timeline:/" + timelineDateString(date.year(), date.month()));
    } else {
        Q_ASSERT(path.endsWith(QLatin1String("today")));
        timelineUrl = url;
    }

    return timelineUrl;
}

QString PlacesItemModel::timelineDateString(int year, int month, int day)
{
    QString date = QString::number(year) + '-';
    if (month < 10) {
        date += '0';
    }
    date += QString::number(month);

    if (day >= 1) {
        date += '-';
        if (day < 10) {
            date += '0';
        }
        date += QString::number(day);
    }

    return date;
}

QUrl PlacesItemModel::createSearchUrl(const QUrl& url)
{
    QUrl searchUrl;

#ifdef HAVE_BALOO
    const QString path = url.toDisplayString(QUrl::PreferLocalFile);
    if (path.endsWith(QLatin1String("documents"))) {
        searchUrl = searchUrlForType(QStringLiteral("Document"));
    } else if (path.endsWith(QLatin1String("images"))) {
        searchUrl = searchUrlForType(QStringLiteral("Image"));
    } else if (path.endsWith(QLatin1String("audio"))) {
        searchUrl = searchUrlForType(QStringLiteral("Audio"));
    } else if (path.endsWith(QLatin1String("videos"))) {
        searchUrl = searchUrlForType(QStringLiteral("Video"));
    } else {
        Q_ASSERT(false);
    }
#else
    Q_UNUSED(url);
#endif

    return searchUrl;
}

#ifdef HAVE_BALOO
QUrl PlacesItemModel::searchUrlForType(const QString& type)
{
    Baloo::Query query;
    query.addType(type);

    return query.toSearchUrl();
}
#endif

#ifdef PLACESITEMMODEL_DEBUG
void PlacesItemModel::showModelState()
{
    qCDebug(DolphinDebug) << "=================================";
    qCDebug(DolphinDebug) << "Model:";
    qCDebug(DolphinDebug) << "hidden-index model-index   text";
    int modelIndex = 0;
    for (int i = 0; i < m_bookmarkedItems.count(); ++i) {
        if (m_bookmarkedItems[i]) {
            qCDebug(DolphinDebug) <<  i << "(Hidden)    " << "             " << m_bookmarkedItems[i]->dataValue("text").toString();
        } else {
            if (item(modelIndex)) {
                qCDebug(DolphinDebug) <<  i << "          " << modelIndex << "           " << item(modelIndex)->dataValue("text").toString();
            } else {
                qCDebug(DolphinDebug) <<  i << "          " << modelIndex << "           " << "(not available yet)";
            }
            ++modelIndex;
        }
    }

    qCDebug(DolphinDebug);
    qCDebug(DolphinDebug) << "Bookmarks:";

    int bookmarkIndex = 0;
    KBookmarkGroup root = m_bookmarkManager->root();
    KBookmark bookmark = root.first();
    while (!bookmark.isNull()) {
        const QString udi = bookmark.metaDataItem("UDI");
        const QString text = udi.isEmpty() ? bookmark.text() : udi;
        if (bookmark.metaDataItem("IsHidden") == QLatin1String("true")) {
            qCDebug(DolphinDebug) << bookmarkIndex << "(Hidden)" << text;
        } else {
            qCDebug(DolphinDebug) << bookmarkIndex << "        " << text;
        }

        bookmark = root.next(bookmark);
        ++bookmarkIndex;
    }
}
#endif

