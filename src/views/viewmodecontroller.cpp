/*
 * SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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

QUrl ViewModeController::url() const
{
    return m_url;
}

void ViewModeController::redirectToUrl(const QUrl& url)
{
    m_url = url;
}

void ViewModeController::indicateActivationChange(bool active)
{
    Q_EMIT activationChanged(active);
}

void ViewModeController::setNameFilter(const QString& nameFilter)
{
    if (nameFilter != m_nameFilter) {
        m_nameFilter = nameFilter;
        Q_EMIT nameFilterChanged(nameFilter);
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
        Q_EMIT zoomLevelChanged(m_zoomLevel);
    }
}

int ViewModeController::zoomLevel() const
{
    return m_zoomLevel;
}

void ViewModeController::setUrl(const QUrl& url)
{
    if (m_url != url) {
        m_url = url;
        Q_EMIT cancelPreviews();
        Q_EMIT urlChanged(url);
    }
}

