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

#ifndef __konqpopupmenu_h
#define __konqpopupmenu_h

#include <sys/types.h>

#include <qpopupmenu.h>
#include <qmap.h>
#include <kaction.h>
#include <qstringlist.h>

#include <kfileitem.h>
#include <kmimetype.h> // for KDEDesktopMimeType

class KNewMenu;
class KService;

enum { KPOPUPMENU_BACK_ID, KPOPUPMENU_FORWARD_ID, KPOPUPMENU_UP_ID, 
       KPOPUPMENU_SHOWMENUBAR_ID, KPOPUPMENU_COPY_ID, KPOPUPMENU_PASTE_ID,
       KPOPUPMENU_TRASH_ID, KPOPUPMENU_DELETE_ID };

/** This class implements the popup menu for URLs in konqueror and kdesktop
 * It's usage is very simple : on right click, create the KonqPopupMenu instance
 * with the correct arguments, then exec() to make it appear, then destroy it.
 *
 */
class KonqPopupMenu : public QPopupMenu
{
  Q_OBJECT
public:

  /**
   * Constructor
   * @param items the list of file items the popupmenu should be shown for
   * @param viewURL the URL shown in the view, to test for RMB click on view background
   * @param actions list of actions the caller wants to see in the menu
   * @param newMenu "New" menu, shared with the File menu, in konqueror
   *
   * The actions to pass in include :
   * Show menubar, Back, Forward, Up, Cut, Copy, Paste, MoveToTrash, Delete
   * The others items are automatically inserted.
   *
   */
  KonqPopupMenu( const KFileItemList &items,
                 QString viewURL,
                 QActionCollection & actions,
                 KNewMenu * newMenu );
  /**
   * Don't forget to destroy the object
   */
  ~KonqPopupMenu();

public slots:
  void slotPopupNewView();
  void slotPopupEmptyTrashBin();
  void slotPopupCopy();
  void slotPopupPaste();
  void slotPopupTrash();
  void slotPopupDelete();
  void slotPopupOpenWith();
  void slotPopupAddToBookmark();
  void slotPopup( int id );
  void slotPopupMimeType();
  void slotPopupProperties();

protected:
  QActionCollection & m_actions;
  KNewMenu *m_pMenuNew;
  QString m_sViewURL;
  QString m_sMimeType;
  KFileItemList m_lstItems;
  QStringList m_lstPopupURLs;
  QMap<int,KService::Ptr> m_mapPopup;
  QMap<int,KDEDesktopMimeType::Service> m_mapPopup2;
  bool m_bHandleEditOperations;
};

#endif
