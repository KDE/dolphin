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

#ifndef __konq_fileicon_h__
#define __konq_fileicon_h__

/*
 * A KFileIcon is a generic class to handle files represented by icons.
 * It includes functionalities such as mimetype, icon, ...
 *
 * TODO : a KFileIconLister (?) to handle listing & refreshing
 * Currently, it's duplicated by kdesktop, konq_iconview and konq_treeview
 */

#include <qstringlist.h>
#include <sys/stat.h>

#include <kio_interface.h>
#include <kurl.h>

class KMimeType;
class QPixmap;

class KFileIcon
{
public:
  /**
   * Create an icon representing a file
   * @param _entry the KIO entry used to get the file, contains info about it
   * @param _url the file url
   * @param _mini whether a mini icon should be used
   */
  KFileIcon( UDSEntry& _entry, KURL& _url, bool _mini );
  /**
   * Destructor
   */
  virtual ~KFileIcon() { }

  /**
   * @return the url of the file
   */
  virtual QString url() const { return m_url.url(); }
  /**
   * @return the name of the file icon (i.e. the filename)
   */
  virtual QString name() const { return m_name; }
  /**
   * @return the mode of the file
   */
  virtual mode_t mode() const { return m_mode; }
  /**
   * @return true if the file is a link
   */
  virtual bool isLink() const;
  /**
   * @return the mimetype of the file
   */
  virtual KMimeType* mimeType() const { return m_pMimeType; }
  /**
   * @return a pixmap representing the file
   * Don't cache it, don't delete it. It's handled by KPixmapCache !
   *
   * The method is named getPixmap because it actually determines the pixmap
   * each time. For a KIconContainerItem derived class, use getPixmap
   * then setPixmap to store the pixmap in the item, then pixmap() to get it later
   * getPixmap() should only be called it might have changed (e.g. m_bMini changed)
   */
  virtual QPixmap* getPixmap() const;

  // TODO : probably setMini is needed

  // Used when updating a directory - marked == seen when refreshing
  virtual bool isMarked() const { return m_bMarked; }
  virtual void mark() { m_bMarked = true; }
  virtual void unmark() { m_bMarked = false; }
  
  /**
   * @return the string to be displayed in the statusbar when the mouse 
   *         is over this item
   */
  virtual QString getStatusBarInfo() const;

  // TODO
  virtual bool acceptsDrops( QStringList& /* _formats */ ) const;

protected:
  /**
   * Computes the name, mode, and mimetype from the UDSEntry
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
   * Mini icon or not mini icon
   */
  bool m_bMini;
  /**
   * The mode (as given by stat()) for the file
   */
  mode_t m_mode;
  /**
   * True if local file
   */
  bool m_bIsLocalURL;

  /**
   * The file name (without path)
   */
  QString m_name;
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
