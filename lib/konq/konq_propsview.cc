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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_propsview.h"
#include "konq_settings.h"

#include <kdebug.h>
#include <kstandarddirs.h>
#include <qpixmapcache.h>
#include <q3iconview.h>
//Added by qt3to4:
#include <QPixmap>
#include <unistd.h>
#include <QFile>
#include <iostream>
#include <ktrader.h>
#include <kinstance.h>
#include <assert.h>

#include <ksimpleconfig.h>

static QPixmap wallpaperPixmap( const QString & _wallpaper )
{
    QString key = "wallpapers/";
    key += _wallpaper;
    QPixmap pix;

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
      pix.load( path );
      if ( pix.isNull() )
        kWarning(1203) << "Could not load wallpaper " << path << endl;
      else
        QPixmapCache::insert( key, pix );
      return pix;
    } else kWarning(1203) << "Couldn't locate wallpaper " << _wallpaper << endl;
    return QPixmap();
}

struct KonqPropsView::Private
{
   QStringList* previewsToShow;
   bool previewsEnabled:1;
   bool caseInsensitiveSort:1;
   bool dirsfirst:1;
   bool descending:1;
   QString sortcriterion;
};

KonqPropsView::KonqPropsView( KInstance * instance, KonqPropsView * defaultProps )
    : m_bSaveViewPropertiesLocally( false ), // will be overridden by setSave... anyway
    // if this is the default properties instance, then keep config object for saving
    m_dotDirExists( true ), // HACK so that enterDir returns true initially
    m_currentConfig( defaultProps ? 0L : instance->config() ),
    m_defaultProps( defaultProps )
{

  KConfig *config = instance->config();
  KConfigGroup cgs(config, "Settings");

  d = new Private;
  d->previewsToShow = 0;
  d->caseInsensitiveSort=cgs.readEntry( "CaseInsensitiveSort", QVariant(true )).toBool();

  m_iIconSize = cgs.readEntry( "IconSize", 0 );
  m_iItemTextPos = cgs.readEntry( "ItemTextPos", int(Qt::DockBottom) );
  d->sortcriterion = cgs.readEntry( "SortingCriterion", "sort_nci" );
  d->dirsfirst = cgs.readEntry( "SortDirsFirst", true );
  d->descending = cgs.readEntry( "SortDescending", false );
  m_bShowDot = cgs.readEntry( "ShowDotFiles", false );
  m_bShowDirectoryOverlays = cgs.readEntry( "ShowDirectoryOverlays", QVariant(false )).toBool();

  m_dontPreview = cgs.readEntry( "DontPreview" , QStringList() );
  m_dontPreview.removeAll("audio/"); //Use the separate setting.
  //We default to this off anyway, so it's no harm to remove this

  //The setting for sound previews is stored separately, so we can force
  //the default-to-off bias to propagate up.
  if (!cgs.readEntry("EnableSoundPreviews", QVariant(false)).toBool())
  {
    if (!m_dontPreview.contains("audio/"))
      m_dontPreview.append("audio/");
  }

  d->previewsEnabled = cgs.readEntry( "PreviewsEnabled", QVariant(true )).toBool();

  QColor tc = KonqFMSettings::settings()->normalTextColor();
  m_textColor = cgs.readEntry( "TextColor", tc );
  m_bgColor = cgs.readEntry( "BgColor",QColor() ); // will be set to QColor() if not found
  m_bgPixmapFile = cgs.readPathEntry( "BgImage" );
  //kDebug(1203) << "KonqPropsView::KonqPropsView from \"config\" : BgImage=" << m_bgPixmapFile << endl;

  // colorsConfig is either the local file (.directory) or the application global file
  // (we want the same colors for all types of view)
  // The code above reads from the view's config file, for compatibility only.
  // So now we read the settings from the app global file, if this is the default props
  if (!defaultProps)
  {
      KConfigGroup cgs2(KGlobal::config(), "Settings");
      m_textColor = KGlobal::config()->readEntry( "TextColor", m_textColor );
      m_bgColor = KGlobal::config()->readEntry( "BgColor", m_bgColor );
      m_bgPixmapFile = KGlobal::config()->readPathEntry( "BgImage", m_bgPixmapFile );
      //kDebug(1203) << "KonqPropsView::KonqPropsView from KGlobal : BgImage=" << m_bgPixmapFile << endl;
  }

  KGlobal::dirs()->addResourceType("tiles",
                                   KGlobal::dirs()->kde_default("data") + "konqueror/tiles/");
}

bool KonqPropsView::isCaseInsensitiveSort() const
{
   return d->caseInsensitiveSort;
}

bool KonqPropsView::isDirsFirst() const
{
   return d->dirsfirst;
}

bool KonqPropsView::isDescending() const
{
   return d->descending;
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
   delete d->previewsToShow;
   delete d;
   d=0;
}

bool KonqPropsView::enterDir( const KUrl & dir )
{
  //kDebug(1203) << "enterDir " << dir.prettyUrl() << endl;
  // Can't do that with default properties
  assert( !isDefaultProperties() );

  // Check for .directory
  KUrl u ( dir );
  u.addPath(".directory");
  bool dotDirExists = u.isLocalFile() && QFile::exists( u.path() );
  dotDirectory = u.isLocalFile() ? u.path() : QString();

  // Revert to default setting first - unless there is no .directory
  // in the previous dir nor in this one (then we can keep the current settings)
  if (dotDirExists || m_dotDirExists)
  {
    m_iIconSize = m_defaultProps->iconSize();
    m_iItemTextPos = m_defaultProps->itemTextPos();
    d->sortcriterion = m_defaultProps->sortCriterion();
    d->dirsfirst = m_defaultProps->isDirsFirst();
    d->descending = m_defaultProps->isDescending();
    m_bShowDot = m_defaultProps->isShowingDotFiles();
    d->caseInsensitiveSort=m_defaultProps->isCaseInsensitiveSort();
    m_dontPreview = m_defaultProps->m_dontPreview;
    m_textColor = m_defaultProps->m_textColor;
    m_bgColor = m_defaultProps->m_bgColor;
    m_bgPixmapFile = m_defaultProps->bgPixmapFile();
  }

  if (dotDirExists)
  {
    //kDebug(1203) << "Found .directory file" << endl;
    KSimpleConfig * config = new KSimpleConfig( dotDirectory, true );
    config->setGroup("URL properties");

    m_iIconSize = config->readEntry( "IconSize", m_iIconSize );
    m_iItemTextPos = config->readEntry( "ItemTextPos", m_iItemTextPos );
    d->sortcriterion = config->readEntry( "SortingCriterion" , d->sortcriterion );
    d->dirsfirst = config->readEntry( "SortDirsFirst", QVariant(d->dirsfirst )).toBool();
    d->descending = config->readEntry( "SortDescending", QVariant(d->descending )).toBool();
    m_bShowDot = config->readEntry( "ShowDotFiles", QVariant(m_bShowDot )).toBool();
    d->caseInsensitiveSort=config->readEntry("CaseInsensitiveSort", QVariant(d->caseInsensitiveSort)).toBool();
    m_bShowDirectoryOverlays = config->readEntry( "ShowDirectoryOverlays", QVariant(m_bShowDirectoryOverlays )).toBool();
    if (config->hasKey( "DontPreview" ))
    {
        m_dontPreview = config->readEntry( "DontPreview" , QStringList() );

        //If the .directory file says something about sound previews,
        //obey it, otherwise propagate the setting up from the defaults
        //All this really should be split into a per-thumbnail setting,
        //but that's too invasive at this point
        if (config->hasKey("EnableSoundPreviews"))
        {

            if (!config->readEntry("EnableSoundPreviews", QVariant(false)).toBool())
                if (!m_dontPreview.contains("audio/"))
                    m_dontPreview.append("audio/");
        }
        else
        {
            if (m_defaultProps->m_dontPreview.contains("audio/"))
                if (!m_dontPreview.contains("audio/"))
                    m_dontPreview.append("audio/");
        }
    }



    m_textColor = config->readEntry( "TextColor", m_textColor );
    m_bgColor = config->readEntry( "BgColor", m_bgColor );
    m_bgPixmapFile = config->readPathEntry( "BgImage", m_bgPixmapFile );
    //kDebug(1203) << "KonqPropsView::enterDir m_bgPixmapFile=" << m_bgPixmapFile << endl;
    d->previewsEnabled = config->readEntry( "PreviewsEnabled", QVariant(d->previewsEnabled )).toBool();
    delete config;
  }
  //if there is or was a .directory then the settings probably have changed
  bool configChanged=(m_dotDirExists|| dotDirExists);
  m_dotDirExists = dotDirExists;
  m_currentConfig = 0L; // new dir, not current config for saving yet
  //kDebug(1203) << "KonqPropsView::enterDir returning " << configChanged << endl;
  return configChanged;
}

void KonqPropsView::setSaveViewPropertiesLocally( bool value )
{
    assert( !isDefaultProperties() );
    //kDebug(1203) << "KonqPropsView::setSaveViewPropertiesLocally " << value << endl;

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
        KConfigGroup cgs(currentConfig(), currentGroup());
        cgs.writeEntry( "IconSize", m_iIconSize );
        cgs.sync();
    }
}

void KonqPropsView::setItemTextPos( int pos )
{
    m_iItemTextPos = pos;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
        m_defaultProps->setItemTextPos( pos );
    else if (currentConfig())
    {
        KConfigGroup cgs(currentConfig(), currentGroup());
        cgs.writeEntry( "ItemTextPos", m_iItemTextPos );
        cgs.sync();
    }
}

void KonqPropsView::setSortCriterion( const QString &criterion )
{
    d->sortcriterion = criterion;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
        m_defaultProps->setSortCriterion( criterion );
    else if (currentConfig())
    {
        KConfigGroup cgs(currentConfig(), currentGroup());
        cgs.writeEntry( "SortingCriterion", d->sortcriterion );
        cgs.sync();
    }
}

void KonqPropsView::setDirsFirst( bool first)
{
    d->dirsfirst = first;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
        m_defaultProps->setDirsFirst( first );
    else if (currentConfig())
    {
        KConfigGroup cgs(currentConfig(), currentGroup());
        cgs.writeEntry( "SortDirsFirst", d->dirsfirst );
        cgs.sync();
    }
}

void KonqPropsView::setDescending( bool descend)
{
    d->descending = descend;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
        m_defaultProps->setDescending( descend );
    else if (currentConfig())
    {
        KConfigGroup cgs(currentConfig(), currentGroup());
        cgs.writeEntry( "SortDescending", d->descending );
        cgs.sync();
    }
}

void KonqPropsView::setShowingDotFiles( bool show )
{
    kDebug(1203) << "KonqPropsView::setShowingDotFiles " << show << endl;
    m_bShowDot = show;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
    {
        kDebug(1203) << "Saving in default properties" << endl;
        m_defaultProps->setShowingDotFiles( show );
    }
    else if (currentConfig())
    {
        kDebug(1203) << "Saving in current config" << endl;
        KConfigGroup cgs(currentConfig(), currentGroup());
        cgs.writeEntry( "ShowDotFiles", m_bShowDot );
        cgs.sync();
    }
}

void KonqPropsView::setCaseInsensitiveSort( bool on )
{
    kDebug(1203) << "KonqPropsView::setCaseInsensitiveSort " << on << endl;
    d->caseInsensitiveSort = on;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
    {
        kDebug(1203) << "Saving in default properties" << endl;
        m_defaultProps->setCaseInsensitiveSort( on );
    }
    else if (currentConfig())
    {
        kDebug(1203) << "Saving in current config" << endl;
        KConfigGroup cgs(currentConfig(), currentGroup());
        cgs.writeEntry( "CaseInsensitiveSort", d->caseInsensitiveSort );
        cgs.sync();
    }
}

void KonqPropsView::setShowingDirectoryOverlays( bool show )
{
    kDebug(1203) << "KonqPropsView::setShowingDirectoryOverlays " << show << endl;
    m_bShowDirectoryOverlays = show;
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
    {
        kDebug(1203) << "Saving in default properties" << endl;
        m_defaultProps->setShowingDirectoryOverlays( show );
    }
    else if (currentConfig())
    {
        kDebug(1203) << "Saving in current config" << endl;
        KConfigGroup cgs(currentConfig(), currentGroup());
        cgs.writeEntry( "ShowDirectoryOverlays", m_bShowDirectoryOverlays );
        cgs.sync();
    }
}

void KonqPropsView::setShowingPreview( const QString &preview, bool show )
{
    if ( m_dontPreview.contains( preview ) != show )
        return;
    else if ( show )
        m_dontPreview.removeAll( preview );
    else
        m_dontPreview.append( preview );
    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
        m_defaultProps->setShowingPreview( preview, show );
    else if (currentConfig())
    {
        KConfigGroup cgs(currentConfig(), currentGroup());

        //Audio is special-cased, as we use a binary setting
        //for it to get it to follow the defaults right.
        bool audioEnabled = !m_dontPreview.contains("audio/");

        //Don't write it out into the DontPreview line
        if (!audioEnabled)
            m_dontPreview.removeAll("audio/");
        cgs.writeEntry( "DontPreview", m_dontPreview );
        cgs.writeEntry( "EnableSoundPreviews", audioEnabled );
        cgs.sync();
        if (!audioEnabled)
            m_dontPreview.append("audio/");

    }

    delete d->previewsToShow;
    d->previewsToShow = 0;
}

void KonqPropsView::setShowingPreview( bool show )
{
    d->previewsEnabled = show;

    if ( m_defaultProps && !m_bSaveViewPropertiesLocally )
    {
        kDebug(1203) << "Saving in default properties" << endl;
        m_defaultProps-> setShowingPreview( show );
    }
    else if (currentConfig())
    {
        kDebug(1203) << "Saving in current config" << endl;
        KConfigGroup cgs(currentConfig(), currentGroup());
        cgs.writeEntry( "PreviewsEnabled", d->previewsEnabled );
        cgs.sync();
    }

    delete d->previewsToShow;
    d->previewsToShow = 0;
}

bool KonqPropsView::isShowingPreview()
{
    return d->previewsEnabled;
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
            KConfigGroup cgs(colorConfig, currentGroup());
          	cgs.writeEntry( "BgColor", m_bgColor );
         	cgs.sync();
        }
    }
}

const QColor & KonqPropsView::bgColor( QWidget * widget ) const
{
    if ( !m_bgColor.isValid() )
        return widget->palette().base().color();
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
            KConfigGroup cgs(colorConfig, currentGroup());
            cgs.writeEntry( "TextColor", m_textColor );
            cgs.sync();
        }
    }
}

const QColor & KonqPropsView::textColor( QWidget * widget ) const
{
    if ( !m_textColor.isValid() )
        return widget->palette().text().color();
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
            KConfigGroup cgs(colorConfig, currentGroup());
            cgs.writePathEntry( "BgImage", file );
            cgs.sync();
        }
    }
}

QPixmap KonqPropsView::loadPixmap() const
{
    //kDebug(1203) << "KonqPropsView::loadPixmap " << m_bgPixmapFile << endl;
    QPixmap bgPixmap;
    if ( !m_bgPixmapFile.isEmpty() )
        bgPixmap = wallpaperPixmap( m_bgPixmapFile );
    return bgPixmap;
}

void KonqPropsView::applyColors(QWidget * widget) const
{
    QPalette palette = widget->palette();

    if ( m_bgPixmapFile.isEmpty() )
    {
        palette.setColor( widget->backgroundRole(), bgColor( widget ) );
    }
    else
    {
        QPixmap pix = loadPixmap();
        // don't set an null pixmap, as this leads to
        // undefined results with regards to the background of widgets
        // that have the iconview as a parent and on the iconview itself
        // e.g. the rename textedit widget when renaming a QIconViewItem
        // Qt-issue: N64698
        if ( ! pix.isNull() )
            palette.setBrush( widget->backgroundRole(), QBrush( pix ) );
        // setPaletteBackgroundPixmap leads to flicker on window activation(!)
    }

    if ( m_textColor.isValid() )
        palette.setColor( widget->foregroundRole(), textColor( widget ) );

    widget->setPalette( palette );
}

const QStringList& KonqPropsView::previewSettings()
{
    if ( ! d->previewsToShow )
    {
        d->previewsToShow = new QStringList;

        if (d->previewsEnabled) {
            KTrader::OfferList plugins = KTrader::self()->query( "ThumbCreator" );
            for ( KTrader::OfferList::ConstIterator it = plugins.begin(); it != plugins.end(); ++it )
            {
            QString name = (*it)->desktopEntryName();
            if ( ! m_dontPreview.contains(name) )
                    d->previewsToShow->append( name );
            }
            if ( ! m_dontPreview.contains( "audio/" ) )
            d->previewsToShow->append( "audio/" );
        }
    }

    return *(d->previewsToShow);
}

const QString& KonqPropsView::sortCriterion() const {
    return d->sortcriterion;
}

