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

#ifndef __kbookmarkmenu_h__
#define __kbookmarkmenu_h__

#include <qlist.h>
#include <qobject.h>

class QString;
class KBookmark;
class QActionCollection;
class QPopupMenu;

/**
 * A widget using the bookmarks must derive from this class. It implements
 * a kind of callback interface for the @ref KBookmarkMenu class.
 */
class KBookmarkOwner
{
public:
  /**
   * This function is called if the user selectes a bookmark. You must overload it.
   */
  virtual void openBookmarkURL( const QString & url ) = 0L;
  /**
   * @return the title of the current page. This is called if the user wants
   *         to add the current page to the bookmarks.
   */
  virtual QString currentTitle() = 0L;
  /**
   * @return the URL of the current page. This is called if the user wants
   *         to add the current page to the bookmarks.
   */
  virtual QString currentURL() = 0L;
};

/**
 * This class provides a bookmark menu. If you want to use
 * it you must in addition implement @ref KBookmarkOwner. 
 * And you must create an instance of @ref KBookmarkManager on startup.
 */
class KBookmarkMenu : public QObject
{
  Q_OBJECT
public:
  /**
   * Fills a bookmark menu (one instance of KBookmarkMenu is created
   * for the toplevel menu, but also one per submenu).
   * @param _owner implementation of the KBookmarkOwner interface (callbacks)
   * @param _parentMenu menu to be filled
   * @param _collec parent for the QActions
   * @param _root true for the toplevel menu
   */
  KBookmarkMenu( KBookmarkOwner * _owner, QPopupMenu * _parentMenu, QActionCollection * _collec,  bool _root );
  ~KBookmarkMenu();  

public slots:
  void slotBookmarksChanged();
  void slotBookmarkSelected();
  
protected:
  void fillBookmarkMenu( KBookmark *parent );

  bool m_bIsRoot;
  KBookmarkOwner *m_pOwner;
  QPopupMenu * m_parentMenu;
  QList<KBookmarkMenu> m_lstSubMenus;
  QActionCollection * m_actionCollection;
};

#endif

