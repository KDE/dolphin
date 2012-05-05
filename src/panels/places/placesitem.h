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

#include <kitemviews/kstandarditem.h>
#include <KUrl>
#include <QPointer>
#include <Solid/Device>
#include <Solid/OpticalDisc>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>

class KBookmark;

/**
 * @brief Extends KStandardItem by places-specific properties.
 */
class PlacesItem : public KStandardItem
{

public:
    explicit PlacesItem(PlacesItem* parent = 0);
    PlacesItem(const KBookmark& bookmark,
               const QString& udi,
               PlacesItem* parent = 0);
    PlacesItem(const PlacesItem& item);
    virtual ~PlacesItem();

    void setUrl(const KUrl& url);
    KUrl url() const;

    void setHidden(bool hidden);
    bool isHidden() const;

private:
    Solid::Device m_device;
    QPointer<Solid::StorageAccess> m_access;
    QPointer<Solid::StorageVolume> m_volume;
    QPointer<Solid::OpticalDisc> m_disc;
};

#endif


