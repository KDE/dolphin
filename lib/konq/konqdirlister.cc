/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konqdirlister.h"
#include "konqfileitem.h"
#include <kdebug.h>

KonqDirLister::KonqDirLister( bool _delayedMimeTypes )
    : KDirLister( _delayedMimeTypes ), DCOPObject( "KonqDirLister" )
{
  //m_bKofficeDocs = false;
}

KonqDirLister::~KonqDirLister()
{
}

void KonqDirLister::FilesAdded( const KURL & directory )
{
  kdDebug(1203) << "FilesAdded " << directory.url() << endl;
  slotURLDirty( directory );
}

void KonqDirLister::FilesRemoved( const KURL::List & fileList )
{
  kdDebug(1203) << "FilesRemoved" << endl;
  // Mark all items
  QListIterator<KFileItem> kit ( m_lstFileItems );
  for( ; kit.current(); ++kit )
    (*kit)->mark();

  KURL::List::ConstIterator it = fileList.begin();
  for ( ; it != fileList.end() ; ++it )
  {
    // For each file removed: look in m_lstFileItems to see if we know it,
    // and if found, unmark it (for deletion)
    kit.toFirst();
    for( ; kit.current(); ++kit )
    {
      if ( (*kit)->url().cmp( (*it), true /* ignore trailing slash */ ) )
      {
        kdDebug(1203) << "FilesRemoved : unmarking " << (*kit)->url().url() << endl;
        (*kit)->unmark();
        break;
      }
    }

    if ( !kit.current() ) // we didn't find it
    {
      // maybe it's the dir we're listing ?
      // Check for dir in d->lstDirs
      // BCI: wait for KDirLister to change lstDirs() into m_lstDirs
      KURL::List m_lstDirs = lstDirs(); // BCI
      for ( KURL::List::ConstIterator dit = m_lstDirs.begin(); dit != m_lstDirs.end(); ++dit )
        if ( (*dit).cmp( (*it), true /* ignore trailing slash */ ) )
        {
          kdDebug(1203) << "emit closeView for " << (*dit).url() << endl;
          emit closeView( (*dit) );
          break;
        }
    }
  }

  // Implemented by our beloved father, KDirLister
  deleteUnmarkedItems();
}

KFileItem * KonqDirLister::createFileItem( const KIO::UDSEntry& entry,
					   const KURL& url,
					   bool determineMimeTypeOnDemand )
{
    /*
       // Detect koffice files
       QString mimeType = item->name();
       if ( mimeType.left(15) == "application/x-k" )
       {
       // Currently this matches all koffice mimetypes
       // To be changed later on if anybody else uses a x-k* mimetype
       m_bKofficeDocs = true;
       }
    */
    return new KonqFileItem( entry, url, determineMimeTypeOnDemand );
}

#include "konqdirlister.moc"
