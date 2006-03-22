/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "KonqMainWindowIface.h"
#include "KonqViewIface.h"
#include "konq_view.h"


#include <dcopclient.h>
#include <kapplication.h>
#include <kdcopactionproxy.h>
#include <kdcoppropertyproxy.h>
#include <kdebug.h>
#include <kstartupinfo.h>
#include <kwin.h>

KonqMainWindowIface::KonqMainWindowIface( KonqMainWindow * mainWindow )
    :
    // ARGL I hate this "virtual public DCOPObject" stuff!
    DCOPObject( mainWindow->name() ),
    KMainWindowInterface( mainWindow ), m_pMainWindow( mainWindow )
{
  m_dcopActionProxy = new KDCOPActionProxy( mainWindow->actionCollection(), this );
}

KonqMainWindowIface::~KonqMainWindowIface()
{
  delete m_dcopActionProxy;
}

void KonqMainWindowIface::openURL( QString url )
{
  m_pMainWindow->openFilteredUrl( url );
}

void KonqMainWindowIface::newTab( QString url )
{
  m_pMainWindow->openFilteredUrl( url, true );
}

void KonqMainWindowIface::openURL( QString url, bool tempFile )
{
  m_pMainWindow->openFilteredUrl( url, false, tempFile );
}

void KonqMainWindowIface::newTab( QString url, bool tempFile )
{
  m_pMainWindow->openFilteredUrl( url, true, tempFile );
}

void KonqMainWindowIface::newTabASN( QString url, const DCOPCString& startup_id, bool tempFile )
{
  KStartupInfo::setNewStartupId( m_pMainWindow, startup_id );
  m_pMainWindow->openFilteredUrl( url, true, tempFile );
}

void KonqMainWindowIface::reload()
{
  m_pMainWindow->slotReload();
}

DCOPRef KonqMainWindowIface::currentView()
{
  DCOPRef res;

  KonqView *view = m_pMainWindow->currentView();
  if ( !view )
    return res;

  return DCOPRef( kapp->dcopClient()->appId(), view->dcopObject()->objId() );
}

DCOPRef KonqMainWindowIface::currentPart()
{
  DCOPRef res;

  KonqView *view = m_pMainWindow->currentView();
  if ( !view )
    return res;

  return view->dcopObject()->part();
}

DCOPRef KonqMainWindowIface::action( const DCOPCString &name )
{
  return DCOPRef( kapp->dcopClient()->appId(), m_dcopActionProxy->actionObjectId( name ) );
}

DCOPCStringList KonqMainWindowIface::actions()
{
  DCOPCStringList res;
  QList<KAction *> lst = m_dcopActionProxy->actions();
  QList<KAction *>::ConstIterator it = lst.begin();
  QList<KAction *>::ConstIterator end = lst.end();
  for (; it != end; ++it )
    res.append( (*it)->name() );

  return res;
}

QMap<DCOPCString,DCOPRef> KonqMainWindowIface::actionMap()
{
  return m_dcopActionProxy->actionMap();
}

DCOPCStringList KonqMainWindowIface::functionsDynamic()
{
    return DCOPObject::functionsDynamic() + KDCOPPropertyProxy::functions( m_pMainWindow );
}

bool KonqMainWindowIface::processDynamic( const DCOPCString &fun, const QByteArray &data, DCOPCString &replyType, QByteArray &replyData )
{
    if ( KDCOPPropertyProxy::isPropertyRequest( fun, m_pMainWindow ) )
        return KDCOPPropertyProxy::processPropertyRequest( fun, data, replyType, replyData, m_pMainWindow );

    return DCOPObject::processDynamic( fun, data, replyType, replyData );
}

bool KonqMainWindowIface::windowCanBeUsedForTab()
{
    KWin::WindowInfo winfo = KWin::windowInfo( m_pMainWindow->winId(), NET::WMDesktop );
    if( !winfo.isOnCurrentDesktop() )
        return false; // this window shows on different desktop
    if( KonqMainWindow::isPreloaded() )
        return false; // we want a tab in an already shown window
    return true;
}

