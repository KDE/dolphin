#include <opMenu.h>

#include <kio_job.h>
#include <kio_openwith.h>
#include <kio_paste.h>
#include <kio_propsdlg.h>
#include <kpixmapcache.h>
#include <kprotocolmanager.h>
#include <kservices.h>
#include <kurl.h>
#include <kuserprofile.h>
#include <userpaths.h>

#include "knewmenu.h"
#include "kfmrun.h"
#include "kpopupmenu.h"

KonqPopupMenu::KonqPopupMenu( const Konqueror::View::MenuPopupRequest &popup, 
                              QString viewURL, 
                              bool canGoBack, 
                              bool canGoForward, 
                              bool canGoUp )
  : m_pMenuNew(0L), m_sViewURL(viewURL), m_popupMode(popup.mode)
{
  assert( popup.urls.length() >= 1 );

  OPMenu *m_popupMenu = new OPMenu;
  bool bHttp          = true;
  bool isTrash        = true;
  bool currentDir     = false;
  bool isCurrentTrash = false;
  bool sReading       = true;
  bool sWriting       = true;
  bool sDeleting      = true;
  bool sMoving        = true;
  int id;

  KProtocolManager pManager = KProtocolManager::self();
  
  KURL url;
  CORBA::ULong i;
  // Check whether all URLs are correct
  for ( i = 0; i < popup.urls.length(); i++ )
  {
    url = KURL( popup.urls[i].in() );

    if ( url.isMalformed() )
    {
//FIXME?
//      emit error( ERR_MALFORMED_URL, s );
      return;
    }
    QString protocol = url.protocol();

    if ( protocol != "http" ) bHttp = false; // not HTTP

    // check if all urls are in the trash
    if ( isTrash )
    {
      QString path = url.path();
      if ( path.right(1) != "/" )
	path += "/";
    
      if ( protocol != "file" ||
	   path != UserPaths::trashPath() )
	isTrash = false;
    }

    if ( sReading )
      sReading = pManager.supportsReading( protocol );

    if ( sWriting )
      sWriting = pManager.supportsWriting( protocol );
    
    if ( sDeleting )
      sDeleting = pManager.supportsDeleting( protocol );

    if ( sMoving )
      sMoving = pManager.supportsMoving( protocol );
  }

  //check if current url is trash
  url = KURL( m_sViewURL );
  QString path = url.path();
  if ( path.right(1) != "/" )
    path += "/";
    
  if ( url.protocol() == "file" &&
       path == UserPaths::trashPath() )
    isTrash = true;

  //check if url is current directory
  if ( popup.urls.length() == 1 )
    if ( m_sViewURL == ((popup.urls)[0]) )
      currentDir = true;

  QObject::disconnect( m_popupMenu, SIGNAL( activated( int ) ), this, SLOT( slotPopup( int ) ) );

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
  else if ( S_ISDIR( (mode_t)popup.mode ) )
  {
    //we don't want to use OpenParts here, because of "missing" interface 
    //methods for the popup menu (wouldn't make much sense imho) (Simon)    
    m_pMenuNew = new KNewMenu(); 
    id = m_popupMenu->insertItem( i18n("&New"), m_pMenuNew->popupMenu() );
    m_popupMenu->insertSeparator();

    id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "up.xpm" ), i18n( "Up" ), this, SLOT( slotUp() ), 100 );
    m_popupMenu->setItemEnabled( id, canGoUp );

    id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "back.xpm" ), i18n( "Back" ), this, SLOT( slotBack() ), 101 );
    m_popupMenu->setItemEnabled( id, canGoBack );

    id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "forward.xpm" ), i18n( "Forward" ), this, SLOT( slotForward() ), 102 );
    m_popupMenu->setItemEnabled( id, canGoForward );

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

  m_lstPopupURLs.clear();
  for ( i = 0; i < popup.urls.length(); i++ )
    m_lstPopupURLs.append( (popup.urls)[i].in() );

  if ( m_pMenuNew ) m_pMenuNew->setPopupFiles( m_lstPopupURLs );

  // Do all URLs have the same mimetype ?
  url = KURL( m_lstPopupURLs.getFirst() );


  KMimeType* mime = KMimeType::findByURL( url, (mode_t)popup.mode, (bool)popup.isLocalFile );
  ASSERT( mime );
  QStringList::Iterator it = m_lstPopupURLs.begin();
  for( ; it != m_lstPopupURLs.end(); ++it )
  {
    KURL u( *it );  
    KMimeType* m = KMimeType::findByURL( u, (mode_t)popup.mode, (bool)popup.isLocalFile );
    if ( m != mime )
      mime = 0L;
  }
  
  if ( mime )
  {
    KServiceTypeProfile::OfferList offers = KServiceTypeProfile::offers( mime->name() );

    QValueList<KDELnkMimeType::Service> builtin;
    QValueList<KDELnkMimeType::Service> user;
    if ( mime->name() == "application/x-kdelnk" )
    {
      builtin = KDELnkMimeType::builtinServices( url );
      user = KDELnkMimeType::userDefinedServices( url );
    }
  
    if ( !offers.isEmpty() || !user.isEmpty() || !builtin.isEmpty() )
      QObject::connect( m_popupMenu, SIGNAL( activated( int ) ), this, SLOT( slotPopup( int ) ) );

    if ( !offers.isEmpty() || !user.isEmpty() )
      m_popupMenu->insertSeparator();
  
    m_mapPopup.clear();
    m_mapPopup2.clear();
  
    KServiceTypeProfile::OfferList::Iterator it = offers.begin();
    for( ; it != offers.end(); it++ )
    {    
      id = m_popupMenu->insertItem( *(KPixmapCache::pixmap( it->service().icon(), true ) ),
				    it->service().name() );
      m_mapPopup[ id ] = &(it->service());
    }
    
    QValueList<KDELnkMimeType::Service>::Iterator it2 = user.begin();
    for( ; it2 != user.end(); ++it2 )
    {
      if ( !it2->m_strIcon.isEmpty() )
	id = m_popupMenu->insertItem( *(KPixmapCache::pixmap( it2->m_strIcon, true ) ), it2->m_strName );
      else
	id = m_popupMenu->insertItem( it2->m_strName );
      m_mapPopup2[ id ] = *it2;
    }
    
    if ( builtin.count() > 0 )
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
  
  if ( m_lstPopupURLs.count() == 1 )
  {
    m_popupMenu->insertSeparator();
    m_popupMenu->insertItem( i18n("Properties"), this, SLOT( slotPopupProperties() ) );
  }

  // m_popupMenu->insertSeparator();
  // Konqueror::View::EventCreateViewMenu viewMenu;
  // viewMenu.menu = OpenPartsUI::Menu::_duplicate( m_popupMenu->interface() );
  // EMIT_EVENT( m_currentView->m_vView, Konqueror::View::eventCreateViewMenu, viewMenu );
    
  m_popupMenu->exec( QPoint( popup.x, popup.y ) );

  //  viewMenu.menu = OpenPartsUI::Menu::_nil();
  // EMIT_EVENT( m_currentView->m_vView, Konqueror::View::eventCreateViewMenu, viewMenu );
    
  delete m_popupMenu;
  if ( m_pMenuNew ) delete m_pMenuNew;
}


void KonqPopupMenu::slotFileNewActivated( CORBA::Long id )
{
  if ( m_pMenuNew )
     {
       QStringList urls;
       urls.append( m_sViewURL );

       m_pMenuNew->setPopupFiles( urls );

       m_pMenuNew->slotNewFile( (int)id );
     }
}

void KonqPopupMenu::slotFileNewAboutToShow()
{
  if ( m_pMenuNew )
    m_pMenuNew->slotCheckUpToDate();
}


void KonqPopupMenu::slotPopupNewView()
{
  assert( m_lstPopupURLs.count() == 1 );
  //  emit sigNewView ( m_lstPopupURLs.getFirst() );
  
  /* KonqMainWindow *m_pShell = new KonqMainWindow( m_lstPopupURLs.getFirst() );
    m_pShell->show();
  */
}

void KonqPopupMenu::slotPopupEmptyTrashBin()
{
  //TODO
}

void KonqPopupMenu::slotPopupCopy()
{
  // TODO (kclipboard.h will probably have to be ported to QStringList)
  //  KClipboard::self()->setURLList( m_lstPopupURLs ); 
}

void KonqPopupMenu::slotPopupPaste()
{
  assert( m_lstPopupURLs.count() == 1 );
  pasteClipboard( m_lstPopupURLs.getFirst() );
}

void KonqPopupMenu::slotPopupTrash()
{
  //TODO
}

void KonqPopupMenu::slotPopupDelete()
{
  KIOJob *job = new KIOJob;
  list<string> lst;
  QStringList::Iterator it = m_lstPopupURLs.begin();
  for ( ; it != m_lstPopupURLs.end(); ++it )
    lst.push_back( it->data() );
  job->del( lst );
}

void KonqPopupMenu::slotPopupOpenWith()
{
  OpenWithDlg l( i18n("Open With:"), "", (QWidget*)0L, true );
  if ( l.exec() )
  {
    KService *service = l.service();
    if ( service )
    {
      KfmRun::run( *service, m_lstPopupURLs );
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

void KonqPopupMenu::slotPopupBookmarks()
{
  //TODO
}

void KonqPopupMenu::slotPopup( int id )
{
  // Is it a usual service
  QMap<int,const KService *>::Iterator it = m_mapPopup.find( id );
  if ( it != m_mapPopup.end() )
  {
    KRun::run( *(it.data()), m_lstPopupURLs );
    return;
  }
  
  // Is it a service specific to kdelnk files ?
  QMap<int,KDELnkMimeType::Service>::Iterator it2 = m_mapPopup2.find( id );
  if ( it2 == m_mapPopup2.end() )
    return;

  QStringList::Iterator it3 = m_lstPopupURLs.begin();
  for( ; it3 != m_lstPopupURLs.end(); ++it3 )
    KDELnkMimeType::executeService( *it3, it2.data() );

  return;
}

void KonqPopupMenu::slotPopupProperties()
{
  if ( m_lstPopupURLs.count() == 1 )
    (void) new PropertiesDialog( m_lstPopupURLs.getFirst(), m_popupMode );
  // else ERROR
}

#include "kpopupmenu.moc"
