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
#include "konq_mainview.h"
#include "konq_childview.h"
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
#include <kaction.h>
#include <kapp.h>
#include <kbookmarkbar.h>
#include <kbookmarkmenu.h>
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
#include <kprocess.h>
#include <kstatusbar.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <kstddirs.h>
#include <ksycoca.h>
#include <ktrader.h>
#include <kurifilter.h>
#include <kurl.h>
#include <kurlrequesterdlg.h>
#include <kuserprofile.h>
#include <kwm.h>

#include "version.h"

#define STATUSBAR_LOAD_ID 1
#define STATUSBAR_SPEED_ID 2
#define STATUSBAR_MSG_ID 3

template class QList<QPixmap>;
template class QList<KToggleAction>;

QStringList *KonqMainView::s_plstAnimatedLogo = 0L;
bool KonqMainView::s_bMoveSelection = false;
QList<KonqMainView> *KonqMainView::s_lstViews = 0;
KonqMainView::ActionSlotMap *KonqMainView::s_actionSlotMap = 0;

KonqMainView::KonqMainView( const KURL &initialURL, bool openInitialURL, const char *name )
 : KParts::MainWindow( name ),  DCOPObject( "KonqMainViewIface" )
{
  if ( !s_lstViews )
    s_lstViews = new QList<KonqMainView>;

  s_lstViews->append( this );

  if ( !s_actionSlotMap )
      s_actionSlotMap = new ActionSlotMap( KParts::BrowserExtension::actionSlotMap() );

  m_currentView = 0L;
  m_pBookmarkMenu = 0L;
  m_bURLEnterLock = false;

  KonqFactory::instanceRef();

  if ( !s_plstAnimatedLogo )
  {
    s_plstAnimatedLogo = new QStringList;
    *s_plstAnimatedLogo += KGlobal::iconLoader()->loadAnimated( "kde", KIcon::MainToolbar );
  }

  m_pViewManager = new KonqViewManager( this );

  connect( m_pViewManager, SIGNAL( activePartChanged( KParts::Part * ) ),
	   this, SLOT( slotPartActivated( KParts::Part * ) ) );

  m_viewModeGUIClient = new ViewModeGUIClient( this );
  m_openWithGUIClient = new OpenWithGUIClient( this );

  m_toggleViewGUIClient = new ToggleViewGUIClient( this );

  initActions();
  initPlugins();

  setInstance( KonqFactory::instance() );

  connect( KSycoca::self(), SIGNAL( databaseChanged() ),
           this, SLOT( slotDatabaseChanged() ) );

  connect( kapp, SIGNAL( kdisplayFontChanged()), SLOT(slotReconfigure()));

  setXMLFile( "konqueror.rc" );

  createGUI( 0L );

  if ( !m_toggleViewGUIClient->empty() )
    guiFactory()->addClient( m_toggleViewGUIClient );
  else
  {
    delete m_toggleViewGUIClient;
    m_toggleViewGUIClient = 0;
  }

  // hide if empty
  KToolBar * bar = (KToolBar *)child( "bookmarkToolBar", "KToolBar" );
  if ( bar && bar->count() <= 1 ) // there is always a separator
  {
    m_paShowBookmarkBar->setChecked( false );
    bar->hide();
  }

  if ( !initialURL.isEmpty() )
    openFilteredURL( 0L, initialURL.url() );
  else if ( openInitialURL )
  {
    KConfig *config = KonqFactory::instance()->config();

    if ( config->hasGroup( "Default View Profile" ) )
    {
      config->setGroup( "Default View Profile" );
      enableAllActions( true );
      m_pViewManager->loadViewProfile( *config );
    }
    else
      openURL( 0L, KURL( QDir::homeDirPath().prepend( "file:" ) ) );
  }

  resize( 700, 480 );
  m_bFullScreen = false;
  kdDebug(1202) << "KonqMainView::KonqMainView done" << endl;
}

KonqMainView::~KonqMainView()
{
  kdDebug(1202) << "KonqMainView::~KonqMainView" << endl;
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
    config->sync();
  }

  delete m_pViewManager;

  delete m_pBookmarkMenu;

  KonqFactory::instanceUnref();
  kdDebug(1202) << "KonqMainView::~KonqMainView done" << endl;
}

QWidget *KonqMainView::createContainer( QWidget *parent, int index, const QDomElement &element, const QByteArray &containerStateBuffer, int &id )
{
  static QString nameBookmarkBar = QString::fromLatin1( "bookmarkToolBar" );
  static QString tagToolBar = QString::fromLatin1( "ToolBar" );

  QWidget *res = KParts::MainWindow::createContainer( parent, index, element, containerStateBuffer, id );

  if ( element.tagName() == tagToolBar && element.attribute( "name" ) == nameBookmarkBar )
  {
    assert( res->inherits( "KToolBar" ) );

    (void)new KBookmarkBar( this, (KToolBar *)res, actionCollection(), this );
  }

  return res;
}

QString KonqMainView::konqFilteredURL( const QString &_url )
{
  KURIFilterData data = _url;
  KURIFilter::self()->filterURI( data );
  if( data.hasBeenFiltered() )
  {
    KURIFilterData::URITypes type = data.uriType();
    if( type == KURIFilterData::UNKNOWN )
    {
      KMessageBox::sorry( this, i18n( "The url \"%1\" is of unknown type" ).arg( _url ) );
      return QString::null;  // should never happen unless the search filter is unavailable.
    }
    else if( type == KURIFilterData::ERROR )
    {
      KMessageBox::sorry( this, i18n( data.errorMsg() ) );
      return QString::null;
    }
    else
      return data.uri().url();
  }
  return _url;  // return the original url if it cannot be filtered.
}

void KonqMainView::openFilteredURL( KonqChildView * /*_view*/, const QString &_url )
{
  KonqURLEnterEvent ev( konqFilteredURL( _url ) );
  QApplication::sendEvent( this, &ev );
}

void KonqMainView::openURL( KonqChildView *_view, const KURL &url, const QString &serviceType )
{
  kdDebug(1202) << "KonqMainView::openURL : url = '" << url.url() << "'  "
                << "serviceType='" << serviceType << "'\n";

  if ( url.isMalformed() )
  {
    QString tmp = i18n("Malformed URL\n%1").arg(url.url());
    KMessageBox::error(0, tmp);
    return;
  }

  KonqChildView *view = _view;
  if ( !view /* || view->passiveMode() */ ) // David: we can open a URL in a passive view
    view = m_currentView;

  if ( view )
  {
    if ( view == m_currentView )
      //will do all the stuff below plus GUI stuff
      slotStop();
    else
      view->stop();
  }

  // Show it for now in the location bar, but we'll need to store it in the view
  // later on (can't do it yet since either view == 0 or updateHistoryEntry will be called).
  //kdDebug(1202) << "setLocationBarURL : url = " << url.url() << endl;
  setLocationBarURL( url.url() );

  bool emitEvent = false;

  kdDebug(1202) << QString("trying openView for %1 (servicetype %2)").arg(url.url()).arg(serviceType) << endl;
  if ( !serviceType.isEmpty() && serviceType != "application/octet-stream" )
  {
    // Built-in view ?
    if ( !openView( serviceType, url, view /* can be 0L */) )
    {
      if ( !view || !view->passiveMode() )
      {
        // We know the servicetype, let's try its preferred service
        KService::Ptr offer = KServiceTypeProfile::preferredService(serviceType);
        KURL::List lst;
        lst.append(url);
        if ( !offer || !KRun::run( *offer, lst ) )
        {
          (void)new KRun( url );
        }
      } else // passive view
          emitEvent = true;
    }
  }
  else // no known serviceType, use KonqRun
  {
    if ( !view || !view->passiveMode() ) // Do nothing if passive view (we'll have a
        // linked active one to take care of this...)
    {
      kdDebug(1202) << QString("Creating new konqrun for %1").arg(url.url()) << endl;
      KonqRun * run = new KonqRun( this, view /* can be 0L */, url, 0, false, true );
      if ( view )
      {
        view->setRun( run );
        if ( view == m_currentView )
        {
          startAnimation();
          view->setLoading( true );
        }
      }
      connect( run, SIGNAL( finished() ),
               this, SLOT( slotRunFinished() ) );
      connect( run, SIGNAL( error() ),
               this, SLOT( slotRunFinished() ) );
    } else emitEvent = true;
  }

  // A passive view doesn't change its view mode. But it still emits the event !
  if ( emitEvent )
  {
    kdDebug(1202) << "Passive view - mimetype mismatch - emitting event" << endl;
    KParts::OpenURLEvent ev( view->view(), url );
    QApplication::sendEvent( this, &ev ); // or customEvent(&ev) directly ?
  }
}

void KonqMainView::openURL( const KURL &url, const KParts::URLArgs &args )
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
      KonqChildView *view = childView( frameName, &hostExtension );
      if ( !view )
      {
        KonqMainView *mainView = 0;
        view = findChildView( frameName, &mainView, &hostExtension );
	
	if ( !view || !mainView )
	{
	  slotCreateNewWindow( url, args );
	  return;
	}

	if ( hostExtension )
	  hostExtension->openURLInFrame( url, args );
	else
  	   mainView->openURL( view, url, args );
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
  KonqChildView *view = childView( part );
  openURL( view, url, args );
}

void KonqMainView::openURL( KonqChildView *childView, const KURL &url, const KParts::URLArgs &args )
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

  if ( !args.reload && urlcmp( url.url(), childView->view()->url().url(), true, true ) )
  {
    QString serviceType = args.serviceType;
    if ( serviceType.isEmpty() )
      serviceType = childView->serviceType();

    openView( serviceType, url, childView );
    return;
  }

  openURL( childView, url, args.serviceType );
}

void KonqMainView::slotCreateNewWindow( const KURL &url, const KParts::URLArgs &args )
{
  //FIXME: obey args (like passing post-data (to KRun), etc.)
  static_cast<KonqFileManager *>( KonqFileManager::getFileManager() )->openFileManagerWindow( url.url(), args.frameName );
}

void KonqMainView::slotNewWindow()
{
  KonqFileManager::getFileManager()->openFileManagerWindow( m_currentView->view()->url().url() );
}

void KonqMainView::slotRun()
{
  // HACK: The command is not executed in the directory
  // we are in currently. Minicli does not support that yet
  QByteArray data;
  kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "popupExecuteCommand()", data );
}

void KonqMainView::slotOpenTerminal()
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

void KonqMainView::slotOpenLocation()
{
  QString u;
  KURL url;

  if (m_currentView)
    u = m_currentView->url().url();

  url = KURLRequesterDlg::getURL( u, this, i18n("Open Location") );
  if (!url.isEmpty())
     openFilteredURL( 0L, url.url() );
}

void KonqMainView::slotToolFind()
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

void KonqMainView::slotViewModeToggle( bool toggle )
{
  if ( !toggle )
    return;

  QString modeName = sender()->name();

  if ( m_currentView->service()->name() == modeName )
    return;

  m_currentView->changeViewMode( m_currentView->serviceType(), modeName,
                                 m_currentView->url() );
}

void KonqMainView::slotOpenWith()
{
  KURL::List lst;
  lst.append( m_currentView->view()->url() );

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

void KonqMainView::slotShowHTML()
{
  bool b = !m_currentView->allowHTML();

  m_currentView->setAllowHTML( b );

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

void KonqMainView::slotUnlockViews()
{
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    (*it)->setPassiveMode( false );

  m_paUnlockAll->setEnabled( false );
}

void KonqMainView::slotLockView()
{
  // Can't access this action in passive mode anyway
  assert(!m_currentView->passiveMode());
  m_currentView->setPassiveMode( true );
  m_paUnlockAll->setEnabled( true );
}

void KonqMainView::slotStop()
{
  if ( m_currentView )
  {
    m_currentView->stop(); // will take care of the statusbar
    stopAnimation();
  }
}

void KonqMainView::slotReload()
{
  m_currentView->reload();
}

void KonqMainView::slotHome()
{
  openURL( 0L, KURL( konqFilteredURL( KonqFMSettings::settings()->homeURL() ) ) );
}

void KonqMainView::slotGoMimeTypes()
{
  openURL( m_currentView, KURL( KonqFactory::instance()->dirs()->saveLocation("mime") ) );
}

void KonqMainView::slotGoApplications()
{
  openURL( m_currentView, KURL( KonqFactory::instance()->dirs()->saveLocation("apps") ) );
}

void KonqMainView::slotGoDirTree()
{
  KonqFileManager::getFileManager()->openFileManagerWindow( locateLocal( "data", "konqueror/dirtree/" ) );
}

void KonqMainView::slotGoTrash()
{
  KonqFileManager::getFileManager()->openFileManagerWindow( KGlobalSettings::trashPath() );
}

void KonqMainView::slotSaveSettings()
{
  // "savePropertiesAsDefault()" is automatically called in the view
  // But we also connected this slot here, in order to
  // save here konqueror settings (show menubar, ...)
  // (TODO)
}

void KonqMainView::slotConfigureFileManager()
{
  if (fork() == 0) {
    execl(locate("exe", "kcmshell"), "kcmshell", "kcmkonq", 0);
    warning("Error launching kcmshell kcmkonq!");
    exit(1);
  }
}

void KonqMainView::slotConfigureBrowser()
{
  if (fork() == 0) {
    execl(locate("exe", "kcmshell"), "kcmshell", "konqhtml", 0);
    warning("Error launching kcmshell konqhtml!");
    exit(1);
  }
}

void KonqMainView::slotConfigureFileTypes()
{
  if (fork() == 0) {
    execl(locate("exe", "kcmshell"), "kcmshell", "filetypes", 0);
    warning("Error launching kcmshell filetypes !");
    exit(1);
  }
}

void KonqMainView::slotConfigureNetwork()
{
  // This should be kcmshell instead, when it supports multiple tabs
  const char * cmd = "kcontrol Network/smb Network/cookies Network/useragent Network/proxy";
  system( cmd );
}

void KonqMainView::slotConfigureKeys()
{
  KKeyDialog::configureKeys(actionCollection(), xmlFile());
}

void KonqMainView::slotConfigureToolbars()
{
  QValueList<KXMLGUIClient*> clients = factory()->clients();

  KEditToolbar edit(factory());
  edit.exec();
}

void KonqMainView::slotViewChanged( KonqChildView *childView, KParts::ReadOnlyPart *oldView, KParts::ReadOnlyPart *newView )
{
  m_mapViews.remove( oldView );
  m_mapViews.insert( newView, childView );

  // Call the partmanager implementation (the one that only removes the part
  // from its internal list. We don't want to destroy the child view
  m_pViewManager->KParts::PartManager::removePart( oldView );

  // Add the new part to the manager
  m_pViewManager->addPart( newView, true );
}


void KonqMainView::slotRunFinished()
{
  KonqRun *run = (KonqRun *)sender();

  if ( run->foundMimeType() )
    return;

  KonqChildView *childView = run->childView();

  if ( run->hasError() )
  {
    kdWarning(1202) << " Couldn't run ... what do we do ? " << endl;
    if ( !childView ) // Nothing to show ??
    {
      close(); // This window is useless
      KMessageBox::sorry(0L, i18n("Could not display the requested URL, closing the window"));
      // or should we go back $HOME ?
      return;
    }
  }

  // No error but no mimetype either... we just drop it.

  if ( !childView )
    return;

  childView->setLoading( false );

  if ( childView == m_currentView )
  {
    stopAnimation();
    // Revert to working URL
    childView->setLocationBarURL( childView->history().current()->locationBarURL );
  }
}

void KonqMainView::slotSetStatusBarText( const QString & )
{
   // Reimplemented to disable KParts::MainWindow default behaviour
   // Does nothing here, see konq_frame.cc
}

bool KonqMainView::openView( QString serviceType, const KURL &_url, KonqChildView *childView )
{
  kdDebug(1202) << " KonqMainView::openView " << serviceType << " " << _url.url() << endl;
  QString indexFile;

  KURL url( _url );

  if ( !childView )
    {
      // Create a new view
      KonqChildView* newView = m_pViewManager->splitView( Qt::Horizontal, url, serviceType );

      if ( !newView )
      {
        KMessageBox::sorry( 0L, i18n( "Could not create view for %1\nCheck your installation").arg(serviceType) );
        return true; // fake everything was ok, we don't want to propagate the error
      }

      enableAllActions( true );

      newView->view()->widget()->setFocus();
      // Triggered by setFocus anyway...
      //m_pViewManager->setActivePart( view );

      newView->setLocationBarURL( url.url() );
      newView->setViewName( m_initialFrameName );
      m_initialFrameName = QString::null;

      return true;
    }
  else // We know the child view
  {
    kdDebug(1202) << "KonqMainView::openView : url = " << url.url() << endl;

    // In case we open an index.html or .kde.html, we want the location bar
    // to still display the original URL (so that 'up' uses that URL,
    // and since that's what the user entered).
    // changeViewMode will take care of setting and storing that url.
    QString originalURL = url.url();

    if ( ( serviceType == "inode/directory" ) &&
         ( childView->allowHTML() ) &&
         ( url.isLocalFile() ) &&
	 ( ( indexFile = findIndexFile( url.path() ) ) != QString::null ) )
    {
      serviceType = "text/html";
      KURL::encode( indexFile );
      url = KURL( indexFile );
    }

    return childView->changeViewMode( serviceType, QString::null, url, originalURL );
  }
}

void KonqMainView::slotPartActivated( KParts::Part *part )
{
  kdDebug(1202) << "slotPartActivated " << part << endl;
  if ( !part )
  {
    createGUI( 0L );
    return;
  }

  KonqChildView *newView = m_mapViews.find( (KParts::ReadOnlyPart *)part ).data();

  if ( newView->passiveMode() )
  {
    if ( newView->browserExtension() )
      connectExtension( newView->browserExtension() );
    return;
  }

  KonqChildView *oldView = m_currentView;

  if ( m_currentView && m_currentView->browserExtension() )
    disconnectExtension( m_currentView->browserExtension() );

  m_currentView = newView;

  if ( m_currentView->browserExtension() )
  {
    connectExtension( m_currentView->browserExtension() );
    createGUI( part );
  }
  else
  {
    // Disable all browser-extension actions

    ActionSlotMap::ConstIterator it = s_actionSlotMap->begin();
    ActionSlotMap::ConstIterator itEnd = s_actionSlotMap->end();

    for ( ; it != itEnd ; ++it )
    {
      QCString actionName = it.key();
      if (actionName == "pastecut" || actionName == "pastecopy")
        actionName = "paste";
      KAction * act = actionCollection()->action( actionName );
      assert(act);
      act->setEnabled( false );
    }

    createGUI( 0L );
  }

  // View-dependent GUI

  guiFactory()->removeClient( m_viewModeGUIClient );
  guiFactory()->removeClient( m_openWithGUIClient );
  m_viewModeGUIClient->update( m_currentView->partServiceOffers() );
  m_openWithGUIClient->update( m_currentView->appServiceOffers() );

  KService::Ptr service = currentChildView()->service();
  QVariant prop = service->property( "X-KDE-BrowserView-Toggable" );
  if ( !prop.isValid() || !prop.toBool() ) // No view mode for toggable views
  // (The other way would be to enforce a better servicetype for them, than Browser/View)

    if ( m_currentView->partServiceOffers().count() > 1 )
      guiFactory()->addClient( m_viewModeGUIClient );

  if ( m_currentView->appServiceOffers().count() > 0 )
    guiFactory()->addClient( m_openWithGUIClient );

  m_currentView->frame()->statusbar()->repaint();

  if ( oldView )
    oldView->frame()->statusbar()->repaint();

  // Can lock a view only if there is a next view
  m_paLockView->setEnabled(m_pViewManager->chooseNextView(m_currentView) != 0L );

  kdDebug(300) << "slotPartActivated: setting location bar url to "
               << m_currentView->locationBarURL() << endl;
  if ( m_combo )
    m_combo->setEditText( m_currentView->locationBarURL() );

  updateStatusBar();
  updateToolBarActions();
}

void KonqMainView::insertChildView( KonqChildView *childView )
{
  m_mapViews.insert( childView->view(), childView );

  m_paRemoveView->setEnabled( activeViewsCount() > 1 );

  emit viewAdded( childView );
}

void KonqMainView::removeChildView( KonqChildView *childView )
{
  MapViews::Iterator it = m_mapViews.find( childView->view() );
  if ( it == m_mapViews.end() )
  {
      kdWarning(1202) << "KonqMainView::removeChildView childView " << childView << " not in map !" << endl;
      return;
  }
  m_mapViews.remove( it );

  m_paRemoveView->setEnabled( activeViewsCount() > 1 );

  if ( childView == m_currentView )
  {
    m_currentView = 0L;
    m_pViewManager->setActivePart( 0L );
  }
  if ( viewCount() == 1 && m_currentView )
    m_currentView->frame()->statusbar()->hideStuff();

  emit viewRemoved( childView );
}

KonqChildView *KonqMainView::childView( KParts::ReadOnlyPart *view )
{
  MapViews::ConstIterator it = m_mapViews.find( view );
  if ( it != m_mapViews.end() )
    return it.data();
  else
    return 0L;
}

KonqChildView *KonqMainView::childView( const QString &name, KParts::BrowserHostExtension **hostExtension )
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
        *hostExtension = KonqChildView::hostExtension( it.data()->view(), name );
      return it.data();
    }
  }

  return 0;
}

KonqChildView *KonqMainView::findChildView( const QString &name, KonqMainView **mainView, KParts::BrowserHostExtension **hostExtension )
{
  if ( !s_lstViews )
    return 0;

  QListIterator<KonqMainView> it( *s_lstViews );
  for (; it.current(); ++it )
  {
    KonqChildView *res = it.current()->childView( name, hostExtension );
    if ( res )
    {
      if ( mainView )
        *mainView = it.current();
      return res;
    }
  }

  return 0;
}

int KonqMainView::activeViewsCount() const
{
  int res = 0;
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    if ( !it.data()->passiveMode() )
      ++res;

  return res;
}

KParts::ReadOnlyPart *KonqMainView::currentView()
{
  if ( m_currentView )
    return m_currentView->view();
  else
    return 0L;
}

void KonqMainView::customEvent( QCustomEvent *event )
{
  KParts::MainWindow::customEvent( event );

  if ( KonqURLEnterEvent::test( event ) )
  {
    QString url = ((KonqURLEnterEvent *)event)->url();

    openURL( 0L, KURL( url ) );

    return;
  }
  if ( KonqFileSelectionEvent::test( event ) )
  {
    // Forward the event to all views
    MapViews::ConstIterator it = m_mapViews.begin();
    MapViews::ConstIterator end = m_mapViews.end();
    for (; it != end; ++it )
      QApplication::sendEvent( (*it)->view(), event );
    return;
  }
  if ( KParts::OpenURLEvent::test( event ) )
  {
    KParts::OpenURLEvent * ev = static_cast<KParts::OpenURLEvent*>(event);
    KonqChildView * senderChildView = childView(ev->part());
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
       // Linked-views feature
       if ( bLinked && (*it)->linkedView()
            && !(*it)->isLoading()
            && (*it)->url() != ev->url() ) // avoid loops !
       {
         // A linked view follows this URL if it supports that service type
         // _OR_ if the sender is passive (in this case it's locked and we want
         // the active view to show the URL anyway - example is a web URL in konqdirtree)
         if ( (*it)->supportsServiceType( senderChildView->serviceType() )
              || senderChildView->passiveMode() )
         {
           kdDebug(1202) << "Sending openURL to view, url:" << ev->url().url() << endl;
           kdDebug(1202) << "Current view url:" << (*it)->url().url() << endl;
           openURL( (*it), ev->url() );
         } else
           kdDebug(1202) << "View doesn't support service type " << senderChildView->serviceType() << endl;
       }
      }
    }
  }
}

void KonqMainView::slotURLEntered( const QString &text )
{
  if ( m_bURLEnterLock )
    return;

  m_bURLEnterLock = true;

  openFilteredURL( 0L, text );

  m_bURLEnterLock = false;
}

void KonqMainView::slotFileNewAboutToShow()
{
  // As requested by KNewMenu :
  m_pMenuNew->slotCheckUpToDate();
  // And set the files that the menu apply on :
  m_pMenuNew->setPopupFiles( m_currentView->url().url() );
}

void KonqMainView::slotSplitViewHorizontal()
{
  m_pViewManager->splitView( Qt::Horizontal, m_currentView->url() );
  m_paLockView->setEnabled( true ); // in case we had only one view previously
}

void KonqMainView::slotSplitViewVertical()
{
  m_pViewManager->splitView( Qt::Vertical, m_currentView->url() );
  m_paLockView->setEnabled( true ); // in case we had only one view previously
}

void KonqMainView::slotSplitWindowHorizontal()
{
  m_pViewManager->splitWindow( Qt::Horizontal );
  m_paLockView->setEnabled( true ); // in case we had only one view previously
}

void KonqMainView::slotSplitWindowVertical()
{
  m_pViewManager->splitWindow( Qt::Vertical );
  m_paLockView->setEnabled( true ); // in case we had only one view previously
}

void KonqMainView::slotRemoveView()
{
  // takes care of choosing the new active view
  m_pViewManager->removeView( m_currentView );

  //if ( m_pViewManager->chooseNextView(m_currentView) == 0L )
  //  m_currentView->frame()->statusbar()->passiveModeCheckBox()->hide();

  // Can lock a view only if there is a next view
  m_paLockView->setEnabled(m_pViewManager->chooseNextView(m_currentView) != 0L );
}

void KonqMainView::slotSaveDefaultProfile()
{
  KConfig *config = KonqFactory::instance()->config();
  config->setGroup( "Default View Profile" );
  m_pViewManager->saveViewProfile( *config );
}

void KonqMainView::callExtensionMethod( KonqChildView * childView, const char * methodName )
{
  QObject *obj = childView->view()->child( 0L, "KParts::BrowserExtension" );
  // assert(obj); Hmm, not all views have a browser extension !
  if ( !obj )
    return;

  QMetaData * mdata = obj->metaObject()->slot( methodName );
  if( mdata )
    (obj->*(mdata->ptr))();
}

void KonqMainView::slotCut()
{
  kdDebug(1202) << "slotCut - sending cut to konqueror* and kdesktop, with true" << endl;
  // Call cut on the child object
  callExtensionMethod( m_currentView, "cut()" );

  QByteArray data;
  QDataStream stream( data, IO_WriteOnly );
  stream << (int)true;
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "setMoveSelection(int)", data );
  kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "setMoveSelection(int)", data );
  s_bMoveSelection = true;
}

void KonqMainView::slotCopy()
{
  kdDebug(1202) << "slotCopy - sending cut to konqueror* and kdesktop, with false" << endl;
  // Call copy on the child object
  callExtensionMethod( m_currentView, "copy()" );

  QByteArray data;
  QDataStream stream( data, IO_WriteOnly );
  stream << (int)false;
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "setMoveSelection(int)", data );
  kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "setMoveSelection(int)", data );
  s_bMoveSelection = false;
}

void KonqMainView::slotPaste()
{
  kdDebug(1202) << "slotPaste() - moveselection is " << s_bMoveSelection << endl;
  if ( s_bMoveSelection )
    callExtensionMethod( m_currentView, "pastecut()" );
  else
    callExtensionMethod( m_currentView, "pastecopy()" );
}

void KonqMainView::slotAbout()
{
  KMessageBox::about( 0, i18n(
"Konqueror Version " KONQUEROR_VERSION "\n"
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
  ));
}

void KonqMainView::slotUpAboutToShow()
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
    popup->insertItem( KMimeType::pixmapForURL( u ), u.decodedURL() );

    if ( u.path() == "/" )
      break;

    if ( ++i > 10 )
      break;

    u = u.upURL();
  }
}

void KonqMainView::slotUp()
{
  kdDebug(1202) << "slotUp. Start URL is " << m_currentView->locationBarURL() << endl;
  openURL( (KonqChildView *)m_currentView, KURL(m_currentView->locationBarURL()).upURL() );
}

void KonqMainView::slotUpActivated( int id )
{
  KURL u( m_currentView->locationBarURL() );
  kdDebug(1202) << "slotUpActivated. Start URL is " << u.url() << endl;
  for ( int i = 0 ; i < m_paUp->popupMenu()->indexOf( id ) + 1 ; i ++ )
      u = u.upURL();
  openURL( 0L, u );
}

void KonqMainView::slotGoMenuAboutToShow()
{
  kdDebug(1202) << "KonqMainView::slotGoMenuAboutToShow" << endl;
  m_paHistory->fillGoMenu( m_currentView->history() );
}

void KonqMainView::slotGoHistoryActivated( int steps )
{
  m_currentView->go( steps );
}

void KonqMainView::slotBackAboutToShow()
{
  m_paBack->popupMenu()->clear();
  KonqHistoryAction::fillHistoryPopup( m_currentView->history(), m_paBack->popupMenu(), true, false );
}

void KonqMainView::slotBack()
{
  m_currentView->go( -1 );
}

void KonqMainView::slotBackActivated( int id )
{
  m_currentView->go( -(m_paBack->popupMenu()->indexOf( id ) + 1) );
}

void KonqMainView::slotForwardAboutToShow()
{
  m_paForward->popupMenu()->clear();
  KonqHistoryAction::fillHistoryPopup( m_currentView->history(), m_paForward->popupMenu(), false, true );
}

void KonqMainView::slotForward()
{
  m_currentView->go( 1 );
}

void KonqMainView::slotForwardActivated( int id )
{
  m_currentView->go( m_paForward->popupMenu()->indexOf( id ) + 1 );
}

void KonqMainView::slotComboPlugged()
{
  m_combo = m_paURLCombo->combo();

  KConfig *config = KonqFactory::instance()->config();
  config->setGroup( "Settings" );
  QStringList locationBarCombo = config->readListEntry( "ToolBarCombo" );

  while ( locationBarCombo.count() > 10 )
    locationBarCombo.remove( locationBarCombo.fromLast() );

  if ( locationBarCombo.count() == 0 )
    locationBarCombo << QString();

  m_combo->clear();
  m_combo->insertStringList( locationBarCombo );
  m_combo->setCurrentItem( 0 );
}

void KonqMainView::slotShowMenuBar()
{
  if (menuBar()->isVisible())
    menuBar()->hide();
  else
    menuBar()->show();
}

/*
void KonqMainView::slotShowStatusBar()
{
  if (statusBar()->isVisible())
    statusBar()->hide();
  else
    statusBar()->show();
}
*/

void KonqMainView::slotShowToolBar()
{
  toggleBar( "mainToolBar", "KToolBar" );
}

void KonqMainView::slotShowLocationBar()
{
  toggleBar( "locationToolBar", "KToolBar" );
}

void KonqMainView::slotShowBookmarkBar()
{
  toggleBar( "bookmarkToolBar", "KToolBar" );
}

void KonqMainView::toggleBar( const char *name, const char *className )
{
  KToolBar *bar = (KToolBar *)child( name, className );
  if ( !bar )
    return;
  if ( bar->isVisible() )
    bar->hide();
  else
    bar->show();
}

void KonqMainView::slotFullScreenStart()
{
  // Create toolbar button for exiting from full-screen mode
  m_fullScreenGUIClient = new FullScreenGUIClient( this );
  guiFactory()->addClient( m_fullScreenGUIClient );

  KonqFrame *widget = m_currentView->frame();
  m_tempContainer = widget->parentContainer();
  m_tempFocusPolicy = widget->focusPolicy();

  widget->statusbar()->hide();

  // see QWidget::showFullScreen()
  widget->reparent( 0L, WStyle_Customize | WStyle_NoBorder | WStyle_StaysOnTop, QPoint( 0, 0 ) );
  widget->resize( QApplication::desktop()->size() );

  attachToolbars( widget );

  widget->setFocusPolicy( QWidget::StrongFocus );
  widget->setFocus();

  widget->show();

  /* Doesn't work
  widget->setMouseTracking( TRUE ); // for autoselect stuff
  QWidget * viewWidget = widget->view()->widget();
  viewWidget->setMouseTracking( TRUE ); // for autoselect stuff
  if ( viewWidget->inherits("QScrollView") )
    ((QScrollView *) viewWidget)->viewport()->setMouseTracking( TRUE );
  */

  m_bFullScreen = true;
}

void KonqMainView::attachToolbars( KonqFrame *frame )
{
  QWidget *toolbar = guiFactory()->container( "locationToolBar", this );
  ((KToolBar*)toolbar)->setEnableContextMenu(false);
  if ( toolbar->parentWidget() != frame )
  {
    toolbar->reparent( frame, 0, QPoint( 0,0 ) );
    toolbar->show();
  }
  frame->layout()->insertWidget( 0, toolbar );

  toolbar = guiFactory()->container( "mainToolBar", this );
  ((KToolBar*)toolbar)->setEnableContextMenu(false);
  if ( toolbar->parentWidget() != frame )
  {
    toolbar->reparent( frame, 0, QPoint( 0, 0 ) );
    toolbar->show();
  }
  frame->layout()->insertWidget( 0, toolbar );
}

void KonqMainView::slotFullScreenStop()
{
  guiFactory()->removeClient( m_fullScreenGUIClient );
  delete m_fullScreenGUIClient;
  m_fullScreenGUIClient = 0L;
  QWidget *toolbar1 = guiFactory()->container( "mainToolBar", this );
  QWidget *toolbar2 = guiFactory()->container( "locationToolBar", this );

  ((KToolBar*)toolbar1)->setEnableContextMenu(true);
  ((KToolBar*)toolbar2)->setEnableContextMenu(true);

  KonqFrame *widget = m_currentView->frame();
  widget->close( false );
  widget->reparent( m_tempContainer, 0, QPoint( 0, 0 ) );
  widget->statusbar()->show();
  widget->show();
  widget->setFocusPolicy( m_tempFocusPolicy );
  m_bFullScreen = false;

  widget->attachInternal();

  toolbar1->reparent( this, 0, QPoint( 0, 0 ) );
  toolbar1->show();
  toolbar2->reparent( this, 0, QPoint( 0, 0 ) );
  toolbar2->show();
}

void KonqMainView::setLocationBarURL( const QString &url )
{
  kdDebug(1202) << "KonqMainView::setLocationBarURL : url = " << url << endl;
  if ( m_combo )
    m_combo->setEditText( url );
}

void KonqMainView::startAnimation()
{
  m_paAnimatedLogo->start();
  m_paStop->setEnabled( true );
}

void KonqMainView::stopAnimation()
{
  m_paAnimatedLogo->stop();
  m_paStop->setEnabled( false );
}

void KonqMainView::setUpEnabled( const KURL &url )
{
  bool bHasUpURL = false;

  if ( url.hasPath() )
    bHasUpURL = ( url.path() != "/" );
  if ( !bHasUpURL )
    bHasUpURL = url.hasSubURL();

  m_paUp->setEnabled( bHasUpURL );
}

void KonqMainView::initActions()
{

  // Note about this method : don't call setEnabled() on any of the actions.
  // They are all disabled then re-enabled with enableAllActions
  // If any one needs to be initially disabled, put that code in enableAllActions

  // File menu
  m_pMenuNew = new KNewMenu ( actionCollection(), "new_menu" );
  QObject::connect( m_pMenuNew->popupMenu(), SIGNAL(aboutToShow()),
                    this, SLOT(slotFileNewAboutToShow()) );

  m_paFileType = new KAction( i18n( "Edit File Type..." ), 0, actionCollection(), "editMimeType" );
  m_paProperties = new KAction( i18n( "Properties..." ), 0, actionCollection(), "properties" );
  m_paNewWindow = new KAction( i18n( "New &Window" ), "window_new", KStdAccel::key(KStdAccel::New), this, SLOT( slotNewWindow() ), actionCollection(), "new_window" );

  (void) new KAction( i18n( "&Run Command..." ), "run", 0/*kdesktop has a binding for it*/, this, SLOT( slotRun() ), actionCollection(), "run" );
  (void) new KAction( i18n( "Open &Terminal..." ), "openterm", CTRL+Key_T, this, SLOT( slotOpenTerminal() ), actionCollection(), "open_terminal" );
  (void) new KAction( i18n( "&Open Location..." ), "fileopen", KStdAccel::key(KStdAccel::Open), this, SLOT( slotOpenLocation() ), actionCollection(), "open_location" );
  (void) new KAction( i18n( "&Find file..." ), "find", 0 /*not KStdAccel::find!*/, this, SLOT( slotToolFind() ), actionCollection(), "findfile" );

  m_paPrint = KStdAction::print( 0, 0, actionCollection(), "print" );
  m_paShellClose = KStdAction::close( this, SLOT( close() ), actionCollection(), "close" );

  m_ptaUseHTML = new KToggleAction( i18n( "&Use HTML" ), 0, this, SLOT( slotShowHTML() ), actionCollection(), "usehtml" );
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

  (void) new KAction( i18n( "Home Directory" ), "gohome", KStdAccel::key(KStdAccel::Home), this, SLOT( slotHome() ), actionCollection(), "home" );

  (void) new KAction( i18n( "File &Types" ), 0, this, SLOT( slotGoMimeTypes() ), actionCollection(), "go_mimetypes" );
  (void) new KAction( i18n( "App&lications" ), 0, this, SLOT( slotGoApplications() ), actionCollection(), "go_applications" );
  (void) new KAction( i18n( "Directory Tree" ), 0, this, SLOT( slotGoDirTree() ), actionCollection(), "go_dirtree" );
  (void) new KAction( i18n( "Trash" ), 0, this, SLOT( slotGoTrash() ), actionCollection(), "go_trash" );
  // Perhaps we should add Templates and Autostart as well ?

  // Options menu
  m_paSaveSettings = new KAction( i18n( "Sa&ve Settings" ), 0, this, SLOT( slotSaveSettings() ), actionCollection(), "savePropertiesAsDefault" );
  m_paSaveLocalProperties = new KAction( i18n( "Save Settings for this &URL" ), 0, actionCollection(), "saveLocalProperties" );

  m_paConfigureFileManager = new KAction( i18n( "Configure File &Manager..." ), 0, this, SLOT( slotConfigureFileManager() ), actionCollection(), "configurefilemanager" );
  m_paConfigureBrowser = new KAction( i18n( "Configure &Browser..." ), 0, this, SLOT( slotConfigureBrowser() ), actionCollection(), "configurebrowser" );
  m_paConfigureFileTypes = new KAction( i18n( "Configure File &Associations..." ), 0, this, SLOT( slotConfigureFileTypes() ), actionCollection(), "configurefiletypes" );
  m_paConfigureNetwork = new KAction( i18n( "Configure &Network..." ), 0, this, SLOT( slotConfigureNetwork() ), actionCollection(), "configurenetwork" );
  m_paConfigureKeys = new KAction( i18n( "Configure &Keys..." ), 0, this, SLOT( slotConfigureKeys() ), actionCollection(), "configurekeys" );

  m_paConfigureToolbars = new KAction( i18n( "Configure Tool&bars..." ), 0, this, SLOT( slotConfigureToolbars() ), actionCollection(), "configuretoolbars" );

  m_paSplitViewHor = new KAction( i18n( "Split View &Horizontally" ), CTRL+SHIFT+Key_H, this, SLOT( slotSplitViewHorizontal() ), actionCollection(), "splitviewh" );
  m_paSplitViewVer = new KAction( i18n( "Split View &Vertically" ), CTRL+SHIFT+Key_V, this, SLOT( slotSplitViewVertical() ), actionCollection(), "splitviewv" );
  m_paSplitWindowHor = new KAction( i18n( "Split Window Horizontally" ), 0, this, SLOT( slotSplitWindowHorizontal() ), actionCollection(), "splitwindowh" );
  m_paSplitWindowVer = new KAction( i18n( "Split Window Vertically" ), 0, this, SLOT( slotSplitWindowVertical() ), actionCollection(), "splitwindowv" );
  m_paRemoveView = new KAction( i18n( "Remove Active View" ), CTRL+SHIFT+Key_R, this, SLOT( slotRemoveView() ), actionCollection(), "removeview" );

  m_paSaveDefaultProfile = new KAction( i18n( "Save Current Profile As Default" ), 0, this, SLOT( slotSaveDefaultProfile() ), actionCollection(), "savedefaultprofile" );

  m_paSaveRemoveViewProfile = new KAction( i18n( "Save/Remove View Profile" ), 0, m_pViewManager, SLOT( slotProfileDlg() ), actionCollection(), "saveremoveviewprofile" );
  m_pamLoadViewProfile = new KActionMenu( i18n( "Load View Profile" ), actionCollection(), "loadviewprofile" );

  m_pViewManager->setProfiles( m_pamLoadViewProfile );

  m_paFullScreenStart = new KAction( i18n( "Fullscreen Mode" ), "window_fullscreen", 0, this, SLOT( slotFullScreenStart() ), actionCollection(), "fullscreenstart" );
  m_paFullScreenStop = new KAction( i18n( "Stop Fullscreen Mode" ), "window_nofullscreen", 0, this, SLOT( slotFullScreenStop() ), actionCollection(), "fullscreenstop" );

  /*
  QPixmap konqpix = KGlobal::iconLoader()->loadIcon( "konqueror", KIcon::Small );
  (void) new KAction( i18n( "&About Konqueror..." ), konqpix, 0, this, SLOT( slotAbout() ), actionCollection(), "about" );
  */

  KHelpMenu * m_helpMenu = new KHelpMenu( this );
  KStdAction::helpContents( m_helpMenu, SLOT( appHelpActivated() ), actionCollection(), "contents" );
  KStdAction::whatsThis( m_helpMenu, SLOT( contextHelpActivated() ), actionCollection(), "whats_this" );
  KStdAction::aboutApp( this, SLOT( slotAbout() ), actionCollection(), "about_app" );
  KStdAction::aboutKDE( m_helpMenu, SLOT( aboutKDE() ), actionCollection(), "about_kde" );
  KStdAction::reportBug( m_helpMenu, SLOT( reportBug() ), actionCollection(), "report_bug" );

  m_paReload = new KAction( i18n( "&Reload" ), "reload", KStdAccel::key(KStdAccel::Reload), this, SLOT( slotReload() ), actionCollection(), "reload" );

  m_paCut = KStdAction::cut( this, SLOT( slotCut() ), actionCollection(), "cut" );
  m_paCopy = KStdAction::copy( this, SLOT( slotCopy() ), actionCollection(), "copy" );
  m_paPaste = KStdAction::paste( this, SLOT( slotPaste() ), actionCollection(), "paste" );
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
  //m_paShowStatusBar = KStdAction::showStatusbar( this, SLOT( slotShowStatusBar() ), actionCollection(), "showstatusbar" );
  m_paShowToolBar = KStdAction::showToolbar( this, SLOT( slotShowToolBar() ), actionCollection(), "showtoolbar" );
  m_paShowLocationBar = new KToggleAction( i18n( "Show &Locationbar" ), 0, actionCollection(), "showlocationbar" );
  m_paShowBookmarkBar = new KToggleAction( i18n( "Show &Bookmarkbar" ), 0, actionCollection(), "showbookmarkbar" );

  m_paShowLocationBar->setChecked( true );
  m_paShowBookmarkBar->setChecked( true );

  connect( m_paShowLocationBar, SIGNAL( activated() ), this, SLOT( slotShowLocationBar() ) );
  connect( m_paShowBookmarkBar, SIGNAL( activated() ), this, SLOT( slotShowBookmarkBar() ) );

  enableAllActions( false );
}

void KonqMainView::initPlugins()
{
  KTrader::OfferList offers = KTrader::self()->query( "Konqueror/Plugin" );
  KTrader::OfferList::ConstIterator it = offers.begin();
  KTrader::OfferList::ConstIterator end = offers.end();

  for (; it != end; ++it )
  {
    if ( (*it)->library().isEmpty() )
      continue;

    KLibFactory *factory = KLibLoader::self()->factory( (*it)->library() );
						
    if ( !factory )
      continue;

    (void)factory->create( this );
  }
}

void KonqMainView::updateToolBarActions()
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

QString KonqMainView::findIndexFile( const QString &dir )
{
  QDir d( dir );

  QString f = d.filePath( "index.html", false );
  if ( QFile::exists( f ) )
    return f;

  f = d.filePath( ".kde.html", false );
  if ( QFile::exists( f ) )
    return f;

  return QString::null;
}

void KonqMainView::connectExtension( KParts::BrowserExtension *ext )
{
  typedef QValueList<QCString> QCStringList;
  static QCStringList * s_dontConnect = 0L;
  // The actions in s_dontConnect are not directly connected to the BrowserExtension.
  // The slots in this class are connected
  // instead and call in turn the ones in the view...

  if ( !s_dontConnect )
  {
      s_dontConnect = new QCStringList;
      // The ones below have an intermediate slot because we do
      // some konqy stuff before calling the view
      s_dontConnect->append( "cut" );
      s_dontConnect->append( "copy" );
      s_dontConnect->append( "pastecut" );
      s_dontConnect->append( "pastecopy" );
  }

  ActionSlotMap::ConstIterator it = s_actionSlotMap->begin();
  ActionSlotMap::ConstIterator itEnd = s_actionSlotMap->end();

  QStrList slotNames =  ext->metaObject()->slotNames();

  for ( ; it != itEnd ; ++it )
  {
    QCString actionName = it.key();
    if (actionName == "pastecut" || actionName == "pastecopy")
      actionName = "paste";
    KAction * act = actionCollection()->action( actionName );
    //kdDebug(1202) << actionName << endl;
    assert(act);
    bool enable = false;
    // Does the extension have a slot with the name of this action ?
    if ( slotNames.contains( it.key()+"()" ) )
    {
      if ( ! s_dontConnect->contains( it.key() ) )
        connect( act, SIGNAL( activated() ), ext, it.data() /* SLOT(slot name) */ );
      enable = true;
    }
    act->setEnabled( enable );
  }

  /*
  // Loop over standard action names
  for ( unsigned int i = 0 ; i < sizeof(s_actionnames)/sizeof(char*) ; i++ )
  {
    //kdDebug(1202) << s_actionnames[i] << endl;
    KAction * act = actionCollection()->action( s_actionnames[i] );
    assert(act);
    QCString slotName = QCString(s_actionnames[i])+"()";
    bool enable = false;
    // Does the extension have a slot with the name of this action ?
    if ( slotNames.contains( slotName ) )
    {
      // Connect ? (see comment about most actions)
      if ( !strcmp( s_actionnames[i], "print" )
        || !strcmp( s_actionnames[i], "saveLocalProperties" )
        || !strcmp( s_actionnames[i], "savePropertiesAsDefault" ) )
      {
        // OUCH. Here we use the fact that qobjectdefs.h:#define SLOT(a) "1"#a
        ext->connect( act, SIGNAL( activated() ), ext, QCString("1") + slotName );
        //kdDebug(1202) << "Connecting to " << s_actionnames[i] << endl;
        enable = true;
      }
    }
    act->setEnabled( enable );
  }
  */
  connect( ext, SIGNAL( enableAction( const char *, bool ) ),
           this, SLOT( slotEnableAction( const char *, bool ) ) );
}

void KonqMainView::disconnectExtension( KParts::BrowserExtension *ext )
{
  //kdDebug(1202) << "Disconnecting extension" << endl;
  QValueList<KAction *> actions = actionCollection()->actions();
  QValueList<KAction *>::ConstIterator it = actions.begin();
  QValueList<KAction *>::ConstIterator end = actions.end();
  for (; it != end; ++it )
    (*it)->disconnect( ext );
  disconnect( ext, SIGNAL( enableAction( const char *, bool ) ),
           this, SLOT( slotEnableAction( const char *, bool ) ) );
}

void KonqMainView::slotEnableAction( const char * name, bool enabled )
{
  // Hmm, we have one action for paste, not two...
  QCString hackName( name );
  if ( hackName == "pastecopy" || hackName == "pastecut" )
    hackName = "paste";

  KAction * act = actionCollection()->action( hackName.data() );
  if (!act)
    kdWarning(1202) << "Unknown action " << hackName.data() << " - can't enable" << endl;
  else
    act->setEnabled( enabled );
}

void KonqMainView::enableAllActions( bool enable )
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
  }
  actionCollection()->action( "close" )->setEnabled( true );
}

void KonqMainView::openBookmarkURL( const QString & url )
{
  kdDebug(1202) << (QString("KonqMainView::openBookmarkURL(%1)").arg(url)) << endl;
  openURL( 0L, KURL( url ) );
}

void KonqMainView::setCaption( const QString &caption )
{
  // Keep an unmodified copy of the caption (before kapp->makeStdCaption is applied)
  m_title = caption;
  KParts::MainWindow::setCaption( caption );
}

QString KonqMainView::currentURL()
{
  assert( m_currentView );
  return m_currentView->view()->url().url();
}

void KonqMainView::slotPopupMenu( const QPoint &_global, const KURL &url, const QString &_mimeType, mode_t _mode )
{
  slotPopupMenu( 0L, _global, url, _mimeType, _mode );
}

void KonqMainView::slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KURL &url, const QString &_mimeType, mode_t _mode )
{
  KFileItem item( url, _mimeType, _mode );
  KFileItemList items;
  items.append( &item );
  slotPopupMenu( client, _global, items ); //BE CAREFUL WITH sender() !
}

void KonqMainView::slotPopupMenu( const QPoint &_global, const KFileItemList &_items )
{
  slotPopupMenu( 0L, _global, _items );
}

void KonqMainView::slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KFileItemList &_items )
{
  m_oldView = m_currentView;

  m_currentView = childView( (KParts::ReadOnlyPart *)sender()->parent() );

  //kdDebug(1202) << "KonqMainView::slotPopupMenu(...)" << endl;

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

  m_currentView = m_oldView;
}

void KonqMainView::slotOpenEmbedded()
{
  QCString name = sender()->name();

  m_popupService = m_popupEmbeddingServices[ name.toInt() ]->name();

  m_popupEmbeddingServices.clear();

  QTimer::singleShot( 0, this, SLOT( slotOpenEmbeddedDoIt() ) );
}

void KonqMainView::slotOpenEmbeddedDoIt()
{
  (void) m_currentView->changeViewMode( m_popupServiceType,
					m_popupService,
                                        m_popupURL,
					m_popupURL.url() );
}

void KonqMainView::slotDatabaseChanged()
{
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    callExtensionMethod( (*it), "refreshMimeTypes()" );
}

void KonqMainView::slotReconfigure()
{
  reparseConfiguration();
}

void KonqMainView::reparseConfiguration()
{
  kdDebug(1202) << "KonqMainView::reparseConfiguration() !" << endl;
  kapp->config()->reparseConfiguration();
  KonqFMSettings::reparseConfiguration();

  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    callExtensionMethod( (*it), "reparseConfiguration()" );
}

void KonqMainView::saveProperties( KConfig *config )
{
  m_pViewManager->saveViewProfile( *config );
}

void KonqMainView::readProperties( KConfig *config )
{
  enableAllActions( true );
  m_pViewManager->loadViewProfile( *config );
}

void KonqMainView::setInitialFrameName( const QString &name )
{
  m_initialFrameName = name;
}

#include "konq_mainview.moc"
