/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
   Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>

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

#include <klocale.h>
#include <kapplication.h>
#include <kbookmarkmanager.h>
#include <kdebug.h>
#include <krun.h>
#include <kprotocolinfo.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <kxmlguifactory.h>
#include <kxmlguibuilder.h>
#include <kparts/componentfactory.h>

#include <assert.h>

#include <kfileshare.h>
#include <kprocess.h>

#include "kpropertiesdialog.h"
#include "knewmenu.h"
#include "konq_popupmenu.h"
#include "konq_operations.h"
#include <dcopclient.h>


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

class KonqPopupMenu::KonqPopupMenuPrivate
{
public:
  KonqPopupMenuPrivate() : m_parentWidget(0)
  {
    m_itemFlags=KParts::BrowserExtension::DefaultPopupItems;
  }
  QString m_urlTitle;
  QWidget *m_parentWidget;
  KParts::BrowserExtension::PopupFlags m_itemFlags;
};

KonqPopupMenu::ProtocolInfo::ProtocolInfo( )
{
  m_Reading = false;
  m_Writing = false;
  m_Deleting = false;
  m_Moving = false;
  m_TrashIncluded = false;
}
bool KonqPopupMenu::ProtocolInfo::supportsReading() const
{
  return m_Reading;
}
bool KonqPopupMenu::ProtocolInfo::supportsWriting() const
{
  return m_Writing;
}
bool KonqPopupMenu::ProtocolInfo::supportsDeleting() const
{
  return m_Deleting;
}
bool KonqPopupMenu::ProtocolInfo::supportsMoving() const
{
  return m_Moving;
}
bool KonqPopupMenu::ProtocolInfo::trashIncluded() const
{
  return m_TrashIncluded;
}

KonqPopupMenu::KonqPopupMenu( KBookmarkManager *mgr, const KFileItemList &items,
                              KURL viewURL,
                              KActionCollection & actions,
                              KNewMenu * newMenu,
                              bool showProperties )
  : QPopupMenu( 0L, "konq_popupmenu" ), m_actions( actions ), m_ownActions( static_cast<QObject *>( 0 ), "KonqPopupMenu::m_ownActions" ),
    m_pMenuNew( newMenu ), m_sViewURL(viewURL), m_lstItems(items), m_pManager(mgr)

{
  KonqPopupFlags kpf = ( showProperties ? ShowProperties : IsLink ) | ShowNewWindow;
  init(0, kpf, KParts::BrowserExtension::DefaultPopupItems);
}

KonqPopupMenu::KonqPopupMenu( KBookmarkManager *mgr, const KFileItemList &items,
                              KURL viewURL,
                              KActionCollection & actions,
                              KNewMenu * newMenu,
			      QWidget * parentWidget,
                              bool showProperties )
  : QPopupMenu( parentWidget, "konq_popupmenu" ), m_actions( actions ), m_ownActions( static_cast<QObject *>( 0 ), "KonqPopupMenu::m_ownActions" ), m_pMenuNew( newMenu ), m_sViewURL(viewURL), m_lstItems(items), m_pManager(mgr)
{
  KonqPopupFlags kpf = ( showProperties ? ShowProperties : IsLink ) | ShowNewWindow;
  init(parentWidget, kpf, KParts::BrowserExtension::DefaultPopupItems);
}

KonqPopupMenu::KonqPopupMenu( KBookmarkManager *mgr, const KFileItemList &items,
                              const KURL& viewURL,
                              KActionCollection & actions,
                              KNewMenu * newMenu,
                              QWidget * parentWidget,
                              KonqPopupFlags kpf,
                              KParts::BrowserExtension::PopupFlags flags)
  : QPopupMenu( parentWidget, "konq_popupmenu" ), m_actions( actions ), m_ownActions( static_cast<QObject *>( 0 ), "KonqPopupMenu::m_ownActions" ), m_pMenuNew( newMenu ), m_sViewURL(viewURL), m_lstItems(items), m_pManager(mgr)
{
  init(parentWidget, kpf, flags);
}

void KonqPopupMenu::init (QWidget * parentWidget, KonqPopupFlags kpf, KParts::BrowserExtension::PopupFlags flags)
{
  d = new KonqPopupMenuPrivate;
  d->m_parentWidget = parentWidget;
  d->m_itemFlags = flags;
  setup(kpf);
}


int KonqPopupMenu::insertServicesSubmenus(const QMap<QString, ServiceList>& submenus,
                                          QDomElement& menu,
                                          bool isBuiltin)
{
    int count = 0;
    QMap<QString, ServiceList>::ConstIterator it;

    for (it = submenus.begin(); it != submenus.end(); ++it)
    {
        if (it.data().isEmpty())
        {
            //avoid empty sub-menus
            continue;
        }

        QDomElement actionSubmenu = m_doc.createElement( "menu" );
        actionSubmenu.setAttribute( "name", "actions " + it.key() );
        menu.appendChild( actionSubmenu );
        QDomElement subtext = m_doc.createElement( "text" );
        actionSubmenu.appendChild( subtext );
        subtext.appendChild( m_doc.createTextNode( it.key() ) );
        count += insertServices(it.data(), actionSubmenu, isBuiltin);
    }

    return count;
}

int KonqPopupMenu::insertServices(const ServiceList& list,
                                  QDomElement& menu,
                                  bool isBuiltin)
{
    static int id = 1000;
    int count = 0;

    ServiceList::const_iterator it = list.begin();
    for( ; it != list.end(); ++it )
    {
        if ((*it).isEmpty())
        {
            if (!menu.firstChild().isNull() &&
                menu.lastChild().toElement().tagName().lower() != "separator")
            {
                QDomElement separator = m_doc.createElement( "separator" );
                menu.appendChild(separator);
            }
            continue;
        }

        if (isBuiltin || (*it).m_display == true)
        {
            QCString name;
            name.setNum( id );
            name.prepend( isBuiltin ? "builtinservice_" : "userservice_" );
            KAction * act = new KAction( (*it).m_strName, 0,
                                         this, SLOT( slotRunService() ),
                                         &m_ownActions, name );

            if ( !(*it).m_strIcon.isEmpty() )
            {
                QPixmap pix = SmallIcon( (*it).m_strIcon );
                act->setIconSet( pix );
            }

            addAction( act, menu ); // Add to toplevel menu

            m_mapPopupServices[ id++ ] = *it;
            ++count;
        }
    }

    return count;
}

bool KonqPopupMenu::KIOSKAuthorizedAction(KConfig& cfg)
{
    if ( !cfg.hasKey( "X-KDE-AuthorizeAction") )
    {
        return true;
    }

    QStringList list = cfg.readListEntry("X-KDE-AuthorizeAction");
    if (kapp && !list.isEmpty())
    {
        for(QStringList::ConstIterator it = list.begin();
            it != list.end();
            ++it)
        {
            if (!kapp->authorize((*it).stripWhiteSpace()))
            {
                return false;
            }
        }
    }

    return true;
}


void KonqPopupMenu::setup(KonqPopupFlags kpf)
{
    assert( m_lstItems.count() >= 1 );

    m_ownActions.setWidget( this );

    bool bIsLink        = (kpf & IsLink);
    bool currentDir     = false;
    bool sReading       = true;
    bool sWriting       = true;
    bool sDeleting      = true;
    bool sMoving        = true;
    m_sMimeType         = m_lstItems.first()->mimetype();
    mode_t mode         = m_lstItems.first()->mode();
    bool isDirectory    = m_sMimeType == "inode/directory";
    bool bTrashIncluded = false;
    bool bCanChangeSharing = false;
    m_lstPopupURLs.clear();
    int id = 0;
    if( isDirectory && m_lstItems.first()->isLocalFile())
        bCanChangeSharing=true;
    setFont(KGlobalSettings::menuFont());
    m_pluginList.setAutoDelete( true );
    m_ownActions.setHighlightingEnabled( true );

    attrName = QString::fromLatin1( "name" );

    prepareXMLGUIStuff();
    m_builder = new KonqPopupMenuGUIBuilder( this );
    m_factory = new KXMLGUIFactory( m_builder );

    KURL url;
    KFileItemListIterator it ( m_lstItems );
    // Check whether all URLs are correct
    bool devicesFile = false;
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
        if ( url.protocol().find("device", 0, false)==0)
            devicesFile = true;
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

    m_info.m_Reading = sReading;
    m_info.m_Writing = sWriting;
    m_info.m_Deleting = sDeleting;
    m_info.m_Moving = sMoving;
    m_info.m_TrashIncluded = bTrashIncluded;

    //check if url is current directory
    if ( m_lstItems.count() == 1 )
    {
        KURL firstPopupURL ( m_lstItems.first()->url() );
        firstPopupURL.cleanPath();
        //kdDebug(1203) << "View path is " << url.url() << endl;
        //kdDebug(1203) << "First popup path is " << firstPopupURL.url() << endl;
        currentDir = firstPopupURL.equals( url, true /* ignore_trailing */ );
    }

    bool isCurrentTrash = ( url.isLocalFile() &&
                            url.path(1) == KGlobalSettings::trashPath() &&
                            currentDir) ||
                          ( m_lstItems.count() == 1 && bTrashIncluded );
    bool isIntoTrash =  url.isLocalFile() && url.path(1).startsWith(KGlobalSettings::trashPath());
    clear();

    //////////////////////////////////////////////////////////////////////////

    KAction * act;

    if (!isCurrentTrash)
        addMerge( "konqueror" );

    bool isKDesktop = QCString(  kapp->name() ) == "kdesktop";
    KAction *actNewWindow = 0;

    if (( kpf & ShowProperties ) && isKDesktop &&
        !kapp->authorize("editable_desktop_icons"))
    {
        kpf &= ~ShowProperties; // remove flag
    }

    // Either 'newview' is in the actions we're given (probably in the tabhandling group)
    // or we need to insert it ourselves (e.g. for kdesktop). In the first case, actNewWindow must remain 0.
    if ( kpf & ShowNewWindow )
    {
        QString openStr = isKDesktop ? i18n( "&Open" ) : i18n( "Open in New &Window" );
        actNewWindow = new KAction( openStr, "window_new", 0, this, SLOT( slotPopupNewView() ), &m_ownActions, "newview" );
    }

    if ( actNewWindow && !isKDesktop )
    {
        if (isCurrentTrash)
            actNewWindow->setStatusText( i18n( "Open the trash in a new window" ) );
        else
            actNewWindow->setStatusText( i18n( "Open the document in a new window" ) );
    }

    if ( isCurrentTrash )
    {
        if (actNewWindow)
        {
            addAction( actNewWindow );
            addSeparator();
        }
        addGroup( "tabhandling" ); // includes a separator

        act = new KAction( i18n( "&Empty Trash Bin" ), 0, this, SLOT( slotPopupEmptyTrashBin() ), &m_ownActions, "empytrash" );
        addAction( act );
        if ( KPropertiesDialog::canDisplay( m_lstItems ) && (kpf & ShowProperties) )
        {
            act = new KAction( i18n( "&Properties" ), 0, this, SLOT( slotPopupProperties() ),
                               &m_ownActions, "properties" );
            addAction( act );
        }
        m_factory->addClient( this );
        return;
    }
    else
    {
        if ( S_ISDIR(mode) && sWriting && !isIntoTrash ) // A dir, and we can create things into it
        {
            if ( currentDir && m_pMenuNew ) // Current dir -> add the "new" menu
            {
                // As requested by KNewMenu :
                m_pMenuNew->slotCheckUpToDate();
                m_pMenuNew->setPopupFiles( m_lstPopupURLs );

                addAction( m_pMenuNew );

                addSeparator();
            }
            else
            {
                if (d->m_itemFlags & KParts::BrowserExtension::ShowCreateDirectory)
                {
                    KAction *actNewDir = new KAction( i18n( "Create &Folder..." ), "folder_new", 0, this, SLOT( slotPopupNewDir() ), &m_ownActions, "newdir" );
                    addAction( actNewDir );
                    addSeparator();
                }
            }
        }

        if (d->m_itemFlags & KParts::BrowserExtension::ShowNavigationItems)
        {
            if (d->m_itemFlags & KParts::BrowserExtension::ShowUp)
                addAction( "up" );
            addAction( "back" );
            addAction( "forward" );
            if (d->m_itemFlags & KParts::BrowserExtension::ShowReload)
                addAction( "reload" );
            addSeparator();
        }

        // "open in new window" is either provided by us, or by the tabhandling group
        if (actNewWindow)
        {
            addAction( actNewWindow );
            addSeparator();
        }
        addGroup( "tabhandling" ); // includes a separator

        if ( !bIsLink )
        {
            if ( !currentDir && sReading && !isIntoTrash &&!devicesFile ) {
                if ( sDeleting ) {
                    addAction( "cut" );
                }
                addAction( "copy" );
            }

            if ( S_ISDIR(mode) && sWriting && !isIntoTrash) {
                if ( currentDir )
                    addAction( "paste" );
                else
                    addAction( "pasteto" );
            }
            if ( !isIntoTrash )
            {
                if (!currentDir )
                {
                    if ( m_lstItems.count() == 1 && sWriting )
                        addAction("rename");

                    if ( sMoving )
                        addAction( "trash" );

                    if ( sDeleting ) {
                        addAction( "del" );
                    }
                }
            }
        }
        addGroup( "editactions" );
    }
    if ( !isCurrentTrash && !isIntoTrash && (d->m_itemFlags & KParts::BrowserExtension::ShowBookmark))
    {
        addSeparator();
        QString caption;
        if (currentDir)
        {
           bool httpPage = (m_sViewURL.protocol().find("http", 0, false) == 0);
           if (httpPage)
              caption = i18n("&Bookmark This Page");
           else
              caption = i18n("&Bookmark This Location");
        }
        else if (S_ISDIR(mode))
           caption = i18n("&Bookmark This Folder");
        else if (bIsLink)
           caption = i18n("&Bookmark This Link");
        else
           caption = i18n("&Bookmark This File");

        act = new KAction( caption, "bookmark_add", 0, this, SLOT( slotPopupAddToBookmark() ), &m_ownActions, "bookmark_add" );
        if (m_lstItems.count() > 1)
            act->setEnabled(false);
        if (kapp->authorizeKAction("bookmarks"))
            addAction( act );
        if (bIsLink)
            addGroup( "linkactions" );
    }

    //////////////////////////////////////////////////////

    ServiceList builtin;
    ServiceList user, userToplevel, userPriority;
    QMap<QString, ServiceList> userSubmenus, userToplevelSubmenus, userPrioritySubmenus;

    bool isSingleLocal = (m_lstItems.count() == 1 && m_lstItems.first()->url().isLocalFile());
    // 1 - Look for builtin and user-defined services
    if ( m_sMimeType == "application/x-desktop" && isSingleLocal ) // .desktop file
    {
        // get builtin services, like mount/unmount
        builtin = KDEDesktopMimeType::builtinServices( m_lstItems.first()->url() );
        user = KDEDesktopMimeType::userDefinedServices( m_lstItems.first()->url().path(), url.isLocalFile() );
    }

    if ( !isCurrentTrash && !isIntoTrash)
    {

        // 2 - Look for "servicesmenus" bindings (konqueror-specific user-defined services)

        // first check the .directory if this is a directory
        if (isDirectory && isSingleLocal)
        {
            QString dotDirectoryFile = m_lstItems.first()->url().path(1).append(".directory");
            KSimpleConfig cfg( dotDirectoryFile, true );
            cfg.setDesktopGroup();

            if (KIOSKAuthorizedAction(cfg))
            {
                QString submenuName = cfg.readEntry( "X-KDE-Submenu" );
                if (submenuName.isEmpty())
                {
                    user += KDEDesktopMimeType::userDefinedServices( dotDirectoryFile, true );
                }
                else
                {
                    userSubmenus[submenuName] += KDEDesktopMimeType::userDefinedServices( dotDirectoryFile, true );
                }
            }
        }

        QStringList dirs = KGlobal::dirs()->findDirs( "data", "konqueror/servicemenus/" );
        QStringList::ConstIterator dIt = dirs.begin();
        QStringList::ConstIterator dEnd = dirs.end();

        for (; dIt != dEnd; ++dIt )
        {
            QDir dir( *dIt );

            QStringList entries = dir.entryList( "*.desktop", QDir::Files );
            QStringList::ConstIterator eIt = entries.begin();
            QStringList::ConstIterator eEnd = entries.end();

            for (; eIt != eEnd; ++eIt )
            {
                KSimpleConfig cfg( *dIt + *eIt, true );
                cfg.setDesktopGroup();

                if (!KIOSKAuthorizedAction(cfg))
                {
                    continue;
                }

                if ( cfg.hasKey( "X-KDE-ShowIfRunning" ) )
                {
                    QString app = cfg.readEntry( "X-KDE-ShowIfRunning" );
                    if ( !kapp->dcopClient()->isApplicationRegistered( app.utf8() ) )
                        continue;
                }

                if ( cfg.hasKey( "Actions" ) && cfg.hasKey( "ServiceTypes" ) )
                {
                    QStringList types = cfg.readListEntry( "ServiceTypes" );
                    QStringList excludeTypes = cfg.readListEntry( "ExcludeServiceTypes" );
                    bool ok = false;
                    QString mimeGroup = m_sMimeType.left(m_sMimeType.find('/'));

                    // check for exact matches or a typeglob'd mimetype if we have a mimetype
                    for (QStringList::iterator it = types.begin(); it != types.end(); ++it)
                    {
                        // we could cram the following three if statements into
                        // one gigantic boolean statement but that would be a
                        // hororr show for readability

                        // first check if we have an all mimetype
                        if (*it == "all/all" ||
                            *it == "allfiles" /*compat with KDE up to 3.0.3*/)
                        {
			  ok = true;
			  for (QStringList::iterator itex = excludeTypes.begin(); itex != excludeTypes.end(); ++itex)
			    {
			      if( (*itex).right(1) == "*" &&(*itex).left((*itex).find('/'))== mimeGroup )
				{
				  ok = false;
				  break;
				}
			      else if ( (*itex).contains( m_sMimeType))
				{
				  ok = false;
				  break;
				}
			    }
                        }

                        // next, do we match all files?
                        if (*it == "all/allfiles" &&
                            !isDirectory) // ## or inherits from it
                        {
			  ok = true;
			  for (QStringList::iterator itex = excludeTypes.begin(); itex != excludeTypes.end(); ++itex)
			    {
			      if( (*itex).right(1) == "*" &&(*itex).left((*itex).find('/'))== mimeGroup )
				{
				  ok = false;
				  break;
				}
			      else if ( (*itex).contains( m_sMimeType))
				{
				  ok = false;
				  break;
				}
			    }

                        }

                        // if we have a mimetype, see if we have an exact or type
                        // globbed match
                        if (!m_sMimeType.isNull() &&
                            (*it == m_sMimeType ||
                             ((*it).right(1) == "*" &&
                              (*it).left((*it).find('/')) == mimeGroup)))
                        {
			  ok = true;
			  for (QStringList::iterator itex = excludeTypes.begin(); itex != excludeTypes.end(); ++itex)
			    {
			      if( (*itex).right(1) == "*" &&(*itex).left((*itex).find('/'))== mimeGroup )
				{
				  ok = false;
				  break;
				}
			      else if ( (*itex).contains( m_sMimeType))
				{
				  ok = false;
				  break;
				}
			    }
                        }
                    }

                    if ( ok )
                    {
                        // we use the categories .desktop entry to define submenus
                        // if none is defined, we just pop it in the main menu
                        QString priority = cfg.readEntry("X-KDE-Priority");
                        QString submenuName = cfg.readEntry( "X-KDE-Submenu" );
                        ServiceList* list = &user;

                        if (submenuName.isEmpty())
                        {
                            if (priority == "TopLevel")
                            {
                                list = &userToplevel;
                            }
                            else if (priority == "Important")
                            {
                                list = &userPriority;
                            }
                        }
                        else if (priority == "TopLevel")
                        {
                            list = &(userToplevelSubmenus[submenuName]);
                        }
                        else if (priority == "Important")
                        {
                            list = &(userPrioritySubmenus[submenuName]);
                        }
                        else
                        {
                            list = &(userSubmenus[submenuName]);
                        }

                        (*list) += KDEDesktopMimeType::userDefinedServices( *dIt + *eIt, url.isLocalFile() );
                    }
                }
            }
        }

        KTrader::OfferList offers;

        if (kapp->authorizeKAction("openwith"))
        {
            // if check m_sMimeType.isNull (no commom mime type) set it to all/all
            // 3 - Query for applications
            offers = KTrader::self()->query( m_sMimeType.isNull( ) ? QString::fromLatin1( "all/all" ) : m_sMimeType ,
                                             "Type == 'Application' and DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'" );
        }

        //// Ok, we have everything, now insert

        m_mapPopup.clear();
        m_mapPopupServices.clear();
        if ( !devicesFile)
        {
            if ( !offers.isEmpty() )
            {
                // First block, app and preview offers
                addSeparator();

                id = 1;

                QDomElement menu = m_menuElement;

                if ( offers.count() > 1 ) // submenu 'open with'
                {
                    menu = m_doc.createElement( "menu" );
                    menu.setAttribute( "name", "openwith submenu" );
                    m_menuElement.appendChild( menu );
                    QDomElement text = m_doc.createElement( "text" );
                    menu.appendChild( text );
                    text.appendChild( m_doc.createTextNode( i18n("&Open With") ) );
                }

                if ( menu == m_menuElement ) // no submenu -> open with... above the single offer
                {
                    KAction *openWithAct = new KAction( i18n( "&Open With..." ), 0, this, SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
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
                    KAction *openWithAct = new KAction( i18n( "&Other..." ), 0, this, SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
                    addAction( openWithAct, menu ); // Other...
                }
            }
            else // no app offers -> Open With...
            {
                addSeparator();
                act = new KAction( i18n( "&Open With..." ), 0, this, SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
                addAction( act );
            }

            addGroup( "preview" );
        }
    }

    // Second block, builtin + user
    QDomElement actionMenu = m_menuElement;
    int userItemCount = 0;
    if (user.count() + userSubmenus.count() +
        userPriority.count() + userPrioritySubmenus.count())
    {
        // we have more than one item, so let's make a submenu
        actionMenu = m_doc.createElement( "menu" );
        actionMenu.setAttribute( "name", "actions submenu" );
        m_menuElement.appendChild( actionMenu );
        QDomElement text = m_doc.createElement( "text" );
        actionMenu.appendChild( text );
        text.appendChild( m_doc.createTextNode( i18n("Ac&tions") ) );
    }

    userItemCount += insertServicesSubmenus(userPrioritySubmenus, actionMenu, false);
    userItemCount += insertServices(userPriority, actionMenu, false);

    // see if we need to put a separator between our priority items and our regular items
    if (userItemCount > 0 &&
        (user.count() > 0 ||
         userSubmenus.count() > 0 ||
         builtin.count() > 0) &&
         actionMenu.lastChild().toElement().tagName().lower() != "separator")
    {
        QDomElement separator = m_doc.createElement( "separator" );
        actionMenu.appendChild(separator);
    }

    userItemCount += insertServicesSubmenus(userSubmenus, actionMenu, false);
    userItemCount += insertServices(user, actionMenu, false);
    userItemCount += insertServices(builtin, m_menuElement, true);

    userItemCount += insertServicesSubmenus(userToplevelSubmenus, m_menuElement, false);
    userItemCount += insertServices(userToplevel, m_menuElement, false);

    if (userItemCount > 0)
    {
        addSeparator();
    }

    if ( !isCurrentTrash && !isIntoTrash && !devicesFile)
        addPlugins( ); // now it's time to add plugins

    if ( KPropertiesDialog::canDisplay( m_lstItems ) && (kpf & ShowProperties) )
    {
        act = new KAction( i18n( "&Properties" ), 0, this, SLOT( slotPopupProperties() ),
                           &m_ownActions, "properties" );
        addAction( act );
    }

    while ( !m_menuElement.lastChild().isNull() &&
            m_menuElement.lastChild().toElement().tagName().lower() == "separator" )
        m_menuElement.removeChild( m_menuElement.lastChild() );

    if( bCanChangeSharing && !isCurrentTrash && !isIntoTrash)
    {
        if(KFileShare::authorization()==KFileShare::Authorized)
        {
            addSeparator();
            QString label;
            label=i18n("Share");

            act = new KAction( label, 0, this, SLOT( slotOpenShareFileDialog() ),
                               &m_ownActions, "sharefile" );
            addAction( act );
        }
    }


    addMerge( 0 );

    m_factory->addClient( this );
}

void KonqPopupMenu::slotOpenShareFileDialog()
{
    //kdDebug()<<"KonqPopupMenu::slotOpenShareFileDialog()\n";
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
            KPropertiesDialog*dlg= new KPropertiesDialog( item->url(), d->m_parentWidget );
            dlg->showFileSharingPage();

            return;
        }
    }
    KPropertiesDialog*dlg=new KPropertiesDialog( m_lstItems, d->m_parentWidget );
    dlg->showFileSharingPage();
}

KonqPopupMenu::~KonqPopupMenu()
{
  m_pluginList.clear();
  delete m_factory;
  delete m_builder;
  delete d;
  kdDebug(1203) << "~KonqPopupMenu leave" << endl;
}

void KonqPopupMenu::setURLTitle( const QString& urlTitle )
{
    d->m_urlTitle = urlTitle;
}

void KonqPopupMenu::slotPopupNewView()
{
  KURL::List::ConstIterator it = m_lstPopupURLs.begin();
  for ( ; it != m_lstPopupURLs.end(); it++ )
    (void) new KRun(*it);
}

void KonqPopupMenu::slotPopupNewDir()
{
  if (m_lstPopupURLs.empty())
    return;

  KonqOperations::newDir(d->m_parentWidget, m_lstPopupURLs.first());
}

void KonqPopupMenu::slotPopupEmptyTrashBin()
{
  KonqOperations::emptyTrash();
}

void KonqPopupMenu::slotPopupOpenWith()
{
  KRun::displayOpenWithDialog( m_lstPopupURLs );
}

void KonqPopupMenu::slotPopupAddToBookmark()
{
  KBookmarkGroup root = m_pManager->root();
  if ( m_lstPopupURLs.count() == 1 ) {
    KURL url = m_lstPopupURLs.first();
    QString title = d->m_urlTitle.isEmpty() ? url.prettyURL() : d->m_urlTitle;
    root.addBookmark( m_pManager, title, url.url() );
  }
  else
  {
    KURL::List::ConstIterator it = m_lstPopupURLs.begin();
    for ( ; it != m_lstPopupURLs.end(); it++ )
      root.addBookmark( m_pManager, (*it).prettyURL(), (*it).url() );
  }
  m_pManager->emitChanged( root );
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
            (void) new KPropertiesDialog( item->url(), d->m_parentWidget );
            return;
        }
    }
    (void) new KPropertiesDialog( m_lstItems, d->m_parentWidget );
}

KAction *KonqPopupMenu::action( const QDomElement &element ) const
{
  QCString name = element.attribute( attrName ).ascii();
  KAction *res = m_ownActions.action( name );

  if ( !res )
    res = m_actions.action( name );

  if ( !res && m_pMenuNew && strcmp( name, m_pMenuNew->name() ) == 0 )
    return m_pMenuNew;

  return res;
}
KActionCollection *KonqPopupMenu::actionCollection() const
{
  return const_cast<KActionCollection *>( &m_ownActions );
}

QString KonqPopupMenu::mimeType( ) const {
    return m_sMimeType;
}
KonqPopupMenu::ProtocolInfo KonqPopupMenu::protocolInfo() const
{
  return m_info;
}
void KonqPopupMenu::addPlugins( ){
	// search for Konq_PopupMenuPlugins inspired by simons kpropsdlg
	//search for a plugin with the right protocol
	KTrader::OfferList plugin_offers;
        unsigned int pluginCount = 0;
	plugin_offers = KTrader::self()->query( m_sMimeType.isNull() ? QString::fromLatin1( "all/all" ) : m_sMimeType , "'KonqPopupMenu/Plugin' in ServiceTypes");
        if ( plugin_offers.isEmpty() )
	  return; // no plugins installed do not bother about it

	KTrader::OfferList::ConstIterator iterator = plugin_offers.begin( );
	KTrader::OfferList::ConstIterator end = plugin_offers.end( );

	addGroup( "plugins" );
	// travers the offerlist
	for(; iterator != end; ++iterator, ++pluginCount ){
		KonqPopupMenuPlugin *plugin =
			KParts::ComponentFactory::
			createInstanceFromLibrary<KonqPopupMenuPlugin>( (*iterator)->library().local8Bit(),
									this,
									(*iterator)->name().latin1() );
		if ( !plugin )
			continue;
                QString pluginClientName = QString::fromLatin1( "Plugin%1" ).arg( pluginCount );
                addMerge( pluginClientName );
                plugin->domDocument().documentElement().setAttribute( "name", pluginClientName );
		m_pluginList.append( plugin );
		insertChildClient( plugin );
	}

	addMerge( "plugins" );
	addSeparator();
}
KURL KonqPopupMenu::url( ) const {
  return m_sViewURL;
}
KFileItemList KonqPopupMenu::fileItemList( ) const {
  return m_lstItems;
}
KURL::List KonqPopupMenu::popupURLList( ) const {
  return m_lstPopupURLs;
}
/**
	Plugin
*/

KonqPopupMenuPlugin::KonqPopupMenuPlugin( KonqPopupMenu *parent, const char *name )
    : QObject( parent, name ) {
}
KonqPopupMenuPlugin::~KonqPopupMenuPlugin( ){

}
#include "konq_popupmenu.moc"
