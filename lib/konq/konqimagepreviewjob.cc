/*  This file is part of the KDE project
    Copyright (C) 2000 David Faure <faure@kde.org>

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

#include "konqimagepreviewjob.h"
#include <kfileivi.h>
#include <kiconloader.h>
#include <konqiconviewwidget.h>
#include <assert.h>
#include <stdio.h> // for tmpnam
#include <unistd.h> // for unlink

/**
 * A job that determines the thumbnails for the images in the current directory
 * of the icon view (KonqIconViewWidget)
 */
KonqImagePreviewJob::KonqImagePreviewJob( KonqIconViewWidget * iconView, bool force )
  : KIO::Job( false /* no GUI */ ), m_bNoWrite( false ), m_iconView( iconView )
{
  kdDebug(1203) << "KonqImagePreviewJob::KonqImagePreviewJob()" << endl;
  // Look for images and store the items in our todo list :)
  for (QIconViewItem * it = m_iconView->firstItem(); it; it = it->nextItem() )
  {
    KFileIVI * ivi = static_cast<KFileIVI *>( it );
    if ( ivi->item()->mimetype().left(6) == "image/" )
      if ( force || !ivi->isThumbnail() )
        m_items.append( ivi );
  }
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
    determineNextIcon();
}

void KonqImagePreviewJob::determineNextIcon()
{
  // No more items ?
  if ( m_items.isEmpty() )
  {
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
      for( ; it != entry.end(); it++ ) {
        if ( (*it).m_uds == KIO::UDS_MODIFICATION_TIME ) {
          m_tOrig = (time_t)((*it).m_long);
        }
      }

      determineThumbnailURL();

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
        unlink( localFile.local8Bit().data() );
      }
      // Whether we suceeded or not, move to next one
      determineNextIcon();
      return;
    }
    case STATE_STATXV:
      // Try to load this thumbnail - takes care of determineNextIcon
      if ( statResultThumbnail( static_cast<KIO::StatJob *>(job) ) )
        return;

      // No thumbnail, or too old -> check dirs, load orig image and create Pixie pic

      // We call this again, because it's the mospics we want to generate, not the xvpics
      // Well, comment this out if you prefer compatibility over quality.
      determineThumbnailURL();

      if ( !m_bNoWrite )
      {
        // m_thumbURL is /blah/.mospics/med/file.png
        QString dir = m_thumbURL.directory();
        QString mospicsPath = dir.left( dir.findRev( '/' ) ); // /blah/.mospics
        KURL mospicsURL( m_thumbURL );
        mospicsURL.setPath( mospicsPath );

        // We don't check + create. We just create and ignore "already exists" errors.
        // Way more efficient, on all protocols.
        m_state = STATE_CREATEDIR1;
        KIO::Job * job = KIO::mkdir( mospicsURL );
        addSubjob(job);
        kdDebug(1203) << "KonqImagePreviewJob: KIO::mkdir mospicsURL=" << mospicsURL.url() << endl;
        return;
      }
      // Fall through if we can't create dirs here
    case STATE_CREATEDIR1:
      if ( !m_bNoWrite )
      {
        // We can save if the dir could be created or if it already exists
        m_bCanSave = (!job->error() || job->error() == KIO::ERR_DIR_ALREADY_EXIST);

        if (m_bCanSave)
        {
          KURL thumbdirURL( m_thumbURL );
          thumbdirURL.setPath( m_thumbURL.directory() ); // /blah/.mospics/med

          // We don't check + create. We just create and ignore "already exists" errors.
          // Way more efficient, on all protocols. (Yes you read that once already)
          m_state = STATE_CREATEDIR2;
          KIO::Job * job = KIO::mkdir( thumbdirURL );
          addSubjob(job);
          kdDebug(1203) << "KonqImagePreviewJob: KIO::mkdir thumbdirURL=" << thumbdirURL.url() << endl;
          return;
        }
        else
          m_bNoWrite = true; // remember not to create dir next time
      }
      // Fall through if we can't save
    case STATE_CREATEDIR2:
      // We can save if the dir could be created or if it already exists
      m_bCanSave = ( !m_bNoWrite &&
                    (!job->error() || job->error() == KIO::ERR_DIR_ALREADY_EXIST) );
      if (!m_bCanSave)
        m_bNoWrite = true;

      // We still need to load the orig file ! (This is getting tedious) :)
      if ( m_currentURL.isLocalFile() )
        createThumbnail( m_currentURL.path() );
      else
      {
        m_state = STATE_GETORIG;
        QString localFile = tmpnam(0);  // Generate a temp file name
        KURL localURL;
        localURL.setPath( localFile );
        KIO::Job * job = KIO::file_copy( m_currentURL, localURL, -1, false, false, false /* No GUI */ );
        kdDebug(1203) << "KonqImagePreviewJob: KIO::file_copy orig " << m_currentURL.url() << " to " << localFile << endl;
        addSubjob(job);
      }
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
      unlink( localFile.local8Bit().data() );
      return;
    }
    case STATE_PUTTHUMB:
    {
      // Ignore errors - there's nothing else we can do for this poor thumbnail
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
  // This also takes care of browsing the .mospics dirs themselves
  // Check if m_currentURL is /blah/.mospics/med/file.png
  QString dir = m_currentURL.directory();               // /blah/.mospics/med
  QString grandFather = dir.left( dir.findRev( '/' ) ); // /blah/.mospics
  QString grandFatherFileName = grandFather.mid( grandFather.findRev('/')+1 );
  QString mospicsPath = (grandFatherFileName == ".mospics") ? grandFather : (dir + "/.mospics");
  m_thumbURL = m_currentURL;

  // Difference with the previous algorithm is: we always honour the
  // requested icon size. Otherwise, one needs to remove .mospics when
  // changing icon size...

  if (size < 28)
  {
    m_extent = 48;
    m_thumbURL.setPath( mospicsPath + "/small/" + m_currentURL.fileName() );
  }
  else if (size < 40)
  {
    m_extent = 64;
    m_thumbURL.setPath( mospicsPath + "/med/" + m_currentURL.fileName() );
  }
  else
  {
    m_extent = 90;
    m_thumbURL.setPath( mospicsPath + "/large/" + m_currentURL.fileName() );
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
    QString localFile = tmpnam(0);  // Generate a temp file name
    KURL localURL;
    localURL.setPath( localFile );
    KIO::Job * job = KIO::file_copy( m_thumbURL, localURL, -1, false, false, false /* No GUI */ );
    kdDebug(1203) << "KonqImagePreviewJob: KIO::file_copy thumb " << m_thumbURL.url() << " to " << localFile << endl;
    addSubjob(job);
    return true;
  }
  return false;
}

void KonqImagePreviewJob::createThumbnail( QString pixPath )
{
  QPixmap pix;
  QImage img;
  if ( pix.load( pixPath ) )
  {
    int w = pix.width(), h = pix.height();
    // scale to pixie size
    if(pix.width() > m_extent || pix.height() > m_extent){
        if(pix.width() > pix.height()){
            float percent = (((float)m_extent)/pix.width());
            h = (int)(pix.height()*percent);
            w = m_extent;
        }
        else{
            float percent = (((float)m_extent)/pix.height());
            w = (int)(pix.width()*percent);
            h = m_extent;
        }
        img = pix.convertToImage().smoothScale( w, h );
        pix.convertFromImage( img );
    }
    else if (m_bCanSave)
        img = pix.convertToImage();

    // Set the thumbnail
    m_iconView->setThumbnailPixmap( m_currentItem, pix );

    if ( m_bCanSave )
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
  }
  determineNextIcon();
}

#include "konqimagepreviewjob.moc"
