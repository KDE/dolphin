/*
 * SPDX-FileCopyrightText: 2014 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mountpointobserver.h"

#include "mountpointobservercache.h"

#include <KIO/FileSystemFreeSpaceJob>

MountPointObserver::MountPointObserver(const QUrl &url, QObject *parent)
    : QObject(parent)
    , m_url(url)
    , m_referenceCount(0)
{
}

MountPointObserver *MountPointObserver::observerForUrl(const QUrl &url)
{
    MountPointObserver *observer = MountPointObserverCache::instance()->observerForUrl(url);
    return observer;
}

void MountPointObserver::update()
{
    if (m_referenceCount == 0) {
        delete this;
    } else {
        KIO::FileSystemFreeSpaceJob *job = KIO::fileSystemFreeSpace(m_url);
        connect(job, &KJob::result, this, [this, job]() {
            if (!job->error()) {
                Q_EMIT spaceInfoChanged(job->size(), job->availableSize());
            } else {
                Q_EMIT spaceInfoChanged(0, 0);
            }
        });
    }
}

#include "moc_mountpointobserver.cpp"
