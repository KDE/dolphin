/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
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

#include <kded_instance.h>
#include <ktrader.h>

#include "kfmrun.h"
#include "konq_mainview.h"
#include "konq_mainwindow.h"
#include "konq_plugins.h"

#include <string.h>

KfmRun::KfmRun( KonqMainView* _view, const char *_url, mode_t _mode, bool _is_local_file, bool _auto_delete )
  : KRun( _url, _mode, _is_local_file, _auto_delete )
{
  m_pView = _view;
}

KfmRun::~KfmRun()
{
// ? do we need this? (Simon)
//  if ( m_pView )
//    m_pView->openNothing();
}

void KfmRun::foundMimeType( const char *_type )
{
  kdebug(0,1202,"FILTERING %s", _type);
  
  if ( strcmp( _type, "inode/directory" ) == 0 )
  {    
    m_pView->openDirectory( m_strURL );
    m_pView = 0L;
    m_bFinished = true;
    m_timer.start( 0, true );
    return;
  }
  else if ( strcmp( _type, "text/html" ) == 0 )
  {
    m_pView->openHTML( m_strURL );
    m_pView = 0L;
    m_bFinished = true;
    m_timer.start( 0, true );
    return;
  }
  else if ( strncmp( _type, "text/", 5 ) == 0 )
  {
    //FIXME: this is a bad hack
    
    QString type( _type );
    type.remove( 0, 5 );
    
    if ( ( type.left( 2 ) == "x-" ) ||
         ( type == "english" ) ||
	 ( type == "plain" ) )
       {
         m_pView->openText( m_strURL );
	 m_pView = 0L;
	 m_bFinished = true;
	 m_timer.start( 0, true );
	 return;
       }
  }

  CORBA::Object_var obj = KonqPlugins::lookupViewServer( _type );
  if ( !CORBA::is_nil( obj ) )
  {
    kdebug(0,1202,"found plugin for %s", _type);

    Konqueror::ViewFactory_var factory = Konqueror::ViewFactory::_narrow( obj );
    Konqueror::View_var v = factory->create();
    KonqPlugins::associate( v->viewName(), _type );
    m_pView->openPluginView( m_strURL, v );
    m_pView = 0L;
    m_bFinished = true;
    m_timer.start( 0, true );
    return;
  }
    
  kdebug(0,1202,"Nothing special to do here");

  KRun::foundMimeType( _type );
}

void KonqFileManager::openFileManagerWindow( const char *_url )
{
  KonqMainWindow *m_pShell = new KonqMainWindow( _url );
  m_pShell->show();
}

#include "kfmrun.moc"
