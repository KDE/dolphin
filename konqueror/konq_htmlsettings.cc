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

#include "konq_htmlsettings.h"
#include "konqdefaults.h"
#include <kconfig.h>
#include <kapp.h>

KonqHTMLSettings * KonqHTMLSettings::s_HTMLSettings = 0L;

KonqHTMLSettings::KonqHTMLSettings()
{
  KConfig config ( "konquerorrc", true );
  config.setGroup( "HTML Settings" );
  init( & config );
}

void KonqHTMLSettings::init( KConfig * config )
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

  m_strFixedFontName = config->readEntry( "FixedFont" );
  if ( m_strFixedFontName.isEmpty() )
    m_strFixedFontName = DEFAULT_VIEW_FIXED_FONT;

  m_bgColor = config->readColorEntry( "BgColor", &HTML_DEFAULT_BG_COLOR );
  m_textColor = config->readColorEntry( "TextColor", &HTML_DEFAULT_TXT_COLOR );
  m_linkColor = config->readColorEntry( "LinkColor", &HTML_DEFAULT_LNK_COLOR );
  m_vLinkColor = config->readColorEntry( "VLinkColor", &HTML_DEFAULT_VLNK_COLOR);

  // Behaviour
  m_bChangeCursor = config->readBoolEntry( "ChangeCursor", DEFAULT_CHANGECURSOR );
  m_underlineLink = config->readBoolEntry( "UnderlineLink", DEFAULT_UNDERLINELINKS );

  // Other
  config->setGroup( "HTML Settings" ); // group will be restored by cgs anyway
  m_bAutoLoadImages = config->readBoolEntry( "AutoLoadImages", true );
  m_bEnableJava = config->readBoolEntry( "EnableJava", false );
  m_bEnableJavaScript = config->readBoolEntry( "EnableJavaScript", false );
  m_strJavaPath = config->readEntry( "JavaPath", "/usr/lib/jdk" );

}

//static
KonqHTMLSettings * KonqHTMLSettings::defaultHTMLSettings()
{
  if (!s_HTMLSettings)
    s_HTMLSettings = new KonqHTMLSettings();
  return s_HTMLSettings;
}

//static
void KonqHTMLSettings::reparseConfiguration()
{
  if ( s_HTMLSettings )
  {
    KConfig config ( "konquerorrc", true );
    config.setGroup( "HTML Settings" );
    s_HTMLSettings->init( &config );
  }
}

