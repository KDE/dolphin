/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000 David Faure <faure@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "KonquerorIface.h"
#include "konq_misc.h"
#include "KonqMainWindowIface.h"
#include "konq_mainwindow.h"
#include "konq_viewmgr.h"
#include <konqsettings.h>
#include <ksimpleconfig.h>
#include <kapp.h>
#include <dcopclient.h>
#include <kdebug.h>

KonquerorIface::KonquerorIface()
 : DCOPObject( "KonquerorIface" )
{
}

KonquerorIface::~KonquerorIface()
{
}

void KonquerorIface::openBrowserWindow( const QString &url )
{
  KonqFileManager::self()->openFileManagerWindow( url );
}

void KonquerorIface::createBrowserWindowFromProfile( const QString &filename )
{
  kdDebug(1202) << "void KonquerorIface::createBrowserWindowFromProfile( const QString &filename ) " << endl;
  kdDebug(1202) << filename << endl;

  KonqMainWindow *mainWindow = new KonqMainWindow( QString::null, false );

  KSimpleConfig cfg( filename, true );
  cfg.setDollarExpansion( true );
  cfg.setGroup( "Profile" );
  mainWindow->viewManager()->loadViewProfile( cfg );
  mainWindow->enableAllActions( true );

  mainWindow->show();
}

void KonquerorIface::setMoveSelection( int move )
{
  kdDebug(1202) << "setMoveSelection: " << move << endl;
  KonqMainWindow::setMoveSelection( (bool)move );
}

void KonquerorIface::reparseConfiguration()
{
  KGlobal::config()->reparseConfiguration();
  KonqFMSettings::reparseConfiguration();

  QList<KonqMainWindow> *mainWindows = KonqMainWindow::mainWindowList();
  if ( mainWindows )
  {
    QListIterator<KonqMainWindow> it( *mainWindows );
    for (; it.current(); ++it )
        it.current()->reparseConfiguration();
  }
}

QValueList<DCOPRef> KonquerorIface::getWindows()
{
    QValueList<DCOPRef> lst;
    QList<KonqMainWindow> *mainWindows = KonqMainWindow::mainWindowList();
    if ( mainWindows )
    {
      QListIterator<KonqMainWindow> it( *mainWindows );
      for (; it.current(); ++it )
        lst.append( DCOPRef( kapp->dcopClient()->appId(), it.current()->dcopObject()->objId() ) );
    }
    return lst;
}
