/* This file is part of the KDE project
   Copyright 2000 Simon Hausmann <hausmann@kde.org>
   Copyright 2000, 2006 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KonqMainWindowAdaptor_h__
#define __KonqMainWindowAdaptor_h__

#include <dbus/qdbus.h>

class KonqMainWindow;

/**
 * DBUS interface for a konqueror main window
 */
class KonqMainWindowAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Konqueror.MainWindow")

public:

    KonqMainWindowAdaptor( KonqMainWindow * mainWindow );
    ~KonqMainWindowAdaptor();

public slots:

    /**
     * Open a url in this window
     * @param url the url to open
     * @param tempFile whether to delete the file after use, usually this is false
     */
    void openUrl( const QString& url, bool tempFile );
    /**
     * Open a url in a new tab in this window
     * @param url the url to open
     * @param tempFile whether to delete the file after use, usually this is false
     */
    void newTab( const QString& url, bool tempFile );

    void newTabASN( const QString& url, const QByteArray& startup_id, bool tempFile );

    /**
     * Reloads the current view.
     */
    void reload();

    /**
     * @return reference to the current KonqView
     */
    QDBusObjectPath currentView();
    /**
     * @return reference to the current part
     */
    QDBusObjectPath currentPart();

    /**
     * Used by kfmclient when searching a window to open a tab within
     */
    bool windowCanBeUsedForTab();

private:

    KonqMainWindow * m_pMainWindow;
};

#endif

