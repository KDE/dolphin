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
#ifndef __konq_listviewwidget_h__
#define __konq_listviewwidget_h__

#include <qcursor.h>
#include <qpixmap.h>
#include <qintdict.h>
#include <qdict.h>
#include <qtimer.h>
#include <kurl.h>
#include <konqfileitem.h>
#include <klistview.h>

namespace KIO { class Job; }
class QCursor;
class KDirLister;
class KonqListViewItem;
class KonqListViewDir;
class KonqListView;
class KonqPropsView;
class KonqFMSettings;
class ListViewPropertiesExtension;

/**
 * The tree view widget (based on KListView).
 * Most of the functionality is here.
 */
class KonqListViewWidget : public KListView
{
  friend KonqListViewItem;
  friend KonqListViewDir;
  friend KonqListView;
  friend class ListViewBrowserExtension;

  Q_OBJECT
public:
  KonqListViewWidget( KonqListView *parent, QWidget *parentWidget, const QString& mode );
  ~KonqListViewWidget();

  void stop();
  const KURL & url();

  enum KonqListViewMode { DetailedList, MixedTree };

  struct iterator
  {
    KonqListViewItem* m_p;

    iterator() : m_p( 0L ) { }
    iterator( KonqListViewItem* _b ) : m_p( _b ) { }
    iterator( const iterator& _it ) : m_p( _it.m_p ) { }

    KonqListViewItem& operator*() { return *m_p; }
    KonqListViewItem* operator->() { return m_p; }
    bool operator==( const iterator& _it ) { return ( m_p == _it.m_p ); }
    bool operator!=( const iterator& _it ) { return ( m_p != _it.m_p ); }
    iterator& operator++();
    iterator operator++(int);
  };

  iterator begin() { iterator it( (KonqListViewItem*)firstChild() ); return it; }
  iterator end() { iterator it; return it; }

  virtual bool openURL( const KURL &url );

  virtual void openSubFolder( const KURL &_url, KonqListViewDir* _dir );

  /**
   * Used by KonqListViewItem, to know how to sort the file details
   * The KonqListViewWidget holds the configuration for it, which is why
   * it provides this method.
   * @returns a pointer to the column number, or 0L if the atom shouldn't be displayed
   */
  int * columnForAtom( int atom ) { return m_dctColumnForAtom[ atom ]; }

  void selectedItems( QValueList<KonqListViewItem*>& _list );
  KURL::List selectedUrls();

  /** @return the KonqListViewDir which handles the directory _url */
  virtual KonqListViewDir * findDir ( const QString & _url );

  /**
   * @return the Properties instance for this view. Used by the items.
   */
  KonqPropsView * props() { return m_pProps; }

  void setCheckMimeTypes( bool enable ) { m_checkMimeTypes = enable; }
  bool checkMimetypes() { return m_checkMimeTypes; }

  void setShowIcons( bool enable ) { m_showIcons = enable; }
  bool showIcons() { return m_showIcons; }

public slots:
  void slotOnItem( QListViewItem* _item );
  void slotOnViewport();

protected slots:
  // from QListView
  void slotReturnPressed( QListViewItem *_item );
  void slotRightButtonPressed( QListViewItem *_item, const QPoint &_global, int _column );
  void slotCurrentChanged( QListViewItem* _item ) { slotOnItem( _item ); }

  // slots connected to the directory lister
  void slotStarted( const QString & );
  void slotCompleted();
  void slotCanceled();
  void slotClear();
  void slotNewItems( const KonqFileItemList & );
  void slotDeleteItem( KonqFileItem * );

  void slotResult( KIO::Job * );

protected:
  void initConfig();
  QStringList readProtocolConfig( const QString & protocol );

  virtual void viewportDragMoveEvent( QDragMoveEvent *_ev );
  virtual void viewportDragEnterEvent( QDragEnterEvent *_ev );
  virtual void viewportDragLeaveEvent( QDragLeaveEvent *_ev );
  virtual void viewportDropEvent( QDropEvent *_ev );

  virtual void viewportMousePressEvent( QMouseEvent *_ev );
  virtual void viewportMouseMoveEvent( QMouseEvent *_ev );
  virtual void viewportMouseReleaseEvent( QMouseEvent *_ev );
  virtual void keyPressEvent( QKeyEvent *_ev );

  virtual void addSubDir( const KURL & _url, KonqListViewDir* _dir );
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
  KonqFMSettings * m_pSettings;

  /** View properties */
  KonqPropsView * m_pProps;

  /** If 0L, we are listing the toplevel.
   * Otherwise, m_pWorkingDir points to the directory item we are listing,
   * and all files found will be created under this directory item.
   */
  KonqListViewDir* m_pWorkingDir;
  // Cache, for findDir
  KonqListViewDir* m_lasttvd;

  /**
   * In which column should go each UDS atom
   * The UDS atom type is the key, the column number is the value
   * Not in the dict -> not shown.
   */
  QIntDict<int> m_dctColumnForAtom;

  bool m_bTopLevelComplete;
  bool m_bSubFolderComplete;

  int m_iColumns;

  QDict<KonqListViewDir> m_mapSubDirs;

  KonqListViewItem* m_dragOverItem;
  KonqListViewItem* m_overItem;
  QStringList m_lstDropFormats;

  bool m_pressed;
  QPoint m_pressedPos;
  KonqListViewItem* m_pressedItem;

  QCursor m_stdCursor;
  QCursor m_handCursor;
  QPixmap m_bgPixmap;

  // TODO remove this and use KonqFMSettings
  bool m_bSingleClick;
  bool m_bUnderlineLink;
  bool m_bChangeCursor;

  long int m_idShowDot;

  KonqListViewMode m_mode;
  bool m_showIcons;
  bool m_checkMimeTypes;  

  KURL m_url;

  KonqListView *m_pBrowserView;

};

#endif
