/***************************************************************************
 *   Copyright (C) 2014 by Frank Reininghaus <frank78ac@googlemail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "spaceinfoobserver.h"

#include "mountpointobserver.h"

#include <QUrl>

SpaceInfoObserver::SpaceInfoObserver(const QUrl& url, QObject* parent) :
    QObject(parent),
    m_mountPointObserver(nullptr),
    m_dataSize(0),
    m_dataAvailable(0)
{
    m_mountPointObserver = MountPointObserver::observerForUrl(url);
    m_mountPointObserver->ref();
    connect(m_mountPointObserver, &MountPointObserver::spaceInfoChanged, this, &SpaceInfoObserver::spaceInfoChanged);
    m_mountPointObserver->update();
}

SpaceInfoObserver::~SpaceInfoObserver()
{
    if (m_mountPointObserver) {
        m_mountPointObserver->deref();
        m_mountPointObserver = nullptr;
    }
}

quint64 SpaceInfoObserver::size() const
{
    return m_dataSize;
}

quint64 SpaceInfoObserver::available() const
{
    return m_dataAvailable;
}

void SpaceInfoObserver::setUrl(const QUrl& url)
{
    MountPointObserver* newObserver = MountPointObserver::observerForUrl(url);
    if (newObserver != m_mountPointObserver) {
        if (m_mountPointObserver) {
            disconnect(m_mountPointObserver, &MountPointObserver::spaceInfoChanged, this, &SpaceInfoObserver::spaceInfoChanged);
            m_mountPointObserver->deref();
            m_mountPointObserver = nullptr;
        }

        m_mountPointObserver = newObserver;
        m_mountPointObserver->ref();
        connect(m_mountPointObserver, &MountPointObserver::spaceInfoChanged, this, &SpaceInfoObserver::spaceInfoChanged);

        // If newObserver is cached it won't call update until the next timer update, 
        // so update the observer now.
        m_mountPointObserver->update();
    }
}

void SpaceInfoObserver::spaceInfoChanged(quint64 size, quint64 available)
{
    // Make sure that the size has actually changed
    if (m_dataSize != size || m_dataAvailable != available) {
        m_dataSize = size;
        m_dataAvailable = available;

        emit valuesChanged();
    }
}
