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

#ifndef MOUNTPOINTOBSERVERCACHE_H
#define MOUNTPOINTOBSERVERCACHE_H

#include <QHash>
#include <QObject>

class MountPointObserver;
class QTimer;

class MountPointObserverCache : public QObject
{
    Q_OBJECT

    MountPointObserverCache();
    ~MountPointObserverCache() override;

public:
    static MountPointObserverCache* instance();

    /**
     * Returns a MountPointObserver for the given \a url. A new observer is created if necessary.
     */
    MountPointObserver* observerForUrl(const QUrl& url);

private slots:
    /**
     * Removes the given \a observer from the cache.
     */
    void slotObserverDestroyed(QObject* observer);

private:
    QHash<QUrl, MountPointObserver*> m_observerForMountPoint;
    QHash<QObject*, QUrl> m_mountPointForObserver;
    QTimer* m_updateTimer;

    friend class MountPointObserverCacheSingleton;
};

#endif
