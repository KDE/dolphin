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
#include <konq_settings.h>
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

ASYNC KonquerorIface::openBrowserWindow( const QString &url )
{
    /*KonqMainWindow *res = */ KonqMisc::createSimpleWindow( KURL(url) );
    /*if ( !res )
        return DCOPRef();
    return res->dcopObject();*/
}

ASYNC KonquerorIface::openBrowserWindowASN( const QString &url, const QCString& startup_id )
{
    kapp->setStartupId( startup_id );
    openBrowserWindow( url );
}

ASYNC KonquerorIface::createNewWindow( const QString &url )
{
    /*KonqMainWindow *res = */KonqMisc::createNewWindow( KURL(url) );
    /*if ( !res )
        return DCOPRef();
    return res->dcopObject();*/
}

ASYNC KonquerorIface::createNewWindowASN( const QString &url, const QCString& startup_id )
{
    kapp->setStartupId( startup_id );
    createNewWindow( url );
}

ASYNC KonquerorIface::createNewWindow( const QString &url, const QString &mimetype )
{
    KParts::URLArgs args;
    args.serviceType = mimetype;
    /*KonqMainWindow *res = */KonqMisc::createNewWindow( KURL(url), args );
    /*if ( !res )
        return DCOPRef();
    return res->dcopObject();*/
}

ASYNC KonquerorIface::createNewWindowASN( const QString &url, const QString &mimetype,
    const QCString& startup_id )
{
    kapp->setStartupId( startup_id );
    createNewWindow( url, mimetype );
}

ASYNC KonquerorIface::createBrowserWindowFromProfile( const QString &path )
{
    kdDebug(1202) << "void KonquerorIface::createBrowserWindowFromProfile( const QString &path ) " << endl;
    kdDebug(1202) << path << endl;
    /*KonqMainWindow *res = */KonqMisc::createBrowserWindowFromProfile( path, QString::null );
    /*if ( !res )
        return DCOPRef();
    return res->dcopObject();*/
}

ASYNC KonquerorIface::createBrowserWindowFromProfileASN( const QString &path, const QCString& startup_id )
{
    kapp->setStartupId( startup_id );
    createBrowserWindowFromProfile( path );
}

ASYNC KonquerorIface::createBrowserWindowFromProfile( const QString & path, const QString &filename )
{
    kdDebug(1202) << "void KonquerorIface::createBrowserWindowFromProfile( path, filename ) " << endl;
    kdDebug(1202) << path << "," << filename << endl;
    /*KonqMainWindow *res = */KonqMisc::createBrowserWindowFromProfile( path, filename );
    /*if ( !res )
        return DCOPRef();
    return res->dcopObject();*/
}

ASYNC KonquerorIface::createBrowserWindowFromProfileASN( const QString &path, const QString &filename,
    const QCString& startup_id )
{
    kapp->setStartupId( startup_id );
    createBrowserWindowFromProfile( path, filename );
}

ASYNC KonquerorIface::createBrowserWindowFromProfileAndURL( const QString & path, const QString &filename, const QString &url )
{
    /*KonqMainWindow *res = */KonqMisc::createBrowserWindowFromProfile( path, filename, KURL(url) );
    /*if ( !res )
        return DCOPRef();
    return res->dcopObject();*/
}

ASYNC KonquerorIface::createBrowserWindowFromProfileAndURLASN( const QString & path, const QString &filename, const QString &url,
    const QCString& startup_id )
{
    kapp->setStartupId( startup_id );
    createBrowserWindowFromProfileAndURL( path, filename, url );
}

ASYNC KonquerorIface::createBrowserWindowFromProfileAndURL( const QString &path, const QString &filename, const QString &url, const QString &mimetype )
{
    KParts::URLArgs args;
    args.serviceType = mimetype;
    /*KonqMainWindow *res = */KonqMisc::createBrowserWindowFromProfile( path, filename, KURL(url), args );
    /*if ( !res )
        return DCOPRef();
    return res->dcopObject();*/
}

ASYNC KonquerorIface::createBrowserWindowFromProfileAndURLASN( const QString & path, const QString &filename, const QString &url, const QString &mimetype,
    const QCString& startup_id )
{
    kapp->setStartupId( startup_id );
    createBrowserWindowFromProfileAndURL( path, filename, url, mimetype );
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

void KonquerorIface::updateProfileList()
{
  QList<KonqMainWindow> *mainWindows = KonqMainWindow::mainWindowList();
  if ( !mainWindows )
    return;

  QListIterator<KonqMainWindow> it( *mainWindows );
  for (; it.current(); ++it )
    it.current()->viewManager()->profileListDirty( false );
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

void KonquerorIface::addToCombo( QString url, QCString objId )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboAdd, url, objId );
}

void KonquerorIface::removeFromCombo( QString url, QCString objId )
{
  KonqMainWindow::comboAction( KonqMainWindow::ComboRemove, url, objId );
}

void KonquerorIface::comboCleared( QCString objId )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboClear,
				 QString::null, objId );
}
