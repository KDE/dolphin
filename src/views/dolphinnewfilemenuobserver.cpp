/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinnewfilemenuobserver.h"

#include "dolphinnewfilemenu.h"

class DolphinNewFileMenuObserverSingleton
{
public:
    DolphinNewFileMenuObserver instance;
};
Q_GLOBAL_STATIC(DolphinNewFileMenuObserverSingleton, s_DolphinNewFileMenuObserver)

DolphinNewFileMenuObserver &DolphinNewFileMenuObserver::instance()
{
    return s_DolphinNewFileMenuObserver->instance;
}

void DolphinNewFileMenuObserver::attach(const DolphinNewFileMenu *menu)
{
    connect(menu, &DolphinNewFileMenu::fileCreated, this, &DolphinNewFileMenuObserver::itemCreated);
    connect(menu, &DolphinNewFileMenu::directoryCreated, this, &DolphinNewFileMenuObserver::itemCreated);
    connect(menu, &DolphinNewFileMenu::errorMessage, this, &DolphinNewFileMenuObserver::errorMessage);
}

void DolphinNewFileMenuObserver::detach(const DolphinNewFileMenu *menu)
{
    disconnect(menu, &DolphinNewFileMenu::fileCreated, this, &DolphinNewFileMenuObserver::itemCreated);
    disconnect(menu, &DolphinNewFileMenu::directoryCreated, this, &DolphinNewFileMenuObserver::itemCreated);
    disconnect(menu, &DolphinNewFileMenu::errorMessage, this, &DolphinNewFileMenuObserver::errorMessage);
}

DolphinNewFileMenuObserver::DolphinNewFileMenuObserver()
    : QObject(nullptr)
{
}

DolphinNewFileMenuObserver::~DolphinNewFileMenuObserver()
{
}
