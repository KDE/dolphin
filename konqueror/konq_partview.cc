/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/ 

#include "konq_partview.h"
#include <opFrame.h>
#include <opUIUtils.h>
#include <klocale.h>

/*
 * Simon's small TODO list :)
 * - emit a fake eventChildGotFocus Event to the Parent Part (only if the
 *   parent part supports the KonqMainView interface) when the attached
 *   part gets the focus. In order to implement this I have to:
 *  1) make the attached part a child part
 *  2) if the child part gets the focus
 *     then do this:
 *     - emit our fake event to the "real" parent
 *     - remove us (KonqPartView) as part parent from our child
 *     - call m_vMainWindow->setActivePart(...)
 *     - re-parent the attached part again
 *     - do something I forgot?
 *  ...tricky, tricky, I wonder whether it will work ;-]
 */

KonqPartView::KonqPartView()
{
  ADD_INTERFACE( "IDL:Konqueror/PartView:1.0" );

  setWidget( this );
  
  m_vPart = 0L;
  m_pFrame = 0L;
}

KonqPartView::~KonqPartView()
{
  cleanUp();
}

void KonqPartView::init()
{
  m_pFrame = new OPFrame( this );
}

void KonqPartView::cleanUp()
{
  if ( m_bIsClean )
    return;
    
  if ( !CORBA::is_nil( m_vPart ) )
    m_vPart = 0L;
    
  if ( m_pFrame )    
  {
    m_pFrame->detach();
    delete m_pFrame;
  }    
    
  KonqBaseView::cleanUp();    
}

bool KonqPartView::mappingFillMenuView( Konqueror::View::EventFillMenu viewMenu )
{
  OpenPartsUI::Menu_var menu = OpenPartsUI::Menu::_duplicate( viewMenu.menu );
  
  if ( !CORBA::is_nil( menu ) )
  {
    CORBA::WString_var text = Q2C( i18n("Detach part") );
    menu->insertItem( text, this, "detachPart", 0 );
  }
}

void KonqPartView::detachPart()
{
  if ( m_pFrame )
  {
    m_pFrame->detach();
    m_pFrame->hide();
    m_vPart = 0L;
  }    
}

void KonqPartView::setPart( OpenParts::Part_ptr part )
{
  m_vPart = OpenParts::Part::_duplicate( part );
  m_pFrame->attach( m_vPart );
  m_pFrame->show();
}

OpenParts::Part_ptr KonqPartView::part()
{
  return OpenParts::Part::_duplicate( m_vPart );
}

void KonqPartView::resizeEvent( QResizeEvent * )
{
  if ( m_pFrame )
    m_pFrame->setGeometry( 0, 0, width(), height() );
}

#include "konq_partview.moc"
