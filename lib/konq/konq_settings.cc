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

#include "konq_settings.h"
#include "konq_defaults.h"
#include "kglobalsettings.h"
#include <kconfig.h>
#include <kglobal.h>
#include <kservicetype.h>
#include <kdesktopfile.h>
#include <kdebug.h>
#include <assert.h>

//static
KonqFMSettings * KonqFMSettings::s_pSettings = 0L;

//static
KonqFMSettings * KonqFMSettings::settings()
{
  if (!s_pSettings)
  {
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cgs(config, "FMSettings");
    s_pSettings = new KonqFMSettings(config);
  }
  return s_pSettings;
}

//static
void KonqFMSettings::reparseConfiguration()
{
  if (s_pSettings)
  {
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cgs(config, "FMSettings");
    s_pSettings->init( config );
  }
}

KonqFMSettings::KonqFMSettings( KConfig * config )
{
  init( config );
}

void KonqFMSettings::init( KConfig * config )
{
  // Fonts and colors
  m_standardFont = config->readFontEntry( "StandardFont" );

  m_normalTextColor = KGlobalSettings::textColor();
  m_normalTextColor = config->readColorEntry( "NormalTextColor", &m_normalTextColor );
  m_highlightedTextColor = KGlobalSettings::highlightedTextColor();
  m_highlightedTextColor = config->readColorEntry( "HighlightedTextColor", &m_highlightedTextColor );
  m_itemTextBackground = config->readColorEntry( "ItemTextBackground" );
  m_bWordWrapText = config->readBoolEntry( "WordWrapText", DEFAULT_WORDWRAPTEXT );
  m_underlineLink = config->readBoolEntry( "UnderlineLinks", DEFAULT_UNDERLINELINKS );
  m_fileSizeInBytes = config->readBoolEntry( "DisplayFileSizeInBytes", DEFAULT_FILESIZEINBYTES );
  m_iconTransparency = config->readNumEntry( "TextpreviewIconOpacity", DEFAULT_TEXTPREVIEW_ICONTRANSPARENCY );
  if ( m_iconTransparency < 0 || m_iconTransparency > 255 )
      m_iconTransparency = DEFAULT_TEXTPREVIEW_ICONTRANSPARENCY;

  // Behaviour
  m_alwaysNewWin = config->readBoolEntry( "AlwaysNewWin", FALSE );

  m_homeURL = config->readEntry("HomeURL", "~");

  m_embedMap = config->entryMap( "EmbedSettings" );
}

bool KonqFMSettings::shouldEmbed( const QString & serviceType ) const
{
    // First check in user's settings whether to embed or not
    // 1 - in the mimetype file itself
    KServiceType::Ptr serviceTypePtr = KServiceType::serviceType( serviceType );

    if ( serviceTypePtr )
    {
        kdDebug(1203) << serviceTypePtr->desktopEntryPath() << endl;
        KDesktopFile deFile( serviceTypePtr->desktopEntryPath(),
                             true /*readonly*/, "mime");
        if ( deFile.hasKey( "X-KDE-AutoEmbed" ) )
        {
            bool autoEmbed = deFile.readBoolEntry( "X-KDE-AutoEmbed" );
            kdDebug(1203) << "X-KDE-AutoEmbed set to " << (autoEmbed ? "true" : "false") << endl;
            return autoEmbed;
        } else
            kdDebug(1203) << "No X-KDE-AutoEmbed, looking for group" << endl;
    }
    // 2 - in the configuration for the group if nothing was found in the mimetype
    QString serviceTypeGroup = serviceType.left(serviceType.find("/"));
    kdDebug(1203) << "KonqFMSettings::shouldEmbed : serviceTypeGroup=" << serviceTypeGroup << endl;
    if ( serviceTypeGroup == "inode" || serviceTypeGroup == "Browser")
        return true; //always embed mimetype inode/* and Browser/*
    QMap<QString, QString>::ConstIterator it = m_embedMap.find( QString::fromLatin1("embed-")+serviceTypeGroup );
    if ( it == m_embedMap.end() )
        return (serviceTypeGroup!="application"); // embedding is true by default except for application/*
    kdDebug(1203) << "KonqFMSettings::shouldEmbed: " << it.data() << endl;
    return it.data() == QString::fromLatin1("true");
}
