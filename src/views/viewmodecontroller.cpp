/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "viewmodecontroller.h"

#include "zoomlevelinfo.h"

ViewModeController::ViewModeController(QObject* parent) :
    QObject(parent),
    m_zoomLevel(0),
    m_nameFilter(),
    m_url()
{
}

ViewModeController::~ViewModeController()
{
}

KUrl ViewModeController::url() const
{
    return m_url;
}

void ViewModeController::redirectToUrl(const KUrl& url)
{
    m_url = url;
}

void ViewModeController::indicateActivationChange(bool active)
{
    emit activationChanged(active);
}

void ViewModeController::setNameFilter(const QString& nameFilter)
{
    if (nameFilter != m_nameFilter) {
        m_nameFilter = nameFilter;
        emit nameFilterChanged(nameFilter);
    }
}

QString ViewModeController::nameFilter() const
{
    return m_nameFilter;
}

void ViewModeController::setZoomLevel(int level)
{
    Q_ASSERT(level >= ZoomLevelInfo::minimumLevel());
    Q_ASSERT(level <= ZoomLevelInfo::maximumLevel());
    if (level != m_zoomLevel) {
        m_zoomLevel = level;
        emit zoomLevelChanged(m_zoomLevel);
    }
}

int ViewModeController::zoomLevel() const
{
    return m_zoomLevel;
}

void ViewModeController::setUrl(const KUrl& url)
{
    if (m_url != url) {
        m_url = url;
        emit cancelPreviews();
        emit urlChanged(url);
    }
}

#include "viewmodecontroller.moc"
