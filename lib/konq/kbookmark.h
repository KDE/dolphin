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

#include <ksimpleconfig.h>
#include <kglobal.h>
#include <kstddirs.h>

class KBookmarkManager;
class KBookmark;

/**
 * This class is an abstraction of a "real" bookmark.  It has
 * knowledge of properties like its name (possibly squeezed for size)
 * and URL and pixmap and type.. etc.  It also contains a pointer to
 * other bookmarks.
 *
 * For the most part, though, you should never use this class
 * directly.  KBookmarkManager is responsible for managing the
 * bookmarks.  Use that instead.
 *
 * @internal
 */
class KBookmark
{
  friend KBookmarkManager;
  
public:
  enum { URL, Folder };

  /**
   * Given a name and a URL, this will create a Folder bookmark.  It
   * will further save this to disk!
   */
  KBookmark( KBookmarkManager *, KBookmark *_parent, QString _text,
             QString _url );
  
  /**
   * Text shown for the bookmark
   */
  QString text() { return stringSqueeze( m_text ); }
  QString url() { return m_url; }
  int type() { return m_type; }
  int id() { return m_id; }
  QString file() { return m_file; }
  QString pixmapFile();
  QPixmap pixmap();
  
  /**
   * Append a new bookmark to the list.
   */
  void append( KBookmark *_bm ); 
  
  QList<KBookmark> *children() { return &m_lstChildren; }
  
  KBookmark* findBookmark( int _id );

  static QString stringSqueeze( const QString & str, unsigned int maxlen = 30 );
 
protected:
  /**
   * Creates a folder.
   */
  KBookmark( KBookmarkManager *, KBookmark *_parent, QString _text );
  /**
   * Creates a bookmark from a file.
   */
  KBookmark( KBookmarkManager *, KBookmark *_parent, QString _text,
             KSimpleConfig& _cfg, const char * _group );

  void clear();
  
  QString m_text;
  QString m_url;
  QString m_file;
  
  QString m_sPixmap;
  
  int m_type;
  int m_id;
  
  QList<KBookmark> m_lstChildren;

  KBookmarkManager *m_pManager;
};

/**
 * This class holds the data structures for the bookmarks and does
 * most of the work behind the scenes.  You may not have to do
 * anything with this class directly if you will accept certain
 * defaults.
 *
 * Specifically, the bookmark classes will use two defaults with this
 * class:
 *
 * 1) The bookmark path is <kde_path>/share/apps/kfm/bookmarks
 * 2) @ref editBookmarks will use the default bookmark editor to edit
 *    the bookmarks
 *
 * If you need to modify either behavior, you must derive your own
 * class from this one and specify your own path and overload
 * @ref editBookmarks
 */
class KBookmarkManager : public QObject
{
  friend KBookmark;
  
  Q_OBJECT
public:
  /**
   * Creates a bookmark manager with a path to the bookmarks.  By
   * default, it will use the KDE standard dirs to find and create the
   * correct location.  If you are using your own app-specific
   * bookmarks directory, you must instantiate this class with your
   * own path <em>before</em> KBookmarkManager::self() is every
   * called.
   */
  KBookmarkManager( QString _path = KGlobal::dirs()->saveLocation("data", "kfm/bookmarks", true) );

  /**
   * Destructor
   */
  virtual ~KBookmarkManager();

  /**
   * This static function will return an instance of the
   * KBookmarkManager.  If you do not instantiate this class either
   * natively or in a derived class, then it will return an object
   * with the default behaviors.  If you wish to use different
   * behaviors, you <em>must</em> derive your own class and
   * instantiate it before this method is ever called.
   *
   * @return a pointer to an instance of the KBookmarkManager.
   */
  static KBookmarkManager* self();

  /**
   * Called if the user wants to edit the bookmarks.  It will use the
   * default bookmarks editor unless you overload it.
   */
  virtual void editBookmarks( const char *_url );

  /**
   * This will return the path that this manager is using to search
   * for bookmarks.  It is mostly used internally -- if you think you
   * need to use it, you are probably using KBookmarkManager wrong.
   *
   * @return the path containing the bookmarks
   */
  QString path() { return m_sPath; }

  /**
   * This will return the root bookmark.  It is used to iterate
   * through the bookmarks manually.  It is mostly used internally --
   * if you think you need to use it, you are probably using
   * KBookmarkManager wrong.
   *
   * @return the root (top-level) bookmark
   */
  KBookmark* root() { return m_Root; }

  /**
   * This will return the root bookmark of the toolbar menu.  It is
   * used in a manner similar to @ref root
   *
   * @return the root bookmark of the toolbar subdir
   */
  KBookmark* toolbar() { return m_Toolbar; }

  /**
   * @internal
   * Used by KBookmarkManager when a bookmark is selected
   */
  KBookmark* findBookmark( int _id ) { return m_Root->findBookmark( _id ); }

  /**
   * @internal
   * For internal use only
   */
  void emitChanged();

  
  
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
  void slotNotify( const QString &_url );
  /**
   * Connect this slot directly to the menu item "edit bookmarks"
   */
  void slotEditBookmarks();
  
protected:
  void scan( const char *filename );
  void scanIntern( KBookmark*, const char *filename );

  void disableNotify() { m_bNotify = false; }
  void enableNotify() { m_bNotify = true; }
    
  bool m_bAllowSignalChanged;
  bool m_bNotify;
  QString m_sPath;
  KBookmark * m_Root;
  KBookmark * m_Toolbar;

  /**
   * This list is to prevent infinite looping while
   * scanning directories with ugly symbolic links
   */
  QList<QString> m_lstParsedDirs;

  static KBookmarkManager* s_pSelf;
};

/**
 * The @ref KBookmarkMenu and @ref KBookmarkBar classes gives the user
 * the ability to either edit bookmarks or add their own.  In the
 * first case, the app may want to open the bookmark in a special way.
 * In the second case, the app <em>must</em> supply the name and the
 * URL for the bookmark.
 *
 * This class gives the app this callback-like ability.
 *
 * If your app does not give the user the ability to add bookmarks and
 * you don't mind using the default bookmark editor to edit your
 * bookmarks, then you don't need to overload this class at all.
 * Rather, just use something like:
 *
 * <CODE>
 * bookmarks = new KBookmarkMenu(new KBookmarkOwner(), ...)
 * </CODE>
 *
 * If you wish to use your own editor or allow the user to add
 * bookmarks, you must overload this class.
 */
class KBookmarkOwner
{
public:
  /**
   * This function is called if the user selects a bookmark.  It will
   * open up the bookmark in a default fashion unless you override it.
   */
  virtual void openBookmarkURL(const QString& _url);
  
  /**
   * This function is called whenever the user wants to add the
   * current page to the bookmarks list.  The title will become the
   * "name" of the bookmark.  You must overload this function if you
   * wish to give your users the ability to add bookmarks.
   *
   * @return the title of the current page.
   */
  virtual QString currentTitle() { return QString::null; }

  /**
   * This function is called whenever the user wants to add the
   * current page to the bookmarks list.  The URL will become the URL
   * of the bookmark.  You must overload this function if you wish to
   * give your users the ability to add bookmarks.
   *
   * @return the URL of the current page.
   */
  virtual QString currentURL() { return QString::null; }
};

#endif

