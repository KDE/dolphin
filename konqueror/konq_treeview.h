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

#include <qvaluelist.h>
#include <qlistview.h>
#include <qtimer.h>
#include <qdict.h>
#include <qstringlist.h>
#include <qcursor.h>
#include <qpixmap.h>
#include <kio_interface.h>
#include <kurl.h>

#include "konq_baseview.h"

#include "mousemode.h"

class KonqMainView;
class KfmTreeViewDir;
class KfmTreeViewItem;
class KonqKfmTreeView;
class KMimeType;
class KFileItem;
class KDirLister;
class KonqPropsView;

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
  KfmTreeViewItem( KonqKfmTreeView *_parent, KFileItem* _fileitem );
  /**
   * Create an item representing a file, inside a directory
   * @param _treeview the parent tree view
   * @param _parent the parent widget, a directory item in the tree view
   * @param _fileitem the file item created by KDirLister
   */
  KfmTreeViewItem( KonqKfmTreeView *_treeview, KfmTreeViewDir *_parent, KFileItem* _fileitem );
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
  
  const char* makeNumericString( const UDSAtom &_atom ) const;
  const char* makeTimeString( const UDSAtom &_atom ) const;
  const char* makeAccessString( const UDSAtom &_atom ) const;
  QString makeTypeString( const UDSAtom &_atom ) const;

  /** Pointer to the file item in KDirLister's list */      
  KFileItem* m_fileitem;
  /** Parent tree view */
  KonqKfmTreeView* m_pTreeView;
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
  KfmTreeViewDir( KonqKfmTreeView *_parent, KFileItem* _fileitem );
  /**
   * Create an item representing a directory, inside a directory
   * @param _treeview the parent tree view
   * @param _parent the parent widget, a directory item in the tree view
   * @param _fileitem the file item created by KDirLister
   */
  KfmTreeViewDir( KonqKfmTreeView *_treeview, KfmTreeViewDir * _parent, KFileItem* _fileitem );
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

/**
 * The tree view
 */
class KonqKfmTreeView : public QListView,
                        public KonqBaseView,
			virtual public Konqueror::KfmTreeView_skel
{
  friend KfmTreeViewItem;
  friend KfmTreeViewDir;

  Q_OBJECT
public:
  KonqKfmTreeView( KonqMainView *mainView = 0L );
  ~KonqKfmTreeView();

  virtual bool mappingOpenURL( Browser::EventOpenURL eventURL );
  virtual bool mappingFillMenuView( Browser::View::EventFillMenu_ptr viewMenu );
  virtual bool mappingFillMenuEdit( Browser::View::EventFillMenu_ptr editMenu );

  virtual void stop();
  virtual char *viewName() { return CORBA::string_dup("KonquerorKfmTreeView"); }
  
  virtual char *url();
  virtual CORBA::Long xOffset();
  virtual CORBA::Long yOffset();

  virtual void slotReloadTree();
  virtual void slotShowDot();
    
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

  // ********* TODO : move this to properties stuff
  virtual void setBgColor( const QColor& _color );
  virtual const QColor& bgColor() { return m_bgColor; }
  virtual void setTextColor( const QColor& _color );
  virtual const QColor& textColor() { return m_textColor; }
  virtual void setLinkColor( const QColor& _color );
  virtual const QColor& linkColor() { return m_linkColor; }
  virtual void setVLinkColor( const QColor& _color );
  virtual const QColor& vLinkColor() { return m_vLinkColor; }

  virtual void setStdFontName( const char *_name );
  virtual const char* stdFontName() { return m_stdFontName; }
  virtual void setFixedFontName( const char *_name );
  virtual const char* fixedFontName() { return m_fixedFontName; }
  virtual void setFontSize( const int _size );
  virtual const int fontSize() { return m_fontSize; }

  virtual void setBgPixmap( const QPixmap& _pixmap );
  virtual const QPixmap& bgPixmap() { return m_bgPixmap; }

  virtual void setMouseMode( MouseMode _mode ) { m_mouseMode = _mode; }
  virtual MouseMode mouseMode() { return m_mouseMode; }

  virtual void setUnderlineLink( bool _underlineLink );
  virtual bool underlineLink() { return m_underlineLink; }
  virtual void setChangeCursor( bool _changeCursor ) { m_changeCursor = _changeCursor; }
  virtual bool changeCursor() { return m_changeCursor; };
  // ********

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
  virtual void dragMoveEvent( QDragMoveEvent *_ev );
  virtual void dragEnterEvent( QDragEnterEvent *_ev );
  virtual void dragLeaveEvent( QDragLeaveEvent *_ev );
  virtual void dropEvent( QDropEvent *_ev );
  /**
   * Needed to get drop events of the viewport.
   */
  virtual bool eventFilter( QObject *o, QEvent *e );

  virtual void mousePressEvent( QMouseEvent *_ev );
  virtual void mouseMoveEvent( QMouseEvent *_ev );
  virtual void mouseReleaseEvent( QMouseEvent *_ev );
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

  /** View properties */
  KonqPropsView * m_pProps;

  /** The view menu */
  OpenPartsUI::Menu_var m_vViewMenu;

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
  QStringList m_lstDropFormats;

  bool m_pressed;
  QPoint m_pressedPos;
  KfmTreeViewItem* m_pressedItem;

  QCursor m_stdCursor;
  QCursor m_handCursor;

  QColor m_bgColor;
  QColor m_textColor;
  QColor m_linkColor;
  QColor m_vLinkColor;

  QString m_stdFontName;
  QString m_fixedFontName;

  KfmTreeViewItem* m_overItem;

  int m_fontSize;

  QPixmap m_bgPixmap;

  MouseMode m_mouseMode;

  bool m_underlineLink;
  bool m_changeCursor;

  int m_iXOffset;
  int m_iYOffset;
  
  KonqMainView *m_pMainView;
};

#endif
