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
#include "metadataloader.h"

#include <applicationadaptor.h>
#include <kurl.h>
#include <QDBusConnection>

DolphinApplication::DolphinApplication() :
    m_lastId(0)
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

MetadataLoader* DolphinApplication::metadataLoader()
{
    static MetadataLoader loader;
    return &loader;
}

int DolphinApplication::openWindow(const QString& url)
{
    DolphinMainWindow* win = createMainWindow();
    if ((win->activeView() != 0) && !url.isEmpty()) {
        win->activeView()->setUrl(KUrl(url));
    }
    win->show();
    //TODO find how to raise a window (as if we've launched a new dolphin process)
    return win->getId();
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

#include "dolphinapplication.moc"
