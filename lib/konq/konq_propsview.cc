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
#include <konq_defaults.h>
#include <konq_settings.h>

#include <kdebug.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kpixmap.h>
#include <kmessagebox.h>
#include <qpixmapcache.h>
#include <qiconview.h>
#include <unistd.h>
#include <qfile.h>
#include <iostream>
#include <kinstance.h>
#include <assert.h>

#include <ksimpleconfig.h>

static QPixmap wallpaperPixmap( const QString & _wallpaper )
{
    QString key = "wallpapers/";
    key += _wallpaper;
    KPixmap pix;

    if ( QPixmapCache::find( key, pix ) )
      return pix;

    QString path = locate("tiles", _wallpaper);
    if (path.isEmpty())
        path = locate("wallpaper", _wallpaper);
    if (!path.isEmpty())
    {
      // This looks really ugly, especially on an 8bit display.
      // I'm not sure what it's good for.
      // Anyway, if you change it here, keep konq_bgnddlg in sync (David)
      // pix.load( path, 0, KPixmap::LowColor );
      pix.load( path );
      if ( pix.isNull() )
        kdWarning(1203) << "Could not load wallpaper " << path << endl;
      else
        QPixmapCache::insert( key, pix );
      return pix;
    } else kdWarning(1203) << "Couldn't locate wallpaper " << _wallpaper << endl;
    return QPixmap();
}

KonqPropsView::KonqPropsView( KInstance * instance, KonqPropsView * defaultProps )
    : m_bSaveViewPropertiesLocally( false ), // will be overriden by setSave... anyway
    // if this is the default properties instance, then keep config object for saving
    m_dotDirExists( true ), // HACK so that enterDir returns true initially
    m_currentConfig( defaultProps ? 0L : instance->config() ),
    m_defaultProps( defaultProps )
{
  KConfig *config = instance->config();
  KConfigGroupSaver cgs(config, "Settings");

  m_iIconSize = config->readNumEntry( "IconSize", 0 );
  m_iItemTextPos = config->readNumEntry( "ItemTextPos", QIconView::Bottom );
  m_bShowDot = config->readBoolEntry( "ShowDotFiles", false );
  m_preview = config->readListEntry( "Preview" );

  m_textColor = config->readColorEntry( "TextColor" ); // will be set to QColor() if not found
  m_bgColor = config->readColorEntry( "BgColor" ); // will be set to QColor() if not found
  m_bgPixmapFile = config->readEntry( "BgImage" );
  //kdDebug(1203) << "KonqPropsView::KonqPropsView from \"config\" : BgImage=" << m_bgPixmapFile << endl;

  // colorsConfig is either the local file (.directory) or the application global file
  // (we want the same colors for all types of view)
  // The code above reads from the view's config file, for compatibility only.
  // So now we read the settings from the app global file, if this is the default props
  if (!defaultProps)
  {
      KConfigGroupSaver cgs2(KGlobal::config(), "Settings");
      m_textColor = KGlobal::config()->readColorEntry( "TextColor", &m_textColor );
      m_bgColor = KGlobal::config()->readColorEntry( "BgColor", &m_bgColor );
      m_bgPixmapFile = KGlobal::config()->readEntry( "BgImage", m_bgPixmapFile );
      //kdDebug(1203) << "KonqPropsView::KonqPropsView from KGlobal : BgImage=" << m_bgPixmapFile << endl;
  }

  KGlobal::dirs()->addResourceType("tiles",
                                   KGlobal::dirs()->kde_default("data") + "konqueror/tiles/");
}

KConfigBase * KonqPropsView::currentConfig()
{
    if ( !m_currentConfig )
    {
        // 0L ? This has to be a non-default save-locally instance...
        assert ( m_bSaveViewPropertiesLocally );
        assert ( !isDefaultProperties() );

        if (!dotDirectory.isEmpty())
            m_currentConfig = new KSimpleConfig( dotDirectory );
        // the "else" is when we want to save locally but this is a remote URL -> no save
    }
    return m_currentConfig;
}

KConfigBase * KonqPropsView::currentColorConfig()
{
    // Saving locally ?
    if ( m_bSaveViewPropertiesLocally && !isDefaultProperties() )
        return currentConfig(); // Will create it if necessary
    else
        // Save color settings in app's file, not in view's file
        return KGlobal::config();
}

KonqPropsView::~KonqPropsView()
{
}

bool KonqPropsView::enterDir( const KURL & dir )
{
  //kdDebug(1203) << "enterDir " << dir.prettyURL() << endl;
  // Can't do that with default properties
  assert( !isDefaultProperties() );

  // Check for .directory
  KURL u ( dir );
  u.addPath(".directory");
  bool dotDirExists = u.isLocalFile() && QFile::exists( u.path() );
  dotDirectory = u.isLocalFile() ? u.path() : QString::null;

  // Revert to default setting first - unless there is no .directory
  // in the previous dir nor in this one (then we can keep the current settings)
  if (dotDirExists || m_dotDirExists)
  {
    m_iIconSize = m_defaultProps->iconSize();
    m_iItemTextPos = m_defaultProps->itemTextPos();
    m_bShowDot = m_defaultProps->isShowingDotFiles();
    m_preview = m_defaultProps->m_preview;
    m_textColor = m_defaultProps->m_textColor;
    m_bgColor = m_defaultProps->m_bgColor;
    m_bgPixmapFile = m_defaultProps->bgPixmapFile();
  }

  if (dotDirExists)
  {
    //kdDebug(1203) << "Found .directory file" << endl;
    KSimpleConfig * config = new KSimpleConfig( dotDirectory, true );
    config->setGroup("URL properties");

    m_iIconSize = config->readNumEntry( "IconSize", m_iIconSize );
    m_iItemTextPos = config->readNumEntry( "ItemTextPos", m_iItemTextPos );
    m_bShowDot = config->readBoolEntry( "ShowDotFiles", m_bShowDot );
    if (config->hasKey( "Preview" ))
        m_preview = config->readListEntry( "Preview" );

    m_textColor = config->readColorEntry( "TextColor", &m_textColor );
    m_bgColor = config->readColorEntry( "BgColor", &m_bgColor );
    m_bgPixmapFile = config->readEntry( "BgImage", m_bgPixmapFile );
    //kdDebug(1203) << "KonqPropsView::enterDir m_bgPixmapFile=" << m_bgPixmapFile << endl;
    delete config;
  }
  //if there is or was a .directory then the settings probably have changed
  bool configChanged=(m_dotDirExists|| dotDirExists);
  m_dotDirExists = dotDirExists;
  m_currentConfig = 0L; // new dir, not current config for saving yet
  //kdDebug(1203) << "KonqPropsView::enterDir returning " << configChanged << endl;
  return configChanged;
}

void KonqPropsView::setSaveViewPropertiesLocally( bool value )
{
    assert( !isDefaultProperties() );
    //kdDebug(1203) << "KonqPropsView::setSaveViewPropertiesLocally " << value << endl;

    if ( m_bSaveViewPropertiesLocally )
        delete m_currentConfig; // points to a KSimpleConfig

    m_bSaveViewPropertiesLocally = value;
    m_currentConfig = 0L; // mark as dirty
}

void KonqPropsView::setIconSize( int size )
{
    m_iIconSize = size;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
        m_defaultProps->setIconSize( size );
    else if (currentConfig())
    {
        KConfigGroupSaver cgs(currentConfig(), currentGroup());
        currentConfig()->writeEntry( "IconSize", m_iIconSize );
        currentConfig()->sync();
    }
}

void KonqPropsView::setItemTextPos( int pos )
{
    m_iItemTextPos = pos;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
        m_defaultProps->setItemTextPos( pos );
    else if (currentConfig())
    {
        KConfigGroupSaver cgs(currentConfig(), currentGroup());
        currentConfig()->writeEntry( "ItemTextPos", m_iItemTextPos );
        currentConfig()->sync();
    }
}

void KonqPropsView::setShowingDotFiles( bool show )
{
    kdDebug(1203) << "KonqPropsView::setShowingDotFiles " << show << endl;
    m_bShowDot = show;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
    {
        kdDebug(1203) << "Saving in default properties" << endl;
        m_defaultProps->setShowingDotFiles( show );
    }
    else if (currentConfig())
    {
        kdDebug(1203) << "Saving in current config" << endl;
        KConfigGroupSaver cgs(currentConfig(), currentGroup());
        currentConfig()->writeEntry( "ShowDotFiles", m_bShowDot );
        currentConfig()->sync();
    }
}

void KonqPropsView::setShowingPreview( const QString &preview, bool show )
{
    if ( m_preview.contains( preview ) == show )
        return;
    else if ( show )
        m_preview.append( preview );
    else
        m_preview.remove( preview );
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
        m_defaultProps->setShowingPreview( preview, show );
    else if (currentConfig())
    {
        KConfigGroupSaver cgs(currentConfig(), currentGroup());
        currentConfig()->writeEntry( "Preview", m_preview );
        currentConfig()->sync();
    }
}

void KonqPropsView::setBgColor( const QColor & color )
{
    m_bgColor = color;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
    {
        m_defaultProps->setBgColor( color );
    }
    else
    {
        KConfigBase * colorConfig = currentColorConfig();
        if (colorConfig) // 0L when saving locally but remote URL
        {
            KConfigGroupSaver cgs(colorConfig, currentGroup());
            colorConfig->writeEntry( "BgColor", m_bgColor );
            colorConfig->sync();
        }
    }
}

const QColor & KonqPropsView::bgColor( QWidget * widget ) const
{
    if ( !m_bgColor.isValid() )
        return widget->colorGroup().base();
    else
        return m_bgColor;
}

void KonqPropsView::setTextColor( const QColor & color )
{
    m_textColor = color;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
    {
        m_defaultProps->setTextColor( color );
    }
    else
    {
        KConfigBase * colorConfig = currentColorConfig();
        if (colorConfig) // 0L when saving locally but remote URL
        {
            KConfigGroupSaver cgs(colorConfig, currentGroup());
            colorConfig->writeEntry( "TextColor", m_textColor );
            colorConfig->sync();
        }
    }
}

const QColor & KonqPropsView::textColor( QWidget * widget ) const
{
    if ( !m_textColor.isValid() )
        return widget->colorGroup().text();
    else
        return m_textColor;
}

void KonqPropsView::setBgPixmapFile( const QString & file )
{
    m_bgPixmapFile = file;

    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
    {
        m_defaultProps->setBgPixmapFile( file );
    }
    else
    {
        KConfigBase * colorConfig = currentColorConfig();
        if (colorConfig) // 0L when saving locally but remote URL
        {
            KConfigGroupSaver cgs(colorConfig, currentGroup());
            colorConfig->writeEntry( "BgImage", file );
            colorConfig->sync();
        }
    }
}

QPixmap KonqPropsView::loadPixmap() const
{
    //kdDebug(1203) << "KonqPropsView::loadPixmap " << m_bgPixmapFile << endl;
    QPixmap bgPixmap;
    if ( !m_bgPixmapFile.isEmpty() )
        bgPixmap = wallpaperPixmap( m_bgPixmapFile );
    return bgPixmap;
}

void KonqPropsView::applyColors(QWidget * widget) const
{
    //kdDebug(1203) << "KonqPropsView::applyColors " << (void*)this << endl;
    QColorGroup a = widget->palette().active();
    QColorGroup d = widget->palette().disabled(); // is this one ever used ?
    QColorGroup i = widget->palette().inactive(); // is this one ever used ?
    bool setPaletteNeeded = false;

    if ( m_bgPixmapFile.isEmpty() )
    {
        if ( m_bgColor.isValid() )
        {
            a.setColor( QColorGroup::Base, m_bgColor );
            d.setColor( QColorGroup::Base, m_bgColor );
            i.setColor( QColorGroup::Base, m_bgColor );
            widget->setBackgroundColor( m_bgColor );
            setPaletteNeeded = true;
        }
    }
    else
    {
        widget->setBackgroundPixmap( loadPixmap() );
    }

    if ( m_textColor.isValid() )
    {
        a.setColor( QColorGroup::Text, m_textColor );
        d.setColor( QColorGroup::Text, m_textColor );
        i.setColor( QColorGroup::Text, m_textColor );
        setPaletteNeeded = true;
    }

    // Avoid calling setPalette if we are fine with the default values.
    // This makes us react to the palette-change event accordingly.
    if ( setPaletteNeeded )
        widget->setPalette( QPalette( a, d, i ) );
}
