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

#include <qfile.h>

#include "konq_view.h"
#include "KonqViewIface.h"
#include "konq_factory.h"
#include "konq_frame.h"
#include "konq_mainwindow.h"
#include "konq_propsview.h"
#include "konq_run.h"
#include "konq_events.h"
#include "konq_viewmgr.h"
#include "konq_browseriface.h"
#include <kio/job.h>

#include <konq_historymgr.h>
#include <konq_pixmapprovider.h>

#include <assert.h>
#include <kdebug.h>
#include <klocale.h>
#include <kurldrag.h>

#include <qapplication.h>
#include <qmetaobject.h>
#include <qobjectlist.h>

#include <kparts/factory.h>

template class QList<HistoryEntry>;

KonqView::KonqView( KonqViewFactory &viewFactory,
                    KonqFrame* viewFrame,
                    KonqMainWindow *mainWindow,
                    const KService::Ptr &service,
                    const KTrader::OfferList &partServiceOffers,
                    const KTrader::OfferList &appServiceOffers,
                    const QString &serviceType,
                    bool passiveMode
                    )
{
  m_pKonqFrame = viewFrame;
  m_pKonqFrame->setView( this );

  m_sLocationBarURL = "";
  m_bLockHistory = false;
  m_pMainWindow = mainWindow;
  m_pRun = 0L;
  m_pPart = 0L;
  m_dcopObject = 0L;

  m_service = service;
  m_partServiceOffers = partServiceOffers;
  m_appServiceOffers = appServiceOffers;
  m_serviceType = serviceType;

  m_bAllowHTML = m_pMainWindow->isHTMLAllowed();
  m_lstHistory.setAutoDelete( true );
  m_bLoading = false;
  m_bPassiveMode = passiveMode;
  m_bLockedLocation = false;
  m_bLinkedView = false;
  m_bAborted = false;
  m_bToggleView = false;
  m_bGotIconURL = false;
  m_bPopupMenuEnabled = true;
  m_browserIface = new KonqBrowserInterface( this, "browseriface" );

  switchView( viewFactory );
}

KonqView::~KonqView()
{
  //kdDebug(1202) << "KonqView::~KonqView : part = " << m_pPart << endl;

  // We did so ourselves for passive views
  if ( isPassiveMode() && m_pPart )
      disconnect( m_pPart, SIGNAL( destroyed() ), m_pMainWindow->viewManager(), SLOT( slotObjectDestroyed() ) );

  delete m_pPart;
  delete (KonqRun *)m_pRun;
  //kdDebug(1202) << "KonqView::~KonqView " << this << " done" << endl;
}

void KonqView::openURL( const KURL &url, const QString & locationBarURL, const QString & nameFilter )
{
  kdDebug(1202) << "KonqView::openURL url=" << url.url() << " locationBarURL=" << locationBarURL << endl;
  setServiceTypeInExtension();

  KParts::BrowserExtension *ext = browserExtension();
  KParts::URLArgs args;
  if ( ext )
    args = ext->urlArgs();

  if ( args.lockHistory() )
    lockHistory();

  if ( !m_bLockHistory )
  {
    // Store this new URL in the history, removing any existing forward history.
    // We do this first so that everything is ready if a part calls completed().
    createHistoryEntry();
  } else
      m_bLockHistory = false;

  callExtensionStringMethod( "setNameFilter(QString)", nameFilter );
  setLocationBarURL( locationBarURL );

  if ( m_bAborted && m_pPart && m_pPart->url() == url )
  {
    args.reload = true;
    if ( ext )
      ext->setURLArgs( args );
  }

  m_bAborted = false;

  m_pPart->openURL( url );

  sendOpenURLEvent( url, args );

  updateHistoryEntry(false /* don't save location bar URL yet */);
  // add pending history entry
  KonqHistoryManager::kself()->addPending( url, locationBarURL, QString::null);

  //kdDebug(1202) << "Current position : " << m_lstHistory.at() << endl;
}

void KonqView::switchView( KonqViewFactory &viewFactory )
{
  kdDebug(1202) << "KonqView::switchView" << endl;
  if ( m_pPart )
    m_pPart->widget()->hide();

  KParts::ReadOnlyPart *oldPart = m_pPart;
  m_pPart = m_pKonqFrame->attach( viewFactory ); // creates the part

  // Activate the new part
  if ( oldPart )
  {
    m_pPart->setName( oldPart->name() );   
    emit sigPartChanged( this, oldPart, m_pPart );
    delete oldPart;
  }

  connectPart();

  if ( !m_pMainWindow->viewManager()->isLoadingProfile() )
  {
    // Honour "non-removeable passive mode" (like the dirtree)
    QVariant prop = m_service->property( "X-KDE-BrowserView-PassiveMode");
    if ( prop.isValid() && prop.toBool() )
    {
      kdDebug(1202) << "KonqView::switchView X-KDE-BrowserView-PassiveMode -> setPassiveMode" << endl;
      setPassiveMode( true ); // set as passive
    }

    // Honour "linked view"
    prop = m_service->property( "X-KDE-BrowserView-LinkedView");
    if ( prop.isValid() && prop.toBool() )
    {
      setLinkedView( true ); // set as linked
      // Two views : link both
      if (m_pMainWindow->viewCount() <= 2) // '1' can happen if this view is not yet in the map
      {
        KonqView * otherView = m_pMainWindow->otherView( this );
        if (otherView)
          otherView->setLinkedView( true );
      }
    }
  }
}

bool KonqView::changeViewMode( const QString &serviceType,
                               const QString &serviceName )
{
  // Caller should call stop first.
  assert ( !m_bLoading );

  kdDebug(1202) << "changeViewMode: serviceType is " << serviceType
                << " serviceName is " << serviceName
                << " current service name is " << m_service->desktopEntryName() << endl;

  if ( !m_service->serviceTypes().contains( serviceType ) ||
       ( !serviceName.isEmpty() && serviceName != m_service->desktopEntryName() ) )
  {

    if ( isLockedViewMode() )
    {
      kdDebug(1202) << "This view's mode is locked - can't change" << endl;
      return false; // we can't do that if our view mode is locked
    }

    kdDebug(1202) << "Switching view modes..." << endl;
    KTrader::OfferList partServiceOffers, appServiceOffers;
    KService::Ptr service = 0L;
    KonqViewFactory viewFactory = KonqFactory::createView( serviceType, serviceName, &service, &partServiceOffers, &appServiceOffers );

    if ( viewFactory.isNull() )
    {
      // Revert location bar's URL to the working one
      if(history().current())
        setLocationBarURL( history().current()->locationBarURL );
      return false;
    }

    m_service = service;
    m_partServiceOffers = partServiceOffers;
    m_appServiceOffers = appServiceOffers;
    m_serviceType = serviceType;

    switchView( viewFactory );

    // Make the new part active. Note that we don't do it each time we
    // open a URL (becomes awful in view-follows-view mode), but we do
    // each time we change the view mode.
    // We don't do it in switchView either because it's called from the constructor too,
    // where the location bar url isn't set yet.
    kdDebug(1202) << "Giving focus to new part " << m_pPart << endl;
    m_pMainWindow->viewManager()->setActivePart( m_pPart );
  }
  else
  {
      m_serviceType = serviceType;
      kdDebug(1202) << "KonqView::changeViewMode service type set to " << m_serviceType << endl;
      // Re-query the trader
      KonqFactory::getOffers( m_serviceType, &m_partServiceOffers, &m_appServiceOffers );
      if ( m_pMainWindow->currentView() == this )
          m_pMainWindow->updateViewModeActions();
  }
  return true;
}

void KonqView::connectPart(  )
{
  //kdDebug(1202) << "KonqView::connectPart" << endl;
  connect( m_pPart, SIGNAL( started( KIO::Job * ) ),
           this, SLOT( slotStarted( KIO::Job * ) ) );
  connect( m_pPart, SIGNAL( completed() ),
           this, SLOT( slotCompleted() ) );
  connect( m_pPart, SIGNAL( completed(bool) ),
           this, SLOT( slotCompleted(bool) ) );
  connect( m_pPart, SIGNAL( canceled( const QString & ) ),
           this, SLOT( slotCanceled( const QString & ) ) );
  connect( m_pPart, SIGNAL( setWindowCaption( const QString & ) ),
           this, SLOT( setCaption( const QString & ) ) );

  KParts::BrowserExtension *ext = browserExtension();

  if ( !ext )
    return;

  ext->setBrowserInterface( m_browserIface );

  connect( ext, SIGNAL( openURLRequestDelayed( const KURL &, const KParts::URLArgs &) ),
           m_pMainWindow, SLOT( slotOpenURLRequest( const KURL &, const KParts::URLArgs & ) ) );

  if ( m_bPopupMenuEnabled )
  {
    m_bPopupMenuEnabled = false; // force
    enablePopupMenu( true );
  }

  connect( ext, SIGNAL( setLocationBarURL( const QString & ) ),
           this, SLOT( setLocationBarURL( const QString & ) ) );

  connect( ext, SIGNAL( setIconURL( const KURL & ) ),
           this, SLOT( setIconURL( const KURL & ) ) );

  connect( ext, SIGNAL( createNewWindow( const KURL &, const KParts::URLArgs & ) ),
           m_pMainWindow, SLOT( slotCreateNewWindow( const KURL &, const KParts::URLArgs & ) ) );

  connect( ext, SIGNAL( createNewWindow( const KURL &, const KParts::URLArgs &, const KParts::WindowArgs &, KParts::ReadOnlyPart *& ) ),
           m_pMainWindow, SLOT( slotCreateNewWindow( const KURL &, const KParts::URLArgs &, const KParts::WindowArgs &, KParts::ReadOnlyPart *& ) ) );

  connect( ext, SIGNAL( loadingProgress( int ) ),
           m_pKonqFrame->statusbar(), SLOT( slotLoadingProgress( int ) ) );

  connect( ext, SIGNAL( speedProgress( int ) ),
           m_pKonqFrame->statusbar(), SLOT( slotSpeedProgress( int ) ) );

  connect( ext, SIGNAL( infoMessage( const QString & ) ),
           m_pKonqFrame->statusbar(), SLOT( message( const QString & ) ) );

  connect( ext, SIGNAL( selectionInfo( const KFileItemList & ) ),
           this, SLOT( slotSelectionInfo( const KFileItemList & ) ) );

  connect( ext, SIGNAL( openURLNotify() ),
           this, SLOT( slotOpenURLNotify() ) );

  connect( ext, SIGNAL( enableAction( const char *, bool ) ),
           this, SLOT( slotEnableAction( const char *, bool ) ) );

  callExtensionBoolMethod( "setSaveViewPropertiesLocally(bool)", m_pMainWindow->saveViewPropertiesLocally() );

  QVariant urlDropHandling;

  if ( browserExtension() )
      urlDropHandling = browserExtension()->property( "urlDropHandling" );
  else
      urlDropHandling = QVariant( true, 0 );

  // install the event filter when
  //  a) either the property says "ok"
  //  or
  //  b) the part is a plain krop (no BE)
  if ( urlDropHandling.type() == QVariant::Bool &&
       urlDropHandling.toBool() )
      m_pPart->widget()->installEventFilter( this );

  // KonqDirPart signal
  if ( m_pPart->inherits("KonqDirPart") )
  {
      connect( m_pPart, SIGNAL( findOpen( KonqDirPart * ) ),
               m_pMainWindow, SLOT( slotFindOpen( KonqDirPart * ) ) );
  }
}

void KonqView::slotEnableAction( const char * name, bool enabled )
{
    if ( m_pMainWindow->currentView() == this )
        m_pMainWindow->enableAction( name, enabled );
    // Otherwise, we don't have to do anything, the state of the action is
    // stored inside the browser-extension.
}

void KonqView::slotStarted( KIO::Job * job )
{
  //kdDebug(1202) << "KonqView::slotStarted"  << job << endl;
  m_bLoading = true;

  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->updateToolBarActions();

  if (job)
  {
      connect( job, SIGNAL( percent( KIO::Job *, unsigned long ) ), this, SLOT( slotPercent( KIO::Job *, unsigned long ) ) );
      connect( job, SIGNAL( speed( KIO::Job *, unsigned long ) ), this, SLOT( slotSpeed( KIO::Job *, unsigned long ) ) );
      connect( job, SIGNAL( infoMessage( KIO::Job *, const QString & ) ), this, SLOT( slotInfoMessage( KIO::Job *, const QString & ) ) );
  }
}

void KonqView::slotPercent( KIO::Job *, unsigned long percent )
{
  m_pKonqFrame->statusbar()->slotLoadingProgress( percent );
}

void KonqView::slotSpeed( KIO::Job *, unsigned long bytesPerSecond )
{
  m_pKonqFrame->statusbar()->slotSpeedProgress( bytesPerSecond );
}

void KonqView::slotInfoMessage( KIO::Job *, const QString &msg )
{
  m_pKonqFrame->statusbar()->message( msg );
}

void KonqView::slotCompleted()
{
   slotCompleted( false );
}

void KonqView::slotCompleted( bool hasPending )
{
  kdDebug(1202) << "KonqView::slotCompleted" << endl;
  m_bLoading = false;
  m_pKonqFrame->statusbar()->slotLoadingProgress( -1 );

  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->updateToolBarActions( hasPending );

  if ( ! m_bLockHistory )
  {
      // Success... update history entry (mostly for location bar URL)
      updateHistoryEntry(true);

      if ( m_bAborted ) // remove the pending entry on error
          KonqHistoryManager::kself()->removePending( url() );
      else if ( m_lstHistory.current() ) // register as proper history entry
          KonqHistoryManager::kself()->confirmPending(url(), typedURL(),
						      m_lstHistory.current()->title);

      emit viewCompleted( this );
  }
  m_bLoading = hasPending;

  if (!m_bGotIconURL)
  {
    KConfig *config = KGlobal::config();
    config->setGroup( "HTML Settings" );

    if ( config->readBoolEntry( "EnableFavicon", true ) == true )
    {
      // Try to get /favicon.ico
      if ( m_serviceType == "text/html" && url().protocol().left(4) == "http" )
      KonqPixmapProvider::self()->downloadHostIcon( url() );
    }
  }
}

void KonqView::slotCanceled( const QString & errorMsg )
{
  kdDebug(1202) << "KonqView::slotCanceled" << endl;
  // The errorMsg comes from the ReadOnlyPart's job.
  // It should probably be used in a KMessageBox
  // Let's use the statusbar for now
  m_pKonqFrame->statusbar()->message( errorMsg );
  m_bAborted = true;
  slotCompleted();
}

void KonqView::slotSelectionInfo( const KFileItemList &items )
{
  KonqFileSelectionEvent ev( items, m_pPart );
  QApplication::sendEvent( m_pMainWindow, &ev );
}

void KonqView::setLocationBarURL( const QString & locationBarURL )
{
  //kdDebug(1202) << "KonqView::setLocationBarURL " << locationBarURL << " this=" << this << endl;
  m_sLocationBarURL = locationBarURL;
  if ( m_pMainWindow->currentView() == this )
  {
    //kdDebug(1202) << "is current view " << this << endl;
    m_pMainWindow->setLocationBarURL( m_sLocationBarURL );
  }
}

void KonqView::setIconURL( const KURL & iconURL )
{
  KonqPixmapProvider::self()->setIconForURL( m_sLocationBarURL, iconURL );
  m_bGotIconURL = true;
}

void KonqView::slotOpenURLNotify()
{
  updateHistoryEntry(true);
  createHistoryEntry();
  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->updateToolBarActions();
}

void KonqView::createHistoryEntry()
{
    // First, remove any forward history
    HistoryEntry * current = m_lstHistory.current();
    if (current)
    {
        //kdDebug(1202) << "Truncating history" << endl;
        m_lstHistory.at( m_lstHistory.count() - 1 ); // go to last one
        for ( ; m_lstHistory.current() != current ; )
        {
            if ( !m_lstHistory.removeLast() ) // and remove from the end (faster and easier)
                assert(0);
        }
        // Now current is the current again.
    }
    // Append a new entry
    //kdDebug(1202) << "Append a new entry" << endl;
    m_lstHistory.append( new HistoryEntry ); // made current
    //kdDebug(1202) << "at=" << m_lstHistory.at() << " count=" << m_lstHistory.count() << endl;
    assert( m_lstHistory.at() == (int) m_lstHistory.count() - 1 );
}

void KonqView::updateHistoryEntry( bool saveLocationBarURL )
{
  ASSERT( !m_bLockHistory ); // should never happen

  HistoryEntry * current = m_lstHistory.current();
  if ( !current )
    return;

  if ( browserExtension() )
  {
    QDataStream stream( current->buffer, IO_WriteOnly );

    browserExtension()->saveState( stream );
  }

  //kdDebug(1202) << "Saving part URL : " << m_pPart->url().url() << " in history position " << m_lstHistory.at() << endl;
  current->url = m_pPart->url();

  if (saveLocationBarURL)
  {
    //kdDebug(1202) << "Saving location bar URL : " << m_sLocationBarURL << " in history position " << m_lstHistory.at() << endl;
    current->locationBarURL = m_sLocationBarURL;
  }
  //kdDebug(1202) << "Saving title : " << m_caption << " in history position " << m_lstHistory.at() << endl;
  current->title = m_caption;
  current->strServiceType = m_serviceType;
  current->strServiceName = m_service->desktopEntryName();
}

void KonqView::goHistory( int steps )
{
  // This is called by KonqBrowserInterface
  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->viewManager()->setActivePart( part() );

  // Delay the go() call (we need to return to the caller first)
  m_pMainWindow->slotGoHistoryActivated( steps );
}

void KonqView::go( int steps )
{
  if ( !steps ) // [WildFox] i bet there are sites on the net with stupid devs who do that :)
    return;

  stop();

  int newPos = m_lstHistory.at() + steps;
  /*kdDebug(1202) << "go : steps=" << steps
                << " newPos=" << newPos
                << " m_lstHistory.count()=" << m_lstHistory.count()
                << endl; */
  if( newPos < 0 || (uint)newPos >= m_lstHistory.count() )
    return;

  // Yay, we can move there without a loop !
  HistoryEntry *currentHistoryEntry = m_lstHistory.at( newPos ); // sets current item

  assert( currentHistoryEntry );
  assert( newPos == m_lstHistory.at() ); // check we moved (i.e. if I understood the docu)
  assert( currentHistoryEntry == m_lstHistory.current() );
  //kdDebug(1202) << "New position " << m_lstHistory.at() << endl;

  HistoryEntry h( *currentHistoryEntry ); // make a copy of the current history entry, as the data
                                          // the pointer points to will change with the following calls

  //kdDebug(1202) << "Restoring servicetype/name, and location bar URL from history : " << h.locationBarURL << endl;
  setLocationBarURL( h.locationBarURL );
  m_sTypedURL = QString::null;
  if ( ! changeViewMode( h.strServiceType, h.strServiceName ) )
  {
    kdWarning(1202) << "Couldn't change view mode to " << h.strServiceType
                    << " " << h.strServiceName << endl;
    return /*false*/;
  }

  setServiceTypeInExtension();

  if ( browserExtension() )
  {
    //kdDebug(1202) << "Restoring view from stream" << endl;
    QDataStream stream( h.buffer, IO_ReadOnly );

    browserExtension()->restoreState( stream );
  }
  else
    m_pPart->openURL( h.url );

  //m_bAborted = false; // should we do that ?

  sendOpenURLEvent( h.url );

  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->updateToolBarActions();

  //kdDebug(1202) << "New position (2) " << m_lstHistory.at() << endl;
}

void KonqView::copyHistory( KonqView *other )
{
    m_lstHistory.clear();

    QListIterator<HistoryEntry> it( other->m_lstHistory );
    for (; it.current(); ++it )
        m_lstHistory.append( new HistoryEntry( *it.current() ) );
}

KURL KonqView::url()
{
  assert( m_pPart );
  return m_pPart->url();
}

void KonqView::setRun( KonqRun * run )
{
  if ( m_pRun )
    delete m_pRun;
  m_pRun = run;
}

void KonqView::stop()
{
  kdDebug(1202) << "KonqView::stop()" << endl;
  m_bAborted = false;
  if ( m_bLoading )
  {
    // aborted -> confirm the pending url. We might as well remove it, but
    // we decided to keep it :)
    KonqHistoryManager::kself()->confirmPending( url(), m_sTypedURL );

    //kdDebug(1202) << "m_pPart->closeURL()" << endl;
    m_pPart->closeURL();
    m_bAborted = true;
    m_pKonqFrame->statusbar()->slotLoadingProgress( -1 );
    m_bLoading = false;
  }
  if ( m_pRun )
  {
    // Revert to working URL - unless the URL was typed manually
    // This is duplicated with KonqMainWindow::slotRunFinished, but we can't call it
    //   since it relies on sender()...
    if ( history().current() && m_pRun->typedURL().isEmpty() ) // not typed
      setLocationBarURL( history().current()->locationBarURL );

    delete static_cast<KonqRun *>(m_pRun); // should set m_pRun to 0L
    m_pKonqFrame->statusbar()->slotLoadingProgress( -1 );
  }
  if ( !m_bLockHistory && m_lstHistory.count() > 0 )
    updateHistoryEntry(true);
}

void KonqView::setPassiveMode( bool mode )
{
  // In theory, if m_bPassiveMode is true and mode is false,
  // the part should be removed from the part manager,
  // and if the other way round, it should be readded to the part manager...
  m_bPassiveMode = mode;

  if ( mode && m_pMainWindow->viewCount() > 1 && m_pMainWindow->currentView() == this )
  {
   KParts::Part * part = m_pMainWindow->viewManager()->chooseNextView( this )->part(); // switch active part
   m_pMainWindow->viewManager()->setActivePart( part );
  }

  // Update statusbar stuff
  m_pMainWindow->viewManager()->viewCountChanged();
}

void KonqView::setLinkedView( bool mode )
{
  m_bLinkedView = mode;
  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->linkViewAction()->setChecked( mode );
  frame()->statusbar()->setLinkedView( mode );
}

void KonqView::setLockedLocation( bool b )
{
  m_bLockedLocation = b;
}

void KonqView::sendOpenURLEvent( const KURL &url, const KParts::URLArgs &args )
{
  KParts::OpenURLEvent ev( m_pPart, url, args );
  QApplication::sendEvent( m_pMainWindow, &ev );

  // We also do here what we want to do after opening an URL, whether a new one
  // or one from the history (common stuff).
  m_bGotIconURL = false;
}

void KonqView::setServiceTypeInExtension()
{
  KParts::BrowserExtension *ext = browserExtension();
  if ( !ext )
    return;

  KParts::URLArgs args( ext->urlArgs() );
  args.serviceType = m_serviceType;
  ext->setURLArgs( args );
}

QStringList KonqView::frameNames() const
{
  return childFrameNames( m_pPart );
}

QStringList KonqView::childFrameNames( KParts::ReadOnlyPart *part )
{
  QStringList res;

  KParts::BrowserHostExtension *hostExtension = KParts::BrowserHostExtension::childObject( part );

  if ( !hostExtension )
    return res;

  res += hostExtension->frameNames();

  const QList<KParts::ReadOnlyPart> children = hostExtension->frames();
  QListIterator<KParts::ReadOnlyPart> it( children );
  for (; it.current(); ++it )
    res += childFrameNames( it.current() );

  return res;
}

KParts::BrowserHostExtension* KonqView::hostExtension( KParts::ReadOnlyPart *part, const QString &name )
{
    KParts::BrowserHostExtension *ext = KParts::BrowserHostExtension::childObject( part );

  if ( !ext )
    return 0;

  if ( ext->frameNames().contains( name ) )
    return ext;

  const QList<KParts::ReadOnlyPart> children = ext->frames();
  QListIterator<KParts::ReadOnlyPart> it( children );
  for (; it.current(); ++it )
  {
    KParts::BrowserHostExtension *childHost = hostExtension( it.current(), name );
    if ( childHost )
      return childHost;
  }

  return 0;
}

bool KonqView::callExtensionMethod( const char *methodName )
{
  QObject *obj = KParts::BrowserExtension::childObject( m_pPart );
  // assert(obj); Hmm, not all views have a browser extension !
  if ( !obj )
    return false;

  QMetaData * mdata = obj->metaObject()->slot( methodName );
  if( mdata )
  {
    (obj->*(mdata->ptr))();
    return true;
  };
  return false;
}

bool KonqView::callExtensionBoolMethod( const char *methodName, bool value )
{
  QObject *obj = KParts::BrowserExtension::childObject( m_pPart );
  // assert(obj); Hmm, not all views have a browser extension !
  if ( !obj )
    return false;

  typedef void (QObject::*BoolMethod)(bool);
  QMetaData * mdata = obj->metaObject()->slot( methodName );
  if( mdata )
  {
    (obj->*((BoolMethod)mdata->ptr))(value);
    return true;
  };
  return false;
}

bool KonqView::callExtensionStringMethod( const char *methodName, QString value )
{
  QObject *obj = KParts::BrowserExtension::childObject( m_pPart );
  // assert(obj); Hmm, not all views have a browser extension !
  if ( !obj )
    return false;

  //kdDebug(1202) << "KonqView::callExtensionStringMethod " << methodName << endl;
  typedef void (QObject::*StringMethod)(QString);
  QMetaData * mdata = obj->metaObject()->slot( methodName );
  if( mdata )
  {
    (obj->*((StringMethod)mdata->ptr))(value);
    return true;
    //kdDebug(1202) << "Call done" << endl;
  };
  return false;
}

void KonqView::setViewName( const QString &name )
{
    //kdDebug() << "KonqView::setViewName this=" << this << " name=" << name << endl;
    if ( m_pPart )
        m_pPart->setName( name.local8Bit().data() );
}

QString KonqView::viewName() const
{
    return m_pPart ? QString::fromLocal8Bit( m_pPart->name() ) : QString::null;
}

void KonqView::enablePopupMenu( bool b )
{
  ASSERT( m_pMainWindow );

  KParts::BrowserExtension *ext = browserExtension();

  if ( !ext )
    return;

  // enable context popup
  if ( !m_bPopupMenuEnabled && b ) {
    m_bPopupMenuEnabled = true;

    connect( ext, SIGNAL( popupMenu( const QPoint &, const KFileItemList & ) ),
             m_pMainWindow, SLOT( slotPopupMenu( const QPoint &, const KFileItemList & ) ) );

    connect( ext, SIGNAL( popupMenu( const QPoint &, const KURL &, const QString &, mode_t ) ),
             m_pMainWindow, SLOT( slotPopupMenu( const QPoint &, const KURL &, const QString &, mode_t ) ) );

    connect( ext, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList & ) ),
             m_pMainWindow, SLOT( slotPopupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList & ) ) );

    connect( ext, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KURL &, const QString &, mode_t ) ),
             m_pMainWindow, SLOT( slotPopupMenu( KXMLGUIClient *, const QPoint &, const KURL &, const QString &, mode_t ) ) );
  }

  // disable context popup
  if ( m_bPopupMenuEnabled && !b ) {
    m_bPopupMenuEnabled = false;

    disconnect( ext, SIGNAL( popupMenu( const QPoint &, const KFileItemList & ) ),
             m_pMainWindow, SLOT( slotPopupMenu( const QPoint &, const KFileItemList & ) ) );

    disconnect( ext, SIGNAL( popupMenu( const QPoint &, const KURL &, const QString &, mode_t ) ),
             m_pMainWindow, SLOT( slotPopupMenu( const QPoint &, const KURL &, const QString &, mode_t ) ) );

    disconnect( ext, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList & ) ),
             m_pMainWindow, SLOT( slotPopupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList & ) ) );

    disconnect( ext, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KURL &, const QString &, mode_t ) ),
             m_pMainWindow, SLOT( slotPopupMenu( KXMLGUIClient *, const QPoint &, const KURL &, const QString &, mode_t ) ) );
  }
}

KonqViewIface * KonqView::dcopObject()
{
  if ( !m_dcopObject )
      m_dcopObject = new KonqViewIface( this );
  return m_dcopObject;
}

bool KonqView::eventFilter( QObject *obj, QEvent *e )
{
    if ( m_pPart && obj != m_pPart->widget() )
        return false;

    if ( e->type() == QEvent::DragEnter )
    {
        QDragEnterEvent *ev = static_cast<QDragEnterEvent *>( e );

        if ( QUriDrag::canDecode( ev ) )
        {
            KURL::List lstDragURLs;
            bool ok = KURLDrag::decode( ev, lstDragURLs );

            QObjectList *children = m_pPart->widget()->queryList( "QWidget" );

            if ( ok &&
                 !lstDragURLs.first().url().contains( "javascript:", false ) && // ### this looks like a hack to me
                 ev->source() != m_pPart->widget() &&
                 children &&
                 children->findRef( ev->source() ) == -1 )
                ev->acceptAction();

            delete children;
        }
    }
    else if ( e->type() == QEvent::Drop )
    {
        QDropEvent *ev = static_cast<QDropEvent *>( e );

        KURL::List lstDragURLs;
        bool ok = KURLDrag::decode( ev, lstDragURLs );

        KParts::BrowserExtension *ext = browserExtension();
        if ( ok && ext )
            emit ext->openURLRequest( lstDragURLs.first() ); // this will call m_pMainWindow::slotOpenURLRequest delayed
    }

    return false;
}

#include "konq_view.moc"
