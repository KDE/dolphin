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

#include <qfile.h>
#include <qimage.h>

#include <kfileivi.h>
#include <konq_iconviewwidget.h>
#include <konq_propsview.h>
#include <kapp.h>
#include <ktempfile.h>
#include <assert.h>

/**
 * A job that determines the thumbnails for the images in the current directory
 * of the icon view (KonqIconViewWidget)
 */
KonqImagePreviewJob::KonqImagePreviewJob( KonqIconViewWidget * iconView,
					  bool force, int transparency,
					  const bool * previewSettings )
  : KIO::Job( false /* no GUI */ ), m_bCanSave( true ), m_iconView( iconView )
{
  m_extent = 0;
  kdDebug(1203) << "KonqImagePreviewJob::KonqImagePreviewJob()" << endl;
  m_bDirsCreated = true; // if no images, no need for dirs
  // shift into the upper 8 bits, so we can use it as alpha-channel in QImage
  m_transparency = (transparency << 24) | 0x00ffffff;

  m_renderHTML = !previewSettings || previewSettings[KonqPropsView::HTMLPREVIEW];
  // Look for images and store the items in our todo list :)
  for (QIconViewItem * it = m_iconView->firstItem(); it; it = it->nextItem() )
  {
    KFileIVI * ivi = static_cast<KFileIVI *>( it );
    if ( force || !ivi->isThumbnail() )
    {
        QString mimeType = ivi->item()->mimetype();
        bool bText = false;
        bool bHTML = false;
        bool bImage = mimeType.startsWith( "image/" ) && (!previewSettings || previewSettings[KonqPropsView::IMAGEPREVIEW]);
        if ( bImage )
            m_bDirsCreated = false; // We'll need dirs
        else
        {
            bText = mimeType.startsWith( "text/") && (!previewSettings || previewSettings[KonqPropsView::TEXTPREVIEW]);
            if ((bHTML = mimeType == "text/html" && m_renderHTML))
                m_bDirsCreated = false;
        }
        if ( bImage || bText || bHTML)
            m_items.append( ivi );
    }
  }
  // Read configuration value for the maximum allowed size
  KConfig * config = KGlobal::config();
  KConfigGroupSaver cgs( config, "FMSettings" );
  m_maximumSize = config->readNumEntry( "MaximumImageSize", 1024*1024 /* 1MB */ );
}

KonqImagePreviewJob::~KonqImagePreviewJob()
{
  kdDebug(1203) << "KonqImagePreviewJob::~KonqImagePreviewJob()" << endl;
}

void KonqImagePreviewJob::startImagePreview()
{
  // The reason for this being separate from determineNextIcon is so
  // that we don't do arrangeItemsInGrid if there is no image at all
  // in the current dir.
  if ( m_items.isEmpty() )
  {
    kdDebug(1203) << "startImagePreview: emitting result" << endl;
    emit result(this);
    delete this;
  }
  else
  {
    determineNextIcon();
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
  assert ( subjobs.isEmpty() ); // We should have only one job at a time ...
  switch ( m_state ) {
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
      unsigned long size = 0;
      int found = 0;
      for( ; it != entry.end() && found < 2; it++ ) {
        if ( (*it).m_uds == KIO::UDS_MODIFICATION_TIME )
        {
          m_tOrig = (time_t)((*it).m_long);
          found++;
        }
        else if ( (*it).m_uds == KIO::UDS_SIZE )
        {
          size = (*it).m_long;
          found++;
        }
      }

      if ( size > m_maximumSize )
      {
          kdWarning(1203) << "Image preview: image " << m_currentURL.prettyURL() << " too big, skipping. " << endl;
          determineNextIcon();
          return;
      }

      determineThumbnailURL();

      QString mimeType = m_currentItem->item()->mimetype();
      if ( mimeType.startsWith( "text/") &&
           ! (mimeType == "text/html" && m_renderHTML))
      {
          // This is a text preview, no need to look for a saved thumbnail
          // Just create it, and be done
          getOrCreateThumbnail();
          return;
      }


      m_state = STATE_STATTHUMB;
      KIO::Job * job = KIO::stat( m_thumbURL, false );
      kdDebug(1203) << "KonqImagePreviewJob: KIO::stat thumb " << m_thumbURL.url() << endl;
      addSubjob(job);

      return;
    }
    case STATE_STATTHUMB:
    {
      // Try to load this thumbnail - takes care of determineNextIcon
      if ( statResultThumbnail( static_cast<KIO::StatJob *>(job) ) )
        return;

      // Not found or not valid
      m_state = STATE_STATXV;
      QString dir = m_currentURL.directory();
      m_thumbURL.setPath( dir +
         ((dir.mid( dir.findRev( '/' ) + 1 ) != ".xvpics")
             ? "/.xvpics/" : "/")                // .xvpics if not already there
         + m_currentURL.fileName() );            // file name
      KIO::Job * job = KIO::stat( m_thumbURL, false );
      kdDebug(1203) << "KonqImagePreviewJob: KIO::stat thumb " << m_thumbURL.url() << endl;
      addSubjob(job);

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

      // No thumbnail, or too old -> check dirs, load orig image and create Pixie pic

      // We call this again, because it's the png pics we want to generate, not the xvpics
      // Well, comment this out if you prefer compatibility over quality.
      determineThumbnailURL();

      //kdDebug(1203) << "After stat xv m_bCanSave=" << m_bCanSave << " m_bDirsCreated=" << m_bDirsCreated << endl;
      if ( m_bCanSave && !m_bDirsCreated )
      {
        // m_thumbURL is /blah/.pics/med/file.png
        QString dir = m_thumbURL.directory();
        QString pngpicsPath = dir.left( dir.findRev( '/' ) ); // /blah/.pngpics
        KURL pngpicsURL( m_thumbURL );
        pngpicsURL.setPath( pngpicsPath );

        // We don't check + create. We just create and ignore "already exists" errors.
        // Way more efficient, on all protocols.
        m_state = STATE_CREATEDIR1;
        KIO::Job * job = KIO::mkdir( pngpicsURL );
        addSubjob(job);
        kdDebug(1203) << "KonqImagePreviewJob: KIO::mkdir pngpicsURL=" << pngpicsURL.url() << endl;
        return;
      }
      // Fall through if we can't create dirs here or if dirs are already created
    case STATE_CREATEDIR1:
      if ( m_bCanSave )
      {
        // We can save if the dir was already created, was just created or if it already exists
        if (m_bDirsCreated || !job->error() || job->error() == KIO::ERR_DIR_ALREADY_EXIST)
        {
            if ( !m_bDirsCreated )
            {
                KURL thumbdirURL( m_thumbURL );
                thumbdirURL.setPath( m_thumbURL.directory() ); // /blah/.pics/med

                // We don't check + create. We just create and ignore "already exists" errors.
                // Way more efficient, on all protocols. (Yes you read that once already)
                m_state = STATE_CREATEDIR2;
                KIO::Job * job = KIO::mkdir( thumbdirURL );
                addSubjob(job);
                kdDebug(1203) << "KonqImagePreviewJob: KIO::mkdir thumbdirURL=" << thumbdirURL.url() << endl;
                return;
            }
        }
        else
          m_bCanSave = false; // remember not to create dir next time
      }
      // Fall through if we can't save or if dirs are already created
    case STATE_CREATEDIR2:
      // We can save if the dir could be created or if it already exists
      if ( m_bCanSave )
      {
          m_bCanSave = (m_bDirsCreated || !job->error() || job->error() == KIO::ERR_DIR_ALREADY_EXIST);
          if (m_bCanSave && !m_bDirsCreated) // First time we create dirs
              m_bDirsCreated = true;
      }

      // This is the next stage
      getOrCreateThumbnail();
      return;
    case STATE_GETORIG:
    {
      if (job->error())
      {
        determineNextIcon();
        return;
      }

      QString localFile( static_cast<KIO::FileCopyJob*>(job)->destURL().path() );
      createThumbnail( localFile );
      return;
    }
    case STATE_PUTTHUMB:
    {
      // Ignore errors - there's nothing else we can do for this poor thumbnail
      determineNextIcon();
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

void KonqImagePreviewJob::determineThumbnailURL()
{
  int size = m_iconView->iconSize() ? m_iconView->iconSize()
    : KGlobal::iconLoader()->currentSize( KIcon::Desktop ); // if 0

  // Check if pixie thumbnail is there first
  // This also takes care of browsing the .pics dirs themselves
  // Check if m_currentURL is /blah/.pics/med/file.png
  QString dir = m_currentURL.directory();               // /blah/.pics/med
  QString grandFather = dir.left( dir.findRev( '/' ) ); // /blah/.pics
  QString grandFatherFileName = grandFather.mid( grandFather.findRev('/')+1 );
  QString picsPath = (grandFatherFileName == ".pics") ? grandFather : (dir + "/.pics");
  m_thumbURL = m_currentURL;

  // Difference with the previous algorithm is: we always honour the
  // requested icon size. Otherwise, one needs to remove .pics when
  // changing icon size...

  if (size < 28)
  {
    m_extent = 48;
    m_thumbURL.setPath( picsPath + "/small/" + m_currentURL.fileName() );
  }
  else if (size < 40)
  {
    m_extent = 64;
    m_thumbURL.setPath( picsPath + "/med/" + m_currentURL.fileName() );
  }
  else
  {
    m_extent = 90;
    m_thumbURL.setPath( picsPath + "/large/" + m_currentURL.fileName() );
  }
}

bool KonqImagePreviewJob::statResultThumbnail( KIO::StatJob * job )
{
    bool bThumbnail = false;
    if (!job->error()) // found a thumbnail
    {
        KIO::UDSEntry entry = job->statResult();
        KIO::UDSEntry::ConstIterator it = entry.begin();
        time_t tThumb = 0;
        for( ; it != entry.end(); it++ ) {
            if ( (*it).m_uds == KIO::UDS_MODIFICATION_TIME ) {
                tThumb = (time_t)((*it).m_long);
            }
        }
        // Only ok if newer than the file
        bThumbnail = (tThumb >= m_tOrig);
    }

    if ( !bThumbnail )
        return false;

    if ( m_thumbURL.isLocalFile() )
    {
        QPixmap pix;
        if ( pix.load( m_thumbURL.path() ) )
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
        KIO::Job * job = KIO::file_copy( m_thumbURL, localURL, -1, true, false, false /* No GUI */ );
        kdDebug(1203) << "KonqImagePreviewJob: KIO::file_copy thumb " << m_thumbURL.url() << " to " << localFile.name() << endl;
        addSubjob(job);
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
    job->addMetaData("extent", QString().setNum(m_extent));
    job->addMetaData("transparency", QString().setNum(m_transparency));
    // FIXME: This needs to be generalized once the thumbnail creator
    // is pluginified (malte)
    job->addMetaData("renderHTML", m_renderHTML ? "true" : "false");
    addSubjob(job);
}

void KonqImagePreviewJob::slotThumbData(KIO::Job *job, const QByteArray &data)
{
    kdDebug(1203) << "KonqImagePreviewJob::slotThumbData" << endl;
    QPixmap pix(data);
    bool save = static_cast<KIO::TransferJob *>(job)->queryMetaData("save") == "true";
    m_iconView->setThumbnailPixmap(m_currentItem, pix);
    if (save && m_bCanSave)
        saveThumbnail(pix.convertToImage());
}

void KonqImagePreviewJob::saveThumbnail(const QImage &img)
{
    QString tmpFile;
    if ( m_thumbURL.isLocalFile() )
        tmpFile = m_thumbURL.path();
    else
        tmpFile = tmpnam(0);
    QImageIO iio;
    iio.setImage(img);
    iio.setFileName( tmpFile );
    iio.setFormat("PNG");
    if ( iio.write() )
        if ( !m_thumbURL.isLocalFile() )
        {
            m_state = STATE_PUTTHUMB;
            KIO::Job * job = KIO::file_copy( tmpFile, m_thumbURL, -1, true /* overwrite */, false, false /* No GUI */ );
            kdDebug(1203) << "KonqImagePreviewJob: KIO::file_copy thumb " << tmpFile << " to " << m_thumbURL.url() << endl;
            addSubjob(job);
            return;
        }
}

#include "konq_imagepreviewjob.moc"
