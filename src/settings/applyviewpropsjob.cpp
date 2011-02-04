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

ApplyViewPropsJob::ApplyViewPropsJob(const KUrl& dir,
                                     const ViewProperties& viewProps) :
    KIO::Job(),
    m_viewProps(0),
    m_progress(0),
    m_dir(dir)
{
    m_viewProps = new ViewProperties(dir);
    m_viewProps->setViewMode(viewProps.viewMode());
    m_viewProps->setShowPreview(viewProps.showPreview());
    m_viewProps->setShowHiddenFiles(viewProps.showHiddenFiles());
    m_viewProps->setSorting(viewProps.sorting());
    m_viewProps->setSortOrder(viewProps.sortOrder());

    KIO::ListJob* listJob = KIO::listRecursive(dir, KIO::HideProgressInfo);
    connect(listJob, SIGNAL(entries(KIO::Job*, const KIO::UDSEntryList&)),
            SLOT(slotEntries(KIO::Job*, const KIO::UDSEntryList&)));
    addSubjob(listJob);
}

ApplyViewPropsJob::~ApplyViewPropsJob()
{
    delete m_viewProps;  // the properties are written by the destructor
    m_viewProps = 0;
}

void ApplyViewPropsJob::slotEntries(KIO::Job*, const KIO::UDSEntryList& list)
{
    KIO::UDSEntryList::ConstIterator it = list.begin();
    const KIO::UDSEntryList::ConstIterator end = list.end();
    foreach(const KIO::UDSEntry& entry, list) {
        const QString name = entry.stringValue(KIO::UDSEntry::UDS_NAME);
        if ((name != ".") && (name != "..") && entry.isDir()) {
            ++m_progress;

            KUrl url(m_dir);
            url.addPath(name);

            Q_ASSERT(m_viewProps != 0);

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

#include "applyviewpropsjob.moc"
