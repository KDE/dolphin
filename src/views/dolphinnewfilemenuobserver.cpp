/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "dolphinnewfilemenuobserver.h"

#include "dolphinnewfilemenu.h"

class DolphinNewFileMenuObserverSingleton
{
public:
    DolphinNewFileMenuObserver instance;
};
Q_GLOBAL_STATIC(DolphinNewFileMenuObserverSingleton, s_DolphinNewFileMenuObserver)

DolphinNewFileMenuObserver& DolphinNewFileMenuObserver::instance()
{
    return s_DolphinNewFileMenuObserver->instance;
}

void DolphinNewFileMenuObserver::attach(const DolphinNewFileMenu* menu)
{
    connect(menu, &DolphinNewFileMenu::fileCreated,
            this, &DolphinNewFileMenuObserver::itemCreated);
    connect(menu, &DolphinNewFileMenu::directoryCreated,
            this, &DolphinNewFileMenuObserver::itemCreated);
    connect(menu, &DolphinNewFileMenu::errorMessage,
            this, &DolphinNewFileMenuObserver::errorMessage);
}

void DolphinNewFileMenuObserver::detach(const DolphinNewFileMenu* menu)
{
    disconnect(menu, &DolphinNewFileMenu::fileCreated,
               this, &DolphinNewFileMenuObserver::itemCreated);
    disconnect(menu, &DolphinNewFileMenu::directoryCreated,
               this, &DolphinNewFileMenuObserver::itemCreated);
    disconnect(menu, &DolphinNewFileMenu::errorMessage,
               this, &DolphinNewFileMenuObserver::errorMessage);
}

DolphinNewFileMenuObserver::DolphinNewFileMenuObserver() :
    QObject(0)
{
}

DolphinNewFileMenuObserver::~DolphinNewFileMenuObserver()
{
}

