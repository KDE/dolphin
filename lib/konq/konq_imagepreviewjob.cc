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

#include <kfileivi.h>
#include <kiconloader.h>
#include <konq_iconviewwidget.h>
#include <kpixmapsplitter.h>
#include <assert.h>
#include <stdio.h> // for tmpnam
#include <unistd.h> // for unlink

/**
 * A job that determines the thumbnails for the images in the current directory
 * of the icon view (KonqIconViewWidget)
 */
KonqImagePreviewJob::KonqImagePreviewJob( KonqIconViewWidget * iconView, bool force, KPixmapSplitter *splitter )
  : KIO::Job( false /* no GUI */ ), m_bCanSave( true ), m_iconView( iconView )
{
  m_extent = 0;
  kdDebug(1203) << "KonqImagePreviewJob::KonqImagePreviewJob()" << endl;
  m_splitter = splitter;

  // Look for images and store the items in our todo list :)
  for (QIconViewItem * it = m_iconView->firstItem(); it; it = it->nextItem() )
  {
    KFileIVI * ivi = static_cast<KFileIVI *>( it );
    QString mimeType = ivi->item()->mimetype();
    if ( mimeType.startsWith( "image/" ) ||
	 (m_splitter && mimeType.startsWith( "text/")) )
      if ( force || !ivi->isThumbnail() )
        m_items.append( ivi );
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
    m_bDirsCreated = false;
    determineNextIcon();
  }
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

      // We call this again, because it's the png pics we want to generate, not the xvpics
      // Well, comment this out if you prefer compatibility over quality.
      determineThumbnailURL();

      kdDebug(1203) << "After stat xv m_bCanSave=" << m_bCanSave << " m_bDirsCreated=" << m_bDirsCreated << endl;
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
  kdDebug(1203) << "KonqImagePreviewJob::createThumbnail loading " << pixPath << endl;

  bool ok = false;
  bool saveImage = m_bCanSave;

  // create text-preview
  if ( m_currentItem->item()->mimetype().startsWith( "text/" ) ) {
      saveImage = false; // generating them on the fly is slightly faster
      const int bytesToRead = 1024; // FIXME, make configurable
      QFile file( pixPath );
      if ( file.open( IO_ReadOnly )) {
	  char data[bytesToRead+1];
	  int read = file.readBlock( data, bytesToRead );
	  if ( read > 0 ) {
	      ok = true;
	      data[read] = '\0';
	      QString text = QString::fromLocal8Bit( data );
	      // kdDebug(1203) << "Textpreview-data: " << text << endl;
	      // FIXME: maybe strip whitespace and read more?

	      QRect rect;
	
	      // example: width: 60, height: 64
	      float ratio = 15.0 / 16.0; // so we get a page-like size
	      int width = (int) (ratio * (float) m_extent);
	      pix.resize( width, m_extent );
	      pix.fill( QColor( 245, 245, 245 ) ); // light-grey background
	
	      QSize chSize = m_splitter->itemSize(); // the size of one char
	      int xOffset = chSize.width();
	      int yOffset = chSize.height();

	      // one pixel for the rectangle, the rest. whitespace
	      int xborder = 1 + width/16;    // minimum x-border
	      int yborder = 1 + m_extent/16; // minimum y-border

	      // calculate a better border so that the text is centered
	      int canvasWidth = width - 2*xborder;
	      int canvasHeight = m_extent -  2*yborder;
	      int numCharsPerLine = (int) (canvasWidth / chSize.width());
	      int numLines = (int) (canvasHeight / chSize.height());

	      int rest = width - (numCharsPerLine * chSize.width());
	      xborder = QMAX( xborder, rest/2); // center horizontally
	      rest = m_extent - (numLines * chSize.height());
	      yborder = QMAX( yborder, rest/2); // center vertically
	      // end centering
	
	      int x = xborder, y = yborder; // where to paint the characters
	      int posNewLine  = width - (chSize.width() + xborder);
	      int posLastLine = m_extent - (chSize.height() + yborder);
	      bool newLine = false;
	      ASSERT( posNewLine > 0 );
	      const QPixmap *fontPixmap = &(m_splitter->pixmap());
	      
	      for ( uint i = 0; i < text.length(); i++ ) {
		  if ( x > posNewLine || newLine ) { // start a new line?
		      x = xborder;
		      y += yOffset;
		
		      if ( y > posLastLine ) // more text than space
			  break;

		      // after starting a new line, we also jump to the next
		      // physical newline in the file if we don't come from one
		      if ( !newLine ) {
			  int pos = text.find( '\n', i );
			  if ( pos > (int) i )
			      i = pos +1;
		      }

		      newLine = false;
		  }
		
		  // check for newlines in the text (unix,dos)
		  QChar ch = text.at( i );
		  if ( ch == '\n' ) {
		      newLine = true;
		      continue;
		  }
		  else if ( ch == '\r' && text.at(i+1) == '\n' ) {
		      newLine = true;
		      i++; // skip the next character (\n) as well
		      continue;
		  }
		
		
		  rect = m_splitter->coordinates( ch );
		  if ( !rect.isEmpty() ) {
		      bitBlt( &pix, QPoint(x,y), fontPixmap, rect, CopyROP );
		  }

		  x += xOffset; // next character
	      }

	      // paint a black rectangle around the "page"
	      QPainter p;
	      p.begin( &pix );
	      p.setPen( QColor( 88, 88, 88 ));
	      p.drawRect( 0, 0, pix.width(), pix.height() );
	      p.end();
	
 	      if ( saveImage )
 		  img = pix.convertToImage();
	  }
	  file.close();
      }
  }

  // create image preview
  else if ( pix.load( pixPath ) )
  {
    ok = true;
    int w = pix.width(), h = pix.height();
    kdDebug(1203) << "w=" << w << " h=" << h << " m_extent=" << m_extent << endl;
    // scale to pixie size
    if(w > m_extent || h > m_extent){
        if(w > h){
            h = (int)( (double)( h * m_extent ) / w );
            if ( h == 0 ) h = 1;
            w = m_extent;
            ASSERT( h <= m_extent );
        }
        else{
            kdDebug(1203) << "Setting h to m_extent" << endl;
            w = (int)( (double)( w * m_extent ) / h );
            if ( w == 0 ) w = 1;
            h = m_extent;
            ASSERT( w <= m_extent );
        }
        kdDebug(1203) << "smoothScale to " << w << "x" << h << endl;
        img = pix.convertToImage().smoothScale( w, h );
        if ( img.width() != w || img.height() != h )
        {
            // Resizing failed. Aborting.
            kdWarning() << "Resizing of " << pixPath << " failed. Aborting. " << endl;
            determineNextIcon();
            return;
        }
        pix.convertFromImage( img );
    }
    else if (m_bCanSave)
        img = pix.convertToImage();
  }


  if ( ok ) {
    // Set the thumbnail
    m_iconView->setThumbnailPixmap( m_currentItem, pix );

    if ( saveImage )
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

#include "konq_imagepreviewjob.moc"
