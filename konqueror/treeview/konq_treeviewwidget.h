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
#ifndef __konq_treeviewwidget_h__
#define __konq_treeviewwidget_h__

#include <qlistview.h>
#include <qcursor.h>
#include <qpixmap.h>
#include <qdict.h>

struct KUDSAtom;
class QCursor;
class KURL;
class KDirLister;
class KonqTreeViewItem;
class KonqTreeViewDir;
class KonqTreeView;
class KFileItem;
class KonqPropsView;
class KonqSettings;

/**
 * The tree view
 */
class KonqTreeViewWidget : public QListView
{
  friend KonqTreeViewItem;
  friend KonqTreeViewDir;
  friend KonqTreeView;

  Q_OBJECT
public:
  KonqTreeViewWidget( KonqTreeView *parent );
  ~KonqTreeViewWidget();
  
  void stop();
  QString url();

  struct iterator
  {
    KonqTreeViewItem* m_p;

    iterator() : m_p( 0L ) { }
    iterator( KonqTreeViewItem* _b ) : m_p( _b ) { }
    iterator( const iterator& _it ) : m_p( _it.m_p ) { }

    KonqTreeViewItem& operator*() { return *m_p; }
    KonqTreeViewItem* operator->() { return m_p; }
    bool operator==( const iterator& _it ) { return ( m_p == _it.m_p ); }
    bool operator!=( const iterator& _it ) { return ( m_p != _it.m_p ); }
    iterator& operator++();
    iterator operator++(int);
  };

  iterator begin() { iterator it( (KonqTreeViewItem*)firstChild() ); return it; }
  iterator end() { iterator it; return it; }

  virtual void openURL( const char* _url, int xOffset, int yOffset );

  virtual void openSubFolder( const KURL &_url, KonqTreeViewDir* _dir );

  virtual void selectedItems( QValueList<KonqTreeViewItem*>& _list );

  /** @return the KonqTreeViewDir which handles the directory _url */
  virtual KonqTreeViewDir * findDir ( const QString & _url );

public slots:
  virtual void slotOnItem( KonqTreeViewItem* _item );

protected slots:
  virtual void slotReturnPressed( QListViewItem *_item );
  virtual void slotRightButtonPressed( QListViewItem *_item, const QPoint &_global, int _column );

  // slots connected to the directory lister
  virtual void slotStarted( const QString & );
  virtual void slotCompleted();
  virtual void slotCanceled();
  virtual void slotClear();
  virtual void slotNewItem( KFileItem * );
  virtual void slotDeleteItem( KFileItem * );

  virtual void slotCurrentChanged( QListViewItem* _item ) { slotOnItem( (KonqTreeViewItem*)_item ); }

protected:
  virtual void initConfig();
  virtual void viewportDragMoveEvent( QDragMoveEvent *_ev );
  virtual void viewportDragEnterEvent( QDragEnterEvent *_ev );
  virtual void viewportDragLeaveEvent( QDragLeaveEvent *_ev );
  virtual void viewportDropEvent( QDropEvent *_ev );

  virtual void viewportMousePressEvent( QMouseEvent *_ev );
  virtual void viewportMouseMoveEvent( QMouseEvent *_ev );
  virtual void viewportMouseReleaseEvent( QMouseEvent *_ev );
  virtual void keyPressEvent( QKeyEvent *_ev );

  virtual void addSubDir( const KURL & _url, KonqTreeViewDir* _dir );
  virtual void removeSubDir( const KURL & _url );
  /** Common method for slotCompleted and slotCanceled */
  virtual void setComplete();

  virtual void popupMenu( const QPoint& _global );

  virtual bool isSingleClickArea( const QPoint& _point );

  virtual void drawContentsOffset( QPainter*, int _offsetx, int _offsety,
				   int _clipx, int _clipy,
				   int _clipw, int _cliph );

  virtual void focusInEvent( QFocusEvent* _event );

  KDirLister *dirLister() const { return m_dirLister; }

  /** The directory lister for this URL */
  KDirLister* m_dirLister;

  /** Konqueror settings */
  KonqSettings * m_pSettings;

  /** View properties */
  KonqPropsView * m_pProps;

  /** If 0L, we are listing the toplevel.
   * Otherwise, m_pWorkingDir points to the directory item we are listing,
   * and all files found will be created under this directory item.
   */
  KonqTreeViewDir* m_pWorkingDir;

  bool m_bTopLevelComplete;
  bool m_bSubFolderComplete;

  int m_iColumns;

  QDict<KonqTreeViewDir> m_mapSubDirs;

  KonqTreeViewItem* m_dragOverItem;
  KonqTreeViewItem* m_overItem;
  QStringList m_lstDropFormats;

  bool m_pressed;
  QPoint m_pressedPos;
  KonqTreeViewItem* m_pressedItem;

  QCursor m_stdCursor;
  QCursor m_handCursor;
  QPixmap m_bgPixmap;

  bool m_bSingleClick;
  bool m_bUnderlineLink;
  bool m_bChangeCursor;

  int m_iXOffset;
  int m_iYOffset;
  
  long int m_idShowDot;
  
  QString m_strURL;

  KonqTreeView *m_pBrowserView;
};

#endif
