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

/**
 * A job that determines the thumbnails for the images in the current directory
 * of the icon view (KonqIconViewWidget)
 */
KonqImagePreviewJob::KonqImagePreviewJob( KonqIconViewWidget * iconView )
  : Job( false /* no GUI */ ), m_iconView( iconView )
{
  // Look for images and store the items in our todo list :)
  for (QIconViewItem * it = m_iconView->firstItem(); it; it = it->nextItem() )
  {
    KFileIVI * ivi = static_cast<KFileIVI *>( it );
    if ( !ivi->isThumbnail() ) // Well, if we did it already, no need to do it again :-)
      if ( ivi->item()->mimetype().left(6) == "image/" )
      {
        m_items.append( ivi );
      }
  }
  determineNextIcon();
}

KonqImagePreviewJob::~KonqImagePreviewJob()
{
}

void KonqImagePreviewJob::determineNextIcon()
{
  // Get next item to determine... but check if it has been deleted first
  while ( !m_items.isEmpty() && m_items.first().isNull() )
    m_items.remove( m_items.begin() );
  // No more items ?
  if ( m_items.isEmpty() )
  {
    // Done
    emit result(this);
    delete this;
  }
  else
  {
    // First, stat the orig file
    m_state = STATE_STATORIG;
    m_currentURL = m_items.first()->item()->url();
    Job * job = KIO::stat( m_currentURL );
    kdDebug(1203) << "KonqImagePreviewJob: KIO::stat orig " << m_currentURL.url() << endl;
    addSubjob(job);
  }
}

void KonqImagePreviewJob::slotResult( KIO::Job *job )
{
  // If done, remove first item from list
  switch ( m_state ) {
    case STATE_STATORIG:
    {
      if (job->error()) // that's no good news...
      {
        Job::slotResult( job );
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
      subjobs.remove( job );
      assert ( subjobs.isEmpty() ); // We should have only one job at a time ...

      int size = m_iconView->iconSize() ? m_iconView->iconSize()
        : KGlobal::iconLoader()->currentSize( KIcon::Desktop ); // if 0

      // Check if pixie thumbnail is there first
      m_thumbURL = m_currentURL;

      if(size < 28)
        m_thumbURL.setPath( m_currentURL.directory() + "/.mospics/small/" + m_currentURL.fileName() );
      else if (size < 40 )
        m_thumbURL.setPath( m_currentURL.directory() + "/.mospics/med/" + m_currentURL.fileName() );
      else
        m_thumbURL.setPath( m_currentURL.directory() + "/.mospics/large/" + m_currentURL.fileName() );

      m_state = STATE_STATTHUMB;
      Job * job = KIO::stat( m_thumbURL );
      kdDebug(1203) << "KonqImagePreviewJob: KIO::stat thumb " << m_thumbURL.url() << endl;
      addSubjob(job);

      break;
    }
    case STATE_STATTHUMB:
    {
      bool bThumbnail = false;
      if (!job->error()) // found a thumbnail
      {
        KIO::UDSEntry entry = ((KIO::StatJob*)job)->statResult();
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

      if ( bThumbnail )
      {
        m_state = STATE_GETTHUMB;
        if ( m_thumbURL.isLocalFile() )
        {
          QPixmap pix;
          if ( pix.load( m_thumbURL.path() ) )
          {
            // Found it, use it
            m_items.first()->setPixmap( pix );
            determineNextIcon();
            return;
          }
        }
        else
        {
          QString localFile = tmpnam(0);  // Generate a temp file name
          // TODO
          return;
        }
      }
      m_state = STATE_STATXV;
      // TODO
      break;
    }
    case STATE_STATXV:
      break;
    case STATE_GETTHUMB:
      break;
    case STATE_GETORIG:
      break;
    case STATE_CREATEDIR1:
      break;
    case STATE_CREATEDIR2:
      break;
    case STATE_PUTTHUMB:
      break;
  }
}

#include "konqimagepreviewjob.moc"
