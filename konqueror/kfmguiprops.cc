/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Faure David <faure@kde.org>
 
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

#include "kfmguiprops.h"
#include "kfm_defaults.h"
#include <kpixmapcache.h>

KfmGuiProps * KfmGuiProps::m_pDefaultProps = 0L;

KfmGuiProps::KfmGuiProps( const KConfig * config )
{
  QString entry;
  // m_bDirTree = config->readBoolEntry( "DirTree", false ); ??
  m_bSplitView = config->readBoolEntry( "SplitView", false );

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

  m_width = config->readNumEntry("WindowWidth",  KFMGUI_WIDTH);
  m_height = config->readNumEntry("WindowHeight",KFMGUI_HEIGHT);

  entry = "";
  QString pix = config->readEntry( "BackgroundPixmap", entry );
  if ( !pix.isEmpty() )
  {
    QPixmap* p = KPixmapCache::wallpaperPixmap( pix );
    if ( p )
    {
      cerr << "Got background" << endl;
      m_bgPixmap = *p;
    }
  }
}

KfmGuiProps::~KfmGuiProps()
{
}

void KfmGuiProps::saveProps( KConfig * config )
{
  // config->writeEntry( "DirTree", m_bDirTree );
  config->writeEntry( "SplitView", m_bSplitView );
  
  if ( !m_bShowToolBar )
      config->writeEntry( "Toolbar", "hide" );
  else // why not a switch here ? David. 
      if ( m_toolBarPos == KToolBar::Top )
      config->writeEntry( "Toolbar", "top" );
  else if ( m_toolBarPos == KToolBar::Bottom )
      config->writeEntry( "Toolbar", "bottom" );
  else if ( m_toolBarPos == KToolBar::Left )
      config->writeEntry( "Toolbar", "left" );
  else if ( m_toolBarPos == KToolBar::Right )
      config->writeEntry( "Toolbar", "right" );
  else if ( m_toolBarPos == KToolBar::Floating )
      config->writeEntry( "Toolbar", "floating" );

  if ( !m_bShowLocationBar )
      config->writeEntry( "LocationBar", "hide" );
  else if ( m_locationBarPos == KToolBar::Top )
      config->writeEntry( "LocationBar", "top" );
  else if ( m_locationBarPos == KToolBar::Bottom )
      config->writeEntry( "LocationBar", "bottom" );
  else if ( m_locationBarPos == KToolBar::Floating )
      config->writeEntry( "LocationBar", "floating" );

  if ( !m_bShowStatusBar )
      config->writeEntry( "Statusbar", "hide" );
  else
      config->writeEntry( "Statusbar", "bottom" );

  if ( !m_bShowMenuBar )
      config->writeEntry( "Menubar", "hide" );
  else if ( m_menuBarPos == KMenuBar::Top )
      config->writeEntry( "Menubar", "top" );
  else if ( m_menuBarPos == KMenuBar::Bottom )
      config->writeEntry( "Menubar", "bottom" );
  else if ( m_menuBarPos == KMenuBar::Floating )
      config->writeEntry( "Menubar", "floating" );

  config->writeEntry( "WindowWidth", m_width );
  config->writeEntry( "WindowHeight", m_height );

  // FIXME pixmap missing

  config->sync();
}
