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

#include <KGlobal>
#include <KNewFileMenu>

class DolphinNewFileMenuObserverSingleton
{
public:
    DolphinNewFileMenuObserver instance;
};
K_GLOBAL_STATIC(DolphinNewFileMenuObserverSingleton, s_DolphinNewFileMenuObserver)

DolphinNewFileMenuObserver& DolphinNewFileMenuObserver::instance()
{
    return s_DolphinNewFileMenuObserver->instance;
}

void DolphinNewFileMenuObserver::attach(const KNewFileMenu* menu)
{
    connect(menu, SIGNAL(fileCreated(KUrl)),
            this, SIGNAL(itemCreated(KUrl)));
    connect(menu, SIGNAL(directoryCreated(KUrl)),
            this, SIGNAL(itemCreated(KUrl)));
}

void DolphinNewFileMenuObserver::detach(const KNewFileMenu* menu)
{
    disconnect(menu, SIGNAL(fileCreated(KUrl)),
               this, SIGNAL(itemCreated(KUrl)));
    disconnect(menu, SIGNAL(directoryCreated(KUrl)),
               this, SIGNAL(itemCreated(KUrl)));
}

DolphinNewFileMenuObserver::DolphinNewFileMenuObserver() :
    QObject(0)
{
}

DolphinNewFileMenuObserver::~DolphinNewFileMenuObserver()
{
}

#include "dolphinnewfilemenuobserver.moc"
