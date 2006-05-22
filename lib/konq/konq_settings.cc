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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_settings.h"
#include "konq_defaults.h"
#include "kglobalsettings.h"
#include <kglobal.h>
#include <kmimetype.h>
#include <kdesktopfile.h>
#include <kdebug.h>
#include <assert.h>
#include <QFontMetrics>

struct KonqFMSettingsPrivate
{
    KonqFMSettingsPrivate() {
        showPreviewsInFileTips = true;
        m_renameIconDirectly = false;
    }

    bool showPreviewsInFileTips;
    bool m_renameIconDirectly;
    bool localeAwareCompareIsCaseSensitive;
    int m_iconTextWidth;
};

//static
KonqFMSettings * KonqFMSettings::s_pSettings = 0L;

//static
KonqFMSettings * KonqFMSettings::settings()
{
  if (!s_pSettings)
  {
    KConfig *config = KGlobal::config();
    KConfigGroup cgs(config, "FMSettings");
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
    KConfigGroup cgs(config, "FMSettings");
    s_pSettings->init( config );
  }
}

KonqFMSettings::KonqFMSettings( KConfig * config )
{
  d = new KonqFMSettingsPrivate;
  init( config );
}

KonqFMSettings::~KonqFMSettings()
{
  delete d;
}

void KonqFMSettings::init( KConfig * config )
{
  // Fonts and colors
  m_standardFont = config->readEntry( "StandardFont", QFont() );

  m_normalTextColor = KGlobalSettings::textColor();
  m_normalTextColor = config->readEntry( "NormalTextColor", m_normalTextColor );
  m_highlightedTextColor = KGlobalSettings::highlightedTextColor();
  m_highlightedTextColor = config->readEntry( "HighlightedTextColor", m_highlightedTextColor );
  m_itemTextBackground = config->readEntry( "ItemTextBackground", QColor() );

  d->m_iconTextWidth = config->readEntry( "TextWidth", DEFAULT_TEXTWIDTH );
  if ( d->m_iconTextWidth == DEFAULT_TEXTWIDTH )
    d->m_iconTextWidth = QFontMetrics(m_standardFont).width("0000000000");

  m_iconTextHeight = config->readEntry( "TextHeight", 0 );
  if ( m_iconTextHeight == 0 ) {
    if ( config->readEntry( "WordWrapText", QVariant(true )).toBool() )
      m_iconTextHeight = DEFAULT_TEXTHEIGHT;
    else
      m_iconTextHeight = 1;
  }
  m_bWordWrapText = ( m_iconTextHeight > 1 );

  m_underlineLink = config->readEntry( "UnderlineLinks", QVariant(DEFAULT_UNDERLINELINKS )).toBool();
  d->m_renameIconDirectly = config->readEntry( "RenameIconDirectly", QVariant(DEFAULT_RENAMEICONDIRECTLY )).toBool();
  m_fileSizeInBytes = config->readEntry( "DisplayFileSizeInBytes", QVariant(DEFAULT_FILESIZEINBYTES )).toBool();
  m_iconTransparency = config->readEntry( "TextpreviewIconOpacity", DEFAULT_TEXTPREVIEW_ICONTRANSPARENCY );
  if ( m_iconTransparency < 0 || m_iconTransparency > 255 )
      m_iconTransparency = DEFAULT_TEXTPREVIEW_ICONTRANSPARENCY;

  // Behaviour
  m_alwaysNewWin = config->readEntry( "AlwaysNewWin", QVariant(false )).toBool();

  m_homeURL = config->readPathEntry("HomeURL", "~");

  m_showFileTips = config->readEntry("ShowFileTips", QVariant(true)).toBool();
  d->showPreviewsInFileTips = config->readEntry("ShowPreviewsInFileTips", QVariant(true)).toBool();
  m_numFileTips = config->readEntry("FileTipsItems", 6);

  m_embedMap = config->entryMap( "EmbedSettings" );

  /// true if QString::localeAwareCompare is case sensitive (it usually isn't, when LC_COLLATE is set)
  d->localeAwareCompareIsCaseSensitive = QString( "a" ).localeAwareCompare( "B" ) > 0; // see #40131
}

bool KonqFMSettings::shouldEmbed( const QString & mimeType ) const
{
    // First check in user's settings whether to embed or not
    // 1 - in the mimetype file itself
    KMimeType::Ptr mimeTypePtr = KMimeType::mimeType( mimeType );
    bool hasLocalProtocolRedirect = false;
    if ( mimeTypePtr )
    {
        hasLocalProtocolRedirect = !mimeTypePtr->property( "X-KDE-LocalProtocol" ).toString().isEmpty();
        kDebug(1203) << mimeTypePtr->desktopEntryPath() << endl;
        KDesktopFile deFile( mimeTypePtr->desktopEntryPath(),
                             true /*readonly*/, "mime");
        if ( deFile.hasKey( "X-KDE-AutoEmbed" ) )
        {
            bool autoEmbed = deFile.readEntry( "X-KDE-AutoEmbed" , QVariant(false)).toBool();
            kDebug(1203) << "X-KDE-AutoEmbed set to " << (autoEmbed ? "true" : "false") << endl;
            return autoEmbed;
        } else
            kDebug(1203) << "No X-KDE-AutoEmbed, looking for group" << endl;
    }
    // 2 - in the configuration for the group if nothing was found in the mimetype
    QString mimeTypeGroup = mimeType.left(mimeType.indexOf("/"));
    kDebug(1203) << "KonqFMSettings::shouldEmbed : mimeTypeGroup=" << mimeTypeGroup << endl;
    if ( mimeTypeGroup == "inode" || mimeTypeGroup == "Browser" || mimeTypeGroup == "Konqueror" )
        return true; //always embed mimetype inode/*, Browser/* and Konqueror/*
    QMap<QString, QString>::ConstIterator it = m_embedMap.find( QString::fromLatin1("embed-")+mimeTypeGroup );
    if ( it != m_embedMap.end() ) {
        kDebug(1203) << "KonqFMSettings::shouldEmbed: " << it.value() << endl;
        return it.value() == QString::fromLatin1("true");
    }
    // 3 - if no config found, use default.
    // Note: if you change those defaults, also change kcontrol/filetypes/typeslistitem.cpp !
    // Embedding is false by default except for image/* and for zip, tar etc.
    if ( mimeTypeGroup == "image" || hasLocalProtocolRedirect )
        return true;
    return false;
}

bool KonqFMSettings::showPreviewsInFileTips() const
{
    return d->showPreviewsInFileTips;
}

bool KonqFMSettings::renameIconDirectly() const
{
    return d->m_renameIconDirectly;
}

int KonqFMSettings::caseSensitiveCompare( const QString& a, const QString& b ) const
{
    if ( d->localeAwareCompareIsCaseSensitive ) {
        return a.localeAwareCompare( b );
    }
    else // can't use localeAwareCompare, have to fallback to normal QString compare
        return a.compare( b );
}

int KonqFMSettings::iconTextWidth() const
{
    return d->m_iconTextWidth;
}
