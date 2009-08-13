/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Holger 'zecke' Freyther <freyther@kde.org>      *
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

#include "dolphinapplication.h"
#include "dolphinmainwindow.h"
#include "dolphinviewcontainer.h"

#include <applicationadaptor.h>
#include <kcmdlineargs.h>
#include <kurl.h>
#include <QtDBus/QDBusConnection>

DolphinApplication::DolphinApplication() :
    m_lastId(0), m_closedBySessionManager(false)
{
    new ApplicationAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/dolphin/Application", this);
}

DolphinApplication::~DolphinApplication()
{
    // cleanup what ever is left from the MainWindows
    while (m_mainWindows.count() != 0) {
        delete m_mainWindows.takeFirst();
    }
}

DolphinApplication* DolphinApplication::app()
{
    return qobject_cast<DolphinApplication*>(qApp);
}

DolphinMainWindow* DolphinApplication::createMainWindow()
{
    DolphinMainWindow* mainWindow = new DolphinMainWindow(m_lastId);
    ++m_lastId;
    mainWindow->init();

    m_mainWindows.append(mainWindow);
    return mainWindow;
}

void DolphinApplication::removeMainWindow(DolphinMainWindow* mainWindow)
{
    m_mainWindows.removeAll(mainWindow);
}

void DolphinApplication::refreshMainWindows()
{
    for (int i = 0; i < m_mainWindows.count(); ++i) {
        m_mainWindows[i]->refreshViews();
    }
}

int DolphinApplication::newInstance()
{
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    static bool first = true;

    switch (args->count()) {
    case 0:
        if( !first || !isSessionRestored()) {
            openWindow(KUrl());
        }
        break;

    case 1:
        openWindow(args->url(0));
        break;

    case 2:
        openSplitWindow(args->url(0),args->url(1));
        break;

    default:
        for (int i = 0; i < args->count(); ++i) {
            openWindow(args->url(i));
        }
    }

    first = false;
    args->clear();
    return 0;
}

bool DolphinApplication::closedBySessionManager() const
{
    return m_closedBySessionManager;
}

void DolphinApplication::commitData(QSessionManager& sessionManager) {
    m_closedBySessionManager = true;
    KUniqueApplication::commitData(sessionManager);
    m_closedBySessionManager = false;
}

int DolphinApplication::openWindow(const KUrl& url)
{
    DolphinMainWindow* win = createMainWindow();
    if ((win->activeViewContainer() != 0) && url.isValid()) {
        win->activeViewContainer()->setUrl(url);
    }
    win->show();
    return win->getId();
}

int DolphinApplication::openSplitWindow(const KUrl& leftUrl, const KUrl& rightUrl)
{
    DolphinMainWindow* win = createMainWindow();
    if ((win->activeViewContainer() != 0) && leftUrl.isValid()) {
        win->activeViewContainer()->setUrl(leftUrl);
    }
    win->toggleSplitView();
    if ((win->activeViewContainer() != 0) && rightUrl.isValid()){
      win->activeViewContainer()->setUrl(rightUrl);
    }
    win->show();
    return win->getId();
}


#include "dolphinapplication.moc"
