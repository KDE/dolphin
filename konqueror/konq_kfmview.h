/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

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

#ifndef __konq_kfmview_h__
#define __konq_kfmview_h__

/*
 * A "kfm view" is a view that shows files, as opposed to HTML and Part views
 * Konq_KfmView is the base class for Konq_IconView and Konq_TreeView,
 * and it deals with everything that is common to those views : popup menus,
 * most properties (fonts, color, mousemode, ...), statusbar information, ...
 *
 * This file also defines a KfmViewItem, the base class for KfmTreeViewItem
 * and KfmIconViewItem.
 */

#include "konq_baseview.h"

#include <kio_interface.h>
#include <kurl.h>

class KonqKfmView; //below
class KMimeType;

class KonqKfmViewItem
{
public:
  KonqKfmViewItem( UDSEntry& _entry, KURL& _url );
  virtual ~KonqKfmViewItem() { }

  virtual QString url() { return m_url.url(); }

  virtual bool isMarked() { return m_bMarked; }
  virtual void mark() { m_bMarked = true; }
  virtual void unmark() { m_bMarked = false; }
  
  virtual UDSEntry udsEntry() { return m_entry; }

  /*
   * @return the string to be displayed in the statusbar when the mouse 
   *         is over this item
   */
  virtual QString getStatusBarInfo();

  virtual bool acceptsDrops( QStrList& /* _formats */ );

  // Hmmmm...
  // virtual void popupMenu( const QPoint& _global, int _column );
  // virtual void popupMenu( const QPoint& _global );

  virtual KMimeType* mimeType() { return m_pMimeType; }

protected:
  void init();
  
  UDSEntry m_entry;
  KURL m_url;

  KMimeType* m_pMimeType;

  bool m_bMarked;
  bool m_bIsLocalURL;
};

#endif
