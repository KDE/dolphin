/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Faure David <faure@kde.org>
 
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

#include "konq_propsmainview.h"
#include <konqdefaults.h>

#include <assert.h>

KonqPropsMainView * KonqPropsMainView::m_pDefaultProps = 0L;

KonqPropsMainView::KonqPropsMainView( KConfig * config )
{
  QString entry;
  m_bSplitView = config->readBoolEntry( "SplitView", false );

  /*
  entry = config->readEntry("Toolbar", "top");
  m_bShowToolBar = true;
  if ( entry == "top" )
    m_toolBarPos = KToolBar::Top;
  else if ( entry == "left" )
    m_toolBarPos = KToolBar::Left;
  else if ( entry == "right" )
    m_toolBarPos = KToolBar::Right;    
  else if ( entry == "bottom" )
    m_toolBarPos = KToolBar::Bottom;    
  else if ( entry == "floating" )
    m_toolBarPos = KToolBar::Floating;    
  else if ( entry == "flat" )
    m_toolBarPos = KToolBar::Flat;    
  else
    m_bShowToolBar = false;

  entry = config->readEntry("LocationBar", "top");
  m_bShowLocationBar = true;
  if ( entry == "top" )
    m_locationBarPos = KToolBar::Top;
  else if ( entry == "bottom" )
    m_locationBarPos = KToolBar::Bottom;    
  else if ( entry == "floating" )
    m_locationBarPos = KToolBar::Floating;    
  else if ( entry == "flat" )
    m_locationBarPos = KToolBar::Flat;    
  else
    m_bShowLocationBar = false;

  entry = config->readEntry("Menubar", "top");
  m_bShowMenuBar = true;
  if ( entry == "top" )
    m_menuBarPos = KMenuBar::Top;
  else if ( entry == "bottom" )
    m_menuBarPos = KMenuBar::Bottom;    
  else if ( entry == "floating" )
    m_menuBarPos = KMenuBar::Floating;    
  else if ( entry == "flat" )
    m_menuBarPos = KMenuBar::Flat;    
  else
    m_bShowMenuBar = false;

  entry = config->readEntry("Statusbar", "top");
  m_bShowStatusBar = true;
  if ( entry == "top" )
    m_statusBarPos = KStatusBar::Top;
  else if ( entry == "bottom" )
    m_statusBarPos = KStatusBar::Bottom;    
  else if ( entry == "floating" )
    m_statusBarPos = KStatusBar::Floating;    
  else
    m_bShowStatusBar = false;
  */

  m_width = config->readNumEntry("WindowWidth",  MAINWINDOW_WIDTH);
  m_height = config->readNumEntry("WindowHeight",MAINWINDOW_HEIGHT);

  KConfigGroupSaver s( config, "Misc Defaults" );
  m_bHaveBigToolBar = config->readBoolEntry( "HaveBigToolBar", false );
}

KonqPropsMainView::~KonqPropsMainView()
{
}

void KonqPropsMainView::saveProps( KConfig * config )
{
  config->writeEntry( "SplitView", m_bSplitView );
  
  /*
  if ( !m_bShowToolBar )
      config->writeEntry( "Toolbar", "hide" );
  else
    switch( m_toolBarPos ) {
      case KToolBar::Top : config->writeEntry( "Toolbar", "top" ); break;
      case KToolBar::Bottom : config->writeEntry( "Toolbar", "bottom" ); break;
      case KToolBar::Left : config->writeEntry( "Toolbar", "left" ); break;
      case KToolBar::Right : config->writeEntry( "Toolbar", "right" ); break;
      case KToolBar::Floating : config->writeEntry( "Toolbar", "floating" ); break;
      case KToolBar::Flat : config->writeEntry( "Toolbar", "flat" ); break;
    }

  if ( !m_bShowLocationBar )
      config->writeEntry( "LocationBar", "hide" );
  else
    switch (m_locationBarPos) {
      case KToolBar::Top : config->writeEntry( "LocationBar", "top" ); break;
      case KToolBar::Bottom : config->writeEntry( "LocationBar", "bottom" ); break;
      case KToolBar::Floating : config->writeEntry( "LocationBar", "floating" ); break;
      case KToolBar::Flat : config->writeEntry( "LocationBar", "flat" ); break;
      default : assert(0);
    }

  if ( !m_bShowStatusBar )
      config->writeEntry( "Statusbar", "hide" );
  else
      config->writeEntry( "Statusbar", "bottom" );

  if ( !m_bShowMenuBar )
      config->writeEntry( "Menubar", "hide" );
  else
    switch (m_menuBarPos) {
      case KMenuBar::Top : config->writeEntry( "Menubar", "top" ); break;
      case KMenuBar::Bottom : config->writeEntry( "Menubar", "bottom" ); break;
      case KMenuBar::Floating : config->writeEntry( "Menubar", "floating" ); break;
      case KMenuBar::Flat : config->writeEntry( "Menubar", "flat" ); break;
      default : assert(0);
    }
  */

  config->writeEntry( "WindowWidth", m_width );
  config->writeEntry( "WindowHeight", m_height );

  config->sync();
}

