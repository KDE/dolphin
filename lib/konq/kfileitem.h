/*  This file is part of the KDE project
    Copyright (C) 1997 David Faure <faure@kde.org>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/ 

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

class KMimeType;
class QPixmap;

class KFileItem
{
public:
  /**
   * Create an item representing a file, from a UDSEntry - e.g. returned by KDirLister
   * @param _entry the KIO entry used to get the file, contains info about it
   * @param _url the file url
   */
  KFileItem( UDSEntry& _entry, KURL& _url );
  /**
   * Create an item representing a file, from all the necessary info for it
   * @param _text the text showed for the file
   * @param _mode the file mode (according to stat())
   * Set to -1 if unknown. For local files, KFileItem will use stat().
   * @param _url the file url
   */
  KFileItem( QString _text, mode_t _mode, const KURL& _url );
  /**
   * Destructor
   */
  ~KFileItem() { }

  /**
   * @return the url of the file
   */
  const KURL & url() const { return m_url; }
  /**
   * @return the mode of the file, as returned by stat()
   */
  mode_t mode() const { return m_mode; }
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
  QString time( int which ) const;
  /**
   * @return true if the file is a local file
   */
  bool isLocalFile() const { return m_bIsLocalURL; }
  /**
   * @return a pixmap representing the file
   * Don't cache it, don't delete it. It's handled by KPixmapCache !
   *
   * The method is named getPixmap because it actually determines the pixmap
   * each time. For a KIconContainerItem derived class, use getPixmap
   * then setPixmap to store the pixmap in the item, then pixmap() to get it later.
   * getPixmap() should only be called it might have changed
   */
  QPixmap* getPixmap( bool _mini ) const;
  /**
   * @return the text of the file item
   * It's not exactly the filename since some decoding happens ('%2F'->'/')
   * Named "getText" instead of "text" for the same reason as above.
   */
  QString getText() const { return m_strText; }

  /**
   * @return the mimetype of the file item
   */
  QString mimetype() const;

  /**
   * @return the UDS entry. Used by the tree view to access all details
   * by position.
   */
  const UDSEntry & entry() const { return m_entry; }

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
  static const char* makeTimeString( time_t _time );
  
protected:
  /**
   * Computes the text, mode, and mimetype from the UDSEntry
   * Called by constructor, but can be called again later
   */
  void init();
  
  /**
   * We keep a copy of the UDSEntry since we need it for @ref #getStatusBarInfo
   */
  UDSEntry m_entry;
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
   * The mode (as given by stat()) for the file - or the link-dest for a link
   */
  mode_t m_mode;
  /**
   * Whether the file is a link
   */
  bool m_bLink; 
  /**
   * The mimetype of the file
   */
  KMimeType* m_pMimeType;

private:
  /**
   * Marked : see @ref #mark()
   */
  bool m_bMarked;
};

typedef QList<KFileItem> KFileItemList;
typedef QListIterator<KFileItem> KFileItemListIterator;

#endif
