/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
 
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

#include <qdir.h>
#include <qclipboard.h>
#include <qapplication.h>
#include <qdragobject.h>

#include <opMenu.h>

#include <kbookmark.h>
#include <kdebug.h>
#include <kded_instance.h>
#include <kio_job.h>
#include <kio_openwith.h>
#include <kio_paste.h>
#include <kmessagebox.h>
#include <kpixmapcache.h>
#include <kprotocolmanager.h>
#include <krun.h>
#include <kservices.h>
#include <ktrader.h>
#include <kurl.h>
#include <kuserprofile.h>
#include <userpaths.h>
#include <kglobal.h>
#include <kstddirs.h>

#include "kpropsdlg.h"
#include "knewmenu.h"
#include "kpopupmenu.h"

KonqPopupMenu::KonqPopupMenu( KFileItemList items,
                              QString viewURL,
                              bool canGoBack,
                              bool canGoForward,
                              bool isMenubarHidden,
			      bool handleEditOperations,
			      bool canCopy,
			      bool canPaste,
			      bool canMove )
  : m_pMenuNew(0L), m_sViewURL(viewURL), m_lstItems(items)
{
  assert( m_lstItems.count() >= 1 );

  m_popupMenu = new OPMenu;
  m_bHandleEditOperations = handleEditOperations;
  bool bHttp          = true;
  bool isTrash        = true;
  bool currentDir     = false;
  bool isCurrentTrash = false;
  bool sReading       = true;
  bool sWriting       = true;
  bool sDeleting      = true;
  bool sMoving        = true;
  bool hasUpURL       = false;
  m_sMimeType         = m_lstItems.first()->mimetype();
  mode_t mode         = m_lstItems.first()->mode();
  m_lstPopupURLs.clear();
  int id;

  KProtocolManager pManager = KProtocolManager::self();
  
  KURL url;
  KFileItemListIterator it ( m_lstItems );
  // Check whether all URLs are correct
  for ( ; it.current(); ++it )
  {
    url = (*it)->url();

    // Build the list of URLs
    m_lstPopupURLs.append( url.url() );

    QString protocol = url.protocol();
    if ( protocol != "http" ) bHttp = false; // not HTTP

    // Determine if common mode among all URLs
    if ( mode != (*it)->mode() )
      mode = 0; // modes are different => reset to 0

    // Determine if common mimetype among all URLs
    if ( m_sMimeType != (*it)->mimetype() )
      m_sMimeType = QString::null; // mimetypes are different => null

    // check if all urls are in the trash
    if ( isTrash )
    {
      QString path = url.path();
      if ( path.at(path.length() - 1) != '/' )
	path += '/';
    
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
  url.cleanPath();
    
  if ( url.protocol() == "file" &&
       url.path(1) == UserPaths::trashPath() )
    isCurrentTrash = true;

  //check if url is current directory
  if ( m_lstItems.count() == 1 )
  {
    KURL firstPopupURL ( m_lstItems.first()->url() );
    firstPopupURL.cleanPath();
    kdebug(0, 1203, "View path is %s",url.path(1).ascii());
    kdebug(0, 1203, "First popup path is %s",firstPopupURL.path(1).ascii());
    if ( firstPopupURL.protocol() == url.protocol()
         && url.path(1) == firstPopupURL.path(1) )
    {
      currentDir = true;
      // ok, now check if we enable 'up'
      if ( url.hasPath() )
        hasUpURL = ( url.path(1) != "/");
    }
  }
  
  QObject::disconnect( m_popupMenu, SIGNAL( activated( int ) ), this, SLOT( slotPopup( int ) ) );

  m_popupMenu->clear();

  // check if menubar is hidden and if yes add "Show Menubar"
  if (isMenubarHidden)
  {
    m_popupMenu->insertItem( i18n("Show Menubar"), KPOPUPMENU_SHOWMENUBAR_ID );
    m_popupMenu->insertSeparator();
  }
  //------------------------------

  if ( isTrash )
  {
    id = m_popupMenu->insertItem( i18n( "New view" ), 
				  this, SLOT( slotPopupNewView() ) );
    m_popupMenu->insertSeparator();    
    id = m_popupMenu->insertItem( i18n( "Empty Trash Bin" ), 
				  this, SLOT( slotPopupEmptyTrashBin() ) );
  } 
  else if ( S_ISDIR( mode ) ) // all URLs are directories
  {
    //we don't want to use OpenParts here, because of "missing" interface 
    //methods for the popup menu (wouldn't make much sense imho) (Simon)    
    m_pMenuNew = new KNewMenu(); 
    id = m_popupMenu->insertItem( i18n("&New"), m_pMenuNew->popupMenu() );
    m_popupMenu->insertSeparator();

    if ( currentDir ) {
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "up.png" ), i18n( "Up" ), KPOPUPMENU_UP_ID );
      m_popupMenu->setItemEnabled( id, hasUpURL );

      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "back.png" ), i18n( "Back" ), KPOPUPMENU_BACK_ID );
      m_popupMenu->setItemEnabled( id, canGoBack );

      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "forward.png" ), i18n( "Forward" ), KPOPUPMENU_FORWARD_ID );
      m_popupMenu->setItemEnabled( id, canGoForward );

      m_popupMenu->insertSeparator();  
    }

    id = m_popupMenu->insertItem( i18n( "New View"), this, SLOT( slotPopupNewView() ) );
    m_popupMenu->insertSeparator();    

    if ( sReading && m_bHandleEditOperations )
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "editcopy.png" ), i18n( "Copy" ), this, SLOT( slotPopupCopy() ) );
    else if ( !m_bHandleEditOperations )
    {
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "editcopy.png" ), i18n( "Copy" ), KPOPUPMENU_COPY_ID );
      m_popupMenu->setItemEnabled( id, canCopy );
    }      
      
    if ( sWriting && m_bHandleEditOperations )
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "editpaste.png" ), i18n( "Paste" ), this, SLOT( slotPopupPaste() ) );
    else if ( !m_bHandleEditOperations )
    {
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "editpaste.png" ), i18n( "Paste" ), KPOPUPMENU_PASTE_ID );
      m_popupMenu->setItemEnabled( id, canPaste );
    }      
    
    if ( isClipboardEmpty() && m_bHandleEditOperations )
      m_popupMenu->setItemEnabled( id, false );
      
    if ( sMoving && !isCurrentTrash && !currentDir && m_bHandleEditOperations )
      id = m_popupMenu->insertItem( *KPixmapCache::pixmap( "kfm_trash.png", true ), i18n( "Move to trash" ), this, SLOT( slotPopupTrash() ) );
    else if ( !m_bHandleEditOperations )
    {
      id = m_popupMenu->insertItem( *KPixmapCache::pixmap( "kfm_trash.png", true ), i18n( "Move to trash" ), KPOPUPMENU_TRASH_ID );
      m_popupMenu->setItemEnabled( id, canMove );
    }      
      
    if ( sDeleting && !currentDir && m_bHandleEditOperations )
      id = m_popupMenu->insertItem( i18n( "Delete" ), this, SLOT( slotPopupDelete() ) );
    else if ( !m_bHandleEditOperations )
    {
      id = m_popupMenu->insertItem( i18n( "Delete" ), KPOPUPMENU_DELETE_ID );
      m_popupMenu->setItemEnabled( id, canMove );
    }
    
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

    if ( sReading && m_bHandleEditOperations )
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "editcopy.png" ), i18n( "Copy" ), this, SLOT( slotPopupCopy() ) );
    else if ( !m_bHandleEditOperations )
    {
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "editcopy.png" ), i18n( "Copy" ), KPOPUPMENU_COPY_ID );
      m_popupMenu->setItemEnabled( id, canCopy );
    }      

    if ( !m_bHandleEditOperations )
    {
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "editpaste.png" ), i18n( "Paste" ), KPOPUPMENU_PASTE_ID );
      m_popupMenu->setItemEnabled( id, canPaste );
    }      
      
    if ( sMoving && !isCurrentTrash && !currentDir && m_bHandleEditOperations )
      id = m_popupMenu->insertItem( *KPixmapCache::pixmap( "kfm_trash.png", true ), i18n( "Move to trash" ), this, SLOT( slotPopupTrash() ) );
    else if ( !m_bHandleEditOperations )
    {
      id = m_popupMenu->insertItem( *KPixmapCache::pixmap( "kfm_trash.png", true ), i18n( "Move to trash" ), KPOPUPMENU_TRASH_ID );
      m_popupMenu->setItemEnabled( id, canMove );
    }      
      
    if ( sDeleting && !currentDir && m_bHandleEditOperations )
      id = m_popupMenu->insertItem( i18n( "Delete" ), this, SLOT( slotPopupDelete() ) );
    else if ( !m_bHandleEditOperations )
    {
      id = m_popupMenu->insertItem( i18n( "Delete" ), KPOPUPMENU_DELETE_ID );
      m_popupMenu->setItemEnabled( id, canMove );
    }
    
  }

  id = m_popupMenu->insertItem( i18n( "Add To Bookmarks" ), this, SLOT( slotPopupAddToBookmark() ) );

  if ( m_pMenuNew ) m_pMenuNew->setPopupFiles( m_lstPopupURLs );

  bool bLastSepInserted = false;

  if ( !m_sMimeType.isNull() ) // common mimetype among all URLs ?
  {
    // Query the trader for offers associated to this mimetype
    KTrader* trader = KdedInstance::self()->ktrader();
       
    KTrader::OfferList offers = trader->query( m_sMimeType );

    QValueList<KDEDesktopMimeType::Service> builtin;
    QValueList<KDEDesktopMimeType::Service> user;
    if ( m_sMimeType == "application/x-desktop" ) // .desktop file (???)
    {
      builtin = KDEDesktopMimeType::builtinServices( m_lstItems.first()->url() );
      user = KDEDesktopMimeType::userDefinedServices( m_lstItems.first()->url() );
    }

    QStringList dirs = KGlobal::dirs()->findDirs( "data", "konqueror/servicemenus/" );
    QStringList::ConstIterator dIt = dirs.begin();
    QStringList::ConstIterator dEnd = dirs.end();
      
    for (; dIt != dEnd; ++dIt )
    {
      QDir dir( *dIt );
 
      QStringList entries = dir.entryList( QDir::Files );
      QStringList::ConstIterator eIt = entries.begin();
      QStringList::ConstIterator eEnd = entries.end();
	
      for (; eIt != eEnd; ++eIt )
      {
        KSimpleConfig cfg( *dIt + *eIt, true );
	  
        cfg.setDesktopGroup();
	
        if ( cfg.hasKey( "Actions" ) && cfg.hasKey( "ServiceTypes" ) &&
             cfg.readListEntry( "ServiceTypes" ).contains( m_sMimeType ) )
        {
          KURL u( *dIt + *eIt );
          user += KDEDesktopMimeType::userDefinedServices( u );
        }
	  
      }
	
    }
  
    if ( !offers.isEmpty() || !user.isEmpty() || !builtin.isEmpty() )
      QObject::connect( m_popupMenu, SIGNAL( activated( int ) ), this, SLOT( slotPopup( int ) ) );

    if ( !offers.isEmpty() || !user.isEmpty() )
      m_popupMenu->insertSeparator();
  
    m_mapPopup.clear();
    m_mapPopup2.clear();
  
    // KServiceTypeProfile::OfferList::Iterator it = offers.begin();
    KTrader::OfferList::Iterator it = offers.begin();
    for( ; it != offers.end(); it++ )
    {    
      id = m_popupMenu->insertItem( *(KPixmapCache::pixmap( (*it)->icon(), true ) ),
				    (*it)->name() );
      m_mapPopup[ id ] = *it;
    }
    
    QValueList<KDEDesktopMimeType::Service>::Iterator it2 = user.begin();
    for( ; it2 != user.end(); ++it2 )
    {
      if ( !(*it2).m_strIcon.isEmpty() )
	id = m_popupMenu->insertItem( *(KPixmapCache::pixmap( (*it2).m_strIcon, true ) ), (*it2).m_strName );
      else
	id = m_popupMenu->insertItem( (*it2).m_strName );
      m_mapPopup2[ id ] = *it2;
    }
    
    if ( builtin.count() > 0 )
      m_popupMenu->insertSeparator();

    it2 = builtin.begin();
    for( ; it2 != builtin.end(); ++it2 )
    {
      if ( !(*it2).m_strIcon.isEmpty() )
	id = m_popupMenu->insertItem( *(KPixmapCache::pixmap( (*it2).m_strIcon, true ) ), (*it2).m_strName );
      else
	id = m_popupMenu->insertItem( (*it2).m_strName );
      m_mapPopup2[ id ] = *it2;
    }

    bLastSepInserted = true;
    m_popupMenu->insertSeparator();
  
    id = m_popupMenu->insertItem( i18n( "Edit Mime Type" ), 
                                  this, SLOT( slotPopupMimeType() ) );
  }
  if ( PropertiesDialog::canDisplay( m_lstItems ) )
  {
    if (!bLastSepInserted) m_popupMenu->insertSeparator();
    m_popupMenu->insertItem( i18n("Properties"), this, SLOT( slotPopupProperties() ) );
  }
}

int KonqPopupMenu::exec( QPoint p )
{
  return m_popupMenu->exec( p );
}

KonqPopupMenu::~KonqPopupMenu()
{
  delete m_popupMenu;
  if ( m_pMenuNew ) delete m_pMenuNew;
}

void KonqPopupMenu::slotPopupNewView()
{
  QStringList::ConstIterator it = m_lstPopupURLs.begin();
  for ( ; it != m_lstPopupURLs.end(); it++ )
    (void) new KRun(*it);
}

void KonqPopupMenu::slotPopupEmptyTrashBin()
{
  QDir trashDir( UserPaths::trashPath() );
  QStringList files = trashDir.entryList( QDir::Files );
  
  QStringList::Iterator it = files.begin();
  QStringList::Iterator end = files.end();
  for (; it != end; ++it )
    (*it).prepend( UserPaths::trashPath() );
  
  KIOJob *job = new KIOJob;
  job->del( files );
}

void KonqPopupMenu::slotPopupCopy()
{
  QUriDrag *urlData = new QUriDrag;
  urlData->setUnicodeUris( m_lstPopupURLs );
  QApplication::clipboard()->setData( urlData );
}

void KonqPopupMenu::slotPopupPaste()
{
  assert( m_lstPopupURLs.count() == 1 );
  pasteClipboard( m_lstPopupURLs.first() );
}

void KonqPopupMenu::slotPopupTrash()
{
  KIOJob *job = new KIOJob;
  job->move( m_lstPopupURLs, UserPaths::trashPath() );
}

void KonqPopupMenu::slotPopupDelete()
{
  if ( KMessageBox::questionYesNo(0, i18n( "Do you really want to delete the file(s) ?" ))
       == KMessageBox::No)
    return;

  KIOJob *job = new KIOJob;
  QStringList lst(m_lstPopupURLs);
  job->del( lst );
}

void KonqPopupMenu::slotPopupOpenWith()
{
  OpenWithDlg l( m_lstPopupURLs, i18n("Open With:"), "", (QWidget*)0L);
  if ( l.exec() )
  {
    KService::Ptr service = l.service();
    if ( !!service )
    {
      KRun::run( *service, m_lstPopupURLs );
      return;
    }
    else
    {
      QString exec = l.text();
      exec += " %f";
      KRun::run( exec, m_lstPopupURLs );
    }
  }
}

void KonqPopupMenu::slotPopupAddToBookmark()
{
  KBookmark *root = KBookmarkManager::self()->root();
  QStringList::ConstIterator it = m_lstPopupURLs.begin();
  for ( ; it != m_lstPopupURLs.end(); it++ )
    (void)new KBookmark( KBookmarkManager::self(), root, *it, *it );
}

void KonqPopupMenu::slotPopup( int id )
{
  // Is it a usual service
  QMap<int,KService::Ptr>::Iterator it = m_mapPopup.find( id );
  if ( it != m_mapPopup.end() )
  {
    KRun::run( **it, m_lstPopupURLs );
    return;
  }
  
  // Is it a service specific to desktop entry files ?
  QMap<int,KDEDesktopMimeType::Service>::Iterator it2 = m_mapPopup2.find( id );
  if ( it2 == m_mapPopup2.end() )
    return;

  QStringList::Iterator it3 = m_lstPopupURLs.begin();
  for( ; it3 != m_lstPopupURLs.end(); ++it3 )
    KDEDesktopMimeType::executeService( *it3, it2.data() );

  return;
}

void KonqPopupMenu::slotPopupMimeType()
{
  QString mimeTypeFile = locate("mime", m_sMimeType + ".desktop");
  if ( mimeTypeFile.isEmpty() )
  {
    mimeTypeFile = locate("mime", m_sMimeType + ".kdelnk");
    if ( mimeTypeFile.isEmpty() )
    {
      mimeTypeFile = locate("mime", m_sMimeType );
      if ( mimeTypeFile.isEmpty() )
        return; // hmmm
    }
  }

  (void) new PropertiesDialog( mimeTypeFile  );
}

void KonqPopupMenu::slotPopupProperties()
{
  (void) new PropertiesDialog( m_lstItems );
}

#include "kpopupmenu.moc"
