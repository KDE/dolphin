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

KfmRun::KfmRun( KonqMainView* _view, unsigned long _viewId, const char *_url, mode_t _mode, bool _is_local_file, bool _auto_delete )
  : KRun( _url, _mode, _is_local_file, _auto_delete )
{
  m_pView = _view;
  m_ulViewId = _viewId;
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

  assert( m_pView );

  if ( m_pView->openView( QString( _type ), m_strURL, m_ulViewId ) )
  {
    m_pView = 0L;
    m_bFinished = true;
    m_timer.start( 0, true );
    return;
  }
  kdebug(0,1202,"Nothing special to do here");

  KRun::foundMimeType( _type );
}

bool KonqFileManager::openFileManagerWindow( const char *_url )
{
  KonqMainWindow *m_pShell = new KonqMainWindow( _url );
  m_pShell->show();
  return true; // why would it fail ? :)
}

#include "kfmrun.moc"
