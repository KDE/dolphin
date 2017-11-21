/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   The code is based on kdelibs/kio/directorysizejob.*                   *
 *  (C) 2006 by David Faure <faure@kde.org>                                *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "applyviewpropsjob.h"
#include <views/viewproperties.h>

ApplyViewPropsJob::ApplyViewPropsJob(const QUrl& dir,
                                     const ViewProperties& viewProps) :
    KIO::Job(),
    m_viewProps(nullptr),
    m_progress(0),
    m_dir(dir)
{
    m_viewProps = new ViewProperties(dir);
    m_viewProps->setViewMode(viewProps.viewMode());
    m_viewProps->setPreviewsShown(viewProps.previewsShown());
    m_viewProps->setHiddenFilesShown(viewProps.hiddenFilesShown());
    m_viewProps->setSortRole(viewProps.sortRole());
    m_viewProps->setSortOrder(viewProps.sortOrder());

    KIO::ListJob* listJob = KIO::listRecursive(dir, KIO::HideProgressInfo);
    connect(listJob, &KIO::ListJob::entries,
            this, &ApplyViewPropsJob::slotEntries);
    addSubjob(listJob);
}

ApplyViewPropsJob::~ApplyViewPropsJob()
{
    delete m_viewProps;  // the properties are written by the destructor
    m_viewProps = nullptr;
}

void ApplyViewPropsJob::slotEntries(KIO::Job*, const KIO::UDSEntryList& list)
{
    foreach (const KIO::UDSEntry& entry, list) {
        const QString name = entry.stringValue(KIO::UDSEntry::UDS_NAME);
        if (name != QLatin1String(".") && name != QLatin1String("..") && entry.isDir()) {
            ++m_progress;

            QUrl url(m_dir);
            url = url.adjusted(QUrl::StripTrailingSlash);
            url.setPath(url.path() + '/' + name);

            Q_ASSERT(m_viewProps);

            ViewProperties props(url);
            props.setDirProperties(*m_viewProps);
        }
    }
}

void ApplyViewPropsJob::slotResult(KJob* job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
    }
    emitResult();
}

