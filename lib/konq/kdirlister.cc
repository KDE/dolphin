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
#include <qmsgbox.h>

#include <kapp.h>
#include <kdebug.h>
#include <kdirwatch.h>
#include <klocale.h>
#include <kio_error.h>
#include <kio_job.h>
#include <kurl.h>
#include <userpaths.h>

KDirLister::KDirLister()
{
  m_bIsLocalURL = false;
  m_bComplete = true;
  m_jobId = 0;

  // Connect the timer
  connect( &m_bufferTimer, SIGNAL( timeout() ), this, SLOT( slotBufferTimeout() ) );
}

KDirLister::~KDirLister()
{
  // Stop running jobs
  if ( m_jobId )
  {
    KIOJob* job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_jobId = 0;
  }
}

void KDirLister::slotDirectoryDirty( const QString& _dir )
{
  QString f = _dir;
  // KURL::encode( f ); ?? Necessary ?
  if ( !urlcmp( m_url.url(), f, true, true ) ) 
    // Used url.cmp since there is no KURL cmp method that ignores refs...
    return;

  updateDirectory();
}

void KDirLister::openURL( const KURL& _url )
{
  if ( _url.isMalformed() )
  {
    QString tmp = i18n("Malformed URL\n%1").arg(_url.url());
    QMessageBox::critical( (QWidget*)0L, i18n( "Error" ), tmp, i18n( "OK" ) );
    return;
  }
    
  // TODO: Check whether the URL is really a directory

  // Stop running jobs
  if ( m_jobId )
  {
    KIOJob* job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_jobId = 0;
  }
  
  // Automatic updating of directories ?
  if ( _url.isLocalFile() )
  {
    kdirwatch->addDir( _url.path() );
    connect( kdirwatch, SIGNAL( dirty( const QString& ) ), 
	     this, SLOT( slotDirectoryDirty( const QString& ) ) );
  }

  m_bComplete = false;

  KIOJob* job = new KIOJob;
  connect( job, SIGNAL( sigListEntry( int, const UDSEntry& ) ), this, SLOT( slotListEntry( int, const UDSEntry& ) ) );
  connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotCloseURL( int ) ) );
  connect( job, SIGNAL( sigError( int, int, const char* ) ),
	   this, SLOT( slotError( int, int, const char* ) ) );

  m_initialURL = _url; // keep a copy
  m_bFoundOne = false;

  m_buffer.clear();
  
  m_jobId = job->id();
  job->listDir( _url.url() );

  emit started( m_initialURL.url() );
}

void KDirLister::slotError( int /*_id*/, int _errid, const char *_errortext )
{
  kioErrorDialog( _errid, _errortext );

  emit canceled();
}

void KDirLister::slotCloseURL( int /*_id*/ )
{
  if ( m_bufferTimer.isActive() )
    m_bufferTimer.stop();
  
  slotBufferTimeout(); // out of the above test, since the dir might be empty (David)
  m_jobId = 0;
  m_bComplete = true;

  emit completed();
}

void KDirLister::slotListEntry( int /*_id*/, const UDSEntry& _entry )
{
  m_buffer.append( _entry ); // Copies _entry, since m_buffer is a QValueList
  if ( !m_bufferTimer.isActive() )
    m_bufferTimer.start( 1000, true );
}

void KDirLister::slotBufferTimeout()
{
  kdebug(0,1203,"BUFFER TIMEOUT");

  //The first entry in the toplevel ?
  if ( !m_bFoundOne )
  { 
    emit clear();
    m_lstFileItems.clear(); // clear our internal list
    m_url = m_initialURL; // HACK. How to detect redirects instead ?
    m_sURL = m_url.url();
    m_bFoundOne = true; // remember not to go here again
    m_bIsLocalURL = m_url.isLocalFile();
  }
  
  QValueListIterator<UDSEntry> it = m_buffer.begin();
  for( ; it != m_buffer.end(); ++it )
  {    
    QString name;
    
    // Find out about the name
    UDSEntry::Iterator it2 = (*it).begin();
    for( ; it2 != (*it).end(); it2++ )
      if ( (*it2).m_uds == UDS_NAME )
	name = (*it2).m_str;

    bool m_isShowingDotFiles = true; // TODO
    assert( !name.isEmpty() );
    if ( m_isShowingDotFiles || name[0] != '.' ) {
      KURL u( m_url );
      u.addPath( name );
      //kdebug(0,1203,"Adding %s", u.url().ascii());
      KFileItem* item = new KFileItem( *it, u );
      m_lstFileItems.append( item );
      emit newItem( item );
    }
  }

  // kdebug(0, 1203, "Update !");
  // Tell the view to redraw itself
  emit update();
  // kdebug(0, 1203, "Update done");

  m_buffer.clear();
}

void KDirLister::updateDirectory()
{
  if ( !m_bComplete )
    return;
  
  // Stop running jobs
  if ( m_jobId )
  {
    KIOJob* job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_jobId = 0;
  }
  
  m_bComplete = false;
  m_buffer.clear();
  
  KIOJob* job = new KIOJob;
  connect( job, SIGNAL( sigListEntry( int, const UDSEntry& ) ), this, SLOT( slotUpdateListEntry( int, const UDSEntry& ) ) );
  connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotUpdateFinished( int ) ) );
  connect( job, SIGNAL( sigError( int, int, const char* ) ),
	   this, SLOT( slotUpdateError( int, int, const char* ) ) );
  
  m_jobId = job->id();
  job->listDir( m_sURL );
  kdebug(KDEBUG_INFO, 1203, "update started in %s", m_sURL.data());

  emit started( QString::null );
}

void KDirLister::slotUpdateError( int /*_id*/, int _errid, const char *_errortext )
{
  kioErrorDialog( _errid, _errortext );

  m_bComplete = true;
  
  emit canceled();
}

void KDirLister::slotUpdateFinished( int /*_id*/ )
{
  if ( m_bufferTimer.isActive() )
    m_bufferTimer.stop();
  
  // slotBufferTimeout(); //  ??
  m_jobId = 0;
  m_bComplete = true;
  
  // Unmark all items
  QListIterator<KFileItem> kit ( m_lstFileItems );
  for( ; kit.current(); ++kit )
    (*kit)->unmark();
    
  QValueListIterator<UDSEntry> it = m_buffer.begin();
  for( ; it != m_buffer.end(); ++it )
  {    
    QString name;
    
    // Find out about the name
    UDSEntry::Iterator it2 = (*it).begin();
    for( ; it2 != (*it).end(); it2++ )
      if ( (*it2).m_uds == UDS_NAME )
	name = (*it2).m_str;
  
    assert( !name.isEmpty() );

    // Form the complete url
    KURL u( m_url );
    u.addPath( name.data() );
    kdebug(KDEBUG_INFO, 1203, "slotUpdateFinished : found %s",name.ascii() );

    // Find this icon
    bool done = false;
    QListIterator<KFileItem> kit ( m_lstFileItems );
    for( ; kit.current() && !done; ++kit )
    {
      if ( u == (*kit)->url() )
      {  
        kdebug(KDEBUG_INFO, 1203, "slotUpdateFinished : keeping %s",name.ascii() );
	(*kit)->mark();
	done = true;
      }
    }
    
    if ( !done )
    {
      // HACK
      bool m_isShowingDotFiles = true;

      if ( m_isShowingDotFiles || name[0]!='.' )
      {
        kdebug(KDEBUG_INFO, 1203,"slotUpdateFinished : inserting %s", name.ascii());
        KFileItem* item = new KFileItem( *it, u );
        m_lstFileItems.append( item );
        item->mark();
        emit newItem( item );
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
      kdebug(0,1203,"Removing %s", (*kit)->getText().ascii());
      lst.append( *kit );
    }
  }
  
  KFileItem* kci;
  for( kci = lst.first(); kci != 0L; kci = lst.next() )
  {
    m_lstFileItems.remove( kci );
    emit deleteItem( kci );
  }
  
  m_buffer.clear();

  emit completed();
}

void KDirLister::slotUpdateListEntry( int /*_id*/, const UDSEntry& _entry )
{
  m_buffer.append( _entry ); // Keep a copy of _entry
}

KFileItem* KDirLister::item( const QString& _url )
{
  QListIterator<KFileItem> it = m_lstFileItems;
  for( ; it.current(); ++it )
  {
    if ( (*it)->url() == _url )
      return (*it);
  }

  return 0L;
}

#include "kdirlister.moc"
