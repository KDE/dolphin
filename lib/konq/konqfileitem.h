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

#include <kfileitem.h>

class QPixmap;

class KonqFileItem: public KFileItem
{
public:
  // Need to redeclare all constructors ! I want Java !!! :)
  KonqFileItem( const KIO::UDSEntry& entry, const KURL& url,
		bool determineMimeTypeOnDemand = false,
	        bool isDirectory = false )
 : KFileItem( entry, url, determineMimeTypeOnDemand, isDirectory ) {}

  KonqFileItem( mode_t mode, mode_t permissions, const KURL& url, bool determineMimeTypeOnDemand = false )
 : KFileItem( mode, permissions, url, determineMimeTypeOnDemand ) {}

  KonqFileItem( const KURL &url, const QString &mimeType, mode_t mode )
 : KFileItem( url, mimeType, mode ) {}

  /**
   * Returns a pixmap representing the file
   * @param size KDE-size for the pixmap
   * @param bImagePreviewAllowed if true, an image file will return a pixmap
   * with the image, loaded from the xvpics dir (created if necessary)
   * @return the pixmap
   */
  QPixmap pixmap( int _size, int _state=KIcon::DefaultState,
                  bool bImagePreviewAllowed=false ) const;

  /**
   * @return true if this item has a thumbnail
   */
  bool isThumbnail() const { return bThumbnail; }

protected:
  mutable bool bThumbnail;
};

/**
 * List of KonqFileItems
 */
typedef QList<KonqFileItem> KonqFileItemList;

/**
 * Iterator for KonqFileItemList
 */
typedef QListIterator<KonqFileItem> KonqFileItemListIterator;

#endif
