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

#include "kfm_part.h"
#include "kfmview.h"

#include <qcolor.h>

KfmPart::KfmPart( QWidget *_parent ) : QWidget( _parent )
{
  setWidget( this );

  // Accept the focus
  OPPartIf::setFocusPolicy( OpenParts::Part::ClickFocus ); 

  setBackgroundColor( red );

  m_pView = new KfmView( this, this );
  m_pView->show();
  m_pView->setFocus();
  m_pView->openURL( "file:/tmp" );
}

void KfmPart::init()
{
  /******************************************************
   * Menu
   ******************************************************/

  // We want to display a menubar
  OpenParts::MenuBarManager_var menu_bar_manager = m_vMainWindow->menuBarManager();
  if ( !CORBA::is_nil( menu_bar_manager ) )
    menu_bar_manager->registerClient( id(), this );
}

KfmPart::~KfmPart()
{
  cout << "-KfmPart" << endl;
}
  
void KfmPart::cleanUp()
{
  // We must do that to avoid recursions.
  if ( m_bIsClean )
    return;

  // Say bye to our menu bar
  OpenParts::MenuBarManager_var menu_bar_manager = m_vMainWindow->menuBarManager();
  if ( !CORBA::is_nil( menu_bar_manager ) )
    menu_bar_manager->unregisterClient( id() );
  
  // Very important!!! The base classes must run
  // their cleanup function bevore! the constructor does it.
  // The reason is that CORBA reference counting is not very
  // happy about Qt, since Qt deletes its child widgets and does
  // not care about reference counters.
  OPPartIf::cleanUp();
}

void KfmPart::resizeEvent( QResizeEvent * )
{
  cerr << "Resizing" << endl;
  m_pView->setGeometry( 0, 0, width(), height() );
}

bool KfmPart::event( const char* _event, const CORBA::Any& _value )
{
  // Here we map events to function calls
  EVENT_MAPPER( _event, _value );

  MAPPING( OpenPartsUI::eventCreateMenuBar, OpenPartsUI::typeCreateMenuBar_var, mappingCreateMenubar );

  END_EVENT_MAPPER;
  
  // No, we could not handle this event
  return false;
}

bool KfmPart::mappingCreateMenubar( OpenPartsUI::MenuBar_ptr _menubar )
{
  // Do we loose control over the menubar `
  if ( CORBA::is_nil( _menubar ) )
  {
    m_vMenuEdit = 0L;
    return true;
  }

  // Edit  
  _menubar->insertMenu( i18n( "&Bookmarks" ), m_vMenuEdit, -1, -1 );
  
  // We want to be informed about activated items
  // m_vMenuEdit->connect( "activated", this, "bookmarkSelected" );
  
  m_vMenuEdit->insertItem( "Bookmark 1", 0L, "", 0 );
  m_vMenuEdit->insertItem( "Bookmark 2", 0L, "", 0 );
  m_vMenuEdit->insertItem( "Bookmark 3", 0L, "", 0 );

  // Yes, we handled this event
  return true;
}

void KfmPart::bookmarkSelected( CORBA::Long id )
{
  // TODO
}

void KfmPart::setStatusBarText( const char *_text )
{
  // TODO
}

void KfmPart::setLocationBarURL( const char *_url )
{
  // TODO
}

void KfmPart::setUpURL( const char *_url )
{
  // TODO
}
  
void KfmPart::addHistory( const char *_url, int _xoffset, int _yoffset )
{
  // TODO
}

void KfmPart::createGUI( const char *_url )
{
  // TODO
}

bool KfmPart::hasUpURL()
{
  return false;
}

bool KfmPart::hasBackHistory()
{
  return false;
}

bool KfmPart::hasForwardHistory()
{
  return false;
}

#include "kfm_part.moc"


