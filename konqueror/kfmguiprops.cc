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

KfmGuiProps::KfmGuiProps( KConfig * config )
{
  QString entry = "LargeIcons"; // default
  m_leftViewMode = KfmView::HOR_ICONS;
  entry = config->readEntry("LeftViewMode", entry);
  if (entry == "TreeView")
    m_leftViewMode = KfmView::FINDER;
  if (entry == "SmallIcons")
    m_leftViewMode = KfmView::VERT_ICONS;
  if (entry == "HTMLView")
    m_leftViewMode = KfmView::HTML;

  m_rightViewMode = KfmView::HOR_ICONS;
  entry = config->readEntry("RightViewMode", entry);
  if (entry == "TreeView")
    m_rightViewMode = KfmView::FINDER;
  if (entry == "SmallIcons")
    m_rightViewMode = KfmView::VERT_ICONS;
  if (entry == "HTMLView")
    m_rightViewMode = KfmView::HTML;
  
  m_currentViewMode = m_rightViewMode;

  m_bLeftShowDot = config->readBoolEntry( "LeftShowDotFiles", false );
  m_bRightShowDot = config->readBoolEntry( "RightShowDotFiles", false );
  m_bCurrentShowDot = m_bRightShowDot;

  m_bLeftImagePreview = config->readBoolEntry( "LeftImagePreview", false );
  m_bRightImagePreview = config->readBoolEntry( "RightImagePreview", false );
  m_bCurrentImagePreview = m_bRightImagePreview;

  m_bDirTree = config->readBoolEntry( "DirTree", false );
  m_bSplitView = config->readBoolEntry( "SplitView", false );
  m_bCache = false; // What is it ???

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

  /////////// Other group, don't move beyond this limit ! ///////////

  config->setGroup( "KFM HTML Defaults" );

  m_iFontSize = config->readNumEntry( "FontSize", DEFAULT_VIEW_FONT_SIZE );
  if ( m_iFontSize < 8 )
    m_iFontSize = 8;
  else if ( m_iFontSize > 24 )
    m_iFontSize = 24;

  m_strStdFontName = config->readEntry( "StandardFont" );
  if ( m_strStdFontName.isEmpty() )
    m_strStdFontName = DEFAULT_VIEW_FONT;

  m_strFixedFontName = config->readEntry( "FixedFont" );
  if ( m_strFixedFontName.isEmpty() )
    m_strFixedFontName = DEFAULT_VIEW_FIXED_FONT;

  m_bChangeCursor = config->readBoolEntry( "ChangeCursor", true );

  entry = config->readEntry( "MouseMode" , "SingleClick");
  if ( entry == "SingleClick" )
    m_mouseMode = KfmAbstractGui::SingleClick;
  else
    m_mouseMode = KfmAbstractGui::DoubleClick;

  m_bgColor = config->readColorEntry( "BgColor", &HTML_DEFAULT_BG_COLOR );
  m_textColor = config->readColorEntry( "TextColor", &HTML_DEFAULT_TXT_COLOR );
  m_linkColor = config->readColorEntry( "LinkColor", &HTML_DEFAULT_LNK_COLOR );
  m_vLinkColor = config->readColorEntry( "VLinkColor", &HTML_DEFAULT_VLNK_COLOR);

  m_underlineLink = config->readBoolEntry( "UnderlineLink", true );
}

KfmGuiProps::~KfmGuiProps()
{
}

void KfmGuiProps::saveProps( KConfig * config )
{
  config->writeEntry( "DirTree", m_bDirTree );
  config->writeEntry( "SplitView", m_bSplitView );

  QString entry;
  switch ( m_leftViewMode )
    {
    case KfmView::FINDER:
      entry = "TreeView";
      break;
    case KfmView::VERT_ICONS:
      entry = "SmallIcons";
      break;
    case KfmView::HOR_ICONS:
      entry = "LargeIcons";
      break;
    default:
      assert( 0 );
      break;
    }
  config->writeEntry( "LeftViewMode", entry);

  switch ( m_rightViewMode )
    {
    case KfmView::FINDER:
      entry = "TreeView";
      break;
    case KfmView::VERT_ICONS:
      entry = "SmallIcons";
      break;
    case KfmView::HOR_ICONS:
      entry = "LargeIcons";
      break;
    default:
      assert( 0 );
      break;
    }
  config->writeEntry( "RightViewMode", entry);

  config->writeEntry( "RightShowDotFiles", m_bRightShowDot );
  config->writeEntry( "LeftShowDotFiles", m_bLeftShowDot );

  config->writeEntry( "RightImagePreview", m_bRightImagePreview );
  config->writeEntry( "LeftImagePreview", m_bLeftImagePreview );
  //  config->writeEntry( "AllowHTML", m_pView->isHTMLAllowed() ); FIXME
  
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
  
  config->sync();
}
