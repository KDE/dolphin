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

#include <kbookmark.h>
#include <kdebug.h>
#include <kio/job.h>
#include <kio/paste.h>
#include <kopenwith.h>
#include <kmessagebox.h>
#include <kprotocolmanager.h>
#include <krun.h>
#include <kservice.h>
#include <ktrader.h>
#include <kurl.h>
#include <kuserprofile.h>
#include <kglobalsettings.h>
#include <kglobal.h>
#include <kstddirs.h>

#include <assert.h>

#include "kpropsdlg.h"
#include "knewmenu.h"
#include "konqpopupmenu.h"

KonqPopupMenu::KonqPopupMenu( const KonqFileItemList &items,
                              KURL viewURL,
                              QActionCollection & actions,
                              KNewMenu * newMenu )
  : QPopupMenu( 0L, "konq_popupmenu" ), m_actions( actions), m_pMenuNew( newMenu ),
    m_sViewURL(viewURL), m_lstItems(items)
{
  assert( m_lstItems.count() >= 1 );

  bool currentDir     = false;
  bool sReading       = true;
  bool sWriting       = true;
  bool sDeleting      = true;
  bool sMoving        = true;
  m_sMimeType         = m_lstItems.first()->mimetype();
  mode_t mode         = m_lstItems.first()->mode();
  m_lstPopupURLs.clear();
  int id;

  KProtocolManager pManager = KProtocolManager::self();

  KURL url;
  KonqFileItemListIterator it ( m_lstItems );
  // Check whether all URLs are correct
  for ( ; it.current(); ++it )
  {
    url = (*it)->url();

    // Build the list of URLs
    m_lstPopupURLs.append( url );

    // Determine if common mode among all URLs
    if ( mode != (*it)->mode() )
      mode = 0; // modes are different => reset to 0

    // Determine if common mimetype among all URLs
    if ( m_sMimeType != (*it)->mimetype() )
      m_sMimeType = QString::null; // mimetypes are different => null

    QString protocol = url.protocol();

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
  url = m_sViewURL;
  url.cleanPath();

  bool isCurrentTrash = ( url.isLocalFile() &&
                          url.path(1) == KGlobalSettings::trashPath() );

  //check if url is current directory
  if ( m_lstItems.count() == 1 )
  {
    KURL firstPopupURL ( m_lstItems.first()->url() );
    firstPopupURL.cleanPath();
    //kdDebug(1203) << "View path is " << url.url() << endl;
    //kdDebug(1203) << "First popup path is " << firstPopupURL.url() << endl;
    currentDir = firstPopupURL.cmp( url, true /* ignore_trailing */ );
  }

  QObject::disconnect( this, SIGNAL( activated( int ) ), this, SLOT( slotPopup( int ) ) );

  clear();

  //////////////////////////////////////////////////////////////////////////

  // Now let's plug the actions we want to plug.
  // Remember, the caller did all the setEnabled stuff, we shouldn't do
  // any of it here.

  QAction * act;
  if ( ( act = m_actions.action("showmenubar") ) )
  {
    act->plug( this );
    insertSeparator();
  }
  //------------------------------

  if ( ( act = m_actions.action( "fullscreenstop" ) ) )
  {
    act->plug( this );
    insertSeparator();
  }

  if ( isCurrentTrash && currentDir )
  {
    id = insertItem( i18n( "New View" ),
				  this, SLOT( slotPopupNewView() ) );
    insertSeparator();
    id = insertItem( i18n( "Empty Trash Bin" ),
				  this, SLOT( slotPopupEmptyTrashBin() ) );
  }
  else
  {
    if ( S_ISDIR( mode ) ) // all URLs are directories
    {
      // Add the "new" menu
      if ( m_pMenuNew )
      {
        // As requested by KNewMenu :
        m_pMenuNew->slotCheckUpToDate();
        m_pMenuNew->setPopupFiles( m_lstPopupURLs );
        m_pMenuNew->plug( this );
      }
      insertSeparator();

      if ( currentDir ) {
        if ( ( act = m_actions.action("up") ) )
          act->plug( this );

        if ( ( act = m_actions.action("back") ) )
          act->plug( this );

        if ( ( act = m_actions.action("forward") ) )
          act->plug( this );

        insertSeparator();
      }

      id = insertItem( i18n( "New View"), this, SLOT( slotPopupNewView() ) );
      insertSeparator();

    }
    else // not all URLs are dirs
    {
      if ( m_sViewURL.protocol() == "http" )
      {
        /* Should be for http URLs (HTML pages) only ... */
        id = insertItem( i18n( "New View"), this, SLOT( slotPopupNewView() ) );
      }
      id = insertItem( i18n( "Open With..." ), this, SLOT( slotPopupOpenWith() ) );
      insertSeparator();
    }

    if ( sReading )
    {
      if ( ( act = m_actions.action( "cut" ) ) )
        act->plug( this );
      if ( ( act = m_actions.action("copy") ) )
        act->plug( this );
    }

    if ( sWriting ) {
      if ( ( act = m_actions.action("paste") ) )
        act->plug( this );
    }

    if ( sMoving && !currentDir ) {
      if ( ( act = m_actions.action("trash") ) )
        act->plug( this );
    }

    if ( sDeleting && !currentDir ) {
      if ( ( act = m_actions.action("del") ) )
        act->plug( this );
      if ( m_sViewURL.isLocalFile() && ( act = m_actions.action("shred") ) )
        act->plug( this );
    }
  }

  id = insertItem( i18n( "Add To Bookmarks" ), this, SLOT( slotPopupAddToBookmark() ) );

  //////////////////////////////////////////////////////

  bool bLastSepInserted = false;

  if ( !m_sMimeType.isNull() ) // common mimetype among all URLs ?
  {
    // Query the trader for offers associated to this mimetype

    KTrader::OfferList offers = KTrader::self()->query( m_sMimeType, "Type == 'Application'" );

    QValueList<KDEDesktopMimeType::Service> builtin;
    QValueList<KDEDesktopMimeType::Service> user;
    if ( m_sMimeType == "application/x-desktop" ) // .desktop file
    {
      // get builtin services, like mount/unmount
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
      QObject::connect( this, SIGNAL( activated( int ) ), this, SLOT( slotPopup( int ) ) );

    if ( !offers.isEmpty() || !user.isEmpty() )
      insertSeparator();

    m_mapPopup.clear();
    m_mapPopup2.clear();

    // KServiceTypeProfile::OfferList::Iterator it = offers.begin();
    KTrader::OfferList::Iterator it = offers.begin();
    for( ; it != offers.end(); it++ )
    {
      id = insertItem( (*it)->pixmap( KIconLoader::Small ), (*it)->name() );
      m_mapPopup[ id ] = *it;
    }

    QValueList<KDEDesktopMimeType::Service>::Iterator it2 = user.begin();
    for( ; it2 != user.end(); ++it2 )
    {
      if ((*it2).m_display == true)
      {
        if ( !(*it2).m_strIcon.isEmpty() )
        {
          QPixmap pix = KGlobal::iconLoader()->loadIcon( (*it2).m_strIcon, KIconLoader::Small );
    id = insertItem( pix, (*it2).m_strName );
        }
        else
          id = insertItem( (*it2).m_strName );
        m_mapPopup2[ id ] = *it2;
      }
    }

    if ( builtin.count() > 0 )
      insertSeparator();

    it2 = builtin.begin();
    for( ; it2 != builtin.end(); ++it2 )
    {
      if ( !(*it2).m_strIcon.isEmpty() )
      {
        QPixmap pix = KGlobal::iconLoader()->loadIcon( (*it2).m_strIcon, KIconLoader::Small );
	id = insertItem( pix, (*it2).m_strName );
      }
      else
	id = insertItem( (*it2).m_strName );
      m_mapPopup2[ id ] = *it2;
    }

    bLastSepInserted = true;
    insertSeparator();

    id = insertItem( i18n( "Edit File Type..." ), // or "File Type Properties" ?
                     this, SLOT( slotPopupMimeType() ) );
  }
  if ( PropertiesDialog::canDisplay( m_lstItems ) )
  {
    if (!bLastSepInserted) insertSeparator();
    insertItem( i18n("Properties..."), this, SLOT( slotPopupProperties() ) );
  }
}

KonqPopupMenu::~KonqPopupMenu()
{
}

void KonqPopupMenu::slotPopupNewView()
{
  KURL::List::ConstIterator it = m_lstPopupURLs.begin();
  for ( ; it != m_lstPopupURLs.end(); it++ )
    (void) new KRun(*it);
}

void KonqPopupMenu::slotPopupEmptyTrashBin()
{
  QDir trashDir( KGlobalSettings::trashPath() );
  QStringList files = trashDir.entryList( QDir::Files && QDir::Dirs );
  files.remove(QString("."));
  files.remove(QString(".."));

  QStringList::Iterator it(files.begin());
  for (; it != files.end(); ++it )
  {
    (*it).prepend( KGlobalSettings::trashPath() );
  }

  KIO::Job *job = KIO::del( files );
  connect( job, SIGNAL( result( KIO::Job * ) ),
           SLOT( slotResult( KIO::Job * ) ) );
}

void KonqPopupMenu::slotResult( KIO::Job * job )
{
  if (job->error())
    job->showErrorDialog();
}

void KonqPopupMenu::slotPopupOpenWith()
{
  KOpenWithDlg l( m_lstPopupURLs, i18n("Open With:"), "", (QWidget*)0L);
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
  KURL::List::ConstIterator it = m_lstPopupURLs.begin();
  for ( ; it != m_lstPopupURLs.end(); it++ )
    (void)new KBookmark( KBookmarkManager::self(), root, (*it).decodedURL(), *it );
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

  KURL::List::Iterator it3 = m_lstPopupURLs.begin();
  for( ; it3 != m_lstPopupURLs.end(); ++it3 )
    KDEDesktopMimeType::executeService( (*it3).path(), it2.data() );

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

#include "konqpopupmenu.moc"
