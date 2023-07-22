/*
 * SPDX-FileCopyrightText: 2014 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MOUNTPOINTOBSERVER_H
#define MOUNTPOINTOBSERVER_H

#include <KIO/Job>

#include <QObject>
#include <QUrl>

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

    explicit MountPointObserver(const QUrl &url, QObject *parent = nullptr);
    ~MountPointObserver() override
    {
    }

public:
    /**
     * Call this function to indicate that the caller intends to continue using this object. An
     * internal reference count is increased then. When the observer is not needed any more,
     * deref() should be called, which decreases the reference count again.
     */
    void ref()
    {
        ++m_referenceCount;
    }

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
     * Returns a MountPointObserver for the given \a url. If the caller intends to continue using
     * the returned object, it must call its ref() method.
     */
    static MountPointObserver *observerForUrl(const QUrl &url);

Q_SIGNALS:
    /**
     * This signal is emitted when the size has been retrieved.
     */
    void spaceInfoChanged(quint64 size, quint64 available);

public Q_SLOTS:
    /**
     * If this slot is invoked, MountPointObserver starts a new driveSize job
     * to get the drive's size.
     */
    void update();

private:
    const QUrl m_url;
    int m_referenceCount;

    friend class MountPointObserverCache;
};

#endif
