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
   * Append a new bookmark to the list.  Note that if the type is a
   * Folder, it will add the bookmark to the top of the list rather
   * then appending it to the bottom
   */
  void append( KBookmark *_bm ) { (_bm->type() == Folder) ? m_lstChildren.prepend( _bm) : m_lstChildren.append( _bm ); }
  
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

  /**
   * This list is to prevent infinite looping while
   * scanning directories with ugly symbolic links
   */
  QList<QString> m_lstParsedDirs;

  static KBookmarkManager* s_pSelf;
};

#endif

