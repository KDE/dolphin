/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/     

#include "kfmrun.h"
#include "konq_mainview.h"

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
  cerr << "FILTERING " << _type << endl;
  
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

  cerr << "Nothing special to do here" << endl;

  KRun::foundMimeType( _type );
}

#include "kfmrun.moc"
