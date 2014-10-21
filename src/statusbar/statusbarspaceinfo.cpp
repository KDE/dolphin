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

#include "spaceinfoobserver.h"

#include <KLocalizedString>
#include <KIO/Job>


StatusBarSpaceInfo::StatusBarSpaceInfo(QWidget* parent) :
    KCapacityBar(KCapacityBar::DrawTextInline, parent),
    m_observer(0)
{
}

StatusBarSpaceInfo::~StatusBarSpaceInfo()
{
}

void StatusBarSpaceInfo::setUrl(const QUrl& url)
{
    if (m_url != url) {
        m_url = url;
        if (m_observer) {
            m_observer->setUrl(url);
        }
    }
}

QUrl StatusBarSpaceInfo::url() const
{
    return m_url;
}

void StatusBarSpaceInfo::showEvent(QShowEvent* event)
{
    KCapacityBar::showEvent(event);
    m_observer.reset(new SpaceInfoObserver(m_url, this));
    slotValuesChanged();
    connect(m_observer.data(), &SpaceInfoObserver::valuesChanged, this, &StatusBarSpaceInfo::slotValuesChanged);
}

void StatusBarSpaceInfo::hideEvent(QHideEvent* event)
{
    m_observer.reset();
    KCapacityBar::hideEvent(event);
}

void StatusBarSpaceInfo::slotValuesChanged()
{
    Q_ASSERT(m_observer);
    const quint64 size = m_observer->size();
    if (size == 0) {
        setText(i18nc("@info:status", "Unknown size"));
        setValue(0);
        update();
    } else {
        const quint64 available = m_observer->available();
        const quint64 used = size - available;
        const int percentUsed = qRound(100.0 * qreal(used) / qreal(size));

        setText(i18nc("@info:status Free disk space", "%1 free", KIO::convertSize(available)));
        setUpdatesEnabled(false);
        setValue(percentUsed);
        setUpdatesEnabled(true);
        update();
    }
}

