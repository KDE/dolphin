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

#ifndef __kfileitem_h__
#define __kfileitem_h__

#include <qstringlist.h>
#include <sys/stat.h>

#include <qlist.h>
#include <kio/global.h>
#include <kurl.h>
#include <kmimetype.h>

/*
 * A KFileItem is a generic class to handle a file, local or remote.
 * In particular, it makes it easier to handle the result of KIO::listDir.
 * (UDSEntry isn't very friendly to use)
 * It includes many file attributes such as mimetype, icon, text, mode, link...
 */
class KFileItem
{
public:
  /**
   * Create an item representing a file, from an UDSEntry (see kio/global.h)
   * This is the preferred constructor when using KIO::listDir().
   *
   * @param _entry the KIO entry used to get the file, contains info about it
   * @param _url the file url
   * @param _determineMimeTypeOnDemand specify if the mimetype of the given URL
   *       should be determined immediately or on demand
   */
  KFileItem( const KIO::UDSEntry& _entry, KURL& _url, bool _determineMimeTypeOnDemand = false );

  /**
   * Create an item representing a file, from all the necessary info for it
   * @param _mode the file mode (according to stat())
   * Set to -1 if unknown. For local files, KFileItem will use stat().
   * @param _mode the mode (S_IFDIR...)
   * @param _permissions the access permissions
   * If you set both the mode and the permissions, you save a ::stat() for
   * local files
   * Set to -1 if you don't know the mode or the permission.
   * @param _url the file url
   *
   * @param _determineMimeTypeOnDemand specify if the mimetype of the given URL
   *       should be determined immediately or on demand
   */
  KFileItem( mode_t _mode, mode_t _permissions, const KURL& _url, bool _determineMimeTypeOnDemand = false );

  /**
   * Create an item representing a file, for which the mimetype is already known
   * @param url the file url
   * @param mimeType the name of the file's mimetype
   * @param mode the mode (S_IFDIR...)
   */
  KFileItem( const KURL &url, const QString &mimeType, mode_t mode );

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
   * @return the time asked for
   */
  time_t time( unsigned int which ) const;

  /**
   * @return true if the file is a local file
   */
  bool isLocalFile() const { return m_bIsLocalURL; }

  /**
   * @return the text of the file item
   * It's not exactly the filename since some decoding happens ('%2F'->'/')
   */
  QString text() const { return m_strText; }

  /**
   * @return the mimetype of the file item
   */
  QString mimetype() const;

  /**
   * @return the mimetype of the file item
   * If determineMimeTypeOnDemand was used, this will determine the mimetype first.
   */
  KMimeType::Ptr determineMimeType();
  /**
   * @return the currently-known mmietype of the file item
   * This will not try to determine the mimetype if unknown.
   */
  KMimeType::Ptr mimeTypePtr();

  /**
   * @return the descriptive comment for this mime type, or
   *         the mime type itself if none is present.
   */
  QString mimeComment();

  /**
   * @return the full path name to the icon that represents
   *         this mime type.
   */
  QString iconName();

  /**
   * @return the UDS entry. Used by the tree view to access all details
   * by position.
   */
  const KIO::UDSEntry & entry() const { return m_entry; }

  // Used when updating a directory - marked == seen when refreshing
  bool isMarked() const { return m_bMarked; }
  void mark() { m_bMarked = true; }
  void unmark() { m_bMarked = false; }

  /////////////

protected:
  /**
   * Computes the text, mode, and mimetype from the UDSEntry
   * Called by constructor, but can be called again later
   */
  void init( bool _determineMimeTypeOnDemand );

  /**
   * We keep a copy of the UDSEntry since we need it for @ref #getStatusBarInfo
   */
  KIO::UDSEntry m_entry;
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
