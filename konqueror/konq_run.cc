/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#include <kdebug.h>

#include "konq_run.h"
#include "konq_mainwindow.h"
#include "kprotocolmanager.h"
#include "kio/job.h"

#include <assert.h>
#include <iostream.h>

KonqRun::KonqRun( KonqMainWindow* mainWindow, KonqView *_childView, const KURL & _url, mode_t _mode, bool _is_local_file, bool _auto_delete )
  : KRun( _url, _mode, _is_local_file, _auto_delete )
{
  m_pMainWindow = mainWindow;
  assert( m_pMainWindow );
  m_pView = _childView;
  m_bFoundMimeType = false;
}

KonqRun::~KonqRun()
{
  kdDebug(1202) << "KonqRun::~KonqRun()" << endl;
}

void KonqRun::foundMimeType( const QString & _type )
{
  kdDebug(1202) << "FILTERING " << _type << endl;

  m_bFoundMimeType = true;

  assert( m_pMainWindow );

  if ( m_pMainWindow->openView( _type, m_strURL, m_pView ) )
  {
    m_pMainWindow = 0L;
    m_bFinished = true;
    m_timer.start( 0, true );
    return;
  }
  KIO::SimpleJob::removeOnHold(); // Kill any slave that was put on hold.
  kdDebug(1202) << "Nothing special to do here" << endl;

  KRun::foundMimeType( _type );
}

void KonqRun::scanFile()
{
  // WABA: We directly do a get for http.
  // There is no compelling reason not to do use this with other protocols
  // as well, but only http has been tested so far.
  if (m_strURL.protocol() != "http")
  {
     KRun::scanFile();
     return;
  }

  // Let's check for well-known extensions
  // Not when there is a query in the URL, in any case.
  if ( m_strURL.query().isEmpty() )
  {
    KMimeType::Ptr mime = KMimeType::findByURL( m_strURL );
    assert( mime != 0L );
    if ( mime->name() != "application/octet-stream" || m_bIsLocalFile )
    {
      // Found something - can we trust it ? (see mimetypeFastMode)
      if ( KProtocolManager::self().mimetypeFastMode( m_strURL.protocol(), mime->name() ) )
      {
        kdDebug(1202) << "Scanfile: MIME TYPE is " << mime->name() << endl;
        foundMimeType( mime->name() );
        return;
      }
    }
  }

  KIO::TransferJob *job = KIO::get(m_strURL, false, false);
  connect( job, SIGNAL( result( KIO::Job *)),
           this, SLOT( slotKonqScanFinished(KIO::Job *)));
  connect( job, SIGNAL( mimetype( KIO::Job *, const QString &)),
           this, SLOT( slotKonqMimetype(KIO::Job *, const QString &)));
  m_job = job;
}

void KonqRun::slotKonqScanFinished(KIO::Job *job)
{
  kdDebug(1202) << "slotKonqScanFinished" << endl;
  KRun::slotScanFinished(job);
}

void KonqRun::slotKonqMimetype(KIO::Job *, const QString &type)
{
  kdDebug(1202) << "slotKonqMimetype" << endl;

  KIO::SimpleJob *job = (KIO::SimpleJob *) m_job;

  job->putOnHold();
  m_job = 0;

  foundMimeType( type );
}


#include "konq_run.moc"
