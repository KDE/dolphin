/*  This file is part of the KDE project
    Copyright (C) 2007 Kevin Ottens <ervin@kde.org>
    Copyright (C) 2007 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/
#include "kfileplacesmodel.h"
#include "kfileplacesitem_p.h"

#include <kglobal.h>
#include <kstandarddirs.h>
#include <kcomponentdata.h>
#include <kicon.h>

#include <kdevicelistmodel.h>
#include <kbookmarkmanager.h>
#include <kbookmark.h>

#include <solid/volume.h>

class KFilePlacesModel::Private
{
public:
    Private(KFilePlacesModel *self) : q(self), deviceModel(0), bookmarkManager(0) {}


    KFilePlacesModel *q;

    QList<KFilePlacesItem*> items;
    QMap<QString, QPersistentModelIndex> availableDevices;

    KDeviceListModel *deviceModel;
    KBookmarkManager *bookmarkManager;

    QVariant bookmarkData(const QString &address, int role) const;
    QVariant deviceData(const QPersistentModelIndex &index, int role) const;

    void _k_devicesInserted(const QModelIndex &parent, int start, int end);
    void _k_devicesRemoved(const QModelIndex &parent, int start, int end);
    void _k_reloadBookmarks();
};

KFilePlacesModel::KFilePlacesModel(QObject *parent)
    : QAbstractItemModel(parent), d(new Private(this))
{
    QString basePath = KGlobal::mainComponent().componentName();
    basePath.append("/bookmarks.xml");
    const QString file = KStandardDirs::locateLocal("data", basePath);

    d->bookmarkManager = KBookmarkManager::managerForFile(file, "dolphin", false);

    d->deviceModel = new KDeviceListModel("IS Volume", this);

    connect(d->deviceModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
            this, SLOT(_k_devicesInserted(const QModelIndex&, int, int)));
    connect(d->deviceModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)),
            this, SLOT(_k_devicesRemoved(const QModelIndex&, int, int)));
    connect(d->bookmarkManager, SIGNAL(changed(const QString&, const QString&)),
            this, SLOT(_k_reloadBookmarks()));

    d->_k_reloadBookmarks();
}

KFilePlacesModel::~KFilePlacesModel()
{
    delete d;
}

KUrl KFilePlacesModel::url(const QModelIndex &index) const
{
    return KUrl(data(index, UrlRole).toUrl());
}

bool KFilePlacesModel::mountNeeded(const QModelIndex &index) const
{
    return data(index, MountNeededRole).toBool();
}

KIcon KFilePlacesModel::icon(const QModelIndex &index) const
{
    return KIcon(data(index, Qt::DecorationRole).value<QIcon>());
}

QString KFilePlacesModel::text(const QModelIndex &index) const
{
    return data(index, Qt::DisplayRole).toString();
}

QVariant KFilePlacesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariant returnData;

    KFilePlacesItem *item = static_cast<KFilePlacesItem*>(index.internalPointer());

    if (item->isDevice()) {
        returnData = d->deviceData(item->deviceIndex(), role);
    } else {
        returnData = d->bookmarkData(item->bookmarkAddress(), role);
    }

    return returnData;
}

QModelIndex KFilePlacesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row<0 || column!=0 || row>=d->items.size())
        return QModelIndex();

    if (parent.isValid())
        return QModelIndex();

    return createIndex(row, column, d->items[row]);
}

QModelIndex KFilePlacesModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

int KFilePlacesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    else
        return d->items.size();
}

int KFilePlacesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    // We only know 1 piece of information for a particular entry
    return 1;
}

QModelIndex KFilePlacesModel::closestItem(const KUrl &url) const
{
    int foundRow = -1;
    int maxLength = 0;

    // Search the item which is equal to the URL or at least is a parent URL.
    // If there are more than one possible item URL candidates, choose the item
    // which covers the bigger range of the URL.
    for (int row = 0; row<d->items.size(); ++row) {
        KFilePlacesItem *item = d->items[row];
        KUrl itemUrl;

        if (item->isDevice()) {
            itemUrl = KUrl(d->deviceData(item->deviceIndex(), UrlRole).toUrl());
        } else {
            itemUrl = KUrl(d->bookmarkData(item->bookmarkAddress(), UrlRole).toUrl());
        }

        if (itemUrl.isParentOf(url)) {
            const int length = itemUrl.prettyUrl().length();
            if (length > maxLength) {
                foundRow = row;
                maxLength = length;
            }
        }
    }

    if (foundRow==-1)
        return QModelIndex();
    else
        return createIndex(foundRow, 0, d->items[foundRow]);
}

QVariant KFilePlacesModel::Private::bookmarkData(const QString &address, int role) const
{
    KBookmark bookmark = bookmarkManager->findByAddress(address);

    if (bookmark.isNull()) return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
        return bookmark.text();
    case Qt::DecorationRole:
        return KIcon(bookmark.icon());
    case UrlRole:
        return QUrl(bookmark.url());
    case MountNeededRole:
        return false;
    default:
        return QVariant();
    }
}

QVariant KFilePlacesModel::Private::deviceData(const QPersistentModelIndex &index, int role) const
{
    if (index.isValid()) {
        switch (role)
        {
        case UrlRole:
            return QUrl(KUrl(deviceModel->deviceForIndex(index).as<Solid::Volume>()->mountPoint()));
        case MountNeededRole:
            return !deviceModel->deviceForIndex(index).as<Solid::Volume>()->isMounted();
        default:
            return deviceModel->data(index, role);
        }
    } else {
        return QVariant();
    }
}

void KFilePlacesModel::Private::_k_devicesInserted(const QModelIndex &parent, int start, int end)
{
    for (int i = start; i<=end; ++i) {
        QModelIndex index = parent.child(i, 0);
        QString udi = deviceModel->deviceForIndex(index).udi();

        availableDevices[udi] = index;
    }

    _k_reloadBookmarks();
}

void KFilePlacesModel::Private::_k_devicesRemoved(const QModelIndex &parent, int start, int end)
{
    for (int i = start; i<=end; ++i) {
        QModelIndex index = parent.child(i, 0);
        // Can't find by UDI since the device is already invalid.
        availableDevices.remove(availableDevices.key(index));
    }

    _k_reloadBookmarks();
}

void KFilePlacesModel::Private::_k_reloadBookmarks()
{
    qDeleteAll(items);
    items.clear();
    q->reset();

    KBookmarkGroup root = bookmarkManager->root();
    KBookmark bookmark = root.first();
    QMap<QString, QPersistentModelIndex> devices = availableDevices;

    while (!bookmark.isNull()) {
        QString udi = bookmark.metaDataItem("UDI");
        QPersistentModelIndex index = devices.take(udi);

        if (udi.isEmpty() || index.isValid()) {
            q->beginInsertRows(QModelIndex(), items.size(), items.size());

            KFilePlacesItem *item = new KFilePlacesItem();
            item->setBookmarkAddress(bookmark.address());
            if (index.isValid()) {
                item->setDeviceIndex(index);
                // TODO: Update bookmark internal element
            }
            items << item;

            q->endInsertRows();
        }

        bookmark = root.next(bookmark);
    }

    // Add bookmarks for the remaining devices, they were previously unknown
    foreach (QString udi, devices.keys()) {
        bookmark = root.createNewSeparator();
        bookmark.setMetaDataItem("UDI", udi);

        q->beginInsertRows(QModelIndex(), items.size(), items.size());

        KFilePlacesItem *item = new KFilePlacesItem();
        item->setBookmarkAddress(bookmark.address());
        item->setDeviceIndex(devices[udi]);
        // TODO: Update bookmark internal element
        items << item;

        q->endInsertRows();
    }
}

#include "kfileplacesmodel.moc"
