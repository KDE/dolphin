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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <kdebug.h>

#include "konq_run.h"
#include "konq_view.h"
#include <kuserprofile.h>
#include <kmessagebox.h>
#include <klocale.h>

#include <konq_historymgr.h>

#include <assert.h>

#define HINT_UTF8	106

KonqRun::KonqRun( KonqMainWindow* mainWindow, KonqView *_childView,
                  const KUrl & _url, const KonqOpenURLRequest & req, bool trustedSource )
    : KParts::BrowserRun( _url, req.args, _childView ? _childView->part() : 0L, mainWindow,
                          //remove referrer if request was typed in manually.
                          // ### TODO: turn this off optionally.
                          !req.typedURL.isEmpty(), trustedSource,
                          // Don't use inline errors on reloading due to auto-refresh sites, but use them in all other cases
                          // (no reload or user-requested reload)
                          !req.args.reload || req.userRequestedReload ),
    m_pMainWindow( mainWindow ), m_pView( _childView ), m_bFoundMimeType( false ), m_req( req )
{
  //kDebug(1202) << "KonqRun::KonqRun() " << this << endl;
  assert( !m_pMainWindow.isNull() );
  if (m_pView)
    m_pView->setLoading(true);

  m_timer.setSingleShot(true);
}

KonqRun::~KonqRun()
{
  //kDebug(1202) << "KonqRun::~KonqRun() " << this << endl;
  if (m_pView && m_pView->run() == this)
    m_pView->setRun(0L);
}

void KonqRun::foundMimeType( const QString & _type )
{
  //kDebug(1202) << "KonqRun::foundMimeType " << _type << " m_req=" << m_req.debug() << endl;

  QString mimeType = _type; // this ref comes from the job, we lose it when using KIO again

  m_bFoundMimeType = true;

  if (m_pView)
    m_pView->setLoading(false); // first phase finished, don't confuse KonqView

  // Check if the main window wasn't deleted meanwhile
  if( !m_pMainWindow )
  {
    m_bFinished = true;
    m_bFault = true;
    m_timer.start( 0 );
    return;
  }

  // Grab the args back from BrowserRun
  m_req.args = m_args;

  bool tryEmbed = true;
  // One case where we shouldn't try to embed, is when the server asks us to save
  // ####### only if content-disposition doesn't say inline
#if 0
  if ( !m_suggestedFilename.isEmpty() )
     tryEmbed = false;
#endif
  if ( KonqMainWindow::isMimeTypeAssociatedWithSelf( mimeType ) )
      m_req.forceAutoEmbed = true;

  if ( tryEmbed )
      m_bFinished = m_pMainWindow->openView( mimeType, m_strURL, m_pView, m_req );

  if ( m_bFinished ) {
    m_pMainWindow = 0L;
    m_timer.start( 0 );
    return;
  }

  // If we were following another view, do nothing if opening didn't work.
  if ( m_req.followMode )
    m_bFinished = true;

  if ( !m_bFinished ) {
    // If we couldn't embed the mimetype, call BrowserRun::handleNonEmbeddable()
    KParts::BrowserRun::NonEmbeddableResult res = handleNonEmbeddable( mimeType );
    if ( res == KParts::BrowserRun::Delayed )
      return;
    m_bFinished = ( res == KParts::BrowserRun::Handled );
  }

  // make Konqueror think there was an error, in order to stop the spinning wheel
  // (we saved, canceled, or we're starting another app... in any case the current view should stop loading).
  m_bFault = true;

  if ( !m_bFinished && // only if we're going to open
       KonqMainWindow::isMimeTypeAssociatedWithSelf( mimeType ) ) {
    KMessageBox::error( m_pMainWindow, i18n( "There appears to be a configuration error. You have associated Konqueror with %1, but it cannot handle this file type." ,  mimeType ) );
    m_bFinished = true;
  }

  if ( m_bFinished ) {
    m_pMainWindow = 0L;
    m_timer.start( 0 );
    return;
  }

  kDebug(1202) << "Nothing special to do in KonqRun, falling back to KRun" << endl;
  KRun::foundMimeType( mimeType );
}

void KonqRun::handleError( KIO::Job *job )
{
  kDebug(1202) << "KonqRun::handleError error:" << job->errorString() << endl;
  if (!m_mailto.isEmpty())
  {
     m_job = 0;
     m_bFinished = true;
     m_timer.start( 0 );
     return;
  }
  KParts::BrowserRun::handleError( job );
}

void KonqRun::init()
{
    KParts::BrowserRun::init();
    // Maybe init went to the "let's try stat'ing" part. Then connect to info messages.
    // (in case it goes to scanFile, this will be done below)
    KIO::StatJob *job = dynamic_cast<KIO::StatJob*>( m_job );
    if ( job && !job->error() && m_pView ) {
        connect( job, SIGNAL( infoMessage( KJob*, const QString&, const QString& ) ),
                 m_pView, SLOT( slotInfoMessage(KJob*, const QString& ) ) );
    }
}

void KonqRun::scanFile()
{
    KParts::BrowserRun::scanFile();
    // could be a static cast as of now, but who would notify when
    // BrowserRun changes
    KIO::TransferJob *job = dynamic_cast<KIO::TransferJob*>( m_job );
    if ( job && !job->error() ) {
        connect( job, SIGNAL( redirection( KIO::Job *, const KUrl& )),
                 SLOT( slotRedirection( KIO::Job *, const KUrl& ) ));
        if ( m_pView && m_pView->service()->desktopEntryName() != "konq_sidebartng") {
            connect( job, SIGNAL( infoMessage( KJob*, const QString&, const QString& ) ),
                     m_pView, SLOT( slotInfoMessage(KJob*, const QString& ) ) );
	}
    }
}

void KonqRun::slotRedirection( KIO::Job *job, const KUrl& redirectedToURL )
{
    KUrl redirectFromURL = static_cast<KIO::TransferJob *>(job)->url();
    kDebug(1202) << "KonqRun::slotRedirection from " <<
        redirectFromURL.prettyURL() << " to " << redirectedToURL.prettyURL() << endl;
    KonqHistoryManager::kself()->confirmPending( redirectFromURL );

    if (redirectedToURL.protocol() == "mailto")
    {
       m_mailto = redirectedToURL;
       return; // Error will follow
    }
    KonqHistoryManager::kself()->addPending( redirectedToURL );

    // Do not post data on reload if we were redirected to a new URL when
    // doing a POST request.
    if (redirectFromURL != redirectedToURL)
        m_args.setDoPost (false);
    m_args.setRedirectedRequest(true);
}

KonqView * KonqRun::childView() const
{
  return m_pView;
}

#include "konq_run.moc"
