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
   * @param _url the file url
   */
  KFileItem::KFileItem( QString _text, mode_t _mode, KURL& _url );
  /**
   * Destructor
   */
  ~KFileItem() { }

  /**
   * @return the url of the file
   */
  QString url() const { return m_url.url(); }
  /**
   * @return the mode of the file
   */
  mode_t mode() const { return m_mode; }
  /**
   * @return true if the file is a link
   */
  bool isLink() const;
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
   * then setPixmap to store the pixmap in the item, then pixmap() to get it later
   * getPixmap() should only be called it might have changed
   */
  QPixmap* getPixmap( bool _mini ) const;
  /**
   * @return the text of the file item (i.e. the filename)
   * Named "getText" instead of "text" for the same reason as above.
   */
  QString getText() const { return m_strText; }

  // Used when updating a directory - marked == seen when refreshing
  bool isMarked() const { return m_bMarked; }
  void mark() { m_bMarked = true; }
  void unmark() { m_bMarked = false; }
  
  /**
   * @return the string to be displayed in the statusbar when the mouse 
   *         is over this item
   */
  QString getStatusBarInfo() const;

  // TODO
  bool acceptsDrops( QStringList& /* _formats */ ) const;

  /**
   * Let's "KRun" this file !
   * (called when file is clicked or double-clicked or return is pressed)
   */
  void run();

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
   * The mode (as given by stat()) for the file
   */
  mode_t m_mode;
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

#endif
