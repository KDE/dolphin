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

#include "konq_baseview.h"
#include <kdebug.h>

KonqBaseView::KonqBaseView()
{
  ADD_INTERFACE( "IDL:Konqueror/View:1.0" );

  SIGNAL_IMPL( "openURL" );
  SIGNAL_IMPL( "started" );
  SIGNAL_IMPL( "completed" );
  SIGNAL_IMPL( "canceled" );
  SIGNAL_IMPL( "setStatusBarText" );
  SIGNAL_IMPL( "setLocationBarURL" );
  SIGNAL_IMPL( "createNewWindow" );
  
  OPPartIf::setFocusPolicy( OpenParts::Part::ClickFocus );
  
  m_strURL = "";
}

KonqBaseView::~KonqBaseView()
{
  kdebug(0,1202,"KonqBaseView::~KonqBaseView() -> %s", debug_ViewName.in());
  cleanUp();
}

void KonqBaseView::init()
{
  debug_ViewName = viewName();
}

void KonqBaseView::cleanUp()
{
  if ( m_bIsClean )
    return;

  kdebug(0,1202,"void KonqBaseView::cleanUp() -> %s", debug_ViewName.in());

  OPPartIf::cleanUp();
}

bool KonqBaseView::event( const char *event, const CORBA::Any &value )
{
  EVENT_MAPPER( event, value );
  
  MAPPING( Konqueror::View::eventFillMenuEdit, Konqueror::View::EventFillMenu, mappingFillMenuEdit );
  MAPPING( Konqueror::View::eventFillMenuView, Konqueror::View::EventFillMenu, mappingFillMenuView );
  MAPPING( Konqueror::View::eventCreateViewToolBar, Konqueror::View::EventCreateViewToolBar, mappingCreateViewToolBar );
  MAPPING( Konqueror::eventOpenURL, Konqueror::EventOpenURL, mappingOpenURL );
    
  END_EVENT_MAPPER;
  
  return false;
}

bool KonqBaseView::mappingFillMenuView( Konqueror::View::EventFillMenu )
{
  return false;
}

bool KonqBaseView::mappingFillMenuEdit( Konqueror::View::EventFillMenu )
{
  return false;
}

bool KonqBaseView::mappingCreateViewToolBar( Konqueror::View::EventCreateViewToolBar )
{
  return false;
}

bool KonqBaseView::mappingOpenURL( Konqueror::EventOpenURL eventURL )
{
  SIGNAL_CALL2( "setLocationBarURL", id(), (char*)eventURL.url );
  return false;
}

char *KonqBaseView::url()
{
  return CORBA::string_dup( m_strURL.data() );
}

void KonqBaseView::openURLRequest( const char *_url )
{
  // Ask the main view to open this URL, since it might not be suitable
  // for the current type of view. It might even be a file (KRun will be used)
  Konqueror::URLRequest urlRequest;
  urlRequest.url = CORBA::string_dup( _url );
  urlRequest.reload = (CORBA::Boolean)false;
  urlRequest.xOffset = 0;
  urlRequest.yOffset = 0;
  SIGNAL_CALL1( "openURL", urlRequest );
}
