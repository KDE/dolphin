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
class KAction;
class KActionCollection;
class KBookmarkOwner;
class QPopupMenu;
namespace KIO { class Job; }

/**
 * This class provides a bookmark menu.  It is typically used in
 * cooperation with KActionMenu but doesn't have to be.
 *
 * If you use this class by itself, then it will use KDE defaults for
 * everything -- the bookmark path, bookmark editor, bookmark launcher..
 * everything.  These defaults reside in the classes
 * @ref KBookmarkOwner (editing bookmarks) and @ref KBookmarkManager
 * (almost everything else).  If you wish to change the defaults in
 * any way, you must reimplement and instantiate those classes
 * <em>before</em> this class is ever called.
 *
 * Using this class if very simple:
 *
 * 1) Create a popup menu (either KActionMenu or QPopupMenu will do)
 * 2) Instantiate a new KBookmarkMenu object using the above popup
 *    menu as a parameter
 * 3) Insert your (now full) popup menu wherever you wish
 *
 * Again, if you wish to modify any defaults, the procedure is:
 *
 * 1a) Reimplement your own KBookmarkOwner
 * 1b) Reimplement and instantiate your own KBookmarkManager
 */
class KBookmarkMenu : public QObject
{
  Q_OBJECT
public:
  /**
   * Fills a bookmark menu (one instance of KBookmarkMenu is created
   * for the toplevel menu, but also one per submenu).
   *
   * @param _owner implementation of the KBookmarkOwner interface (callbacks)
   * @param _parentMenu menu to be filled
   * @param _collec parent for the KActions
   * @param _root true for the toplevel menu
   * @param _add true to show the "Add Bookmark" entry
   */
  KBookmarkMenu( KBookmarkOwner * _owner, QPopupMenu * _parentMenu,
                 KActionCollection * _collec,  bool _root, bool _add = true);
  ~KBookmarkMenu();

  /**
   * Even if you think you need to use this, you are probably wrong.
   * It fills a bookmark menu starting a given KBookmark.
   * This is public for KBookmarkBar.
   * @param _parent a root bookmark that <em>must</em> be defined
   */
  void fillBookmarkMenu( KBookmark *_parent );

protected slots:
  void slotBookmarksChanged();
  void slotBookmarkSelected();
  void slotNSBookmarkSelected();
  void slotNSLoad();
  void slotResult( KIO::Job * );

protected:
  /**
   * function that loads Netscape's bookmarks
   */
  void openNSBookmarks();

  bool m_bIsRoot;
  bool m_bAddBookmark;
  KBookmarkOwner *m_pOwner;
  QPopupMenu * m_parentMenu;
  QList<KBookmarkMenu> m_lstSubMenus;
  KActionCollection * m_actionCollection;
  QList<KAction> m_actions;
};

#endif
