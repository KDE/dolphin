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

#include <klocale.h>
#include <krun.h>
#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kdebug.h>
#include <kio/job.h>
#include <kio/paste.h>
#include <kopenwith.h>
#include <kmessagebox.h>
#include <kprotocolinfo.h>
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
#include "konq_popupmenu.h"
#include "konq_operations.h"

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
          const QDomElement &element,
          int &id )
  {
    if ( !parent && element.attribute( "name" ) == "popupmenu" )
      return m_menu;

    return KXMLGUIBuilder::createContainer( parent, index, element, id );
  }

  QPopupMenu *m_menu;
};

KonqPopupMenu::KonqPopupMenu( const KFileItemList &items,
                              KURL viewURL,
                              KActionCollection & actions,
                              KNewMenu * newMenu,
                  bool showPropertiesAndFileType )
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
  bool bTrashIncluded = false;
  m_lstPopupURLs.clear();
  int id = 0;

  setFont(KGlobalSettings::menuFont());

  m_ownActions.setHighlightingEnabled( true );

  attrName = QString::fromLatin1( "name" );

  prepareXMLGUIStuff();

  KURL url;
  KFileItemListIterator it ( m_lstItems );
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

    if ( !bTrashIncluded &&
         (*it)->url().isLocalFile() &&
         (*it)->url().path( 1 ) == KGlobalSettings::trashPath() )
        bTrashIncluded = true;

    if ( sReading )
      sReading = KProtocolInfo::supportsReading( url );

    if ( sWriting )
      sWriting = KProtocolInfo::supportsWriting( url );

    if ( sDeleting )
      sDeleting = KProtocolInfo::supportsDeleting( url );

    if ( sMoving )
      sMoving = KProtocolInfo::supportsMoving( url );
  }
  // Be on the safe side when including the trash
  if ( bTrashIncluded )
  {
      sMoving = false;
      sDeleting = false;
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

  clear();

  //////////////////////////////////////////////////////////////////////////

  KAction * act;

  addMerge( "konqueror" );

  m_paNewView = new KAction( i18n( "New Window" ), "window_new", 0, this, SLOT( slotPopupNewView() ), &m_ownActions, "newview" );
  m_paNewView->setStatusText( i18n( "Open the document in a new window" ) );

  if ( ( isCurrentTrash && currentDir ) ||
       ( m_lstItems.count() == 1 && bTrashIncluded ) )
  {
    addAction( m_paNewView );
    addSeparator();

    act = new KAction( i18n( "Empty Trash Bin" ), 0, this, SLOT( slotPopupEmptyTrashBin() ), &m_ownActions, "empytrash" );
    addAction( act );
  }
  else
  {
    // hack for khtml pages/frames
    bool httpPage = (m_sViewURL.protocol().find("http", 0, false) == 0);

    //kdDebug() << "httpPage=" << httpPage << " dir=" << S_ISDIR( mode ) << " currentDir=" << currentDir << endl;

    if ( S_ISDIR( mode ) ) // all URLs are directories (also set for rmb-on-background of html pages)
    {
      if ( currentDir && sWriting && m_pMenuNew ) // Add the "new" menu
      {
        // As requested by KNewMenu :
        m_pMenuNew->slotCheckUpToDate();
        m_pMenuNew->setPopupFiles( m_lstPopupURLs );

        addAction( m_pMenuNew );

        addSeparator();
      }

      if ( currentDir || httpPage )
      {
        addAction( "up" );
        addAction( "back" );
        addAction( "forward" );
        if ( currentDir )
            addAction( "reload" );
        addGroup( "reload" );
        addSeparator();
      }

      addAction( m_paNewView );
      addSeparator();
    }
    else // not all URLs are dirs
    {
      if ( httpPage ) // HTML link
      {
        addAction( m_paNewView );
        addSeparator();
      }
    }

    if ( sReading ) {
      if ( sDeleting ) {
        addAction( "undo" );
        addAction( "cut" );
      }
      addAction( "copy" );
    }

    if ( sWriting ) {
      addAction( "paste" );
    }

    // The actions in this group are defined in PopupMenuGUIClient
    // When defined, it includes a separator before the 'find' action
    addGroup( "find" );

    if (!currentDir)
    {
        if ( sReading || sWriting ) // only if we added an action above
            addSeparator();

        if ( m_lstItems.count() == 1 && sWriting )
            addAction("rename");

        if ( sMoving )
            addAction( "trash" );

        if ( sDeleting ) {
            addAction( "del" );
            //if ( m_sViewURL.isLocalFile() )
            //    addAction( "shred" );
        }
    }
  }

  act = new KAction( i18n( "Add To Bookmarks" ), "bookmark_add", 0, this, SLOT( slotPopupAddToBookmark() ), &m_ownActions, "addbookmark" );
  addAction( act );

  //////////////////////////////////////////////////////

  QValueList<KDEDesktopMimeType::Service> builtin;
  QValueList<KDEDesktopMimeType::Service> user;

  // 1 - Look for builtin and user-defined services
  if ( m_sMimeType == "application/x-desktop" && m_lstItems.count() == 1 && m_lstItems.first()->url().isLocalFile() ) // .desktop file
  {
      // get builtin services, like mount/unmount
      builtin = KDEDesktopMimeType::builtinServices( m_lstItems.first()->url() );
      user = KDEDesktopMimeType::userDefinedServices( m_lstItems.first()->url().path(), url.isLocalFile() );
  }

  // 2 - Look for "servicesmenus" bindings (konqueror-specific user-defined services)
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

          if ( cfg.hasKey( "Actions" ) && cfg.hasKey( "ServiceTypes" ) )
          {
              if ( ( !m_sMimeType.isNull() && cfg.readListEntry( "ServiceTypes" ).contains( m_sMimeType ) )
                   || ( cfg.readEntry( "ServiceTypes" ) == "allfiles" ) )
              {
                  user += KDEDesktopMimeType::userDefinedServices( *dIt + *eIt, url.isLocalFile() );
              }
          }
      }
  }

  KTrader::OfferList offers;
  if ( !m_sMimeType.isNull() ) // common mimetype among all URLs ?
  {
      // 3 - Query for applications
      offers = KTrader::self()->query( m_sMimeType,
   "Type == 'Application' and DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'" );
  }

  //// Ok, we have everything, now insert

  m_mapPopup.clear();
  m_mapPopupServices.clear();

  if ( !offers.isEmpty() )
  {
      // First block, app and preview offers
      addSeparator();

      id = 1;

      QDomElement menu = m_menuElement;

      if ( offers.count() > 1 ) // submenu 'open with'
      {
        menu = m_doc.createElement( "menu" );
        m_menuElement.appendChild( menu );
        QDomElement text = m_doc.createElement( "text" );
        menu.appendChild( text );
        text.appendChild( m_doc.createTextNode( i18n("Open With") ) );
      }

      if ( menu == m_menuElement ) // no submenu -> open with... above the single offer
      {
        KAction *openWithAct = new KAction( i18n( "Open With..." ), 0, this, SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
        addAction( openWithAct, menu );
      }

      KTrader::OfferList::ConstIterator it = offers.begin();
      for( ; it != offers.end(); it++ )
      {
        QCString nam;
        nam.setNum( id );

        act = new KAction( (*it)->name(), (*it)->pixmap( KIcon::Small ), 0,
                           this, SLOT( slotRunService() ),
                           &m_ownActions, nam.prepend( "appservice_" ) );
        addAction( act, menu );

        m_mapPopup[ id++ ] = *it;
      }

      if ( menu != m_menuElement ) // submenu
      {
        addSeparator( menu );
        KAction *openWithAct = new KAction( i18n( "Other..." ), 0, this, SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
        addAction( openWithAct, menu ); // Other...
      }
  }
  else // no app offers -> Open With...
  {
      addSeparator();
      act = new KAction( i18n( "Open With..." ), 0, this, SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
      addAction( act );
  }

  addGroup( "preview" );

  addSeparator();

  // Second block, builtin + user
  if ( !user.isEmpty() || !builtin.isEmpty() )
  {
      bool insertedOffer = false;

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
            QPixmap pix = SmallIcon( (*it2).m_strIcon );
            act->setIconSet( pix );
          }

          addAction( act, m_menuElement ); // Add to toplevel menu

          m_mapPopupServices[ id++ ] = *it2;
          insertedOffer = true;
        }
      }

      it2 = builtin.begin();
      for( ; it2 != builtin.end(); ++it2 )
      {
        QCString nam;
        nam.setNum( id );

        act = new KAction( (*it2).m_strName, 0, this, SLOT( slotRunService() ), &m_ownActions, nam.prepend( "builtinservice_" ) );

        if ( !(*it2).m_strIcon.isEmpty() )
        {
          QPixmap pix = SmallIcon( (*it2).m_strIcon );
          act->setIconSet( pix );
        }

        addAction( act, m_menuElement );

        m_mapPopupServices[ id++ ] = *it2;
        insertedOffer = true;
      }

      if ( insertedOffer )
        addSeparator();
  }

  if ( !m_sMimeType.isEmpty() && showPropertiesAndFileType )
  {
      act = new KAction( i18n( "Edit File Type..." ), 0, this, SLOT( slotPopupMimeType() ),
                       &m_ownActions, "editfiletype" );
      addAction( act );
  }

  if ( KPropertiesDialog::canDisplay( m_lstItems ) && showPropertiesAndFileType )
  {
      act = new KAction( i18n( "Properties..." ), 0, this, SLOT( slotPopupProperties() ),
                         &m_ownActions, "properties" );
      addAction( act );
  }

  while ( !m_menuElement.lastChild().isNull() &&
            m_menuElement.lastChild().toElement().tagName().lower() == "separator" )
    m_menuElement.removeChild( m_menuElement.lastChild() );

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
  KonqOperations::emptyTrash();
}

void KonqPopupMenu::slotPopupOpenWith()
{
  KOpenWithHandler::getOpenWithHandler()->displayOpenWithDialog( m_lstPopupURLs );
}

void KonqPopupMenu::slotPopupAddToBookmark()
{
  KBookmarkGroup root = KBookmarkManager::self()->root();
  KURL::List::ConstIterator it = m_lstPopupURLs.begin();
  for ( ; it != m_lstPopupURLs.end(); it++ )
      root.addBookmark( (*it).prettyURL(), (*it).url() );
  KBookmarkManager::self()->emitChanged( root );
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

  // Is it a service specific to desktop entry files ?
  QMap<int,KDEDesktopMimeType::Service>::Iterator it2 = m_mapPopupServices.find( id );
  if ( it2 != m_mapPopupServices.end() )
  {
      KDEDesktopMimeType::executeService( m_lstPopupURLs, it2.data() );
  }

  return;
}

void KonqPopupMenu::slotPopupMimeType()
{
  KonqOperations::editMimeType( m_sMimeType );
}

void KonqPopupMenu::slotPopupProperties()
{
    // It may be that the kfileitem was created by hand
    // (see KonqKfmIconView::slotMouseButtonPressed)
    // In that case, we can get more precise info in the properties
    // (like permissions) if we stat the URL.
    if ( m_lstItems.count() == 1 )
    {
        KFileItem * item = m_lstItems.first();
        if (item->entry().count() == 0) // this item wasn't listed by a slave
        {
            // KPropertiesDialog will use stat to get more info on the file
            (void) new KPropertiesDialog( item->url() );
            return;
        }
    }
    (void) new KPropertiesDialog( m_lstItems );
}

KAction *KonqPopupMenu::action( const QDomElement &element ) const
{
  QCString name = element.attribute( attrName ).ascii();

  KAction *res = m_ownActions.action( name );

  if ( !res )
    res = m_actions.action( name );

  if ( !res && strcmp( name, m_pMenuNew->name() ) == 0 )
    return m_pMenuNew;

  return res;
}

QDomDocument KonqPopupMenu::domDocument() const
{
  return m_doc;
}

KActionCollection *KonqPopupMenu::actionCollection() const
{
  return const_cast<KActionCollection *>( &m_ownActions );
}

void KonqPopupMenu::addAction( KAction *act, const QDomElement &menu )
{
  addAction( act->name(), menu );
}

void KonqPopupMenu::addAction( const char *name, const QDomElement &menu )
{
  static QString tagAction = QString::fromLatin1( "action" );

  QDomElement parent = menu;
  if ( parent.isNull() )
    parent = m_menuElement;

  QDomElement e = m_doc.createElement( tagAction );
  parent.appendChild( e );
  e.setAttribute( attrName, name );
}

void KonqPopupMenu::addSeparator( const QDomElement &menu )
{
  static QString tagSeparator = QString::fromLatin1( "separator" );

  QDomElement parent = menu;
  if ( parent.isNull() )
    parent = m_menuElement;

  parent.appendChild( m_doc.createElement( tagSeparator ) );
}

void KonqPopupMenu::addMerge( const char *name )
{
  static QString tagMerge = QString::fromLatin1( "merge" );
  QDomElement merge = m_doc.createElement( tagMerge );
  m_menuElement.appendChild( merge );
  if ( name )
    merge.setAttribute( attrName, name );
}

void KonqPopupMenu::addGroup( const QString &grp )
{
  QDomElement group = m_doc.createElement( "definegroup" );
  m_menuElement.appendChild( group );
  group.setAttribute( "name", grp );
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


#include "konq_popupmenu.moc"
