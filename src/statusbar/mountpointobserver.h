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

#ifndef MOUNTPOINTOBSERVER_H
#define MOUNTPOINTOBSERVER_H

#include <KDiskFreeSpaceInfo>

#include <QObject>

/**
 * A MountPointObserver can be used to determine the free space on a mount
 * point. It will then check the free space periodically, and emit the signal
 * spaceInfoChanged() if the return value of spaceInfo() has changed.
 *
 * Since multiple users which watch paths on the same mount point can share
 * a MountPointObserver, it is not possible to create a MountPointObserver
 * manually. Instead, the function observerForPath(QString&) should be called,
 * which returns either an existing or a newly created MountPointObserver for
 * the mount point where the path is mounted. observerForPath(QString&) looks
 * for a suitable MountPointObserver in an internal cache and creates new
 * MountPointObservers and adds them to the cache if necessary.
 *
 * Reference counting is used to keep track of the number of users, and to
 * decide when the object can be deleted. A user of this class should call
 * the method ref() when it starts using it, and deref() when it does not need
 * the MountPointObserver any more.
 *
 * The object will not be deleted immediately if the reference count reaches
 * zero. The object will only be destroyed when the next periodic update of
 * the free space information happens, and the reference count is still zero.
 * This approach makes it possible to re-use the object if a new user requests
 * the free space for the same mount point before the next update.
 */
class MountPointObserver : public QObject
{
    Q_OBJECT

    explicit MountPointObserver(const QString& mountPoint, QObject* parent = 0);
    virtual ~MountPointObserver() {}

public:
    /**
     * Obtains information about the available space on the observed mount point.
     */
    KDiskFreeSpaceInfo spaceInfo() const { return m_spaceInfo; }

    /**
     * Call this function to indicate that the caller intends to continue using this object. An
     * internal reference count is increased then. When the observer is not needed any more,
     * deref() should be called, which decreases the reference count again.
     */
    void ref() { ++m_referenceCount; }

    /**
     * This function can be used to indicate that the caller does not need this MountPointObserver
     * any more. Internally, a reference count is decreased. If the reference count is zero while
     * update() is called, the object deletes itself.
     */
    void deref()
    {
        --m_referenceCount;
        Q_ASSERT(m_referenceCount >= 0);
    }

    /**
     * Returns a MountPointObserver for the given \a path. If the caller intends to continue using
     * the returned object, it must call its ref() method.
     */
    static MountPointObserver* observerForPath(const QString& path);

signals:
    /**
     * This signal is emitted if the information that spaceInfo() will return has changed.
     */
    void spaceInfoChanged();

public slots:
    /**
     * If this slot is invoked, MountPointObserver checks if the available space on the observed
     * mount point has changed, and emits spaceInfoChanged() if that is the case.
     */
    void update();

private:
    const QString m_mountPoint;
    int m_referenceCount;
    KDiskFreeSpaceInfo m_spaceInfo;

    friend class MountPointObserverCache;
};

#endif
