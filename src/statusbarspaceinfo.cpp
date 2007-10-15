/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at) and              *
 *   and Patrice Tremblay                                                  *
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

#include "statusbarspaceinfo.h"

#include <kdiskfreespace.h>
#include <kmountpoint.h>
#include <klocale.h>
#include <kio/job.h>

#include <QTimer>
#include <QKeyEvent>

StatusBarSpaceInfo::StatusBarSpaceInfo(QWidget* parent) :
    QProgressBar(parent),
    m_text()
{
    setMaximumWidth(200);

    // Update the space information each 10 seconds. Polling is useful
    // here, as files can be deleted/added outside the scope of Dolphin.
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(refresh()));
    timer->start(10000);
}

StatusBarSpaceInfo::~StatusBarSpaceInfo()
{
}

void StatusBarSpaceInfo::setUrl(const KUrl& url)
{
    m_url = url;
    refresh();
    QTimer::singleShot(300, this, SLOT(update()));
}

QString StatusBarSpaceInfo::text() const
{
    return m_text;
}

void StatusBarSpaceInfo::slotFoundMountPoint(const QString& mountPoint,
                                             quint64 kBSize,
                                             quint64 kBUsed,
                                             quint64 kBAvailable)
{
    Q_UNUSED(kBUsed);
    Q_UNUSED(mountPoint);

    m_text = i18nc("@info:status", "%1 free", KIO::convertSizeFromKiB(kBAvailable));

    setMinimum(0);
    setMaximum(kBAvailable);
    setValue(kBUsed);
}

void StatusBarSpaceInfo::refresh()
{
    // KDiskFreeSpace is for local paths only
    if (!m_url.isLocalFile()) {
        return;
    }

    m_text = i18nc("@info:status", "Getting size...");
    setMinimum(0);
    setMaximum(0);

    KMountPoint::Ptr mp = KMountPoint::currentMountPoints().findByPath(m_url.path());
    if (!mp) {
        return;
    }

    KDiskFreeSpace* job = new KDiskFreeSpace(this);
    connect(job, SIGNAL(foundMountPoint(const QString&,
                                        quint64,
                                        quint64,
                                        quint64)),
            this, SLOT(slotFoundMountPoint(const QString&,
                                           quint64,
                                           quint64,
                                           quint64)));

    job->readDF(mp->mountPoint());
}

#include "statusbarspaceinfo.moc"
