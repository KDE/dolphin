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
#include <kxmlgui.h>
#include <kxmlguibuilder.h>

#include <assert.h>

#include "kpropsdlg.h"
#include "knewmenu.h"
#include "konqpopupmenu.h"

class KonqPopupMenuGUIBuilder : public KXMLGUIBuilder
{
public:
  KonqPopupMenuGUIBuilder( QPopupMenu *menu )
  : KXMLGUIBuilder( 0 )
  {
    m_menu = menu;
  }
  virtual ~KonqPopupMenuGUIBuilder()
  {
  }

  virtual QWidget *createContainer( QWidget *parent, int index,
          const QDomElement &element, const QByteArray &containerStateBuffer,
          int &id )
  {
    if ( !parent && element.attribute( "name" ) == "popupmenu" )
      return m_menu;

    return KXMLGUIBuilder::createContainer( parent, index, element, containerStateBuffer, id );
  }

  QPopupMenu *m_menu;
};

KonqPopupMenu::KonqPopupMenu( const KonqFileItemList &items,
                              KURL viewURL,
                              KActionCollection & actions,
                              KNewMenu * newMenu,
                              bool allowEmbeddingServices,
			      bool addTrailingSeparator )
  : QPopupMenu( 0L, "konq_popupmenu" ), m_actions( actions ), m_pMenuNew( newMenu ),
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

  setFont(KGlobal::menuFont());

  attrName = QString::fromLatin1( "name" );
  
  prepareXMLGUIStuff();

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

  KAction * act;

  addMerge( "konqueror" );

  m_paNewView = new KAction( i18n( "New View" ), 0, this, SLOT( slotPopupNewView() ), &m_ownActions, "newview" );

  if ( isCurrentTrash && currentDir )
  {
    addAction( m_paNewView );
    addSeparator();

    act = new KAction( i18n( "Empty Trash Bin" ), 0, this, SLOT( slotPopupEmptyTrashBin() ), &m_ownActions, "empytrash" );
    addAction( act );
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

	addAction( m_pMenuNew );
      }

      addSeparator();

      if ( currentDir ) {
        addAction( "up" );
	addAction( "back" );
	addAction( "forward" );
	addSeparator();
      }

      addAction( m_paNewView );
      addSeparator();
    }
    else // not all URLs are dirs
    {
      if ( m_sViewURL.protocol() == "http" )
      {
        /* Should be for http URLs (HTML pages) only ... */
        addAction( m_paNewView );
      }

      act = new KAction( i18n( "Open With..." ), 0, this, SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
      addAction( act );
      addSeparator();
    }

    if ( sReading )
    {
      addAction( "cut" );
      addAction( "copy" );
    }

    if ( sWriting ) {
      addAction( "paste" );
    }

    if ( sMoving && !currentDir ) {
      addAction( "trash" );
    }

    if ( sDeleting && !currentDir ) {
      addAction( "del" );
      if ( m_sViewURL.isLocalFile() )
       addAction( "shred" );
    }
  }

  act = new KAction( i18n( "Add To Bookmarks" ), 0, this, SLOT( slotPopupAddToBookmark() ), &m_ownActions, "addbookmark" );
  addAction( act );

  //////////////////////////////////////////////////////

  bool bLastSepInserted = false;

  if ( !m_sMimeType.isNull() ) // common mimetype among all URLs ?
  {
    // Query the trader for offers associated to this mimetype

    // 1 - Query for services that could embed it
    KTrader::OfferList embeddingOffers;
    if ( m_lstItems.count() == 1 && allowEmbeddingServices )
    { // Only if one item was selected
      // Can't use X-KDE-BrowserView-Builtin directly in the query because '-' is a substraction !!!
        embeddingOffers = KTrader::self()->query( m_sMimeType,
        "('Browser/View' in ServiceTypes) or ('KParts/ReadOnlyPart' in ServiceTypes)" );
    }

    // 2 - Look for builtin and user-defined services
    QValueList<KDEDesktopMimeType::Service> builtin;
    QValueList<KDEDesktopMimeType::Service> user;
    if ( m_sMimeType == "application/x-desktop" ) // .desktop file
    {
      // get builtin services, like mount/unmount
      builtin = KDEDesktopMimeType::builtinServices( m_lstItems.first()->url() );
      user = KDEDesktopMimeType::userDefinedServices( m_lstItems.first()->url() );
    }

    // 3 - Look for "servicesmenus" bindings (konqueror-specific user-defined services)
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

    // 4 - Query for applications
    KTrader::OfferList offers = KTrader::self()->query( m_sMimeType, "Type == 'Application'" );

    //// Ok, we have everything, now insert

    m_mapPopup.clear();
    m_mapPopupServices.clear();
    m_mapPopupEmbedding.clear();

    if ( !embeddingOffers.isEmpty() || !offers.isEmpty() || !user.isEmpty() || !builtin.isEmpty() )
    {
      // First block, embedding offers + app offers + user
      addSeparator();

      id = 1;

      bool insertedOffer = false;
      KTrader::OfferList::Iterator it = embeddingOffers.begin();
      for( ; it != embeddingOffers.end(); it++ )
      {
        QVariant builtin = (*it)->property( "X-KDE-BrowserView-Builtin" );
        if (!builtin.isValid() || !builtin.toBool()) // Either not specified or false
        {
	  QCString nam;
	  nam.setNum( id );
	
	  act = new KAction( i18n( "Preview in %1" ).arg( (*it)->name() ), 
			     (*it)->pixmap( KIconLoader::Small ), 0, this, SLOT( slotRunService() ), 
			     &m_ownActions, nam.prepend( "embeddservice_" ) );
	  addAction( act );
	
	  m_mapPopupEmbedding[ id++ ] = *it;
	
          insertedOffer = true;
        }
      }

      // KServiceTypeProfile::OfferList::Iterator it = offers.begin();
      it = offers.begin();
      for( ; it != offers.end(); it++ )
      {
        QCString nam;
	nam.setNum( id );

        act = new KAction( (*it)->name(), (*it)->pixmap( KIconLoader::Small ), 0, 
			   this, SLOT( slotRunService() ), 
			   &m_ownActions, nam.prepend( "appservice_" ) );
	addAction( act );
	
	m_mapPopup[ id++ ] = *it;

        insertedOffer = true;
      }

      QValueList<KDEDesktopMimeType::Service>::Iterator it2 = user.begin();
      for( ; it2 != user.end(); ++it2 )
      {
        if ((*it2).m_display == true)
        {
          QCString nam;
   	  nam.setNum( id );
	  act = new KAction( (*it2).m_strName, 0, this, SLOT( slotRunService() ), &m_ownActions, nam.prepend( "userservice_" ) );
			
          if ( !(*it2).m_strIcon.isEmpty() )
          {
            QPixmap pix = KGlobal::iconLoader()->loadIcon( (*it2).m_strIcon, KIconLoader::Small );
	    act->setIconSet( pix );
          }
	
	  addAction( act );
	
          m_mapPopupServices[ id++ ] = *it2;
          insertedOffer = true;
        }
      }

      // Second block, builtin
      if ( insertedOffer )
        addSeparator();

      it2 = builtin.begin();
      for( ; it2 != builtin.end(); ++it2 )
      {
        QCString nam;
	nam.setNum( id );
	
        act = new KAction( (*it2).m_strName, 0, this, SLOT( slotService() ), &m_ownActions, nam.prepend( "builtinservice_" ) );
	
        if ( !(*it2).m_strIcon.isEmpty() )
        {
          QPixmap pix = KGlobal::iconLoader()->loadIcon( (*it2).m_strIcon, KIconLoader::Small );
	  act->setIconSet( pix );
        }
	
	addAction( act );
	
        m_mapPopupServices[ id++ ] = *it2;
      }
    }
    else
      addSeparator();

    bLastSepInserted = true;

    //  or "File Type Properties" ?
    act = new KAction( i18n( "Edit File Type..." ), 0, this, SLOT( slotPopupMimeType() ), 
		       &m_ownActions, "editfiletype" );
    addAction( act );
  }

  if ( PropertiesDialog::canDisplay( m_lstItems ) )
  {
    if ( !bLastSepInserted ) addSeparator();
    
    act = new KAction( i18n( "Properties..." ), 0, this, SLOT( slotPopupProperties() ), 
		       &m_ownActions, "properties" );
    addAction( act );
  }

  if ( addTrailingSeparator )
    addSeparator();
  
  addMerge( 0 );
  
  m_factory->addClient( this );
}

KonqPopupMenu::~KonqPopupMenu()
{
  delete m_factory;
  delete m_builder;
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

void KonqPopupMenu::slotRunService()
{
  QCString senderName = sender()->name();
  int id = senderName.mid( senderName.find( '_' ) + 1 ).toInt();

  // Is it a usual service (application)
  QMap<int,KService::Ptr>::Iterator it = m_mapPopup.find( id );
  if ( it != m_mapPopup.end() )
  {
    KRun::run( **it, m_lstPopupURLs );
    return;
  }

  // Is it an embedding service ?
  QMap<int,KService::Ptr>::Iterator itEmbed = m_mapPopupEmbedding.find( id );
  if ( itEmbed != m_mapPopupEmbedding.end() )
  {
    emit openEmbedded( m_sMimeType, m_lstPopupURLs.first(), (*itEmbed)->name() );
    return;
  }

  // Is it a service specific to desktop entry files ?
  QMap<int,KDEDesktopMimeType::Service>::Iterator it2 = m_mapPopupServices.find( id );
  if ( it2 != m_mapPopupServices.end() )
  {
    KURL::List::Iterator it3 = m_lstPopupURLs.begin();
    for( ; it3 != m_lstPopupURLs.end(); ++it3 )
      KDEDesktopMimeType::executeService( (*it3).path(), it2.data() );
  }

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

KAction *KonqPopupMenu::action( const QDomElement &element ) const
{
  QString name = element.attribute( attrName ); 

  KAction *res = m_ownActions.action( name );

  if ( !res )
    res = m_actions.action( name );

  if ( !res && strcmp( name, m_pMenuNew->name() ) == 0 )
    return m_pMenuNew;

  return res;
}

QDomDocument KonqPopupMenu::document() const
{
  return m_doc;
}

void KonqPopupMenu::addAction( KAction *act )
{
  addAction( act->name() );
}

void KonqPopupMenu::addAction( const char *name )
{
  static QString tagAction = QString::fromLatin1( "action" ); 
  QDomElement e = m_doc.createElement( tagAction );
  m_menuElement.appendChild( e );
  e.setAttribute( attrName, name );
}

void KonqPopupMenu::addSeparator()
{
  static QString tagSeparator = QString::fromLatin1( "separator" ); 
  m_menuElement.appendChild( m_doc.createElement( tagSeparator ) );
}

void KonqPopupMenu::addMerge( const char *name )
{
  static QString tagMerge = QString::fromLatin1( "merge" ); 
  QDomElement merge = m_doc.createElement( tagMerge );
  m_menuElement.appendChild( merge );
  if ( name )
    merge.setAttribute( attrName, name );
} 

void KonqPopupMenu::prepareXMLGUIStuff()
{
  m_doc = QDomDocument( "kpartgui" );

  QDomElement root = m_doc.createElement( "kpartgui" );
  m_doc.appendChild( root );
  root.setAttribute( attrName, "popupmenu" );

  m_menuElement = m_doc.createElement( "Menu" );
  root.appendChild( m_menuElement );
  m_menuElement.setAttribute( attrName, "popupmenu" );

  m_builder = new KonqPopupMenuGUIBuilder( this );
  m_factory = new KXMLGUIFactory( m_builder );
}

#include "konqpopupmenu.moc"
