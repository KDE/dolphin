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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "KonqMainWindowIface.h"
#include "konq_mainwindow.h"

#include <dcopclient.h>
#include <kapp.h>
#include <kdcopactionproxy.h>

KonqMainWindowIface::KonqMainWindowIface( KonqMainWindow * mainWindow )
    : m_pMainWindow( mainWindow )
{
  m_dcopActionProxy = new KDCOPActionProxy( mainWindow->actionCollection(), this ); 
}

KonqMainWindowIface::~KonqMainWindowIface()
{
  delete m_dcopActionProxy; 
}

int KonqMainWindowIface::viewCount()
{
    return m_pMainWindow->viewCount();
}

int KonqMainWindowIface::activeViewsCount()
{
    return m_pMainWindow->activeViewsCount();
}

DCOPRef KonqMainWindowIface::currentView()
{
    // TODO
    return DCOPRef("todo","todo");
}

DCOPRef KonqMainWindowIface::currentPart()
{
    // TODO
    return DCOPRef("todo","todo");
}

DCOPRef KonqMainWindowIface::action( const QString &name )
{
  return DCOPRef( kapp->dcopClient()->appId(), m_dcopActionProxy->action( name ) ); 
}

QStringList KonqMainWindowIface::actions()
{
  QStringList res; 
  QValueList<KAction *> lst = m_pMainWindow->actionCollection()->actions();
  QValueList<KAction *>::ConstIterator it = lst.begin();
  QValueList<KAction *>::ConstIterator end = lst.end();
  for (; it != end; ++it )
    res.append( QString::fromLatin1( (*it)->name() ) );
  
  return res;
}

QMap<QString,DCOPRef> KonqMainWindowIface::actionMap()
{
  return m_dcopActionProxy->actionMap();
} 

