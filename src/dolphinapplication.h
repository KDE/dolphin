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


#ifndef _DOLPHIN_APPLICATION_H
#define _DOLPHIN_APPLICATION_H

#include <kuniqueapplication.h>

class DolphinMainWindow;
class KUrl;

/**
 * @brief Holds the application data which can be accessed.
 * At first this will hold a list of DolphinMainWindows which
 * we will delete on application exit.
 */

class DolphinApplication : public KUniqueApplication
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.dolphin.Application")
    friend class DolphinMainWindow;

public:
    DolphinApplication();
    virtual ~DolphinApplication();

    static DolphinApplication* app();

    /**
     * Construct a new mainwindow which is owned
     * by the application.
     */
    DolphinMainWindow* createMainWindow();
    void refreshMainWindows();

    /** @see KUniqueApplication::newInstance(). */
    virtual int newInstance();

public slots:
    int openWindow(const KUrl& url);

protected:
    /** Called by the DolphinMainWindow to deregister. */
    void removeMainWindow(DolphinMainWindow* mainWindow);

private:
    QList<DolphinMainWindow*> m_mainWindows;
    int m_lastId;
};

#endif
