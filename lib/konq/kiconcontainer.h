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

#include "kicondrag.h"

#include <qscrollview.h>
#include <qrect.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qpoint.h>
#include <qcolor.h>
#include <qsortedlist.h>
#include <qdropsite.h>
#include <qstringlist.h>

class KIconContainer;
class QPainter;

/**
 * This class holds one icon, i.e. one item in the icon container
 * The icon can be anything, not necessarily the representation of a file
 */
class KIconContainerItem
{
  friend KIconContainer;
  
public:
  enum Flags { NoFlags = 0, Selected = 1, Invisible = 2 };

  /** Construct a "full" icon (we know all about it)
   * @param _container the parent container
   * @param _pixmap the pixmap drawn as the icon
   * @param _text the text displayed for the icon
   * @param _name a unique name to identify the icon (e.g. url)
   */
  KIconContainerItem( KIconContainer* _container, const QPixmap& _pixmap,
                      const QString& _text, const QString& _name );
  /**
   * Construct an "empty" icon, with empty pixmap, text and name
   */
  KIconContainerItem( KIconContainer* _container );
  /**
   * Destroy the item 
   */
  virtual ~KIconContainerItem() { }
  
  KIconContainer* container() { return m_pContainer; }
  
  QPoint position() const { return QPoint( m_x, m_y ); }
  int x() const { return m_x; }
  int y() const { return m_y; }

  virtual void setSelected( bool _selected, bool _refresh = true );
  virtual bool isSelected() const { return ( m_flags & Selected ) != 0; }
  virtual bool isVisible() const { return ( m_flags & Invisible ) == 0; }
  
  // Pixmap
  void setPixmap( const QPixmap& _pixmap ) { m_pixmap = _pixmap; }
  QPixmap pixmap() const { return m_pixmap; }
  // Text displayed
  virtual QString text() const { return m_strText; }
  virtual void setText( const QString& _text );
  // Unique identifier for the icon (e.g. url)
  virtual QString name() const { return m_strName; }
  virtual void setName( const QString& _name ) { m_strName = _name; }
  
  virtual bool hasFixedPos() const { return m_bFixedPos; }
  /**
   * @param _pos is relative to the upper left corner of the container.
   * @param _refresh
   */
  virtual void setFixedPos( const QPoint& _pos, bool _refresh = true );
  /** Call this to re-arrange icons */
  virtual void resetFixedPos() { m_bFixedPos = false; };
  
  virtual int height() const;
  virtual int width() const;
  QSize size() const { return QSize( width(), height() ); }

  /**
   * Bu default no drops are accepted.
   */
  virtual bool acceptsDrops( QStringList& /* _formats */ ) { return false; }

  bool operator< ( const KIconContainerItem& _c ) { return isSmallerThen( _c ); }
  bool operator== ( const KIconContainerItem& _c ) { return isEqual( _c ); }

protected:
  static const int s_iPixmapTextHSpacing = 3;

  /**
   * Used internally
   */
  virtual void setVisible( bool _visible );

  /**
   * Used internally
   */
  virtual bool isSmallerThen( const KIconContainerItem& _item );
  /**
   * Used internally
   */
  virtual bool isEqual( const KIconContainerItem& _item );

  virtual void paint( QPainter* _painter, bool _drag = false );
  virtual void paintFocus( QPainter *_painter, const QColorGroup &, const QRect& _rect );
  
  virtual void breakText();
  virtual void breakText( QPainter* );
  virtual void refresh( bool _display_mode_changed = false );

  virtual QColor color();
  
  bool contains( int _x, int _y );
  
  void setPos( int _x, int _y ) { m_x = _x; m_y = _y; }
  
  KIconContainer* m_pContainer;
  QPixmap m_pixmap;
  int m_x;
  int m_y;

  QRect m_textBoundingRect;
  QString m_strText;
  QString m_strBrokenText;
  QString m_strName;
  bool m_bFixedPos;
  Flags m_flags;

  bool m_bMarked;
};

class KIconContainer : public QScrollView, public QDropSite
{  
  friend KIconContainerItem;
  
  Q_OBJECT
public:
  enum MouseMode { DoubleClick, SingleClick };
  enum DisplayMode { Horizontal, SmallVertical, Vertical };
  
  KIconContainer( QWidget *_parent = 0L, const char *_name = 0L, WFlags _flags = 0 );
  virtual ~KIconContainer();
  
  virtual void setBgColor( const QColor& _color, bool _refresh = true );
  virtual QColor bgColor() const { return m_bgColor; }
  virtual void setTextColor( const QColor& _color, bool _refresh = true );
  virtual QColor textColor() const { return m_textColor; }
  virtual void setLinkColor( const QColor& _color, bool _refresh = true );
  virtual QColor linkColor() const { return m_linkColor; }
  virtual void setVLinkColor( const QColor& _color, bool _refresh = true );
  virtual QColor vLinkColor() const { return m_vLinkColor; }
  
  virtual void setStdFontName( const QString& _name, bool _refresh = true );
  virtual QString stdFontName() const { return m_stdFontName; }
  virtual void setFixedFontName( const QString& _name, bool _refresh = true );
  virtual QString fixedFontName() const { return m_fixedFontName; }
  virtual void setFontSize( const int _size, bool _refresh = true );
  virtual const int fontSize() const { return m_fontSize; }
  
  virtual void setBgPixmap( const QPixmap& _pixmap, bool _refresh = true );
  virtual QPixmap bgPixmap() const { return m_bgPixmap; }
  
  virtual void setMouseMode( MouseMode _mode ) { m_mouseMode = _mode; }
  virtual MouseMode mouseMode() const { return m_mouseMode; }
  
  virtual void setUnderlineLink( bool _underlineLink, bool _refresh = true );
  virtual bool underlineLink() const { return m_underlineLink; }
  virtual void setChangeCursor( bool _changeCursor ) { m_changeCursor = _changeCursor; }
  virtual bool changeCursor() const { return m_changeCursor; };
  virtual void setShowingDotFiles( bool _isShowingDotFiles, bool _refresh  = true );
  virtual bool isShowingDotFiles() const { return m_isShowingDotFiles; }

  /**
   * A convenience function. Creates an item and inserts it into the container
   */
  virtual void insert( const QPixmap& _pixmap, const QString& _txt, const QString& _name,
		       int _x = -1, int _y = -1, bool _refresh = true );
  /**
   * Insert an existing item into the container
   * @param _refresh tells whether the display should be updated after inserting. If you want to
   *                 do many insertions then avoid the refresh until the last item is inserted.
   */
  virtual void insert( KIconContainerItem* _item, int _x = -1, int _y = -1, bool _refresh = true );
  /**
   * Deletes the item.
   *
   * @param _refresh tells whether all icons should be rearranged or whether the
   *                 place of the removed icon is just going to be empty.
   *                 The default tells to rearrange the icons.
   */
  virtual void remove( KIconContainerItem* _item, bool _refresh = true );
  /**
   * Remove all icons from the view.
   */
  virtual void clear();
  
  typedef QListIterator<KIconContainerItem> iterator;

  virtual iterator begin() { QListIterator<KIconContainerItem> it( m_lstItems ); return it; }
  virtual iterator end() { QListIterator<KIconContainerItem> it( m_lstItems ); it.toLast(); return it; }
  /**
   * Locates the item with a given name in the container
   * @return the item, or 0L if not found
   */
  virtual KIconContainerItem * find( const QString& _name );

  virtual int itemWidth() const { return m_iItemWidth; }
  virtual int minItemWidth() const { return m_iMinItemWidth; }
  virtual void setMinItemWidth( int _width ) { m_iMinItemWidth = _width; refreshAll(); }
  virtual int minItemHeight() const { return m_iMinItemHeight; }
  virtual void setMinItemHeight( int _min ) { m_iMinItemHeight = _min; refreshAll(); }

  virtual void setPixmapSize( const QSize& _size ) { m_pixmapSize = _size; refreshAll(); }
  virtual QSize pixmapSize() const { return m_pixmapSize; }
  
  virtual DisplayMode displayMode() const { return m_displayMode; }
  virtual void setDisplayMode( DisplayMode _m );
  
  /**
   * Returns the icon that contains the given point. The point (0,0)
   * is the upper left corner of the viewports visible part.
   */
  virtual KIconContainerItem* itemAt( const QPoint& _pos );
  virtual KIconContainerItem* currentItem() { return m_currentItem; }
  /**
   * Changes the current item of the container.
   *
   * @param _item may be 0. In this case there is no item currently active.
   */
  virtual void setCurrentItem( KIconContainerItem *_item );
  
  virtual void setSelected( KIconContainerItem* _item, bool _selected );
  virtual bool isSelected( KIconContainerItem* _item ) { return _item->isSelected(); }
  /**
   * Selects all icons
   */
  virtual void selectAll();
  /**
   * Clears the selection and repaints all icons which changed their selection state.
   */
  virtual void unselectAll();
  
  virtual void rearrangeIcons();

signals:
  void mousePressed( KIconContainerItem* _item, const QPoint& _global, int _button );
  void mouseReleased( KIconContainerItem* _item, const QPoint& _global, int _button );
  void doubleClicked( KIconContainerItem* _item, const QPoint& _global, int _button );
  void returnPressed( KIconContainerItem* _item, const QPoint& _global );
  void currentChanged( KIconContainerItem* _item );
  void selectionChanged( KIconContainerItem* _item );
  void onItem( KIconContainerItem* _item );

  /**
   * @param _item may be 0L. This indicates that the drop occured on the background of the window
   */
  void drop( QDropEvent *_ev, KIconContainerItem* _item, QStringList& m_lstFormats );
  
protected:
  /**
   * Refresh the display and recalculate the geometry stuff.
   * It has to be assumed that the geometry of the icons themselve is still ok.
   */
  virtual void refresh() { m_bDirty = true; viewport()->update(); }

  virtual void dragMoveEvent( QDragMoveEvent *_ev );
  virtual void dragEnterEvent( QDragEnterEvent *_ev );
  virtual void dragLeaveEvent( QDragLeaveEvent *_ev );
  virtual void dropEvent( QDropEvent *_ev );
  /**
   * Needed to get drop events of the viewport.
   */
  virtual bool eventFilter( QObject *o, QEvent *e );
  
  static const int m_iLineSpacing = 2;
  static const int m_iHeadSpacing = 5;
  static const int m_iFootSpacing = 5;
  static const int m_iColumnSpacing = 3;
  static const int m_iPixTextSpace = 3;

  virtual void keyPressEvent( QKeyEvent *_ev );
  virtual void viewportMousePressEvent( QMouseEvent *_ev );
  virtual void viewportMouseReleaseEvent( QMouseEvent *_ev );
  virtual void viewportMouseMoveEvent( QMouseEvent *_ev );
  virtual void viewportMouseDoubleClickEvent( QMouseEvent *_ev );
  virtual void resizeEvent( QResizeEvent *_ev );

  virtual void drawContentsOffset( QPainter*, int _offsetx, int _offsety,
				   int _clipx, int _clipy, int _clipw, int _cliph );
  virtual void drawBackground( QPainter* _painter, int _offsetx, int _offsety,
			       int _clipx, int _clipy, int _clipw, int _cliph );
  
  virtual void setup();
  /**
   * Calls the @ref KIconContainerItem::refresh function for every item and then
   * calls @ref #refresh.
   */
  virtual void refreshAll( bool _display_mode_changed = false );
  /**
   * @param _erase tells whether the background should be repainted, too, or
   *               whether the icon should just be drawn ontop of the current stuff.
   */
  virtual void repaintItem( KIconContainerItem* _item, bool _erase = true );

  /**
   * Clears the area where the item is displayed right now.
   * This function is used if an item changes its position or is
   * removed entirely from the container.
   *
   * @ref #remove
   * @ref KIconContainerItem::setFixedPos
   */
  virtual void wipeOutItem( KIconContainerItem* _item );

  virtual void drawDragShadow( const QPoint& _offset );
  virtual void clearDragShadow();

  QSortedList<KIconContainerItem> m_lstItems;

  /**
   * We need this dummy to create a paint device.
   */
  QWidget* m_pDummy;
  
  bool m_bDirty;
  int m_iItemWidth;
  int m_iMinItemWidth;
  int m_iMinItemHeight;
  DisplayMode m_displayMode;
  KIconContainerItem* m_currentItem;
  QSize m_pixmapSize;

  KIconContainerItem* m_dragOverItem;
  QStringList m_lstDropFormats;
  KIconDrag::IconList m_dragIcons;
  QPoint m_dragIconsOffset;

  KIconContainerItem* m_pressedItem;
  QPoint m_pressedPos;
  bool m_pressed;
  KIconContainerItem* m_overItem;

  QCursor m_stdCursor;
  QCursor m_handCursor;
  
  QColor m_bgColor;
  QColor m_textColor;
  QColor m_linkColor;
  QColor m_vLinkColor;

  QString m_stdFontName;
  QString m_fixedFontName;

  int m_fontSize;

  QPixmap m_bgPixmap;

  MouseMode m_mouseMode;

  bool m_underlineLink;
  bool m_changeCursor;
  bool m_isShowingDotFiles;

  bool m_onItem;
};

#endif
