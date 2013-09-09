/***************************************************************************
 * Copyright (C) 2013 by Emmanuel Pescosta <emmanuelpescosta099@gmail.com> *
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

#include "placespanelvisibilityobserver.h"

#include <KGlobal>
#include <KUrlNavigator>

class PlacesPanelVisibilityObserverSingleton
{
public:
    PlacesPanelVisibilityObserver instance;
};
K_GLOBAL_STATIC(PlacesPanelVisibilityObserverSingleton, s_PlacesPanelVisibilityObserver)

PlacesPanelVisibilityObserver& PlacesPanelVisibilityObserver::instance()
{
    return s_PlacesPanelVisibilityObserver->instance;
}

void PlacesPanelVisibilityObserver::addUrlNavigator(KUrlNavigator* navigator)
{
    if (navigator) {
        navigator->setPlacesSelectorVisible(!m_placesPanelVisible);
        m_urlNavigators.insert(navigator);
    }
}

void PlacesPanelVisibilityObserver::removeUrlNavigator(KUrlNavigator* navigator)
{
    m_urlNavigators.remove(navigator);
}

void PlacesPanelVisibilityObserver::placesPanelVisibilityChanged(bool visible)
{
    m_placesPanelVisible = visible;

    foreach (KUrlNavigator* navigator, m_urlNavigators) {
        navigator->setPlacesSelectorVisible(!visible);
    }
}

PlacesPanelVisibilityObserver::PlacesPanelVisibilityObserver() :
    QObject(0),
    m_placesPanelVisible(false),
    m_urlNavigators()
{
}

PlacesPanelVisibilityObserver::~PlacesPanelVisibilityObserver()
{
    m_urlNavigators.clear();
}