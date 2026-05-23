/*
 * SPDX-FileCopyrightText: 2014 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mountpointobserver.h"

#include "mountpointobservercache.h"

MountPointObserver::MountPointObserver(const QUrl &url, QObject *parent)
    : QObject(parent)
    , m_url(url)
    , m_referenceCount(0)
{
}

MountPointObserver::~MountPointObserver()
{
    if (m_pendingJob) {
        m_pendingJob->kill(KJob::Quietly);
    }
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
        return;
    }
    if (m_pendingJob) {
        m_pendingJob->kill(KJob::Quietly);
    }
    m_pendingJob = KIO::fileSystemFreeSpace(m_url);
    connect(m_pendingJob, &KJob::result, this, &MountPointObserver::freeSpaceResult);
}

void MountPointObserver::freeSpaceResult(KJob *job)
{
    if (!job->error()) {
        KIO::FileSystemFreeSpaceJob *freeSpaceJob = qobject_cast<KIO::FileSystemFreeSpaceJob *>(job);
        Q_ASSERT(freeSpaceJob);
        Q_EMIT spaceInfoChanged(freeSpaceJob->size(), freeSpaceJob->availableSize());
    } else {
        Q_EMIT spaceInfoChanged(0, 0);
    }
}

#include "moc_mountpointobserver.cpp"
