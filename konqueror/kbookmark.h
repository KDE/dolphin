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

#ifndef __kbookmark_h__
#define __kbookmark_h__

#include <qlist.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qpopupmenu.h>

#include <ksimpleconfig.h>

#include "kfm.h"
#include <openparts_ui.h>

class KBookmarkManager;
class KBookmark;

/**
 * A widget using the bookmarks must derive from this class. It imeplements
 * a kind of callback interface for the @ref KBookmarkMenu class.
 */
class KBookmarkOwner
{
public:
  /**
   * This function is called if the user selectes a bookmark. You must overload it.
   */
  virtual void openBookmarkURL( const char *_url ) = 0L;
  /**
   * @return the title of the current page. This is called if the user wants
   *         to add the current page to the bookmarks.
   */
  virtual QString currentTitle() = 0;
  /**
   * @return the URL of the current page. This is called if the user wants
   *         to add the current page to the bookmarks.
   */
  virtual QString currentURL() = 0;
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
  KBookmarkMenu( KBookmarkOwner *_owner, OpenPartsUI::Menu_ptr menu, KFM::Part_ptr part, bool _root = true );
 ~KBookmarkMenu();  

public slots:
  void slotBookmarksChanged();
  void slotBookmarkSelected( int _id );
  
protected:
  void fillBookmarkMenu( KBookmark *parent );

  bool m_bIsRoot;
  KBookmarkOwner *m_pOwner;
  OpenPartsUI::Menu_var m_vMenu;
  KFM::Part_var m_vPart;
  QList<KBookmarkMenu> m_lstSubMenus;
};

/**
 * Internal Class
 */
class KBookmark
{
  friend KBookmarkManager;
  
public:
  enum { URL, Folder };

  /**
   * Creates a real bookmark ( type = Folder ) and saves the bookmark on the disk.
   */
  KBookmark( KBookmarkManager *, KBookmark *_parent, const char *_text, const char *_url );
  
  const char *text() { return m_text; }
  const char *url() { return m_url; }
  int type() { return m_type; }
  int id() { return m_id; }
  const char* file() { return m_file; }
  QPixmap* pixmap( bool _mini );
  
  void append( KBookmark *_bm ) { m_lstChildren.append( _bm ); }
  
  QList<KBookmark> *children() { return &m_lstChildren; }
  
  KBookmark* findBookmark( int _id );
 
  static QString encode( const char* );
  static QString decode( const char* );
  
protected:
  /**
   * Creates a folder.
   */
  KBookmark( KBookmarkManager *, KBookmark *_parent, const char *_text );
  /**
   * Creates a bookmark from a file.
   */
  KBookmark( KBookmarkManager *, KBookmark *_parent, const char *_text, KSimpleConfig& _cfg );

  void clear();
  
  QString m_text;
  QString m_url;
  QString m_file;
  
  QPixmap* m_pPixmap;
  QPixmap* m_pMiniPixmap;
  
  int m_type;
  int m_id;
  
  QList<KBookmark> m_lstChildren;

  KBookmarkManager *m_pManager;
};

/**
 * This class holds the data structures for the bookmarks.
 * You must create one instance of this class on startup.
 * In addition you have to overload @ref editBookmarks.
 */
class KBookmarkManager : public QObject
{
  friend KBookmark;
  
  Q_OBJECT
public:
  KBookmarkManager();
  ~KBookmarkManager() {};

  /**
   * For internal use only
   */
  KBookmark* root() { return &m_Root; }
  /**
   * For internal use only
   */
  KBookmark* findBookmark( int _id ) { return m_Root.findBookmark( _id ); }

  /**
   * For internal use only
   */
  void emitChanged();

  /**
   * Called if the user wants to edit the bookmarks. You must somehow open a filemanager
   * and display the given URL.
   */
  virtual void editBookmarks( const char *_url ) = 0L;
  
  /**
   * @return a pointer to the instance of the KBookmarkManager. Make shure that
   *         an instance of this class or a derived one exists before you call this
   *         function. It wont return 0L in this case, but it will raise an error.
   */
  static KBookmarkManager* self();
  
signals:
  /**
   * For internal use only
   */
  void changed();

public slots:
  /**
   * If you know that something in the bookmarks directory tree changed, call this
   * function to update the bookmark menus. If the given URL is not of interest for
   * the bookmarks then nothing will happen.
   */
  void slotNotify( const char* _url );
  /**
   * For internal use only
   */
  void slotEditBookmarks();
  
protected:
  void scan( const char *filename );
  void scanIntern( KBookmark*, const char *filename );

  void disableNotify() { m_bNotify = false; }
  void enableNotify() { m_bNotify = true; }
    
  KBookmark m_Root;
  bool m_bAllowSignalChanged;
  bool m_bNotify;

  /**
   * This list is to prevent infinite looping while
   * scanning directories with ugly symbolic links
   */
  QList<QString> m_lstParsedDirs;

  static KBookmarkManager* s_pSelf;
};

#endif

