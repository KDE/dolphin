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

#include <qdir.h>

#include "konq_baseview.h"
#include <kdebug.h>

#include <opUIUtils.h>

KonqBaseView::KonqBaseView()
{
  ADD_INTERFACE( "IDL:Browser/View:1.0" );

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
  cleanUp();
}

void KonqBaseView::init()
{
}

void KonqBaseView::cleanUp()
{
  if ( m_bIsClean )
    return;
    
  OPPartIf::cleanUp();
}

bool KonqBaseView::event( const char *event, const CORBA::Any &value )
{
  EVENT_MAPPER( event, value );
  
  MAPPING( Browser::View::eventFillMenuEdit, Browser::View::EventFillMenu_ptr, mappingFillMenuEdit );
  MAPPING( Browser::View::eventFillMenuView, Browser::View::EventFillMenu_ptr, mappingFillMenuView );
  MAPPING( Browser::View::eventFillToolBar, Browser::View::EventFillToolBar, mappingFillToolBar );
  MAPPING( Browser::eventOpenURL, Browser::EventOpenURL, mappingOpenURL );
    
  END_EVENT_MAPPER;
  
  return false;
}

bool KonqBaseView::mappingFillMenuView( Browser::View::EventFillMenu_ptr )
{
  return false;
}

bool KonqBaseView::mappingFillMenuEdit( Browser::View::EventFillMenu_ptr )
{
  return false;
}

bool KonqBaseView::mappingFillToolBar( Browser::View::EventFillToolBar )
{
  return false;
}

bool KonqBaseView::mappingOpenURL( Browser::EventOpenURL eventURL )
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
  Browser::URLRequest urlRequest;
  urlRequest.url = CORBA::string_dup( _url );
  urlRequest.reload = (CORBA::Boolean)false;
  urlRequest.xOffset = 0;
  urlRequest.yOffset = 0;
  SIGNAL_CALL2( "openURL", id(), urlRequest );
}

void KonqBaseView::setCaptionFromURL( const QString &_url )
{
  QString url = _url;

  if ( url.left( 6 ) == "file:/" )
  {
    if ( url.left( 7 ) == "file://" )
      url.remove( 0, 6 );
    else
      url.remove( 0, 5 );
  }    

  int l = QDir::homeDirPath().length();

  if ( url.length() >= l &&
       url.left( l ) == QDir::homeDirPath() )
  {
    url.remove( 0, l );
    url.prepend( "~" );
  }

  url.append( " - Konqueror" );
  
  CORBA::WString_var wCaption = Q2C( url );
  m_vMainWindow->setPartCaption( id(), wCaption );
}
