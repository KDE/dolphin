/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

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

#include <qdir.h>

#include <kparts/browserextension.h>
#include "konq_guiclients.h"
#include "KonqMainWindowIface.h"
#include "konq_mainwindow.h"
#include "konq_view.h"
#include "konq_run.h"
#include "konq_misc.h"
#include "konq_factory.h"
#include "konq_viewmgr.h"
#include "konq_frame.h"
#include "konq_events.h"
#include "konq_actions.h"
#include "konq_pixmapprovider.h"

#include <pwd.h>
// we define STRICT_ANSI to get rid of some warnings in glibc
#ifndef __STRICT_ANSI__
#define __STRICT_ANSI__
#define _WE_DEFINED_IT_
#endif
#include <netdb.h>
#ifdef _WE_DEFINED_IT_
#undef __STRICT_ANSI__
#undef _WE_DEFINED_IT_
#endif
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include <qapplication.h>
#include <qclipboard.h>
#include <qmetaobject.h>
#include <qlayout.h>

#include <dcopclient.h>
#include <kaboutdata.h>
#include <kaction.h>
#include <kapp.h>
#include <kbookmarkbar.h>
#include <kbookmarkmenu.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kedittoolbar.h>
#include <kglobalsettings.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <kkeydialog.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <knewmenu.h>
#include <konq_defaults.h>
#include <konq_popupmenu.h>
#include <konq_settings.h>
#include <konq_undo.h>
#include <kparts/part.h>
#include <kpopupmenu.h>
#include <kprocess.h>
#include <kprotocolinfo.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <kstddirs.h>
#include <ksycoca.h>
#include <ktempfile.h>
#include <ktrader.h>
#include <kurl.h>
#include <kurlrequesterdlg.h>
#include <kuserprofile.h>

#include "version.h"

template class QList<QPixmap>;
template class QList<KToggleAction>;

QStringList *KonqMainWindow::s_plstAnimatedLogo = 0L;
QList<KonqMainWindow> *KonqMainWindow::s_lstViews = 0;
KonqMainWindow::ActionSlotMap *KonqMainWindow::s_actionSlotMap = 0;

KonqMainWindow::KonqMainWindow( const KURL &initialURL, bool openInitialURL, const char *name )
 : KParts::MainWindow( name, WDestructiveClose | WStyle_ContextHelp )
{
  if ( !s_lstViews )
    s_lstViews = new QList<KonqMainWindow>;

  s_lstViews->append( this );

  if ( !s_actionSlotMap )
      s_actionSlotMap = new ActionSlotMap( KParts::BrowserExtension::actionSlotMap() );

  m_currentView = 0L;
  m_pBookmarkMenu = 0L;
  m_dcopObject = 0L;
  m_combo = 0L;
  m_bURLEnterLock = false;
  m_bLocationBarConnected = false;
  m_bLockLocationBarURL = false;

  m_bViewModeToggled = false;

  if ( !s_plstAnimatedLogo )
  {
    s_plstAnimatedLogo = new QStringList;
    *s_plstAnimatedLogo += KGlobal::iconLoader()->loadAnimated( "kde", KIcon::MainToolbar );
  }

  m_pViewManager = new KonqViewManager( this );

  // See KonqViewManager::setActivePart
  //connect( m_pViewManager, SIGNAL( activePartChanged( KParts::Part * ) ),
  //       this, SLOT( slotPartActivated( KParts::Part * ) ) );

  m_toggleViewGUIClient = new ToggleViewGUIClient( this );

  m_openWithActions.setAutoDelete( true );
  m_viewModeActions.setAutoDelete( true );
  m_viewModeMenu = 0;
  m_paCopyFiles = 0L;
  m_paMoveFiles = 0L;

  initActions();

  setInstance( KGlobal::instance() );

  connect( KSycoca::self(), SIGNAL( databaseChanged() ),
           this, SLOT( slotDatabaseChanged() ) );

  connect( kapp, SIGNAL( kdisplayFontChanged()), SLOT(slotReconfigure()));

  setXMLFile( "konqueror.rc" );

  createGUI( 0L );

  if ( !m_toggleViewGUIClient->empty() )
    plugActionList( QString::fromLatin1( "toggleview" ), m_toggleViewGUIClient->actions() );
  else
  {
    delete m_toggleViewGUIClient;
    m_toggleViewGUIClient = 0;
  }

  updateBookmarkBar();

  m_bNeedApplyMainWindowSettings = true;
  if ( !initialURL.isEmpty() )
  {
    openFilteredURL( initialURL.url() );
  }
  else if ( openInitialURL )
  {
    /*
    KConfig *config = KGlobal::config();

    if ( config->hasGroup( "Default View Profile" ) )
    {
      config->setGroup( "Default View Profile" );
      enableAllActions( true );
      m_pViewManager->loadViewProfile( *config );
    }
    else
    */
    KURL homeURL;
    homeURL.setPath( QDir::homeDirPath() );
    openURL( 0L, homeURL );
  }
  else
      // silent
      m_bNeedApplyMainWindowSettings = false;

  // Read basic main-view settings
  KConfig *config = KGlobal::config();
  KConfigGroupSaver cgs( config, "MainView Settings" );
  m_bSaveViewPropertiesLocally = config->readBoolEntry( "SaveViewPropertiesLocally", false );
  m_paSaveViewPropertiesLocally->setChecked( m_bSaveViewPropertiesLocally );
  m_bHTMLAllowed = config->readBoolEntry( "HTMLAllowed", false );
  m_ptaUseHTML->setChecked( m_bHTMLAllowed );
  m_sViewModeForDirectory = config->readEntry( "ViewMode" );

  KonqUndoManager::incRef();

  connect( KonqUndoManager::self(), SIGNAL( undoAvailable( bool ) ),
           this, SLOT( slotUndoAvailable( bool ) ) );

  resize( 700, 480 );
  kdDebug(1202) << "KonqMainWindow::KonqMainWindow done" << endl;
}

KonqMainWindow::~KonqMainWindow()
{
  kdDebug(1202) << "KonqMainWindow::~KonqMainWindow" << endl;
  if ( s_lstViews )
  {
    s_lstViews->removeRef( this );
    if ( s_lstViews->count() == 0 )
    {
      delete s_lstViews;
      s_lstViews = 0;
    }
  }

  if ( m_combo )
  {
    KConfig *config = KGlobal::config();
    config->setGroup( "Settings" );
    config->writeEntry( "Maximum of URLs in combo", m_combo->maxCount() );
    QStringList histItems = m_combo->historyItems();
    config->writeEntry( "ToolBarCombo", histItems );
    config->writeEntry( "CompletionMode", (int)m_combo->completionMode() );
    config->writeEntry( "CompletionItems", m_combo->completionObject()->items() );
    KonqPixmapProvider *prov = static_cast<KonqPixmapProvider*> (m_combo->pixmapProvider());
    if ( prov )
        prov->save( config, "ComboIconCache", histItems );

    config->sync();
  }

  delete m_pViewManager;

  delete m_pBookmarkMenu;

  m_viewModeActions.clear();

  KonqUndoManager::decRef();

  kdDebug(1202) << "KonqMainWindow::~KonqMainWindow done" << endl;
}

QWidget * KonqMainWindow::createContainer( QWidget *parent, int index, const QDomElement &element, int &id )
{
  static QString nameBookmarkBar = QString::fromLatin1( "bookmarkToolBar" );
  static QString tagToolBar = QString::fromLatin1( "ToolBar" );

  QWidget *res = KParts::MainWindow::createContainer( parent, index, element, id );

  if ( element.tagName() == tagToolBar && element.attribute( "name" ) == nameBookmarkBar )
  {
    assert( res->inherits( "KToolBar" ) );

    (void)new KBookmarkBar( this, static_cast<KToolBar *>( res ), actionCollection(), this );
  }

  return res;
}

// Note: this removes the filter from the URL.
QString KonqMainWindow::detectNameFilter( QString & url )
{
    // Look for wildcard selection
    QString nameFilter;
    int pos = url.findRev( '/' );
    if ( pos != -1 )
    {
      QString lastbit = url.mid( pos + 1 );
      if ( lastbit.find( '*' ) != -1 )
      {
        nameFilter = lastbit;
        url = url.left( pos + 1 );
        kdDebug(1202) << "Found wildcard. nameFilter=" << nameFilter << "  New url=" << url << endl;
      }
    }
    return nameFilter;
}

void KonqMainWindow::openFilteredURL( const QString & _url )
{
    QString url( _url );
    QString nameFilter = detectNameFilter( url );

    // Filter URL to build a correct one
    KURL filteredURL( konqFilteredURL( this, url ) );
    kdDebug(1202) << "url " << url << " filtered into " << filteredURL.url() << endl;

    // Remember the initial (typed) URL
    KonqOpenURLRequest req( _url );
    req.nameFilter = nameFilter;

    openURL( 0L, filteredURL, QString::null, req );

    // #4070: Give focus to view after URL was entered manually
    // Note: we do it here if the view mode (i.e. part) wasn't changed
    // If it is changed, then it's done in KonqView::changeViewMode
    if ( m_currentView && m_currentView->part() )
      m_currentView->part()->widget()->setFocus();

}

void KonqMainWindow::openURL( KonqView *_view, const KURL &url,
                              const QString &serviceType, const KonqOpenURLRequest & req,
                              bool trustedSource )
{
  kdDebug(1202) << "KonqMainWindow::openURL : url = '" << url.url() << "'  "
                << "serviceType='" << serviceType << "' view=" << _view << endl;

  if ( url.isMalformed() )
  {
    KMessageBox::error(0, i18n("Malformed URL\n%1").arg(url.url()));
    return;
  }
  if ( !KProtocolInfo::isKnownProtocol( url.protocol() ) )
  {
    // after message freeze QString tmp = i_18_n("Protocol not supported\n%1").arg(url.protocol());
    KMessageBox::error(0, i18n("Malformed URL\n%1").arg(url.url()));
    return;
  }

  KonqView *view = _view;
  if ( !view )
    view = m_currentView; /* Note, this can be 0L, e.g. on startup */

  if ( view )
  {
    if ( view == m_currentView )
    {
      //will do all the stuff below plus GUI stuff
      abortLoading();
      // Show it for now in the location bar, but we'll need to store it in the view
      // later on (can't do it yet since either view == 0 or updateHistoryEntry will be called).
      kdDebug(1202) << "setLocationBarURL : url = " << url.prettyURL() << endl;
      setLocationBarURL( url.prettyURL() );
    }
    else
    {
      view->stop();
      // Don't change location bar if not current view
    }
  }
  else // startup with argument
    setLocationBarURL( url.prettyURL() );

  kdDebug(1202) << QString("trying openView for %1 (servicetype %2)").arg(url.url()).arg(serviceType) << endl;
  if ( !serviceType.isEmpty() && serviceType != "application/octet-stream" )
  {
    // Built-in view ?
    if ( !openView( serviceType, url, view /* can be 0L */, req ) )
    {
        // We know the servicetype, let's try its preferred service
        KService::Ptr offer = KServiceTypeProfile::preferredService(serviceType, true);
        KURL::List lst;
        lst.append(url);
        //kdDebug(1202) << "Got offer " << (offer ? offer->name().latin1() : "0") << endl;
        if ( ( trustedSource || KonqRun::allowExecution( serviceType, url ) ) &&
             ( !offer || !KRun::run( *offer, lst ) ) )
        {
          (void)new KRun( url );
        }
    }
  }
  else // no known serviceType, use KonqRun
  {
      kdDebug(1202) << QString("Creating new konqrun for %1").arg(url.url()) << endl;
      KonqRun * run = new KonqRun( this, view /* can be 0L */, url, req, trustedSource );
      if ( view )
        view->setRun( run );
      if ( view == m_currentView )
        startAnimation();
      connect( run, SIGNAL( finished() ),
               this, SLOT( slotRunFinished() ) );
      connect( run, SIGNAL( error() ),
               this, SLOT( slotRunFinished() ) );
  }
}

bool KonqMainWindow::openView( QString serviceType, const KURL &_url, KonqView *childView, KonqOpenURLRequest req )
{
  // Contract: the caller of this method should ensure the view is stopped first.

  kdDebug(1202) << "KonqMainWindow::openView " << serviceType << " " << _url.url() << " " << childView << endl;
  kdDebug(1202) << "req.followMode=" << req.followMode << endl;
  kdDebug(1202) << "req.nameFilter= " << req.nameFilter << endl;
  kdDebug(1202) << "req.typedURL= " << req.typedURL << endl;

  // If linked view and if we are not already following another view
  if ( childView && childView->isLinkedView() && !req.followMode )
    makeViewsFollow( _url, serviceType, childView );

  if ( childView && childView->isLockedLocation() )
    return true;

  QString indexFile;

  KURL url( _url );

  //////////// Tar files support

  if ( url.isLocalFile() // kio_tar only supports local files
       && ( serviceType == QString::fromLatin1("application/x-tar")  ||
            serviceType == QString::fromLatin1("application/x-tgz") ) )
    {
      url.setProtocol( QString::fromLatin1("tar") );
      url.setPath( url.path() + '/' );

      serviceType = "inode/directory";

      kdDebug(1202) << "TAR FILE. Now trying with " << url.url() << endl;
    }

  ///////////

  // In case we open an index.html, we want the location bar
  // to still display the original URL (so that 'up' uses that URL,
  // and since that's what the user entered).
  // changeViewMode will take care of setting and storing that url.
  QString originalURL = url.prettyURL();
  if ( !req.nameFilter.isEmpty() ) // keep filter in location bar
    originalURL += req.nameFilter;

  QString serviceName; // default: none provided

  // Look for which view mode to use, if a directory - not if view locked, and not if following a URL
  if ( ( !childView || (!req.followMode && !childView->isLockedViewMode()) )
       && serviceType == "inode/directory" )
    { // Phew !

      // Set view mode if necessary (current view doesn't support directories)
      if ( !childView || !childView->supportsServiceType( serviceType ) )
        serviceName = m_sViewModeForDirectory;

      if ( url.isLocalFile() ) // local, we can do better (.directory)
        {
          // Read it in the .directory file, default to m_bHTMLAllowed
          KURL urlDotDir( url );
          urlDotDir.addPath(".directory");
          bool HTMLAllowed = m_bHTMLAllowed;
          QFile f( urlDotDir.path() );
          if ( f.open(IO_ReadOnly) )
            {
              f.close();
              KSimpleConfig config( urlDotDir.path(), true );
              config.setGroup( "URL properties" );
              HTMLAllowed = config.readBoolEntry( "HTMLAllowed", m_bHTMLAllowed );
              serviceName = config.readEntry( "ViewMode", m_sViewModeForDirectory );
            }
          if ( HTMLAllowed &&
               ( ( indexFile = findIndexFile( url.path() ) ) != QString::null ) )
            {
              serviceType = "text/html";
              url = KURL();
              url.setPath( indexFile );
              serviceName = QString::null; // cancel what we just set, this is not a dir finally
            }

          // Reflect this setting in the menu
          m_ptaUseHTML->setChecked( HTMLAllowed );
        }
    }

  bool ok = true;
  if ( !childView )
    {
      // Create a new view
      childView = m_pViewManager->splitView( Qt::Horizontal, serviceType, serviceName );

      if ( !childView )
        {
          KMessageBox::sorry( 0L, i18n( "Could not create view for %1\nCheck your installation").arg(serviceType) );
          return true; // fake everything was ok, we don't want to propagate the error
        }

      enableAllActions( true );

      m_pViewManager->setActivePart( childView->part() );

      childView->setViewName( m_initialFrameName );
      m_initialFrameName = QString::null;
    }
  else // We know the child view
    {
      //childView->stop();
      ok = childView->changeViewMode( serviceType, serviceName );
    }

  if (ok)
    {
      //kdDebug(1202) << "req.nameFilter= " << req.nameFilter << endl;
      //kdDebug(1202) << "req.typedURL= " << req.typedURL << endl;
      childView->setTypedURL( req.typedURL );
      childView->openURL( url, originalURL, req.nameFilter );
    }
  return ok;
}

void KonqMainWindow::slotOpenURLRequest( const KURL &url, const KParts::URLArgs &args )
{
  QString frameName = args.frameName;

  if ( !frameName.isEmpty() )
  {
    static QString _top = QString::fromLatin1( "_top" );
    static QString _self = QString::fromLatin1( "_self" );
    static QString _parent = QString::fromLatin1( "_parent" );
    static QString _blank = QString::fromLatin1( "_blank" );

    if ( frameName == _blank )
    {
      slotCreateNewWindow( url, args );
      return;
    }

    if ( frameName != _top &&
         frameName != _self &&
         frameName != _parent )
    {
      KParts::BrowserHostExtension *hostExtension = 0;
      KonqView *view = childView( frameName, &hostExtension );
      if ( !view )
      {
        KonqMainWindow *mainWindow = 0;
        view = findChildView( frameName, &mainWindow, &hostExtension );

        if ( !view || !mainWindow )
        {
          slotCreateNewWindow( url, args );
          return;
        }

        if ( hostExtension )
          hostExtension->openURLInFrame( url, args );
        else
           mainWindow->openURL( view, url, args );
        return;
      }

      if ( hostExtension )
        hostExtension->openURLInFrame( url, args );
      else
        openURL( view, url, args );
      return;
    }
  }

  KParts::ReadOnlyPart *part = static_cast<KParts::ReadOnlyPart *>( sender()->parent() );
  KonqView *view = childView( part );
  openURL( view, url, args );
}

//Called by slotOpenURLRequest
void KonqMainWindow::openURL( KonqView *childView, const KURL &url, const KParts::URLArgs &args )
{
  //TODO: handle post data!

  if ( childView->browserExtension() )
    childView->browserExtension()->setURLArgs( args );

  //  ### HACK !!
  if ( args.postData.size() > 0 )
  {
    openURL( childView, url, QString::fromLatin1( "text/html" ), KonqOpenURLRequest(), args.trustedSource );
    return;
  }

  if ( !args.reload && urlcmp( url.url(), childView->url().url(), true, true ) )
  {
    QString serviceType = args.serviceType;
    if ( serviceType.isEmpty() )
      serviceType = childView->serviceType();

    childView->stop();
    openView( serviceType, url, childView );
    return;
  }

  openURL( childView, url, args.serviceType, KonqOpenURLRequest(), args.trustedSource );
}

// Linked-views feature
void KonqMainWindow::makeViewsFollow( const KURL & url, const QString & serviceType, KonqView * senderView )
{
  kdDebug(1202) << "makeViewsFollow " << senderView->className() << " url=" << url.url() << " serviceType=" << serviceType << endl;
  KonqOpenURLRequest req;
  req.followMode = true;
  // We can't iterate over the map here, and openURL for each, because the map can get modified
  // (e.g. by part changes). Better copy the views into a list.
  QList<KonqView> listViews;
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    listViews.append( it.data() );

  for ( KonqView * view = listViews.first() ; view ; view = listViews.next() )
  {
    if ( (view != senderView) && (view)->isLinkedView() )
    {
      kdDebug(1202) << "Sending openURL to view " << view->part()->className() << " url=" << url.url() << endl;
      openURL( view, url, serviceType, req );
    }
  }
}

void KonqMainWindow::abortLoading()
{
  if ( m_currentView )
  {
    m_currentView->stop(); // will take care of the statusbar
    stopAnimation();
  }
}

void KonqMainWindow::slotCreateNewWindow( const KURL &url, const KParts::URLArgs &args )
{
  // If we want to open an HTTP url, use the web browsing profile
  if (url.protocol().left(4) == QString::fromLatin1("http"))
  {
    QString profile = locate( "data", QString::fromLatin1("konqueror/profiles/webbrowsing") );
    KonqMainWindow * mainWindow = KonqFileManager::self()->createBrowserWindowFromProfile( profile, url.url() );
    mainWindow->setInitialFrameName( args.frameName );
    //FIXME: obey args (like passing post-data (to KRun), etc.)
  }
  //KonqFileManager::self()->openFileManagerWindow( url, args.frameName );
}

void KonqMainWindow::slotNewWindow()
{
  KonqFileManager::self()->createBrowserWindowFromProfile(
      locate( "data", QString::fromLatin1("konqueror/profiles/webbrowsing") ) );
}

void KonqMainWindow::slotDuplicateWindow()
{
  KTempFile tempFile;
  tempFile.setAutoDelete( true );
  KConfig config( tempFile.name() );
  config.setGroup( "View Profile" );
  m_pViewManager->saveViewProfile( config, true );

  KonqMainWindow *mainWindow = new KonqMainWindow( QString::null, false );
  mainWindow->viewManager()->loadViewProfile( config );
  mainWindow->enableAllActions( true );
  mainWindow->show();
}

void KonqMainWindow::slotRun()
{
  // HACK: The command is not executed in the directory
  // we are in currently. Minicli does not support that yet
  QByteArray data;
  kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "popupExecuteCommand()", data );
}

void KonqMainWindow::slotOpenTerminal()
{
  KConfig *config = KGlobal::config();
  config->setGroup( "Misc Defaults" );
  QString term = config->readEntry( "Terminal", DEFAULT_TERMINAL );

  QString dir ( QDir::homeDirPath() );

  KURL u( m_currentView->url() );
  if ( u.isLocalFile() )
    dir = u.path();

  QString cmd = QString("cd \"%1\" ; %2 &").arg( dir ).arg( term );
  system( cmd.latin1() );
}

void KonqMainWindow::slotOpenLocation()
{
  QString u;
  KURL url;

  if (m_currentView)
    u = m_currentView->url().prettyURL();

  url = KURLRequesterDlg::getURL( u, this, i18n("Open Location") );
  if (!url.isEmpty())
     openFilteredURL( url.url().stripWhiteSpace() );
}

void KonqMainWindow::slotToolFind()
{
  KShellProcess proc;
  proc << "kfind";

  if ( m_currentView ) // play safe
  {
    KURL url;
    url = m_currentView->url();

    if( url.isLocalFile() )
    {
      if ( m_currentView->serviceType() == "inode/directory" )
        proc << url.path();
      else
        proc << url.directory();
    }
  }

  proc.start(KShellProcess::DontCare);
}

void KonqMainWindow::slotOpenWith()
{
  KURL::List lst;
  lst.append( m_currentView->url() );

  QString serviceName = sender()->name();

  KTrader::OfferList offers = m_currentView->appServiceOffers();
  KTrader::OfferList::ConstIterator it = offers.begin();
  KTrader::OfferList::ConstIterator end = offers.end();
  for (; it != end; ++it )
    if ( (*it)->name() == serviceName )
    {
      KRun::run( **it, lst );
      return;
    }
}

void KonqMainWindow::slotViewModeToggle( bool toggle )
{
  if ( !toggle )
    return;

  QString modeName = sender()->name();

  if ( m_currentView->service()->name() == modeName )
    return;

  m_bViewModeToggled = true;

  m_currentView->stop();
  m_currentView->lockHistory();

  // Save those, because changeViewMode will lose them
  KURL url = m_currentView->url();
  QString locationBarURL = m_currentView->locationBarURL();

  bool bQuickViewModeChange = false;

  // check if we can do a quick property-based viewmode change
  KTrader::OfferList offers = m_currentView->partServiceOffers();
  KTrader::OfferList::ConstIterator oIt = offers.begin();
  KTrader::OfferList::ConstIterator oEnd = offers.end();
  for (; oIt != oEnd; ++oIt )
  {
      KService::Ptr service = *oIt;
      if ( service->name() == modeName &&
           service->library() == m_currentView->service()->library() )
      {
        QVariant modeProp = service->property( "X-KDE-BrowserView-ModeProperty" );
        QVariant modePropValue = service->property( "X-KDE-BrowserView-ModePropertyValue" );
        if ( !modeProp.isValid() || !modePropValue.isValid() )
          break;

        m_currentView->part()->setProperty( modeProp.toString().latin1(), modePropValue );

        m_currentView->setService( service );

        bQuickViewModeChange = true;
        break;
      }
  }

  if ( !bQuickViewModeChange )
  {
    m_currentView->changeViewMode( m_currentView->serviceType(), modeName );

    QString locURL( locationBarURL );
    QString nameFilter = detectNameFilter( locURL );
    m_currentView->openURL( locURL, locationBarURL, nameFilter );
  }

  // Now save this setting, either locally or globally
  if ( m_bSaveViewPropertiesLocally )
  {
      KURL u ( m_currentView->url() );
      u.addPath(".directory");
      if ( u.isLocalFile() )
      {
          KSimpleConfig config( u.path() ); // if we have no write access, just drop it
          config.setGroup( "URL properties" );
          config.writeEntry( "ViewMode", modeName );
          config.sync();
      }
  } else
  {
      KConfig *config = KGlobal::config();
      KConfigGroupSaver cgs( config, "MainView Settings" );
      config->writeEntry( "ViewMode", modeName );
      config->sync();
      m_sViewModeForDirectory = modeName;
  }
}

void KonqMainWindow::slotShowHTML()
{
  bool b = !m_currentView->allowHTML();

  m_currentView->stop();
  m_currentView->setAllowHTML( b );

  // Save this setting, either locally or globally
  // This has to be done before calling openView since it relies on it
  if ( m_bSaveViewPropertiesLocally )
  {
      KURL u ( b ? m_currentView->url() : m_currentView->url().directory() );
      u.addPath(".directory");
      if ( u.isLocalFile() )
      {
          KSimpleConfig config( u.path() ); // No checks for access
          config.setGroup( "URL properties" );
          config.writeEntry( "HTMLAllowed", b );
          config.sync();
      }
  } else
  {
      KConfig *config = KGlobal::config();
      KConfigGroupSaver cgs( config, "MainView Settings" );
      config->writeEntry( "HTMLAllowed", b );
      config->sync();
      m_bHTMLAllowed = b;
  }

  if ( b && m_currentView->supportsServiceType( "inode/directory" ) )
  {
    m_currentView->lockHistory();
    openView( "inode/directory", m_currentView->url(), m_currentView );
  }
  else if ( !b && m_currentView->supportsServiceType( "text/html" ) )
  {
    KURL u( m_currentView->url() );
    m_currentView->lockHistory();
    openView( "inode/directory", m_currentView->url().directory(), m_currentView );
  }

}

void KonqMainWindow::slotUnlockViews()
{
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
  {
    if ( (*it)->isLockedLocation() )
    {
      (*it)->setLockedLocation( false );
      (*it)->setPassiveMode( false );
    }
  }

  m_paUnlockAll->setEnabled( false );
}

void KonqMainWindow::slotLockView()
{
  // Can't access this action in passive mode anyway
  assert(!m_currentView->isPassiveMode());
  // Those two feature are one for the user: passive mode and locked location
  // (Only the dirtree uses one and not the other)
  m_currentView->setLockedLocation( true );
  m_currentView->setPassiveMode( true ); // do this one last !
  m_paUnlockAll->setEnabled( true );
}

void KonqMainWindow::slotStop()
{
  abortLoading();
  if ( m_currentView )
  {
    m_currentView->frame()->statusbar()->message( i18n("Canceled.") );
  }
}

void KonqMainWindow::slotLinkView()
{
  // Can't access this action in passive mode anyway
  assert(!m_currentView->isPassiveMode());
  bool link = !m_currentView->isLinkedView();
  m_currentView->setLinkedView( link ); // takes care of the statusbar and the action
}

void KonqMainWindow::slotReload()
{
  m_currentView->reload();
}

void KonqMainWindow::slotHome()
{
  openURL( 0L, KURL( konqFilteredURL( this, KonqFMSettings::settings()->homeURL() ) ) );
}

void KonqMainWindow::slotGoApplications()
{
  openURL( 0L, KURL( KGlobal::dirs()->saveLocation("apps") ) );
}

void KonqMainWindow::slotGoDirTree()
{
  KonqFileManager::self()->openFileManagerWindow( locateLocal( "data", "konqueror/dirtree/" ) );
}

void KonqMainWindow::slotGoTrash()
{
  KonqFileManager::self()->openFileManagerWindow( KGlobalSettings::trashPath() );
}

void KonqMainWindow::slotGoTemplates()
{
  KonqFileManager::self()->openFileManagerWindow(
      KGlobal::dirs()->resourceDirs("templates").last() );
}

void KonqMainWindow::slotGoAutostart()
{
  KonqFileManager::self()->openFileManagerWindow( KGlobalSettings::autostartPath() );
}

void KonqMainWindow::slotConfigureFileManager()
{
  if (fork() == 0) {
    execl(QFile::encodeName(locate("exe", "kcmshell")),
          "kcmshell", "kcmkonq", 0);
    kdWarning(1202) << "Error launching kcmshell kcmkonq!" << endl;
    exit(1);
  }
}

void KonqMainWindow::slotConfigureFileTypes()
{
  if (fork() == 0) {
    execl(QFile::encodeName(locate("exe", "kcmshell")),
          "kcmshell", "filetypes", 0);
    kdWarning(1202) << "Error launching kcmshell filetypes !" << endl;
    exit(1);
  }
}

void KonqMainWindow::slotConfigureBrowser()
{
  if (fork() == 0) {
    execl(QFile::encodeName(locate("exe", "kcmshell")),
          "kcmshell", "konqhtml", 0);
    kdWarning(1202) << "Error launching kcmshell konqhtml!" << endl;
    exit(1);
  }
}

void KonqMainWindow::slotConfigureEBrowsing()
{
  if (fork() == 0) {
    execl(QFile::encodeName(locate("exe", "kcmshell")),
          "kcmshell", "ebrowsing", 0);
    kdWarning(1202) << "Error launching kcmshell ebrowsing!" << endl;
    exit(1);
  }
}

void KonqMainWindow::slotConfigureCookies()
{
  if (fork() == 0) {
    execl(QFile::encodeName(locate("exe", "kcmshell")),
          "kcmshell", "cookies", 0);
    kdWarning(1202) << "Error launching kcmshell cookies!" << endl;
    exit(1);
  }
}

void KonqMainWindow::slotConfigureProxies()
{
  if (fork() == 0) {
    execl(QFile::encodeName(locate("exe", "kcmshell")),
          "kcmshell", "proxy", 0);
    kdWarning(1202) << "Error launching kcmshell proxy!" << endl;
    exit(1);
  }
}

void KonqMainWindow::slotConfigureKeys()
{
  KKeyDialog::configureKeys(actionCollection(), xmlFile());
}

void KonqMainWindow::slotConfigureToolbars()
{
  KEditToolbar edit(factory());
  if ( edit.exec() )
  {
    if ( m_toggleViewGUIClient )
      plugActionList( QString::fromLatin1( "toggleview" ), m_toggleViewGUIClient->actions() );
    if ( m_currentView->appServiceOffers().count() > 0 )
      plugActionList( "openwith", m_openWithActions );

    updateBookmarkBar();
  }
}

void KonqMainWindow::slotUndoAvailable( bool avail )
{
  bool enable = false;

  if ( avail && m_currentView && m_currentView->part() )
  {
    QVariant prop = m_currentView->part()->property( "supportsUndo" );
    if ( prop.isValid() && prop.toBool() )
      enable = true;
  }

  m_paUndo->setEnabled( enable );
}

void KonqMainWindow::slotPartChanged( KonqView *childView, KParts::ReadOnlyPart *oldPart, KParts::ReadOnlyPart *newPart )
{
  m_mapViews.remove( oldPart );
  m_mapViews.insert( newPart, childView );

  // Remove the old part, and add the new part to the manager
  // Note: this makes the new part active... so it calls slotPartActivated
  // When it does that from here, we don't want to revert the location bar URL,
  // hence the m_bLockLocationBarURL hack.
  m_bLockLocationBarURL = true;

  m_pViewManager->replacePart( oldPart, newPart, false );
  // Set active immediately
  m_pViewManager->setActivePart( newPart, true );

  viewsChanged();
}


void KonqMainWindow::slotRunFinished()
{
  kdDebug(1202) << "KonqMainWindow::slotRunFinished()" << endl;
  const KonqRun *run = static_cast<const KonqRun *>( sender() );

  KonqView *childView = run->childView();

  // Check if we found a mimetype _and_ we got no error (example: cancel in openwith dialog)
  if ( run->foundMimeType() && !run->hasError() )
  {

    // We do this here and not in the constructor, because
    // we are waiting for the first view to be set up before doing this...

    if ( m_bNeedApplyMainWindowSettings )
    {
      m_bNeedApplyMainWindowSettings = false; // only once
      applyMainWindowSettings();
    }

    return;
  }

  if ( !childView )
  {
    if ( run->hasError() ) // Nothing to show ??
    {
      close(); // This window is useless
      KMessageBox::sorry(0L, i18n("Could not display the requested URL, closing the window"));
      // or should we go back $HOME ?
    }
    // else : no error but no mimetype either... we just drop it.
  } else
  {
    childView->setLoading( false );

    if ( childView == m_currentView )
    {
      stopAnimation();
      // Revert to working URL - unless the URL was typed manually
      if ( run->typedURL().isEmpty() ) // not typed
        childView->setLocationBarURL( childView->history().current()->locationBarURL );
    }
  }
}

void KonqMainWindow::applyMainWindowSettings()
{
  KConfig *config = KGlobal::config();
  KConfigGroupSaver cgs( config, "MainView Settings" );
  QStringList toggableViewsShown = config->readListEntry( "ToggableViewsShown" );
  QStringList::ConstIterator togIt = toggableViewsShown.begin();
  QStringList::ConstIterator togEnd = toggableViewsShown.end();
  for ( ; togIt != togEnd ; ++togIt )
  {
    // Find the action by name
  //    KAction * act = m_toggleViewGUIClient->actionCollection()->action( (*togIt).latin1() );
    KAction *act = m_toggleViewGUIClient->action( *togIt );
    if ( act )
      act->activate();
    else
      kdWarning(1202) << "Unknown toggable view in ToggableViewsShown " << *togIt << endl;
  }
}

void KonqMainWindow::slotSetStatusBarText( const QString & )
{
   // Reimplemented to disable KParts::MainWindow default behaviour
   // Does nothing here, see konq_frame.cc
}

void KonqMainWindow::slotViewCompleted( KonqView * view )
{
  assert( view );

  if ( currentView() == view )
  {
    //kdDebug(1202) << "updating toolbar actions" << endl;
    updateToolBarActions();
  }

  if (!m_combo) // happens if removed from .rc file :)
    return;

  // Register this URL as a working one, in the completion object and the combo.
  if ( !m_combo->contains( view->locationBarURL() ) )
      // goes both into the combo and the completion object
       m_combo->addToHistory( view->locationBarURL() );
  else {
      // or just into the completion object (for proper weighting)
      m_combo->completionObject()->addItem( view->locationBarURL() );

      // it's already in the combo, so we better make it the current item
      // ... _if_ the user didn't change the url while we were loading
      for ( int i = 0; i < m_combo->count(); i++ ) {
          if ( m_combo->text( i ) == view->locationBarURL() &&
               m_combo->text( i ) == m_combo->currentText() ) {
              m_combo->setCurrentItem( i );
          }
      }
  }


  QString u = view->typedURL();
  if ( !u.isEmpty() && u != view->locationBarURL() )
    m_combo->completionObject()->addItem( u ); // short version
}

void KonqMainWindow::slotPartActivated( KParts::Part *part )
{
  kdDebug(1202) << "slotPartActivated " << part << " "
                <<  ( part && part->instance() && part->instance()->aboutData() ? part->instance()->aboutData()->appName() : "" ) << endl;

  KonqView *newView = 0;
  KonqView *oldView = m_currentView;

  if ( part )
  {
    newView = m_mapViews.find( static_cast<KParts::ReadOnlyPart *>( part ) ).data();

    if ( newView->isPassiveMode() )
    {
      // Passive view. Don't connect anything, don't change m_currentView
      // Another view will become the current view very soon
      kdDebug(1202) << "Passive mode - return" << endl;
      return;
    }
  }
  /*
    // Hmm, KonqViewManager::loadViewProfile has to set the active part to 0L
    // before clearing all views. No warning, then.
  else
    if ( viewCount() > 0 ) // No warning if we're closing the window
      kdWarning(1202) << "No more active part !!!! This shouldn't happen anymore !" << endl;
      */

  KParts::BrowserExtension *ext = 0;

  if ( oldView )
  {
    ext = oldView->browserExtension();
    if ( ext )
    {
      //kdDebug() << "Disconnecting extension for view " << oldView << endl;
      disconnectExtension( ext );
    }
  }

  kdDebug(1202) << "New current view " << newView << endl;
  m_currentView = newView;
  if ( !part )
  {
    kdDebug(1202) << "No part activated - returning" << endl;
    createGUI( 0L );
    return;
  }

  ext = m_currentView->browserExtension();

  if ( ext )
  {
    //kdDebug() << "Connecting extension for view " << newView << endl;
    connectExtension( ext );
    createGUI( part );
  }
  else
  {
    kdDebug(1202) << "No Browser Extension for the new part" << endl;
    // Disable all browser-extension actions

    ActionSlotMap::ConstIterator it = s_actionSlotMap->begin();
    ActionSlotMap::ConstIterator itEnd = s_actionSlotMap->end();

    for ( ; it != itEnd ; ++it )
    {
      KAction * act = actionCollection()->action( it.key() );
      ASSERT(act);
      if (act)
        act->setEnabled( false );
    }

    if ( m_paCopyFiles )
      m_paCopyFiles->setEnabled( false );
    if ( m_paMoveFiles )
      m_paMoveFiles->setEnabled( false );

    createGUI( 0L );
  }

  // View-dependent GUI

  unplugActionList( "openwith" );
  updateOpenWithActions( m_currentView->appServiceOffers() );
  if ( m_currentView->appServiceOffers().count() > 0 )
    plugActionList( "openwith", m_openWithActions );

  if ( !m_bViewModeToggled ) // if we just toggled the view mode via the view mode actions, then
                             // we don't need to do all the time-taking stuff below (Simon)
  {
    unplugViewModeActions();
    updateViewModeActions( m_currentView->partServiceOffers() );

    KService::Ptr service = currentView()->service();
    QVariant prop = service->property( "X-KDE-BrowserView-Toggable" );
    if ( !prop.isValid() || !prop.toBool() ) // No view mode for toggable views
    // (The other way would be to enforce a better servicetype for them, than Browser/View)
      if ( m_currentView->partServiceOffers().count() > 1 && m_viewModeMenu )
        plugViewModeActions();
  }

  m_bViewModeToggled = false;

  slotUndoAvailable( KonqUndoManager::self()->undoAvailable() );

  m_currentView->frame()->statusbar()->repaint();

  if ( oldView )
    oldView->frame()->statusbar()->repaint();

  // Can lock a view only if there is a next view
  m_paLockView->setEnabled(m_pViewManager->chooseNextView(m_currentView) != 0L );

  m_paLinkView->setChecked( m_currentView->isLinkedView() );

  if ( !m_bLockLocationBarURL )
  {
    kdDebug(1202) << "slotPartActivated: setting location bar url to "
                  << m_currentView->locationBarURL() << " m_currentView=" << m_currentView << endl;
    if ( m_combo )
      m_combo->setEditText( m_currentView->locationBarURL() );
  }
  else
    m_bLockLocationBarURL = false;

  updateToolBarActions();

  // Set active instance - but take care of builtin views
  // TODO: a real mechanism for detecting builtin views (X-KDE-BrowserView-Builtin back ?)
  if ( part->instance() && part->instance()->aboutData() &&
       ( !strcmp( part->instance()->aboutData()->appName(), "konqiconview") ||
         !strcmp( part->instance()->aboutData()->appName(), "konqlistview") ) )
      KGlobal::_activeInstance = KGlobal::instance();
  else
      KGlobal::_activeInstance = part->instance();
}

void KonqMainWindow::insertChildView( KonqView *childView )
{
  m_mapViews.insert( childView->part(), childView );

  connect( childView, SIGNAL( viewCompleted( KonqView * ) ),
           this, SLOT( slotViewCompleted( KonqView * ) ) );

  childView->callExtensionBoolMethod( "setSaveViewPropertiesLocally(bool)", m_bSaveViewPropertiesLocally );
  viewCountChanged();
  emit viewAdded( childView );
}

void KonqMainWindow::removeChildView( KonqView *childView )
{
  disconnect( childView, SIGNAL( viewCompleted( KonqView * ) ),
              this, SLOT( slotViewCompleted( KonqView * ) ) );

  MapViews::Iterator it = m_mapViews.begin();
  MapViews::Iterator end = m_mapViews.end();
  // find it in the map - can't use the key since childView->part() might be 0L
  while ( it != end && it.data() != childView )
      ++it;
  if ( it == m_mapViews.end() )
  {
      kdWarning(1202) << "KonqMainWindow::removeChildView childView " << childView << " not in map !" << endl;
      return;
  }
  m_mapViews.remove( it );

  viewCountChanged();
  emit viewRemoved( childView );

  // KonqViewManager takes care of m_currentView
}

void KonqMainWindow::viewCountChanged()
{
  // This is called when the number of views changes.

  m_paRemoveView->setEnabled( activeViewsCount() > 1 );
  m_paLinkView->setEnabled( viewCount() > 1 );

  viewsChanged();

  m_pViewManager->viewCountChanged();
}

void KonqMainWindow::viewsChanged()
{
  // This is called when the number of views changes OR when
  // the type of some view changes.

  // Do we activate the operations menu ?
  if ( viewCount() == 2 )
  {
    MapViews::ConstIterator it = m_mapViews.begin();
    if ( it++.data()->serviceType() == "inode/directory"
         && it.data()->serviceType() == "inode/directory"
         && !m_paCopyFiles )
    {
      // F5 is the default key binding for Reload.... a la Windows.
      // mc users want F5 for Copy and F6 for move, but I can't make that default.
      m_paCopyFiles = new KAction( i18n("Copy files"), Key_F7, this, SLOT( slotCopyFiles() ), actionCollection(), "copyfiles" );
      m_paMoveFiles = new KAction( i18n("Move files"), Key_F8, this, SLOT( slotMoveFiles() ), actionCollection(), "movefiles" );
      QList<KAction> lst;
      lst.append( m_paCopyFiles );
      lst.append( m_paMoveFiles );
      m_paCopyFiles->setEnabled( false );
      m_paMoveFiles->setEnabled( false );
      plugActionList( "operations", lst );
    }
  }
  else if (m_paCopyFiles)
  {
    unplugActionList( "operations" );
    delete m_paCopyFiles;
    m_paCopyFiles = 0L;
    delete m_paMoveFiles;
    m_paMoveFiles = 0L;
  }
}

KonqView * KonqMainWindow::childView( KParts::ReadOnlyPart *view )
{
  MapViews::ConstIterator it = m_mapViews.find( view );
  if ( it != m_mapViews.end() )
    return it.data();
  else
    return 0L;
}

KonqView * KonqMainWindow::childView( const QString &name, KParts::BrowserHostExtension **hostExtension )
{
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
  {
    QString viewName = it.data()->viewName();
    if ( !viewName.isEmpty() && viewName == name )
    {
      if ( hostExtension )
        *hostExtension = 0;
      return it.data();
    }

    if ( it.data()->frameNames().contains( name ) )
    {
      if ( hostExtension )
        *hostExtension = KonqView::hostExtension( it.data()->part(), name );
      return it.data();
    }
  }

  return 0;
}

KonqView * KonqMainWindow::findChildView( const QString &name, KonqMainWindow **mainWindow, KParts::BrowserHostExtension **hostExtension )
{
  if ( !s_lstViews )
    return 0;

  QListIterator<KonqMainWindow> it( *s_lstViews );
  for (; it.current(); ++it )
  {
    KonqView *res = it.current()->childView( name, hostExtension );
    if ( res )
    {
      if ( mainWindow )
        *mainWindow = it.current();
      return res;
    }
  }

  return 0;
}

int KonqMainWindow::activeViewsCount() const
{
  int res = 0;
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    if ( !it.data()->isPassiveMode() )
      ++res;

  return res;
}

KParts::ReadOnlyPart * KonqMainWindow::currentPart()
{
  /// ### This is currently unused. Check in the final version (!) if still unused.
  if ( m_currentView )
    return m_currentView->part();
  else
    return 0L;
}

void KonqMainWindow::customEvent( QCustomEvent *event )
{
  KParts::MainWindow::customEvent( event );

  if ( KonqFileSelectionEvent::test( event ) )
  {
    // Forward the event to all views
    MapViews::ConstIterator it = m_mapViews.begin();
    MapViews::ConstIterator end = m_mapViews.end();
    for (; it != end; ++it )
      QApplication::sendEvent( (*it)->part(), event );
    return;
  }
  if ( KParts::OpenURLEvent::test( event ) )
  {
    KParts::OpenURLEvent * ev = static_cast<KParts::OpenURLEvent*>(event);
    KonqView * senderChildView = childView(ev->part());

    // Enable/disable local properties actions if current view
    if ( senderChildView == m_currentView )
    {
      bool canWrite = false;
      if ( ev->url().isLocalFile() )
      {
        // Can we write ?
        QFileInfo info( ev->url().path() );
        canWrite = info.isWritable();
      }
      m_paSaveViewPropertiesLocally->setEnabled( canWrite );
      m_paRemoveLocalProperties->setEnabled( canWrite );
    }

    // Forward the event to all views
    MapViews::ConstIterator it = m_mapViews.begin();
    MapViews::ConstIterator end = m_mapViews.end();
    for (; it != end; ++it )
    {
      // Don't resend to sender
      if (it.key() != ev->part())
      {
        //kdDebug(1202) << "Sending event to view " << it.key()->className() << endl;
        QApplication::sendEvent( it.key(), event );

      }
    }
  }
}

void KonqMainWindow::slotURLEntered( const QString &text )
{
  if ( m_bURLEnterLock )
    return;

  m_bURLEnterLock = true;

  openFilteredURL( text.stripWhiteSpace() );

  m_bURLEnterLock = false;
}

void KonqMainWindow::slotFileNewAboutToShow()
{
  // As requested by KNewMenu :
  m_pMenuNew->slotCheckUpToDate();
  // And set the files that the menu apply on :
  m_pMenuNew->setPopupFiles( m_currentView->url().url() );
}

void KonqMainWindow::slotSplitViewHorizontal()
{
  KonqView * newView = m_pViewManager->splitView( Qt::Horizontal );
  newView->openURL( m_currentView->url(), m_currentView->locationBarURL() );
  m_paLockView->setEnabled( true ); // in case we had only one view previously
}

void KonqMainWindow::slotSplitViewVertical()
{
  KonqView * newView = m_pViewManager->splitView( Qt::Vertical );
  newView->openURL( m_currentView->url(), m_currentView->locationBarURL() );
  m_paLockView->setEnabled( true ); // in case we had only one view previously
}

void KonqMainWindow::slotSplitWindowHorizontal()
{
  m_pViewManager->splitWindow( Qt::Horizontal );
  m_paLockView->setEnabled( true ); // in case we had only one view previously
}

void KonqMainWindow::slotSplitWindowVertical()
{
  m_pViewManager->splitWindow( Qt::Vertical );
  m_paLockView->setEnabled( true ); // in case we had only one view previously
}

void KonqMainWindow::slotRemoveView()
{
  // takes care of choosing the new active view
  m_pViewManager->removeView( m_currentView );

  // Can lock a view only if there is a next view
  m_paLockView->setEnabled(m_pViewManager->chooseNextView(m_currentView) != 0L );
}

void KonqMainWindow::slotSaveViewPropertiesLocally()
{
  m_bSaveViewPropertiesLocally = !m_bSaveViewPropertiesLocally;
  // And this is a main-view setting, so save it
  KConfig *config = KGlobal::config();
  KConfigGroupSaver cgs( config, "MainView Settings" );
  config->writeEntry( "SaveViewPropertiesLocally", m_bSaveViewPropertiesLocally );
  config->sync();
  // Now tell the views
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    (*it)->callExtensionBoolMethod( "setSaveViewPropertiesLocally(bool)", m_bSaveViewPropertiesLocally );
}

void KonqMainWindow::slotRemoveLocalProperties()
{
  assert( m_currentView );
  KURL u ( m_currentView->url() );
  u.addPath(".directory");
  if ( u.isLocalFile() )
  {
      QFile f( u.path() );
      if ( f.open(IO_ReadWrite) )
      {
          f.close();
          KSimpleConfig config( u.path() );
          config.deleteGroup( "URL properties" ); // Bye bye
          config.sync();
          // TODO: Notify the view...
          // Or the hard way: (and hoping it doesn't cache the values!)
          slotReload();
      } else
      {
         ASSERT( QFile::exists(u.path()) ); // The action shouldn't be enabled, otherwise.
         KMessageBox::sorry( this, i18n("No permissions to write to %1").arg(u.path()) );
      }
  }
}

void KonqMainWindow::slotCopyFiles()
{
  kdDebug(1202) << "KonqMainWindow::slotCopyFiles()" << endl;
  assert( viewCount() == 2 );
  // Copy files is copy + paste. I'm sooo lazy :)
  m_currentView->callExtensionMethod( "copy()" );
  otherView( m_currentView )->callExtensionMethod( "paste()" );
}

void KonqMainWindow::slotMoveFiles()
{
  kdDebug(1202) << "KonqMainWindow::slotMoveFiles()" << endl;
  assert( viewCount() == 2 );
  // Copy files is cut + paste. I'm sooo lazy :)
  m_currentView->callExtensionMethod( "cut()" );
  otherView( m_currentView )->callExtensionMethod( "paste()" );
}

// Only valid if there are one or two views
KonqView * KonqMainWindow::otherView( KonqView * view ) const
{
  assert( viewCount() <= 2 );
  MapViews::ConstIterator it = m_mapViews.begin();
  if ( (*it) == view )
    ++it;
  if ( it != m_mapViews.end() )
    return (*it);
  return 0L;
}

/*
void KonqMainWindow::slotSaveDefaultProfile()
{
  KConfig *config = KGlobal::config();
  config->setGroup( "Default View Profile" );
  m_pViewManager->saveViewProfile( *config );
}
*/

/*
  Now uses KAboutData and the standard about box
void KonqMainWindow::slotAbout()
{
  KMessageBox::about( 0, i18n(
"Konqueror Version %1\n"
"Author: Torben Weis <weis@kde.org>\n"
"Current maintainer: David Faure <faure@kde.org>\n\n"
"Current team:\n"
"  David Faure <faure@kde.org>\n"
"  Simon Hausmann <hausmann@kde.org>\n"
"  Michael Reiher <michael.reiher@gmx.de>\n"
"  Matthias Welk <welk@fokus.gmd.de>\n"
"HTML rendering engine:\n"
"  Lars Knoll <knoll@kde.org>\n"
"  Antti Koivisto <koivisto@kde.org>\n"
"  Waldo Bastian <bastian@kde.org>\n"
"KIO library/slaves:\n"
"  Matt Koss <koss@miesto.sk>\n"
"  Alex Zepeda <garbanzo@hooked.net>\n"
"Java applet support:\n"
"  Richard Moore <rich@kde.org>\n"
"  Dina Rogozin <dima@mercury.co.il>\n"
  ).arg(KONQUEROR_VERSION));
}
*/

void KonqMainWindow::slotUpAboutToShow()
{
  QPopupMenu *popup = m_paUp->popupMenu();

  popup->clear();

  uint i = 0;

  // Use the location bar URL, because in case we display a index.html
  // we want to go up from the dir, not from the index.html
  KURL u( m_currentView->locationBarURL() );
  u = u.upURL();
  while ( u.hasPath() )
  {
    popup->insertItem( KMimeType::pixmapForURL( u ), u.prettyURL() );

    if ( u.path() == "/" )
      break;

    if ( ++i > 10 )
      break;

    u = u.upURL();
  }
}

void KonqMainWindow::slotUp()
{
  kdDebug(1202) << "slotUp. Start URL is " << m_currentView->locationBarURL() << endl;
  openURL( 0L, KURL(m_currentView->locationBarURL()).upURL() );
}

void KonqMainWindow::slotUpActivated( int id )
{
  KURL u( m_currentView->locationBarURL() );
  kdDebug(1202) << "slotUpActivated. Start URL is " << u.url() << endl;
  for ( int i = 0 ; i < m_paUp->popupMenu()->indexOf( id ) + 1 ; i ++ )
      u = u.upURL();
  openURL( 0L, u );
}

void KonqMainWindow::slotGoMenuAboutToShow()
{
  kdDebug(1202) << "KonqMainWindow::slotGoMenuAboutToShow" << endl;
  if ( m_paHistory && m_currentView ) // (maybe this is before initialisation)
      m_paHistory->fillGoMenu( m_currentView->history() );
}

void KonqMainWindow::slotGoHistoryActivated( int steps )
{
  m_currentView->go( steps );
}

void KonqMainWindow::slotBackAboutToShow()
{
  m_paBack->popupMenu()->clear();
  if ( m_currentView )
      KonqHistoryAction::fillHistoryPopup( m_currentView->history(), m_paBack->popupMenu(), true, false );
}

void KonqMainWindow::slotBack()
{
  m_currentView->go( -1 );
}

void KonqMainWindow::slotBackActivated( int id )
{
  m_currentView->go( -(m_paBack->popupMenu()->indexOf( id ) + 1) );
}

void KonqMainWindow::slotForwardAboutToShow()
{
  m_paForward->popupMenu()->clear();
  if ( m_currentView )
      KonqHistoryAction::fillHistoryPopup( m_currentView->history(), m_paForward->popupMenu(), false, true );
}

void KonqMainWindow::slotForward()
{
  m_currentView->go( 1 );
}

void KonqMainWindow::slotForwardActivated( int id )
{
  m_currentView->go( m_paForward->popupMenu()->indexOf( id ) + 1 );
}

void KonqMainWindow::slotComboPlugged()
{
  m_combo = m_paURLCombo->combo();

  m_combo->clearHistory();
  KConfig *config = KGlobal::config();
  config->setGroup( "Settings" );
  KonqPixmapProvider *prov = static_cast<KonqPixmapProvider*> (m_combo->pixmapProvider());
  prov->load( config, "ComboIconCache" );
  m_combo->setMaxCount( config->readNumEntry("Maximum of URLs in combo", 10 ));
  QStringList locationBarCombo = config->readListEntry( "ToolBarCombo" );
  int mode = config->readNumEntry("CompletionMode", KGlobalSettings::completionMode());
  m_combo->setCompletionMode( (KGlobalSettings::Completion) mode ); // set the previous completion-mode
  m_combo->completionObject()->setItems( config->readListEntry( "CompletionItems" ) );
  m_combo->setHistoryItems( locationBarCombo );

  m_combo->lineEdit()->installEventFilter(this);
}

bool KonqMainWindow::eventFilter(QObject*obj,QEvent *ev)
{
  if (m_currentView &&
      ( ev->type()==QEvent::FocusIn || ev->type()==QEvent::FocusOut ))
  {
    ASSERT( obj == m_combo->lineEdit() );

    QFocusEvent * focusEv = static_cast<QFocusEvent*>(ev);
    if (focusEv->reason() == QFocusEvent::Popup )
    {
      //kdDebug(1202) << "Reason for focus change was popup. gotFocus=" << focusEv->gotFocus() << endl;
      return KParts::MainWindow::eventFilter( obj, ev );
    }

    KParts::BrowserExtension * ext = m_currentView->browserExtension();
    QStrList slotNames;
    if (ext)
      ext->metaObject()->slotNames();
    if (ev->type()==QEvent::FocusIn)
    {
      //kdDebug(1202) << "ComboBox got the focus..." << endl;
      if (m_bLocationBarConnected)
      {
        //kdDebug(1202) << "Was already connected..." << endl;
        return KParts::MainWindow::eventFilter( obj, ev );
      }
      m_bLocationBarConnected = true;
      if (slotNames.contains("cut()"))
        disconnect( m_paCut, SIGNAL( activated() ), ext, SLOT( cut() ) );
      if (slotNames.contains("copy()"))
        disconnect( m_paCopy, SIGNAL( activated() ), ext, SLOT( copy() ) );
      if (slotNames.contains("paste()"))
        disconnect( m_paPaste, SIGNAL( activated() ), ext, SLOT( paste() ) );

      connect( m_paCut, SIGNAL( activated() ), this, SLOT( slotComboCut() ) );
      connect( m_paCopy, SIGNAL( activated() ), this, SLOT( slotComboCopy() ) );
      connect( m_paPaste, SIGNAL( activated() ), this, SLOT( slotComboPaste() ) );
      connect( QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()) );
      connect( m_combo->lineEdit(), SIGNAL(textChanged(const QString &)), this, SLOT(slotClipboardDataChanged()) );

      m_bCutWasEnabled = m_paCut->isEnabled();
      m_bCopyWasEnabled = m_paCopy->isEnabled();
      m_bPasteWasEnabled = m_paPaste->isEnabled();

      slotClipboardDataChanged();

    } else if ( ev->type()==QEvent::FocusOut)
    {
      //kdDebug(1202) << "ComboBox lost focus..." << endl;
      if (!m_bLocationBarConnected)
      {
        //kdDebug(1202) << "Was already disconnected..." << endl;
        return KParts::MainWindow::eventFilter( obj, ev );
      }
      m_bLocationBarConnected = false;
      if (slotNames.contains("cut()"))
        connect( m_paCut, SIGNAL( activated() ), ext, SLOT( cut() ) );
      if (slotNames.contains("copy()"))
        connect( m_paCopy, SIGNAL( activated() ), ext, SLOT( copy() ) );
      if (slotNames.contains("paste()"))
        connect( m_paPaste, SIGNAL( activated() ), ext, SLOT( paste() ) );

      disconnect( m_paCut, SIGNAL( activated() ), this, SLOT( slotComboCut() ) );
      disconnect( m_paCopy, SIGNAL( activated() ), this, SLOT( slotComboCopy() ) );
      disconnect( m_paPaste, SIGNAL( activated() ), this, SLOT( slotComboPaste() ) );
      disconnect( QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()) );
      disconnect( m_combo->lineEdit(), SIGNAL(textChanged(const QString &)), this, SLOT(slotClipboardDataChanged()) );

      m_paCut->setEnabled( m_bCutWasEnabled );
      m_paCopy->setEnabled( m_bCopyWasEnabled );
      m_paPaste->setEnabled( m_bPasteWasEnabled );

    }
  }
  return KParts::MainWindow::eventFilter( obj, ev );
}

void KonqMainWindow::slotClipboardDataChanged()
{
  //kdDebug(1202) << "KonqMainWindow::slotClipboardDataChanged()" << endl;
  QMimeSource *data = QApplication::clipboard()->data();
  m_paPaste->setEnabled( data->provides( "text/plain" ) );
  //kdDebug(1202) << "m_combo->lineEdit()->hasMarkedText() : " << m_combo->lineEdit()->hasMarkedText() << endl;
  m_paCopy->setEnabled( m_combo->lineEdit()->hasMarkedText() );
  m_paCut->setEnabled( m_combo->lineEdit()->hasMarkedText() );
}

void KonqMainWindow::slotComboCut()
{
  // Don't ask me why this isn't a slot...
  m_combo->lineEdit()->cut();
}

void KonqMainWindow::slotComboCopy()
{
  m_combo->lineEdit()->copy();
}

void KonqMainWindow::slotComboPaste()
{
  m_combo->lineEdit()->paste();
}

void KonqMainWindow::slotClearLocationBar()
{
  kdDebug(1202) << "slotClearLocationBar" << endl;
  m_combo->clearEdit();
  m_combo->setFocus();
}

void KonqMainWindow::slotShowMenuBar()
{
  if (menuBar()->isVisible())
    menuBar()->hide();
  else
    menuBar()->show();
}

/*
void KonqMainWindow::slotShowStatusBar()
{
  if (statusBar()->isVisible())
    statusBar()->hide();
  else
    statusBar()->show();
}
*/

void KonqMainWindow::slotShowToolBar()
{
  toggleBar( "mainToolBar", "KToolBar" );
}

void KonqMainWindow::slotShowLocationBar()
{
  toggleBar( "locationToolBar", "KToolBar" );
}

void KonqMainWindow::slotShowBookmarkBar()
{
  toggleBar( "bookmarkToolBar", "KToolBar" );
}

void KonqMainWindow::toggleBar( const char *name, const char *className )
{
  KToolBar *bar = static_cast<KToolBar *>( child( name, className ) );
  if ( !bar )
    return;
  if ( bar->isVisible() )
    bar->hide();
  else
    bar->show();
}

void KonqMainWindow::slotToggleFullScreen( bool toggle )
{
  if ( toggle )
  {
    m_oldCaption = QWidget::caption();
    kdDebug() << "old caption is " << m_oldCaption << endl;

    // Create toolbar button for exiting from full-screen mode
    QList<KAction> lst;
    lst.append( m_ptaFullScreen );
    plugActionList( "fullscreen", lst );

    QListIterator<KToolBar> barIt = toolBarIterator();
    for (; barIt.current(); ++barIt )
        barIt.current()->setEnableContextMenu( false );

    menuBar()->hide();

    showFullScreen();

    m_ptaFullScreen->setText( i18n( "Stop Fullscreen Mode" ) );
    m_ptaFullScreen->setIcon( "window_nofullscreen" );
  }
  else
  {
    unplugActionList( "fullscreen" );

    QListIterator<KToolBar> barIt = toolBarIterator();
    for (; barIt.current(); ++barIt )
        barIt.current()->setEnableContextMenu( true );

    menuBar()->show();

    showNormal();

    kdDebug() << "caption is " << m_oldCaption << endl;

    QWidget::setCaption( m_oldCaption );

    m_ptaFullScreen->setText( i18n( "Fullscreen Mode" ) );
    m_ptaFullScreen->setIcon( "window_fullscreen" );
  }
}

void KonqMainWindow::setLocationBarURL( const QString &url )
{
  kdDebug(1202) << "KonqMainWindow::setLocationBarURL : url = " << url << endl;
  ASSERT( !url.isEmpty());
  // FIXME, change the current pixmap of the combo, using
  // QComboBox::setCurrentPixmap() (in Qt 2.2 as Reggie promised :) (pfeiffer)
  if ( m_combo )
    m_combo->setEditText( url );
}

void KonqMainWindow::startAnimation()
{
  m_paAnimatedLogo->start();
  m_paStop->setEnabled( true );
}

void KonqMainWindow::stopAnimation()
{
  m_paAnimatedLogo->stop();
  m_paStop->setEnabled( false );
}

void KonqMainWindow::setUpEnabled( const KURL &url )
{
  //kdDebug(1202) << "KonqMainWindow::setUpEnabled(" << url.url() << ")" << endl;
  bool bHasUpURL = false;

  if ( url.hasPath() )
    bHasUpURL = ( url.path() != "/" );
  if ( !bHasUpURL )
    bHasUpURL = url.hasSubURL();

  m_paUp->setEnabled( bHasUpURL );
}

void KonqMainWindow::initActions()
{
  actionCollection()->setHighlightingEnabled( true );
  connect( actionCollection(), SIGNAL( actionStatusText( const QString &) ),
           this, SLOT( slotActionStatusText( const QString & ) ) );
  connect( actionCollection(), SIGNAL( clearStatusText() ),
           this, SLOT( slotClearStatusText() ) );


  // Note about this method : don't call setEnabled() on any of the actions.
  // They are all disabled then re-enabled with enableAllActions
  // If any one needs to be initially disabled, put that code in enableAllActions

  // File menu
  m_pMenuNew = new KNewMenu ( actionCollection(), "new_menu" );
  QObject::connect( m_pMenuNew->popupMenu(), SIGNAL(aboutToShow()),
                    this, SLOT(slotFileNewAboutToShow()) );

  m_paFileType = new KAction( i18n( "Edit File Type..." ), 0, actionCollection(), "editMimeType" );
  m_paProperties = new KAction( i18n( "Properties..." ), 0, actionCollection(), "properties" );
  (void) new KAction( i18n( "New &Window" ), "window_new", KStdAccel::key(KStdAccel::New), this, SLOT( slotNewWindow() ), actionCollection(), "new_window" );
  (void) new KAction( i18n( "&Duplicate Window" ), "window_new", 0 /*conflict with New Window! KStdAccel::key(KStdAccel::New)*/,
                      this, SLOT( slotDuplicateWindow() ), actionCollection(), "duplicate_window" );

  (void) new KAction( i18n( "&Run Command..." ), "run", 0/*kdesktop has a binding for it*/, this, SLOT( slotRun() ), actionCollection(), "run" );
  (void) new KAction( i18n( "Open &Terminal..." ), "openterm", CTRL+Key_T, this, SLOT( slotOpenTerminal() ), actionCollection(), "open_terminal" );
  (void) new KAction( i18n( "&Open Location..." ), "fileopen", KStdAccel::key(KStdAccel::Open), this, SLOT( slotOpenLocation() ), actionCollection(), "open_location" );
  (void) new KAction( i18n( "&Find file..." ), "find", 0 /*not KStdAccel::find!*/, this, SLOT( slotToolFind() ), actionCollection(), "findfile" );

  m_paPrint = KStdAction::print( 0, 0, actionCollection(), "print" );
  m_paShellClose = KStdAction::close( this, SLOT( close() ), actionCollection(), "close" );

  m_ptaUseHTML = new KToggleAction( i18n( "&Use index.html" ), 0, this, SLOT( slotShowHTML() ), actionCollection(), "usehtml" );
  m_paLockView = new KAction( i18n( "Lock to current location"), 0, this, SLOT( slotLockView() ), actionCollection(), "lock" );
  m_paUnlockAll = new KAction( i18n( "Unlock all views"), 0, this, SLOT( slotUnlockViews() ), actionCollection(), "unlockall" );
  m_paLinkView = new KToggleAction( i18n( "Link view"), 0, this, SLOT( slotLinkView() ), actionCollection(), "link" );

  // Go menu
  m_paUp = new KonqHistoryAction( i18n( "&Up" ), "up", CTRL+Key_Up, actionCollection(), "up" );

  connect( m_paUp, SIGNAL( activated() ), this, SLOT( slotUp() ) );
  connect( m_paUp->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotUpAboutToShow() ) );
  connect( m_paUp->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotUpActivated( int ) ) );

  m_paBack = new KonqHistoryAction( i18n( "&Back" ), "back", CTRL+Key_Left, actionCollection(), "back" );


  connect( m_paBack, SIGNAL( activated() ), this, SLOT( slotBack() ) );
  // toolbar button
  connect( m_paBack->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotBackAboutToShow() ) );
  connect( m_paBack->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotBackActivated( int ) ) );

  m_paForward = new KonqHistoryAction( i18n( "&Forward" ), "forward", CTRL+Key_Right, actionCollection(), "forward" );


  connect( m_paForward, SIGNAL( activated() ), this, SLOT( slotForward() ) );
  connect( m_paForward->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotForwardAboutToShow() ) );
  connect( m_paForward->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotForwardActivated( int ) ) );

  m_paHistory = new KonqBidiHistoryAction( actionCollection(), "history" );
  connect( m_paHistory, SIGNAL( menuAboutToShow() ), this, SLOT( slotGoMenuAboutToShow() ) );
  connect( m_paHistory, SIGNAL( activated( int ) ), this, SLOT( slotGoHistoryActivated( int ) ) );

  m_paHome = new KAction( i18n( "Home URL" ), "gohome", KStdAccel::key(KStdAccel::Home), this, SLOT( slotHome() ), actionCollection(), "home" );

  (void) new KAction( i18n( "App&lications" ), 0, this, SLOT( slotGoApplications() ), actionCollection(), "go_applications" );
  (void) new KAction( i18n( "Directory Tree" ), 0, this, SLOT( slotGoDirTree() ), actionCollection(), "go_dirtree" );
  (void) new KAction( i18n( "Trash" ), 0, this, SLOT( slotGoTrash() ), actionCollection(), "go_trash" );
  (void) new KAction( i18n( "Templates" ), 0, this, SLOT( slotGoTemplates() ), actionCollection(), "go_templates" );
  (void) new KAction( i18n( "Autostart" ), 0, this, SLOT( slotGoAutostart() ), actionCollection(), "go_autostart" );

  // Settings menu
  m_paSaveViewPropertiesLocally = new KToggleAction( i18n( "Sa&ve View Properties In Directory" ), 0, this, SLOT( slotSaveViewPropertiesLocally() ), actionCollection(), "saveViewPropertiesLocally" );
   // "Remove" ? "Reset" ? The former is more correct, the latter is more kcontrol-like...
  m_paRemoveLocalProperties = new KAction( i18n( "Remove Directory Properties" ), 0, this, SLOT( slotRemoveLocalProperties() ), actionCollection(), "removeLocalProperties" );

  // Configure submenu

  new KAction( i18n( "File &Manager..." ), 0, this, SLOT( slotConfigureFileManager() ), actionCollection(), "configurefilemanager" );
  new KAction( i18n( "File &Associations..." ), 0, this, SLOT( slotConfigureFileTypes() ), actionCollection(), "configurefiletypes" );

  new KAction( i18n( "&Browser..." ), 0, this, SLOT( slotConfigureBrowser() ), actionCollection(), "configurebrowser" );
  new KAction( i18n( "&Internet Keywords..." ), 0, this, SLOT( slotConfigureEBrowsing() ), actionCollection(), "configureebrowsing" );
  new KAction( i18n( "&Cookies..." ), 0, this, SLOT( slotConfigureCookies() ), actionCollection(), "configurecookies" );
  new KAction( i18n( "&Proxies..." ), 0, this, SLOT( slotConfigureProxies() ), actionCollection(), "configureproxies" );

  new KAction( i18n( "&Key Bindings..." ), 0, this, SLOT( slotConfigureKeys() ), actionCollection(), "configurekeys" );
  new KAction( i18n( "&Toolbars..." ), 0, this, SLOT( slotConfigureToolbars() ), actionCollection(), "configuretoolbars" );

  // Window menu
  m_paSplitViewHor = new KAction( i18n( "Split View &Left/Right" ), CTRL+SHIFT+Key_L, this, SLOT( slotSplitViewHorizontal() ), actionCollection(), "splitviewh" );
  m_paSplitViewVer = new KAction( i18n( "Split View &Top/Bottom" ), CTRL+SHIFT+Key_T, this, SLOT( slotSplitViewVertical() ), actionCollection(), "splitviewv" );
  m_paSplitWindowHor = new KAction( i18n( "New View On Right" ), 0, this, SLOT( slotSplitWindowHorizontal() ), actionCollection(), "splitwindowh" );
  m_paSplitWindowVer = new KAction( i18n( "New View At Bottom" ), 0, this, SLOT( slotSplitWindowVertical() ), actionCollection(), "splitwindowv" );
  m_paRemoveView = new KAction( i18n( "&Remove Active View" ), CTRL+SHIFT+Key_R, this, SLOT( slotRemoveView() ), actionCollection(), "removeview" );

  // makes no sense since konqueror isn't launched directly (but through kfmclient)
  // m_paSaveDefaultProfile = new KAction( i18n( "Save Current Profile As Default" ), 0, this, SLOT( slotSaveDefaultProfile() ), actionCollection(), "savedefaultprofile" );

  m_paSaveRemoveViewProfile = new KAction( i18n( "Save/Remove View Profile" ), 0, m_pViewManager, SLOT( slotProfileDlg() ), actionCollection(), "saveremoveviewprofile" );
  m_pamLoadViewProfile = new KActionMenu( i18n( "Load View Profile" ), actionCollection(), "loadviewprofile" );

  m_pViewManager->setProfiles( m_pamLoadViewProfile );

  m_ptaFullScreen = new KToggleAction( i18n( "Fullscreen Mode" ), "window_fullscreen", CTRL+SHIFT+Key_F, actionCollection(), "fullscreen" );

  m_ptaFullScreen->setChecked( false );

  connect( m_ptaFullScreen, SIGNAL( toggled( bool ) ),
           this, SLOT( slotToggleFullScreen( bool ) ) );

  KHelpMenu * m_helpMenu = new KHelpMenu( this, KonqFactory::aboutData() );
  KStdAction::helpContents( m_helpMenu, SLOT( appHelpActivated() ), actionCollection(), "contents" );
  KStdAction::whatsThis( m_helpMenu, SLOT( contextHelpActivated() ), actionCollection(), "whats_this" );
  KStdAction::aboutApp( m_helpMenu, SLOT( aboutApplication() ), actionCollection(), "about_app" );
  // old one KStdAction::aboutApp( this, SLOT( slotAbout() ), actionCollection(), "about_app" );
  KStdAction::aboutKDE( m_helpMenu, SLOT( aboutKDE() ), actionCollection(), "about_kde" );
  KStdAction::reportBug( m_helpMenu, SLOT( reportBug() ), actionCollection(), "report_bug" );

  m_paReload = new KAction( i18n( "&Reload" ), "reload", KStdAccel::key(KStdAccel::Reload), this, SLOT( slotReload() ), actionCollection(), "reload" );

  m_paUndo = KStdAction::undo( KonqUndoManager::self(), SLOT( undo() ), actionCollection(), "undo" );
  m_paUndo->setEnabled( KonqUndoManager::self()->undoAvailable() );
  connect( KonqUndoManager::self(), SIGNAL( undoTextChanged( const QString & ) ),
           m_paUndo, SLOT( setText( const QString & ) ) );

  // Those are connected to the browserextension directly
  m_paCut = KStdAction::cut( 0, 0, actionCollection(), "cut" );
  m_paCopy = KStdAction::copy( 0, 0, actionCollection(), "copy" );
  m_paPaste = KStdAction::paste( 0, 0, actionCollection(), "paste" );
  m_paStop = new KAction( i18n( "&Stop" ), "stop", Key_Escape, this, SLOT( slotStop() ), actionCollection(), "stop" );

  // Which is the default
  // Key_Delete conflicts with the location bar
  m_paTrash = new KAction( i18n( "&Move to Trash" ), "trash", CTRL+Key_Delete, actionCollection(), "trash" );
  m_paDelete = new KAction( i18n( "&Delete" ), CTRL+SHIFT+Key_Delete, actionCollection(), "del" );
  m_paShred = new KAction( i18n( "&Shred" ), 0, actionCollection(), "shred" );

  m_paAnimatedLogo = new KonqLogoAction( *s_plstAnimatedLogo, this, SLOT( slotNewWindow() ), actionCollection(), "animated_logo" );

  // Location bar
  (void)new KonqLabelAction( i18n( "Location " ), actionCollection(), "location_label" );

  m_paURLCombo = new KonqComboAction( i18n( "Location " ), 0, this, SLOT( slotURLEntered( const QString & ) ), actionCollection(), "toolbar_url_combo" );
  connect( m_paURLCombo, SIGNAL( plugged() ),
           this, SLOT( slotComboPlugged() ) );

  // Tackat asked me to force 16x16... (Werner)
  (void)new KAction( i18n( "Clear location bar" ), BarIcon("eraser", 16),
                     0, this, SLOT( slotClearLocationBar() ), actionCollection(), "clear_location" );

  // Bookmarks menu
  m_pamBookmarks = new KActionMenu( i18n( "&Bookmarks" ), actionCollection(), "bookmarks" );
  m_pBookmarkMenu = new KBookmarkMenu( this, m_pamBookmarks->popupMenu(), actionCollection(), true );

  m_paShowMenuBar = KStdAction::showMenubar( this, SLOT( slotShowMenuBar() ), actionCollection(), "showmenubar" );
  m_paShowToolBar = KStdAction::showToolbar( this, SLOT( slotShowToolBar() ), actionCollection(), "showtoolbar" );
  m_paShowLocationBar = new KToggleAction( i18n( "Show &Location Toolbar" ), 0, actionCollection(), "showlocationbar" );
  m_paShowBookmarkBar = new KToggleAction( i18n( "Show &Bookmark Toolbar" ), 0, actionCollection(), "showbookmarkbar" );

  m_paShowLocationBar->setChecked( true );
  m_paShowBookmarkBar->setChecked( true );

  connect( m_paShowLocationBar, SIGNAL( activated() ), this, SLOT( slotShowLocationBar() ) );
  connect( m_paShowBookmarkBar, SIGNAL( activated() ), this, SLOT( slotShowBookmarkBar() ) );

  enableAllActions( false );

  // help stuff

  m_paBack->setWhatsThis( i18n( "Click this button to display the previous document<br><br>\n\n"
                                "You can also select the <b>Back Command</b> from the Go menu." ) );
  m_paBack->setStatusText( i18n( "Display the previous document" ) );

  m_paForward->setWhatsThis( i18n( "Click this button to display the next document<br><br>\n\n"
                                   "You can also select the <b>Forward Command</b> from the Go Menu." ) );

  m_paHome->setWhatsThis( i18n( "Click this button to display your 'Home URL'<br><br>\n\n"
                                "You can configure the location this button brings you to in the"
                                "<b>File Manager Configuration</b> in the <b>KDE Control Center</b>" ) );
  m_paHome->setStatusText( i18n( "Enter your home directory" ) );

  m_paReload->setWhatsThis( i18n( "Reloads the currently displayed document<br><br>\n\n"
                                  "You can also select the <b>Reload Command</b> from the View menu." ) );
  m_paReload->setStatusText( i18n( "Reload the current document" ) );

  m_paCut->setWhatsThis( i18n( "Click this button to cut the currently selected text or items and move it "
                               "to the system clipboard<br><br>\n\n"
                               "You can also select the <b>Cut Command</b> from the Edit menu." ) );
  m_paCut->setStatusText( i18n( "Moves the selected text/item(s) to the clipboard" ) );

  m_paCopy->setWhatsThis( i18n( "Click this button to copy the currently selected text or items to the "
                                "system clipboard<br><br>\n\n"
                                "You can also select the <b>Copy Command</b> from the Edit menu." ) );
  m_paCopy->setStatusText( i18n( "Copies the selected text/item(s) to the clipboard" ) );

  m_paPaste->setWhatsThis( i18n( "Click this button to paste the previously cutted or copied clipboard "
                                 "content<br><br>\n\n"
                                 "You can also select the <b>Paste Command</b> from the Edit menu." ) );
  m_paPaste->setStatusText( i18n( "Pastes the clipboard content" ) );

  m_paPrint->setWhatsThis( i18n( "Click this button to print the currently displayed document<br><br>\n\n"
                                 "You can also select the <b>Print Command</b> from the View menu." ) );
  m_paPrint->setStatusText( i18n( "Print the current document" ) );

  m_paStop->setWhatsThis( i18n( "Click this button to abort loading the document<br><br>\n\n"
                                "You can also select the <b>Stop Command</b> from the View menu." ) );
  m_paStop->setStatusText( i18n( "Stop loading the document" ) );


  // Please proof-read those (David)

  m_ptaUseHTML->setStatusText( i18n("Open index.html when entering a directory, if present") );
  m_paLockView->setStatusText( i18n("A locked view can't change directories. Use in combination with 'link view' to explore many files from one directory") );
  m_paUnlockAll->setStatusText( i18n("Removes locking for all views. Unlock a single view is not possible since it can't be activated.") );
  m_paLinkView->setStatusText( i18n("Sets the view as 'linked'. A linked view follows directory changes done in other linked views") );

}

void KonqMainWindow::updateToolBarActions()
{
  // Enables/disables actions that depend on the current view (mostly toolbar)
  // Up, back, forward, the edit extension, stop button, wheel
  setUpEnabled( m_currentView->url() );
  m_paBack->setEnabled( m_currentView->canGoBack() );
  m_paForward->setEnabled( m_currentView->canGoForward() );

  if ( m_currentView->isLoading() )
    startAnimation(); // takes care of m_paStop
  else
    stopAnimation(); // takes care of m_paStop
}

QString KonqMainWindow::findIndexFile( const QString &dir )
{
  QDir d( dir );

  QString f = d.filePath( "index.html", false );
  if ( QFile::exists( f ) )
    return f;

  f = d.filePath( "index.htm", false );
  if ( QFile::exists( f ) )
    return f;

  f = d.filePath( "index.HTML", false );
  if ( QFile::exists( f ) )
    return f;

  return QString::null;
}

void KonqMainWindow::connectExtension( KParts::BrowserExtension *ext )
{
  kdDebug(1202) << "Connecting extension " << ext << endl;
  ActionSlotMap::ConstIterator it = s_actionSlotMap->begin();
  ActionSlotMap::ConstIterator itEnd = s_actionSlotMap->end();

  QStrList slotNames =  ext->metaObject()->slotNames();

  for ( ; it != itEnd ; ++it )
  {
    KAction * act = actionCollection()->action( it.key() );
    //kdDebug(1202) << it.key() << endl;
    if ( act )
    {
      bool enable = false;
      // Does the extension have a slot with the name of this action ?
      if ( slotNames.contains( it.key()+"()" ) )
      {
        connect( act, SIGNAL( activated() ), ext, it.data() /* SLOT(slot name) */ );
        enable = true;
      }
      act->setEnabled( enable );
    } else kdError(1202) << "Error in BrowserExtension::actionSlotMap(), unknown action : " << it.key() << endl;
  }

  connect( ext, SIGNAL( enableAction( const char *, bool ) ),
           this, SLOT( slotEnableAction( const char *, bool ) ) );
}

void KonqMainWindow::disconnectExtension( KParts::BrowserExtension *ext )
{
  kdDebug(1202) << "Disconnecting extension " << ext << endl;
  ActionSlotMap::ConstIterator it = s_actionSlotMap->begin();
  ActionSlotMap::ConstIterator itEnd = s_actionSlotMap->end();

  QStrList slotNames =  ext->metaObject()->slotNames();

  for ( ; it != itEnd ; ++it )
  {
    KAction * act = actionCollection()->action( it.key() );
    //kdDebug(1202) << it.key() << endl;
    if ( act && slotNames.contains( it.key()+"()" ) )
    {
        //kdDebug(1202) << "disconnectExtension: " << act << " " << act->name() << endl;
        act->disconnect( ext );
    }
  }
  disconnect( ext, SIGNAL( enableAction( const char *, bool ) ),
           this, SLOT( slotEnableAction( const char *, bool ) ) );
}

void KonqMainWindow::slotEnableAction( const char * name, bool enabled )
{
  KAction * act = actionCollection()->action( name );
  if (!act)
    kdWarning(1202) << "Unknown action " << name << " - can't enable" << endl;
  else
    act->setEnabled( enabled );

  // Update "copy files" and "move files" accordingly
  if (m_paCopyFiles && !strcmp( name, "copy" ))
  {
    m_paCopyFiles->setEnabled( enabled );
  }
  if (m_paMoveFiles && !strcmp( name, "cut" ))
  {
    m_paMoveFiles->setEnabled( enabled );
  }
}

void KonqMainWindow::enableAllActions( bool enable )
{
  int count = actionCollection()->count();
  for ( int i = 0; i < count; i++ )
  {
    KAction *act = actionCollection()->action( i );
    if ( !enable || !s_actionSlotMap->contains( act->name() ) )
      act->setEnabled( enable );
  }

  // This method is called with enable=false on startup, and
  // then only once with enable=true when the first view is setup.
  // So the code below is where actions that should initially be disabled are disabled.
  if (enable)
  {
      // we surely don't have any history buffers at this time
      m_paBack->setEnabled( false );
      m_paForward->setEnabled( false );
      // no locked views either
      m_paUnlockAll->setEnabled( false );
      // removeview only if more than one active view
      m_paRemoveView->setEnabled( activeViewsCount() > 1 );
      // link view only if more than one view
      m_paLinkView->setEnabled( viewCount() > 1 );
      // Load profile submenu
      m_pViewManager->profileListDirty();

      slotUndoAvailable( KonqUndoManager::self()->undoAvailable() );

      m_paStop->setEnabled( m_currentView && m_currentView->isLoading() );
  }
  actionCollection()->action( "close" )->setEnabled( true );
}

void KonqMainWindow::openBookmarkURL( const QString & url )
{
  kdDebug(1202) << (QString("KonqMainWindow::openBookmarkURL(%1)").arg(url)) << endl;
  openURL( 0L, KURL( url ) );
}

void KonqMainWindow::setCaption( const QString &caption )
{
  // KParts sends us empty captions when activating a brand new part
  // We can't change it there (in case of apps removing all parts altogether)
  // but here we never do that.
  if ( !caption.isEmpty() )
  {
    //kdDebug(1202) << "KonqMainWindow::setCaption(" << caption << ")" << endl;
    // Keep an unmodified copy of the caption (before kapp->makeStdCaption is applied)
    m_title = caption;
    KParts::MainWindow::setCaption( caption );
  }
}

QString KonqMainWindow::currentURL() const
{
  assert( m_currentView );
  return m_currentView->url().url();
}

void KonqMainWindow::slotPopupMenu( const QPoint &_global, const KURL &url, const QString &_mimeType, mode_t _mode )
{
  slotPopupMenu( 0L, _global, url, _mimeType, _mode );
}

void KonqMainWindow::slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KURL &url, const QString &_mimeType, mode_t _mode )
{
  KFileItem item( url, _mimeType, _mode );
  KFileItemList items;
  items.append( &item );
  slotPopupMenu( client, _global, items, false ); //BE CAREFUL WITH sender() !
}

void KonqMainWindow::slotPopupMenu( const QPoint &_global, const KFileItemList &_items )
{
  slotPopupMenu( 0L, _global, _items );
}

void KonqMainWindow::slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KFileItemList &_items )
{
    slotPopupMenu( client, _global, _items, true );
}

void KonqMainWindow::slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KFileItemList &_items, bool showPropsAndFileType )
{
  KonqView * m_oldView = m_currentView;

  // Make this view active temporarily, if not the current one (e.g. because it's passive)
  m_currentView = childView( static_cast<KParts::ReadOnlyPart *>( sender()->parent() ) );

  if ( m_oldView && m_oldView != m_currentView )
  {
    if ( m_oldView->browserExtension() )
      disconnectExtension( m_oldView->browserExtension() );
    if ( m_currentView->browserExtension() )
      connectExtension( m_currentView->browserExtension() );
  }

  kdDebug(1202) << "KonqMainWindow::slotPopupMenu( " << client << "...)" << " current view=" << m_currentView << " " << m_currentView->part()->className() << endl;

  KActionCollection popupMenuCollection;
  popupMenuCollection.insert( m_paBack );
  popupMenuCollection.insert( m_paForward );
  popupMenuCollection.insert( m_paUp );
  popupMenuCollection.insert( m_paReload );

  popupMenuCollection.insert( m_paUndo );
  popupMenuCollection.insert( m_paCut );
  popupMenuCollection.insert( m_paCopy );
  popupMenuCollection.insert( m_paPaste );
  popupMenuCollection.insert( m_paTrash );
  popupMenuCollection.insert( m_paDelete );
  popupMenuCollection.insert( m_paShred );

  if ( _items.count() == 1 )
    // Can't use X-KDE-BrowserView-HideFromMenus directly in the query because '-' is a substraction !!!
    m_popupEmbeddingServices = KTrader::self()->query( _items.getFirst()->mimetype(),
                                                  "('Browser/View' in ServiceTypes) or "
                                                  "('KParts/ReadOnlyPart' in ServiceTypes)" );

  if ( _items.count() > 0 )
  {
    m_popupURL = _items.getFirst()->url();
    m_popupServiceType = _items.getFirst()->mimetype();
  }
  else
  {
    m_popupURL = KURL();
    m_popupServiceType = QString::null;
  }

  PopupMenuGUIClient *konqyMenuClient = new PopupMenuGUIClient( this, m_popupEmbeddingServices );

  KonqPopupMenu pPopupMenu ( _items,
                             m_currentView->url(),
                             popupMenuCollection,
                             m_pMenuNew,
                 showPropsAndFileType );

  pPopupMenu.factory()->addClient( konqyMenuClient );

  if ( client )
    pPopupMenu.factory()->addClient( client );

  pPopupMenu.exec( _global );

  delete konqyMenuClient;
  m_popupEmbeddingServices.clear();

  if ( m_oldView && m_oldView != m_currentView )
  {
    if ( m_currentView->browserExtension() )
      disconnectExtension( m_currentView->browserExtension() );
    if ( m_oldView->browserExtension() )
      connectExtension( m_oldView->browserExtension() );
  }

  m_currentView = m_oldView;
}

void KonqMainWindow::slotOpenEmbedded()
{
  QCString name = sender()->name();

  m_popupService = m_popupEmbeddingServices[ name.toInt() ]->name();

  m_popupEmbeddingServices.clear();

  QTimer::singleShot( 0, this, SLOT( slotOpenEmbeddedDoIt() ) );
}

void KonqMainWindow::slotOpenEmbeddedDoIt()
{
  m_currentView->stop();
  m_currentView->setLocationBarURL(m_popupURL.url());
  m_currentView->setTypedURL(QString::null);
  if ( m_currentView->changeViewMode( m_popupServiceType,
                                      m_popupService ) )
       m_currentView->openURL( m_popupURL, m_popupURL.prettyURL() );
}

void KonqMainWindow::slotDatabaseChanged()
{
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    (*it)->callExtensionMethod( "refreshMimeTypes()" );
}

void KonqMainWindow::slotReconfigure()
{
  reparseConfiguration();
}

void KonqMainWindow::reparseConfiguration()
{
  kdDebug(1202) << "KonqMainWindow::reparseConfiguration() !" << endl;
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    (*it)->callExtensionMethod( "reparseConfiguration()" );
}

void KonqMainWindow::saveProperties( KConfig *config )
{
  m_pViewManager->saveViewProfile( *config, true /* save URLs */ );
}

void KonqMainWindow::readProperties( KConfig *config )
{
  enableAllActions( true );
  m_pViewManager->loadViewProfile( *config );
}

void KonqMainWindow::setInitialFrameName( const QString &name )
{
  m_initialFrameName = name;
}

void KonqMainWindow::slotActionStatusText( const QString &text )
{
  if ( !m_currentView )
    return;

  KonqFrameStatusBar *statusBar = m_currentView->frame()->statusbar();

  if ( !statusBar )
    return;

  statusBar->message( text );
}

void KonqMainWindow::slotClearStatusText()
{
  if ( !m_currentView )
    return;

  KonqFrameStatusBar *statusBar = m_currentView->frame()->statusbar();

  if ( !statusBar )
    return;

  statusBar->slotClear();
}

void KonqMainWindow::updateOpenWithActions( const KTrader::OfferList &services )
{
  m_openWithActions.clear();

  KTrader::OfferList::ConstIterator it = services.begin();
  KTrader::OfferList::ConstIterator end = services.end();
  for (; it != end; ++it )
  {
    QString name = (*it)->name();

    KAction *action = new KAction( i18n( "Open With %1" ).arg( name ), 0, 0, (*it)->name().latin1() );
    action->setIcon( (*it)->icon() );

    connect( action, SIGNAL( activated() ),
             this, SLOT( slotOpenWith() ) );

    m_openWithActions.append( action );
  }
}

void KonqMainWindow::updateViewModeActions( const KTrader::OfferList &services )
{
  if ( m_viewModeMenu )
  {
    QListIterator<KAction> it( m_viewModeActions );
    for (; it.current(); ++it )
      it.current()->unplug( m_viewModeMenu->popupMenu() );
    delete m_viewModeMenu;
  }

  m_viewModeMenu = 0;
  m_viewModeActions.clear();

  if ( services.count() <= 1 )
    return;

  m_viewModeMenu = new KActionMenu( i18n( "View Mode" ), this );

  KTrader::OfferList::ConstIterator it = services.begin();
  KTrader::OfferList::ConstIterator end = services.end();
  for (; it != end; ++it )
  {
      QVariant prop = (*it)->property( "X-KDE-BrowserView-Toggable" );
      if ( !prop.isValid() || !prop.toBool() ) // No toggable views in view mode
      {
          KRadioAction *action;

          QString icon = (*it)->icon();
          if ( icon != QString::fromLatin1( "unknown" ) )
            // we *have* to specify a parent qobject, otherwise the exclusive group stuff doesn't work!(Simon)
            action = new KRadioAction( (*it)->comment(), icon, 0, this, (*it)->name().ascii() );
          else
            action = new KRadioAction( (*it)->comment(), 0, this, (*it)->name().ascii() );

          if ( (*it)->name() == m_currentView->service()->name() )
              action->setChecked( true );

          action->setExclusiveGroup( "KonqMainWindow_ViewModes" );

          connect( action, SIGNAL( toggled( bool ) ),
                   this, SLOT( slotViewModeToggle( bool ) ) );

          m_viewModeActions.append( action );
          action->plug( m_viewModeMenu->popupMenu() );
      }
  }
}

void KonqMainWindow::plugViewModeActions()
{
  QList<KAction> lst;
  lst.append( m_viewModeMenu );
  plugActionList( "viewmode", lst );
  // display the toolbar viewmode icons only for inode/directory, as here we have dedicated icons
  if ( m_currentView->serviceType() == "inode/directory" )
    plugActionList( "viewmode_toolbar", m_viewModeActions );
}

void KonqMainWindow::unplugViewModeActions()
{
  unplugActionList( "viewmode" );
  unplugActionList( "viewmode_toolbar" );
}

KonqMainWindowIface* KonqMainWindow::dcopObject()
{
  if ( !m_dcopObject )
      m_dcopObject = new KonqMainWindowIface( this );
  return m_dcopObject;
}

void KonqMainWindow::updateBookmarkBar()
{
  // hide if empty
  KToolBar * bar = static_cast<KToolBar *>( child( "bookmarkToolBar", "KToolBar" ) );
  if ( bar && bar->count() <= 1 ) // there is always a separator
  {
    m_paShowBookmarkBar->setChecked( false );
    bar->hide();
  }
  else if ( bar )
    m_paShowBookmarkBar->setChecked( true );
}

#include "konq_mainwindow.moc"
