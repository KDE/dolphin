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
#include <konqdefaults.h>
#include <konqsettings.h>

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

    QString path = locate("wallpaper", _wallpaper);
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
  m_bImagePreview = config->readBoolEntry( "ImagePreview", false );

  m_bgColor = config->readColorEntry( "BgColor", & Qt::color0 /* default */ );
  m_bgPixmapFile = config->readEntry( "BgImage", "" );
  loadPixmap();
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
    }
    return m_currentConfig;
}

KonqPropsView::~KonqPropsView()
{
}

bool KonqPropsView::enterDir( const KURL & dir )
{
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
    m_bImagePreview = m_defaultProps->isShowingImagePreview();
    m_bgColor = m_defaultProps->m_bgColor;
    m_bgPixmap = m_defaultProps->bgPixmap();
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
    m_bImagePreview = config->readBoolEntry( "ImagePreview", m_bImagePreview );

    m_bgColor = config->readColorEntry( "BgColor", &m_bgColor );
    m_bgPixmapFile = config->readEntry( "BgImage", "" );
    loadPixmap();
    delete config;
  }
  //if there is or was a .directory then the settings probably have changed
  bool configChanged=(m_dotDirExists|| dotDirExists);
  m_dotDirExists = dotDirExists;
  m_currentConfig = 0L; // new dir, not current config for saving yet
  return configChanged;
}

void KonqPropsView::setSaveViewPropertiesLocally( bool value )
{
    assert( !isDefaultProperties() );
    kdDebug(1203) << "KonqPropsView::setSaveViewPropertiesLocally " << value << endl;

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

void KonqPropsView::setShowingImagePreview( bool show )
{
    m_bImagePreview = show;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
        m_defaultProps->setShowingImagePreview( show );
    else if (currentConfig())
    {
        KConfigGroupSaver cgs(currentConfig(), currentGroup());
        currentConfig()->writeEntry( "ImagePreview", m_bImagePreview );
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
    else if (currentConfig())
    {
        KConfigGroupSaver cgs(currentConfig(), currentGroup());
        currentConfig()->writeEntry( "BgColor", m_bgColor );
        currentConfig()->sync();
    }
}

const QColor & KonqPropsView::bgColor( QWidget * widget ) const
{
  if ( m_bgColor == Qt::color0 )
    return widget->colorGroup().base();
  else
    return m_bgColor;
}

void KonqPropsView::setBgPixmapFile( const QString & file )
{
    m_bgPixmapFile = file;
    loadPixmap();

    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
    {
        m_defaultProps->setBgPixmapFile( file );
    }
    else if (currentConfig())
    {
        KConfigGroupSaver cgs(currentConfig(), currentGroup());
        currentConfig()->writeEntry( "BgImage", file );
        currentConfig()->sync();
    }
}

void KonqPropsView::loadPixmap()
{
  m_bgPixmap.resize(0,0);
  if ( !m_bgPixmapFile.isEmpty() )
  {
    QPixmap p = wallpaperPixmap( m_bgPixmapFile );
    if ( !p.isNull() )
      m_bgPixmap = p;
  }
}

