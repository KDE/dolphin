/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
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

#ifndef __kfm_finder_h__
#define __kfm_finder_h__

#include <qlistview.h>
#include <qtimer.h>
#include <qdict.h>
#include <qstrlist.h>
#include <qcursor.h>

#include <string>

#include <k2url.h>

#include <kio_interface.h>

#include "kfm_abstract_gui.h"

class KfmFinderDir;
class KfmFinderItem;
class KfmFinder;
class KfmView;
class KMimeType;

class KfmFinderItem : public QListViewItem
{
public:
  KfmFinderItem( KfmFinder *_finder, KfmFinderDir *_parent, UDSEntry& _entry, K2URL& _url );
  KfmFinderItem( KfmFinder *_parent, UDSEntry& _entry, K2URL& _url );
  virtual ~KfmFinderItem() { }

  virtual const char* name() const { return text( 0 ); }
  virtual const char* text( int column ) const;
  virtual const char* key( int _column, bool ) const;

  virtual const char* url() { return m_strURL.c_str(); }

  // virtual void popupMenu( const QPoint& _global, int _column );
  virtual void returnPressed();
  
  virtual bool isMarked() { return m_bMarked; }
  virtual void mark() { m_bMarked = true; }
  virtual void unmark() { m_bMarked = false; }

  virtual UDSEntry& udsEntry() { return m_entry; }

  virtual bool acceptsDrops( QStrList& /* _formats */ );

  virtual void prepareToDie() { m_pFinder = 0L; }
  virtual void paintCell( QPainter *_painter, const QColorGroup & cg, int column, int width, int alignment );
  virtual KMimeType* mimeType() { return m_pMimeType; }

protected:
  virtual void init( KfmFinder* _finder, UDSEntry& _entry, K2URL& _url );

  const char* makeNumericString( const UDSAtom &_atom ) const;
  const char* makeTimeString( const UDSAtom &_atom ) const;
  const char* makeAccessString( const UDSAtom &_atom ) const;
  const char* makeTypeString( const UDSAtom &_atom ) const;
  
  UDSEntry m_entry;
  string m_strURL;
  KfmFinder* m_pFinder;
  KMimeType* m_pMimeType;
  
  bool m_bIsLocalURL;
  bool m_bMarked;
};

class KfmFinderDir : public KfmFinderItem
{
public:
  KfmFinderDir( KfmFinder *_parent, UDSEntry& _entry, K2URL& _url );
  KfmFinderDir( KfmFinder *_finder, KfmFinderDir * _parent, UDSEntry& _entry, K2URL& _url );
  virtual ~KfmFinderDir();
  
  virtual void setOpen( bool _open );
  virtual void setComplete( bool _b ) { m_bComplete = _b; }
  
  virtual void setup();

  string url( int _trailing );
  
protected:
  bool m_bComplete;
}; 

class KfmFinder : public QListView
{
  friend KfmFinderItem;
  friend KfmFinderDir;
  
  Q_OBJECT
public:

  struct iterator
  {    
    KfmFinderItem* m_p;
    
    iterator() : m_p( 0L ) { }
    iterator( KfmFinderItem* _b ) : m_p( _b ) { }
    iterator( const iterator& _it ) : m_p( _it.m_p ) { }
    
    KfmFinderItem& operator*() { return *m_p; }
    KfmFinderItem* operator->() { return m_p; }
    bool operator==( const iterator& _it ) { return ( m_p == _it.m_p ); }
    bool operator!=( const iterator& _it ) { return ( m_p != _it.m_p ); }
    iterator& operator++();
    iterator operator++(int);
  };

  KfmFinder( KfmView *_parent );
  ~KfmFinder();
  
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
  
  virtual void setMouseMode( KfmAbstractGui::MouseMode _mode ) { m_mouseMode = _mode; }
  virtual KfmAbstractGui::MouseMode mouseMode() { return m_mouseMode; }
  
  virtual void setUnderlineLink( bool _underlineLink );
  virtual bool underlineLink() { return m_underlineLink; }
  virtual void setChangeCursor( bool _changeCursor ) { m_changeCursor = _changeCursor; }
  virtual bool changeCursor() { return m_changeCursor; };
  virtual void setShowingDotFiles( bool _isShowingDotFiles );
  virtual bool isShowingDotFiles() { return m_isShowingDotFiles; }

  virtual void openURL( const char* _url );
  
  virtual void openSubFolder( const char *_url, KfmFinderDir* _dir );

  virtual bool isLocalURL() { return m_bIsLocalURL; }

  virtual KfmView* view() { return m_pView; }
  virtual const char* url() { return m_strURL.c_str(); }
  
  iterator begin() { iterator it( (KfmFinderItem*)firstChild() ); return it; }
  iterator end() { iterator it; return it; }
  
  virtual void selectedItems( list<KfmFinderItem*>& _list );

  virtual void updateDirectory();
  
signals:
  void started( const char* _url );
  void completed();
  void canceled();
  void gotFocus();
  
public slots:
  virtual void slotCloseSubFolder( int _id );
  virtual void slotCloseURL( int _id );
  virtual void slotListEntry( int _id, UDSEntry& _entry );
  virtual void slotError( int _id, int _errid, const char *_errortext );
  
  virtual void slotBufferTimeout();

  virtual void slotOnItem( KfmFinderItem* _item );

protected slots:  
  virtual void slotReturnPressed( QListViewItem *_item );
  virtual void slotRightButtonPressed( QListViewItem *_item, const QPoint &_global, int _column );

  virtual void slotUpdateError( int _id, int _errid, const char *_errortext );
  virtual void slotUpdateFinished( int _id );
  virtual void slotUpdateListEntry( int _id, UDSEntry& _entry );
  virtual void slotDirectoryDirty( const char *_url );
  virtual void slotCurrentChanged( QListViewItem* _item ) { slotOnItem( (KfmFinderItem*)_item ); }

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
  
  virtual void updateDirectory( KfmFinderDir *_dir, const char *_url );

  virtual void addSubDir( const char* _url, KfmFinderDir* _dir );
  virtual void removeSubDir( const char *_url );

  virtual void popupMenu( const QPoint& _global );
  
  virtual bool isSingleClickArea( const QPoint& _point );

  virtual void drawContentsOffset( QPainter*, int _offsetx, int _offsety, 
				   int _clipx, int _clipy, 
				   int _clipw, int _cliph );

  virtual void focusInEvent( QFocusEvent* _event );

  string m_strURL;
  K2URL m_url;
  bool m_bIsLocalURL;
  
  KfmView* m_pView;

  KfmFinderDir* m_pWorkingDir;
  string m_strWorkingURL;
  K2URL m_workingURL;
  
  int m_jobId;

  bool m_bTopLevelComplete;
  bool m_bSubFolderComplete;

  int m_iColumns;
  
  list<UDSEntry> m_buffer;
  QTimer m_bufferTimer;

  QDict<KfmFinderDir> m_mapSubDirs;

  KfmFinderItem* m_dragOverItem;
  QStrList m_lstDropFormats;

  bool m_pressed;
  QPoint m_pressedPos;
  KfmFinderItem* m_pressedItem;

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

  KfmAbstractGui::MouseMode m_mouseMode;

  bool m_underlineLink;
  bool m_changeCursor;
  bool m_isShowingDotFiles;
};

#endif
