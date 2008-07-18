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
    KCapacityBar(KCapacityBar::DrawTextInline, parent),
    m_gettingSize(false),
    m_foundMountPoint(false)
{
    setMaximumWidth(200);
    setMinimumWidth(200); // something to fix on kcapacitybar (ereslibre)

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
}

void StatusBarSpaceInfo::slotFoundMountPoint(const QString& mountPoint,
                                             quint64 kBSize,
                                             quint64 kBUsed,
                                             quint64 kBAvailable)
{
    Q_UNUSED(mountPoint);

    m_gettingSize = false;
    m_foundMountPoint = true;
    const bool valuesChanged = (kBUsed != static_cast<quint64>(value()));
    if (valuesChanged) {
        setText(i18nc("@info:status Free disk space", "%1 free", KIO::convertSize(kBAvailable * 1024)));
        setUpdatesEnabled(false);
        setValue((kBUsed * 100) / kBSize);
        setUpdatesEnabled(true);
        update();
    }
}

void StatusBarSpaceInfo::slotDiskFreeSpaceDone()
{
    if (m_foundMountPoint) {
        return;
    }

    m_gettingSize = false;
    setText(i18nc("@info:status", "Unknown size"));
    setValue(0);
    update();
}

void StatusBarSpaceInfo::refresh()
{
    // KDiskFreeSpace is for local paths only
    if (!m_url.isLocalFile()) {
        setText(i18nc("@info:status", "Unknown size"));
        setValue(0);
        update();
        return;
    }

    KMountPoint::Ptr mp = KMountPoint::currentMountPoints().findByPath(m_url.path());
    if (!mp) {
        return;
    }

    m_gettingSize = true;
    m_foundMountPoint = false;
    KDiskFreeSpace* job = new KDiskFreeSpace(this);
    connect(job, SIGNAL(foundMountPoint(const QString&,
                                        quint64,
                                        quint64,
                                        quint64)),
            this, SLOT(slotFoundMountPoint(const QString&,
                                           quint64,
                                           quint64,
                                           quint64)));
    connect(job, SIGNAL(done()), this, SLOT(slotDiskFreeSpaceDone()));

    job->readDF(mp->mountPoint());

    // refresh() is invoked for each directory change. Usually getting
    // the size information can be done very fast, so to prevent any
    // flickering the "Getting size..." indication is only shown if
    // at least 300 ms have been passed.
    QTimer::singleShot(300, this, SLOT(showGettingSizeInfo()));
}

void StatusBarSpaceInfo::showGettingSizeInfo()
{
    if (m_gettingSize) {
        setText(i18nc("@info:status", "Getting size..."));
        update();
    }
}

#include "statusbarspaceinfo.moc"
