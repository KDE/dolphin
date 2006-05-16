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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konqpopupmenu_h
#define __konqpopupmenu_h

#include <sys/types.h>
#include <kactioncollection.h>
#include <QMenu>
#include <QMap>
#include <kaction.h>
#include <kactioncollection.h>

#include <QStringList>

#include <kfileitem.h>
#include <kmimetype.h> // for KDEDesktopMimeType
#include <libkonq_export.h>

#include <kparts/browserextension.h>
#include <kdedesktopmimetype.h>
#include "konq_xmlguiclient.h"

typedef QList<KDEDesktopMimeType::Service> ServiceList;

class KPropertiesDialog;
class KNewMenu;
class KService;
class KonqPopupMenuPlugin;
class KBookmarkManager;

/**
 * This class implements the popup menu for URLs in konqueror and kdesktop
 * It's usage is very simple : on right click, create the KonqPopupMenu instance
 * with the correct arguments, then exec() to make it appear, then destroy it.
 *
 */
class LIBKONQ_EXPORT KonqPopupMenu : public QMenu, public KonqXMLGUIClient
{
  Q_OBJECT
public:

  /**
   * Flags set by the calling application (konqueror/kdesktop), unlike
   * KParts::BrowserExtension::PopupFlags, which are set by the calling part
   */
  typedef uint KonqPopupFlags;
  enum { NoFlags = 0,
         ShowProperties = 1,  ///< whether to show the "Properties" menu item
         IsLink = 2,          ///< HTML link. If set, we won't have cut/copy/paste, and we'll say "bookmark this link"
         ShowNewWindow = 4 };
         // WARNING: bitfield. Next item is 8

#if 0
  /**
   * @deprecated lacks parentWidget pointer, and
   * uses bool instead of KonqPopupFlags enum,
   * might do strange things with the 'new window' action.
   */
  KonqPopupMenu( KBookmarkManager* manager,
                 const KFileItemList &items,
                 KUrl viewURL,
                 KActionCollection & actions,
                 KNewMenu * newMenu,
                 bool showPropertiesAndFileType = true ) KDE_DEPRECATED;

  /**
   * @deprecated uses bool instead of KonqPopupFlags enum,
   * might do strange things with the 'new window' action.
   */
  KonqPopupMenu( KBookmarkManager* manager,
                 const KFileItemList &items,
                 KUrl viewURL,
                 KActionCollection & actions,
                 KNewMenu * newMenu,
		 QWidget * parentWidget,
		 bool showPropertiesAndFileType = true ) KDE_DEPRECATED;
#endif

  /**
   * Constructor
   * @param manager the bookmark manager for this bookmark
   * @param items the list of file items the popupmenu should be shown for
   * @param viewURL the URL shown in the view, to test for RMB click on view background
   * @param actions list of actions the caller wants to see in the menu
   * @param newMenu "New" menu, shared with the File menu, in konqueror
   * @param parentWidget the widget we're showing this popup for. Helps destroying
   * the popup if the widget is destroyed before the popup.
   * @param kpf flags from the KonqPopupFlags enum, set by the calling application
   * @param f flags from the BrowserExtension enum, set by the calling part
   *
   * The actions to pass in include :
   * showmenubar, back, forward, up, cut, copy, paste, pasteto, trash, rename, del
   * The others items are automatically inserted.
   *
   * @todo that list is probably not be up-to-date
   */
  KonqPopupMenu( KBookmarkManager* manager,
                 const KFileItemList &items,
                 const KUrl& viewURL,
                 KActionCollection & actions,
                 KNewMenu * newMenu,
                 QWidget * parentWidget,
                 KonqPopupFlags kpf,
                 KParts::BrowserExtension::PopupFlags f /*= KParts::BrowserExtension::DefaultPopupItems*/);

  /**
   * Don't forget to destroy the object
   */
  ~KonqPopupMenu();

  /**
   * Set the title of the URL, when the popupmenu is opened for a single URL.
   * This is used if the user chooses to add a bookmark for this URL.
   */
  void setURLTitle( const QString& urlTitle );

  class LIBKONQ_EXPORT ProtocolInfo {
   public:
    ProtocolInfo();
    bool supportsReading()  const;
    bool supportsWriting()  const;
    bool supportsDeleting() const;
    bool supportsMoving()   const;
    bool trashIncluded()    const;
   private:
    friend class KonqPopupMenu;
    bool m_Reading:1;
    bool m_Writing:1;
    bool m_Deleting:1;
    bool m_Moving:1;
    bool m_TrashIncluded:1;
  };
  /**
   * Reimplemented for internal purpose
   */
  virtual KAction *action( const QDomElement &element ) const;


  virtual KActionCollection *actionCollection() const;
  QString mimeType( ) const;
  KUrl url( ) const;
  KFileItemList fileItemList() const;
  KUrl::List popupURLList( ) const;
  ProtocolInfo protocolInfo() const;

public Q_SLOTS: // KDE4: why public?
  void slotPopupNewDir();
  void slotPopupNewView();
  void slotPopupEmptyTrashBin();
  void slotPopupRestoreTrashedItems();
  void slotPopupOpenWith();
  void slotPopupAddToBookmark();
  void slotRunService();
  void slotPopupMimeType();
  void slotPopupProperties();
  void slotOpenShareFileDialog();
protected:
  KActionCollection &m_actions;
  KActionCollection m_ownActions;

private:
  void init (QWidget * parentWidget, KonqPopupFlags kpf, KParts::BrowserExtension::PopupFlags itemFlags);
  void setup(KonqPopupFlags kpf);
  void addPlugins( );
  int  insertServicesSubmenus(const QMap<QString, ServiceList>& list, QDomElement& menu, bool isBuiltin);
  int  insertServices(const ServiceList& list, QDomElement& menu, bool isBuiltin);
  bool KIOSKAuthorizedAction(KConfig& cfg);
  KPropertiesDialog* showPropertiesDialog();

  class KonqPopupMenuPrivate;
  KonqPopupMenuPrivate *d;
  KNewMenu *m_pMenuNew;
  KUrl m_sViewURL;
  QString m_sMimeType;
  KFileItemList m_lstItems;
  KUrl::List m_lstPopupURLs;
  QMap<int,KService::Ptr> m_mapPopup;
  QMap<int,KDEDesktopMimeType::Service> m_mapPopupServices;
  bool m_bHandleEditOperations;
  KXMLGUIFactory *m_factory;
  KXMLGUIBuilder *m_builder;
  QString attrName;
  ProtocolInfo m_info;
  KBookmarkManager* m_pManager;
};

class LIBKONQ_EXPORT KonqPopupMenuPlugin : public QObject, public KonqXMLGUIClient {
	Q_OBJECT
public:
  /**
  * Constructor
  * If you want to insert a dynamic item or menu to konqpopupmenu
  * this class is the right choice.
  * Create a KAction and use _popup->addAction(new KAction );
  * If you want to create a submenu use _popup->addGroup( );
  */
  KonqPopupMenuPlugin( KonqPopupMenu *_popup); // this should also be the parent
  virtual ~KonqPopupMenuPlugin ( );
};

#endif

