/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

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

#include "konq_childview.h"
#include "konq_frame.h"
#include "konq_propsview.h"
#include "konq_factory.h"
#include "konq_run.h"
#include "konq_viewmgr.h"
#include "konq_mainview.h"

#include <assert.h>

#include <kdebug.h>

template class QList<HistoryEntry>;

KonqChildView::KonqChildView( KonqViewFactory &viewFactory,
			      KonqFrame* viewFrame,
			      KonqMainView *mainView,
			      const KService::Ptr &service,
			      const KTrader::OfferList &serviceOffers
                              )
{
  m_pKonqFrame = viewFrame;
  m_pKonqFrame->setChildView( this );

  m_sLocationBarURL = "";
  m_bBack = false;
  m_bForward = false;
  m_bHistoryLock = false;
  m_pMainView = mainView;
  m_pRun = 0L;
  m_pView = 0L;

  switchView( viewFactory );

  m_service = service;
  m_serviceOffers = serviceOffers;

  QStringList serviceTypes = m_service->serviceTypes();
  QStringList::ConstIterator serviceTypeIt = serviceTypes.begin();
  while ( serviceTypeIt != serviceTypes.end() )
  {
    if ( *serviceTypeIt != "Browser/View" )
      break;
    serviceTypeIt++;
  }

  if ( *serviceTypeIt == "Browser/View" )
    qFatal( "invalid history entry! we're missing a proper servicetype!" );

  m_serviceType = *serviceTypeIt;

  m_bAllowHTML = KonqPropsView::defaultProps()->isHTMLAllowed();
  m_lstBack.setAutoDelete( true );
  m_lstForward.setAutoDelete( true );
  m_bReloadURL = false;
  m_iXOffset = 0;
  m_iYOffset = 0;
  m_bLoading = false;
  m_bViewStarted = false;
  m_iProgress = -1;
  m_bPassiveMode = false;
  m_bProgressSignals = true;
}

KonqChildView::~KonqChildView()
{
  kdebug(0,1202,"KonqChildView::~KonqChildView");
  delete m_pView;
  delete m_pKonqFrame;
  delete (KonqRun *)m_pRun;
}

void KonqChildView::repaint()
{
//  kdebug(0,1202,"KonqChildView::repaint()");
  if (m_pKonqFrame != 0L)
    m_pKonqFrame->repaint();
//  kdebug(0,1202,"KonqChildView::repaint() : done");
}

void KonqChildView::show()
{
  kdebug(0,1202,"KonqChildView::show()");
  if ( m_pKonqFrame )
    m_pKonqFrame->show();
}

void KonqChildView::openURL( const QString &url, bool useMiscURLData  )
{
  m_pView->openURL( KURL( url ) );

  if ( useMiscURLData && browserView() )
    browserView()->setXYOffset( m_iXOffset, m_iYOffset );

  QString decodedURL = url;
  KURL::decode( decodedURL );
  m_pMainView->setLocationBarURL( this, decodedURL );
}

void KonqChildView::switchView( KonqViewFactory &viewFactory )
{
  if ( m_pView )
    m_pView->widget()->hide();

  KParts::ReadOnlyPart *newView = m_pKonqFrame->attach( viewFactory );

  if ( m_pView )
  {
    emit sigViewChanged( m_pView, newView );

    delete m_pView;
  }

  m_pView = newView;
  connectView();
  show();
}

bool KonqChildView::changeViewMode( const QString &serviceType,
                                    const QString &_url, bool useMiscURLData,
				    const QString &serviceName )
{
  QString url = _url;
  if ( url.isEmpty() )
    url = KonqChildView::url();

  if ( m_bViewStarted )
    stop();

  makeHistory( false );

  if ( !m_service->serviceTypes().contains( serviceType ) ||
       ( !serviceName.isEmpty() && serviceName != m_service->name() ) )
  {

    KTrader::OfferList serviceOffers;
    KService::Ptr service;
    KonqViewFactory viewFactory = KonqFactory::createView( serviceType, serviceName, &service, &serviceOffers );

    if ( viewFactory.isNull() )
      return false;

    m_service = service;
    m_serviceOffers = serviceOffers;
    m_serviceType = serviceType;

    switchView( viewFactory );
  }

  openURL( url, useMiscURLData );

  return true;
}

void KonqChildView::connectView(  )
{

  connect( m_pView, SIGNAL( started() ),
           m_pMainView, SLOT( slotStarted() ) );
  connect( m_pView, SIGNAL( completed() ),
           m_pMainView, SLOT( slotCompleted() ) );
  #warning "FIXME: obey errormsg!"
  connect( m_pView, SIGNAL( canceled( const QString & ) ),
           m_pMainView, SLOT( slotCanceled() ) );

  BrowserView *view = browserView();

  if ( !view )
    return;

  connect( view, SIGNAL( openURLRequest( const QString &, bool, int, int, const QString & ) ),
           m_pMainView, SLOT( openURL( const QString &, bool, int, int, const QString & ) ) );

  connect( view, SIGNAL( setStatusBarText( const QString & ) ),
           m_pMainView, SLOT( slotSetStatusBarText( const QString & ) ) );

  connect( view, SIGNAL( popupMenu( const QPoint &, const KFileItemList & ) ),
           m_pMainView, SLOT( slotPopupMenu( const QPoint &, const KFileItemList & ) ) );

  QObject *obj = m_pView->child( 0L, "EditExtension" );
  if ( obj )
  {
    EditExtension *editExtension = (EditExtension *)obj;
    connect( editExtension, SIGNAL( selectionChanged() ),
             m_pMainView, SLOT( checkEditExtension() ) );
  }

  connect( view, SIGNAL( setLocationBarURL( const QString & ) ),
           m_pMainView, SLOT( slotSetLocationBarURL( const QString & ) ) );

  connect( view, SIGNAL( createNewWindow( const QString & ) ),
           m_pMainView, SLOT( slotCreateNewWindow( const QString & ) ) );

  connect( view, SIGNAL( loadingProgress( int ) ),
           m_pMainView, SLOT( slotLoadingProgress( int ) ) );

  connect( view, SIGNAL( speedProgress( int ) ),
           m_pMainView, SLOT( slotSpeedProgress( int ) ) );

}

void KonqChildView::makeHistory( bool pushEntry )
{
  if ( pushEntry )
  {
    if ( !m_bHistoryLock )
    {
      if ( m_bBack )
      {
        m_bBack = false;
//        kdebug(0,1202,"pushing into forward history : %s", m_pCurrentHistoryEntry->strURL.ascii() );
        m_lstForward.insert( 0, m_pCurrentHistoryEntry );
      }
      else if ( m_bForward )
      {
        m_bForward = false;
//        kdebug(0,1202,"pushing into backward history : %s", m_pCurrentHistoryEntry->strURL.ascii() );
        m_lstBack.insert( 0, m_pCurrentHistoryEntry );
      }
      else
      {
        m_lstForward.clear();
//        kdebug(0,1202,"pushing into backward history : %s", m_pCurrentHistoryEntry->strURL.ascii() );
        m_lstBack.insert( 0, m_pCurrentHistoryEntry );
      }
    }
    else
      m_bHistoryLock = false;
  }

  if ( pushEntry || !m_pCurrentHistoryEntry )
    m_pCurrentHistoryEntry = new HistoryEntry;

  if ( browserView() )
  {
    QDataStream stream( m_pCurrentHistoryEntry->buffer, IO_WriteOnly );

    browserView()->saveState( stream );
  }

  m_pCurrentHistoryEntry->strURL = m_pView->url().url();
  m_pCurrentHistoryEntry->strServiceType = m_serviceType;
  m_pCurrentHistoryEntry->strServiceName = m_service->name();
}

void KonqChildView::go( QList<HistoryEntry> &stack, int steps )
{
  assert( (int)stack.count() >= steps );

  for ( int i = 0; i < steps-1; i++ )
    stack.removeFirst();

  HistoryEntry *h = stack.first();

  assert( h );

//  m_bReloadURL = false;
//  m_iXOffset = h->xOffset;
//  m_iYOffset = h->yOffset;
//  changeViewMode( h->strServiceType, h->strURL, true, h->strServiceName );

  if ( m_bViewStarted )
    stop();

  makeHistory( false );

  if ( !m_service->serviceTypes().contains( h->strServiceType ) ||
       h->strServiceName != m_service->name() )
  {
    KTrader::OfferList serviceOffers;
    KService::Ptr service;
    KonqViewFactory viewFactory = KonqFactory::createView( h->strServiceType, h->strServiceName, &service, &serviceOffers );

    if ( viewFactory.isNull() )
     return;

    m_service = service;
    m_serviceOffers = serviceOffers;
    m_serviceType = h->strServiceType;

    switchView( viewFactory );
  }

  if ( browserView() )
  {
    QDataStream stream( h->buffer, IO_ReadOnly );

    browserView()->restoreState( stream );
  }
  else
    m_pView->openURL( KURL( h->strURL ) );

  QString url = h->strURL;
  KURL::decode( url );
  m_pMainView->setLocationBarURL( this, url );

  stack.removeFirst();
}

void KonqChildView::goBack( int steps )
{
  m_bBack = true;
  go( m_lstBack, steps );
}

void KonqChildView::goForward( int steps )
{
  m_bForward = true;
  go( m_lstForward, steps );
}

QString KonqChildView::url()
{
  assert( m_pView );
  return m_pView->url().url();
}

void KonqChildView::run( const QString & url )
{
  debug(" ********** KonqChildView::run ");
  m_pRun = new KonqRun( mainView(), this, url, 0, false, true );

  connect( m_pRun, SIGNAL( finished() ),
	   mainView(), SLOT( slotRunFinished() ) );
  connect( m_pRun, SIGNAL( error() ),
	   mainView(), SLOT( slotRunFinished() ) );

  // stop() will get called by KonqMainView::openView or the KonqRun will
  // be destroyed upon completion (autodelete)
}

void KonqChildView::stop()
{
  if ( m_bViewStarted )
  {
    m_pView->closeURL();
    m_bViewStarted = false;
  }
  else if ( m_pRun )
    delete (KonqRun *)m_pRun; // should set m_pRun to 0L

  m_bLoading = false;

    //  if ( m_pRun ) debug(" m_pRun is not NULL "); else debug(" m_pRun is NULL ");
  //if ( m_pRun ) delete (KonqRun *)m_pRun; // should set m_pRun to 0L
}

void KonqChildView::reload()
{
  m_bForward = false;
  m_bBack = false;
  lockHistory();

  //  m_pView->openURL( m_pView->url(), true, m_pView->xOffset(), m_pView->yOffset() );
  m_pView->openURL( m_pView->url() );

  if ( browserView() )
    browserView()->setXYOffset( browserView()->xOffset(), browserView()->yOffset() );
}

void KonqChildView::setPassiveMode( bool mode )
{
  m_bPassiveMode = mode;

  if ( mode && m_pMainView->viewCount() > 1 && m_pMainView->currentChildView() == this )
  //    m_pMainView->setActiveView( m_pMainView->viewManager()->chooseNextView( this )->view() );
    m_pMainView->viewManager()->setActivePart( m_pMainView->viewManager()->chooseNextView( this )->view() );

  // Hide the mode button for the last active view
  //
  KonqChildView *current = m_pMainView->currentChildView();

  if (current != 0L) {
    if ( m_pMainView->viewManager()->chooseNextView( current ) == 0L ) {
      current->frame()->header()->passiveModeCheckBox()->hide();
    } else {
      current->frame()->header()->passiveModeCheckBox()->show();
    }
  }
}

#include "konq_childview.moc"
