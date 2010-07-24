/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "dolphinnewmenuobserver.h"

#include <kglobal.h>
#include <knewmenu.h>

class DolphinNewMenuObserverSingleton
{
public:
    DolphinNewMenuObserver instance;
};
K_GLOBAL_STATIC(DolphinNewMenuObserverSingleton, s_dolphinNewMenuObserver)

DolphinNewMenuObserver& DolphinNewMenuObserver::instance()
{
    return s_dolphinNewMenuObserver->instance;
}

void DolphinNewMenuObserver::attach(const KNewFileMenu* menu)
{
    connect(menu, SIGNAL(fileCreated(const KUrl&)),
            this, SIGNAL(itemCreated(const KUrl&)));
    connect(menu, SIGNAL(directoryCreated(const KUrl&)),
            this, SIGNAL(itemCreated(const KUrl&)));
}

void DolphinNewMenuObserver::detach(const KNewFileMenu* menu)
{
    disconnect(menu, SIGNAL(fileCreated(const KUrl&)),
               this, SIGNAL(itemCreated(const KUrl&)));
    disconnect(menu, SIGNAL(directoryCreated(const KUrl&)),
               this, SIGNAL(itemCreated(const KUrl&)));
}

DolphinNewMenuObserver::DolphinNewMenuObserver() :
    QObject(0)
{
}

DolphinNewMenuObserver::~DolphinNewMenuObserver()
{
}

#include "dolphinnewmenuobserver.moc"
