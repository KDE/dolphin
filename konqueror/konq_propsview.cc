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

#include "konq_propsview.h"
#include "konq_defaults.h"

#include <kapp.h>
#include <kdebug.h>
#include <kpixmapcache.h>
#include <iostream>

KonqPropsView * KonqPropsView::m_pDefaultProps = 0L;

// static
KonqPropsView * KonqPropsView::defaultProps()
{
  if (!m_pDefaultProps)
  {
    kdebug(0,1202,"Reading global config for konq_propsview");
    KConfig *config = kapp->getConfig();
    KConfigGroupSaver cgs(config, "Settings");
    m_pDefaultProps = new KonqPropsView(config);
  }
  return m_pDefaultProps;
}

KonqPropsView::KonqPropsView( KConfig * config )
{
  QString entry = "LargeIcons"; // default
/*  m_viewMode = KfmView::HOR_ICONS;
  entry = config->readEntry("ViewMode", entry);
  if (entry == "SmallIcons")
    m_viewMode = KfmView::VERT_ICONS;
  if (entry == "TreeView")
    m_viewMode = KfmView::FINDER;
  if (entry == "HTMLView")
    m_viewMode = KfmView::HTML;
*/
  m_bShowDot = config->readBoolEntry( "ShowDotFiles", false );
  m_bImagePreview = config->readBoolEntry( "ImagePreview", false );
  m_bHTMLAllowed = config->readBoolEntry( "HTMLAllowed", false );
  // m_bCache = false; // What is it ???

  QString pix = config->readEntry( "BackgroundPixmap", "" );
  if ( !pix.isEmpty() )
  {
    QPixmap* p = KPixmapCache::wallpaperPixmap( pix );
    if ( p )
    {
     kdebug(0,1202,"Got background");
      m_bgPixmap = *p;
    }
  }
}

KonqPropsView::~KonqPropsView()
{
}

void KonqPropsView::saveProps( KConfig * config )
{
  QString entry;
/*  switch ( m_viewMode )
    {
    case KfmView::HOR_ICONS: entry = "LargeIcons"; break;
    case KfmView::FINDER: entry = "TreeView"; break;
    case KfmView::VERT_ICONS: entry = "SmallIcons"; break;
    case KfmView::HTML: entry = "HTMLView"; break;
    default: assert( 0 ); break;
    }
  config->writeEntry( "ViewMode", entry);
*/
  config->writeEntry( "ShowDotFiles", m_bShowDot );
  config->writeEntry( "ImagePreview", m_bImagePreview );
  config->writeEntry( "HTMLAllowed", m_bHTMLAllowed );
  config->sync();
}

//////////////////// KonqSettings ///////////////////////////////

KonqSettings * KonqSettings::m_pDefaultFMSettings = 0L;
KonqSettings * KonqSettings::m_pDefaultHTMLSettings = 0L;

#define HTMLGROUP "KFM HTML Defaults"
#define FMGROUP   "KFM HTML Defaults" // FM Defaults when kcmkonq supports it

//static
KonqSettings * KonqSettings::defaultFMSettings() 
{
  if (!m_pDefaultFMSettings)
  {
    KConfig *config = kapp->getConfig();
    KConfigGroupSaver cgs(config, FMGROUP);
    m_pDefaultFMSettings = new KonqSettings(config);
  }
  return m_pDefaultFMSettings;
}

//static
KonqSettings * KonqSettings::defaultHTMLSettings() 
{
  if (!m_pDefaultHTMLSettings)
  {
    KConfig *config = kapp->getConfig();
    KConfigGroupSaver cgs(config, HTMLGROUP);
    m_pDefaultHTMLSettings = new KonqSettings(config);
  }
  return m_pDefaultHTMLSettings;
}

//static
void KonqSettings::reparseConfiguration()
{
  if (m_pDefaultHTMLSettings) 
  {
    KConfig *config = kapp->getConfig();
    KConfigGroupSaver cgs(config, HTMLGROUP);
    m_pDefaultHTMLSettings->init( config );
  }
  if (m_pDefaultFMSettings) 
  {
    KConfig *config = kapp->getConfig();
    KConfigGroupSaver cgs(config, FMGROUP);
    m_pDefaultFMSettings->init( config );
  }
}

KonqSettings::KonqSettings( KConfig * config )
{
  init( config );
}

void KonqSettings::init( KConfig * config )
{
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

  m_bgColor = config->readColorEntry( "BgColor", &HTML_DEFAULT_BG_COLOR );
  m_textColor = config->readColorEntry( "TextColor", &HTML_DEFAULT_TXT_COLOR );
  m_linkColor = config->readColorEntry( "LinkColor", &HTML_DEFAULT_LNK_COLOR );
  m_vLinkColor = config->readColorEntry( "VLinkColor", &HTML_DEFAULT_VLNK_COLOR);

  // Behaviour
  KConfigGroupSaver cgs( config, "Behaviour" );
  m_bSingleClick = config->readBoolEntry("SingleClick", true);
  m_iAutoSelect = config->readNumEntry("AutoSelect", 50);
  m_bChangeCursor = config->readBoolEntry( "ChangeCursor", false );
  m_underlineLink = config->readBoolEntry( "UnderlineLink", true );
  
  // Other
  config->setGroup( "Misc Defaults" ); // group will be restored by cgs anyway
  m_bAutoLoadImages = config->readBoolEntry( "AutoLoadImages", true );
}

KonqSettings::~KonqSettings()
{
}
