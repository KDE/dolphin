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
#include <kfiledialog.h>
#include <kuserprofile.h>
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

  if (m_pView)
    m_pView->setLoading(false); // first phase finished, don't confuse KonqView

  //kdDebug(1202) << "m_req.nameFilter= " << m_req.nameFilter << endl;
  //kdDebug(1202) << "m_req.typedURL= " << m_req.typedURL << endl;

  // Check if the main window wasn't deleted meanwhile
  if( !m_pMainWindow )
  {
      m_bFinished = true;
      m_bFault = true;
      m_timer.start( 0, true );
      return;
  }

  // Try to open in a view
  m_bFinished = m_pMainWindow->openView( mimeType, m_strURL, m_pView, m_req );

  // Support for saving remote files.
  if ( !m_bFinished && // couldn't embed
       mimeType != "inode/directory" && // dirs can't be saved
       !m_strURL.isLocalFile() ) // ... and remote URL
  {
      KService::Ptr offer = KServiceTypeProfile::preferredService(mimeType, true);
      if ( askSave( m_strURL, offer ) ) // ... -> ask whether to save
      { // true: saving done or canceled
          m_bFinished = true;
          m_bFault = true; // make Konqueror think there was an error, in order to stop the spinning wheel
      }
  }

  // Check if running is allowed
  if ( !m_bFinished &&  //     If not embedddable ...
       !m_bTrustedSource && // ... and untrusted source...
       !allowExecution( mimeType, m_strURL ) ) // ...and the user said no (for executables etc.)
    {
      m_bFinished = true;
      m_bFault = true; // make Konqueror think there was an error (even if we really execute it) , in order to stop the spinning wheel
    }

  if ( m_bFinished )
    {
      m_pMainWindow = 0L;
      m_timer.start( 0, true );
      return;
    }
  KIO::SimpleJob::removeOnHold(); // Kill any slave that was put on hold.
  kdDebug(1202) << "Nothing special to do in KonqRun, falling back to KRun" << endl;

  // ??????????????? (David)
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

bool KonqRun::askSave( const KURL & url, KService::Ptr offer )
{
    // Inspired from kmail
    QString question = offer ? i18n("Open '%1' using '%2'?").
                               arg( url.prettyURL() ).arg(offer->name())
                       : i18n("Open '%1' ?").arg( url.prettyURL() );
    int choice = KMessageBox::warningYesNoCancel(0L, question, QString::null,
                                                 i18n("Save to disk"), i18n("Open"));
    if ( choice == KMessageBox::Yes ) // Save
        save( url );

    return choice != KMessageBox::No; // saved or canceled -> don't open
}

void KonqRun::save( const KURL & url )
{
    // Inspired from khtml_part :-)
    KFileDialog *dlg = new KFileDialog( QString::null, QString::null /*all files*/,
                                        0L , "filedialog", true );
    dlg->setCaption(i18n("Save as"));

    dlg->setSelection( url.fileName() );
    if ( dlg->exec() )
    {
        KURL destURL( dlg->selectedURL() );
        if ( !destURL.isMalformed() )
        {
            /*KIO::Job *job =*/ KIO::file_copy( url, destURL );
            // TODO connect job result, to display errors
            // Hmm, no object to connect to, here.
            // -> to move to konqoperations
        }
    }
    delete dlg;
}
#include "konq_run.moc"
