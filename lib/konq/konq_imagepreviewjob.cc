/*  This file is part of the KDE project
    Copyright (C) 2000 David Faure <faure@kde.org>
                  2000 Carsten Pfeiffer <pfeiffer@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "konq_imagepreviewjob.h"

//#define TIME_PREVIEW
#undef TIME_PREVIEW
#ifdef TIME_PREVIEW
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
static struct timeval startTime;
#endif

#include <sys/stat.h>
#include <sys/shm.h>

#include <qdir.h>
#include <qfile.h>
#include <qimage.h>

#include <kdatastream.h> // Do not remove, needed for correct bool serialization
#include <kfileivi.h>
#include <konq_iconviewwidget.h>
#include <konq_propsview.h>
#include <kapp.h>
#include <ktempfile.h>
#include <ktrader.h>
#include <kmdcodec.h>
#include <kglobal.h>
#include <kstddirs.h>

/**
 * A job that determines the thumbnails for the images in the current directory
 * of the icon view (KonqIconViewWidget)
 */
KonqImagePreviewJob::KonqImagePreviewJob( KonqIconViewWidget * iconView,
                      bool force, int transparency,
                      const QStringList &previewSettings )
  : KIO::Job( false /* no GUI */ ), m_bCanSave( true ), m_iconView( iconView )
{
  kdDebug(1203) << "KonqImagePreviewJob::KonqImagePreviewJob()" << endl;
  m_shmid = -1;
  m_shmaddr = 0;
  // shift into the upper 8 bits, so we can use it as alpha-channel in QImage
  m_transparency = transparency;

  // Load the list of plugins to determine which mimetypes are supported
  KTrader::OfferList plugins = KTrader::self()->query("ThumbCreator");
  PluginMap mimeMap;

  for (KTrader::OfferList::ConstIterator it = plugins.begin(); it != plugins.end(); ++it)
      if (previewSettings.contains((*it)->desktopEntryName()))
      {
          m_plugins.insert((*it)->desktopEntryName(), *it);
          QStringList mimeTypes = (*it)->property("MimeTypes").toStringList();
          for (QStringList::ConstIterator mt = mimeTypes.begin(); mt != mimeTypes.end(); ++mt)
              mimeMap.insert(*mt, *it);
      }
  
  // Look for images and store the items in our todo list :)
  bool bNeedCache = false;
  for (QIconViewItem * it = m_iconView->firstItem(); it; it = it->nextItem() )
  {
    KFileIVI * ivi = static_cast<KFileIVI *>( it );
    if ( force || !ivi->isThumbnail() )
    {
        QString mimeType = ivi->item()->mimetype();
        PluginMap::ConstIterator plugin = mimeMap.find(mimeType);
        if (plugin == mimeMap.end())
        {
            mimeType.replace(QRegExp("/.*"), "/*");
            plugin = mimeMap.find(mimeType);
        }
        if (plugin != mimeMap.end())
        {
            m_items.append( ivi );
            ivi->setThumbnailName( (*plugin)->desktopEntryName() );
            if (!bNeedCache && (*plugin)->property("CacheThumbnail").toBool())
                bNeedCache = true;
        }
    }
  }
  // Read configuration value for the maximum allowed size
  KConfig * config = KGlobal::config();
  KConfigGroupSaver cgs( config, "FMSettings" );
  m_maximumSize = config->readNumEntry( "MaximumImageSize", 1024*1024 /* 1MB */ );
  
  int size = m_iconView->iconSize() ? m_iconView->iconSize()
    : KGlobal::iconLoader()->currentSize( KIcon::Desktop ); // if 0

  QString sizeName;
  if (size < 28)
  {
    m_extent = 48;
    sizeName = "small";
  }
  else if (size < 40)
  {
    m_extent = 64;
    sizeName = "med";
  }
  else
  {
    m_extent = 90;
    sizeName = "large";
  }

  if ( bNeedCache )
  {
    QString thumbPath;
    KGlobal::dirs()->addResourceType( "thumbnails", "share/thumbnails/" );
    // Check if we're in a thumbnail dir already
    if ( m_iconView->url().isLocalFile() )
    {
      // there's exactly one path
      QString cachePath = QDir::cleanDirPath( KGlobal::dirs()->resourceDirs( "thumbnails" )[0] );
      QString dir = QDir::cleanDirPath( m_iconView->url().directory() );
      if ( dir.startsWith( cachePath ) )
        thumbPath = dir.mid( cachePath.length() );
    }

    if ( thumbPath.isEmpty() ) // not a thumbnail dir, generate a name
    {
      KURL cleanURL( m_iconView->url() );
      // clean out the path to avoid multiple cache dirs for the same
      // location (e.g. file:/foo/bar vs. file:/foo//bar)
      cleanURL.setPath( QDir::cleanDirPath( m_iconView->url().path() ) );
      HASHHEX hash;
      KMD5 md5( QFile::encodeName( cleanURL.url() ) );
      md5.hexDigest( hash );
      thumbPath = QString::fromLatin1( hash, 4 ) + "/" +
                  QString::fromLatin1( &hash[4], 4 ) + "/" +
                  QString::fromLatin1( &hash[8] ) + "/";
    }

    m_thumbPath = locateLocal( "thumbnails", thumbPath + "/" + sizeName + "/" );
  }
}

KonqImagePreviewJob::~KonqImagePreviewJob()
{
#ifdef TIME_PREVIEW
  struct timeval now;
  gettimeofday(&now, 0);
  kdDebug(1203) << "KonqImagePreviewJob: Preview generation took "
	<< ((now.tv_sec + (double)(now.tv_usec) / 1000000)
	    - (startTime.tv_sec + (double)(startTime.tv_usec) / 1000000))
	<< " seconds" << endl;
#endif
  kdDebug(1203) << "KonqImagePreviewJob::~KonqImagePreviewJob()" << endl;
  if (m_shmaddr) {
      shmdt(m_shmaddr);
      shmctl(m_shmid, IPC_RMID, 0);
  }
}

void KonqImagePreviewJob::startImagePreview()
{
  // The reason for this being separate from determineNextIcon is so
  // that we don't do arrangeItemsInGrid if there is no image at all
  // in the current dir.
#ifdef TIME_PREVIEW
  gettimeofday(&startTime, 0);
#endif
  if ( m_items.isEmpty() )
  {
    kdDebug(1203) << "startImagePreview: emitting result" << endl;
    emit result(this);
    delete this;
  }
  else
  {
    m_bHasXvpics = false;
    m_state = STATE_STATXVDIR;
    QString dir = m_currentURL.directory();
    KURL xvURL( m_currentURL );
         xvURL.setPath( dir +
         ((dir.mid( dir.findRev( '/' ) + 1 ) != ".xvpics")
           ? "/.xvpics/" : "/"));                // .xvpics if not already there
    KIO::Job * job = KIO::stat( xvURL, false );
    addSubjob( job );
  }
}

void KonqImagePreviewJob::determineNextIcon()
{
  // No more items ?
  if ( m_items.isEmpty() )
  {
    if (m_iconView->autoArrange())
        m_iconView->arrangeItemsInGrid();
    // Done
    kdDebug(1203) << "determineNextIcon: emitting result" << endl;
    emit result(this);
    delete this;
  }
  else
  {
    // First, stat the orig file
    m_state = STATE_STATORIG;
    m_currentURL = m_items.first()->item()->url();
    m_currentItem = m_items.first();
    KIO::Job * job = KIO::stat( m_currentURL, false );
    kdDebug(1203) << "KonqImagePreviewJob: KIO::stat orig " << m_currentURL.url() << endl;
    addSubjob(job);
    m_items.removeFirst();
  }
}

void KonqImagePreviewJob::itemRemoved( KFileIVI * item )
{
  m_items.removeRef( item );

  if ( item == m_currentItem )
  {
    // Abort
    subjobs.first()->kill();
    subjobs.removeFirst();
    determineNextIcon();
  }
}

void KonqImagePreviewJob::slotResult( KIO::Job *job )
{
  subjobs.remove( job );
  ASSERT ( subjobs.isEmpty() ); // We should have only one job at a time ...
  switch ( m_state ) {
    case STATE_STATXVDIR:
    {
      if (!job->error())
      {
        KIO::UDSEntry entry = ((KIO::StatJob*)job)->statResult();
        for ( KIO::UDSEntry::ConstIterator it = entry.begin();
              it != entry.end(); ++it )
          if ( (*it).m_uds == KIO::UDS_FILE_TYPE && S_ISDIR( (*it).m_long ) )
          {
              m_bHasXvpics = true;
              break;
          }
      }
      determineNextIcon();
      return;
    }
    case STATE_STATORIG:
    {
      if (job->error()) // that's no good news...
      {
        // Drop this one and move on to the next one
        determineNextIcon();
        return;
      }
      KIO::UDSEntry entry = ((KIO::StatJob*)job)->statResult();
      KIO::UDSEntry::ConstIterator it = entry.begin();
      m_tOrig = 0;
      int found = 0;
      for( ; it != entry.end() && found < 2; it++ ) {
        if ( (*it).m_uds == KIO::UDS_MODIFICATION_TIME )
        {
          m_tOrig = (time_t)((*it).m_long);
          found++;
        }
        else if ( (*it).m_uds == KIO::UDS_SIZE )
        {
          if ( static_cast<unsigned long>((*it).m_long) > m_maximumSize )
          {
            kdWarning(1203) << "Image preview: image " << m_currentURL.prettyURL() << " too big, skipping. " << endl;
            determineNextIcon();
            return;
          }
          found++;
        }
      }

      QString mimeType = m_currentItem->item()->mimetype();
      if ( !m_plugins[m_currentItem->thumbnailName()]->property( "CacheThumbnail" ).toBool() )
      {
          // This preview will not be cached, no need to look for a saved thumbnail
          // Just create it, and be done
          getOrCreateThumbnail();
          return;
      }

      if ( statResultThumbnail() )
          return;

      if ( m_bHasXvpics )
      {
        // Not found or not valid, look for a matching file in .xvpics
        m_state = STATE_STATXV;
        QString dir = m_currentURL.directory();
        KURL thumbURL( m_currentURL );
        thumbURL.setPath( dir +
           ((dir.mid( dir.findRev( '/' ) + 1 ) != ".xvpics")
               ? "/.xvpics/" : "/")                // .xvpics if not already there
           + m_currentURL.fileName() );            // file name
        KIO::Job * job = KIO::stat( thumbURL, false );
        kdDebug(1203) << "KonqImagePreviewJob: KIO::stat thumb " << thumbURL.url() << endl;
        addSubjob(job);
      }
      else // there is no .xvpics subdir, no need to stat a file inside it :)
        getOrCreateThumbnail();

      return;
    }
    case STATE_GETTHUMB:
    {
      // We arrive here if statResultThumbnail found a remote thumbnail
      if (job->error())
      {
        // Drop this one (if get fails, I wouldn't be on mkdir and put)
        determineNextIcon();
        return;
      }
      QString localFile( static_cast<KIO::FileCopyJob*>(job)->destURL().path() );
      QPixmap pix;
      if ( pix.load( localFile ) )
      {
        // Found it, use it
        m_iconView->setThumbnailPixmap( m_currentItem, pix );
      }
      QFile::remove( localFile );
      // Whether we suceeded or not, move to next one
      determineNextIcon();
      return;
    }
    case STATE_STATXV:
      // Try to load this thumbnail - takes care of determineNextIcon
      if ( statResultThumbnail( static_cast<KIO::StatJob *>(job) ) )
        return;

      // No thumbnail, or too old -> check dirs, load orig image and create one
      getOrCreateThumbnail();
      return;
    case STATE_GETORIG:
    {
      if (job->error())
      {
        QFile::remove( static_cast<KIO::FileCopyJob*>(job)->destURL().path() );
        determineNextIcon();
        return;
      }
      
      createThumbnail( static_cast<KIO::FileCopyJob*>(job)->destURL().path() );
      return;
    }
    case STATE_CREATETHUMB:
    {
      //kdDebug(1203) << "KonqImagePreviewJob: got slotResult in STATE_CREATETHUMB" << endl;
      if (!m_tempName.isEmpty())
      {
          QFile::remove(m_tempName);
          m_tempName = QString::null;
      }
      determineNextIcon();
      return;
    }
  }
}

bool KonqImagePreviewJob::statResultThumbnail( KIO::StatJob * job )
{
    QString thumbPath = m_thumbPath + m_currentURL.fileName();
    time_t tThumb = 0;
    if ( job ) // had a job (.xvpics)
    {
      if (!job->error()) // found a thumbnail
      {
          KIO::UDSEntry entry = job->statResult();
          KIO::UDSEntry::ConstIterator it = entry.begin();
          for( ; it != entry.end(); it++ ) {
            if ( (*it).m_uds == KIO::UDS_MODIFICATION_TIME ) {
              tThumb = (time_t)((*it).m_long);
            }
          }
          // Only ok if newer than the file
      }
    }
    else
    {
      struct stat st;
      if ( stat( QFile::encodeName( thumbPath ), &st ) == 0 )
        tThumb = st.st_mtime;
    }

    if ( tThumb < m_tOrig )
        return false;

    if ( !job || job->url().isLocalFile() )
    {
        QPixmap pix;
        if ( pix.load( job ? job->url().url() : thumbPath ) )
        {
            // Found it, use it
            m_iconView->setThumbnailPixmap( m_currentItem, pix );
            determineNextIcon();
            return true;
        }
    }
    else
    {
        m_state = STATE_GETTHUMB;
        KTempFile localFile;
        KURL localURL;
        localURL.setPath( localFile.name() );
        KIO::Job * getJob = KIO::file_copy( job->url(), localURL, -1, true, false, false /* No GUI */ );
        kdDebug(1203) << "KonqImagePreviewJob:: KIO::file_copy thumb " << job->url().url() << " to " << localFile.name() << endl;
        addSubjob(getJob);
        return true;
    }
    return false;
}


void KonqImagePreviewJob::getOrCreateThumbnail()
{
    // We still need to load the orig file ! (This is getting tedious) :)
    if ( m_currentURL.isLocalFile() )
        createThumbnail( m_currentURL.path() );
    else
    {
        m_state = STATE_GETORIG;
        KTempFile localFile;
        KURL localURL;
        localURL.setPath( m_tempName = localFile.name() );
        KIO::Job * job = KIO::file_copy( m_currentURL, localURL, -1, true,
                                         false, false /* No GUI */ );
        kdDebug(1203) << "KonqImagePreviewJob: KIO::file_copy orig "
                      << m_currentURL.url() << " to " << m_tempName << endl;
        addSubjob(job);
    }
}

void KonqImagePreviewJob::createThumbnail( QString pixPath )
{
    m_state = STATE_CREATETHUMB;
    KURL thumbURL;
    thumbURL.setProtocol("thumbnail");
    thumbURL.setPath(pixPath);
    KIO::TransferJob *job = KIO::get(thumbURL, false, false);
    connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)), SLOT(slotThumbData(KIO::Job *, const QByteArray &)));
    job->addMetaData("mimeType", m_currentItem->item()->mimetype());
    job->addMetaData("iconSize", QString().setNum(m_iconView->iconSize() ?
        m_iconView->iconSize() : KGlobal::iconLoader()->currentSize(KIcon::Desktop)));
    job->addMetaData("width", QString().setNum(m_extent));
    job->addMetaData("height", QString().setNum(m_extent));
    job->addMetaData("iconAlpha", QString().setNum(m_transparency));
    job->addMetaData("plugin", m_currentItem->thumbnailName());
    if (m_shmid == -1
#ifdef TIME_PREVIEW
		&& (getenv("PREVIEW_NO_SHM") == 0)
#endif
		)
    {
        if (m_shmaddr)
            shmdt(m_shmaddr);
        m_shmid = shmget(IPC_PRIVATE, m_extent * m_extent * 4, IPC_CREAT|0777);
        if (m_shmid != -1)
        {
            m_shmaddr = (char*) shmat(m_shmid, 0, SHM_RDONLY);
            if (m_shmaddr == (char *)-1)
            {
                shmctl(m_shmid, IPC_RMID, 0);
                m_shmaddr = 0;
                m_shmid = -1;
            }
        }
        else
            m_shmaddr = 0;
    }
    if (m_shmid != -1)
        job->addMetaData("shmid", QString().setNum(m_shmid));
    addSubjob(job);
}

void KonqImagePreviewJob::slotThumbData(KIO::Job *, const QByteArray &data)
{
    kdDebug(1203) << "KonqImagePreviewJob::slotThumbData" << endl;
    bool save = m_bCanSave && m_plugins[m_currentItem->thumbnailName()]->property("CacheThumbnail").toBool();
    QPixmap pix;
    if (m_shmaddr)
    {
        QDataStream str(data, IO_ReadOnly);
        int w, h, d;
        bool alpha;
        str >> w >> h >> d >> alpha;
        QImage img((uchar *)m_shmaddr, w, h, d, 0, 0, QImage::IgnoreEndian);
        img.setAlphaBuffer(alpha);
        pix.convertFromImage(img);
        if (save)
        {
            QByteArray saveData;
            QDataStream saveStr(saveData, IO_WriteOnly);
            saveStr << img;
            saveThumbnail(saveData);
        }
    }
    else
    {
        pix.loadFromData(data);
        if (save)
            saveThumbnail(data);
    }
    m_iconView->setThumbnailPixmap(m_currentItem, pix);
}

void KonqImagePreviewJob::saveThumbnail(const QByteArray &imgData)
{
    QFile file( m_thumbPath + m_currentURL.fileName() );
    if ( file.open(IO_WriteOnly) )
    {
        file.writeBlock( imgData.data(), imgData.size() );
        file.close();
    }
}

#include "konq_imagepreviewjob.moc"
