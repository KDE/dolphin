/*  This file is part of the KDE project
    Copyright (C) 2007 Kevin Ottens <ervin@kde.org>

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

#include "kfileplacesitem_p.h"

KFilePlacesItem::KFilePlacesItem()
    : m_isDevice(false)
{
}

KFilePlacesItem::~KFilePlacesItem()
{
}

bool KFilePlacesItem::isDevice() const
{
    return m_isDevice;
}

QString KFilePlacesItem::bookmarkAddress() const
{
    return m_bookmarkAddress;
}

void KFilePlacesItem::setBookmarkAddress(const QString &address)
{
    m_bookmarkAddress = address;
}

QPersistentModelIndex KFilePlacesItem::deviceIndex() const
{
    return m_deviceIndex;
}

void KFilePlacesItem::setDeviceIndex(const QPersistentModelIndex &index)
{
    m_deviceIndex = index;
    m_isDevice = index.isValid();
}

