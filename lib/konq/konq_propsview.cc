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
#include <unistd.h>
#include <qfile.h>
#include <iostream>
#include <kinstance.h>
#include <assert.h>

#include <ksimpleconfig.h>

QPixmap wallpaperPixmap( const char *_wallpaper )
{
    QString key = "wallpapers/";
    key += _wallpaper;
    KPixmap pix;

    if ( QPixmapCache::find( key, pix ) )
      return pix;

    QString path = locate("wallpaper", _wallpaper);
    if (!path.isEmpty())
    {
      pix.load( path, 0, KPixmap::LowColor ); // ?
      if ( pix.isNull() )
        debug("Wallpaper %s couldn't be loaded",path.ascii());
      else
        QPixmapCache::insert( key, pix );
      return pix;
    } else debug("Wallpaper %s not found",_wallpaper);
    return QPixmap();
}

KonqPropsView::KonqPropsView( KInstance * instance, KonqPropsView * defaultProps )
    : m_bSaveViewPropertiesLocally( false ), // will be overriden by setSave... anyway
    // if this is the default properties instance, then keep config object for saving
    m_dotDirExists( false ),
    m_currentConfig( defaultProps ? 0L : instance->config() ),
    m_defaultProps( defaultProps )
{
  KConfig *config = instance->config();
  KConfigGroupSaver cgs(config, "Settings");

  m_bShowDot = config->readBoolEntry( "ShowDotFiles", false );
  m_bImagePreview = config->readBoolEntry( "ImagePreview", false );
  m_bHTMLAllowed = config->readBoolEntry( "HTMLAllowed", false );
  //m_sViewMode = config->readEntry( "ViewMode", "KonqKfmIconView" );

  // Default background color is the one from the settings, i.e. configured in kcmkonq
  // TODO: remove it from there ?
  m_bgColor = KonqFMSettings::settings()->bgColor();

  m_bgPixmap.resize(0,0);
  m_bgPixmapFile = config->readEntry( "BackgroundPixmap", "" );
  if ( !m_bgPixmapFile.isEmpty() )
  {
    QPixmap p = wallpaperPixmap( m_bgPixmapFile );
    if ( !p.isNull() )
    {
      kdDebug(1203) << "Got background" << endl;
      m_bgPixmap = p;
    }
  }
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

void KonqPropsView::enterDir( const KURL & dir )
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
    m_bShowDot = m_defaultProps->isShowingDotFiles();
    m_bImagePreview = m_defaultProps->isShowingImagePreview();
    m_bHTMLAllowed = m_defaultProps->isHTMLAllowed();
    m_bgColor = m_defaultProps->bgColor();
    m_bgPixmap = m_defaultProps->bgPixmap();
    m_bgPixmapFile = m_defaultProps->bgPixmapFile();
    //m_sViewMode = m_defaultProps->viewMode();
  }

  if (dotDirExists)
  {
    //kdDebug(1203) << "Found .directory file" << endl;
    KSimpleConfig * config = new KSimpleConfig( dotDirectory, true );
    config->setGroup("URL properties");

    m_bShowDot = config->readBoolEntry( "ShowDotFiles", m_bShowDot );
    m_bImagePreview = config->readBoolEntry( "ImagePreview", m_bImagePreview );
    m_bHTMLAllowed = config->readBoolEntry( "HTMLAllowed", m_bHTMLAllowed );
    //m_sViewMode = config->readEntry( "ViewMode", m_sViewMode );

    m_bgColor = config->readColorEntry( "BgColor", &m_bgColor );
    m_bgPixmapFile = config->readEntry( "BgImage", "" );
    if ( !m_bgPixmapFile.isEmpty() )
    {
        QPixmap p = wallpaperPixmap( m_bgPixmapFile );
        if ( !p.isNull() )
            m_bgPixmap = p;
        else debug("Wallpaper not found");
    }
    delete config;
  }
  m_dotDirExists = dotDirExists;
  m_currentConfig = 0L; // new dir, not current config for saving yet
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

void KonqPropsView::setHTMLAllowed( bool allowed )
{
    m_bHTMLAllowed = allowed;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
        m_defaultProps->setHTMLAllowed( allowed );
    else if (currentConfig())
    {
        KConfigGroupSaver cgs(currentConfig(), currentGroup());
        currentConfig()->writeEntry( "HTMLAllowed", m_bHTMLAllowed );
        currentConfig()->sync();
    }
}

// TODO setBgColor, setBgPixmapFile

