/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

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

#include "konqsettings.h"
#include "konqdefaults.h"
#include <kconfig.h>
#include <kapp.h>
#include <assert.h>

// We have to handle three instances of this class in the code
// Everything's handled by arrays to avoid code duplication

KonqFMSettings * KonqFMSettings::s_pSettings[] = { 0L, 0L };

static const char * s_sGroupName[] = { "Icon Settings", "Tree Settings" };

#define KS_ICON 0
#define KS_TREE 1
#define MAXINSTANCE KS_TREE  // last one

KonqFMSettings * KonqFMSettings::getInstance( int nr )
{
  assert( nr >= 0 && nr <= MAXINSTANCE );
  if (!s_pSettings[nr])
  {
    KConfig *config = kapp->config();
    KConfigGroupSaver cgs(config, s_sGroupName[nr]);
    s_pSettings[nr] = new KonqFMSettings(config);
  }
  return s_pSettings[nr];
}

//static
KonqFMSettings * KonqFMSettings::defaultTreeSettings()
{
  return getInstance( KS_TREE );
}

//static
KonqFMSettings * KonqFMSettings::defaultIconSettings()
{
  return getInstance( KS_ICON );
}

//static
void KonqFMSettings::reparseConfiguration()
{
  KConfig *config = kapp->config();
  for (int i = 0 ; i < MAXINSTANCE+1 ; i++ )
  {
    if (s_pSettings[i])
    {
      KConfigGroupSaver cgs(config, s_sGroupName[i]);
      s_pSettings[i]->init( config );
    }
  }
}

KonqFMSettings::KonqFMSettings( KConfig * config )
{
  init( config );
}

void KonqFMSettings::init( KConfig * config )
{
  // Fonts and colors
  m_iFontSize = config->readNumEntry( "FontSize", DEFAULT_VIEW_FONT_SIZE );
  if ( m_iFontSize < 8 )
    m_iFontSize = 8;
  else if ( m_iFontSize > 24 )
    m_iFontSize = 24;

  m_strStdFontName = config->readEntry( "StandardFont" );
  if ( m_strStdFontName.isEmpty() )
    m_strStdFontName = DEFAULT_VIEW_FONT;

  m_bgColor = config->readColorEntry( "BgColor", &FM_DEFAULT_BG_COLOR );
  m_normalTextColor = config->readColorEntry( "NormalTextColor", &FM_DEFAULT_TXT_COLOR );
  m_highlightedTextColor = config->readColorEntry( "HighlightedTextColor", &FM_DEFAULT_HIGHLIGHTED_TXT_COLOR );

  // Behaviour
  m_bSingleClick = config->readBoolEntry("SingleClick", DEFAULT_SINGLECLICK);
  m_iAutoSelect = config->readNumEntry("AutoSelect", DEFAULT_AUTOSELECT);
  m_bChangeCursor = config->readBoolEntry( "ChangeCursor", DEFAULT_CHANGECURSOR );
  m_underlineLink = config->readBoolEntry( "UnderlineLink", DEFAULT_UNDERLINELINKS );
  m_bWordWrapText = config->readBoolEntry( "WordWrapText", DEFAULT_WORDWRAPTEXT );

}
