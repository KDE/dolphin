/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef PLACESITEM_H
#define PLACESITEM_H

#include <KBookmark>
#include <kitemviews/kstandarditem.h>
#include <KUrl>
#include <QPointer>
#include <Solid/Device>
#include <Solid/OpticalDisc>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>
#include <Solid/PortableMediaPlayer>

class KDirLister;
class PlacesItemSignalHandler;

/**
 * @brief Extends KStandardItem by places-specific properties.
 */
class PlacesItem : public KStandardItem
{

public:
    enum GroupType
    {
        PlacesType,
        SearchForType,
        RecentlyAccessedType,
        DevicesType
    };

    explicit PlacesItem(const KBookmark& bookmark, PlacesItem* parent = 0);
    virtual ~PlacesItem();

    void setUrl(const KUrl& url);
    KUrl url() const;

    void setUdi(const QString& udi);
    QString udi() const;

    void setHidden(bool hidden);
    bool isHidden() const;

    void setSystemItem(bool isSystemItem);
    bool isSystemItem() const;

    Solid::Device device() const;

    void setBookmark(const KBookmark& bookmark);
    KBookmark bookmark() const;

    GroupType groupType() const;

    bool storageSetupNeeded() const;

    static KBookmark createBookmark(KBookmarkManager* manager,
                                    const QString& text,
                                    const KUrl& url,
                                    const QString& iconName);
    static KBookmark createDeviceBookmark(KBookmarkManager* manager,
                                          const QString& udi);

protected:
    virtual void onDataValueChanged(const QByteArray& role,
                                    const QVariant& current,
                                    const QVariant& previous);

    virtual void onDataChanged(const QHash<QByteArray, QVariant>& current,
                               const QHash<QByteArray, QVariant>& previous);

private:
    PlacesItem(const PlacesItem& item);

    void initializeDevice(const QString& udi);

    /**
     * Is invoked if the accessibility of the storage access
     * m_access has been changed and updates the emblem.
     */
    void onAccessibilityChanged();

    /**
     * Is invoked if the listing of the trash has been completed.
     * Updates the state of the trash-icon to be empty or full.
     */
    void onTrashDirListerCompleted();

    /**
     * Applies the data-value from the role to m_bookmark.
     */
    void updateBookmarkForRole(const QByteArray& role);

    static QString generateNewId();

private:
    Solid::Device m_device;
    QPointer<Solid::StorageAccess> m_access;
    QPointer<Solid::StorageVolume> m_volume;
    QPointer<Solid::OpticalDisc> m_disc;
    QPointer<Solid::PortableMediaPlayer> m_mtp;
    QPointer<PlacesItemSignalHandler> m_signalHandler;
    QPointer<KDirLister> m_trashDirLister;
    KBookmark m_bookmark;

    friend class PlacesItemSignalHandler; // Calls onAccessibilityChanged()
};

#endif


