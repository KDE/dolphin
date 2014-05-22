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

#include <KUrl>

SpaceInfoObserver::SpaceInfoObserver(const KUrl& url, QObject* parent) :
    QObject(parent),
    m_mountPointObserver(0)
{
    if (url.isLocalFile()) {
        m_mountPointObserver = MountPointObserver::observerForPath(url.toLocalFile());
        m_mountPointObserver->ref();
        connect(m_mountPointObserver, SIGNAL(spaceInfoChanged()), this, SIGNAL(valuesChanged()));
    }
}

SpaceInfoObserver::~SpaceInfoObserver()
{
    if (m_mountPointObserver) {
        m_mountPointObserver->deref();
        m_mountPointObserver = 0;
    }
}

quint64 SpaceInfoObserver::size() const
{
    if (m_mountPointObserver && m_mountPointObserver->spaceInfo().isValid()) {
        return m_mountPointObserver->spaceInfo().size();
    } else {
        return 0;
    }
}

quint64 SpaceInfoObserver::available() const
{
    if (m_mountPointObserver && m_mountPointObserver->spaceInfo().isValid()) {
        return m_mountPointObserver->spaceInfo().available();
    } else {
        return 0;
    }
}

void SpaceInfoObserver::setUrl(const KUrl& url)
{
    if (url.isLocalFile()) {
        MountPointObserver* newObserver = MountPointObserver::observerForPath(url.toLocalFile());
        if (newObserver != m_mountPointObserver) {
            if (m_mountPointObserver) {
                disconnect(m_mountPointObserver, SIGNAL(spaceInfoChanged()), this, SIGNAL(valuesChanged()));
                m_mountPointObserver->deref();
                m_mountPointObserver = 0;
            }

            m_mountPointObserver = newObserver;
            m_mountPointObserver->ref();
            connect(m_mountPointObserver, SIGNAL(spaceInfoChanged()), this, SIGNAL(valuesChanged()));

            emit valuesChanged();
        }
    } else {
        if (m_mountPointObserver) {
            disconnect(m_mountPointObserver, SIGNAL(spaceInfoChanged()), this, SIGNAL(valuesChanged()));
            m_mountPointObserver->deref();
            m_mountPointObserver = 0;

            emit valuesChanged();
        }
    }
}
