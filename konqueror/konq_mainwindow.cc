/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

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
#include <konq_pixmapprovider.h>
#include <konq_faviconmgr.h>
#include <konq_operations.h>


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
#include <qhbox.h>

#include <dcopclient.h>
#include <kaboutdata.h>
#include <kaction.h>
#include <kapp.h>
#include <kbookmarkbar.h>
#include <kbookmarkmenu.h>
#include <kbookmarkmanager.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kedittoolbar.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kkeydialog.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <knewmenu.h>
#include <konq_defaults.h>
#include <konq_dirpart.h>
#include <konq_historymgr.h>
#include <konq_popupmenu.h>
#include <konq_settings.h>
#include <konq_main.h>
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
#include <kurlrequester.h>
#include <kuserprofile.h>
#include <kwin.h>
#include <kfile.h>
#include <kfiledialog.h>

#include "version.h"

template class QList<QPixmap>;
template class QList<KToggleAction>;

QList<KonqMainWindow> *KonqMainWindow::s_lstViews = 0;
KConfig * KonqMainWindow::s_comboConfig = 0;
KCompletion * KonqMainWindow::s_pCompletion = 0;

KonqMainWindow::KonqMainWindow( const KURL &initialURL, bool openInitialURL, const char *name )
 : KParts::MainWindow( name, WDestructiveClose | WStyle_ContextHelp )
{
  if ( !s_lstViews )
    s_lstViews = new QList<KonqMainWindow>;

  s_lstViews->append( this );

  m_currentView = 0L;
  m_initialKonqRun = 0L;
  m_pBookmarkMenu = 0L;
  m_dcopObject = 0L;
  m_combo = 0L;
  m_bURLEnterLock = false;
  m_bLocationBarConnected = false;
  m_bLockLocationBarURL = false;
  m_paBookmarkBar = 0L;
  m_pURLCompletion = 0L;
  m_bFullScreen = false;
  m_accel = new KAccel(this);
  m_goBuffer = 0;

  m_bViewModeToggled = false;

  m_pViewManager = new KonqViewManager( this );

  m_toggleViewGUIClient = new ToggleViewGUIClient( this );

  m_openWithActions.setAutoDelete( true );
  m_viewModeActions.setAutoDelete( true );
  m_toolBarViewModeActions.setAutoDelete( true );
  m_viewModeMenu = 0;
  m_paCopyFiles = 0L;
  m_paMoveFiles = 0L;

  initActions();

  setInstance( KGlobal::instance() );

  connect( KSycoca::self(), SIGNAL( databaseChanged() ),
           this, SLOT( slotDatabaseChanged() ) );

  connect( kapp, SIGNAL( kdisplayFontChanged()), SLOT(slotReconfigure()));

  setXMLFile( "konqueror.rc" );

  KConfig *config = KGlobal::config();

  // init history-manager, load history, get completion object
  if ( !s_pCompletion ) {
    KonqHistoryManager *mgr = new KonqHistoryManager( kapp, "history mgr" );
    s_pCompletion = mgr->completionObject();

    // setup the completion object before createGUI(), so that the combo
    // picks up the correct mode from the HistoryManager (in slotComboPlugged)
    KConfigGroupSaver cs( config, QString::fromLatin1("Settings") );
    int mode = config->readNumEntry( "CompletionMode",
				     KGlobalSettings::completionMode() );
    s_pCompletion->setCompletionMode( (KGlobalSettings::Completion) mode );
  }

  KonqPixmapProvider *prov = KonqPixmapProvider::self();
  if ( !s_comboConfig ) {
      s_comboConfig = new KConfig( "konq_history", false, false );
      KonqCombo::setConfig( s_comboConfig );
      s_comboConfig->setGroup( "Location Bar" );
      prov->load( s_comboConfig, "ComboIconCache" );
  }
  connect( prov, SIGNAL( changed() ), SLOT( slotIconsChanged() ) );

  createGUI( 0L );

  if ( !m_toggleViewGUIClient->empty() )
    plugActionList( QString::fromLatin1( "toggleview" ), m_toggleViewGUIClient->actions() );
  else
  {
    delete m_toggleViewGUIClient;
    m_toggleViewGUIClient = 0;
  }

  m_bNeedApplyKonqMainWindowSettings = true;
  if ( !initialURL.isEmpty() )
  {
    openFilteredURL( initialURL.url() );
  }
  else if ( openInitialURL )
  {
    KURL homeURL;
    homeURL.setPath( QDir::homeDirPath() );
    openURL( 0L, homeURL );
  }
  else
      // silent
      m_bNeedApplyKonqMainWindowSettings = false;

  // Read basic main-view settings, and set to autosave
  setAutoSaveSettings( "KonqMainWindow", false );

  m_paShowMenuBar->setChecked( !menuBar()->isHidden() );
  KToolBar *tb = toolBarByName("mainToolBar");
  if (tb) m_paShowToolBar->setChecked( !tb->isHidden() );
    else m_paShowToolBar->setEnabled(false);
  tb = toolBarByName("extraToolBar");
  if (tb) m_paShowExtraToolBar->setChecked( !tb->isHidden() );
    else m_paShowExtraToolBar->setEnabled(false);
  tb = toolBarByName("locationToolBar");
  if (tb) m_paShowLocationBar->setChecked( !tb->isHidden() );
    else m_paShowLocationBar->setEnabled(false);
  tb = toolBarByName("bookmarkToolBar");
  if (tb) m_paShowBookmarkBar->setChecked( !tb->isHidden() );
    else m_paShowBookmarkBar->setEnabled(false);
  updateBookmarkBar(); // hide if empty

  KConfigGroupSaver cgs(config,"MainView Settings");
  m_bSaveViewPropertiesLocally = config->readBoolEntry( "SaveViewPropertiesLocally", false );
  m_paSaveViewPropertiesLocally->setChecked( m_bSaveViewPropertiesLocally );
  m_bHTMLAllowed = config->readBoolEntry( "HTMLAllowed", false );
  m_ptaUseHTML->setChecked( m_bHTMLAllowed );
  m_sViewModeForDirectory = config->readEntry( "ViewMode" );

  KonqUndoManager::incRef();

  connect( KonqUndoManager::self(), SIGNAL( undoAvailable( bool ) ),
           this, SLOT( slotUndoAvailable( bool ) ) );

  if ( !initialGeometrySet() )
      resize( 700, 480 );
  //kdDebug(1202) << "KonqMainWindow::KonqMainWindow " << this << " done" << endl;
}

KonqMainWindow::~KonqMainWindow()
{
  //kdDebug(1202) << "KonqMainWindow::~KonqMainWindow " << this << endl;
  if ( s_lstViews )
  {
    s_lstViews->removeRef( this );
    if ( s_lstViews->count() == 0 )
    {
      delete s_lstViews;
      s_lstViews = 0;
    }
  }

  disconnect( actionCollection(), SIGNAL( actionStatusText( const QString &) ),
              this, SLOT( slotActionStatusText( const QString & ) ) );
  disconnect( actionCollection(), SIGNAL( clearStatusText() ),
              this, SLOT( slotClearStatusText() ) );

  saveToolBarServicesMap();

  delete m_pViewManager;

  //  createShellGUI( false );

  delete m_pBookmarkMenu;
  delete m_bookmarksActionCollection;
  delete m_pURLCompletion;

  m_viewModeActions.clear();

  KonqUndoManager::decRef();

  if ( s_lstViews == 0 )
      delete KonqPixmapProvider::self();

  //kdDebug(1202) << "KonqMainWindow::~KonqMainWindow " << this << " done" << endl;
}

QWidget * KonqMainWindow::createContainer( QWidget *parent, int index, const QDomElement &element, int &id )
{
  static QString nameBookmarkBar = QString::fromLatin1( "bookmarkToolBar" );
  static QString tagToolBar = QString::fromLatin1( "ToolBar" );

  QWidget *res = KParts::MainWindow::createContainer( parent, index, element, id );

  if ( element.tagName() == tagToolBar && element.attribute( "name" ) == nameBookmarkBar )
  {
    assert( res->inherits( "KToolBar" ) );

    m_paBookmarkBar = new KBookmarkBar( this, static_cast<KToolBar *>( res ), actionCollection(), this );
  }

  return res;
}

void KonqMainWindow::removeContainer( QWidget *container, QWidget *parent, QDomElement &element, int id )
{
  static QString nameBookmarkBar = QString::fromLatin1( "bookmarkToolBar" );
  static QString tagToolBar = QString::fromLatin1( "ToolBar" );

  if ( element.tagName() == tagToolBar && element.attribute( "name" ) == nameBookmarkBar )
  {
    assert( container->inherits( "KToolBar" ) );

    m_paBookmarkBar->clear();
  }

  KParts::MainWindow::removeContainer( container, parent, element, id );
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
    KURL filteredURL = KonqMisc::konqFilteredURL( this, url, m_currentDir );
    kdDebug(1202) << "url " << url << " filtered into " << filteredURL.url() << endl;

    kdDebug() << "KonqMainWindow::openFilteredURL" << endl;

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

  if ( url.url() == "about:blank" )
  {
    m_pViewManager->clear();
    return;
  }

  if ( url.isMalformed() )
  {
      KMessageBox::error(0, i18n("Malformed URL\n%1").arg(url.url()));
      return;
  }
  if ( !KProtocolInfo::isKnownProtocol( url.protocol() ) && url.protocol() != "about" )
  {
      KMessageBox::error(0, i18n("Protocol not supported\n%1").arg(url.protocol()));
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
  if ( ( !serviceType.isEmpty() && serviceType != "application/octet-stream") || url.protocol() == "about" )
  {
    // Built-in view ?
    if ( !openView( serviceType, url, view /* can be 0L */, req ) )
    {
        //kdDebug(1202) << "KonqMainWindow::openURL : openView returned false" << endl;
        // Are we following another view ? Then forget about this URL. Otherwise fire app.
        if ( !req.followMode )
        {
            //kdDebug(1202) << "KonqMainWindow::openURL : we were not following. Fire app." << endl;
            // We know the servicetype, let's try its preferred service
            KService::Ptr offer = KServiceTypeProfile::preferredService(serviceType, true);
            // Remote URL: save or open ?
            if ( url.isLocalFile() || !KonqRun::askSave( url, offer ) )
            {
                KURL::List lst;
                lst.append(url);
                //kdDebug(1202) << "Got offer " << (offer ? offer->name().latin1() : "0") << endl;
                if ( ( trustedSource || KonqRun::allowExecution( serviceType, url ) ) &&
                     ( KonqRun::isExecutable( serviceType ) || !offer || !KRun::run( *offer, lst ) ) )
                {
                    (void)new KRun( url );
                }
            }
        }
    }
  }
  else // no known serviceType, use KonqRun
  {
      kdDebug(1202) << "Creating new konqrun for " << url.url() << " req.typedURL=" << req.typedURL << endl;
      KonqRun * run = new KonqRun( this, view /* can be 0L */, url, req, trustedSource );
      if ( view )
        view->setRun( run );
      else
      {
        if (m_initialKonqRun)
            delete m_initialKonqRun; // there can be only one :)
        m_initialKonqRun = run;
      }
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
  if ( KonqRun::isExecutable( serviceType ) )
     return false; // execute, don't open
  // Contract: the caller of this method should ensure the view is stopped first.

  kdDebug(1202) << "KonqMainWindow::openView " << serviceType << " " << _url.url() << " " << childView << endl;
  kdDebug(1202) << "req.followMode=" << req.followMode << endl;
  kdDebug(1202) << "req.nameFilter= " << req.nameFilter << endl;
  kdDebug(1202) << "req.typedURL= " << req.typedURL << endl;


  bool bOthersFollowed = false;
  // If linked view and if we are not already following another view
  if ( childView && childView->isLinkedView() && !req.followMode && !m_pViewManager->isLoadingProfile() )
    bOthersFollowed = makeViewsFollow( _url, req.args, serviceType, childView );

  if ( childView && childView->isLockedLocation() )
    return bOthersFollowed;

  QString indexFile;

  KURL url( _url );

  //////////// Tar files support

  if ( url.isLocalFile() // kio_tar only supports local files
       && ( serviceType == QString::fromLatin1("application/x-tar")  ||
            serviceType == QString::fromLatin1("application/x-tgz")  ||
            serviceType == QString::fromLatin1("application/x-tbz") ) )
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
  {
    if (originalURL.right(1) != "/")
        originalURL += '/';
    originalURL += req.nameFilter;
  }

  QString serviceName; // default: none provided

  if ( url.url() == "about:konqueror" )
  {
      serviceType = "KonqAboutPage"; // not KParts/ReadOnlyPart, it fills the Location menu ! :)
      serviceName = "konq_aboutpage";
      originalURL = req.typedURL.isEmpty() ? QString::null : QString::fromLatin1("about:konqueror");
      // empty if from profile, about:konqueror if the user typed it (not req.typedURL, it could be "about:")
  }

  // Look for which view mode to use, if a directory - not if view locked
  if ( ( !childView || (!childView->isLockedViewMode()) )
       && serviceType == "inode/directory" )
    { // Phew !

      // Set view mode if necessary (current view doesn't support directories)
      if ( !childView || !childView->supportsServiceType( serviceType ) )
        serviceName = m_sViewModeForDirectory;
      kdDebug(1202) << "serviceName=" << serviceName << " m_sViewModeForDirectory=" << m_sViewModeForDirectory << endl;

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
              serviceName = config.readEntry( "ViewMode", serviceName );
              kdDebug(1202) << "serviceName=" << serviceName << endl;
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
          KMessageBox::sorry( 0L, i18n( "Could not create view for %1\nThe diagnostics is:\n%2").arg(serviceType)
              .arg(KLibLoader::self()->lastErrorMessage()) );
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
      if ( childView->browserExtension() )
          childView->browserExtension()->setURLArgs( req.args );
      if ( !url.isEmpty() )
	  childView->openURL( url, originalURL, req.nameFilter );
    }
  kdDebug(1202) << "KonqMainWindow::openView ok=" << ok << " bOthersFollowed=" << bOthersFollowed << " returning " << (ok || bOthersFollowed) << endl;
  return ok || bOthersFollowed;
}

void KonqMainWindow::slotOpenURLRequest( const KURL &url, const KParts::URLArgs &args )
{
  kdDebug(1202) << "KonqMainWindow::slotOpenURLRequest" << endl;

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
  kdDebug(1202) << "KonqMainWindow::openURL (from slotOpenURLRequest) url=" << url.prettyURL() << endl;
  KonqOpenURLRequest req;
  req.args = args;

  if ( args.postData.size() > 0 ) // todo merge in if statement below
  {
    kdDebug(1202) << "KonqMainWindow::openURL - with postData. serviceType=" << args.serviceType << endl;
    openURL( childView, url, args.serviceType, req, args.trustedSource );
    return;
  }

  if ( !args.reload && urlcmp( url.url(), childView->url().url(), true, true ) )
  {
    QString serviceType = args.serviceType;
    if ( serviceType.isEmpty() )
      serviceType = childView->serviceType();

    childView->stop();
    openView( serviceType, url, childView, req );
    return;
  }

  openURL( childView, url, args.serviceType, req, args.trustedSource );
}

// Linked-views feature
bool KonqMainWindow::makeViewsFollow( const KURL & url, const KParts::URLArgs &args,
                                      const QString & serviceType, KonqView * senderView )
{
  bool res = false;

  kdDebug(1202) << "makeViewsFollow " << senderView->className() << " url=" << url.url() << " serviceType=" << serviceType << endl;
  KonqOpenURLRequest req;
  req.followMode = true;
  req.args = args;
  // We can't iterate over the map here, and openURL for each, because the map can get modified
  // (e.g. by part changes). Better copy the views into a list.
  QList<KonqView> listViews;
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    listViews.append( it.data() );

  for ( KonqView * view = listViews.first() ; view ; view = listViews.next() )
  {
    // Views that should follow this URL as views that are linked
    // (we are here only if the view opening a URL initially is linked)
    if ( (view != senderView) && view->isLinkedView() )
    {
      kdDebug(1202) << "makeViewsFollow: Sending openURL to view " << view->part()->className() << " url=" << url.url() << endl;

      // XXX duplicate code from ::openURL
      if ( view == m_currentView )
      {
          abortLoading();
          setLocationBarURL( url.prettyURL() );
      }
      else
          view->stop();

      res = openView( serviceType, url, view, req ) || res;
    }
  }

  return res;
}

void KonqMainWindow::abortLoading()
{
  kdDebug(1202) << "KonqMainWindow::abortLoading()" << endl;
  if ( m_currentView )
  {
    m_currentView->stop(); // will take care of the statusbar
    stopAnimation();
  }
}

void KonqMainWindow::slotCreateNewWindow( const KURL &url, const KParts::URLArgs &args )
{
    kdDebug(1202) << "KonqMainWindow::slotCreateNewWindow url=" << url.prettyURL() << endl;
    KonqMisc::createNewWindow( url, args );
}

void KonqMainWindow::slotCreateNewWindow( const KURL &url, const KParts::URLArgs &args,
                                          const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart *&part )
{
    kdDebug(1202) << "KonqMainWindow::slotCreateNewWindow(4 args) url=" << url.prettyURL()
                  << " args.serviceType=" << args.serviceType
                  << " args.frameName=" << args.frameName << endl;

    KonqMainWindow *mainWindow = 0L;
    KonqView * view = 0L;
    if ( !args.frameName.isEmpty() && args.frameName != "_blank" ) // TODO _parent and _top (?)
    {
        KParts::BrowserHostExtension *hostExtension = 0;
        view = findChildView( args.frameName, &mainWindow, &hostExtension );
        kdDebug() << " frame=" << args.frameName << " -> found view=" << view << endl;
        if ( view )
        {
            // Found a view. If url isn't empty, we should open it - but this never happens currently
            part = view->part();
            return;
        }
    }

    mainWindow = new KonqMainWindow( KURL(), false );
    mainWindow->setInitialFrameName( args.frameName );

    KonqOpenURLRequest req;
    req.args = args;

    if ( !mainWindow->openView( args.serviceType, url, 0L, req ) )
    {
	// we have problems. abort.
	delete mainWindow;
	part = 0;
	return;
    }

    mainWindow->show();

    // cannot use activePart/currentView, because the activation through the partmanager
    // is delayed by a singleshot timer (see KonqViewManager::setActivePart)
    MapViews::ConstIterator it = mainWindow->viewMap().begin();
    view = it.data();
    part = it.key();

    // activate the view _now_ in order to make the menuBar() hide call work
    if ( part )
        mainWindow->viewManager()->setActivePart( part, true );

    KSimpleConfig cfg( locate( "data", QString::fromLatin1( "konqueror/profiles/webbrowsing" ) ), true );
    cfg.setGroup( "Profile" );
    QSize size = KonqViewManager::readConfigSize( cfg );

    if ( windowArgs.x != -1 )
        mainWindow->move( windowArgs.x, mainWindow->y() );
    if ( windowArgs.y != -1 )
        mainWindow->move( mainWindow->x(), windowArgs.y );

    int width;
    if ( windowArgs.width != -1 )
        width = windowArgs.width;
    else
        width = size.isValid() ? size.width() : mainWindow->width();

    int height;
    if ( windowArgs.height != -1 )
        height = windowArgs.height;
    else
        height = size.isValid() ? size.height() : mainWindow->height();

    mainWindow->resize( width, height );

    // process the window args

    if ( !windowArgs.menuBarVisible )
    {
        mainWindow->menuBar()->hide();
        mainWindow->m_paShowMenuBar->setChecked( false );
    }

    if ( !windowArgs.toolBarsVisible )
    {
        // Maybe we could get all KToolBar children at once ?
        static_cast<KToolBar *>( mainWindow->child( "mainToolBar", "KToolBar" ) )->hide();
        static_cast<KToolBar *>( mainWindow->child( "extraToolBar", "KToolBar" ) )->hide();
        static_cast<KToolBar *>( mainWindow->child( "locationToolBar", "KToolBar" ) )->hide();
        static_cast<KToolBar *>( mainWindow->child( "bookmarkToolBar", "KToolBar" ) )->hide();

        mainWindow->m_paShowToolBar->setChecked( false );
        mainWindow->m_paShowLocationBar->setChecked( false );
        mainWindow->m_paShowBookmarkBar->setChecked( false );
        mainWindow->m_paShowExtraToolBar->setChecked( false );
    }

    if ( view && !windowArgs.statusBarVisible )
        view->frame()->statusbar()->hide();

    if ( !windowArgs.resizable )
        // ### this doesn't seem to work :-(
        mainWindow->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );

    if ( windowArgs.fullscreen )
        mainWindow->action( "fullscreen" )->activate();
}

void KonqMainWindow::slotNewWindow()
{
  // Use profile from current window, if set
  QString profile = m_pViewManager->currentProfile();
  if ( profile.isEmpty() )
  {
    if ( m_currentView && m_currentView->url().protocol() == QString::fromLatin1( "http" ) )
       profile = QString::fromLatin1("webbrowsing");
    else
       profile = QString::fromLatin1("filemanagement");
  }
  KonqMisc::createBrowserWindowFromProfile(
    locate( "data", QString::fromLatin1("konqueror/profiles/")+profile ),
    profile );
}

void KonqMainWindow::slotDuplicateWindow()
{
  KTempFile tempFile;
  tempFile.setAutoDelete( true );
  KConfig config( tempFile.name() );
  config.setGroup( "View Profile" );
  m_pViewManager->saveViewProfile( config, true, true );

  KonqMainWindow *mainWindow = new KonqMainWindow( QString::null, false );
  mainWindow->viewManager()->loadViewProfile( config, m_pViewManager->currentProfile() );
  if (mainWindow->currentView())
  {
      mainWindow->viewManager()->mainContainer()->copyHistory( m_pViewManager->mainContainer() );
  }
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

  if ( m_currentView )
  {
      KURL u( m_currentView->url() );
      if ( u.isLocalFile() )
          if ( m_currentView->serviceType() == "inode/directory" )
              dir = u.path();
          else
              dir = u.directory();
  }

  QString cmd = QString("cd \"%1\" ; %2 &").arg( dir ).arg( term );
  kdDebug(1202) << "slotOpenTerminal: " << cmd << endl;
  system( QFile::encodeName(cmd) );
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
  if ( m_currentView && m_currentView->part()->inherits("KonqDirPart") )
  {
    KonqDirPart * dirPart = static_cast<KonqDirPart *>(m_currentView->part());

    //KonqView * findView = m_pViewManager->splitView( Vertical, "Konqueror/FindPart", QString::null, true /*make it on top*/ );

    KonqViewFactory factory = KonqFactory::createView( "Konqueror/FindPart" );
    if ( factory.isNull() )
    {
        KMessageBox::error( this, i18n("Can't create the find part, check your installation.") );
        return;
    }
    KParts::ReadOnlyPart * findPart = factory.create( m_currentView->frame(), "findPartWidget", dirPart, "findPart" );
    dirPart->setFindPart( findPart );

    m_currentView->frame()->insertTopWidget( findPart->widget() );
    findPart->widget()->show();

    connect( dirPart, SIGNAL( findClosed(KonqDirPart *) ),
             this, SLOT( slotFindClosed(KonqDirPart *) ) );

    // Don't allow to do this twice for this view :-)
    m_paFindFiles->setEnabled(false);
  }
  else
  {
      KonqMainWindow * mw = KonqMisc::createBrowserWindowFromProfile(
          locate( "data", QString::fromLatin1("konqueror/profiles/filemanagement") ),
          "filemanagement" );
      mw->slotToolFind();
  }

  /* Old version
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
  */
}

void KonqMainWindow::slotFindOpen( KonqDirPart * dirPart )
{
    kdDebug(1202) << "KonqMainWindow::slotFindOpen " << dirPart << endl;
    ASSERT( m_currentView );
    ASSERT( m_currentView->part() == dirPart );
    slotToolFind(); // lazy me
}

void KonqMainWindow::slotFindClosed( KonqDirPart * dirPart )
{
    kdDebug(1202) << "KonqMainWindow::slotFindClosed " << dirPart << endl;
    KonqView * dirView = m_mapViews.find( dirPart ).data();
    ASSERT(dirView);
    kdDebug(1202) << "dirView=" << dirView << endl;
    if ( dirView && dirView == m_currentView )
        m_paFindFiles->setEnabled( true );
}

void KonqMainWindow::slotIconsChanged()
{
    if ( m_combo )
	m_combo->updatePixmaps();
//  const QPixmap &pix = KonqPixmapProvider::self()->pixmapFor( currentText );
//  topLevelWidget()->setIcon( pix );
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
    if ( (*it)->desktopEntryName() == serviceName )
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

  if ( m_currentView->service()->desktopEntryName() == modeName )
    return;

  m_bViewModeToggled = true;

  m_currentView->stop();
  m_currentView->lockHistory();

  // Save those, because changeViewMode will lose them
  KURL url = m_currentView->url();
  QString locationBarURL = m_currentView->locationBarURL();

  bool bQuickViewModeChange = false;

  // iterate over all services, update the toolbar service map
  // and check if we can do a quick property-based viewmode change
  KTrader::OfferList offers = m_currentView->partServiceOffers();
  KTrader::OfferList::ConstIterator oIt = offers.begin();
  KTrader::OfferList::ConstIterator oEnd = offers.end();
  for (; oIt != oEnd; ++oIt )
  {
      KService::Ptr service = *oIt;

      if ( service->desktopEntryName() == modeName )
      {
          // we changed the viewmode of either iconview or listview
          // -> update the service in the corresponding map, so that
          // we can set the correct text, icon, etc. properties to the
          // KonqViewModeAction when rebuilding the view-mode actions in
          // updateViewModeActions
          // (I'm saying iconview/listview here, but theoretically it could be
          //  any view :)
          m_viewModeToolBarServices[ service->library() ] = service;

          if (  service->library() == m_currentView->service()->library() )
          {
              QVariant modeProp = service->property( "X-KDE-BrowserView-ModeProperty" );
              QVariant modePropValue = service->property( "X-KDE-BrowserView-ModePropertyValue" );
              if ( !modeProp.isValid() || !modePropValue.isValid() )
                  break;

              m_currentView->part()->setProperty( modeProp.toString().latin1(), modePropValue );

              KService::Ptr oldService = m_currentView->service();

              // we aren't going to re-build the viewmode actions but instead of a
              // quick viewmode change (iconview) -> find the iconview-konqviewmode
              // action and set new text,icon,etc. properties, to show the new
              // current viewmode
              QListIterator<KAction> it( m_toolBarViewModeActions );
              for (; it.current(); ++it )
                  if ( QString::fromLatin1( it.current()->name() ) == oldService->desktopEntryName() )
                  {
                      assert( it.current()->inherits( "KonqViewModeAction" ) );

                      KonqViewModeAction *action = static_cast<KonqViewModeAction *>( it.current() );

                      action->setChecked( true );
                      action->setText( service->name() );
                      action->setIcon( service->icon() );
                      action->setName( service->desktopEntryName().ascii() );

                      break;
                  }

              m_currentView->setService( service );

              bQuickViewModeChange = true;
              break;
          }
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
      kdDebug(1202) << "m_sViewModeForDirectory=" << m_sViewModeForDirectory << endl;
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
      KURL u ( b ? m_currentView->url() : KURL( m_currentView->url().directory() ) );
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

void KonqMainWindow::slotUnlockView()
{
  ASSERT(m_currentView->isLockedLocation());
  m_currentView->setLockedLocation( false );
  m_paLockView->setEnabled( true );
  m_paUnlockView->setEnabled( false );
}

void KonqMainWindow::slotLockView()
{
  ASSERT(!m_currentView->isLockedLocation());
  m_currentView->setLockedLocation( true );
  m_paLockView->setEnabled( false );
  m_paUnlockView->setEnabled( true );
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
  openURL( 0L, KURL( KonqMisc::konqFilteredURL( this, KonqFMSettings::settings()->homeURL() ) ) );
}

void KonqMainWindow::slotGoApplications()
{
  KonqMisc::createSimpleWindow( KGlobal::dirs()->saveLocation("apps") );
}

void KonqMainWindow::slotGoDirTree()
{
  KonqMisc::createSimpleWindow( locateLocal( "data", "konqueror/dirtree/" ) );
}

void KonqMainWindow::slotGoTrash()
{
  KonqMisc::createSimpleWindow( KGlobalSettings::trashPath() );
}

void KonqMainWindow::slotGoTemplates()
{
  KonqMisc::createSimpleWindow( KGlobal::dirs()->resourceDirs("templates").last() );
}

void KonqMainWindow::slotGoAutostart()
{
  KonqMisc::createSimpleWindow( KGlobalSettings::autostartPath() );
}

void KonqMainWindow::slotConfigure()
{
  kapp->startServiceByDesktopName("konqueror_config");
}

void KonqMainWindow::slotConfigureKeys()
{
  KKeyDialog::configureKeys(actionCollection(), xmlFile());
}

void KonqMainWindow::slotConfigureToolbars()
{
  QString savedURL = m_combo ? m_combo->currentText() : QString::null;
  KEditToolbar edit(factory());
  if ( edit.exec() )
  {
    if ( m_toggleViewGUIClient )
      plugActionList( QString::fromLatin1( "toggleview" ), m_toggleViewGUIClient->actions() );
    if ( m_currentView && m_currentView->appServiceOffers().count() > 0 )
      plugActionList( "openwith", m_openWithActions );

    plugViewModeActions();

    applyMainWindowSettings( KGlobal::config(), "KonqMainWindow" );
    //updateBookmarkBar();

    if ( m_combo )
      m_combo->setTemporary(savedURL);
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
  kdDebug(1202) << "KonqMainWindow::slotPartChanged" << endl;
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

  if ( run == m_initialKonqRun )
      m_initialKonqRun = 0L;

  KonqView *childView = run->childView();

  // Check if we found a mimetype _and_ we got no error (example: cancel in openwith dialog)
  if ( run->foundMimeType() && !run->hasError() )
  {

    // We do this here and not in the constructor, because
    // we are waiting for the first view to be set up before doing this...
    // Note: this is only used when konqueror is started from command line.....
    if ( m_bNeedApplyKonqMainWindowSettings )
    {
      m_bNeedApplyKonqMainWindowSettings = false; // only once
      applyKonqMainWindowSettings();
    }

    return;
  }

  if ( childView )
  {
    childView->setLoading( false );

    if ( childView == m_currentView )
    {
      stopAnimation();

      // Revert to working URL - unless the URL was typed manually
      kdDebug(1202) << " typed URL = " << run->typedURL() << endl;
      if ( run->typedURL().isEmpty() && childView->history().current() ) // not typed
        childView->setLocationBarURL( childView->history().current()->locationBarURL );
    }
  }
}

void KonqMainWindow::applyKonqMainWindowSettings()
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

  if (!m_combo) // happens if removed from .rc file :)
    return;

  QString currentText = m_combo->currentText();
  const QPixmap &pix = KonqPixmapProvider::self()->pixmapFor( currentText );
  topLevelWidget()->setIcon( pix );

  // Need to update the current working directory
  // of the completion object everytime the user
  // changes the directory!! (DA)
  if( m_pURLCompletion )
  {
    KURL u( view->locationBarURL() );
    if( u.isLocalFile() )
      m_pURLCompletion->setDir( u.path() );
    else
      m_pURLCompletion->setDir( u.url() );  //needs work!! (DA)
  }
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

  KParts::BrowserExtension *ext = 0;

  if ( oldView )
  {
    ext = oldView->browserExtension();
    if ( ext )
    {
      //kdDebug(1202) << "Disconnecting extension for view " << oldView << endl;
      disconnectExtension( ext );
    }
  }

  kdDebug(1202) << "New current view " << newView << endl;
  m_currentView = newView;
  if ( !part )
  {
    kdDebug(1202) << "No part activated - returning" << endl;
    unplugViewModeActions();
    createGUI( 0L );
    return;
  }

  ext = m_currentView->browserExtension();

  if ( ext )
  {
    connectExtension( ext );
    createGUI( part );
  }
  else
  {
    kdDebug(1202) << "No Browser Extension for the new part" << endl;
    // Disable all browser-extension actions

    KParts::BrowserExtension::ActionSlotMap * actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
    KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->begin();
    KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->end();

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

  KParts::MainWindow::setCaption( m_currentView->caption() );
  updateOpenWithActions();
  updateLocalPropsActions();
  updateViewActions(); // undo, lock, link and other view-dependent actions

  if ( !m_bViewModeToggled ) // if we just toggled the view mode via the view mode actions, then
                             // we don't need to do all the time-taking stuff below (Simon)
  {
    updateViewModeActions();

    m_pMenuNew->setEnabled( m_currentView->serviceType() == QString::fromLatin1( "inode/directory" ) );
  }

  m_bViewModeToggled = false;

  m_currentView->frame()->statusbar()->repaint();

  if ( oldView )
    oldView->frame()->statusbar()->repaint();

  if ( !m_bLockLocationBarURL )
  {
    //kdDebug(1202) << "slotPartActivated: setting location bar url to "
    //              << m_currentView->locationBarURL() << " m_currentView=" << m_currentView << endl;
    if ( m_combo )
      setLocationBarURL( m_currentView->locationBarURL() );
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

  if ( !m_pViewManager->isLoadingProfile() ) // see KonqViewManager::loadViewProfile
      viewCountChanged();
  emit viewAdded( childView );
}

// Called by KonqViewManager, internal
void KonqMainWindow::removeChildView( KonqView *childView )
{
  kdDebug(1202) << "KonqMainWindow::removeChildView childView " << childView << endl;
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
  kdDebug(1202) << "KonqMainWindow::viewCountChanged" << endl;

  m_paLinkView->setEnabled( viewCount() > 1 );

  // Only one view -> make it unlinked
  if ( viewCount() == 1 )
      m_mapViews.begin().data()->setLinkedView( false );

  viewsChanged();

  m_pViewManager->viewCountChanged();
}

void KonqMainWindow::viewsChanged()
{
  // This is called when the number of views changes OR when
  // the type of some view changes.

  // Nothing here anymore, but don't cleanup, some might come back later.

  updateViewActions(); // undo, lock, link and other view-dependent actions
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

int KonqMainWindow::mainViewsCount() const
{
  int res = 0;
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    if ( !it.data()->isPassiveMode() && !it.data()->isToggleView() )
    {
      //kdDebug(1202) << "KonqMainWindow::mainViewsCount " << res << " " << it.data() << " " << it.data()->part()->widget() << endl;
      ++res;
    }

  return res;
}

KParts::ReadOnlyPart * KonqMainWindow::currentPart() const
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
        updateLocalPropsActions();

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

void KonqMainWindow::updateLocalPropsActions()
{
    bool canWrite = false;
    if ( m_currentView && m_currentView->url().isLocalFile() )
    {
        // Can we write ?
        QFileInfo info( m_currentView->url().path() );
        canWrite = info.isDir() && info.isWritable();
    }
    m_paSaveViewPropertiesLocally->setEnabled( canWrite );
    m_paRemoveLocalProperties->setEnabled( canWrite );
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
}

void KonqMainWindow::slotSplitViewVertical()
{
  KonqView * newView = m_pViewManager->splitView( Qt::Vertical );
  newView->openURL( m_currentView->url(), m_currentView->locationBarURL() );
}

void KonqMainWindow::slotSplitWindowHorizontal()
{
  m_pViewManager->splitWindow( Qt::Horizontal );
}

void KonqMainWindow::slotSplitWindowVertical()
{
  m_pViewManager->splitWindow( Qt::Vertical );
}

void KonqMainWindow::slotRemoveView()
{
  // takes care of choosing the new active view
  m_pViewManager->removeView( m_currentView );
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

bool KonqMainWindow::askForTarget(const QString& text, KURL& url)
{
   KDialog *dlg=new KDialog(this,"blah",true);
   QVBoxLayout *layout=new QVBoxLayout(dlg,dlg->marginHint(),dlg->spacingHint());
   QLabel *label=new QLabel(text,dlg);
   layout->addWidget(label);
   label=new QLabel(m_currentView->url().prettyURL(),dlg);
   layout->addWidget(label);
   label=new QLabel(i18n("to"),dlg);
   layout->addWidget(label);
   QString initialUrl = (viewCount()==2) ? otherView(m_currentView)->url().prettyURL() : m_currentView->url().prettyURL();
   KURLRequester *urlReq=new KURLRequester(initialUrl, dlg);
   connect( urlReq, SIGNAL( openFileDialog( KURLRequester * )),
	    SLOT( slotRequesterClicked( KURLRequester * )));

   layout->addWidget(urlReq);
   QHBox *hbox=new QHBox(dlg);
   layout->addWidget(hbox);
   hbox->setSpacing(dlg->spacingHint());
   QPushButton *ok=new QPushButton(i18n("OK"),hbox);
   QPushButton *cancel=new QPushButton(i18n("Cancel"),hbox);
   connect(ok,SIGNAL(clicked()),dlg,SLOT(accept()));
   connect(cancel,SIGNAL(clicked()),dlg,SLOT(reject()));
   ok->setDefault(true);
   if (dlg->exec()==QDialog::Rejected)
   {
      delete dlg;
      return false;
   }
   url=urlReq->url();
   delete dlg;
   return true;
}

void KonqMainWindow::slotRequesterClicked( KURLRequester *req )
{
    req->fileDialog()->setMode(KFile::Mode(KFile::Directory|KFile::ExistingOnly));
}

void KonqMainWindow::slotCopyFiles()
{
  //kdDebug(1202) << "KonqMainWindow::slotCopyFiles()" << endl;
  KURL dest;
  if (!askForTarget(i18n("Copy selected files from"),dest))
     return;

  KonqDirPart * part = static_cast<KonqDirPart *>(m_currentView->part());
  KURL::List lst;
  KFileItemList tmpList=part->selectedFileItems();
  for (KFileItem *item=tmpList.first(); item!=0; item=tmpList.next())
     lst.append(item->url());

  KonqOperations::copy(this,KonqOperations::COPY,lst,dest);
}

void KonqMainWindow::slotMoveFiles()
{
  //kdDebug(1202) << "KonqMainWindow::slotMoveFiles()" << endl;
  KURL dest;
  if (!askForTarget(i18n("Move selected files from"),dest))
     return;

   KonqDirPart * part = static_cast<KonqDirPart *>(m_currentView->part());
   KURL::List lst;
   KFileItemList tmpList=part->selectedFileItems();

  for (KFileItem *item=tmpList.first(); item!=0; item=tmpList.next())
     lst.append(item->url());

  KonqOperations::copy(this,KonqOperations::MOVE,lst,dest);
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

void KonqMainWindow::slotSaveViewProfile()
{
    if ( m_pViewManager->currentProfile().isEmpty() )
    {
        // The action should be disabled...........
        kdWarning(1202) << "No known profile. Use the Save Profile dialog box" << endl;
    } else {

        m_pViewManager->saveViewProfile( m_pViewManager->currentProfile(),
                                         m_pViewManager->currentProfileText(),
                                         true /* URLs */, true /* size */ );

    }
}

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
    popup->insertItem( KonqPixmapProvider::self()->pixmapFor( u.url() ),
		       u.prettyURL() );

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
  if (!m_goBuffer)
  {
    // Only start 1 timer.
    m_goBuffer = steps;
    QTimer::singleShot( 0, this, SLOT(slotGoHistoryDelayed()));
  }
}

void KonqMainWindow::slotGoHistoryDelayed()
{
  if (!m_goBuffer) return;
  int steps = m_goBuffer;
  m_goBuffer = 0;
  m_currentView->go( steps );
  if ( m_currentView->isLinkedView() )
      // make other views follow
      /*bOthersFollowed = */makeViewsFollow( m_currentView->url(), KParts::URLArgs(),
                                             m_currentView->serviceType(), m_currentView );
}

void KonqMainWindow::slotBackAboutToShow()
{
  m_paBack->popupMenu()->clear();
  if ( m_currentView )
      KonqBidiHistoryAction::fillHistoryPopup( m_currentView->history(), m_paBack->popupMenu(), true, false );
}

void KonqMainWindow::slotBack()
{
  slotGoHistoryActivated(-1);
}

void KonqMainWindow::slotBackActivated( int id )
{
  slotGoHistoryActivated( -(m_paBack->popupMenu()->indexOf( id ) + 1) );
}

void KonqMainWindow::slotForwardAboutToShow()
{
  m_paForward->popupMenu()->clear();
  if ( m_currentView )
      KonqBidiHistoryAction::fillHistoryPopup( m_currentView->history(), m_paForward->popupMenu(), false, true );
}

void KonqMainWindow::slotForward()
{
  slotGoHistoryActivated( 1 );
}

void KonqMainWindow::slotForwardActivated( int id )
{
  slotGoHistoryActivated( m_paForward->popupMenu()->indexOf( id ) + 1 );
}

void KonqMainWindow::slotComboPlugged()
{
  m_combo = m_paURLCombo->combo();

  KAction * act = actionCollection()->action("location_label");
  if (act && act->inherits("KonqLabelAction") )
  {
      QLabel * label = static_cast<KonqLabelAction *>(act)->label();
      if (label)
          label->setBuddy( m_combo );
      else
          kdError() << "Label not constructed yet!" << endl;;
  } else kdError() << "Not a KonqLabelAction !" << endl;;

  m_combo->setCompletionObject( s_pCompletion, false ); //we handle the signals
  m_combo->setAutoDeleteCompletionObject( false );
  m_combo->setCompletionMode( s_pCompletion->completionMode() );

  m_pURLCompletion = new KURLCompletion( KURLCompletion::FileCompletion );
  m_pURLCompletion->setCompletionMode( s_pCompletion->completionMode() );
  // This only turns completion off. ~ is still there in the result
  // We do want completion of user names, right?
  //m_pURLCompletion->setReplaceHome( false );  // Leave ~ alone! Will be taken care of by filters!!

  connect( m_combo,SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
	   SLOT( slotCompletionModeChanged( KGlobalSettings::Completion )));
  connect( m_combo, SIGNAL( completion( const QString& )),
           SLOT( slotMakeCompletion( const QString& )));
  connect( m_combo, SIGNAL( textRotation( KCompletionBase::KeyBindingType) ),
           SLOT( slotRotation( KCompletionBase::KeyBindingType )));

  m_combo->lineEdit()->installEventFilter(this);
}

// the user changed the completion mode in the combo
void KonqMainWindow::slotCompletionModeChanged( KGlobalSettings::Completion m )
{
  s_pCompletion->setCompletionMode( m );
  KConfig *config = KGlobal::config();
  config->setGroup( "Settings" );
  config->writeEntry( "CompletionMode", (int)m_combo->completionMode() );
  config->sync();

  // tell the other windows too (only this instance currently)
  KonqMainWindow *window = s_lstViews->first();
  while ( window ) {
    if ( window->m_combo ) {
      window->m_combo->setCompletionMode( m );
      window->m_pURLCompletion->setCompletionMode( m );
    }
    window = s_lstViews->next();
  }
}

// at first, try to find a completion in the current view, then use the global
// completion (history)
void KonqMainWindow::slotMakeCompletion( const QString& text )
{
  if( m_pURLCompletion )
  {
    // kdDebug(1202) << "Local Completion object found!" << endl;
    QString completion = m_pURLCompletion->makeCompletion( text );
    m_currentDir = QString::null;

    if ( completion.isNull() )
    {
      // ask the global one
      // tell the static completion object about the current completion mode
      completion = s_pCompletion->makeCompletion( text );
    }
    else
    {
      if( !m_pURLCompletion->dir().isEmpty() )
        m_currentDir = m_pURLCompletion->dir();
    }

    // some special handling necessary for CompletionPopup
    if ( m_combo->completionMode() == KGlobalSettings::CompletionPopup ) {
	QStringList items = m_pURLCompletion->allMatches( text );
	items += s_pCompletion->allMatches( text );
	// items.sort(); // should we?
	m_combo->setCompletedItems( items );
    }

    else {
	if ( !completion.isNull() ) {
	    m_combo->setCompletedText( completion );
	}
    }
  }
  // kdDebug(1202) << "Current dir: " << m_currentDir << "  Current text: " << text << endl;
}

void KonqMainWindow::slotRotation( KCompletionBase::KeyBindingType type )
{
  bool prev = (type == KCompletionBase::PrevCompletionMatch);
  if ( prev || type == KCompletionBase::NextCompletionMatch ) {
    QString completion = prev ? m_pURLCompletion->previousMatch() :
                                m_pURLCompletion->nextMatch();

    if( completion.isNull() ) { // try the history KCompletion object
        completion = prev ? s_pCompletion->previousMatch() :
                            s_pCompletion->nextMatch();
    }
    if ( completion.isEmpty() || completion == m_combo->currentText() )
      return;

    m_combo->setCompletedText( completion );
  }
}


bool KonqMainWindow::eventFilter(QObject*obj,QEvent *ev)
{
  if ( ( ev->type()==QEvent::FocusIn || ev->type()==QEvent::FocusOut ) &&
       m_combo && m_combo->lineEdit() == obj )
  {
    kdDebug(1202) << "KonqMainWindow::eventFilter " << obj << " " << obj->className() << " " << obj->name() << endl;

    QFocusEvent * focusEv = static_cast<QFocusEvent*>(ev);
    if (focusEv->reason() == QFocusEvent::Popup)
    {
      return KParts::MainWindow::eventFilter( obj, ev );
    }

    KParts::BrowserExtension * ext = 0;
    if ( m_currentView )
        ext = m_currentView->browserExtension();
    QStrList slotNames;
    if (ext)
      slotNames = ext->metaObject()->slotNames();

    //for ( char * s = slotNames.first() ; s ; s = slotNames.next() )
    //{
    //    kdDebug(1202) << "slotNames=" << s << endl;
    //}

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
      if (slotNames.contains("del()"))
        disconnect( m_paDelete, SIGNAL( activated() ), ext, SLOT( del() ) );
      if (slotNames.contains("trash()"))
        disconnect( m_paTrash, SIGNAL( activated() ), ext, SLOT( trash() ) );
      if (slotNames.contains("shred()"))
        disconnect( m_paShred, SIGNAL( activated() ), ext, SLOT( shred() ) );

      connect( m_paCut, SIGNAL( activated() ), this, SLOT( slotComboCut() ) );
      connect( m_paCopy, SIGNAL( activated() ), this, SLOT( slotComboCopy() ) );
      connect( m_paPaste, SIGNAL( activated() ), this, SLOT( slotComboPaste() ) );
      //connect( m_paDelete, SIGNAL( activated() ), this, SLOT( slotComboDelete() ) );
      connect( m_paTrash, SIGNAL( activated() ), this, SLOT( slotComboDelete() ) );
      //connect( m_paShred, SIGNAL( activated() ), this, SLOT( slotComboDelete() ) );
      connect( QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()) );
      connect( m_combo->lineEdit(), SIGNAL(textChanged(const QString &)), this, SLOT(slotCheckComboSelection()) );

      m_bCutWasEnabled = m_paCut->isEnabled();
      m_bCopyWasEnabled = m_paCopy->isEnabled();
      m_bPasteWasEnabled = m_paPaste->isEnabled();
      m_bDeleteWasEnabled = m_paDelete->isEnabled();
      m_paTrash->setText( i18n("&Delete") ); // Name Delete the action associated with 'del'
      m_paTrash->setEnabled(true);
      m_paDelete->setEnabled(false);
      m_paShred->setEnabled(false);

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
      if (slotNames.contains("del()"))
        connect( m_paDelete, SIGNAL( activated() ), ext, SLOT( del() ) );
      if (slotNames.contains("trash()"))
        connect( m_paTrash, SIGNAL( activated() ), ext, SLOT( trash() ) );
      if (slotNames.contains("shred()"))
        connect( m_paShred, SIGNAL( activated() ), ext, SLOT( shred() ) );

      disconnect( m_paCut, SIGNAL( activated() ), this, SLOT( slotComboCut() ) );
      disconnect( m_paCopy, SIGNAL( activated() ), this, SLOT( slotComboCopy() ) );
      disconnect( m_paPaste, SIGNAL( activated() ), this, SLOT( slotComboPaste() ) );
      //disconnect( m_paDelete, SIGNAL( activated() ), this, SLOT( slotComboDelete() ) );
      disconnect( m_paTrash, SIGNAL( activated() ), this, SLOT( slotComboDelete() ) );
      //disconnect( m_paShred, SIGNAL( activated() ), this, SLOT( slotComboDelete() ) );
      disconnect( QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()) );
      disconnect( m_combo->lineEdit(), SIGNAL(textChanged(const QString &)), this, SLOT(slotClipboardDataChanged()) );

      m_paCut->setEnabled( m_bCutWasEnabled );
      m_paCopy->setEnabled( m_bCopyWasEnabled );
      m_paPaste->setEnabled( m_bPasteWasEnabled );
      m_paDelete->setEnabled( m_bDeleteWasEnabled );
      m_paTrash->setEnabled( m_bDeleteWasEnabled );
      m_paShred->setEnabled( m_bDeleteWasEnabled );
      m_paTrash->setText( i18n("&Move to Trash") ); // Name back
    }
  }
  return KParts::MainWindow::eventFilter( obj, ev );
}

void KonqMainWindow::slotClipboardDataChanged()
{
  kdDebug(1202) << "KonqMainWindow::slotClipboardDataChanged()" << endl;
  QMimeSource *data = QApplication::clipboard()->data();
  m_paPaste->setEnabled( data->provides( "text/plain" ) );
  slotCheckComboSelection();
}

void KonqMainWindow::slotCheckComboSelection()
{
  //kdDebug(1202) << "m_combo->lineEdit()->hasMarkedText() : " << hasSelection << endl;
  bool hasSelection = m_combo->lineEdit()->hasMarkedText();
  m_paCopy->setEnabled( hasSelection );
  m_paCut->setEnabled( hasSelection );
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

void KonqMainWindow::slotComboDelete()
{
  m_combo->lineEdit()->del();
}

void KonqMainWindow::slotClearLocationBar()
{
    if ( m_combo )
    {
        kdDebug(1202) << "slotClearLocationBar" << endl;
        m_combo->clearTemporary();
        m_combo->setFocus();
    }
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
  toggleBar( "mainToolBar" );
}

void KonqMainWindow::slotShowExtraToolBar()
{
  toggleBar( "extraToolBar" );
}

void KonqMainWindow::slotShowLocationBar()
{
  toggleBar( "locationToolBar" );
}

void KonqMainWindow::slotShowBookmarkBar()
{
  toggleBar( "bookmarkToolBar" );
}

KToolBar * KonqMainWindow::toolBarByName( const char *name )
{
  KToolBar *bar = static_cast<KToolBar *>( child( name, "KToolBar" ) );
  return bar;
}

void KonqMainWindow::toggleBar( const char *name )
{
  KToolBar *bar = toolBarByName( name );
  if ( !bar )
    return;
  if ( bar->isVisible() )
    bar->hide();
  else
    bar->show();

  saveMainWindowSettings( KGlobal::config(), "KonqMainWindow" );
  KGlobal::config()->sync();
  // otherwise changing toolbar style in kcontrol will lose our changes
}

void KonqMainWindow::slotToggleFullScreen()
{
  m_bFullScreen = !m_bFullScreen;
  if ( m_bFullScreen )
  {
    // Create toolbar button for exiting from full-screen mode
    QList<KAction> lst;
    lst.append( m_ptaFullScreen );
    plugActionList( "fullscreen", lst );

    QListIterator<KToolBar> barIt = toolBarIterator();
    for (; barIt.current(); ++barIt )
        barIt.current()->setEnableContextMenu( false );

    menuBar()->hide();
    m_paShowMenuBar->setChecked( false );

    // Preserve caption, reparent calls setCaption (!)
    QString m_oldTitle = m_currentView ? m_currentView->caption() : QString::null;
    showFullScreen();
    if (!m_oldTitle.isEmpty())
      setCaption( m_oldTitle );

    // Qt bug (see below)
    setAcceptDrops( FALSE );
    topData()->dnd = 0;
    setAcceptDrops( TRUE );

    m_ptaFullScreen->setText( i18n( "Stop Fullscreen Mode" ) );
    m_ptaFullScreen->setIcon( "window_nofullscreen" );
  }
  else
  {
    unplugActionList( "fullscreen" );

    QListIterator<KToolBar> barIt = toolBarIterator();
    for (; barIt.current(); ++barIt )
        barIt.current()->setEnableContextMenu( true );

    menuBar()->show(); // maybe we should store this setting instead of forcing it
    m_paShowMenuBar->setChecked( true );

    QString m_oldTitle = m_currentView ? m_currentView->caption() : QString::null;
    showNormal();  // (calls setCaption, i.e. the one in this class!)
    if (!m_oldTitle.isEmpty())
      setCaption( m_oldTitle );

    // Qt bug, the flags aren't restored. They know about it.
    setWFlags( WType_TopLevel | WDestructiveClose );
    // Other Qt bug
    setAcceptDrops( FALSE );
    topData()->dnd = 0;
    setAcceptDrops( TRUE );

    m_ptaFullScreen->setText( i18n( "Fullscreen Mode" ) );
    m_ptaFullScreen->setIcon( "window_fullscreen" );
  }
}

void KonqMainWindow::setLocationBarURL( const QString &url )
{
  kdDebug(1202) << "KonqMainWindow::setLocationBarURL: url = " << url << endl;

  if ( m_combo )
      m_combo->setURL( url );
}

// called via DCOP from KonquerorIface
void KonqMainWindow::addToCombos( const QString& url, const QCString& objId )
{
    if (!s_lstViews) // this happens in "konqueror --silent"
        return;
    kdDebug(1202) << "KonqMainWindow::addToCombos " << url << " " << objId << endl;
    KonqCombo *combo = 0L;
    KonqMainWindow *window = s_lstViews->first();
    while ( window ) {
	if ( window->m_combo ) {
	    combo = window->m_combo;
	    combo->insertPermanent( url );
	}
	window = s_lstViews->next();
    }

    // only one instance should save...
    if ( combo && objId == kapp->dcopClient()->defaultObject() )
	combo->saveItems();
}

QString KonqMainWindow::locationBarURL() const
{
    return m_combo ? m_combo->currentText() : QString::null;
}

void KonqMainWindow::focusLocationBar()
{
    if ( m_combo )
        m_combo->setFocus();
}

void KonqMainWindow::startAnimation()
{
  //kdDebug(1202) << "KonqMainWindow::startAnimation" << endl;
  m_paAnimatedLogo->start();
  m_paStop->setEnabled( true );
}

void KonqMainWindow::stopAnimation()
{
  //kdDebug(1202) << "KonqMainWindow::stopAnimation" << endl;
  m_paAnimatedLogo->stop();
  m_paStop->setEnabled( false );
}

void KonqMainWindow::setUpEnabled( const KURL &url )
{
  //kdDebug(1202) << "KonqMainWindow::setUpEnabled(" << url.url() << ")" << endl;
  //kdDebug(1202) << "hasPath=" << url.hasPath() << endl;
  bool bHasUpURL = false;

  if ( url.hasPath() )
    bHasUpURL = ( url.hasPath() && url.path() != "/" && url.path()[0]=='/' );
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
  (void) new KAction( i18n( "&Duplicate Window" ), "window_new", CTRL+Key_D,
                      this, SLOT( slotDuplicateWindow() ), actionCollection(), "duplicate_window" );


  (void) new KAction( i18n( "&Run Command..." ), "run", 0/*kdesktop has a binding for it*/, this, SLOT( slotRun() ), actionCollection(), "run" );
  (void) new KAction( i18n( "Open &Terminal..." ), "openterm", CTRL+Key_T, this, SLOT( slotOpenTerminal() ), actionCollection(), "open_terminal" );
  (void) new KAction( i18n( "&Open Location..." ), "fileopen", KStdAccel::key(KStdAccel::Open), this, SLOT( slotOpenLocation() ), actionCollection(), "open_location" );

  m_paFindFiles = new KAction( i18n( "&Find file..." ), "filefind", 0 /*not KStdAccel::find!*/, this, SLOT( slotToolFind() ), actionCollection(), "findfile" );

  m_paPrint = KStdAction::print( 0, 0, actionCollection(), "print" );
  (void) KStdAction::quit( this, SLOT( close() ), actionCollection(), "quit" );

  m_ptaUseHTML = new KToggleAction( i18n( "&Use index.html" ), 0, this, SLOT( slotShowHTML() ), actionCollection(), "usehtml" );
  m_paLockView = new KAction( i18n( "Lock to current location"), 0, this, SLOT( slotLockView() ), actionCollection(), "lock" );
  m_paUnlockView = new KAction( i18n( "Unlock view"), 0, this, SLOT( slotUnlockView() ), actionCollection(), "unlock" );
  m_paLinkView = new KToggleAction( i18n( "Link view"), 0, this, SLOT( slotLinkView() ), actionCollection(), "link" );

  // Go menu
  m_paUp = new KToolBarPopupAction( i18n( "&Up" ), "up", ALT+Key_Up, this, SLOT( slotUp() ), actionCollection(), "up" );
  connect( m_paUp->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotUpAboutToShow() ) );
  connect( m_paUp->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotUpActivated( int ) ) );

  m_paBack = new KToolBarPopupAction( i18n( "&Back" ), "back", ALT+Key_Left, this, SLOT( slotBack() ), actionCollection(), "back" );
  connect( m_paBack->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotBackAboutToShow() ) );
  connect( m_paBack->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotBackActivated( int ) ) );

  m_paForward = new KToolBarPopupAction( i18n( "&Forward" ), "forward", ALT+Key_Right, this, SLOT( slotForward() ), actionCollection(), "forward" );
  connect( m_paForward->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotForwardAboutToShow() ) );
  connect( m_paForward->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotForwardActivated( int ) ) );

  m_paHistory = new KonqBidiHistoryAction( i18n("History"), actionCollection(), "history" );
  connect( m_paHistory, SIGNAL( menuAboutToShow() ), this, SLOT( slotGoMenuAboutToShow() ) );
  connect( m_paHistory, SIGNAL( activated( int ) ), this, SLOT( slotGoHistoryActivated( int ) ) );

  m_paHome = new KAction( i18n( "Home URL" ), "gohome", KStdAccel::key(KStdAccel::Home), this, SLOT( slotHome() ), actionCollection(), "home" );

  (void) new KAction( i18n( "App&lications" ), 0, this, SLOT( slotGoApplications() ), actionCollection(), "go_applications" );
  (void) new KAction( i18n( "SideBar Configuration" ), 0, this, SLOT( slotGoDirTree() ), actionCollection(), "go_dirtree" );
  (void) new KAction( i18n( "Trash" ), 0, this, SLOT( slotGoTrash() ), actionCollection(), "go_trash" );
  (void) new KAction( i18n( "Templates" ), 0, this, SLOT( slotGoTemplates() ), actionCollection(), "go_templates" );
  (void) new KAction( i18n( "Autostart" ), 0, this, SLOT( slotGoAutostart() ), actionCollection(), "go_autostart" );

  // Settings menu

  m_paSaveViewProfile = new KAction( i18n( "&Save View Profile" ), 0, this, SLOT( slotSaveViewProfile() ), actionCollection(), "saveviewprofile" );
  m_paSaveViewPropertiesLocally = new KToggleAction( i18n( "View Properties Saved In &Directory" ), 0, this, SLOT( slotSaveViewPropertiesLocally() ), actionCollection(), "saveViewPropertiesLocally" );
   // "Remove" ? "Reset" ? The former is more correct, the latter is more kcontrol-like...
  m_paRemoveLocalProperties = new KAction( i18n( "Remove Directory Properties" ), 0, this, SLOT( slotRemoveLocalProperties() ), actionCollection(), "removeLocalProperties" );

  KStdAction::preferences (this, SLOT (slotConfigure()), actionCollection(), "configure");
  //  new KAction( i18n( "&Configure..." ), "configure", 0, this, SLOT( slotConfigure() ), actionCollection(), "configure" );

  KStdAction::keyBindings( this, SLOT( slotConfigureKeys() ), actionCollection(), "configurekeys" );
  KStdAction::configureToolbars( this, SLOT( slotConfigureToolbars() ), actionCollection(), "configuretoolbars" );

  // Window menu
  m_paSplitViewHor = new KAction( i18n( "Split View &Left/Right" ), "view_left_right", CTRL+SHIFT+Key_L, this, SLOT( slotSplitViewHorizontal() ), actionCollection(), "splitviewh" );
  m_paSplitViewVer = new KAction( i18n( "Split View &Top/Bottom" ), "view_top_bottom", CTRL+SHIFT+Key_T, this, SLOT( slotSplitViewVertical() ), actionCollection(), "splitviewv" );
  m_paSplitWindowHor = new KAction( i18n( "New View On Right" ), "view_right", 0, this, SLOT( slotSplitWindowHorizontal() ), actionCollection(), "splitwindowh" );
  m_paSplitWindowVer = new KAction( i18n( "New View At Bottom" ), "view_bottom", 0, this, SLOT( slotSplitWindowVertical() ), actionCollection(), "splitwindowv" );
  m_paRemoveView = new KAction( i18n( "&Remove Active View" ), "remove_view", CTRL+SHIFT+Key_R, this, SLOT( slotRemoveView() ), actionCollection(), "removeview" );

  m_paSaveRemoveViewProfile = new KAction( i18n( "&Configure View Profiles..." ), 0, m_pViewManager, SLOT( slotProfileDlg() ), actionCollection(), "saveremoveviewprofile" );
  m_pamLoadViewProfile = new KActionMenu( i18n( "Load &View Profile" ), actionCollection(), "loadviewprofile" );

  m_pViewManager->setProfiles( m_pamLoadViewProfile );

  m_ptaFullScreen = new KAction( i18n( "Fullscreen Mode" ), "window_fullscreen", CTRL+SHIFT+Key_F, this, SLOT( slotToggleFullScreen() ), actionCollection(), "fullscreen" );

  m_paReload = new KAction( i18n( "&Reload" ), "reload", KStdAccel::key(KStdAccel::Reload), this, SLOT( slotReload() ), actionCollection(), "reload" );

  m_paUndo = KStdAction::undo( KonqUndoManager::self(), SLOT( undo() ), actionCollection(), "undo" );
  //m_paUndo->setEnabled( KonqUndoManager::self()->undoAvailable() );
  connect( KonqUndoManager::self(), SIGNAL( undoTextChanged( const QString & ) ),
           m_paUndo, SLOT( setText( const QString & ) ) );

  // Those are connected to the browserextension directly
  m_paCut = KStdAction::cut( 0, 0, actionCollection(), "cut" );
  m_paCopy = KStdAction::copy( 0, 0, actionCollection(), "copy" );
  m_paPaste = KStdAction::paste( 0, 0, actionCollection(), "paste" );
  m_paStop = new KAction( i18n( "&Stop" ), "stop", Key_Escape, this, SLOT( slotStop() ), actionCollection(), "stop" );

  m_paRename = new KAction( i18n( "&Rename" ), /*"editrename",*/ Key_F2, actionCollection(), "rename" );
  m_paTrash = new KAction( i18n( "&Move to Trash" ), "edittrash", Key_Delete, actionCollection(), "trash" );
  m_paDelete = new KAction( i18n( "&Delete" ), "editdelete", SHIFT+Key_Delete, actionCollection(), "del" );
  m_paShred = new KAction( i18n( "&Shred" ), "editshred", CTRL+SHIFT+Key_Delete, actionCollection(), "shred" );

  m_paAnimatedLogo = new KonqLogoAction( i18n("Animated Logo"), 0, this, SLOT( slotDuplicateWindow() ), actionCollection(), "animated_logo" );

  // Location bar
  (void)new KonqLabelAction( i18n( "L&ocation " ), actionCollection(), "location_label" );

  m_paURLCombo = new KonqComboAction( i18n( "Location Bar" ), 0, this, SLOT( slotURLEntered( const QString & ) ), actionCollection(), "toolbar_url_combo" );
  connect( m_paURLCombo, SIGNAL( plugged() ),
           this, SLOT( slotComboPlugged() ) );

  // Tackat asked me to force 16x16... (Werner)
  (void)new KAction( i18n( "Clear location bar" ),BarIcon("locationbar_erase", 16),
                     0, this, SLOT( slotClearLocationBar() ), actionCollection(), "clear_location" );

  // Bookmarks menu
  m_pamBookmarks = new KActionMenu( i18n( "&Bookmarks" ), actionCollection(), "bookmarks" );
  // The actual menu needs a different action collection, so that the bookmarks
  // don't appear in kedittoolbar
  m_bookmarksActionCollection = new KActionCollection;
  m_bookmarksActionCollection->setHighlightingEnabled( true );
  connect( m_bookmarksActionCollection, SIGNAL( actionStatusText( const QString &) ),
           this, SLOT( slotActionStatusText( const QString & ) ) );
  connect( m_bookmarksActionCollection, SIGNAL( clearStatusText() ),
           this, SLOT( slotClearStatusText() ) );

  m_pBookmarkMenu = new KBookmarkMenu( this, m_pamBookmarks->popupMenu(), m_bookmarksActionCollection, true );

  m_paShowMenuBar = KStdAction::showMenubar( this, SLOT( slotShowMenuBar() ), actionCollection(), "showmenubar" );
  m_paShowToolBar = KStdAction::showToolbar( this, SLOT( slotShowToolBar() ), actionCollection(), "showtoolbar" );
  m_paShowExtraToolBar = new KToggleAction( i18n( "Show &Extra Toolbar" ), 0, this, SLOT( slotShowExtraToolBar() ), actionCollection(), "showextratoolbar" );
  m_paShowLocationBar = new KToggleAction( i18n( "Show &Location Toolbar" ), 0, this, SLOT( slotShowLocationBar() ), actionCollection(), "showlocationbar" );
  m_paShowBookmarkBar = new KToggleAction( i18n( "Show &Bookmark Toolbar" ), 0, this, SLOT( slotShowBookmarkBar() ),actionCollection(), "showbookmarkbar" );

  enableAllActions( false );

  // help stuff

  m_paBack->setWhatsThis( i18n( "Click this button to display the previous document<br><br>\n\n"
                                "You can also select the <b>Back Command</b> from the Go menu." ) );
  m_paBack->setStatusText( i18n( "Display the previous document" ) );

  m_paForward->setWhatsThis( i18n( "Click this button to display the next document<br><br>\n\n"
                                   "You can also select the <b>Forward Command</b> from the Go Menu." ) );

  m_paHome->setWhatsThis( i18n( "Click this button to display your 'Home URL'<br><br>\n\n"
                                "You can configure the location this button brings you to in the "
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
  m_paUnlockView->setStatusText( i18n("Unlocks the current view, so that it becomes normal again.") );
  m_paLinkView->setStatusText( i18n("Sets the view as 'linked'. A linked view follows directory changes done in other linked views") );

  QValueList<KAction*> actions = actionCollection()->actions();
  for (QValueList<KAction*>::ConstIterator it = actions.begin(); it != actions.end(); it++)
      (*it)->plugAccel(m_accel);
}

void KonqMainWindow::updateToolBarActions( bool pendingAction /*=false*/)
{
  // Enables/disables actions that depend on the current view (mostly toolbar)
  // Up, back, forward, the edit extension, stop button, wheel
  setUpEnabled( m_currentView->url() );
  m_paBack->setEnabled( m_currentView->canGoBack() );
  m_paForward->setEnabled( m_currentView->canGoForward() );

  if ( m_currentView->isLoading() )
  {
    startAnimation(); // takes care of m_paStop
  }
  else
  {
    m_paAnimatedLogo->stop();
    m_paStop->setEnabled( pendingAction );  //enable/disable based on any pending actions...
  }
}

void KonqMainWindow::updateViewActions()
{
  slotUndoAvailable( KonqUndoManager::self()->undoAvailable() );

  // Can lock a view only if there is a next view
  //m_paLockView->setEnabled( m_pViewManager->chooseNextView(m_currentView) != 0L && );
  //kdDebug(1202) << "KonqMainWindow::updateViewActions m_paLockView enabled ? " << m_paLockView->isEnabled() << endl;

  m_paLockView->setEnabled( m_currentView && !m_currentView->isLockedLocation() );
  m_paUnlockView->setEnabled( m_currentView && m_currentView->isLockedLocation() );

  // Can remove view if we'll still have a main view after that
  m_paRemoveView->setEnabled( mainViewsCount() > 1 ||
                              ( m_currentView && m_currentView->isToggleView() ) );

  // Can split a view if it's not a toggle view (because a toggle view can be here only once)
  bool isNotToggle = m_currentView && !m_currentView->isToggleView();
  m_paSplitViewHor->setEnabled( isNotToggle );
  m_paSplitViewVer->setEnabled( isNotToggle );
  m_paSplitWindowVer->setEnabled( isNotToggle );
  m_paSplitWindowHor->setEnabled( isNotToggle );

  m_paLinkView->setChecked( m_currentView && m_currentView->isLinkedView() );

  if ( m_currentView && m_currentView->part()->inherits("KonqDirPart") )
  {
    KonqDirPart * dirPart = static_cast<KonqDirPart *>(m_currentView->part());
    m_paFindFiles->setEnabled( dirPart->findPart() == 0 );

    // Create the copy/move options if not already done
    if ( !m_paCopyFiles )
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
  KParts::BrowserExtension::ActionSlotMap * actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
  KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->begin();
  KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->end();

  QStrList slotNames = ext->metaObject()->slotNames();

  for ( ; it != itEnd ; ++it )
  {
    KAction * act = actionCollection()->action( it.key() );
    //kdDebug(1202) << it.key() << endl;
    if ( act )
    {
      // Does the extension have a slot with the name of this action ?
      if ( slotNames.contains( it.key()+"()" ) )
      {
          connect( act, SIGNAL( activated() ), ext, it.data() /* SLOT(slot name) */ );
          act->setEnabled( ext->isActionEnabled( it.key() ) );
      } else
          act->setEnabled(false);

    } else kdError(1202) << "Error in BrowserExtension::actionSlotMap(), unknown action : " << it.key() << endl;
  }

}

void KonqMainWindow::disconnectExtension( KParts::BrowserExtension *ext )
{
  //kdDebug(1202) << "Disconnecting extension " << ext << endl;
  KParts::BrowserExtension::ActionSlotMap * actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
  KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->begin();
  KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->end();

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
}

void KonqMainWindow::enableAction( const char * name, bool enabled )
{
  KAction * act = actionCollection()->action( name );
  if (!act)
    kdWarning(1202) << "Unknown action " << name << " - can't enable" << endl;
  else
  {
    //kdDebug(1202) << "KonqMainWindow::enableAction " << name << " " << enabled << endl;
    act->setEnabled( enabled );
  }

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

void KonqMainWindow::currentProfileChanged()
{
    bool enabled = !m_pViewManager->currentProfile().isEmpty();
    m_paSaveViewProfile->setEnabled( enabled );
    m_paSaveViewProfile->setText( enabled ? i18n("&Save View Profile \"%1\"").arg(m_pViewManager->currentProfileText())
                                          : i18n("&Save View Profile") );
}

void KonqMainWindow::enableAllActions( bool enable )
{
  kdDebug(1202) << "KonqMainWindow::enableAllActions " << enable << endl;
  KParts::BrowserExtension::ActionSlotMap * actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();

  QValueList<KAction *> actions = actionCollection()->actions();
  QValueList<KAction *>::Iterator it = actions.begin();
  QValueList<KAction *>::Iterator end = actions.end();
  for (; it != end; ++it )
  {
    KAction *act = *it;
    if ( strncmp( act->name(), "configure", 9 ) /* do not touch the configureblah actions */
         && ( !enable || !actionSlotMap->contains( act->name() ) ) ) /* don't enable BE actions */
      act->setEnabled( enable );
  }
  // This method is called with enable=false on startup, and
  // then only once with enable=true when the first view is setup.
  // So the code below is where actions that should initially be disabled are disabled.
  if (enable)
  {
      setUpEnabled( m_currentView ? m_currentView->url() : KURL() );
      // we surely don't have any history buffers at this time
      m_paBack->setEnabled( false );
      m_paForward->setEnabled( false );

      // Load profile submenu
      m_pViewManager->profileListDirty( false );

      currentProfileChanged();

      updateViewActions(); // undo, lock, link and other view-dependent actions

      m_paStop->setEnabled( m_currentView && m_currentView->isLoading() );

      if (m_toggleViewGUIClient)
      {
          QList<KAction> actions = m_toggleViewGUIClient->actions();
          for ( KAction * it = actions.first(); it ; it = actions.next() )
              it->setEnabled( true );
      }

  }
  actionCollection()->action( "quit" )->setEnabled( true );
}

void KonqMainWindow::disableActionsNoView()
{
    // No view -> there are some things we can't do
    m_paUp->setEnabled( false );
    m_paReload->setEnabled( false );
    m_paBack->setEnabled( false );
    m_paForward->setEnabled( false );
    m_ptaUseHTML->setEnabled( false );
    m_pMenuNew->setEnabled( false );
    m_paLockView->setEnabled( false );
    m_paUnlockView->setEnabled( false );
    m_paSplitWindowVer->setEnabled( false );
    m_paSplitWindowHor->setEnabled( false );
    m_paSplitViewVer->setEnabled( false );
    m_paSplitViewHor->setEnabled( false );
    m_paRemoveView->setEnabled( false );
    m_paLinkView->setEnabled( false );
    if (m_toggleViewGUIClient)
    {
        QList<KAction> actions = m_toggleViewGUIClient->actions();
        for ( KAction * it = actions.first(); it ; it = actions.next() )
            it->setEnabled( false );
    }
    // There are things we can do, though : bookmarks, view profile, location bar, new window,
    // settings, etc.
    m_paHome->setEnabled( true );
    m_pamBookmarks->setEnabled( true );
    static const char* s_enActions[] = { "new_window", "duplicate_window", "open_location",
                                         "toolbar_url_combo", "clear_location", "animated_logo", 0 };
    for ( int i = 0 ; s_enActions[i] ; ++i )
    {
        KAction * act = action(s_enActions[i]);
        if (act)
            act->setEnabled( true );
    }
    m_pamLoadViewProfile->setEnabled( true );
    if ( m_combo )
        m_combo->clearTemporary();
    m_paShowMenuBar->setEnabled( true );
    m_paShowToolBar->setEnabled( true );
    m_paShowLocationBar->setEnabled( true );
    m_paShowBookmarkBar->setEnabled( true );
    updateLocalPropsActions();
}

void KonqMainWindow::openBookmarkURL( const QString & url )
{
  kdDebug(1202) << (QString("KonqMainWindow::openBookmarkURL(%1)").arg(url)) << endl;
  openFilteredURL( url );
}

void KonqMainWindow::setCaption( const QString &caption )
{
  // KParts sends us empty captions when activating a brand new part
  // We can't change it there (in case of apps removing all parts altogether)
  // but here we never do that.
  if ( !caption.isEmpty() && m_currentView )
  {
    kdDebug(1202) << "KonqMainWindow::setCaption(" << caption << ")" << endl;
    // Keep an unmodified copy of the caption (before kapp->makeStdCaption is applied)
    m_currentView->setCaption( caption );
    KParts::MainWindow::setCaption( caption );
  }
}

QString KonqMainWindow::currentURL() const
{
  return m_currentView ? m_currentView->url().prettyURL() : QString::null;
}

QString KonqMainWindow::currentTitle() const
{
  return m_currentView ? m_currentView->caption() : QString::null;
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

  // the page is currently loading something -> Don't enter a local event loop
  // by launching a popupmenu!
  if ( m_currentView->run() != 0 )
      return;

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
  popupMenuCollection.insert( m_paRename );
  popupMenuCollection.insert( m_paDelete );
  popupMenuCollection.insert( m_paShred );

  if ( _items.count() == 1 )
      /*
    // Can't use X-KDE-BrowserView-HideFromMenus directly in the query because '-' is a substraction !!!
    m_popupEmbeddingServices = KTrader::self()->query( _items.getFirst()->mimetype(),
                                                  "('Browser/View' in ServiceTypes) or "
                                                  "('KParts/ReadOnlyPart' in ServiceTypes)" );
                                                  */
    m_popupEmbeddingServices = KTrader::self()->query( _items.getFirst()->mimetype(),
                                                       "KParts/ReadOnlyPart",
                                                       QString::null,
                                                       QString::null );

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

  // Don't set the view URL for a toggle view.
  // (This is a bit of a hack for the directory tree....)
  const KURL & viewURL = m_currentView->isToggleView() ? KURL() : m_currentView->url();

  kdDebug(1202) << "KonqMainWindow::slotPopupMenu " << viewURL.prettyURL() << endl;

  KonqPopupMenu pPopupMenu ( _items,
                             viewURL,
                             popupMenuCollection,
                             m_pMenuNew,
                             showPropsAndFileType );

  pPopupMenu.factory()->addClient( konqyMenuClient );

  if ( client )
    pPopupMenu.factory()->addClient( client );

  QObject::disconnect( m_pMenuNew->popupMenu(), SIGNAL(aboutToShow()),
                       this, SLOT(slotFileNewAboutToShow()) );

  pPopupMenu.exec( _global );

  QObject::connect( m_pMenuNew->popupMenu(), SIGNAL(aboutToShow()),
                       this, SLOT(slotFileNewAboutToShow()) );

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

  m_popupService = m_popupEmbeddingServices[ name.toInt() ]->desktopEntryName();

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
  m_pViewManager->saveViewProfile( *config, true /* save URLs */, false );
}

void KonqMainWindow::readProperties( KConfig *config )
{
  kdDebug(1202) << "KonqMainWindow::readProperties( KConfig *config )" << endl;
  m_pViewManager->loadViewProfile( *config, QString::null /*no profile name*/ );
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

void KonqMainWindow::updateOpenWithActions()
{
  unplugActionList( "openwith" );

  m_openWithActions.clear();

  const KTrader::OfferList & services = m_currentView->appServiceOffers();
  KTrader::OfferList::ConstIterator it = services.begin();
  KTrader::OfferList::ConstIterator end = services.end();
  for (; it != end; ++it )
  {
    KAction *action = new KAction( i18n( "Open With %1" ).arg( (*it)->name() ), 0, 0, (*it)->desktopEntryName().latin1() );
    action->setIcon( (*it)->icon() );

    connect( action, SIGNAL( activated() ),
             this, SLOT( slotOpenWith() ) );

    m_openWithActions.append( action );
  }
  if ( services.count() > 0 )
  {
      m_openWithActions.append( new KActionSeparator );
      plugActionList( "openwith", m_openWithActions );
  }
}

void KonqMainWindow::updateViewModeActions()
{
  unplugViewModeActions();
  if ( m_viewModeMenu )
  {
    QListIterator<KAction> it( m_viewModeActions );
    for (; it.current(); ++it )
      it.current()->unplugAll();
    delete m_viewModeMenu;
  }

  m_viewModeMenu = 0;
  m_toolBarViewModeActions.clear();
  m_viewModeActions.clear();

  // if we changed the viewmode to something new, then we have to
  // make sure to also clear our [libiconview,liblistview]->service-for-viewmode
  // map
  if ( m_viewModeToolBarServices.count() > 0 &&
       !m_viewModeToolBarServices.begin().data()->serviceTypes().contains( m_currentView->serviceType() ) )
  {
      // Save the current map to the config file, for later reuse
      saveToolBarServicesMap();

      m_viewModeToolBarServices.clear();
  }

  KTrader::OfferList services = m_currentView->partServiceOffers();

  if ( services.count() <= 1 )
    return;

  m_viewModeMenu = new KActionMenu( i18n( "View Mode" ), this );

  // a temporary map, just like the m_viewModeToolBarServices map, but
  // mapping to a KonqViewModeAction object. It's just temporary as we
  // of use it to group the viewmode actions (iconview,multicolumnview,
  // treeview, etc.) into to two groups -> icon/list
  // Although I wrote this now only of icon/listview it has to work for
  // any view, that's why it's so general :)
  QMap<QString,KonqViewModeAction*> groupedServiceMap;

  // Another temporary map, the preferred service for each library (2 entries in our example)
  QMap<QString,QString> preferredServiceMap;

  KConfig * config = KGlobal::config();
  config->setGroup( "ModeToolBarServices" );

  KTrader::OfferList::ConstIterator it = services.begin();
  KTrader::OfferList::ConstIterator end = services.end();
  for (; it != end; ++it )
  {
      QVariant prop = (*it)->property( "X-KDE-BrowserView-Toggable" );
      if ( prop.isValid() && prop.toBool() ) // No toggable views in view mode
          continue;

      KRadioAction *action;

      QString icon = (*it)->icon();
      if ( icon != QString::fromLatin1( "unknown" ) )
          // we *have* to specify a parent qobject, otherwise the exclusive group stuff doesn't work!(Simon)
          action = new KRadioAction( (*it)->name(), icon, 0, this, (*it)->desktopEntryName().ascii() );
      else
          action = new KRadioAction( (*it)->name(), 0, this, (*it)->desktopEntryName().ascii() );

      action->setExclusiveGroup( "KonqMainWindow_ViewModes" );

      connect( action, SIGNAL( toggled( bool ) ),
               this, SLOT( slotViewModeToggle( bool ) ) );

      m_viewModeActions.append( action );
      action->plug( m_viewModeMenu->popupMenu() );

      // look if we already have a KonqViewModeAction (in the toolbar)
      // for this component
      QMap<QString,KonqViewModeAction*>::Iterator mapIt = groupedServiceMap.find( (*it)->library() );

      // if we don't have -> create one
      if ( mapIt == groupedServiceMap.end() )
      {
          // default service on this action: the current one (i.e. the first one)
          QString text = (*it)->name();
          QString icon = (*it)->icon();
          QCString name = (*it)->desktopEntryName().latin1();
          //kdDebug(1202) << " Creating action for " << (*it)->library() << ". Default service " << (*it)->name() << endl;

          // if we previously changed the viewmode (see slotViewModeToggle!)
          // then we will want to use the previously used settings (previous as
          // in before the actions got deleted)
          QMap<QString,KService::Ptr>::ConstIterator serviceIt = m_viewModeToolBarServices.find( (*it)->library() );
          if ( serviceIt != m_viewModeToolBarServices.end() )
          {
              //kdDebug(1202) << " Setting action for " << (*it)->library() << " to " << (*serviceIt)->name() << endl;
              text = (*serviceIt)->name();
              icon = (*serviceIt)->icon();
              name = (*serviceIt)->desktopEntryName().ascii();
          } else
          {
              // if we don't have it in the map, we should look for a setting
              // for this library in the config file.
              QString preferredService = config->readEntry( (*it)->library() );
              if ( !preferredService.isEmpty() && name != preferredService.latin1() )
              {
                  //kdDebug(1202) << " Inserting into preferredServiceMap(" << (*it)->library() << ") : " << preferredService << endl;
                  // The preferred service isn't the current one, so remember to set it later
                  preferredServiceMap[ (*it)->library() ] = preferredService;
              }
          }

          KonqViewModeAction *tbAction = new KonqViewModeAction( text,
                                                                 icon,
                                                                 this,
                                                                 name );

          tbAction->setExclusiveGroup( "KonqMainWindow_ToolBarViewModes" );

          tbAction->setChecked( action->isChecked() );

          connect( tbAction, SIGNAL( toggled( bool ) ),
                   this, SLOT( slotViewModeToggle( bool ) ) );

          m_toolBarViewModeActions.append( tbAction );

          mapIt = groupedServiceMap.insert( (*it)->library(), tbAction );
      }

      // Check the actions (toolbar button and menu item) if they correspond to the current view
      bool bIsCurrentView = (*it)->desktopEntryName() == m_currentView->service()->desktopEntryName();
      if ( bIsCurrentView )
      {
          (*mapIt)->setChecked( true );
          action->setChecked( true );
      }

      // Set the contents of the button from the current service, either if it's the current view
      // or if it's our preferred service for this button (library)
      if ( bIsCurrentView
           || ( preferredServiceMap.contains( (*it)->library() ) && (*it)->desktopEntryName() == preferredServiceMap[ (*it)->library() ] ) )
      {
          //kdDebug(1202) << " Changing action for " << (*it)->library() << " into service " << (*it)->name() << endl;

          (*mapIt)->setText( (*it)->name() );
          (*mapIt)->setIcon( (*it)->icon() );
          (*mapIt)->setName( (*it)->desktopEntryName().ascii() ); // tricky...
          preferredServiceMap.remove( (*it)->library() ); // The current view has priority over the saved settings
      }

      // plug action also into the delayed popupmenu of appropriate toolbar action
      action->plug( (*mapIt)->popupMenu() );
  }

#ifndef NDEBUG
  ASSERT( preferredServiceMap.isEmpty() );
  QMap<QString,QString>::Iterator debugIt = preferredServiceMap.begin();
  QMap<QString,QString>::Iterator debugEnd = preferredServiceMap.end();
  for ( ; debugIt != debugEnd ; ++debugIt )
      kdDebug(1202) << " STILL IN preferredServiceMap : " << debugIt.key() << " | " << debugIt.data() << endl;
#endif

  if ( !m_currentView->isToggleView() ) // No view mode for toggable views
      // (The other way would be to enforce a better servicetype for them, than Browser/View)
      if ( /* already tested: services.count() > 1 && */ m_viewModeMenu )
          plugViewModeActions();
}

void KonqMainWindow::saveToolBarServicesMap()
{
    QMap<QString,KService::Ptr>::ConstIterator serviceIt = m_viewModeToolBarServices.begin();
    QMap<QString,KService::Ptr>::ConstIterator serviceEnd = m_viewModeToolBarServices.end();
    KConfig * config = KGlobal::config();
    config->setGroup( "ModeToolBarServices" );
    for ( ; serviceIt != serviceEnd ; ++serviceIt )
        config->writeEntry( serviceIt.key(), serviceIt.data()->desktopEntryName() );
    config->sync();
}

void KonqMainWindow::plugViewModeActions()
{
  QList<KAction> lst;
  lst.append( m_viewModeMenu );
  plugActionList( "viewmode", lst );
  // display the toolbar viewmode icons only for inode/directory, as here we have dedicated icons
  if ( m_currentView->serviceType() == "inode/directory" )
    plugActionList( "viewmode_toolbar", m_toolBarViewModeActions );
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
  if ( bar && bar->count() == 0 )
  {
    m_paShowBookmarkBar->setChecked( false );
    bar->hide();
  }
}

void KonqMainWindow::closeEvent( QCloseEvent *e )
{
  //kdDebug(1202) << "KonqMainWindow::closeEvent begin" << endl;
  // This breaks session management (the window is withdrawn in kwin)
  // so let's do this only when closed by the user.
  if ( static_cast<KonquerorApplication *>(kapp)->closedByUser() )
  {
    hide();
    qApp->flushX();
  }
  KParts::MainWindow::closeEvent( e );
  //kdDebug(1202) << "KonqMainWindow::closeEvent end" << endl;
}


#include "konq_mainwindow.moc"
