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

#include "dolphin_generalsettings.h"
#include "dolphindebug.h"
#include "dolphinplacesmodelsingleton.h"
#include "placesitem.h"
#include "placesitemsignalhandler.h"
#include "views/dolphinview.h"
#include "views/viewproperties.h"

#include <KAboutData>
#include <KLocalizedString>
#include <KUrlMimeData>
#include <Solid/DeviceNotifier>
#include <Solid/OpticalDrive>

#include <QAction>
#include <QIcon>
#include <QMimeData>
#include <QTimer>

namespace {
    static QList<QUrl> balooURLs = {
        QUrl(QStringLiteral("timeline:/today")),
        QUrl(QStringLiteral("timeline:/yesterday")),
        QUrl(QStringLiteral("timeline:/thismonth")),
        QUrl(QStringLiteral("timeline:/lastmonth")),
        QUrl(QStringLiteral("search:/documents")),
        QUrl(QStringLiteral("search:/images")),
        QUrl(QStringLiteral("search:/audio")),
        QUrl(QStringLiteral("search:/videos"))
    };
}

PlacesItemModel::PlacesItemModel(QObject* parent) :
    KStandardItemModel(parent),
    m_hiddenItemsShown(false),
    m_deviceToTearDown(nullptr),
    m_storageSetupInProgress(),
    m_sourceModel(DolphinPlacesModelSingleton::instance().placesModel())
{
    cleanupBookmarks();
    loadBookmarks();
    initializeDefaultViewProperties();

    connect(m_sourceModel, &KFilePlacesModel::rowsInserted, this, &PlacesItemModel::onSourceModelRowsInserted);
    connect(m_sourceModel, &KFilePlacesModel::rowsAboutToBeRemoved, this, &PlacesItemModel::onSourceModelRowsAboutToBeRemoved);
    connect(m_sourceModel, &KFilePlacesModel::dataChanged, this, &PlacesItemModel::onSourceModelDataChanged);
    connect(m_sourceModel, &KFilePlacesModel::rowsAboutToBeMoved, this, &PlacesItemModel::onSourceModelRowsAboutToBeMoved);
    connect(m_sourceModel, &KFilePlacesModel::rowsMoved, this, &PlacesItemModel::onSourceModelRowsMoved);
    connect(m_sourceModel, &KFilePlacesModel::groupHiddenChanged, this, &PlacesItemModel::onSourceModelGroupHiddenChanged);
}

PlacesItemModel::~PlacesItemModel()
{
}

void PlacesItemModel::createPlacesItem(const QString& text,
                                       const QUrl& url,
                                       const QString& iconName,
                                       int after)
{
    m_sourceModel->addPlace(text, url, iconName, {}, mapToSource(after));
}

PlacesItem* PlacesItemModel::placesItem(int index) const
{
    return dynamic_cast<PlacesItem*>(item(index));
}

int PlacesItemModel::hiddenCount() const
{
    return m_sourceModel->hiddenCount();
}

void PlacesItemModel::setHiddenItemsShown(bool show)
{
    if (m_hiddenItemsShown == show) {
        return;
    }

    m_hiddenItemsShown = show;

    if (show) {
        for (int r = 0, rMax = m_sourceModel->rowCount(); r < rMax; r++) {
            const QModelIndex index = m_sourceModel->index(r, 0);
            if (!m_sourceModel->isHidden(index)) {
                continue;
            }
            addItemFromSourceModel(index);
        }
    } else {
        for (int r = 0, rMax = m_sourceModel->rowCount(); r < rMax; r++) {
            const QModelIndex index = m_sourceModel->index(r, 0);
            if (m_sourceModel->isHidden(index)) {
                removeItemByIndex(index);
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
    return mapFromSource(m_sourceModel->closestItem(url));
}

// look for the correct position for the item based on source model
void PlacesItemModel::insertSortedItem(PlacesItem* item)
{
    if (!item) {
        return;
    }

    const KBookmark iBookmark = item->bookmark();
    const QString iBookmarkId = bookmarkId(iBookmark);
    QModelIndex sourceIndex;
    int pos = 0;

    for(int r = 0, rMax = m_sourceModel->rowCount(); r < rMax; r++) {
        sourceIndex = m_sourceModel->index(r, 0);
        const KBookmark sourceBookmark = m_sourceModel->bookmarkForIndex(sourceIndex);

        if (bookmarkId(sourceBookmark) == iBookmarkId) {
            break;
        }

        if (m_hiddenItemsShown || !m_sourceModel->isHidden(sourceIndex)) {
            pos++;
        }
    }

    m_indexMap.insert(pos, sourceIndex);
    insertItem(pos, item);
}

void PlacesItemModel::onItemInserted(int index)
{
    KStandardItemModel::onItemInserted(index);
#ifdef PLACESITEMMODEL_DEBUG
    qCDebug(DolphinDebug) << "Inserted item" << index;
    showModelState();
#endif
}

void PlacesItemModel::onItemRemoved(int index, KStandardItem* removedItem)
{
    m_indexMap.removeAt(index);

    KStandardItemModel::onItemRemoved(index, removedItem);
#ifdef PLACESITEMMODEL_DEBUG
    qCDebug(DolphinDebug) << "Removed item" << index;
    showModelState();
#endif
}

void PlacesItemModel::onItemChanged(int index, const QSet<QByteArray>& changedRoles)
{
    const QModelIndex sourceIndex = mapToSource(index);
    const PlacesItem *changedItem = placesItem(mapFromSource(sourceIndex));

    if (!changedItem || !sourceIndex.isValid()) {
        qWarning() << "invalid item changed signal";
        return;
    }
    if (changedRoles.contains("isHidden")) {
        if (m_sourceModel->isHidden(sourceIndex) != changedItem->isHidden()) {
            m_sourceModel->setPlaceHidden(sourceIndex, changedItem->isHidden());
        } else {
            m_sourceModel->refresh();
        }
    }
    KStandardItemModel::onItemChanged(index, changedRoles);
}

QAction* PlacesItemModel::ejectAction(int index) const
{
    const PlacesItem* item = placesItem(index);
    if (item && item->device().is<Solid::OpticalDisc>()) {
        return new QAction(QIcon::fromTheme(QStringLiteral("media-eject")), i18nc("@item", "Eject"), nullptr);
    }

    return nullptr;
}

QAction* PlacesItemModel::teardownAction(int index) const
{
    const PlacesItem* item = placesItem(index);
    if (!item) {
        return nullptr;
    }

    Solid::Device device = item->device();
    const bool providesTearDown = device.is<Solid::StorageAccess>() &&
                                  device.as<Solid::StorageAccess>()->isAccessible();
    if (!providesTearDown) {
        return nullptr;
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
        return new QAction(text, nullptr);
    }

    return new QAction(QIcon::fromTheme(iconName), text, nullptr);
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

        m_sourceModel->movePlace(oldIndex, index);
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

            createPlacesItem(text, url, KIO::iconNameForUrl(url), qMax(0, index - 1));
        }
    }
    // will save bookmark alteration and fix sort if that is broken by the drag/drop operation
    refresh();
}

void PlacesItemModel::addItemFromSourceModel(const QModelIndex &index)
{
    if (!m_hiddenItemsShown && m_sourceModel->isHidden(index)) {
        return;
    }

    const KBookmark bookmark = m_sourceModel->bookmarkForIndex(index);
    Q_ASSERT(!bookmark.isNull());
    PlacesItem *item = itemFromBookmark(bookmark);
    if (!item) {
        item = new PlacesItem(bookmark);
    }
    updateItem(item, index);
    insertSortedItem(item);

    if (m_sourceModel->isDevice(index)) {
        connect(item->signalHandler(), &PlacesItemSignalHandler::tearDownExternallyRequested,
                this, &PlacesItemModel::storageTearDownExternallyRequested);
    }
}

void PlacesItemModel::removeItemByIndex(const QModelIndex &sourceIndex)
{
    QString id = bookmarkId(m_sourceModel->bookmarkForIndex(sourceIndex));

    for (int i = 0, iMax = count(); i < iMax; ++i) {
        if (bookmarkId(placesItem(i)->bookmark()) == id) {
            removeItem(i);
            return;
        }
    }
}

QString PlacesItemModel::bookmarkId(const KBookmark &bookmark) const
{
    QString id = bookmark.metaDataItem(QStringLiteral("UDI"));
    if (id.isEmpty()) {
        id = bookmark.metaDataItem(QStringLiteral("ID"));
    }
    return id;
}

void PlacesItemModel::initializeDefaultViewProperties() const
{
    for(int i = 0, iMax = m_sourceModel->rowCount(); i < iMax; i++) {
        const QModelIndex index = m_sourceModel->index(i, 0);
        const PlacesItem *item = placesItem(mapFromSource(index));
        if (!item) {
            continue;
        }

        // Create default view-properties for all "Search For" and "Recently Saved" bookmarks
        // in case the user has not already created custom view-properties for a corresponding
        // query yet.
        const bool createDefaultViewProperties = item->isSearchOrTimelineUrl() && !GeneralSettings::self()->globalViewProps();
        if (createDefaultViewProperties) {
            const QUrl itemUrl = item->url();
            ViewProperties props(KFilePlacesModel::convertedUrl(itemUrl));
            if (!props.exist()) {
                const QString path = itemUrl.path();
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
                } else if (itemUrl.scheme() == QLatin1String("timeline")) {
                    props.setViewMode(DolphinView::DetailsView);
                    props.setVisibleRoles({"text", "modificationtime"});
                }
                props.save();
            }
        }
    }
}

void PlacesItemModel::updateItem(PlacesItem *item, const QModelIndex &index)
{
    item->setGroup(index.data(KFilePlacesModel::GroupRole).toString());
    item->setIcon(index.data(KFilePlacesModel::IconNameRole).toString());
    item->setGroupHidden(index.data(KFilePlacesModel::GroupHiddenRole).toBool());
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

void PlacesItemModel::onSourceModelRowsInserted(const QModelIndex &parent, int first, int last)
{
    for (int i = first; i <= last; i++) {
        const QModelIndex index = m_sourceModel->index(i, 0, parent);
        addItemFromSourceModel(index);
    }
}

void PlacesItemModel::onSourceModelRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    for(int r = first; r <= last; r++) {
        const QModelIndex index = m_sourceModel->index(r, 0, parent);
        int oldIndex = mapFromSource(index);
        if (oldIndex != -1) {
            removeItem(oldIndex);
        }
    }
}

void PlacesItemModel::onSourceModelRowsAboutToBeMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row)
{
    Q_UNUSED(destination);
    Q_UNUSED(row);

    for(int r = start; r <= end; r++) {
        const QModelIndex sourceIndex = m_sourceModel->index(r, 0, parent);
        // remove moved item
        removeItem(mapFromSource(sourceIndex));
    }
}

void PlacesItemModel::onSourceModelRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row)
{
    Q_UNUSED(destination);
    Q_UNUSED(parent);

    const int blockSize = (end - start) + 1;

    for (int r = start; r <= end; r++) {
        // insert the moved item in the new position
        const int targetRow = row + (start - r) - (r < row ? blockSize : 0);
        const QModelIndex targetIndex = m_sourceModel->index(targetRow, 0, destination);

        addItemFromSourceModel(targetIndex);
    }
}

void PlacesItemModel::onSourceModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(roles);

    for (int r = topLeft.row(); r <= bottomRight.row(); r++) {
        const QModelIndex sourceIndex = m_sourceModel->index(r, 0);
        const KBookmark bookmark = m_sourceModel->bookmarkForIndex(sourceIndex);
        PlacesItem *placeItem = itemFromBookmark(bookmark);

        if (placeItem && (!m_hiddenItemsShown && m_sourceModel->isHidden(sourceIndex))) {
            //hide item if it became invisible
            removeItem(index(placeItem));
            return;
        }

        if (!placeItem && (m_hiddenItemsShown || !m_sourceModel->isHidden(sourceIndex))) {
            //show item if it became visible
            addItemFromSourceModel(sourceIndex);
            return;
        }

        if (placeItem && !m_sourceModel->isDevice(sourceIndex)) {
            placeItem->setText(bookmark.text());
            placeItem->setIcon(sourceIndex.data(KFilePlacesModel::IconNameRole).toString());
            placeItem->setUrl(m_sourceModel->url(sourceIndex));
            placeItem->bookmark().setMetaDataItem(QStringLiteral("OnlyInApp"),
                                                  bookmark.metaDataItem(QStringLiteral("OnlyInApp")));
            // must update the bookmark object
            placeItem->setBookmark(bookmark);
        }
    }
}

void PlacesItemModel::onSourceModelGroupHiddenChanged(KFilePlacesModel::GroupType group, bool hidden)
{
    const auto groupIndexes = m_sourceModel->groupIndexes(group);
    for (const QModelIndex &sourceIndex : groupIndexes) {
        PlacesItem *item = placesItem(mapFromSource(sourceIndex));
        if (item) {
            item->setGroupHidden(hidden);
        }
    }
}

void PlacesItemModel::cleanupBookmarks()
{
    // KIO model now provides support for baloo urls, and because of that we
    // need to remove old URLs that were visible only in Dolphin to avoid duplication
    int row = 0;
    do {
        const QModelIndex sourceIndex = m_sourceModel->index(row, 0);
        const KBookmark bookmark = m_sourceModel->bookmarkForIndex(sourceIndex);
        const QUrl url = bookmark.url();
        const QString appName = bookmark.metaDataItem(QStringLiteral("OnlyInApp"));

        if ((appName == KAboutData::applicationData().componentName() ||
             appName == KAboutData::applicationData().componentName() + DolphinPlacesModelSingleton::applicationNameSuffix()) && balooURLs.contains(url)) {
            qCDebug(DolphinDebug) << "Removing old baloo url:" << url;
            m_sourceModel->removePlace(sourceIndex);
        } else {
            row++;
        }
    } while (row < m_sourceModel->rowCount());
}

void PlacesItemModel::loadBookmarks()
{
    for(int r = 0, rMax = m_sourceModel->rowCount(); r < rMax; r++) {
        const QModelIndex sourceIndex = m_sourceModel->index(r, 0);
        if (m_hiddenItemsShown || !m_sourceModel->isHidden(sourceIndex)) {
            addItemFromSourceModel(sourceIndex);
        }
    }

#ifdef PLACESITEMMODEL_DEBUG
    qCDebug(DolphinDebug) << "Loaded bookmarks";
    showModelState();
#endif
}

void PlacesItemModel::clear() {
    KStandardItemModel::clear();
}

void PlacesItemModel::proceedWithTearDown()
{
    Q_ASSERT(m_deviceToTearDown);

    connect(m_deviceToTearDown, &Solid::StorageAccess::teardownDone,
            this, &PlacesItemModel::slotStorageTearDownDone);
    m_deviceToTearDown->teardown();
}

void PlacesItemModel::deleteItem(int index)
{
    QModelIndex sourceIndex = mapToSource(index);
    Q_ASSERT(sourceIndex.isValid());
    m_sourceModel->removePlace(sourceIndex);
}

void PlacesItemModel::refresh()
{
    m_sourceModel->refresh();
}

void PlacesItemModel::hideItem(int index)
{
    PlacesItem* shownItem = placesItem(index);
    if (!shownItem) {
        return;
    }

    shownItem->setHidden(true);
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
    const QString group = item->group();

    const int itemCount = count();
    if (index < 0) {
        dropIndex = itemCount;
    }

    // Search nearest previous item with the same group
    int previousIndex = -1;
    for (int i = dropIndex - 1; i >= 0; --i) {
        if (placesItem(i)->group() == group) {
            previousIndex = i;
            break;
        }
    }

    // Search nearest next item with the same group
    int nextIndex = -1;
    for (int i = dropIndex; i < count(); ++i) {
        if (placesItem(i)->group() == group) {
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

int PlacesItemModel::mapFromSource(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return -1;
    }

    return m_indexMap.indexOf(index);
}

bool PlacesItemModel::isDir(int index) const
{
    Q_UNUSED(index);
    return true;
}

KFilePlacesModel::GroupType PlacesItemModel::groupType(int row) const
{
    return m_sourceModel->groupType(mapToSource(row));
}

bool PlacesItemModel::isGroupHidden(KFilePlacesModel::GroupType type) const
{
    return m_sourceModel->isGroupHidden(type);
}

void PlacesItemModel::setGroupHidden(KFilePlacesModel::GroupType type, bool hidden)
{
    return m_sourceModel->setGroupHidden(type, hidden);
}

QModelIndex PlacesItemModel::mapToSource(int row) const
{
    return m_indexMap.value(row);
}

PlacesItem *PlacesItemModel::itemFromBookmark(const KBookmark &bookmark) const
{
    const QString id = bookmarkId(bookmark);
    for (int i = 0, iMax = count(); i < iMax; i++) {
        PlacesItem *item = placesItem(i);
        const KBookmark itemBookmark = item->bookmark();
        if (bookmarkId(itemBookmark) == id) {
            return item;
        }
    }
    return nullptr;
}

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

