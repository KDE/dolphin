/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on KFilePlacesItem from kdelibs:                                *
 *   Copyright (C) 2007 Kevin Ottens <ervin@kde.org>                       *
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

#include "placesitem.h"

#include <KBookmark>
#include <KIcon>
#include <KLocale>
#include "placesitemstorageaccesslistener.h"
#include <Solid/Block>

PlacesItem::PlacesItem(PlacesItem* parent) :
    KStandardItem(parent),
    m_device(),
    m_access(),
    m_volume(),
    m_disc(),
    m_accessListener(0)
{
}

PlacesItem::PlacesItem(const KBookmark& bookmark, PlacesItem* parent) :
    KStandardItem(parent),
    m_device(),
    m_access(),
    m_volume(),
    m_disc(),
    m_accessListener(0)
{
    setHidden(bookmark.metaDataItem("IsHidden") == QLatin1String("true"));

    const QString udi = bookmark.metaDataItem("UDI");
    if (udi.isEmpty()) {
        setIcon(bookmark.icon());
        setText(bookmark.text());
        setUrl(bookmark.url());
        setDataValue("address", bookmark.address());
        setGroup(i18nc("@item", "Places"));
    } else {
        initializeDevice(udi);
    }
}

PlacesItem::PlacesItem(const QString& udi, PlacesItem* parent) :
    KStandardItem(parent),
    m_device(),
    m_access(),
    m_volume(),
    m_disc(),
    m_accessListener(0)
{
    initializeDevice(udi);
}

PlacesItem::PlacesItem(const PlacesItem& item) :
    KStandardItem(item),
    m_device(),
    m_access(),
    m_volume(),
    m_disc(),
    m_accessListener(0)
{
}

PlacesItem::~PlacesItem()
{
    delete m_accessListener;
    m_accessListener = 0;
}

void PlacesItem::setUrl(const KUrl& url)
{
    setDataValue("url", url);
}

KUrl PlacesItem::url() const
{
    return dataValue("url").value<KUrl>();
}

void PlacesItem::setUdi(const QString& udi)
{
    setDataValue("udi", udi);
}

QString PlacesItem::udi() const
{
    return dataValue("udi").toString();
}

void PlacesItem::setHidden(bool hidden)
{
    setDataValue("isHidden", hidden);
}

bool PlacesItem::isHidden() const
{
    return dataValue("isHidden").toBool();
}

Solid::Device PlacesItem::device() const
{
    return m_device;
}

void PlacesItem::initializeDevice(const QString& udi)
{
    m_device = Solid::Device(udi);
    if (!m_device.isValid()) {
        return;
    }

    m_access = m_device.as<Solid::StorageAccess>();
    m_volume = m_device.as<Solid::StorageVolume>();
    m_disc = m_device.as<Solid::OpticalDisc>();

    setText(m_device.description());
    setIcon(m_device.icon());
    setIconOverlays(m_device.emblems());
    setUdi(udi);
    setGroup(i18nc("@item", "Devices"));

    if (m_access) {
        setUrl(m_access->filePath());

        // The access listener takes care to call PlacesItem::onAccessibilityChanged()
        // in case if the accessibility of m_access has been changed.
        Q_ASSERT(!m_accessListener);
        m_accessListener = new PlacesItemStorageAccessListener(this);
    } else if (m_disc && (m_disc->availableContent() & Solid::OpticalDisc::Audio) != 0) {
        const QString device = m_device.as<Solid::Block>()->device();
        setUrl(QString("audiocd:/?device=%1").arg(device));
    }
}

void PlacesItem::onAccessibilityChanged()
{
    setIconOverlays(m_device.emblems());
}

