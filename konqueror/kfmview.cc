/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/     

#include "kfmview.h"
#include "kfmviewprops.h"
#include "kfmgui.h"
#include "kpixmapcache.h"
#include "kservices.h"
#include "kfm_abstract_gui.h"
#include "krun.h"
#include "kfmfinder.h"
#include "kfmicons.h"
#include "kfmbrowser.h"
#include "kdirwatch.h"
#include "kfmrun.h"
#include "kio_openwith.h"
#include "kio_job.h"
#include "kio_error.h"
#include "kcursor.h"
#include "khtml.h"
#include "kfmpaths.h"

#include <iostream.h>

#include <kapp.h>
#include <k2url.h>
#include <kclipboard.h>
#include <kio_paste.h>
#include <kio_manager.h>

#include <qstring.h>
#include <qlayout.h>

KfmView::KfmView( KfmAbstractGui *_gui, QWidget *_parent ) : QWidgetStack( _parent )
{
  m_pGui         = _gui;
  m_pRun         = 0L;

  
  setFocusPolicy( StrongFocus );
  
  m_oldViewMode = NOMODE;
  m_hasFocus     = false;
  m_popupMenu    = new QPopupMenu();
  m_menuNew      = new KNewMenu();

  setFrameStyle( QFrame::Panel | QFrame::Sunken );
  //  setLineWidth( 2 );

  initConfig(); // props has to be ready now - it's read by kfmfinder and others.

  m_pFinder = new KfmFinder( this );
  addWidget( m_pFinder , FINDER);
  connect( m_pFinder, SIGNAL( started( const char* ) ), 
	   this, SLOT( slotURLStarted( const char* ) ) );
  connect( m_pFinder, SIGNAL( canceled() ), 
	   this, SLOT( slotURLCanceled() ) );
  connect( m_pFinder, SIGNAL( completed() ), 
	   this, SLOT( slotURLCompleted() ) );
  connect( m_pFinder, SIGNAL( gotFocus() ),
	   this, SLOT( slotGotFocus() ) );

  m_pIconView = new KfmIconView( this, this );
  addWidget( m_pIconView , HOR_ICONS);
  connect( m_pIconView, SIGNAL( started( const char* ) ), 
	   this, SLOT( slotURLStarted( const char* ) ) );
  connect( m_pIconView, SIGNAL( canceled() ), 
	   this, SLOT( slotURLCanceled() ) );
  connect( m_pIconView, SIGNAL( completed() ), 
	   this, SLOT( slotURLCompleted() ) );
  connect( m_pIconView, SIGNAL( gotFocus() ),
	   this, SLOT( slotGotFocus() ) );

  m_pBrowser = new KfmBrowser( this, this );
  addWidget( m_pBrowser , HTML);
  connect( m_pBrowser, SIGNAL( started( const char* ) ), 
	   this, SLOT( slotURLStarted( const char* ) ) );
  connect( m_pBrowser, SIGNAL( canceled() ), 
	   this, SLOT( slotURLCanceled() ) );
  connect( m_pBrowser, SIGNAL( completed() ), 
	   this, SLOT( slotURLCompleted() ) );
  connect( m_pBrowser, SIGNAL( gotFocus() ),
	   this, SLOT( slotGotFocus() ) );

  connect( kdirwatch, SIGNAL( dirty( const char* ) ), 
	   this, SLOT( slotDirectoryDirty( const char* ) ) );
  connect( kdirwatch, SIGNAL( deleted( const char* ) ), 
	   this, SLOT( slotDirectoryDeleted( const char* ) ) );

  //  connect( m_pGui(), SIGNAL( configChanged() ), SLOT( initConfig() ) );

  setViewMode( m_viewMode, false );

}

KfmView::~KfmView()
{
  if ( m_pFinder )
    delete m_pFinder;

  if ( m_pIconView )
    delete m_pIconView;

  if ( m_pBrowser )
    delete m_pBrowser;

  if ( m_pRun )
    delete m_pRun;

  if ( m_menuNew )
    delete m_menuNew;

  if ( m_popupMenu )
    delete m_popupMenu;

  if ( m_Props )
    delete m_Props;
}

KfmViewSettings * KfmView::settings()
{
  if (m_viewMode == HTML)
    return KfmViewSettings::m_pDefaultHTMLSettings;
  else
    return KfmViewSettings::m_pDefaultFMSettings;
}

void KfmView::initConfig()
{
  // Read application config file if not already done
  if (!KfmViewProps::m_pDefaultProps)
  {
    debug("Reading global config for kfmviewprops");
    KConfig *config = kapp->getConfig();
    KConfigGroupSaver cgs(config, "Settings");
    KfmViewProps::m_pDefaultProps = new KfmViewProps(config);
  }

  // Read application config file if not already done
  if (!KfmViewSettings::m_pDefaultFMSettings)
  {
    debug("Reading global config for kfmviewsettings");
    KConfig *config = kapp->getConfig();
    KConfigGroupSaver cgs(config, "KFM HTML Defaults" );
    KfmViewSettings::m_pDefaultHTMLSettings = new KfmViewSettings(config);
    config->setGroup("KFM FM Defaults");
    KfmViewSettings::m_pDefaultFMSettings = new KfmViewSettings(config);
  }
  
  // For the moment, no local properties
  // Copy the default properties
  m_Props = new KfmViewProps( *KfmViewProps::m_pDefaultProps );

  m_viewMode = m_Props->m_viewMode; // cache a copy, will make things simpler

  // TODO : when saving configuration,   m_Props->m_viewMode = m_viewMode;
}

void KfmView::fetchFocus()
{
  switch( m_viewMode )
  {
  case FINDER:
    m_pFinder->setFocus();
    break;
  case HOR_ICONS:
  case VERT_ICONS:
    m_pIconView->setFocus();
    break;
  case HTML:
    m_pBrowser->setFocus();
    break;
  case NOMODE:
    break;
  }

  //  QWidgetStack::setFocus();
}

void KfmView::setViewMode( ViewMode _mode, bool _open_url )
{
  if ( _mode == m_viewMode )
    return;

  ViewMode oldview = m_viewMode;
  
  QString url = currentURL();

  // Impossible to show HTML stuff with some other view.
  // The menu is changed, but we wont change the view.
  if ( _open_url && oldview == HTML && !m_pBrowser->isFileManagerMode() )
    return;
  
  m_viewMode = _mode;
  
  if ( oldview != HTML && m_viewMode != HTML )
    m_oldViewMode = oldview;

  switch( m_viewMode )
  {
  case FINDER:
    //    setFocusProxy( m_pFinder );
    if ( !url.isEmpty() && _open_url )
      m_pFinder->openURL( url );
    raiseWidget( m_pFinder );
    m_pFinder->setFocus();
    break;
  case HOR_ICONS:
    //    setFocusProxy( m_pIconView );
    m_pIconView->setDisplayMode( KIconContainer::Horizontal );
    if ( !url.isEmpty() && oldview != VERT_ICONS && _open_url )
      m_pIconView->openURL( url );
    raiseWidget( m_pIconView );
    m_pIconView->setFocus();
    break;
  case VERT_ICONS:
    //    setFocusProxy( m_pIconView );
    m_pIconView->setDisplayMode( KIconContainer::Vertical );
    if ( !url.isEmpty() && oldview != HOR_ICONS && _open_url )
      m_pIconView->openURL( url );
    raiseWidget( m_pIconView );
    m_pIconView->setFocus();
    break;
  case HTML:
    //    setFocusProxy( m_pBrowser );
    // m_pBrowser->openURL( "/usr/lib/qt/html/classes.html" );
    if ( !url.isEmpty() && _open_url )
      m_pBrowser->openURL( "http://localhost/doc" );
    raiseWidget( m_pBrowser );
    m_pBrowser->setFocus();
    break;
  default:
    assert( 0 );
  }
}

void KfmView::openURL( const char *_url, mode_t _mode, bool _is_local_file,
		       int _xoffset, int _yoffset )
{
  if ( !m_strWorkingURL.empty() )
    stop();
  
  //  m_strWorkingURL = _url;
  m_iWorkingXOffset = _xoffset;
  m_iWorkingYOffset = _yoffset;
  
  emit started();

  m_pRun = new KfmRun( this, _url, _mode, _is_local_file );
}

void KfmView::openDirectory( const char *_url )
{
  m_pRun = 0L;

  // Not very intelligent since the view will most propably
  // send us a started signal soon, but in case of an error
  // we dont get it => We stop the spinning wheel here
  canceled();
 
  // Parse URL
  K2URLList lst;
  assert( K2URL::split( _url, lst ) );

  // Do we perhaps want to display a html index file ? => Save the path of the URL
  QString tmppath;
  if ( lst.size() == 1 && lst.front().isLocalFile() && isHTMLAllowed() )
    tmppath = lst.front().path();
  
  // Get parent directory
  QString p = lst.back().path();
  if ( p.isEmpty() || p == "/" )
    m_pGui->setUpURL( 0 );
  else
  {
    string dir = lst.back().directory( true, true );
    lst.back().setPath( dir.c_str() );
    string url;
    K2URL::join( lst, url );
    m_pGui->setUpURL( url.c_str() );
  }

  if ( !tmppath.isEmpty() )
  {
    cerr << "////////////////////// TESTING index /////////////////////////" << endl;
    QString index = findIndexFile( tmppath );
    if ( !index.isEmpty() )
    {
      cerr << "/////////////////// INDEX=" << index << "//////////////////" << endl;
      setViewMode( HTML, false );
      K2URL u( "file:/" );
      u.setPath( index );
      string url = u.url();
      m_pBrowser->openURL( url.c_str(), false, m_iWorkingXOffset, m_iWorkingYOffset );
      return;
    }
  }
  
// ?
//  if ( m_viewMode == HTML && m_Props->viewMode() != HTML )
//    setViewMode( m_Props->viewMode(), false );
  
  if ( m_viewMode == FINDER )
    m_pFinder->openURL( _url );
  else if ( m_viewMode == HOR_ICONS || m_viewMode == VERT_ICONS )
    m_pIconView->openURL( _url );
  else if ( m_viewMode == HTML )
    cerr << "NOT IMPLEMENTED YET" << endl;
}

void KfmView::openHTML( const char *_url )
{
  m_pRun = 0L;

  // Not very intelligent since the view will most propably
  // send us a started signal soon, but in case of an error
  // we dont get it => We stop the spining wheel here
  canceled();

  m_pGui->setUpURL( 0 );
  
  if ( m_viewMode != HTML )
    setViewMode( HTML, false );
  
  m_pBrowser->openURL( _url, false, m_iWorkingXOffset, m_iWorkingYOffset );
}

void KfmView::openNothing()
{
  completed();
  
  m_pRun = 0L;
}

void KfmView::slotURLStarted( const char *_url )
{
  if ( !_url )
    return;

  // avoid adding same url to history
  if ( _url != m_strURL )
    makeHistory();

  m_strWorkingURL = _url;

  m_pGui->setLocationBarURL( m_strWorkingURL.c_str() );
  
  emit started();
}

void KfmView::slotURLCanceled()
{
  cerr << "===================================================" << endl;
  cerr << "========== CANCELED ===============================" << endl;
  cerr << "===================================================" << endl;

  // set current url to working url if we already started loading
  if ( !m_strWorkingURL.empty() )
    m_strURL = m_strWorkingURL;

  m_strWorkingURL = "";

  m_pGui->setLocationBarURL( m_strURL.c_str() );
  
  emit canceled();
}

void KfmView::slotURLCompleted()
{
  emit completed();

  // If m_strWorkingURL is empty, then we just opened
  // a subfolder in the tree view => we just stop the spinning
  // wheel and return
  if ( m_strWorkingURL.empty() )
    return;
  
  cerr << "===================================================" << endl;
  cerr << "========== COMPLETED ==============================" << endl;
  cerr << "===================================================" << endl;
  
  m_strURL = m_strWorkingURL;
  m_strWorkingURL = "";

  if ( !m_strURL.empty() )
  {
    K2URL u( m_strURL );
    if ( u.isLocalFile() )
    {
      string p = u.path( 0 );
      kdirwatch->removeDir( p.c_str() );
    }
  }
  
  if ( m_viewMode == FINDER )
    m_pFinder->setContentsPos( m_iWorkingXOffset, m_iWorkingYOffset );
  else if ( m_viewMode == HOR_ICONS || m_viewMode == VERT_ICONS )
      m_pIconView->setContentsPos( m_iWorkingXOffset, m_iWorkingYOffset );
  
  K2URL u( m_strURL );
  if ( u.isLocalFile() )
  {
    string p = u.path( 0 );
    kdirwatch->addDir( p.c_str() );
  }
}

void KfmView::stop()
{
  if ( m_pRun )
  {
    delete m_pRun;
    m_pRun = 0L;
  }   

  // set current url to working url if we already started loading
  if ( !m_strWorkingURL.empty() )
    m_strURL = m_strWorkingURL;

  m_strWorkingURL = "";

  // HACK TODO
  // m_pFinder->stop();
  // m_pIconView->stop();
  m_pBrowser->slotStop();
}

void KfmView::slotPopupCd()
{
  assert( m_lstPopupURLs.count() == 1 );
  openURL( m_lstPopupURLs.getFirst() );
}

void KfmView::slotPopupNewView()
{
  assert( m_lstPopupURLs.count() == 1 );
  m_pGui->createGUI(m_lstPopupURLs.getFirst() );
}

void KfmView::slotPopupCopy()
{
  KClipboard::self()->setURLList( m_lstPopupURLs );
}

void KfmView::slotPopupDelete()
{
  KIOJob* job = new KIOJob;
  list<string> lst;
  const char *s;
  for( s = m_lstPopupURLs.first(); s != 0L; s = m_lstPopupURLs.next() )
    lst.push_back( s );
  job->del( lst );
}

void KfmView::slotPopupPaste()
{
  assert( m_lstPopupURLs.count() == 1 );
  pasteClipboard( m_lstPopupURLs.getFirst() );
}

void KfmView::slotPopupOpenWith()
{
  OpenWithDlg l( klocale->translate("Open With:"), "", this, true );
  if ( l.exec() )
  {
    KService *service = l.service();
    if ( service )
    {
      KfmRun::run( service, m_lstPopupURLs );
      return;
    }
    else
    {
      QString exec = l.text();
      exec += " %f";
      KfmRun::runOldApplication( exec, m_lstPopupURLs, false );
    }
  }
}              

void KfmView::slotPopup( int _id )
{
  // Is it a usual service
  map<int,KService*>::iterator it = m_mapPopup.find( _id );
  if ( it != m_mapPopup.end() )
  {
    KRun::run( it->second, m_lstPopupURLs );
    return;
  }
  
  // Is it a service specific to kdelnk files ?
  map<int,KDELnkMimeType::Service>::iterator it2 = m_mapPopup2.find( _id );
  if ( it2 == m_mapPopup2.end() )
    return;
  
  const char *u;
  for( u = m_lstPopupURLs.first(); u != 0L; u = m_lstPopupURLs.next() )
    KDELnkMimeType::executeService( u, it2->second );

  return;
}

void KfmView::popupMenu( const QPoint &_global, QStrList& _urls, mode_t _mode, bool _is_local_file )
{
  cerr << "void KfmView::popupMenu( const QPoint &_global, QStrList& _urls, mode_t _mode, bool _is_local_file )" << endl;
  
  assert( _urls.count() >= 1 );
  
  bool bHttp          = true;
  bool isTrash        = true;
  bool currentDir     = false;
  bool isCurrentTrash = false;
  bool sReading       = true;
  bool sWriting       = true;
  bool sDeleting      = true;
  bool sMoving        = true;
  int id;

  ProtocolManager* pManager = ProtocolManager::self();
  
  K2URL url;

  // Check wether all URLs are correct
  const char *s;

  for ( s = _urls.first(); s != 0L; s = _urls.next() )
  {
    url = K2URL( s );
    const char* protocol = url.protocol();

    if ( url.isMalformed() )
    {
      emit error( ERR_MALFORMED_URL, s );
      return;
    }
    if (strcmp( protocol, "http" )) bHttp = false; // not HTTP

    // check if all urls are in the trash
    if ( isTrash )
    {
      QString path = url.path();
      if ( path.right(1) != "/" )
	path += "/";
    
      if ( strcmp( protocol, "file" ) != 0L ||
	   path != KfmPaths::trashPath() )
	isTrash = false;
    }

    if ( sReading )
      sReading = pManager->supportsReading( protocol );

    if ( sWriting )
      sWriting = pManager->supportsWriting( protocol );
    
    if ( sDeleting )
      sDeleting = pManager->supportsDeleting( protocol );

    if ( sMoving )
      sMoving = pManager->supportsMoving( protocol );
  }

  //check if current url is trash
  url = K2URL( m_strURL );
  QString path = url.path();
  if ( path.right(1) != "/" )
    path += "/";
    
  if ( strcmp( url.protocol(), "file" ) == 0L &&
       path == KfmPaths::trashPath() )
    isTrash = true;

  //check if url is current directory
  if ( _urls.count() == 1 )
    if ( strcmp( m_strURL.c_str(), _urls.first() ) == 0 )
      currentDir = true;

  m_lstPopupURLs = _urls;
  
  disconnect( m_popupMenu, SIGNAL( activated( int ) ), this, SLOT( slotPopup( int ) ) );

  m_popupMenu->clear();

//   //---------- Sven --------------
//   // check if menubar is hidden and if yes add "Show Menubar"
//   if (view->getGUI()->isMenuBarHidden())
//   {
    
//     popupMenu->insertItem(klocale->getAlias(ID_STRING_SHOW_MENUBAR),
// 			  view->getGUI(), SLOT(slotShowMenubar()));
//     popupMenu->insertSeparator();
//   }
//   //------------------------------

  if ( isTrash )
  {
    /* Commented out. Left click does it. Why have it on right click menu ?. David.
       id = popupMenu->insertItem( klocale->getAlias(ID_STRING_CD), 
       view, SLOT( slotPopupCd() ) );
    */
    id = m_popupMenu->insertItem( i18n( "New view" ), 
				  this, SLOT( slotPopupNewView() ) );
    m_popupMenu->insertSeparator();    
    id = m_popupMenu->insertItem( i18n( "Empty Trash Bin" ), 
				  this, SLOT( slotPopupEmptyTrashBin() ) );
  } 
  else if ( S_ISDIR( _mode ) )
  {
    id = m_popupMenu->insertItem( i18n("&New"), m_menuNew->popupMenu() );
    m_popupMenu->insertSeparator();

    id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "up.xpm" ), i18n( "Up" ), this, SLOT( slotUp() ), 100 );
    if ( !m_pGui->hasUpURL() )
      m_popupMenu->setItemEnabled( id, false );

    id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "back.xpm" ), i18n( "Back" ), this, SLOT( slotBack() ), 101 );
    if ( !m_pGui->hasBackHistory() )
      m_popupMenu->setItemEnabled( id, false );

    id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "forward.xpm" ), i18n( "Forward" ), this, SLOT( slotForward() ), 102 );
    if ( !m_pGui->hasForwardHistory() )
      m_popupMenu->setItemEnabled( id, false );

    m_popupMenu->insertSeparator();  

    id = m_popupMenu->insertItem( i18n( "New View"), this, SLOT( slotPopupNewView() ) );
    m_popupMenu->insertSeparator();    

    if ( sReading )
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "editcopy.xpm" ), i18n( "Copy" ), this, SLOT( slotPopupCopy() ) );
    if ( sWriting )
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "editpaste.xpm" ), i18n( "Paste" ), this, SLOT( slotPopupPaste() ) );
    if ( isClipboardEmpty() )
      m_popupMenu->setItemEnabled( id, false );
    if ( sMoving && !isCurrentTrash && !currentDir )
      id = m_popupMenu->insertItem( *KPixmapCache::pixmap( "kfm_trash.xpm", true ), i18n( "Move to trash" ), this, SLOT( slotPopupTrash() ) );
    if ( sDeleting && !currentDir )
      id = m_popupMenu->insertItem( i18n( "Delete" ), this, SLOT( slotPopupDelete() ) );
  }
  else
  {
    if ( bHttp )
    {
      /* Should be for http URLs (HTML pages) only ... */
      id = m_popupMenu->insertItem( i18n( "New View"), this, SLOT( slotPopupNewView() ) );
    }
    id = m_popupMenu->insertItem( i18n( "Open with" ), this, SLOT( slotPopupOpenWith() ) );
    m_popupMenu->insertSeparator();

    if ( sReading )
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "editcopy.xpm" ), i18n( "Copy" ), this, SLOT( slotPopupCopy() ) );
    if ( sMoving && !isCurrentTrash && !currentDir )
      id = m_popupMenu->insertItem( *KPixmapCache::pixmap( "kfm_trash.xpm", true ), i18n( "Move to trash" ), this, SLOT( slotPopupTrash() ) );
    if ( sDeleting /* && !_current_dir */)
      id = m_popupMenu->insertItem( i18n( "Delete" ), this, SLOT( slotPopupDelete() ) );
  }

  id = m_popupMenu->insertItem( i18n( "Add To Bookmarks" ), this, SLOT( slotPopupBookmarks() ) );

  m_menuNew->setPopupFiles( _urls );

  // Do all URLs have the same mimetype ?
  url = K2URL( _urls.first() );  

  KMimeType* mime = KMimeType::findByURL( url, _mode, _is_local_file );
  assert( mime );
  for( s = _urls.next(); mime != 0L && s != 0L; s = _urls.next() )
  {
    K2URL u( s );  
    KMimeType* m = KMimeType::findByURL( u, _mode, _is_local_file );
    if ( m != mime )
      mime = 0L;
  }
  
  if ( mime )
  {
    // Get all services for this mime type
    list<KService::Offer> offers;
    KService::findServiceByServiceType( mime->mimeType(), offers );

    list<KDELnkMimeType::Service> builtin;
    list<KDELnkMimeType::Service> user;
    if ( strcmp( mime->mimeType(), "application/x-kdelnk" ) == 0 )
    {
      KDELnkMimeType::builtinServices( url, builtin );
      KDELnkMimeType::userDefinedServices( url, user );
    }
  
    if ( !offers.empty() || !user.empty() || !builtin.empty() )
      connect( m_popupMenu, SIGNAL( activated( int ) ), this, SLOT( slotPopup( int ) ) );

    if ( !offers.empty() || !user.empty() )
      m_popupMenu->insertSeparator();
  
    m_mapPopup.clear();
    m_mapPopup2.clear();
  
    list<KService::Offer>::iterator it = offers.begin();
    for( ; it != offers.end(); it++ )
    {    
      id = m_popupMenu->insertItem( *(KPixmapCache::pixmap( it->m_pService->icon(), true ) ), it->m_pService->name() );
      m_mapPopup[ id ] = it->m_pService;
    }
    
    list<KDELnkMimeType::Service>::iterator it2 = user.begin();
    for( ; it2 != user.end(); ++it2 )
    {
      if ( !it2->m_strIcon.isEmpty() )
	id = m_popupMenu->insertItem( *(KPixmapCache::pixmap( it2->m_strIcon, true ) ), it2->m_strName );
      else
	id = m_popupMenu->insertItem( it2->m_strName );
      m_mapPopup2[ id ] = *it2;
    }
    
    if ( builtin.size() > 0 )
      m_popupMenu->insertSeparator();

    it2 = builtin.begin();
    for( ; it2 != builtin.end(); ++it2 )
    {
      if ( !it2->m_strIcon.isEmpty() )
	id = m_popupMenu->insertItem( *(KPixmapCache::pixmap( it2->m_strIcon, true ) ), it2->m_strName );
      else
	id = m_popupMenu->insertItem( it2->m_strName );
      m_mapPopup2[ id ] = *it2;
    }
  }
  
  if ( _urls.count() == 1 )
  {
    m_popupMenu->insertSeparator();
    m_popupMenu->insertItem( i18n("Properties"), this, SLOT( slotPopupProperties() ) );
  }
  
  m_popupMenu->popup( _global );
}

void KfmView::openBookmarkURL( const char *_url )
{
  openURL( _url );
}

QString KfmView::currentTitle()
{
  QString title = "";
  if ( m_pFinder && m_viewMode == FINDER )
    title = m_pFinder->url();
  else if ( m_pIconView && ( m_viewMode == HOR_ICONS || m_viewMode == VERT_ICONS ) )
    title = m_pIconView->url();
  else if ( m_pBrowser && ( m_viewMode == HTML ) )
    title = m_pBrowser->url();
  return title;
}

QString KfmView::currentURL()
{
  /* QString url = "";
  if ( m_viewMode == FINDER )
    url = m_pFinder->url();
  else if ( m_viewMode == HOR_ICONS || m_viewMode == VERT_ICONS )
    url = m_pIconView->url();
  else if ( m_viewMode == HTML )
    url = m_pBrowser->url();
  return url; */
  QString url = m_strURL.c_str();
  return url;
}

void KfmView::makeHistory()
{
  if ( m_strURL.empty() )
    return;
  
  int xoffset = 0, yoffset = 0;
  if ( m_viewMode == FINDER )
  {
    xoffset = m_pFinder->contentsX();
    yoffset = m_pFinder->contentsY();
  }
  else if ( m_viewMode == HOR_ICONS || m_viewMode == VERT_ICONS )
  {
    xoffset = m_pIconView->contentsX();
    yoffset = m_pIconView->contentsY();
  }
  else if ( m_viewMode == HTML )
  {
    xoffset = m_pBrowser->xOffset();
    yoffset = m_pBrowser->yOffset();
  }
  else
    assert( 0 );
  
  m_pGui->addHistory( m_strURL.c_str(), xoffset, yoffset );
}

void KfmView::slotDirectoryDirty( const char * _dir )
{
  string f = _dir;
  K2URL::encode( f );
  if ( !urlcmp( m_strURL.c_str(), f.c_str(), true, true ) )
    return;
    
  if ( m_viewMode == FINDER )
    m_pFinder->updateDirectory();
  else if ( m_viewMode == HOR_ICONS || m_viewMode == VERT_ICONS )
    m_pIconView->updateDirectory();  
}

void KfmView::slotDirectoryDeleted( const char * _dir )
{
  K2URL u( m_strURL );
  if ( u.isMalformed() )
    return;
  
  string s = u.path( 0 );
  if ( s != _dir )
    return;

  stop();
  delete m_pGui;
}

bool KfmView::isHTMLAllowed( ) { return m_Props->m_bHTMLAllowed; }

void KfmView::setHTMLAllowed( bool _allow )
{
  if ( m_Props->m_bHTMLAllowed == _allow )
    return;
  
  m_Props->m_bHTMLAllowed = _allow;
  
  if ( !_allow )
  {
    ////////////
    // Check whether we are displaying some HTML stuff here
    // while we could as well use the usual icon/tree view as
    // selected in the "view" menu. In this case we would have to
    // drop the HTML view due to the changed policy.
    ////////////
    //if ( m_viewMode == m_Props->viewMode() ) // probably wrong...
    //  return;
    // Ok, we can conclude now that we display HTML
    // assert( m_viewMode == KfmView::HTML );

    QString url = m_strWorkingURL.c_str();
    if ( url.isEmpty() )
      url = currentURL();
    if ( url.isEmpty() )
      return;
    
    K2URL u( url );
    if ( strcmp( u.protocol(), "file" ) != 0L )
      return;
    
    QString index = findIndexFile( u.path() );
    if ( index.isEmpty() )
      return;
    
    // We can assume now that we are displaying some index.html or .kde.html file.
    openURL( url, 0, true );
  }
  else
  {
    ////////////////
    // Check whether we could perhaps show HTML for the current URL
    ////////////////
    QString url = m_strWorkingURL.c_str();
    if ( url.isEmpty() )
      url = currentURL();
    if ( url.isEmpty() )
      return;
    
    K2URL u( url );
    if ( strcmp( u.protocol(), "file" ) != 0L )
      return;
    
    QString index = findIndexFile( u.path() );
    if ( index.isEmpty() )
      return;

    // Ok, lets display the HTML stuff
    openURL( url, 0, true );
  }
}

QString KfmView::findIndexFile( const char *_path )
{
  QString path = _path;
  if ( path.right( 1 ) != "/" )
    path += "/";
  
  QString test = path.data();
  test += "index.html";

  cerr << "//////////////// 1. Testing " << test << endl;
  
  FILE *f = fopen( test, "rb" );
  if ( f != 0L )
  {
    fclose( f );
    return test;
  }

  test = path.data();
  test += ".kde.html";
  
  cerr << "//////////////// 2. Testing " << test << endl;

  f = fopen( test, "rb" );
  if ( f != 0L )
  {
    fclose( f );
    return test;
  }
  
  cerr << "//////////////// 3. Testing end" << endl;
  QString e;
  return e;
}

void KfmView::reload()
{
  switch( m_viewMode )
  {
  case FINDER:
  case HOR_ICONS:
  case VERT_ICONS:
    openURL( currentURL() );
    break;
  case HTML:
    m_pBrowser->slotReload();
    break;
  default:
    assert( 0 );
  }
}

void KfmView::slotGotFocus()
{
  if ( !m_hasFocus )
  {
    m_hasFocus = true;
    setFrameStyle( QFrame::Panel | QFrame::Plain );
    // setFrameStyle should call repaint but it doesn´t !
    repaint();
    
    emit gotFocus( this );
  }
}

void KfmView::clearFocus()
{
  if ( m_hasFocus )
  {
    m_hasFocus = false;
    setFrameStyle( QFrame::Panel | QFrame::Sunken );
    // setFrameStyle should call repaint but it doesn´t !
    repaint();
  }
  
  QWidgetStack::clearFocus();
}

#include "kfmview.moc"
