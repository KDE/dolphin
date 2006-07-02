/* This file is part of the KDE project
   Copyright (C) 2006 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_application.h"
#include <konq_settings.h>
#include <QtDBus/QtDBus>
#include "konq_mainwindow.h"
#include "KonquerorAdaptor.h"

KonquerorApplication::KonquerorApplication()
    : KApplication(),
      closed_by_sm( false )
{
    new KonquerorAdaptor;
    const QString dbusPath = "/Konqueror";
    const QString dbusInterface = "org.kde.Konqueror";
    QDBusConnection& dbus = QDBus::sessionBus();
    dbus.connect(QString(), dbusPath, dbusInterface, "reparseConfiguration", this, SLOT(slotReparseConfiguration()));
}

void KonquerorApplication::slotReparseConfiguration()
{
    KGlobal::config()->reparseConfiguration();
    KonqFMSettings::reparseConfiguration();

    QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
    if ( mainWindows )
    {
        foreach ( KonqMainWindow* window, *mainWindows )
            window->reparseConfiguration();
    }
}

#include "konq_application.moc"
