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
#include "konq_view.h"
#include "konq_mainwindow.h"
#include <kio/job.h>
#include <kmessagebox.h>
#include <klocale.h>

#include <assert.h>
#include <iostream.h>

KonqRun::KonqRun( KonqMainWindow* mainWindow, KonqView *_childView,
                  const KURL & _url, const KonqOpenURLRequest & req, bool trustedSource )
  : KRun( _url, 0 /*mode*/, false/*_is_local_file*/, false /* no GUI */ ),
    m_req( req ), m_bTrustedSource( trustedSource )
{
  m_pMainWindow = mainWindow;
  assert( m_pMainWindow );
  m_pView = _childView;
  if (m_pView)
    m_pView->setLoading(true);
  m_bFoundMimeType = false;
}

KonqRun::~KonqRun()
{
  kdDebug(1202) << "KonqRun::~KonqRun()" << endl;
}

void KonqRun::foundMimeType( const QString & _type )
{
  kdDebug(1202) << "KonqRun::foundMimeType " << _type << endl;

  QString mimeType = _type; // this ref comes from the job, we lose it when using KIO again

  m_bFoundMimeType = true;

  assert( m_pMainWindow );

  if (m_pView)
    m_pView->setLoading(false); // first phase finished, don't confuse KonqView

  //kdDebug(1202) << "m_req.nameFilter= " << m_req.nameFilter << endl;
  //kdDebug(1202) << "m_req.typedURL= " << m_req.typedURL << endl;

  m_bFinished = m_pMainWindow->openView( mimeType, m_strURL, m_pView, m_req );
  if ( !m_bFinished &&  // .... if not embedddable ...
      !m_bTrustedSource && // ... and untrusted source...
      !allowExecution( mimeType, m_strURL ) ) // ...and the user *really* wants to execute ...
  {
      m_bFinished = true;
      m_bFault = true; // make Konqueror think there was an error (even if we really execute it) , in order to stop the spinning wheel
  }

  //  if ( m_pMainWindow->openView( _type, m_strURL, m_pView, m_req ) ||
  //       ( !m_bTrustedSource && !allowExecution( _type, m_strURL ) ) )

  if ( m_bFinished )
  {
    m_pMainWindow = 0L;
    m_bFinished = true;
    m_timer.start( 0, true );
    return;
  }
  KIO::SimpleJob::removeOnHold(); // Kill any slave that was put on hold.
  kdDebug(1202) << "Nothing special to do in KonqRun, falling back to KRun" << endl;

  m_bFault = true; // make Konqueror believe that there was an error, in order to stop the spinning wheel...
  KRun::foundMimeType( mimeType );
}

void KonqRun::scanFile()
{
  // WABA: We directly do a get for http.
  // There is no compelling reason not to do use this with other protocols
  // as well, but only http has been tested so far.
  // David: added support for FTP and SMB... See kde-cvs for comments.
  if (m_strURL.protocol().left(4) != "http" // http and https
      && m_strURL.protocol() != "ftp"
      && m_strURL.protocol() != "smb")
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
      kdDebug(1202) << "Scanfile: MIME TYPE is " << mime->name() << endl;
      foundMimeType( mime->name() );
      return;
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
  //kdDebug(1202) << "slotKonqScanFinished" << endl;
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

bool KonqRun::allowExecution( const QString &serviceType, const KURL &url )
{
    if ( serviceType != "application/x-desktop" &&
         serviceType != "application/x-executable" &&
         serviceType != "application/x-shellscript" )
      return true;

    return ( KMessageBox::warningYesNo( 0, i18n( "Do you really want to execute '%1' ? " ).arg( url.prettyURL() ) ) == KMessageBox::Yes );
}

#include "konq_run.moc"
