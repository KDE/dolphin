/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#include "kdirlister.h"

#include <assert.h>

#include <qdir.h>

#include <kapp.h>
#include <kdebug.h>
#include <kdirwatch.h>
#include <klocale.h>
#include <kio/job.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <kuserpaths.h>

KDirLister::KDirLister( bool _delayedMimeTypes )
{
  m_bComplete = true;
  m_job = 0L;
  m_lstFileItems.setAutoDelete( true );
  m_rootFileItem = 0L;
  m_bDirOnlyMode = false;
  m_bDelayedMimeTypes = _delayedMimeTypes;
}

KDirLister::~KDirLister()
{
  // Stop running jobs
  if ( m_job )
  {
    m_job->kill();
    m_job = 0;
  }
  delete m_rootFileItem; // no problem if 0L says the master
  forgetDirs();
}

void KDirLister::slotFileDirty( const QString& _file )
{
  kDebugInfo( 1203, "KDirLister::slotFileDirty( %s )", _file.ascii() );
  KFileItem * item = find( _file );
  if ( item ) {
    emit deleteItem( item );
    // We need to refresh the item, because i.e. the permissions can have changed.
    item->refresh();
    KFileItemList lst;
    lst.append( item );
    emit newItems( lst );
  }
}

void KDirLister::slotDirectoryDirty( const QString& _dir )
{
  // _dir does not contain a trailing slash
  kDebugInfo( 1203, "KDirLister::slotDirectoryDirty( %s )", _dir.ascii() );
  // Check for _dir in m_lstDirs (which contains decoded paths)
  for ( QStringList::Iterator it = m_lstDirs.begin(); it != m_lstDirs.end(); ++it )
    if ( _dir == (*it) )
    {
      updateDirectory( _dir );
      break;
    }
}

void KDirLister::openURL( const KURL& _url, bool _showDotFiles, bool _keep )
{
  if ( _url.isMalformed() )
  {
    QString tmp = i18n("Malformed URL\n%1").arg(_url.url());
    KMessageBox::error( (QWidget*)0L, tmp);
    return;
  }

  m_isShowingDotFiles = _showDotFiles;

  // TODO: Check whether the URL is really a directory

  // Stop running jobs
  if ( m_job )
  {
    m_job->kill();
    m_job = 0;
  }

  // Complete switch, don't keep previous URLs
  if ( !_keep )
    forgetDirs();

  // Automatic updating of directories ?
  if ( _url.isLocalFile() )
  {
    //kDebugInfo( 1203, "adding %s", _url.path().ascii() );
    kdirwatch->addDir( _url.path() );
    if ( !_keep ) // already done if keep == true
    {
      connect( kdirwatch, SIGNAL( dirty( const QString& ) ),
               this, SLOT( slotDirectoryDirty( const QString& ) ) );
      connect( kdirwatch, SIGNAL( fileDirty( const QString& ) ),
               this, SLOT( slotFileDirty( const QString& ) ) );
    }
  }
  m_lstDirs.append( _url.path( -1 ) ); // store path without trailing slash

  m_bComplete = false;

  m_job = KIO::listDir( m_sURL );
  connect( m_job, SIGNAL( entries( KIO::Job*, const KIO::UDSEntryList&)),
           SLOT( slotEntries( KIO::Job*, const KIO::UDSEntryList&)));
  connect( m_job, SIGNAL( result( KIO::Job * ) ),
	   SLOT( slotResult( KIO::Job * ) ) );

  m_url = _url; // keep a copy
  m_sURL = _url.url(); // filled in now, in case somebody calls url(). Will be updated later in case of redirection

  emit started( m_sURL );
  if ( !_keep )
  {
    emit clear();
    m_lstFileItems.clear(); // clear our internal list
    delete m_rootFileItem;
    m_rootFileItem = 0L;
    m_bKofficeDocs = false;
  }
}

void KDirLister::slotResult( KIO::Job * job )
{
  m_job = 0;
  m_bComplete = true;
  if (job && job->error())
  {
    job->showErrorDialog();
    emit canceled();
  } else
    emit completed();
}

void KDirLister::slotEntries( KIO::Job*, const KIO::UDSEntryList& entries )
{
  KFileItemList lstNewItems;
  KIO::UDSEntryListIterator it(entries);
  for (; it.current(); ++it) {
    QString name;

    // Find out about the name
    KIO::UDSEntry::ConstIterator entit = it.current()->begin();
    for( ; entit != it.current()->end(); entit++ )
      if ( (*entit).m_uds == KIO::UDS_NAME )
        name = (*entit).m_str;

    assert( !name.isEmpty() );

    if ( name == ".." )
      continue;
    else if ( name == "." )
    {
      KURL u( m_url );
      m_rootFileItem = new KFileItem( *(*it), u, m_bDelayedMimeTypes );
    }
    else if ( m_isShowingDotFiles || name[0] != '.' )
    {
      KURL u( m_url );
      u.addPath( name );
      //kDebugInfo(1203,"Adding %s", u.url().ascii());
      KFileItem* item = new KFileItem( *(*it), u, m_bDelayedMimeTypes );

      if ( m_bDirOnlyMode && !S_ISDIR( item->mode() ) )
      {
        delete item;
        continue;
      }

      m_lstFileItems.append( item );
      lstNewItems.append( item );
      /*
        // Detect koffice files
        QString mimeType = item->mimetype();
        if ( mimeType.left(15) == "application/x-k" )
        {
        // Currently this matches all koffice mimetypes
        // To be changed later on if anybody else uses a x-k* mimetype
        m_bKofficeDocs = true;
        }
      */
    }
  }
  emit newItems( lstNewItems );
}

void KDirLister::updateDirectory( const QString& _dir )
{
  kDebugInfo( 1203, "KDirLister::updateDirectory( %s )", _dir.ascii() );
  if ( !m_bComplete )
  {
    m_lstPendingUpdates.append( _dir );
    return;
  }

  // Stop running jobs
  if ( m_job )
  {
    m_job->kill();
    m_job = 0;
  }

  m_bComplete = false;
  m_buffer.clear();

  m_job = KIO::listDir( m_sURL );
  connect( m_job, SIGNAL( entries( KIO::Job*, const KIO::UDSEntryList&)),
           SLOT( slotUpdateEntries( KIO::Job*, const KIO::UDSEntryList&)));
  connect( m_job, SIGNAL( result( KIO::Job * ) ),
	   SLOT( slotUpdateResult( KIO::Job * ) ) );

  m_url = KURL( _dir );
  m_sURL = m_url.url();

  kDebugInfo( 1203, "update started in %s", debugString(m_sURL));

  emit started( m_sURL );
}

void KDirLister::slotUpdateResult( KIO::Job * job )
{
  m_job = 0;
  m_bComplete = true;
  if (job->error())
  {
    //don't bother the user
    //job->showErrorDialog();
    //emit canceled();
    return;
  }

  KFileItemList lstNewItems;
  QStringList::Iterator pendingIt = m_lstPendingUpdates.find( m_url.path( 0 ) );
  if ( pendingIt != m_lstPendingUpdates.end() )
    m_lstPendingUpdates.remove( pendingIt );

  // Unmark all items whose path is m_sURL
  QString sPath = m_url.path( 1 ); // with trailing slash
  QListIterator<KFileItem> kit ( m_lstFileItems );
  for( ; kit.current(); ++kit )
  {
    if ( (*kit)->url().directory( false /* keep trailing slash */, false ) == sPath )
    {
      //kDebugInfo( 1203, "slotUpdateFinished : unmarking %s", (*kit)->url().url().ascii() );
      (*kit)->unmark();
    } else
      (*kit)->mark(); // keep the other items
  }

  QValueListIterator<KIO::UDSEntry> it = m_buffer.begin();
  for( ; it != m_buffer.end(); ++it )
  {
    QString name;

    // Find out about the name
    KIO::UDSEntry::Iterator it2 = (*it).begin();
    for( ; it2 != (*it).end(); it2++ )
      if ( (*it2).m_uds == KIO::UDS_NAME )
        name = (*it2).m_str;

    assert( !name.isEmpty() );

    if ( name == "." || name == ".." )
      continue;

    if ( m_isShowingDotFiles || name[0]!='.' )
    {
      // Form the complete url
      KURL u( m_url );
      u.addPath( name );
      //kDebugInfo( 1203, "slotUpdateFinished : found %s",name.ascii() );

      // Find this icon
      bool done = false;
      QListIterator<KFileItem> kit ( m_lstFileItems );
      for( ; kit.current() && !done; ++kit )
      {
        if ( u == (*kit)->url() )
        {
          //kDebugInfo( 1203, "slotUpdateFinished : keeping %s",name.ascii() );
          (*kit)->mark();
          done = true;
        }
      }

      if ( !done )
      {
        //kDebugInfo( 1203,"slotUpdateFinished : inserting %s", name.ascii());
        KFileItem* item = new KFileItem( *it, u, m_bDelayedMimeTypes );
	
	if ( m_bDirOnlyMode && !S_ISDIR( item->mode() ) )
	{
	  delete item;
	  continue;
	}
	
        m_lstFileItems.append( item );
        lstNewItems.append( item );
        item->mark();
      }
    }
  }

  // Find all unmarked items and delete them
  QList<KFileItem> lst;
  kit.toFirst();
  for( ; kit.current(); ++kit )
  {
    if ( !(*kit)->isMarked() )
    {
      //kDebugInfo(1203,"Removing %s", (*kit)->text().ascii());
      lst.append( *kit );
    }
  }

  emit newItems( lstNewItems );

  KFileItem* kci;
  for( kci = lst.first(); kci != 0L; kci = lst.next() )
  {
    emit deleteItem( kci );
    m_lstFileItems.remove( kci );
  }

  m_buffer.clear();

  emit completed();

  // continue with pending updates
  // as this will result in a recursive loop it's sufficient to only
  // take the first entry
  pendingIt = m_lstPendingUpdates.begin();
  if ( pendingIt != m_lstPendingUpdates.end() )
    updateDirectory( *pendingIt );
}

void KDirLister::slotUpdateEntries( KIO::Job*, const KIO::UDSEntryList& list )
{
  // list is a QList, m_buffer is a QValueList, keeps a copy
  KIO::UDSEntryListIterator it(list);
  for (; it.current(); ++it) {
      m_buffer.append( *(*it) );
  }
}

void KDirLister::setShowingDotFiles( bool _showDotFiles )
{
  if ( m_isShowingDotFiles != _showDotFiles )
  {
    m_isShowingDotFiles = _showDotFiles;
    // updateDirectory( m_sURL ); // update only one directory
    for ( QStringList::Iterator it = m_lstDirs.begin(); it != m_lstDirs.end(); ++it )
      updateDirectory( *it ); // update all directories
  }
}

KFileItem* KDirLister::find( const QString& _url )
{
  QListIterator<KFileItem> it = m_lstFileItems;
  for( ; it.current(); ++it )
  {
    if ( (*it)->url() == _url )
      return (*it);
  }

  return 0L;
}

void KDirLister::forgetDirs()
{
  for ( QStringList::Iterator it = m_lstDirs.begin(); it != m_lstDirs.end(); ++it ) {
    if ( KURL( *it ).isLocalFile() )
    {
      kDebugInfo( 1203, "forgetting about %s", (*it).ascii() );
      kdirwatch->removeDir( *it );
    }
  }
  m_lstDirs.clear();
  kdirwatch->disconnect( this );
}

#include "kdirlister.moc"
