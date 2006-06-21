/* -*- c-basic-offset: 2 -*-
   This file is part of the KDE project
   Copyright (C) 1998-2005 David Faure <faure@kde.org>

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


#include "konq_view.h"
//#include "KonqViewIface.h"
#include "konq_settingsxt.h"
#include "konq_frame.h"
#include "konq_run.h"
#include "konq_events.h"
#include "konq_viewmgr.h"
#include "konq_tabs.h"
#include "konq_browseriface.h"
#include <kparts/statusbarextension.h>
#include <kparts/browserextension.h>

#include <konq_historymgr.h>
#include <konq_pixmapprovider.h>

#include <assert.h>
#include <kdebug.h>
#include <kcursor.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <fixx11h.h>
#include <krandom.h>

#include <QApplication>
#include <QMetaObject>
#include <QObject>
#include <QByteArray>
#include <QEvent>
#include <QDropEvent>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QMouseEvent>
#include <QFile>
#include <QScrollArea>

//#define DEBUG_HISTORY

KonqView::KonqView( KonqViewFactory &viewFactory,
                    KonqFrame* viewFrame,
                    KonqMainWindow *mainWindow,
                    const KService::Ptr &service,
                    const KService::List &partServiceOffers,
                    const KService::List &appServiceOffers,
                    const QString &serviceType,
                    bool passiveMode
                    )
{
  m_pKonqFrame = viewFrame;
  m_pKonqFrame->setView( this );

  m_sLocationBarURL = "";
  m_pageSecurity = KonqMainWindow::NotCrypted;
  m_bLockHistory = false;
  m_doPost = false;
  m_pMainWindow = mainWindow;
  m_pRun = 0L;
  m_pPart = 0L;
  m_dcopObject = 0L;

  m_randID = KRandom::random();

  m_service = service;
  m_partServiceOffers = partServiceOffers;
  m_appServiceOffers = appServiceOffers;
  m_serviceType = serviceType;

  m_bAllowHTML = m_pMainWindow->isHTMLAllowed();
  m_lstHistoryIndex = -1;
  m_bLoading = false;
  m_bPendingRedirection = false;
  m_bPassiveMode = passiveMode;
  m_bLockedLocation = false;
  m_bLinkedView = false;
  m_bAborted = false;
  m_bToggleView = false;
  m_bHierarchicalView = false;
  m_bDisableScrolling = false;
  m_bGotIconURL = false;
  m_bPopupMenuEnabled = true;
  m_browserIface = new KonqBrowserInterface( this );
  m_bBackRightClick = KonqSettings::backRightClick();
  m_bFollowActive = false;
  m_bBuiltinView = false;
  m_bURLDropHandling = false;

  switchView( viewFactory );
}

KonqView::~KonqView()
{
  //kDebug(1202) << "KonqView::~KonqView : part = " << m_pPart << endl;

  if (KonqMainWindow::s_crashlog_file) {
     QString part_url;
     if (m_pPart)
        part_url = m_pPart->url().url();
     if (part_url.isNull())
        part_url = "";
     QByteArray line;
     line = ( QString("close(%1):%2\n").arg(m_randID,0,16).arg(part_url) ).toUtf8();
     KonqMainWindow::s_crashlog_file->write(line, line.length());
     KonqMainWindow::s_crashlog_file->flush();
  }

  // We did so ourselves for passive views
  if (m_pPart != 0L)
  {
    finishedWithCurrentURL();
    if ( isPassiveMode() )
      disconnect( m_pPart, SIGNAL( destroyed() ), m_pMainWindow->viewManager(), SLOT( slotObjectDestroyed() ) );

    delete m_pPart;
  }

  setRun( 0L );
  //kDebug(1202) << "KonqView::~KonqView " << this << " done" << endl;
}

void KonqView::openURL( const KUrl &url, const QString & locationBarURL,
                        const QString & nameFilter, bool tempFile )
{
  kDebug(1202) << "KonqView::openURL url=" << url << " locationBarURL=" << locationBarURL << endl;
  setServiceTypeInExtension();

  if (KonqMainWindow::s_crashlog_file) {
     QString part_url;
     if (m_pPart)
        part_url = m_pPart->url().url();
     if (part_url.isNull())
        part_url = "";

     QString url_url = url.url();
     if (url_url.isNull())
        url_url = QString("");

     QByteArray line;

     line = ( QString("closed(%1):%2\n").arg(m_randID,0,16).arg(part_url) ).toUtf8();
     KonqMainWindow::s_crashlog_file->write(line,line.length());
     line = ( QString("opened(%3):%4\n").arg(m_randID,0,16).arg(url_url)  ).toUtf8();
     KonqMainWindow::s_crashlog_file->write(line,line.length());
     KonqMainWindow::s_crashlog_file->flush();
  }

  KParts::BrowserExtension *ext = browserExtension();
  KParts::URLArgs args;
  if ( ext )
    args = ext->urlArgs();

  // Typing "Enter" again after the URL of an aborted view, triggers a reload.
  if ( m_bAborted && m_pPart && m_pPart->url() == url && !args.doPost())
  {
    if ( !prepareReload( args ) )
      return;
    if ( ext )
      ext->setURLArgs( args );
  }

#ifdef DEBUG_HISTORY
  kDebug(1202) << "m_bLockedLocation=" << m_bLockedLocation << " args.lockHistory()=" << args.lockHistory() << endl;
#endif
  if ( args.lockHistory() )
    lockHistory();

  if ( !m_bLockHistory )
  {
    // Store this new URL in the history, removing any existing forward history.
    // We do this first so that everything is ready if a part calls completed().
    createHistoryEntry();
  } else
    m_bLockHistory = false;

  callExtensionStringMethod( "setNameFilter(const QString&)", nameFilter );
  if ( m_bDisableScrolling )
    callExtensionMethod( "disableScrolling()" );

  setLocationBarURL( locationBarURL );
  setPageSecurity(KonqMainWindow::NotCrypted);

  if ( !args.reload )
  {
    // Save the POST data that is necessary to open this URL
    // (so that reload can re-post it)
    m_doPost = args.doPost();
    m_postContentType = args.contentType();
    m_postData = args.postData;
    // Save the referrer
    m_pageReferrer = args.metaData()["referrer"];
  }

  if ( tempFile ) {
      // Store the path to the tempfile. Yes, we could store a bool only,
      // but this would be more dangerous. If anything goes wrong in the code,
      // we might end up deleting a real file.
      if ( url.isLocalFile() )
          m_tempFile = url.path();
      else
          kWarning(1202) << "Tempfile option is set, but URL is remote: " << url << endl;
  }

  aboutToOpenURL( url, args );

  m_pPart->openURL( url );

  updateHistoryEntry(false /* don't save location bar URL yet */);
  // add pending history entry
  KonqHistoryManager::kself()->addPending( url, locationBarURL, QString());

#ifdef DEBUG_HISTORY
  kDebug(1202) << "Current position : " << historyIndex() << endl;
#endif
}

void KonqView::switchView( KonqViewFactory &viewFactory )
{
  kDebug(1202) << "KonqView::switchView" << endl;
  if ( m_pPart )
    m_pPart->widget()->hide();

  KParts::ReadOnlyPart *oldPart = m_pPart;
  m_pPart = m_pKonqFrame->attach( viewFactory ); // creates the part

  // Set the statusbar in the BE asap to avoid a KMainWindow statusbar being created.
  KParts::StatusBarExtension* sbext = statusBarExtension();
  if ( sbext )
    sbext->setStatusBar( frame()->statusbar() );

  // Activate the new part
  if ( oldPart )
  {
    m_pPart->setObjectName( oldPart->objectName() );
    emit sigPartChanged( this, oldPart, m_pPart );
    delete oldPart;
  }

  connectPart();

  QVariant prop;

  prop = m_service->property( "X-KDE-BrowserView-FollowActive");
  if (prop.isValid() && prop.toBool())
  {
    //kDebug(1202) << "KonqView::switchView X-KDE-BrowserView-FollowActive -> setFollowActive" <<endl;
    setFollowActive(true);
  }

  prop = m_service->property( "X-KDE-BrowserView-Built-Into" );
  m_bBuiltinView = (prop.isValid() && prop.toString() == "konqueror");

  if ( !m_pMainWindow->viewManager()->isLoadingProfile() )
  {
    // Honour "non-removeable passive mode" (like the dirtree)
    prop = m_service->property( "X-KDE-BrowserView-PassiveMode");
    if ( prop.isValid() && prop.toBool() )
    {
      kDebug(1202) << "KonqView::switchView X-KDE-BrowserView-PassiveMode -> setPassiveMode" << endl;
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

  prop = m_service->property( "X-KDE-BrowserView-HierarchicalView");
  if ( prop.isValid() && prop.toBool() )
  {
    kDebug() << "KonqView::switchView X-KDE-BrowserView-HierarchicalView -> setHierarchicalView" << endl;
    setHierarchicalView( true );  // set as hierarchial
  }
  else
  {
    setHierarchicalView( false );
  }
}

bool KonqView::changeViewMode( const QString &serviceType,
                               const QString &serviceName,
                               bool forceAutoEmbed )
{
  // Caller should call stop first.
  assert ( !m_bLoading );

  kDebug(1202) << "changeViewMode: serviceType is " << serviceType
                << " serviceName is " << serviceName
                << " current service name is " << m_service->desktopEntryName() << endl;

  if ( KMimeType::mimeType(serviceType)->is(m_serviceType) && (serviceName.isEmpty() || serviceName == m_service->desktopEntryName()) )
    return true;

  if ( isLockedViewMode() )
  {
    //kDebug(1202) << "This view's mode is locked - can't change" << endl;
    return false; // we can't do that if our view mode is locked
  }

  kDebug(1202) << "Switching view modes..." << endl;
  KService::List partServiceOffers, appServiceOffers;
  KService::Ptr service;
  KonqViewFactory viewFactory = KonqFactory::createView( serviceType, serviceName, &service, &partServiceOffers, &appServiceOffers, forceAutoEmbed );

  if ( viewFactory.isNull() )
  {
    // Revert location bar's URL to the working one
    if(currentHistoryEntry())
      setLocationBarURL( currentHistoryEntry()->locationBarURL );
    return false;
  }

  m_serviceType = serviceType;
  m_partServiceOffers = partServiceOffers;
  m_appServiceOffers = appServiceOffers;

  // Check if that's already the kind of part we have -> no need to recreate it
  // Note: we should have an operator= for KService...
  if ( m_service && m_service->desktopEntryPath() == service->desktopEntryPath() )
  {
    kDebug( 1202 ) << "KonqView::changeViewMode. Reusing service. Service type set to " << m_serviceType << endl;
    if (  m_pMainWindow->currentView() == this )
      m_pMainWindow->updateViewModeActions();
  }
  else
  {
    m_service = service;

    switchView( viewFactory );
  }

  if ( m_pMainWindow->viewManager()->activePart() != m_pPart )
  {
    // Make the new part active. Note that we don't do it each time we
    // open a URL (becomes awful in view-follows-view mode), but we do
    // each time we change the view mode.
    // We don't do it in switchView either because it's called from the constructor too,
    // where the location bar url isn't set yet.
    //kDebug(1202) << "Giving focus to new part " << m_pPart << endl;
    m_pMainWindow->viewManager()->setActivePart( m_pPart );
  }
  return true;
}

void KonqView::connectPart(  )
{
  //kDebug(1202) << "KonqView::connectPart" << endl;
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

  if ( ext )
  {
      ext->setBrowserInterface( m_browserIface );

      connect( ext, SIGNAL( openURLRequestDelayed( const KUrl &, const KParts::URLArgs &) ),
               m_pMainWindow, SLOT( slotOpenURLRequest( const KUrl &, const KParts::URLArgs & ) ) );

      if ( m_bPopupMenuEnabled )
      {
          m_bPopupMenuEnabled = false; // force
          enablePopupMenu( true );
      }

      connect( ext, SIGNAL( setLocationBarURL( const QString & ) ),
               this, SLOT( setLocationBarURL( const QString & ) ) );

      connect( ext, SIGNAL( setIconURL( const KUrl & ) ),
               this, SLOT( setIconURL( const KUrl & ) ) );

      connect( ext, SIGNAL( setPageSecurity( int ) ),
               this, SLOT( setPageSecurity( int ) ) );

      connect( ext, SIGNAL( createNewWindow( const KUrl &, const KParts::URLArgs & ) ),
               m_pMainWindow, SLOT( slotCreateNewWindow( const KUrl &, const KParts::URLArgs & ) ) );

      connect( ext, SIGNAL( createNewWindow( const KUrl &, const KParts::URLArgs &, const KParts::WindowArgs &, KParts::ReadOnlyPart *& ) ),
               m_pMainWindow, SLOT( slotCreateNewWindow( const KUrl &, const KParts::URLArgs &, const KParts::WindowArgs &, KParts::ReadOnlyPart *& ) ) );

      connect( ext, SIGNAL( loadingProgress( int ) ),
               m_pKonqFrame->statusbar(), SLOT( slotLoadingProgress( int ) ) );

      connect( ext, SIGNAL( speedProgress( int ) ),
               m_pKonqFrame->statusbar(), SLOT( slotSpeedProgress( int ) ) );

      connect( ext, SIGNAL( selectionInfo( const KFileItemList & ) ),
               this, SLOT( slotSelectionInfo( const KFileItemList & ) ) );

      connect( ext, SIGNAL( mouseOverInfo( const KFileItem * ) ),
               this, SLOT( slotMouseOverInfo( const KFileItem * ) ) );

      connect( ext, SIGNAL( openURLNotify() ),
               this, SLOT( slotOpenURLNotify() ) );

      connect( ext, SIGNAL( enableAction( const char *, bool ) ),
               this, SLOT( slotEnableAction( const char *, bool ) ) );

      connect( ext, SIGNAL( setActionText( const char *, const QString& ) ),
               this, SLOT( slotSetActionText( const char *, const QString& ) ) );

      connect( ext, SIGNAL( moveTopLevelWidget( int, int ) ),
               this, SLOT( slotMoveTopLevelWidget( int, int ) ) );

      connect( ext, SIGNAL( resizeTopLevelWidget( int, int ) ),
               this, SLOT( slotResizeTopLevelWidget( int, int ) ) );

      connect( ext, SIGNAL( requestFocus(KParts::ReadOnlyPart *) ),
               this, SLOT( slotRequestFocus(KParts::ReadOnlyPart *) ) );

      if (service()->desktopEntryName() != "konq_sidebartng") {
          connect( ext, SIGNAL( infoMessage( const QString & ) ),
               m_pKonqFrame->statusbar(), SLOT( message( const QString & ) ) );

          connect( ext,
                   SIGNAL( addWebSideBar(const KUrl&, const QString&) ),
                   m_pMainWindow,
                   SLOT( slotAddWebSideBar(const KUrl&, const QString&) ) );
      }

      callExtensionBoolMethod( "setSaveViewPropertiesLocally(bool)", m_pMainWindow->saveViewPropertiesLocally() );
  }

  QVariant urlDropHandling;

  if ( ext )
      urlDropHandling = ext->property( "urlDropHandling" );
  else
      urlDropHandling = QVariant( true );

  // Handle url drops if
  //  a) either the property says "ok"
  //  or
  //  b) the part is a plain krop (no BE)
  m_bURLDropHandling = ( urlDropHandling.type() == QVariant::Bool &&
                         urlDropHandling.toBool() );

  m_pPart->widget()->installEventFilter( this );

  if (m_bBackRightClick && m_pPart->widget()->inherits("QScrollView") )
  {
    (static_cast<QScrollArea *>(m_pPart->widget()))->viewport()->installEventFilter( this );
  }

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

void KonqView::slotSetActionText( const char* name, const QString& text )
{
    if ( m_pMainWindow->currentView() == this )
        m_pMainWindow->setActionText( name, text );
    // Otherwise, we don't have to do anything, the state of the action is
    // stored inside the browser-extension.
}

void KonqView::slotMoveTopLevelWidget( int x, int y )
{
  KonqFrameContainerBase* container = frame()->parentContainer();
  // If tabs are shown, only accept to move the whole window if there's only one tab.
  if ( container->frameType() != "Tabs" || static_cast<KonqFrameTabs*>(container)->count() == 1 )
    m_pMainWindow->move( x, y );
}

void KonqView::slotResizeTopLevelWidget( int w, int h )
{
  KonqFrameContainerBase* container = frame()->parentContainer();
  // If tabs are shown, only accept to resize the whole window if there's only one tab.
  // ### Maybe we could store the size requested by each tab and resize the window to the biggest one.
  if ( container->frameType() != "Tabs" || static_cast<KonqFrameTabs*>(container)->count() == 1 )
    m_pMainWindow->resize( w, h );
}

void KonqView::slotStarted( KIO::Job * job )
{
  //kDebug(1202) << "KonqView::slotStarted"  << job << endl;
  setLoading( true );

  if (job)
  {
      // Manage passwords properly...
      kDebug(7035) << "slotStarted: Window ID = " << m_pMainWindow->topLevelWidget()->winId() << endl;
      job->setWindow (m_pMainWindow->topLevelWidget ());

      connect( job, SIGNAL( percent( KJob *, unsigned long ) ), this, SLOT( slotPercent( KJob *, unsigned long ) ) );
      connect( job, SIGNAL( speed( KIO::Job *, unsigned long ) ), this, SLOT( slotSpeed( KIO::Job *, unsigned long ) ) );
      connect( job, SIGNAL( infoMessage( KJob *, const QString &, const QString & ) ), this, SLOT( slotInfoMessage( KJob *, const QString & ) ) );
  }
}

void KonqView::slotRequestFocus( KParts::ReadOnlyPart * )
{
  m_pMainWindow->viewManager()->showTab(this);
}

void KonqView::setLoading( bool loading, bool hasPending /*= false*/)
{
    //kDebug(1202) << "KonqView::setLoading loading=" << loading << " hasPending=" << hasPending << endl;
    m_bLoading = loading;
    m_bPendingRedirection = hasPending;
    if ( m_pMainWindow->currentView() == this )
        m_pMainWindow->updateToolBarActions( hasPending );

    m_pMainWindow->viewManager()->setLoading( this, loading || hasPending );
}

void KonqView::slotPercent( KJob *, unsigned long percent )
{
  m_pKonqFrame->statusbar()->slotLoadingProgress( percent );
}

void KonqView::slotSpeed( KIO::Job *, unsigned long bytesPerSecond )
{
  m_pKonqFrame->statusbar()->slotSpeedProgress( bytesPerSecond );
}

void KonqView::slotInfoMessage( KJob *, const QString &msg )
{
  m_pKonqFrame->statusbar()->message( msg );
}

void KonqView::slotCompleted()
{
   slotCompleted( false );
}

void KonqView::slotCompleted( bool hasPending )
{
  //kDebug(1202) << "KonqView::slotCompleted hasPending=" << hasPending << endl;
  m_pKonqFrame->statusbar()->slotLoadingProgress( -1 );

  if ( ! m_bLockHistory )
  {
      // Success... update history entry, including location bar URL
      updateHistoryEntry( true );

      if ( m_bAborted ) // remove the pending entry on error
          KonqHistoryManager::kself()->removePending( url() );
      else if ( currentHistoryEntry() ) // register as proper history entry
          KonqHistoryManager::kself()->confirmPending(url(), typedUrl(),
						      currentHistoryEntry()->title);

      emit viewCompleted( this );
  }
  setLoading( false, hasPending );

  if (!m_bGotIconURL && !m_bAborted)
  {
    if ( KonqSettings::enableFavicon() == true )
    {
      // Try to get /favicon.ico
      if ( supportsServiceType( "text/html" ) && url().protocol().startsWith( "http" ) )
          KonqPixmapProvider::downloadHostIcon( url() );
    }
  }
}

void KonqView::slotCanceled( const QString & errorMsg )
{
  kDebug(1202) << "KonqView::slotCanceled" << endl;
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

void KonqView::slotMouseOverInfo( const KFileItem *item )
{
  KonqFileMouseOverEvent ev( item, m_pPart );
  QApplication::sendEvent( m_pMainWindow, &ev );
}

void KonqView::setLocationBarURL( const KUrl& locationBarURL )
{
  setLocationBarURL( locationBarURL.pathOrUrl() );
}

void KonqView::setLocationBarURL( const QString & locationBarURL )
{
  //kDebug(1202) << "KonqView::setLocationBarURL " << locationBarURL << " this=" << this << endl;

  m_sLocationBarURL = locationBarURL;
  if ( m_pMainWindow->currentView() == this )
  {
    //kDebug(1202) << "is current view " << this << endl;
    m_pMainWindow->setLocationBarURL( m_sLocationBarURL );
    m_pMainWindow->setPageSecurity( m_pageSecurity );
  }
  if (!m_bPassiveMode) setTabIcon( KUrl( m_sLocationBarURL ) );
}

void KonqView::setIconURL( const KUrl & iconURL )
// This function sets the favIcon in konqui's window if enabled,
// thus it is responsible for the icon in the taskbar.
// It does not set the tab's favIcon.
{
  if ( KonqSettings::enableFavicon() )
  {
    KonqPixmapProvider::setIconForURL( KUrl( m_sLocationBarURL ), iconURL );
    m_bGotIconURL = true;
  }
}

void KonqView::setPageSecurity( int pageSecurity )
{
  m_pageSecurity = (KonqMainWindow::PageSecurity)pageSecurity;

  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->setPageSecurity( m_pageSecurity );
}

void KonqView::setTabIcon( const KUrl &url )
{
  if (!m_bPassiveMode) frame()->setTabIcon( url, 0L );
}

void KonqView::setCaption( const QString & caption )
{
  if (caption.isEmpty()) return;

  QString adjustedCaption = caption;
  // For local URLs we prefer to use only the directory name
  if (url().isLocalFile())
  {
     // Is the caption a URL?  If so, is it local?  If so, only display the filename!
     KUrl url(caption);
     if (url.isValid() && url.isLocalFile() && url.fileName() == this->url().fileName())
        adjustedCaption = url.fileName();
  }

  m_caption = adjustedCaption;
  if (!m_bPassiveMode) frame()->setTitle( adjustedCaption , 0L );
}

void KonqView::slotOpenURLNotify()
{
#ifdef DEBUG_HISTORY
  kDebug(1202) << "KonqView::slotOpenURLNotify" << endl;
#endif
  updateHistoryEntry(true);
  createHistoryEntry();
  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->updateToolBarActions();
}

void KonqView::createHistoryEntry()
{
    // First, remove any forward history
    HistoryEntry * current = currentHistoryEntry();
    if (current)
    {
#ifdef DEBUG_HISTORY
        kDebug(1202) << "Truncating history" << endl;
#endif
        while ( current != m_lstHistory.last() )
            delete m_lstHistory.takeLast();
    }
    // Append a new entry
#ifdef DEBUG_HISTORY
    kDebug(1202) << "Append a new entry" << endl;
#endif
    m_lstHistory.append( new HistoryEntry );
    setHistoryIndex( m_lstHistory.count()-1 ); // made current
#ifdef DEBUG_HISTORY
    kDebug(1202) << "at=" << historyIndex() << " count=" << m_lstHistory.count() << endl;
#endif
}

void KonqView::updateHistoryEntry( bool saveLocationBarURL )
{
  Q_ASSERT( !m_bLockHistory ); // should never happen

  HistoryEntry * current = currentHistoryEntry();
  if ( !current )
    return;

  if ( browserExtension() )
  {
    current->buffer = QByteArray(); // Start with empty buffer.
    QDataStream stream( &current->buffer, QIODevice::WriteOnly );

    browserExtension()->saveState( stream );
  }

#ifdef DEBUG_HISTORY
  kDebug(1202) << "Saving part URL : " << m_pPart->url() << " in history position " << historyIndex() << endl;
#endif
  current->url = m_pPart->url();

  if (saveLocationBarURL)
  {
#ifdef DEBUG_HISTORY
    kDebug(1202) << "Saving location bar URL : " << m_sLocationBarURL << " in history position " << historyIndex() << endl;
#endif
    current->locationBarURL = m_sLocationBarURL;
    current->pageSecurity = m_pageSecurity;
  }
#ifdef DEBUG_HISTORY
  kDebug(1202) << "Saving title : " << m_caption << " in history position " << historyIndex() << endl;
#endif
  current->title = m_caption;
  current->strServiceType = m_serviceType;
  current->strServiceName = m_service->desktopEntryName();

  current->doPost = m_doPost;
  current->postData = m_doPost ? m_postData : QByteArray();
  current->postContentType = m_doPost ? m_postContentType : QString();
  current->pageReferrer = m_pageReferrer;
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
  {
#ifdef DEBUG_HISTORY
      kDebug(1202) << "KonqView::go(0)" << endl;
#endif
      // [David] and you're right. And they expect that it reloads, apparently.
      // [George] I'm going to make nspluginviewer rely on this too. :-)
      m_pMainWindow->slotReload();
      return;
  }

  int newPos = historyIndex() + steps;
#ifdef DEBUG_HISTORY
  kDebug(1202) << "go : steps=" << steps
                << " newPos=" << newPos
                << " m_lstHistory.count()=" << m_lstHistory.count()
                << endl;
#endif
  if( newPos < 0 || newPos >= m_lstHistory.count() )
    return;

  stop();

  setHistoryIndex( newPos ); // sets current item

#ifdef DEBUG_HISTORY
  kDebug(1202) << "New position " << historyIndex() << endl;
#endif

  restoreHistory();
}

void KonqView::restoreHistory()
{
  HistoryEntry h( *currentHistoryEntry() ); // make a copy of the current history entry, as the data
                                          // the pointer points to will change with the following calls

#ifdef DEBUG_HISTORY
  kDebug(1202) << "Restoring servicetype/name, and location bar URL from history : " << h.locationBarURL << endl;
#endif
  setLocationBarURL( h.locationBarURL );
  setPageSecurity( h.pageSecurity );
  m_sTypedURL.clear();
  if ( ! changeViewMode( h.strServiceType, h.strServiceName ) )
  {
    kWarning(1202) << "Couldn't change view mode to " << h.strServiceType
                    << " " << h.strServiceName << endl;
    return /*false*/;
  }

  setServiceTypeInExtension();

  aboutToOpenURL( h.url );

  if ( browserExtension() )
  {
    //kDebug(1202) << "Restoring view from stream" << endl;
    QDataStream stream( h.buffer );

    browserExtension()->restoreState( stream );

    m_doPost = h.doPost;
    m_postContentType = h.postContentType;
    m_postData = h.postData;
    m_pageReferrer = h.pageReferrer;
  }
  else
    m_pPart->openURL( h.url );

  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->updateToolBarActions();

#ifdef DEBUG_HISTORY
  kDebug(1202) << "New position (2) " << historyIndex() << endl;
#endif
}

const HistoryEntry * KonqView::historyAt(int pos)
{
    return m_lstHistory.value(pos);
}

void KonqView::copyHistory( KonqView *other )
{
    qDeleteAll( m_lstHistory );
    m_lstHistory.clear();

    foreach ( HistoryEntry* he, other->m_lstHistory )
        m_lstHistory.append( new HistoryEntry( *he ) );
    setHistoryIndex(other->historyIndex());
}

KUrl KonqView::url() const
{
  assert( m_pPart );
  return m_pPart->url();
}

KUrl KonqView::upUrl() const
{
    KUrl currentURL;
    if ( m_pRun )
	currentURL = m_pRun->url();
    else
	currentURL = m_sLocationBarURL;
    return currentURL.upUrl();
}

void KonqView::setRun( KonqRun * run )
{
  if ( m_pRun )
  {
    // Tell the KonqRun to abort, but don't delete it ourselves.
    // It could be showing a message box right now. It will delete itself anyway.
    m_pRun->abort();
    // finish() will be emitted later (when back to event loop)
    // and we don't want it to call slotRunFinished (which stops the animation and stop button).
    m_pRun->disconnect( m_pMainWindow );
    if ( !run )
        frame()->unsetCursor();
  }
  else if ( run )
      frame()->setCursor( KCursor::workingCursor() );
  m_pRun = run;
}

void KonqView::stop()
{
  //kDebug(1202) << "KonqView::stop()" << endl;
  m_bAborted = false;
  finishedWithCurrentURL();
  if ( m_bLoading || m_bPendingRedirection )
  {
    // aborted -> confirm the pending url. We might as well remove it, but
    // we decided to keep it :)
    KonqHistoryManager::kself()->confirmPending( url(), m_sTypedURL );

    //kDebug(1202) << "m_pPart->closeURL()" << endl;
    m_pPart->closeURL();
    m_bAborted = true;
    m_pKonqFrame->statusbar()->slotLoadingProgress( -1 );
    setLoading( false, false );
  }
  if ( m_pRun )
  {
    // Revert to working URL - unless the URL was typed manually
    // This is duplicated with KonqMainWindow::slotRunFinished, but we can't call it
    //   since it relies on sender()...
    if ( currentHistoryEntry() && m_pRun->typedUrl().isEmpty() ) { // not typed
      setLocationBarURL( currentHistoryEntry()->locationBarURL );
      setPageSecurity( currentHistoryEntry()->pageSecurity );
    }

    setRun( 0L );
    m_pKonqFrame->statusbar()->slotLoadingProgress( -1 );
  }
  if ( !m_bLockHistory && m_lstHistory.count() > 0 )
    updateHistoryEntry(true);
}

void KonqView::finishedWithCurrentURL()
{
  if ( !m_tempFile.isEmpty() )
  {
    kDebug(1202) << "######### Deleting tempfile after use:" << m_tempFile << endl;
    QFile::remove( m_tempFile );
    m_tempFile.clear();
  }
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

void KonqView::setHierarchicalView( bool mode )
{
 m_bHierarchicalView=mode;
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

void KonqView::aboutToOpenURL( const KUrl &url, const KParts::URLArgs &args )
{
  KParts::OpenURLEvent ev( m_pPart, url, args );
  QApplication::sendEvent( m_pMainWindow, &ev );

  m_bGotIconURL = false;
  m_bAborted = false;
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

  const QList<KParts::ReadOnlyPart*> children = hostExtension->frames();
  QListIterator<KParts::ReadOnlyPart *> i(children);
  while (i.hasNext())
		res += childFrameNames( i.next() );

  return res;
}

KParts::BrowserHostExtension* KonqView::hostExtension( KParts::ReadOnlyPart *part, const QString &name )
{
    KParts::BrowserHostExtension *ext = KParts::BrowserHostExtension::childObject( part );

  if ( !ext )
    return 0;

  if ( ext->frameNames().contains( name ) )
    return ext;

  const QList<KParts::ReadOnlyPart*> children = ext->frames();
  QListIterator<KParts::ReadOnlyPart *> i(children);
  while (i.hasNext())
  {
     KParts::BrowserHostExtension *childHost = hostExtension( i.next(), name);
    if ( childHost )
      return childHost;
  }


  return 0;
}

bool KonqView::callExtensionMethod( const char *methodName )
{
  QObject *obj = KParts::BrowserExtension::childObject( m_pPart );
  if ( !obj ) // not all views have a browser extension !
    return false;

  int id = obj->metaObject()->indexOfSlot( methodName );
  if ( id == -1 )
    return false;
  QMetaObject::invokeMethod( obj, methodName,  Qt::DirectConnection);

  return true;
}

bool KonqView::callExtensionBoolMethod( const char *methodName, bool value )
{
  QObject *obj = KParts::BrowserExtension::childObject( m_pPart );
  if ( !obj ) // not all views have a browser extension !
    return false;

  int id = obj->metaObject()->indexOfSlot( methodName );
  if ( id == -1 )
    return false;

  QMetaObject::invokeMethod( obj, methodName,  Qt::DirectConnection, Q_ARG(bool,value));

  return true;
}

bool KonqView::callExtensionStringMethod( const char *methodName, QString value )
{
  QObject *obj = KParts::BrowserExtension::childObject( m_pPart );
  if ( !obj ) // not all views have a browser extension !
    return false;

  int id = obj->metaObject()->indexOfSlot( methodName );
  if ( id == -1 )
    return false;

  QMetaObject::invokeMethod( obj, methodName,  Qt::DirectConnection, Q_ARG(QString, value));

  return true;
}

bool KonqView::callExtensionURLMethod( const char *methodName, const KUrl& value )
{
  QObject *obj = KParts::BrowserExtension::childObject( m_pPart );
  if ( !obj ) // not all views have a browser extension !
    return false;

  int id = obj->metaObject()->indexOfSlot( methodName );
  if ( id == -1 )
    return false;

  QMetaObject::invokeMethod( obj, methodName,  Qt::DirectConnection, Q_ARG(KUrl, value));

  return true;
}

void KonqView::setViewName( const QString &name )
{
    //kDebug() << "KonqView::setViewName this=" << this << " name=" << name << endl;
    if ( m_pPart )
        m_pPart->setObjectName( name );
}

QString KonqView::viewName() const
{
    return m_pPart ? m_pPart->objectName() : QString();
}

void KonqView::enablePopupMenu( bool b )
{
  Q_ASSERT( m_pMainWindow );

  KParts::BrowserExtension *ext = browserExtension();

  if ( !ext )
    return;

  if ( m_bPopupMenuEnabled == b )
      return;

  // enable context popup
  if ( b ) {
    m_bPopupMenuEnabled = true;

    connect( ext, SIGNAL( popupMenu( const QPoint &, const KFileItemList & ) ),
             m_pMainWindow, SLOT( slotPopupMenu( const QPoint &, const KFileItemList & ) ) );

    connect( ext, SIGNAL( popupMenu( const QPoint &, const KUrl &, const QString &, mode_t ) ),
             m_pMainWindow, SLOT( slotPopupMenu( const QPoint &, const KUrl &, const QString &, mode_t ) ) );

    connect( ext, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList & ) ),
             m_pMainWindow, SLOT( slotPopupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList & ) ) );

    connect( ext, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList &, const KParts::URLArgs &, KParts::BrowserExtension::PopupFlags ) ),
             m_pMainWindow, SLOT( slotPopupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList &, const KParts::URLArgs &, KParts::BrowserExtension::PopupFlags ) ) );

    connect( ext, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KUrl &, const QString &, mode_t ) ),
             m_pMainWindow, SLOT( slotPopupMenu( KXMLGUIClient *, const QPoint &, const KUrl &, const QString &, mode_t ) ) );

    connect( ext, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KUrl &, const KParts::URLArgs &, KParts::BrowserExtension::PopupFlags, mode_t ) ),
             m_pMainWindow, SLOT( slotPopupMenu( KXMLGUIClient *, const QPoint &, const KUrl &, const KParts::URLArgs &, KParts::BrowserExtension::PopupFlags, mode_t ) ) );
  }
  else // disable context popup
  {
    m_bPopupMenuEnabled = false;

    disconnect( ext, SIGNAL( popupMenu( const QPoint &, const KFileItemList & ) ),
             m_pMainWindow, SLOT( slotPopupMenu( const QPoint &, const KFileItemList & ) ) );

    disconnect( ext, SIGNAL( popupMenu( const QPoint &, const KUrl &, const QString &, mode_t ) ),
             m_pMainWindow, SLOT( slotPopupMenu( const QPoint &, const KUrl &, const QString &, mode_t ) ) );

    disconnect( ext, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList & ) ),
             m_pMainWindow, SLOT( slotPopupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList & ) ) );

    disconnect( ext, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KUrl &, const QString &, mode_t ) ),
             m_pMainWindow, SLOT( slotPopupMenu( KXMLGUIClient *, const QPoint &, const KUrl &, const QString &, mode_t ) ) );
  }
  enableBackRightClick( m_bBackRightClick );
}

// caller should ensure that this is called only when b changed, or for new parts
void KonqView::enableBackRightClick( bool b )
{
    m_bBackRightClick = b;
    if ( b )
        connect( this, SIGNAL( backRightClick() ),
                 m_pMainWindow, SLOT( slotBack() ) );
    else
        disconnect( this, SIGNAL( backRightClick() ),
                    m_pMainWindow, SLOT( slotBack() ) );
}

void KonqView::reparseConfiguration()
{
    callExtensionMethod( "reparseConfiguration()" );
    bool b = KonqSettings::backRightClick();
    if ( m_bBackRightClick != b )
    {
        if (m_bBackRightClick && m_pPart->widget()->inherits("QScrollView") )
        {
            (static_cast<QScrollArea *>(m_pPart->widget()))->viewport()->installEventFilter( this );
        }
        enableBackRightClick( b );
    }
}

void KonqView::disableScrolling()
{
  m_bDisableScrolling = true;
  callExtensionMethod( "disableScrolling()" );
}

KonqViewIface * KonqView::dcopObject()
{
// TODO
//  if ( !m_dcopObject )
//      m_dcopObject = new KonqViewIface( this );
  return m_dcopObject;
}

bool KonqView::eventFilter( QObject *obj, QEvent *e )
{
    if ( !m_pPart )
        return false;
//  kDebug() << "--" << obj->className() << "--" << e->type() << "--"  << endl;
    if ( e->type() == QEvent::DragEnter && m_bURLDropHandling && obj == m_pPart->widget() )
    {
        QDragEnterEvent *ev = static_cast<QDragEnterEvent *>( e );

        if ( KUrl::List::canDecode( ev->mimeData() ) )
        {
            KUrl::List lstDragURLs = KUrl::List::fromMimeData( ev->mimeData() );

            QList<QWidget *> children = qFindChildren<QWidget *>( m_pPart->widget() );

            if ( !lstDragURLs.isEmpty()
                 && !lstDragURLs.first().url().startsWith( "javascript:", Qt::CaseInsensitive ) && // ### this looks like a hack to me
                 ev->source() != m_pPart->widget() &&
                 !children.contains( ev->source() ) )
                ev->acceptProposedAction();
        }
    }
    else if ( e->type() == QEvent::Drop && m_bURLDropHandling && obj == m_pPart->widget() )
    {
        QDropEvent *ev = static_cast<QDropEvent *>( e );

        KUrl::List lstDragURLs = KUrl::List::fromMimeData( ev->mimeData() );
        KParts::BrowserExtension *ext = browserExtension();
        if ( !lstDragURLs.isEmpty() && ext && lstDragURLs.first().isValid() )
            emit ext->openURLRequest( lstDragURLs.first() ); // this will call m_pMainWindow::slotOpenURLRequest delayed
    }

    if ( m_bBackRightClick )
    {
        if ( e->type() == QEvent::ContextMenu )
        {
            QContextMenuEvent *ev = static_cast<QContextMenuEvent *>( e );
            if ( ev->reason() == QContextMenuEvent::Mouse )
            {
                return true;
            }
        }
        else if ( e->type() == QEvent::MouseButtonPress )
        {
            QMouseEvent *ev = static_cast<QMouseEvent *>( e );
            if ( ev->button() == Qt::RightButton )
            {
                return true;
            }
        }
        else if ( e->type() == QEvent::MouseButtonRelease )
        {
            QMouseEvent *ev = static_cast<QMouseEvent *>( e );
            if ( ev->button() == Qt::RightButton )
            {
                emit backRightClick();
                return true;
            }
        }
        else if ( e->type() == QEvent::MouseMove )
        {
            QMouseEvent *ev = static_cast<QMouseEvent *>( e );
            if ( ev->button() == Qt::RightButton )
            {
                obj->removeEventFilter( this );
                QMouseEvent me( QEvent::MouseButtonPress, ev->pos(), Qt::RightButton, Qt::RightButton, Qt::NoModifier );
                QApplication::sendEvent( obj, &me );
                QContextMenuEvent ce( QContextMenuEvent::Mouse, ev->pos(), ev->globalPos() );
                QApplication::sendEvent( obj, &ce );
                obj->installEventFilter( this );
                return true;
            }
        }
    }

    if ( e->type() == QEvent::FocusIn )
    {
        setActiveInstance();
    }
    return false;
}

void KonqView::setActiveInstance()
{
  if ( m_bBuiltinView || !m_pPart->instance() /*never!*/)
      KGlobal::setActiveInstance( KGlobal::instance() );
  else
      KGlobal::setActiveInstance( m_pPart->instance() );
}

bool KonqView::prepareReload( KParts::URLArgs& args )
{
    args.reload = true;
    // Repost form data if this URL is the result of a POST HTML form.
    if ( m_doPost && !args.redirectedRequest() )
    {
        if ( KMessageBox::warningContinueCancel( 0, i18n(
            "The page you are trying to view is the result of posted form data. "
            "If you resend the data, any action the form carried out (such as search or online purchase) will be repeated. "),
            i18n( "Warning" ), i18n( "Resend" ) ) == KMessageBox::Continue )
        {
            args.setDoPost( true );
            args.setContentType( m_postContentType );
            args.postData = m_postData;
        }
        else
            return false;
    }
    // Re-set referrer
    args.metaData()["referrer"] = m_pageReferrer;

    return true;
}

KParts::BrowserExtension * KonqView::browserExtension() const
{
    return KParts::BrowserExtension::childObject( m_pPart );
}

KParts::StatusBarExtension * KonqView::statusBarExtension() const
{
    return KParts::StatusBarExtension::childObject( m_pPart );
}

bool KonqView::supportsServiceType( const QString &serviceType ) const
{
    const QStringList lst = serviceTypes();
    for( QStringList::ConstIterator it = lst.begin(); it != lst.end(); ++it ) {
        if ( *it == serviceType )
            return true;
        // Maybe we should keep around a list of KServiceType::Ptr?
        KMimeType::Ptr mime = KMimeType::mimeType( *it );
        if ( mime && mime->is( serviceType ) ) // respect inheritance
            return true;
    }
    return false;
}

#include "konq_view.moc"
