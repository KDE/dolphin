/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
 
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

#include "konq_childview.h"
#include "konq_frame.h"
#include <qsplitter.h>

/*
 The award for the worst hack (aka Bug Oscar) goes to..... ah no, 
 it's not Microsoft, it's me :-(
 Well, there's somewhere in there a bug in regard to freeing references
 of a view. As a quick/bad hack we "force" the destruction of the object 
 (in case of a local view, for a remote view this doesn't matter) by
 simply "deref'ing" .... (ask me for further details about this)
 In contrary to the Microsoft way of hacking this hack should increase the
 stability of the product ;-) . Anyway, the problem remains and this is not
 meant to be a solution.
 Simon
 */
void VeryBadHackToFixCORBARefCntBug( CORBA::Object_ptr obj )
{
  while ( obj->_refcnt() > 1 ) obj->_deref();
}

KonqChildView::KonqChildView( Konqueror::View_ptr view, 
                              Row * row, 
                              Konqueror::NewViewPosition newViewPosition )
{
  m_pFrame = new KonqFrame( row );
  m_bCompleted = false;
  m_row = row;
  m_strLocationBarURL = "";
  m_bBack = false;
  m_bForward = false;
  m_iHistoryLock = 0;

  if (newViewPosition == Konqueror::left)
    m_row->moveToFirst( m_pFrame );
  
  attach( view );
  m_vView->show( true );
}

KonqChildView::~KonqChildView()
{
  detach();
  delete m_pFrame;
}

void KonqChildView::attach( Konqueror::View_ptr view )
{
  m_vView = Konqueror::View::_duplicate( view );
  m_pFrame->attach( view );
  m_pFrame->show();
}

void KonqChildView::detach()
{
  m_pFrame->detach();
  m_pFrame->hide();
  m_vView->decRef();
  VeryBadHackToFixCORBARefCntBug( m_vView );
  m_vView = 0L;
}

void KonqChildView::repaint()
{
  m_pFrame->repaint();
}

#include "konq_childview.moc"
