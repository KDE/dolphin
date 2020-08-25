/*
 * SPDX-FileCopyrightText: 2014 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
