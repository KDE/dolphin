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
#include <kio/job.h>
#include <kkeydialog.h>
#include <klibloader.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <knewmenu.h>
#include <konqdefaults.h>
#include <konqpopupmenu.h>
#include <konqsettings.h>
#include <kparts/part.h>
#include <kpopupmenu.h>
#include <kprocess.h>
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

#define STATUSBAR_LOAD_ID 1
#define STATUSBAR_SPEED_ID 2
#define STATUSBAR_MSG_ID 3

template class QList<QPixmap>;
template class QList<KToggleAction>;

QStringList *KonqMainWindow::s_plstAnimatedLogo = 0L;
QList<KonqMainWindow> *KonqMainWindow::s_lstViews = 0;
KonqMainWindow::ActionSlotMap *KonqMainWindow::s_actionSlotMap = 0;

KonqMainWindow::KonqMainWindow( const KURL &initialURL, bool openInitialURL, const char *name )
 : KParts::MainWindow( name )
{
  if ( !s_lstViews )
    s_lstViews = new QList<KonqMainWindow>;

  s_lstViews->append( this );

  if ( !s_actionSlotMap )
      s_actionSlotMap = new ActionSlotMap( KParts::BrowserExtension::actionSlotMap() );

  m_currentView = 0L;
  m_pBookmarkMenu = 0L;
  m_dcopObject = 0L;
  m_bURLEnterLock = false;

  m_bViewModeToggled = false;

  if ( !s_plstAnimatedLogo )
  {
    s_plstAnimatedLogo = new QStringList;
    *s_plstAnimatedLogo += KGlobal::iconLoader()->loadAnimated( "kde", KIcon::MainToolbar );
  }

  m_pViewManager = new KonqViewManager( this );

  connect( m_pViewManager, SIGNAL( activePartChanged( KParts::Part * ) ),
	   this, SLOT( slotPartActivated( KParts::Part * ) ) );

  m_toggleViewGUIClient = new ToggleViewGUIClient( this );

  m_openWithActions.setAutoDelete( true );
  m_viewModeActions.setAutoDelete( true );
  m_viewModeMenu = 0;

  initActions();

  setInstance( KonqFactory::instance() );

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
    KConfig *config = KonqFactory::instance()->config();

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
  KConfig *config = KonqFactory::instance()->config();
  KConfigGroupSaver cgs( config, "MainView Settings" );
  m_bSaveViewPropertiesLocally = config->readBoolEntry( "SaveViewPropertiesLocally", false );
  m_paSaveViewPropertiesLocally->setChecked( m_bSaveViewPropertiesLocally );
  m_bHTMLAllowed = config->readBoolEntry( "HTMLAllowed", false );
  m_ptaUseHTML->setChecked( m_bHTMLAllowed );
  m_sViewModeForDirectory = config->readEntry( "ViewMode" );

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

    QStringList comboItems;

    for ( int i = 0; i < m_combo->count(); i++ )
      comboItems.append( m_combo->text( i ) );

    while ( comboItems.count() > 10 )
      comboItems.remove( comboItems.fromLast() );

    KConfig *config = KonqFactory::instance()->config();
    config->setGroup( "Settings" );
    config->writeEntry( "ToolBarCombo", comboItems );
    config->writeEntry( "CompletionMode", (int)m_combo->completionMode() );
    config->writeEntry( "CompletionItems", m_combo->completionObject()->items() );
    config->sync();
  }

  delete m_pViewManager;

  delete m_pBookmarkMenu;

  m_viewModeActions.clear();

  kdDebug(1202) << "KonqMainWindow::~KonqMainWindow done" << endl;
}

QWidget *KonqMainWindow::createContainer( QWidget *parent, int index, const QDomElement &element, const QByteArray &containerStateBuffer, int &id )
{
  static QString nameBookmarkBar = QString::fromLatin1( "bookmarkToolBar" );
  static QString tagToolBar = QString::fromLatin1( "ToolBar" );

  QWidget *res = KParts::MainWindow::createContainer( parent, index, element, containerStateBuffer, id );

  if ( element.tagName() == tagToolBar && element.attribute( "name" ) == nameBookmarkBar )
  {
    assert( res->inherits( "KToolBar" ) );

    (void)new KBookmarkBar( this, static_cast<KToolBar *>( res ), actionCollection(), this );
  }

  return res;
}

void KonqMainWindow::openFilteredURL( const QString & _url )
{
    KURL filteredURL( konqFilteredURL( this, _url ) );

    KonqOpenURLRequest req( _url );

    openURL( 0L, filteredURL, QString::null, req );

    // #4070: Give focus to view after URL was entered manually
    // Note: we do it here if the view mode (i.e. part) wasn't changed
    // If it is changed, then it's done in KonqView::changeViewMode
    if ( m_currentView && m_currentView->part() )
      m_currentView->part()->widget()->setFocus();

}

void KonqMainWindow::openURL( KonqView *_view, const KURL &url,
                              const QString &serviceType, const KonqOpenURLRequest & req )
{
  kdDebug(1202) << "KonqMainWindow::openURL : url = '" << url.url() << "'  "
                << "serviceType='" << serviceType << "'\n";

  if ( url.isMalformed() )
  {
    QString tmp = i18n("Malformed URL\n%1").arg(url.url());
    KMessageBox::error(0, tmp);
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
    if ( view->passiveMode() )
    {
      // Passive views are handled differently. No view-mode change allowed,
      // no need to use the serviceType argument. We just force an openURL on the
      // view and hope it can cope with it. If not, it should just ignore it.
      // In all theory we should call updateHistoryEntry here. Bah.
      view->openURL( url );
      return;
    }
  }

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
        if ( !offer || !KRun::run( *offer, lst ) )
        {
          (void)new KRun( url );
        }
    }
  }
  else // no known serviceType, use KonqRun
  {
      kdDebug(1202) << QString("Creating new konqrun for %1").arg(url.url()) << endl;
      KonqRun * run = new KonqRun( this, view /* can be 0L */, url, req );
      if ( view )
      {
        view->setRun( run );
        if ( view == m_currentView )
          startAnimation();
      }
      connect( run, SIGNAL( finished() ),
               this, SLOT( slotRunFinished() ) );
      connect( run, SIGNAL( error() ),
               this, SLOT( slotRunFinished() ) );
  }
}

bool KonqMainWindow::openView( QString serviceType, const KURL &_url, KonqView *childView, const KonqOpenURLRequest & req )
{
  kdDebug(1202) << "KonqMainWindow::openView " << serviceType << " " << _url.url() << " " << childView << endl;
  QString indexFile;

  KURL url( _url );

  // In case we open an index.html, we want the location bar
  // to still display the original URL (so that 'up' uses that URL,
  // and since that's what the user entered).
  // changeViewMode will take care of setting and storing that url.
  QString originalURL = url.prettyURL();

  QString serviceName; // default: none provided

  // Look for which view mode to use, if a directory - not if view locked to
  // its current mode or passive.
  if ( ( !childView ||
         (!childView->lockedViewMode() && !childView->passiveMode())
       )
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

  if ( !childView )
    {
      // Create a new view
      KonqView* newView = m_pViewManager->splitView( Qt::Horizontal, url, serviceType, serviceName );

      if ( !newView )
      {
        KMessageBox::sorry( 0L, i18n( "Could not create view for %1\nCheck your installation").arg(serviceType) );
        return true; // fake everything was ok, we don't want to propagate the error
      }

      enableAllActions( true );

      newView->setTypedURL( req.typedURL );
      newView->setLocationBarURL( originalURL );

      newView->part()->widget()->setFocus();

      newView->setViewName( m_initialFrameName );
      m_initialFrameName = QString::null;

      return true;
    }
  else // We know the child view
  {
    // We used the 'locking' for keeping the servicename empty,
    // Now we can reset it.
    if ( childView->lockedViewMode() )
        childView->setLockedViewMode( false );
    return childView->changeViewMode( serviceType, serviceName, url, originalURL, req.typedURL );
  }
}

void KonqMainWindow::slotOpenURL( const KURL &url, const KParts::URLArgs &args )
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

//Callled by slotOpenURL
void KonqMainWindow::openURL( KonqView *childView, const KURL &url, const KParts::URLArgs &args )
{
  //TODO: handle post data!

  if ( childView->browserExtension() )
    childView->browserExtension()->setURLArgs( args );

  //  ### HACK !!
  if ( args.postData.size() > 0 )
  {
    openURL( childView, url, QString::fromLatin1( "text/html" ) );
    return;
  }

  if ( !args.reload && urlcmp( url.url(), childView->url().url(), true, true ) )
  {
    QString serviceType = args.serviceType;
    if ( serviceType.isEmpty() )
      serviceType = childView->serviceType();

    openView( serviceType, url, childView );
    return;
  }

  openURL( childView, url, args.serviceType );
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
  //FIXME: obey args (like passing post-data (to KRun), etc.)
  KonqFileManager::self()->openFileManagerWindow( url.url(), args.frameName );
}

void KonqMainWindow::slotNewWindow()
{
  //KonqFileManager::self()->openFileManagerWindow( m_currentView->url() );

  // This code is very related to KonquerorIface::createBrowserWindowFromProfile
  // -> for konq_misc ?
  KonqMainWindow *mainWindow = new KonqMainWindow( QString::null, false );
  // We assume it exists, since we install it... otherwise the window should remain empty
  mainWindow->viewManager()->loadViewProfile( QString::fromLatin1("webbrowsing") );
  mainWindow->enableAllActions( true );
  mainWindow->show();
}

void KonqMainWindow::slotDuplicateWindow()
{
  //KonqFileManager::self()->openFileManagerWindow( m_currentView->url() );

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
  KConfig *config = KonqFactory::instance()->config();
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
    u = m_currentView->url().url();

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

  m_currentView->lockHistory();
  m_currentView->changeViewMode( m_currentView->serviceType(), modeName,
                                 m_currentView->url() );

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
      KConfig *config = KonqFactory::instance()->config();
      KConfigGroupSaver cgs( config, "MainView Settings" );
      config->writeEntry( "ViewMode", modeName );
      config->sync();
      m_sViewModeForDirectory = modeName;
  }
}

void KonqMainWindow::slotShowHTML()
{
  bool b = !m_currentView->allowHTML();

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
      KConfig *config = KonqFactory::instance()->config();
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
    (*it)->setPassiveMode( false );

  m_paUnlockAll->setEnabled( false );
}

void KonqMainWindow::slotLockView()
{
  // Can't access this action in passive mode anyway
  assert(!m_currentView->passiveMode());
  m_currentView->setPassiveMode( true );
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
  openURL( 0L, KURL( KonqFactory::instance()->dirs()->saveLocation("apps") ) );
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
      KonqFactory::instance()->dirs()->resourceDirs("templates").last() );
}

void KonqMainWindow::slotGoAutostart()
{
  KonqFileManager::self()->openFileManagerWindow( KGlobalSettings::autostartPath() );
}

void KonqMainWindow::slotConfigureFileManager()
{
  if (fork() == 0) {
    execl(locate("exe", "kcmshell"), "kcmshell", "kcmkonq", 0);
    warning("Error launching kcmshell kcmkonq!");
    exit(1);
  }
}

void KonqMainWindow::slotConfigureFileTypes()
{
  if (fork() == 0) {
    execl(locate("exe", "kcmshell"), "kcmshell", "filetypes", 0);
    warning("Error launching kcmshell filetypes !");
    exit(1);
  }
}

void KonqMainWindow::slotConfigureBrowser()
{
  if (fork() == 0) {
    execl(locate("exe", "kcmshell"), "kcmshell", "konqhtml", 0);
    warning("Error launching kcmshell konqhtml!");
    exit(1);
  }
}

void KonqMainWindow::slotConfigureEBrowsing()
{
  if (fork() == 0) {
    execl(locate("exe", "kcmshell"), "kcmshell", "ebrowsing", 0);
    warning("Error launching kcmshell ebrowsing!");
    exit(1);
  }
}

void KonqMainWindow::slotConfigureCookies()
{
  if (fork() == 0) {
    execl(locate("exe", "kcmshell"), "kcmshell", "cookies", 0);
    warning("Error launching kcmshell cookies!");
    exit(1);
  }
}

void KonqMainWindow::slotConfigureProxies()
{
  if (fork() == 0) {
    execl(locate("exe", "kcmshell"), "kcmshell", "proxy", 0);
    warning("Error launching kcmshell proxy!");
    exit(1);
  }
}

void KonqMainWindow::slotConfigureKeys()
{
  KKeyDialog::configureKeys(actionCollection(), xmlFile());
}

void KonqMainWindow::slotConfigureToolbars()
{
  QValueList<KXMLGUIClient*> clients = factory()->clients();

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

void KonqMainWindow::slotPartChanged( KonqView *childView, KParts::ReadOnlyPart *oldPart, KParts::ReadOnlyPart *newPart )
{
  m_mapViews.remove( oldPart );
  m_mapViews.insert( newPart, childView );

  // Call the partmanager implementation (the one that only removes the part
  // from its internal list. We don't want to destroy the child view
  m_pViewManager->KParts::PartManager::removePart( oldPart );

  // Add the new part to the manager
  m_pViewManager->addPart( newPart, true );
}


void KonqMainWindow::slotRunFinished()
{
  //kdDebug(1202) << "KonqMainWindow::slotRunFinished()" << endl;
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
      // Revert to working URL
      //childView->setLocationBarURL( childView->history().current()->locationBarURL );
      // Not anymore - it's been reported to be very annoying
    }
  }
}

void KonqMainWindow::applyMainWindowSettings()
{
  KConfig *config = KonqFactory::instance()->config();
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

  // Register this URL as a working one, in the completion object and the combo.

  m_combo->insertItem( view->locationBarURL() );

  QString u = view->typedURL();
  if ( !u.isEmpty() && u != view->locationBarURL() )
    m_combo->completionObject()->addItem( u ); // short version

  m_combo->completionObject()->addItem( view->locationBarURL() );

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

    if ( newView->passiveMode() )
    {
      // Passive view. Don't connect anything, don't change m_currentView
      // Another view will become the current view very soon
      return;
    }
  }

  KParts::BrowserExtension *ext = 0;

  if ( oldView )
    ext = oldView->browserExtension();

  if ( ext )
    disconnectExtension( ext );

  if ( !part )
  {
    createGUI( 0L );
    return;
  }

  m_currentView = newView;
  ext = m_currentView->browserExtension();

  if ( ext )
  {
    connectExtension( ext );
    createGUI( part );
  }
  else
  {
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

  m_currentView->frame()->statusbar()->repaint();

  if ( oldView )
    oldView->frame()->statusbar()->repaint();

  // Can lock a view only if there is a next view
  m_paLockView->setEnabled(m_pViewManager->chooseNextView(m_currentView) != 0L );

  kdDebug(1202) << "slotPartActivated: setting location bar url to "
               << m_currentView->locationBarURL() << endl;
  if ( m_combo )
    m_combo->setEditText( m_currentView->locationBarURL() );

  updateStatusBar();
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

  m_paRemoveView->setEnabled( activeViewsCount() > 1 );

  childView->callExtensionBoolMethod( "setSaveViewPropertiesLocally(bool)", m_bSaveViewPropertiesLocally );
  m_pViewManager->viewCountChanged();
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

  m_paRemoveView->setEnabled( activeViewsCount() > 1 );

  if ( childView == m_currentView )
  {
    m_currentView = 0L;
    m_pViewManager->setActivePart( 0L );
  }
  m_pViewManager->viewCountChanged();
  emit viewRemoved( childView );
}

KonqView *KonqMainWindow::childView( KParts::ReadOnlyPart *view )
{
  MapViews::ConstIterator it = m_mapViews.find( view );
  if ( it != m_mapViews.end() )
    return it.data();
  else
    return 0L;
}

KonqView *KonqMainWindow::childView( const QString &name, KParts::BrowserHostExtension **hostExtension )
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

KonqView *KonqMainWindow::findChildView( const QString &name, KonqMainWindow **mainWindow, KParts::BrowserHostExtension **hostExtension )
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
    if ( !it.data()->passiveMode() )
      ++res;

  return res;
}

KParts::ReadOnlyPart *KonqMainWindow::currentPart()
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

    // Check if sender is linked
    bool bLinked = senderChildView->linkedView();
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

       bool reload = ev->args().reload;
       kdDebug(1202) << "ev->args().reload=" << reload << endl;

       // Linked-views feature
       if ( bLinked && (*it)->linkedView()
            && !(*it)->isLoading()
            && ( (*it)->url() != ev->url() || reload ) ) // avoid loops !
       {
         // A linked view follows this URL if it supports that service type
         // _OR_ if the sender is passive (in this case it's locked and we want
         // the active view to show the URL anyway - example is a web URL in konqdirtree)
         if ( (*it)->supportsServiceType( senderChildView->serviceType() )
              || senderChildView->passiveMode() )
         {
           kdDebug(1202) << "Sending openURL to view " << it.key()->className() << " url:" << ev->url().url() << endl;
           kdDebug(1202) << "Current view url:" << (*it)->url().url() << " reload=" << reload << endl;
           (*it)->setLockedViewMode(true);
	   openURL( (*it), ev->url(), ev->args() );
         } else
           kdDebug(1202) << "View doesn't support service type " << senderChildView->serviceType() << endl;
       }
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
  m_pViewManager->splitView( Qt::Horizontal, m_currentView->url() );
  m_paLockView->setEnabled( true ); // in case we had only one view previously
}

void KonqMainWindow::slotSplitViewVertical()
{
  m_pViewManager->splitView( Qt::Vertical, m_currentView->url() );
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

  //if ( m_pViewManager->chooseNextView(m_currentView) == 0L )
  //  m_currentView->frame()->statusbar()->passiveModeCheckBox()->hide();

  // Can lock a view only if there is a next view
  m_paLockView->setEnabled(m_pViewManager->chooseNextView(m_currentView) != 0L );
}

void KonqMainWindow::slotSaveViewPropertiesLocally()
{
  m_bSaveViewPropertiesLocally = !m_bSaveViewPropertiesLocally;
  // And this is a main-view setting, so save it
  KConfig *config = KonqFactory::instance()->config();
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

/*
void KonqMainWindow::slotSaveDefaultProfile()
{
  KConfig *config = KonqFactory::instance()->config();
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
"  Matt Koss <koss@napri.sk>\n"
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

  KConfig *config = KonqFactory::instance()->config();
  config->setGroup( "Settings" );
  QStringList locationBarCombo = config->readListEntry( "ToolBarCombo" );
  int mode = config->readNumEntry("CompletionMode", KGlobalSettings::completionMode());
  m_combo->setCompletionMode( (KGlobalSettings::Completion) mode ); // set the previous completion-mode
  m_combo->completionObject()->setItems( config->readListEntry( "CompletionItems" ) );

  while ( locationBarCombo.count() > 10 )
    locationBarCombo.remove( locationBarCombo.fromLast() );

/*
  This leaves an empty item in the list box which is no longer
  necessary using KComboBox.  As a side effect though the user
  cannot drop down the list box originally if there was no text
  entered in it.
  if ( locationBarCombo.count() == 0 )
    locationBarCombo << QString();
*/

  m_combo->clear();
  m_combo->insertStringList( locationBarCombo );
  m_combo->setEditText( "" );  // replacement the above commented code
// m_combo->setCurrentItem( 0 ); // not necessary since we use "QComboBox::AtTop"

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
    slotFullScreenStart();
    m_ptaFullScreen->setText( i18n( "Stop Fullscreen Mode" ) );
    m_ptaFullScreen->setIcon( "window_nofullscreen" );
  }
  else
  {
    slotFullScreenStop();
    m_ptaFullScreen->setText( i18n( "Fullscreen Mode" ) );
    m_ptaFullScreen->setIcon( "window_fullscreen" );
  }
}

void KonqMainWindow::slotFullScreenStart()
{
  // Create toolbar button for exiting from full-screen mode
  QList<KAction> lst;
  lst.append( m_ptaFullScreen );
  plugActionList( "fullscreen", lst );

  KonqFrame *widget = m_currentView->frame();
  m_tempContainer = widget->parentContainer();
  m_tempFocusPolicy = widget->focusPolicy();

  widget->statusbar()->hide();

  // see QWidget::showFullScreen()
  widget->reparent( 0L, WStyle_Customize | WStyle_NoBorder | WStyle_StaysOnTop, QPoint( 0, 0 ) );
  widget->resize( QApplication::desktop()->size() );

  m_tempContainer->removeChildFrame( widget );

  attachToolbars( widget );

  widget->setFocusPolicy( QWidget::StrongFocus );
  widget->setFocus();

  widget->show();

  /* Doesn't work
  widget->setMouseTracking( TRUE ); // for autoselect stuff
  QWidget * viewWidget = widget->part()->widget();
  viewWidget->setMouseTracking( TRUE ); // for autoselect stuff
  if ( viewWidget->inherits("QScrollView") )
    ((QScrollView *) viewWidget)->viewport()->setMouseTracking( TRUE );
  */
}

void KonqMainWindow::attachToolbars( KonqFrame *frame )
{
  KToolBar *toolbar = static_cast<KToolBar *>( guiFactory()->container( "locationToolBar", this ) );
  toolbar->setEnableContextMenu(false);
  if ( toolbar->parentWidget() != frame )
  {
    toolbar->reparent( frame, 0, QPoint( 0,0 ) );
    toolbar->show();
  }
  frame->layout()->insertWidget( 0, toolbar );

  toolbar = static_cast<KToolBar *>( guiFactory()->container( "mainToolBar", this ) );
  toolbar->setEnableContextMenu(false);
  if ( toolbar->parentWidget() != frame )
  {
    toolbar->reparent( frame, 0, QPoint( 0, 0 ) );
    toolbar->show();
  }
  frame->layout()->insertWidget( 0, toolbar );
}

void KonqMainWindow::slotFullScreenStop()
{
  unplugActionList( "fullscreen" );
  KToolBar *toolbar1 = static_cast<KToolBar *>( guiFactory()->container( "mainToolBar", this ) );
  KToolBar *toolbar2 = static_cast<KToolBar *>( guiFactory()->container( "locationToolBar", this ) );

  toolbar1->setEnableContextMenu(true);
  toolbar2->setEnableContextMenu(true);

  KonqFrame *widget = m_currentView->frame();
  widget->close( false );
  widget->reparent( m_tempContainer, 0, QPoint( 0, 0 ) );
  widget->statusbar()->show();
  widget->show();
  widget->setFocusPolicy( m_tempFocusPolicy );

  widget->attachInternal();

  toolbar1->reparent( this, 0, QPoint( 0, 0 ), true );
  toolbar2->reparent( this, 0, QPoint( 0, 0 ), true );
}

void KonqMainWindow::setLocationBarURL( const QString &url )
{
  kdDebug(1202) << "KonqMainWindow::setLocationBarURL : url = " << url << endl;
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
  connect( actionCollection(), SIGNAL( actionHighlighted( KAction * ) ),
	   this, SLOT( slotActionHighlighted( KAction * ) ) );

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

  // Those are connected to the browserextension directly
  m_paCut = KStdAction::cut( 0, 0, actionCollection(), "cut" );
  m_paCopy = KStdAction::copy( 0, 0, actionCollection(), "copy" );
  m_paPaste = KStdAction::paste( 0, 0, actionCollection(), "paste" );
  m_paStop = new KAction( i18n( "&Stop" ), "stop", Key_Escape, this, SLOT( slotStop() ), actionCollection(), "stop" );

  // Which is the default
  KConfig *config = KonqFactory::instance()->config();
  config->setGroup( "Trash" );
  int deleteAction = config->readNumEntry("DeleteAction", DEFAULT_DELETEACTION);
  const int deleteKey = CTRL+Key_Delete ; // Key_Delete conflict with the location bar

  m_paTrash = new KAction( i18n( "&Move to Trash" ), "trash", deleteAction==1 ? deleteKey : 0, actionCollection(), "trash" );
  m_paDelete = new KAction( i18n( "&Delete" ), deleteAction==2 ? deleteKey : 0, actionCollection(), "del" );
  m_paShred = new KAction( i18n( "&Shred" ), deleteAction==3 ? deleteKey : 0, actionCollection(), "shred" );

  m_paAnimatedLogo = new KonqLogoAction( *s_plstAnimatedLogo, this, SLOT( slotNewWindow() ), actionCollection(), "animated_logo" );

  (void)new KonqLabelAction( i18n( "Location " ), actionCollection(), "location_label" );

  m_paURLCombo = new KonqComboAction( i18n( "Location " ), 0, this, SLOT( slotURLEntered( const QString & ) ), actionCollection(), "toolbar_url_combo" );
  connect( m_paURLCombo, SIGNAL( plugged() ),
           this, SLOT( slotComboPlugged() ) );

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
  m_paBack->setShortText( i18n( "Display the previous document" ) );

  m_paForward->setWhatsThis( i18n( "Click this button to display the next document<br><br>\n\n"
				   "You can also select the <b>Forward Command</b> from the Go Menu." ) );
	
  m_paHome->setWhatsThis( i18n( "Click this button to display your home directory<br><br>\n\n"
				"You can configure the path of your home directory in the"
				"<b>File Manager Configuration</b> in the <b>KDE Control Center</b>" ) );
  m_paHome->setShortText( i18n( "Enter your home directory" ) );
				
  m_paReload->setWhatsThis( i18n( "Reloads the currently displayed document<br><br>\n\n"
				  "You can also select the <b>Reload Command</b> from the View menu." ) );
  m_paReload->setShortText( i18n( "Reload the current document" ) );
			
  m_paCut->setWhatsThis( i18n( "Click this button to cut the currently selected text or items and move it "
                               "to the system clipboard<br><br>\n\n"
			       "You can also select the <b>Cut Command</b> from the Edit menu." ) );
  m_paCut->setShortText( i18n( "Moves the selected text/item(s) to the clipboard" ) );

  m_paCopy->setWhatsThis( i18n( "Click this button to copy the currently selected text or items to the "
				"system clipboard<br><br>\n\n"
				"You can also select the <b>Copy Command</b> from the Edit menu." ) );
  m_paCopy->setShortText( i18n( "Copies the selected text/item(s) to the clipboard" ) );

  m_paPaste->setWhatsThis( i18n( "Click this button to paste the previously cutted or copied clipboard "
                                 "content<br><br>\n\n"
				 "You can also select the <b>Paste Command</b> from the Edit menu." ) );
  m_paPaste->setShortText( i18n( "Pastes the clipboard content" ) );

  m_paPrint->setWhatsThis( i18n( "Click this button to print the currently displayed document<br><br>\n\n"
				 "You can also select the <b>Print Command</b> from the View menu." ) );
  m_paPrint->setShortText( i18n( "Print the current document" ) );

  m_paStop->setWhatsThis( i18n( "Click this button to abort loading the document<br><br>\n\n"
				"You can also select the <b>Stop Command</b> from the View menu." ) );
  m_paStop->setShortText( i18n( "Stop loading the document" ) );

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
  //kdDebug(1202) << "Connecting extension " << ext << endl;
  typedef QValueList<QCString> QCStringList;
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
        //if ( ! s_dontConnect->contains( it.key() ) )
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
  //kdDebug(1202) << "Disconnecting extension " << ext << endl;
  QValueList<KAction *> actions = actionCollection()->actions();
  QValueList<KAction *>::ConstIterator it = actions.begin();
  QValueList<KAction *>::ConstIterator end = actions.end();
  for (; it != end; ++it )
    (*it)->disconnect( ext );
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
}

void KonqMainWindow::enableAllActions( bool enable )
{
  int count = actionCollection()->count();
  for ( int i = 0; i < count; i++ )
    actionCollection()->action( i )->setEnabled( enable );

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
      // Load profile submenu
      m_pViewManager->profileListDirty();
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
  // Keep an unmodified copy of the caption (before kapp->makeStdCaption is applied)
  m_title = caption;
  KParts::MainWindow::setCaption( caption );
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
  slotPopupMenu( client, _global, items ); //BE CAREFUL WITH sender() !
}

void KonqMainWindow::slotPopupMenu( const QPoint &_global, const KFileItemList &_items )
{
  slotPopupMenu( 0L, _global, _items );
}

void KonqMainWindow::slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KFileItemList &_items )
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
                             m_pMenuNew );

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
  (void) m_currentView->changeViewMode( m_popupServiceType,
					m_popupService,
                                        m_popupURL,
					m_popupURL.url() );
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

void KonqMainWindow::slotActionHighlighted( KAction *action )
{
  if ( !m_currentView )
    return;

  KonqFrameStatusBar *statusBar = m_currentView->frame()->statusbar();

  if ( !statusBar )
    return;

  QString text = action->shortText();
  if ( !text.isEmpty() )
    statusBar->message( text );
}

void KonqMainWindow::updateOpenWithActions( const KTrader::OfferList &services )
{
  static QString openWithText = i18n( "Open With" ).append( ' ' );

  m_openWithActions.clear();

  KTrader::OfferList::ConstIterator it = services.begin();
  KTrader::OfferList::ConstIterator end = services.end();
  for (; it != end; ++it )
  {
    QString comment = (*it)->comment();
    if ( comment.isEmpty() )
      comment = (*it)->name();

    KAction *action = new KAction( comment.prepend( openWithText ), 0, 0, (*it)->name().latin1() );
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

  m_viewModeMenu = new KActionMenu( i18n( "View Mode..." ), this );

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
	    action = new KRadioAction( (*it)->comment(), icon, 0, this, (*it)->name() );
	  else
            action = new KRadioAction( (*it)->comment(), 0, this, (*it)->name() );

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
