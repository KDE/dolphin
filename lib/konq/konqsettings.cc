/*  This file is part of the KDE project
    Copyright (C) 1997 David Faure <faure@kde.org>
 
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
#include "konqsettings.h"
#include "konqdefaults.h"
#include <kconfig.h>
#include <kapp.h>

// We have to handle three instances of this class in the code
// Everything's handled by arrays to avoid code duplication

KonqSettings * KonqSettings::s_pSettings[] = { 0L, 0L, 0L };

static const char * s_sGroupName[] = { "HTML Settings", "HTML Settings", "Desktop Settings" };
// TODO : update second group name to FM Settings if we decide to have
// a separate icon/html config
// (but then has to be done in kcmkonq of course)

#define KS_HTML 0
#define KS_FM 1
#define KS_DESKTOP 2

KonqSettings * KonqSettings::getInstance( int nr )
{
  if (!s_pSettings[nr])
  {
    KConfig *config = kapp->config();
    KConfigGroupSaver cgs(config, s_sGroupName[nr]);
    s_pSettings[nr] = new KonqSettings(config);
  }
  return s_pSettings[nr];
}

//static
inline KonqSettings * KonqSettings::defaultDesktopSettings() 
{
  return getInstance( KS_DESKTOP );
}

//static
inline KonqSettings * KonqSettings::defaultFMSettings() 
{
  return getInstance( KS_FM );
}

//static
inline KonqSettings * KonqSettings::defaultHTMLSettings() 
{
  return getInstance( KS_HTML );
}

//static
void KonqSettings::reparseConfiguration()
{
  KConfig *config = kapp->config();
  for (int i = 0 ; i < 3 ; i++ )
  {
    if (s_pSettings[i])
    {
      KConfigGroupSaver cgs(config, s_sGroupName[i]);
      s_pSettings[i]->init( config );
    }
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
  m_bSingleClick = config->readBoolEntry("SingleClick", DEFAULT_SINGLECLICK);
  m_iAutoSelect = config->readNumEntry("AutoSelect", DEFAULT_AUTOSELECT);
  m_bChangeCursor = config->readBoolEntry( "ChangeCursor", DEFAULT_CHANGECURSOR );
  m_underlineLink = config->readBoolEntry( "UnderlineLink", DEFAULT_UNDERLINELINKS );
  
  // Other
  config->setGroup( "Misc Defaults" ); // group will be restored by cgs anyway
  m_bAutoLoadImages = config->readBoolEntry( "AutoLoadImages", true );
}

KonqSettings::~KonqSettings()
{
}
