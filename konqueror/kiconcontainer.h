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

#ifndef __kiconcontainer_h__
#define __kiconcontainer_h__

#include <qscrollview.h>
#include <qrect.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qpoint.h>
#include <qcolor.h>
#include <qlist.h>
#include <qdropsite.h>
#include <qstrlist.h>

#include "mousemode.h"

#include <list>
#include <string>

class KIconContainer;
class QPainter;

class KIconContainerItem
{
  friend KIconContainer;
  
public:
  KIconContainerItem( KIconContainer* _container, QPixmap& _pixmap, const char *_txt );
  KIconContainerItem( KIconContainer* _container );
  virtual ~KIconContainerItem() { }
  
  KIconContainer* container() { return m_pContainer; }
  
  virtual void setSelected( bool _selected ) { m_bSelected = _selected; }
  virtual bool isSelected() { return m_bSelected; }
  
  void setPixmap( QPixmap& _pixmap ) { m_pixmap = _pixmap; }
  QPixmap pixmap() { return m_pixmap; }
  virtual const char* text() { return m_strText.c_str(); }
  virtual void setText( const char *_text );
  
  virtual bool hasFixedPos() { return m_bFixedPos; }
  /**
   * @param _x is relative to the upper left corner of the container.
   * @param _y is relative to the upper left corner of the container.
   */
  virtual void setFixedPos( int _x, int _y );
  
  virtual int height();
  
  virtual bool isSmallerThen( KIconContainerItem* _item );

  virtual bool acceptsDrops( QStrList& /* _formats */ ) { return false; }
  
protected:
  const static int m_iPixmapTextHSpacing = 3;
  
  virtual void paint( QPainter* _painter, const QColorGroup _grp );
  virtual void paintFocus( QPainter *_painter, const QColorGroup &, const QRect& _rect );
  
  virtual void breakText();
  virtual void breakText( QPainter* );
  virtual void refresh();

  virtual const QColor& color();
  
  bool contains( int _x, int _y );
  
  void setPos( int _x, int _y ) { m_x = _x; m_y = _y; }
  int x() { return m_x; }
  int y() { return m_y; }
  
  QPixmap m_pixmap;
  int m_x;
  int m_y;
  // list<string> m_lstBrokeDownText;
  QRect m_textBoundingRect;
  string m_strText;
  string m_strBrokenText;
  bool m_bSelected;
  bool m_bFixedPos;
  KIconContainer* m_pContainer;

  bool m_bMarked;
};

class KIconContainer : public QScrollView, public QDropSite
{  
  friend KIconContainerItem;
  
  Q_OBJECT
public:
  enum DisplayMode { Horizontal, Vertical };
  
  KIconContainer( QWidget *_parent = 0L, const char *_name = 0L, WFlags _flags = 0 );
  virtual ~KIconContainer();
  
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

  virtual void refresh() { m_bDirty = true; viewport()->update(); }

  struct InternIcon
  {
    InternIcon() { m_pItem = 0L; m_id = KIconContainer::s_id++; }
    InternIcon( KIconContainerItem* _item ) { m_pItem = _item; m_id = KIconContainer::s_id++; }
    InternIcon( const InternIcon &_item ) { m_pItem = _item.m_pItem; m_id = _item.m_id; }
    KIconContainerItem& operator*() { return *m_pItem; }
    KIconContainerItem* operator->() { return m_pItem; }
    bool operator==( const InternIcon& _it ) { return ( m_pItem == _it.m_pItem ); }
    bool operator!=( const InternIcon& _it ) { return ( m_pItem != _it.m_pItem ); }
    bool operator< ( InternIcon& _item ) { return m_pItem->isSmallerThen( _item.m_pItem ); }
    void free() { if ( m_pItem ) delete m_pItem; m_pItem = 0L; }
    int id() { return m_id; }
    KIconContainerItem* item() { return m_pItem; }
    KIconContainerItem* m_pItem;
    int m_id;
  };

  friend KIconContainer::InternIcon;
  
  typedef list<InternIcon>::iterator iterator;

  virtual void insert( QPixmap& _pixmap, const char *_txt, int _x = -1, int _y = -1, bool _refresh = true );
  virtual void insert( KIconContainerItem* _item, int _x = -1, int _y = -1, bool _refresh = true );
  virtual void remove( KIconContainerItem* _item, bool _refresh = true );
  virtual void erase( iterator _it, bool _refresh = true );
  virtual void clear();
  
  virtual iterator begin() { return m_lstIcons.begin(); }
  virtual iterator end() { return m_lstIcons.end(); }

  virtual int itemWidth() { return m_iItemWidth; }
  virtual int minItemWidth() { return m_iMinItemWidth; }
  virtual void setMinItemWidth( int _width ) { m_iMinItemWidth = _width; refreshAll(); }
  virtual int minItemHeight() { return m_iMinItemHeight; }
  virtual void setMinItemHeight( int _min ) { m_iMinItemHeight = _min; refreshAll(); }

  virtual void setPixmapSize( const QSize& _size ) { m_pixmapSize = _size; refreshAll(); }
  virtual QSize pixmapSize() { return m_pixmapSize; }
  
  virtual DisplayMode displayMode() { return m_displayMode; }
  virtual void setDisplayMode( DisplayMode _m );
  
  virtual KIconContainerItem* currentItem();
  virtual void setCurrentItem( iterator _item );
  virtual void setCurrentItem( KIconContainerItem *_item );
  
  virtual void setSelected( KIconContainerItem* _item, bool _selected );
  virtual bool isSelected( KIconContainerItem* _item ) { return _item->isSelected(); }
  virtual void unselectAll();
  
signals:
  void dragStart( const QPoint& _hotspot, QList<KIconContainerItem>& _selected, QPixmap _pixmap );
  void mousePressed( KIconContainerItem* _item, const QPoint& _global, int _button );
  void mouseReleased( KIconContainerItem* _item, const QPoint& _global, int _button );
  void doubleClicked( KIconContainerItem* _item, const QPoint& _global, int _button );
  void returnPressed( KIconContainerItem* _item, const QPoint& _global );
  void currentChanged( KIconContainerItem* _item );
  void selectionChanged( KIconContainerItem* _item );
  void selectionChanged();
  void onItem( KIconContainerItem* _item );

  /**
   * @param _item may be 0L. This indicates that the drop occured on the background of the window
   */
  void drop( QDropEvent *_ev, KIconContainerItem* _item, QStrList& m_lstFormats );
  
protected:
  virtual void dragMoveEvent( QDragMoveEvent *_ev );
  virtual void dragEnterEvent( QDragEnterEvent *_ev );
  virtual void dragLeaveEvent( QDragLeaveEvent *_ev );
  virtual void dropEvent( QDropEvent *_ev );
  /**
   * Needed to get drop events of the viewport.
   */
  virtual bool eventFilter( QObject *o, QEvent *e );
  
  const static int m_iLineSpacing = 2;
  const static int m_iHeadSpacing = 5;
  const static int m_iFootSpacing = 5;
  const static int m_iColumnSpacing = 3;
  
  virtual void keyPressEvent( QKeyEvent *_ev );
  virtual void viewportMousePressEvent( QMouseEvent *_ev );
  virtual void viewportMouseReleaseEvent( QMouseEvent *_ev );
  virtual void viewportMouseMoveEvent( QMouseEvent *_ev );
  virtual void viewportMouseDoubleClickEvent( QMouseEvent *_ev );
  virtual void resizeEvent( QResizeEvent *_ev );

  virtual void drawContentsOffset( QPainter*, int _offsetx, int _offsety, int _clipx, int _clipy, int _clipw, int _cliph );
  virtual void drawBackground( QPainter* _painter, int _offsetx, int _offsety,
			       int _clipx, int _clipy, int _clipw, int _cliph );
  
  virtual void setup();
  virtual void refreshAll();
  virtual void repaintItem( iterator _item );

  virtual iterator itemAt( int _x, int _y );

  iterator idToIterator( int _id );
  iterator itemToIterator( KIconContainerItem* _item );
  
  list<InternIcon> m_lstIcons;

  QWidget* m_pDummy;
  
  bool m_bDirty;
  int m_iItemWidth;
  int m_iMinItemWidth;
  int m_iMinItemHeight;
  DisplayMode m_displayMode;
  int m_currentItemId;
  QSize m_pixmapSize;

  iterator m_dragOverItem;
  QStrList m_lstDropFormats;
  
  iterator m_pressedItem;
  QPoint m_pressedPos;
  bool m_pressed;
  
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

  bool m_onItem;

  static int s_id;
};

#endif
