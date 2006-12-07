/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *                                                                         *
 *   The code is based on kdelibs/kio/kio/directorysizejob.*               *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef APPLYVIEWPROPSJOB_H
#define APPLYVIEWPROPSJOB_H

#include <kio/job.h>
#include <kfileitem.h>
#include <kurl.h>

class ViewProperties;

/**
 * @brief Applies view properties recursively to directories.
 *
 * Usage:
 * \code
 * KJob* job = new ApplyViewPropsJob(dir, viewProps);
 * connect(job, SIGNAL(result(KJob*)),
 *          this, SLOT(slotResult(KJob*)));
 * \endcode
 *
 * To be able to show a progress of the operation, the following steps
 * are recommended:
 * - Use a DirectorySizeJob to count the number of directories.
 * - Use a timer to show the current count of directories by invoking
 *   DirectorySizeJob::totalSubdirs() until the result signal is emitted.
 * - Use the ApplyViewPropsJob.
 * - Use a timer to show the progress by invoking ApplyViwePropsJob::progress().
 *   In combination with the total directory count it is possible to show a
 *   progress bar now.
 */
class ApplyViewPropsJob : public KIO::Job
{
    Q_OBJECT

public:
    /**
     * @param dir       Directory where the view properties should be applied to
     *                  (including sub directories).
     * @param viewProps View properties for the directory \a dir including its
     *                  sub directories.
     */
    ApplyViewPropsJob(const KUrl& dir, const ViewProperties& viewProps);
    virtual ~ApplyViewPropsJob();
    int progress() const { return m_progress; }

private:
    void startNextJob(const KUrl & url, const ViewProperties& viewProps);

private slots:
    virtual void slotResult(KJob* job);
    void slotEntries(KIO::Job*, const KIO::UDSEntryList&);
    void processNextItem();

private:
    ViewProperties* m_viewProps;
    KFileItemList m_lstItems;
    int m_currentItem;
    int m_progress;
    KUrl m_dir;
};

#endif
