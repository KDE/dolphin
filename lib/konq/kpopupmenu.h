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

#ifndef __kpopupmenu_h
#define __kpopupmenu_h

#include <sys/types.h>

#include <qobject.h>
#include <qmap.h>
#include <qstringlist.h>

#include <kfileitem.h>
#include <kmimetypes.h> // for KDEDesktopMimeType

class KNewMenu;
class KService;
class OPMenu;

enum { KPOPUPMENU_BACK_ID, KPOPUPMENU_FORWARD_ID, KPOPUPMENU_UP_ID, 
       KPOPUPMENU_SHOWMENUBAR_ID, KPOPUPMENU_COPY_ID, KPOPUPMENU_PASTE_ID,
       KPOPUPMENU_TRASH_ID, KPOPUPMENU_DELETE_ID };

/** This class implements the popup menu for URLs in konqueror and kdesktop
 * It's usage is very simple : on right click, create the KonqPopupMenu instance
 * with the correct arguments, then exec() to make it appear, then destroy it.
 *
 * No action needs to be taken, except testing the return code of exec() in konqueror,
 * since it can tell that one of the actions defined in the enum above have 
 * been chosen.
 */
class KonqPopupMenu : public QObject
{
  Q_OBJECT
public:

  /**
   * Constructor
   * @param items the list of file items the popupmenu should be shown for
   * @param viewURL the URL shown in the view, to test for RMB click on view background
   * @param canGoBack set to true if the view can go back
   * @param canGoForward set to true if the view can go forward
   * @param canGoUp set to true if the view can go up
   * @param isMenubarHidden if true, additionnal entry : "show Menu"
   * @param handleEditOperations if true, let KPopupMenu handle operations like
   *                             copy/paste or delete
   * if handleEditOperations is false, then the following arguments specify
   * about whether the edit operations are enabled.
   * @param canCopy view is able to copy the selected item(s)
   * @param canPaste view is able to paste data from the clipboard
   * @param canMove view is able to delete the selected item(s) or move them
   *                to the trash
   */
  KonqPopupMenu( KFileItemList items,
                 QString viewURL,
                 bool canGoBack, 
                 bool canGoForward,
                 bool isMenubarHidden,
		 bool handleEditOperations = true,
		 bool canCopy = false,
		 bool canPaste = false,
		 bool canMove = false );
  /**
   * Don't forget to destroy the object
   */
  ~KonqPopupMenu();

  /**
   * Execute this popup synchronously.
   * @param p menu position
   * @return ID of the selected item in the popup menu
   * The up, back, forward, and showmenubar values should be tested, 
   * because they are NOT implemented here.
   */
  int exec( QPoint p );

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
  OPMenu *m_popupMenu;
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
