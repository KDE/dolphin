/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

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
// $Id$

#ifndef __konq_fileitem_h__
#define __konq_fileitem_h__

/*
 * A KFileItem is a generic class to handle files listed by KDirLister.
 * It includes functionalities such as mimetype, icon, text, mode, link, ...
 */

#include <qstringlist.h>
#include <sys/stat.h>

#include <qlist.h>
#include <kio_interface.h>
#include <kurl.h>
#include <kmimetype.h>

class QPixmap;

class KFileItem : public KIO
{
public:
  /**
   * Create an item representing a file, from a UDSEntry - e.g. returned by KDirLister
   * @param _entry the KIO entry used to get the file, contains info about it
   * @param _url the file url
   */
  KFileItem( const KUDSEntry& _entry, KURL& _url );
  /**
   * Create an item representing a file, from all the necessary info for it
   * @param _mode the file mode (according to stat())
   * Set to -1 if unknown. For local files, KFileItem will use stat().
   * @param _url the file url
   */
  KFileItem( mode_t _mode, const KURL& _url );
  /**
   * Destructor
   */
  ~KFileItem() { }

  /**
   * Re-read information (currently only permissions and mimetype)
   * This is called when the _file_ changes
   */
  void refresh();

  /**
   * Re-read mimetype information
   * This is called when the mimetype database changes
   */
  void refreshMimeType();

  /**
   * @return the url of the file
   */
  const KURL & url() const { return m_url; }
  /**
   * @return the permissions of the file (stat.st_mode containing only permissions)
   */
  mode_t permissions() const { return m_permissions; }
  /**
   * @return the file type (stat.st_mode containing only S_IFDIR, S_IFLNK, ...)
   */
  mode_t mode() const { return m_fileMode; }

  /**
   * @return the owner of the file.
   */
  QString user() const { return m_user; }

  /**
   * @return the group of the file.
   */
  QString group() const { return m_group; }

  /**
   * @return true if this icons represents a link in the UNIX sense of
   * a link. If yes, then we have to draw the label with an italic font.
   */
  bool isLink() const { return m_bLink; }
  /**
   * @return the link destination if isLink() == true
   */
  QString linkDest() const;
  /**
   * @return the size of the file, if known
   */
  long size() const;
  /**
   * @param which UDS_MODIFICATION_TIME, UDS_ACCESS_TIME or even UDS_CREATION_TIME
   * @return the time asked for, in string format
   */
  QString time( unsigned int which ) const;
  /**
   * @return true if the file is a local file
   */
  bool isLocalFile() const { return m_bIsLocalURL; }
  /**
   * Returns a pixmap representing the file
   * @param size KDE-size for the pixmap
   * @param bImagePreviewAllowed if true, an image file will return a pixmap
   * with the image, loaded from the xvpics dir (created if necessary)
   * @return the pixmap
   */
  QPixmap pixmap( KIconLoader::Size _size, bool bImagePreviewAllowed ) const;

  /**
   * @return the text of the file item
   * It's not exactly the filename since some decoding happens ('%2F'->'/')
   */
  QString text() const;

  /**
   * @return the mimetype of the file item
   */
  QString mimetype() const;

  /**
   * @return the descriptive comment for this mime type, or
   *         the mime type itself if none is present.
   */
  QString mimeComment() const;

  /**
   * @return the full path name to the icon that represents
   *         this mime type.
   */
  QString iconName() const;

  /**
   * @return the UDS entry. Used by the tree view to access all details
   * by position.
   */
  const KUDSEntry & entry() const { return m_entry; }

  // Used when updating a directory - marked == seen when refreshing
  bool isMarked() const { return m_bMarked; }
  void mark() { m_bMarked = true; }
  void unmark() { m_bMarked = false; }

  /**
   * @return the string to be displayed in the statusbar when the mouse
   *         is over this item
   */
  QString getStatusBarInfo() const;

  /**
   * @return true if files can be dropped over this item
   * Contrary to popular belief, not only dirs will return true :)
   * Executables, .desktop files, will do so as well.
   */
  bool acceptsDrops( ) const;

  /**
   * Let's "KRun" this file !
   * (called when file is clicked or double-clicked or return is pressed)
   */
  void run();

  /////////////

  /**
   * Encode (from the text displayed to the real filename)
   * This translates % into %% and / into %2f
   * Not used here, but by the 'add bookmark' methods
   * But put here for consistency
   */
  static QString encodeFileName( const QString & _str );
  /**
   * Decode (from the filename to the text displayed)
   * This translates %2[fF] into / and %% into %
   */
  static QString decodeFileName( const QString & _str );

  /**
   * Convert a time information into a string
   */
  static QString makeTimeString( time_t _time );

protected:
  /**
   * Computes the text, mode, and mimetype from the UDSEntry
   * Called by constructor, but can be called again later
   */
  void init();

  /**
   * We keep a copy of the UDSEntry since we need it for @ref #getStatusBarInfo
   */
  KUDSEntry m_entry;
  /**
   * The url of the file
   */
  KURL m_url;
  /**
   * True if local file
   */
  bool m_bIsLocalURL;

  /**
   * The text for this item, i.e. the file name without path
   */
  QString m_strText;
  /**
   * The file mode
   */
  mode_t m_fileMode;
  /**
   * The permissions
   */
  mode_t m_permissions;

  /**
   * the user and group assigned to the file.
   */
  QString m_user, m_group;

  /**
   * Whether the file is a link
   */
  bool m_bLink;
  /**
   * The mimetype of the file
   */
  KMimeType::Ptr m_pMimeType;

private:
  /**
   * Marked : see @ref #mark()
   */
  bool m_bMarked;
};

/**
 * List of KFileItems
 */
typedef QList<KFileItem> KFileItemList;

/**
 * Iterator for KFileItemList
 */
typedef QListIterator<KFileItem> KFileItemListIterator;

#endif
