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

#include "kfmviewprops.h"
#include "kfm_defaults.h"

KfmViewProps * KfmViewProps::m_pDefaultProps = 0L;

KfmViewProps::KfmViewProps( const KConfig * config )
{
  QString entry = "LargeIcons"; // default
  m_ViewMode = KfmView::HOR_ICONS;
  entry = config->readEntry("ViewMode", entry);
  if (entry == "SmallIcons")
    m_ViewMode = KfmView::VERT_ICONS;
  if (entry == "TreeView")
    m_ViewMode = KfmView::FINDER;
  if (entry == "HTMLView")
    m_ViewMode = KfmView::HTML;

  m_bShowDot = config->readBoolEntry( "ShowDotFiles", false );
  m_bImagePreview = config->readBoolEntry( "ImagePreview", false );
  // m_bCache = false; // What is it ???
}

KfmViewProps::~KfmViewProps()
{
}

void KfmViewProps::saveProps( KConfig * config )
{
  QString entry;
  switch ( m_ViewMode )
    {
    case KfmView::HOR_ICONS: entry = "LargeIcons"; break;
    case KfmView::FINDER: entry = "TreeView"; break;
    case KfmView::VERT_ICONS: entry = "SmallIcons"; break;
    case KfmView::HTML: entry = "HTMLView"; break;
    default: assert( 0 ); break;
    }
  config->writeEntry( "ViewMode", entry);

  config->writeEntry( "ShowDotFiles", m_bShowDot );
  config->writeEntry( "ImagePreview", m_bImagePreview );
  config->sync();
}

//////////////////// KfmViewSettings ///////////////////////////////

KfmViewSettings * KfmViewSettings::m_pDefaultFMSettings;
KfmViewSettings * KfmViewSettings::m_pDefaultHTMLSettings;

KfmViewSettings::KfmViewSettings( const KConfig * config )
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

  m_bChangeCursor = config->readBoolEntry( "ChangeCursor", true );

  QString entry = config->readEntry( "MouseMode" , "SingleClick");
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

KfmViewSettings::~KfmViewSettings()
{
}

void KfmViewSettings::saveProps( KConfig * config )
{
  // TODO !

  config->sync();
}
