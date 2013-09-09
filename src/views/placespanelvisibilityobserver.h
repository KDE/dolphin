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

#ifndef PLACESPANEL_VISIBILITY_OBSERVER_H
#define PLACESPANEL_VISIBILITY_OBSERVER_H

#include <QObject>
#include <QSet>

class KUrlNavigator;

/**
 * Keeps a list of all added KUrlNavigators, to update the places selector
 * visibility of the url navigator when the Places panel got visible/invisible.
 *
 * The places-selector of all views is only shown if the Places panel
 * is invisible.
 */
class PlacesPanelVisibilityObserver : public QObject
{
    Q_OBJECT

public:
    static PlacesPanelVisibilityObserver& instance();

    void addUrlNavigator(KUrlNavigator* navigator);
    void removeUrlNavigator(KUrlNavigator* navigator);

public slots:
    void placesPanelVisibilityChanged(bool visible);

private:
    PlacesPanelVisibilityObserver();
    virtual ~PlacesPanelVisibilityObserver();

private:
    bool m_placesPanelVisible;
    QSet<KUrlNavigator*> m_urlNavigators;

    friend class PlacesPanelVisibilityObserverSingleton;
};

#endif