/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __konq_listviewitems_h__
#define __konq_listviewitems_h__

#include <qlistview.h>
#include <qstring.h>
#include <kio/global.h>

class KonqListViewWidget;
class KMimeType;
class KonqFileItem;
class KonqListViewDir;
class QPainter;

/**
 * One item in the tree
 */
class KonqListViewItem : public QListViewItem
{
public:
  /**
   * Create an item in the tree toplevel representing a file
   * @param _parent the parent widget, the tree view
   * @param _fileitem the file item created by KDirLister
   */
  KonqListViewItem( KonqListViewWidget *_listViewWidget, KonqFileItem* _fileitem );
  /**
   * Create an item representing a file, inside a directory
   * @param _treeview the parent tree view
   * @param _parent the parent widget, a directory item in the tree view
   * @param _fileitem the file item created by KDirLister
   */
  KonqListViewItem( KonqListViewWidget *_listViewWidget, KonqListViewDir *_parent, KonqFileItem* _fileitem );
  virtual ~KonqListViewItem() { }

  /** @return the file item held by this instance */
  KonqFileItem * item() { return m_fileitem; }

  virtual QString key( int _column, bool ) const;
  //virtual QString text( int column ) const;

  /** Call this before destroying the tree view (decreases reference count
   * on the view) */
  virtual void prepareToDie() { m_pListViewWidget = 0L; }
  virtual void paintCell( QPainter *_painter, const QColorGroup & cg,
                          int column, int width, int alignment );

protected:
  void init();

  QString makeNumericString( const KIO::UDSAtom &_atom ) const;
  QString makeTimeString( const KIO::UDSAtom &_atom ) const;
  QString makeAccessString( const KIO::UDSAtom &_atom ) const;
  QString makeTypeString( const KIO::UDSAtom &_atom ) const;

  /** Pointer to the file item in KDirLister's list */
  KonqFileItem* m_fileitem;

  /** Parent tree view */
  KonqListViewWidget* m_pListViewWidget;
};

/**
 * An item specialized for directories
 */
class KonqListViewDir : public KonqListViewItem
{
public:
  /**
   * Create an item in the tree toplevel representing a directory
   * @param _parent the parent widget, the tree view
   * @param _fileitem the file item created by KDirLister
   */
  KonqListViewDir( KonqListViewWidget *_parent, KonqFileItem* _fileitem );
  /**
   * Create an item representing a directory, inside a directory
   * @param _treeview the parent tree view
   * @param _parent the parent widget, a directory item in the tree view
   * @param _fileitem the file item created by KDirLister
   */
  KonqListViewDir( KonqListViewWidget *_treeview, KonqListViewDir * _parent, KonqFileItem* _fileitem );
  virtual ~KonqListViewDir();

  /**
   * Called when user opens the directory (inherited from QListViewItem).
   * Checks whether its contents is known (@see #setComplete).
   */
  virtual void setOpen( bool _open );
  /**
   * Set to true when contents is completely known (one sublevel only)
   */
  virtual void setComplete( bool _b ) { m_bComplete = _b; }

  /**
   * (inherited from QListViewItem)
   */
  virtual void setup();

  /**
   * URL of this directory
   * @param _trailing set to true for a trailing slash (see KURL)
   */
  QString url( int _trailing );

protected:
  bool m_bComplete;
};

#endif
