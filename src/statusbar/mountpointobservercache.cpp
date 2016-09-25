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

#include "mountpointobservercache.h"

#include "mountpointobserver.h"

#include <KMountPoint>

#include <QTimer>

class MountPointObserverCacheSingleton
{
public:
    MountPointObserverCache instance;
};
Q_GLOBAL_STATIC(MountPointObserverCacheSingleton, s_MountPointObserverCache)


MountPointObserverCache::MountPointObserverCache() :
    m_observerForMountPoint(),
    m_mountPointForObserver(),
    m_updateTimer(0)
{
    m_updateTimer = new QTimer(this);
}

MountPointObserverCache::~MountPointObserverCache()
{
}

MountPointObserverCache* MountPointObserverCache::instance()
{
    return &s_MountPointObserverCache->instance;
}

MountPointObserver* MountPointObserverCache::observerForUrl(const QUrl& url)
{
    QUrl cachedObserverUrl;
    // If the url is a local path we can extract the root dir by checking the mount points.
    if (url.isLocalFile()) {
        // Try to share the observer with other paths that have the same mount point.
        KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByPath(url.toLocalFile());
        if (mountPoint) {
            cachedObserverUrl = QUrl::fromLocalFile(mountPoint->mountPoint());
        } else {
            // Even if determining the mount point failed, the observer might still
            // be able to retrieve information about the url.
            cachedObserverUrl = url;
        }
    } else {
        cachedObserverUrl = url;
    }

    MountPointObserver* observer = m_observerForMountPoint.value(cachedObserverUrl);
    if (!observer) {
        observer = new MountPointObserver(cachedObserverUrl, this);
        m_observerForMountPoint.insert(cachedObserverUrl, observer);
        m_mountPointForObserver.insert(observer, cachedObserverUrl);
        Q_ASSERT(m_observerForMountPoint.count() == m_mountPointForObserver.count());

        connect(observer, &MountPointObserver::destroyed, this, &MountPointObserverCache::slotObserverDestroyed);

        if (!m_updateTimer->isActive()) {
            m_updateTimer->start(10000);
        }

        connect(m_updateTimer, &QTimer::timeout, observer, &MountPointObserver::update);
    }

    return observer;
}

void MountPointObserverCache::slotObserverDestroyed(QObject* observer)
{
    Q_ASSERT(m_mountPointForObserver.contains(observer));
    const QUrl& url = m_mountPointForObserver.value(observer);
    Q_ASSERT(m_observerForMountPoint.contains(url));
    m_observerForMountPoint.remove(url);
    m_mountPointForObserver.remove(observer);

    Q_ASSERT(m_observerForMountPoint.count() == m_mountPointForObserver.count());

    if (m_mountPointForObserver.isEmpty()) {
        m_updateTimer->stop();
    }
}
