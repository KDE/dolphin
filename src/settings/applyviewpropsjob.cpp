/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 *
 * The code is based on kdelibs/kio/directorysizejob:
 * SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "applyviewpropsjob.h"

#include "views/viewproperties.h"
#include <KIO/ListJob>

ApplyViewPropsJob::ApplyViewPropsJob(const QUrl &dir, const ViewProperties &viewProps)
    : KIO::Job()
    , m_viewProps(nullptr)
    , m_progress(0)
    , m_dir(dir)
{
    m_viewProps = new ViewProperties(dir);
    m_viewProps->setViewMode(viewProps.viewMode());
    m_viewProps->setPreviewsShown(viewProps.previewsShown());
    m_viewProps->setHiddenFilesShown(viewProps.hiddenFilesShown());
    m_viewProps->setSortRole(viewProps.sortRole());
    m_viewProps->setSortOrder(viewProps.sortOrder());

    KIO::ListJob *listJob = KIO::listRecursive(dir, KIO::HideProgressInfo);
    connect(listJob, &KIO::ListJob::entries, this, &ApplyViewPropsJob::slotEntries);
    addSubjob(listJob);
}

ApplyViewPropsJob::~ApplyViewPropsJob()
{
    delete m_viewProps; // the properties are written by the destructor
    m_viewProps = nullptr;
}

void ApplyViewPropsJob::slotEntries(KIO::Job *, const KIO::UDSEntryList &list)
{
    for (const KIO::UDSEntry &entry : list) {
        const QString name = entry.stringValue(KIO::UDSEntry::UDS_NAME);
        if (name != QLatin1Char('.') && name != QLatin1String("..") && entry.isDir()) {
            ++m_progress;

            QUrl url(m_dir);
            url = url.adjusted(QUrl::StripTrailingSlash);
            url.setPath(url.path() + QLatin1Char('/') + name);

            Q_ASSERT(m_viewProps);

            ViewProperties props(url);
            props.setDirProperties(*m_viewProps);
        }
    }
}

void ApplyViewPropsJob::slotResult(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
    }
    emitResult();
}

#include "moc_applyviewpropsjob.cpp"
