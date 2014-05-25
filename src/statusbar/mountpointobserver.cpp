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

#include "mountpointobserver.h"
#include "mountpointobservercache.h"

MountPointObserver::MountPointObserver(const QString& mountPoint, QObject* parent) :
    QObject(parent),
    m_mountPoint(mountPoint),
    m_referenceCount(0),
    m_spaceInfo(KDiskFreeSpaceInfo::freeSpaceInfo(mountPoint))
{
}

MountPointObserver* MountPointObserver::observerForPath(const QString& path)
{
    MountPointObserver* observer = MountPointObserverCache::instance()->observerForPath(path);
    return observer;
}

void MountPointObserver::update()
{
    if (m_referenceCount == 0) {
        delete this;
    } else {
        const KDiskFreeSpaceInfo spaceInfo = KDiskFreeSpaceInfo::freeSpaceInfo(m_mountPoint);
        if (spaceInfo.size() != m_spaceInfo.size() || spaceInfo.available() != m_spaceInfo.available()) {
            m_spaceInfo = spaceInfo;
            emit spaceInfoChanged();
        }
    }
}
