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

#ifndef __konq_treeview_h__
#define __konq_treeview_h__

#include "browser.h"
#include "konq_defs.h"

#include <qvaluelist.h>
#include <qlistview.h>
#include <qdict.h>
#include <qstringlist.h>
#include <qcursor.h>
#include <qpixmap.h>

struct KUDSAtom;
class QCursor;
class KURL;
class KfmTreeViewDir;
class KfmTreeViewItem;
class KfmTreeView;
class KonqTreeView;
class KMimeType;
class KFileItem;
class KDirLister;
class KonqPropsView;
class KonqSettings;

/**
 * One item in the tree
 */
class KfmTreeViewItem : public QListViewItem
{
public:
  /**
   * Create an item in the tree toplevel representing a file
   * @param _parent the parent widget, the tree view
   * @param _fileitem the file item created by KDirLister
   */
  KfmTreeViewItem( KfmTreeView *_parent, KFileItem* _fileitem );
  /**
   * Create an item representing a file, inside a directory
   * @param _treeview the parent tree view
   * @param _parent the parent widget, a directory item in the tree view
   * @param _fileitem the file item created by KDirLister
   */
  KfmTreeViewItem( KfmTreeView *_treeview, KfmTreeViewDir *_parent, KFileItem* _fileitem );
  virtual ~KfmTreeViewItem() { }

  virtual QString text( int column ) const;
  virtual QString key( int _column, bool ) const;

  /** Call this before destroying the tree view (decreases reference count
   * on the view) */
  virtual void prepareToDie() { m_pTreeView = 0L; }
  virtual void paintCell( QPainter *_painter, const QColorGroup & cg, int column, int width, int alignment );

  /** @return the file item held by this instance */
  KFileItem * item() { return m_fileitem; }

protected:
  void init();
  
  const char* makeNumericString( const KUDSAtom &_atom ) const;
  const char* makeTimeString( const KUDSAtom &_atom ) const;
  const char* makeAccessString( const KUDSAtom &_atom ) const;
  QString makeTypeString( const KUDSAtom &_atom ) const;

  /** Pointer to the file item in KDirLister's list */      
  KFileItem* m_fileitem;
  /** Parent tree view */
  KfmTreeView* m_pTreeView;
};

/**
 * An item specialized for directories
 */
class KfmTreeViewDir : public KfmTreeViewItem
{
public:
  /**
   * Create an item in the tree toplevel representing a directory
   * @param _parent the parent widget, the tree view
   * @param _fileitem the file item created by KDirLister
   */
  KfmTreeViewDir( KfmTreeView *_parent, KFileItem* _fileitem );
  /**
   * Create an item representing a directory, inside a directory
   * @param _treeview the parent tree view
   * @param _parent the parent widget, a directory item in the tree view
   * @param _fileitem the file item created by KDirLister
   */
  KfmTreeViewDir( KfmTreeView *_treeview, KfmTreeViewDir * _parent, KFileItem* _fileitem );
  virtual ~KfmTreeViewDir();

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

class KonqTreeView : public BrowserView
{
  friend class KfmTreeView;
  Q_OBJECT
public:
  KonqTreeView();
  virtual ~KonqTreeView();
  
  virtual void openURL( const QString &url, bool reload = false,
                        int xOffset = 0, int yOffset = 0 );

  virtual QString url();
  virtual int xOffset();
  virtual int yOffset();
  virtual void stop();

protected:
  virtual void resizeEvent( QResizeEvent * );

private:
  KfmTreeView *m_pTreeView;
};

/**
 * The tree view
 */
class KfmTreeView : public QListView
{
  friend KfmTreeViewItem;
  friend KfmTreeViewDir;

  Q_OBJECT
public:
  KfmTreeView( KonqTreeView *parent );
  ~KfmTreeView();
  
  void stop();
  QString url();

/*
  virtual void can( bool &copy, bool &paste, bool &move );
  virtual void copySelection();
  virtual void pasteSelection();
  virtual void moveSelection( const QCString &destinationURL );

  virtual void slotReloadTree();
  virtual void slotShowDot();
*/    
  struct iterator
  {
    KfmTreeViewItem* m_p;

    iterator() : m_p( 0L ) { }
    iterator( KfmTreeViewItem* _b ) : m_p( _b ) { }
    iterator( const iterator& _it ) : m_p( _it.m_p ) { }

    KfmTreeViewItem& operator*() { return *m_p; }
    KfmTreeViewItem* operator->() { return m_p; }
    bool operator==( const iterator& _it ) { return ( m_p == _it.m_p ); }
    bool operator!=( const iterator& _it ) { return ( m_p != _it.m_p ); }
    iterator& operator++();
    iterator operator++(int);
  };

  iterator begin() { iterator it( (KfmTreeViewItem*)firstChild() ); return it; }
  iterator end() { iterator it; return it; }

  virtual void openURL( const char* _url, int xOffset, int yOffset );

  virtual void openSubFolder( const KURL &_url, KfmTreeViewDir* _dir );

  virtual void selectedItems( QValueList<KfmTreeViewItem*>& _list );

  /** @return the KfmTreeViewDir which handles the directory _url */
  virtual KfmTreeViewDir * findDir ( const QString & _url );

public slots:
  virtual void slotOnItem( KfmTreeViewItem* _item );

protected slots:
  virtual void slotReturnPressed( QListViewItem *_item );
  virtual void slotRightButtonPressed( QListViewItem *_item, const QPoint &_global, int _column );

//  void slotSelectionChanged();

  // slots connected to the directory lister
  virtual void slotStarted( const QString & );
  virtual void slotCompleted();
  virtual void slotCanceled();
  virtual void slotUpdate();
  virtual void slotClear();
  virtual void slotNewItem( KFileItem * );
  virtual void slotDeleteItem( KFileItem * );

  virtual void slotCurrentChanged( QListViewItem* _item ) { slotOnItem( (KfmTreeViewItem*)_item ); }

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

  virtual void addSubDir( const KURL & _url, KfmTreeViewDir* _dir );
  virtual void removeSubDir( const KURL & _url );
  /** Common method for slotCompleted and slotCanceled */
  virtual void setComplete();

  virtual void popupMenu( const QPoint& _global );

  virtual bool isSingleClickArea( const QPoint& _point );

  virtual void drawContentsOffset( QPainter*, int _offsetx, int _offsety,
				   int _clipx, int _clipy,
				   int _clipw, int _cliph );

  virtual void focusInEvent( QFocusEvent* _event );

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
  KfmTreeViewDir* m_pWorkingDir;

  bool m_bTopLevelComplete;
  bool m_bSubFolderComplete;

  int m_iColumns;

  QDict<KfmTreeViewDir> m_mapSubDirs;

  KfmTreeViewItem* m_dragOverItem;
  KfmTreeViewItem* m_overItem;
  QStringList m_lstDropFormats;

  bool m_pressed;
  QPoint m_pressedPos;
  KfmTreeViewItem* m_pressedItem;

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
