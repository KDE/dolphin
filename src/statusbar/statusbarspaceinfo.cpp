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

#include <KDiskFreeSpaceInfo>
#include <kmountpoint.h>
#include <KLocale>
#include <KIO/Job>

#include <QTimer>
#include <QKeyEvent>

StatusBarSpaceInfo::StatusBarSpaceInfo(QWidget* parent) :
    KCapacityBar(KCapacityBar::DrawTextInline, parent),
    m_kBSize(0),
    m_timer(0)
{
    // Use a timer to update the space information. Polling is useful
    // here, as files can be deleted/added outside the scope of Dolphin.
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(refresh()));
}

StatusBarSpaceInfo::~StatusBarSpaceInfo()
{
}

void StatusBarSpaceInfo::setUrl(const KUrl& url)
{
    m_url = url;
    refresh();
}

KUrl StatusBarSpaceInfo::url() const
{
    return m_url;
}

void StatusBarSpaceInfo::showEvent(QShowEvent* event)
{
    KCapacityBar::showEvent(event);
    if (!event->spontaneous()) {
        refresh();
        m_timer->start(10000);
    }
}

void StatusBarSpaceInfo::hideEvent(QHideEvent* event)
{
    m_timer->stop();
    KCapacityBar::hideEvent(event);
}

void StatusBarSpaceInfo::refresh()
{
    if (!isVisible()) {
        return;
    }

    // KDiskFreeSpace is for local paths only
    if (!m_url.isLocalFile()) {
        setText(i18nc("@info:status", "Unknown size"));
        setValue(0);
        update();
        return;
    }

    KMountPoint::Ptr mp = KMountPoint::currentMountPoints().findByPath(m_url.toLocalFile());
    if (!mp) {
        return;
    }

    KDiskFreeSpaceInfo job = KDiskFreeSpaceInfo::freeSpaceInfo(mp->mountPoint());
    if (!job.isValid()) {
        setText(i18nc("@info:status", "Unknown size"));
        setValue(0);
        update();
        return;
    }

    KIO::filesize_t kBSize = job.size() / 1024;
    KIO::filesize_t kBUsed = job.used() / 1024;

    const bool valuesChanged = (kBUsed != static_cast<quint64>(value())) || (kBSize != m_kBSize);
    if (valuesChanged) {
        setText(i18nc("@info:status Free disk space", "%1 free",
                KIO::convertSize(job.available())));

        setUpdatesEnabled(false);
        m_kBSize = kBSize;
        setValue(kBSize > 0 ? (kBUsed * 100) / kBSize : 0);
        setUpdatesEnabled(true);
        update();
    }
}

#include "statusbarspaceinfo.moc"
