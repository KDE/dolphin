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
#include "konq_factory.h"
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

KonqPropsView * KonqPropsView::m_pDefaultProps = 0L;

// static
KonqPropsView * KonqPropsView::defaultProps()
{
  if (!m_pDefaultProps)
  {
    kDebugInfo(1202,"Reading global config for konq_propsview");
    KConfig *config = KonqFactory::instance()->config();
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

  // Default background color is the one from the settings, i.e. configured in kcmkonq
  // OUCH how do we know if this is icon view or tree view ????
  m_bgColor = KonqFMSettings::defaultIconSettings()->bgColor();

  m_bgPixmap = 0L;
  QString pix = config->readEntry( "BackgroundPixmap", "" );
  if ( !pix.isEmpty() )
  {
    QPixmap p = wallpaperPixmap( pix );
    if ( !p.isNull() )
    {
      kDebugInfo(1202,"Got background");
      m_bgPixmap = p;
    }
  }
}

KonqPropsView::~KonqPropsView()
{
}

bool KonqPropsView::enterDir( const KURL & dir )
{
  // Revert to default setting first
  m_bgPixmap = m_pDefaultProps->m_bgPixmap;
  m_bgColor = m_pDefaultProps->m_bgColor;
  // Check for .directory
  KURL u ( dir );
  u.addPath(".directory");
  if (u.isLocalFile() && QFile::exists( u.path() ))
  {
    //kDebugInfo( 1202, "Found .directory file" );
    KSimpleConfig config( u.path(), true);
    config.setGroup("URL properties");
    m_bgColor = config.readColorEntry( "BgColor", &m_bgColor );
    QString pix = config.readEntry( "BgImage", "" );
    if ( !pix.isEmpty() )
    {
      debug("BgImage is %s", debugString(pix));
      QPixmap p = wallpaperPixmap( pix );
      if ( !p.isNull() )
        m_bgPixmap = p;
      else debug("Wallpaper not found");
    }
  }
  return true;
}

void KonqPropsView::saveAsDefault()
{
  KConfig *config = KonqFactory::instance()->config();
  KConfigGroupSaver cgs(config, "Settings");
  saveProps( config );
}

void KonqPropsView::saveLocal( const KURL & dir )
{
  KURL u ( dir );
  u.addPath(".directory");
  if (!u.isLocalFile() || access (dir.path(), W_OK) != 0)
  {
    KMessageBox::sorry( 0L, i18n( QString("Can't write to %1").arg(dir.path()) ) );
    // Well in theory we could write even to some FTP dir, but not with KSimpleConfig :)
    return;
  }

  //kDebugInfo( 1202, "Found .directory file" );
  KSimpleConfig config( u.path());
  config.setGroup("URL properties");
  saveProps( & config );
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
  config->writeEntry( "BgColor", m_bgColor );
  // TODO save FILENAME for the BgImage...
  config->sync();
}

