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

#ifndef __konq_treeview_h__
#define __konq_treeview_h__

#include <qlistview.h>
#include <qtimer.h>
#include <qdict.h>
#include <qstrlist.h>
#include <qcursor.h>
#include <qpixmap.h>

#include <string>

#include "konq_kfmview.h"
#include "mousemode.h"

class KfmTreeViewDir;
class KfmTreeViewItem;
class KonqKfmTreeView;
class KMimeType;

class KfmTreeViewItem : public QListViewItem,
                        public KonqKfmViewItem
{
public:
  KfmTreeViewItem( KonqKfmTreeView *_treeview, KfmTreeViewDir *_parent, UDSEntry& _entry, KURL& _url );
  KfmTreeViewItem( KonqKfmTreeView *_parent, UDSEntry& _entry, KURL& _url );
  virtual ~KfmTreeViewItem() { }

  virtual const QString name() const { return text( 0 ); }
  virtual QString text( int column ) const;
  virtual QString key( int _column, bool ) const;

  virtual void prepareToDie() { m_pTreeView = 0L; }
  virtual void paintCell( QPainter *_painter, const QColorGroup & cg, int column, int width, int alignment );

protected:
  virtual void init();
  
  const char* makeNumericString( const UDSAtom &_atom ) const;
  const char* makeTimeString( const UDSAtom &_atom ) const;
  const char* makeAccessString( const UDSAtom &_atom ) const;
  QString makeTypeString( const UDSAtom &_atom ) const;

  KonqKfmTreeView* m_pTreeView;
};

class KfmTreeViewDir : public KfmTreeViewItem
{
public:
  KfmTreeViewDir( KonqKfmTreeView *_parent, UDSEntry& _entry, KURL& _url );
  KfmTreeViewDir( KonqKfmTreeView *_treeview, KfmTreeViewDir * _parent, UDSEntry& _entry, KURL& _url );
  virtual ~KfmTreeViewDir();

  virtual void setOpen( bool _open );
  virtual void setComplete( bool _b ) { m_bComplete = _b; }

  virtual void setup();

  string url( int _trailing );

protected:
  bool m_bComplete;
};

class KonqKfmTreeView : public QListView,
                        public KonqBaseView,
			virtual public Konqueror::KfmTreeView_skel
{
  friend KfmTreeViewItem;
  friend KfmTreeViewDir;

  Q_OBJECT
public:

  virtual bool mappingOpenURL( Konqueror::EventOpenURL eventURL );
  virtual bool mappingFillMenuView( Konqueror::View::EventFillMenu viewMenu );
  virtual bool mappingFillMenuEdit( Konqueror::View::EventFillMenu editMenu );

  virtual void stop();
  virtual char *viewName() { return CORBA::string_dup("KonquerorKfmTreeView"); }

  virtual char *url();

  virtual void slotReloadTree();
  virtual void slotShowDot();
    
  virtual void openURLRequest( const char *_url );

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

  KonqKfmTreeView();
  ~KonqKfmTreeView();

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
  virtual void setShowingDotFiles( bool _isShowingDotFiles );
  virtual bool isShowingDotFiles() { return m_isShowingDotFiles; }

  virtual void openURL( const char* _url );

  virtual void openSubFolder( const char *_url, KfmTreeViewDir* _dir );

  virtual bool isLocalURL() { return m_bIsLocalURL; }

  iterator begin() { iterator it( (KfmTreeViewItem*)firstChild() ); return it; }
  iterator end() { iterator it; return it; }

  virtual void selectedItems( list<KfmTreeViewItem*>& _list );

  virtual void updateDirectory();

public slots:
  virtual void slotCloseSubFolder( int _id );
  virtual void slotCloseURL( int _id );
  virtual void slotListEntry( int _id, const UDSEntry& _entry );
  virtual void slotError( int _id, int _errid, const char *_errortext );

  virtual void slotBufferTimeout();

  virtual void slotOnItem( KfmTreeViewItem* _item );

protected slots:
  virtual void slotReturnPressed( QListViewItem *_item );
  virtual void slotRightButtonPressed( QListViewItem *_item, const QPoint &_global, int _column );

  virtual void slotUpdateError( int _id, int _errid, const char *_errortext );
  virtual void slotUpdateFinished( int _id );
  virtual void slotUpdateListEntry( int _id, const UDSEntry& _entry );
  virtual void slotDirectoryDirty( const char *_url );
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

  virtual void updateDirectory( KfmTreeViewDir *_dir, const char *_url );

  virtual void addSubDir( const char* _url, KfmTreeViewDir* _dir );
  virtual void removeSubDir( const char *_url );

  virtual void popupMenu( const QPoint& _global );

  virtual bool isSingleClickArea( const QPoint& _point );

  virtual void drawContentsOffset( QPainter*, int _offsetx, int _offsety,
				   int _clipx, int _clipy,
				   int _clipw, int _cliph );

  virtual void focusInEvent( QFocusEvent* _event );

  KURL m_url;
  bool m_bIsLocalURL;

  KfmTreeViewDir* m_pWorkingDir;
  string m_strWorkingURL;
  KURL m_workingURL;

  int m_jobId;

  bool m_bTopLevelComplete;
  bool m_bSubFolderComplete;

  int m_iColumns;

  list<UDSEntry> m_buffer;
  QTimer m_bufferTimer;

  QDict<KfmTreeViewDir> m_mapSubDirs;

  KfmTreeViewItem* m_dragOverItem;
  QStrList m_lstDropFormats;

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

  QString m_overItem;

  int m_fontSize;

  QPixmap m_bgPixmap;

  MouseMode m_mouseMode;

  bool m_underlineLink;
  bool m_changeCursor;
  bool m_isShowingDotFiles;
};

#endif
