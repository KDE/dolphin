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

#include "kiconcontainer.h"

#include <qbitmap.h>
#include <qdragobject.h>
#include <qkeycode.h>
#include <qpainter.h>
#include <qpixmap.h>

#include <kapp.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstddirs.h>

#include <assert.h>
#include <ctype.h>

#include <X11/X.h>
#include <X11/Xlib.h>

/*******************************************************
 *
 * KIconContainer
 *
 *******************************************************/

KIconContainer::KIconContainer( QWidget *_parent, const char *_name, WFlags _flags ) :
  QScrollView( _parent, _name, _flags ), QDropSite( this )
{
  m_pDummy         = new QWidget( 0L );

  m_bDirty         = true;
  m_iMinItemWidth  = 90;
  m_pixmapSize     = QSize( 16, 16 );
  m_iMinItemHeight = fontMetrics().lineSpacing();
  m_displayMode    = Horizontal;
  m_currentItem    = 0;
  m_pressedItem    = 0;

  m_stdCursor      = KCursor().arrowCursor();
  m_handCursor     = KCursor().handCursor();

  m_dragOverItem   = 0;
  m_pressed        = false;
  m_onItem         = false;
  m_overItem       = 0;

  m_lstItems.setAutoDelete( true );

  refreshAll();
  /*
  // How many icons fit in the horizontal ?
  int x = viewport()->width() / m_iMinItemWidth;
  if ( x != 0 )
    // Device space among them
    m_iItemWidth = viewport()->width() / x;
  else
    // There does not fit a single icon in a line but
    // Nevertheless, we take at least the minimum width
    m_iItemWidth = m_iMinItemWidth;
  // Is the pixmap supposed to be heigher than the minimum item height ?
  // So we have to correct that.
  if ( m_pixmapSize.height() > m_iMinItemHeight )
    m_iMinItemHeight = m_pixmapSize.height(); */

  // QScrollView stuff
  setFontPropagation( AllChildren );
  setHScrollBarMode( AlwaysOff );
  viewport()->setBackgroundColor( colorGroup().base() );

  // Qt 1.41 hack to get drop events
  // viewport()->installEventFilter( this );
  viewport()->setAcceptDrops( true );
  
  viewport()->setFocusProxy( this );
  setFocusPolicy( TabFocus );
  viewport()->setMouseTracking( true );
}

KIconContainer::~KIconContainer()
{
  delete m_pDummy;
  
  clear();
}

bool KIconContainer::eventFilter( QObject *o, QEvent *e )
{
  // We are only interested in XDND Events of the viewport
  if ( o != viewport() )
     return false;
  
  if ( e->type() == QEvent::Drop )
  {
    dropEvent( (QDropEvent*)e );
    return true;
  }
  else if ( e->type() == QEvent::DragEnter )
  {
    dragEnterEvent( (QDragEnterEvent*)e );
    return true;
  }
  else if ( e->type() == QEvent::DragLeave )
  {
    dragLeaveEvent( (QDragLeaveEvent*)e );
    return true;
  }
  else if ( e->type() == QEvent::DragMove )
  {
    dragMoveEvent( (QDragMoveEvent*)e );
    return true;
  }

  // Let the parent do the rest.
  return QScrollView::eventFilter( o, e );
}

void KIconContainer::clear()
{
  // TODO: We should repaint here ????
  m_lstItems.clear();
}

KIconContainerItem *KIconContainer::find( const QString& _name )
{
  iterator it = begin();
  for( ; *it; ++it )
  {
    // debug code
    kdebug(KDEBUG_INFO, 1205, "::find '%s' | '%s'", (*it)->name().ascii(), _name.ascii() );

    if ( (*it)->name() == _name )
    {
      kdebug(KDEBUG_INFO, 1205, "OK '%s' = '%s'", (*it)->name().ascii(), _name.ascii() );
      return (*it);
    }
  }
  kdebug(KDEBUG_WARN, 1205, "not found !");
  return 0L;
}

void KIconContainer::setDisplayMode( DisplayMode _m )
{
  if ( m_displayMode == _m )
    return;
  
  m_displayMode = _m;

  // Choose some minimum width. These seem to look pretty
  if ( m_displayMode == Horizontal || m_displayMode == Vertical )
    m_iMinItemWidth = 90;
  else if ( m_displayMode == SmallVertical )
    m_iMinItemWidth = 180;
  else
    assert( 0 );

  // We need to update the icons
  refreshAll( true );
}

void KIconContainer::refreshAll( bool _display_mode_changed )
{
  // Calculate the minimum height a item will have
  if ( m_displayMode == SmallVertical )
  {
    // We need at least space for a line of text
    m_iMinItemHeight = fontMetrics().lineSpacing();
    // Is the pixmap supposed to be heigher than the minimum item height ?
    // So we have to correct that.
    if ( m_pixmapSize.height() > m_iMinItemHeight )
      m_iMinItemHeight = m_pixmapSize.height();
  }
  else if ( m_displayMode == Vertical || m_displayMode == Horizontal )
    m_iMinItemHeight = m_pixmapSize.height() + m_iPixTextSpace + fontMetrics().lineSpacing();
  else
    assert( 0 );

  // How many icons fit in the horizontal ?
  int x = viewport()->width() / m_iMinItemWidth;
  if ( x != 0 )
    // Device space among them
    m_iItemWidth = viewport()->width() / x - m_iColumnSpacing;
  else
    // There does not fit a single icon in a line but
    // Nevertheless, we take at least the minimum width
    m_iItemWidth = m_iMinItemWidth;
  
  // In the constructor there are no item => we are done
  // if( m_bInit )
  // return;

  // Lets make all icons break their text according
  // to the sizes we decided upon above.
  iterator it = begin();
  for( ; *it; ++it )
    (*it)->refresh( _display_mode_changed );

  // Redisplay
  refresh();
}

void KIconContainer::insert( const QPixmap& _pixmap, const QString& _txt,
                             const QString& _name, int _x, int _y, bool _refresh )
{
  KIconContainerItem *item = new KIconContainerItem( this, _pixmap, _txt, _name );
  insert(item, _x, _y, _refresh); // see just below
}

void KIconContainer::insert( KIconContainerItem* _item, int _x, int _y, bool _refresh )
{
  m_lstItems.append( _item );

  // Make sure that both or no coordinate is >= 0
  if ( _x >= 0 && _y >= 0 )
  {
    int x = _x + contentsX(); // is this right ? it wasn't in both equivalent methods (David)
    int y = _y + contentsY();
    _item->setFixedPos( QPoint( x, y ), false );
  }
  
  if ( _refresh )
    refresh();
}

void KIconContainer::remove( KIconContainerItem* _item, bool _refresh )
{
  if ( _item == m_currentItem )
    m_currentItem = 0;

  if ( !_refresh )
    wipeOutItem( _item );
  
  m_lstItems.removeRef( _item );

  if ( _refresh )
    refresh();
}

void KIconContainer::wipeOutItem( KIconContainerItem* _item )
{
  repaintContents( _item->x(), _item->y(), m_iItemWidth, _item->height() );

  /*
  // Erase the icon and redraw the background
  QPainter painter;
  painter.begin( viewport() );
  if ( m_bgPixmap.isNull() )
  {
    painter.translate( _item->x() - contentsX(), _item->y() - contentsY() );
    painter.eraseRect( 0, 0, m_iItemWidth, _item->height() );
  }
  else
  {
    QRect r( _item->x() - contentsX(), _item->y() - contentsY(), m_iItemWidth, _item->height() );
    painter.setClipRect( r );
    drawBackground( &painter, contentsX(), contentsY(), _item->x(), _item->y(), m_iItemWidth, _item->height() );
  }
    
  painter.end(); */
}

void KIconContainer::setCurrentItem( KIconContainerItem *_item )
{
  KIconContainerItem* old = m_currentItem;
  m_currentItem = _item;
  // There was another icon seletec until now ? => repaint it ...
  if ( old != 0 )
    repaintItem( old );
  
  if ( _item != 0 )
  {
    // Draw the item
    repaintItem( _item );
    // Make it visible
    int dx = 0;
    int dy = 0;
    int x = _item->x();
    int y = _item->y();
    int ih = _item->height();
    if ( x < contentsX() )
      dx = x - contentsX();
    else if ( x + m_iItemWidth > contentsX() + viewport()->width() )
    {
      dx = ( x + m_iItemWidth ) - ( contentsX() + viewport()->width() );
      if ( x - dx < contentsX() )
	dx = x - contentsX();
    }

    if ( y < contentsY() )
      dy = y - contentsY();
    else if ( y + ih > contentsY() + viewport()->height() )
    {
      dy = ( y + ih ) - ( contentsY() + viewport()->height() );
      if ( y - dy < contentsY() )
      {
	dy = y - contentsY();
      }
    }
    if ( dx != 0 || dy != 0 )
      scrollBy( dx, dy );

    emit currentChanged( _item );
    emit onItem( _item );
  }

  emit currentChanged( 0 );
}

void KIconContainer::selectAll()
{
  iterator it = begin();
  for( ; *it; ++it )
    if ( !(*it)->isSelected() )
    {
      setSelected( *it, false );
      (*it)->refresh();
    }
}

void KIconContainer::unselectAll()
{
  iterator it = begin();
  for( ; *it; ++it )
    if ( (*it)->isSelected() )
      setSelected( *it, false );
}

void KIconContainer::selectIcons( const QRect& rect, bool _refresh )
{
  iterator it = begin();
  for( ; *it; ++it )
  {
    bool inRect = rect.intersects( QRect((*it)->position(), (*it)->size()) );
    // select the icon's box intersects the rectangle [and not selected]
    // deselect if not in rectangle [and selected]
    if ( inRect != (*it)->isSelected() )
    {
      (*it)->setSelected( inRect, _refresh /* whether to emit signals */);
      repaintItem( (*it) );
    }
  }
}

void KIconContainer::rearrangeIcons()
{
  iterator it = begin();
  for( ; *it; ++it )
    (*it)->resetFixedPos();
  setup();
  viewport()->update();
}

void KIconContainer::setSelected( KIconContainerItem* _item, bool _selected )
{
  _item->setSelected( _selected, false );
  repaintItem( _item );

  emit selectionChanged( _item );
}

void KIconContainer::selectedItems( QList<KIconContainerItem> & _list )
{
  iterator it = begin();
  for( ; *it; ++it )
    if ( (*it)->isSelected() )
      _list.append( *it );
}

void KIconContainer::keyPressEvent( QKeyEvent *_ev )
{
  switch( _ev->key() )
    {
    case Key_Return:
      _ev->accept();
      {
	if ( m_currentItem == 0 )
	  return;
	QPoint p( m_currentItem->x() - contentsX(), m_currentItem->y() - contentsY() );
	p = mapToGlobal( p );
	emit returnPressed( m_currentItem, p );
	return;
      }
    case Key_Down:
      _ev->accept();
      /* if ( m_displayMode == Horizontal )
      {    
	if ( m_currentItem == 0 )
        {  
	  iterator it = begin();
	  setCurrentItem( it() );
	  return;
	}
	else
	{
	  for( ; it != end(); ++it )
	    if ( (*it)->x() == (*curr)->x() && (*it)->y() > (*curr)->y() )
	    {
	      setCurrentItem( it );
	      return;
	    }
	  it = end();
	    it--;
	    if ( it != end() && (*it)->y() > (*curr)->y() )
	      setCurrentItem( it );
	    return;
	}
	return;
      }
      else if ( m_displayMode == SmallVertical )
      {
	iterator it = idToIterator( m_currentItemId );
	if ( it == end() )
        {  
	  it = begin();
	  if ( it == end() )
	    return;
	}
	else
        {
	  it++;
	  if ( it == end() )
	    return;
	}
	setCurrentItem( it );
	return;
      }
      else
	assert( 0 ); */
    case Key_Up:
      _ev->accept();
      /* if ( m_displayMode == Horizontal )
      {
	iterator it = idToIterator( m_currentItemId );
	iterator curr = it;
	if ( it == end() )
        {  
	  it = begin();
	  if ( it == end() )
	    return;
	  setCurrentItem( it );
	  return;
	}
	else
        {
	  for( ; it != end(); --it )
	    if ( (*it)->x() == (*curr)->x() && (*it)->y() < (*curr)->y() )
	    {
	      setCurrentItem( it );
	      return;
	    }
	  it = begin();
	  if ( it != end() && (*it)->y() < (*curr)->y() )
	    setCurrentItem( it );
	  return;
	}
	return;
      }
      else if ( m_displayMode == SmallVertical )
      {
	iterator it = idToIterator( m_currentItemId );
	if ( it == end() )
        {  
	  it = begin();
	  if ( it == end() )
	    return;
	}
	else
        {
	  it--;
	  if ( it == end() )
	    return;
	}
	setCurrentItem( it );
	return;
      }
      else
	assert( 0 ); */
    case Key_Left:
      _ev->accept();
      /*if ( m_displayMode == Horizontal )
      {
	iterator it = idToIterator( m_currentItemId );
	if ( it == end() )
        {  
	  it = begin();
	  if ( it == end() )
	    return;
	}
	else
        {
	  it--;
	  if ( it == end() )
	    return;
	}
	setCurrentItem( it );
	return;
      }
      else if ( m_displayMode == SmallVertical )
      {
	iterator it = idToIterator( m_currentItemId );
	iterator curr = it;
	if ( it == end() )
        {  
	  it = begin();
	  if ( it == end() )
	    return;
	  setCurrentItem( it );
	  return;
	}
	else if ( it == begin() )
	  return;
	else
        {
	  do
	  {
	    it--;
	    if ( (*it)->y() <= (*curr)->y() && (*it)->x() < (*curr)->x() )
	    {
	      setCurrentItem( it );
	      return;
	    }
	  } while ( it != begin() );
	  
	  it = begin();
	  if ( it != end() && (*it)->x() < (*curr)->x() )
	    setCurrentItem( it );
	  return;
	}
      }
      else
	assert( 0 ); */
    case Key_Right:
      _ev->accept();
      /* if ( m_displayMode == Horizontal )
      {
	iterator it = idToIterator( m_currentItemId );
	if ( it == end() )
        {  
	  it = begin();
	  if ( it == end() )
	    return;
	}
	else
	{
	  it++;
	  if ( it == end() )
	    return;
	}
	setCurrentItem( it );
	return;
      }
      else if ( m_displayMode == SmallVertical )
      {
	iterator it = idToIterator( m_currentItemId );
	iterator curr = it;
	if ( it == end() )
        {  
	  it = begin();
	  if ( it == end() )
	    return;
	  setCurrentItem( it );
	  return;
	}
	else
        {
	  do
	  {
	    if ( (*it)->y() + (*it)->height() > (*curr)->y() && (*it)->x() > (*curr)->x() )
	    {
	      setCurrentItem( it );
	      return;
	    }
	    it++;
	  } while ( it != end() );
	  
	  it = begin();
	  if ( it != end() && (*it)->x() > (*curr)->x() )
	    setCurrentItem( it );
	  return;
	}
      }
      else
	assert( 0 ); */
    case Key_Space:
      _ev->accept();
      /* {
	iterator it = idToIterator( m_currentItemId );
	if ( it == end() )
	  return;

	setSelected( &**it, !(*it)->isSelected() );
	return;
      } */
    }
}

void KIconContainer::dragMoveEvent( QDragMoveEvent *_ev )
{
  clearDragShadow();

  // Find the item
  KIconContainerItem* it = itemAt( _ev->pos() );
  // Are we over some item ?
  if ( it )
  {
    // The drag is still over the same icon ?
    if ( m_dragOverItem == it )
    {
      drawDragShadow( _ev->pos() );
      return;
    }
    // unhighlight the icon over which the drag was before
    if ( m_dragOverItem != 0 )
      setSelected( m_dragOverItem, false );
    
    // Does the new item accept this drop ?
    if ( it->acceptsDrops( m_lstDropFormats ) )
    {
      _ev->accept();
      setSelected( it, true );
      m_dragOverItem = it;
    }
    else
    {
      // If we move icons around in the container the user
      // may drop them everywhere.
      if ( _ev->source() == viewport() )
	_ev->accept();
      else
	_ev->ignore();
      m_dragOverItem = 0;
    }
    drawDragShadow( _ev->pos() );
    return;
  }

  // We are over the background so unhighlight the icon
  // over which the mouse was before ( if any )
  if ( m_dragOverItem != 0 )
    setSelected( m_dragOverItem, false ); 
  m_dragOverItem = 0;

  drawDragShadow( _ev->pos() );

  _ev->accept();
}

void KIconContainer::dragEnterEvent( QDragEnterEvent *_ev )
{
  // Defensive programming ...
  m_dragOverItem = 0;

  // Save the available formats
  m_lstDropFormats.clear();
  
  for( int i = 0; _ev->format( i ); i++ )
  {
    debug("Got offer %s", _ev->format( i ) );
    if ( *( _ev->format( i ) ) )
      m_lstDropFormats.append( _ev->format( i ) );
  }
  
  // Is it a KIconDrag ?
  if ( KIconDrag::decode( _ev, m_dragIcons ) )
  {
    drawDragShadow( _ev->pos() );
    debug("Yaeh ... I got an iconlist\n");
  }
  // ... defensive Programming
  else
    m_dragIcons.clear();

  // By default we accept any format
  _ev->accept();
}

void KIconContainer::dragLeaveEvent( QDragLeaveEvent * )
{
  // Did we highlight some icons ?
  if ( m_dragOverItem )
    setSelected( m_dragOverItem, false );
  m_dragOverItem = 0;

  if ( !m_dragIcons.isEmpty() )
  {
    clearDragShadow();
    m_dragIcons.clear();
  }
}

void KIconContainer::dropEvent( QDropEvent * e )
{
  if ( !m_dragIcons.isEmpty() )
    clearDragShadow();

  // Unhighlight the icon that got the drop ( if any )
  if ( m_dragOverItem  )
    setSelected( m_dragOverItem, false );
  m_dragOverItem = 0;
  

  KIconContainerItem* item = itemAt( e->pos() );
  
  // Background drop (if drop on no item OR moving icons)
  if ( !item || ( !item->acceptsDrops( m_lstDropFormats ) && e->source() == viewport() ) )
  {
    // Test whether this is our drag (moving icons around)
    if ( e->source() == viewport() )
    {
      kdebug(KDEBUG_INFO, 1205, "Moving icons around");
      // we already have the list of icons, no need to decode it
      KIconDrag::IconList::Iterator it = m_dragIcons.begin();
      for( ; it != m_dragIcons.end(); ++it )
      {
        // for each icon we need to find the item related to it
        KIconContainerItem* movedItem = find( (*it).url );
        if ( movedItem )
          movedItem->setFixedPos( e->pos() + (*it).pos );
        else
          kdebug(KDEBUG_ERROR, 1205, "Can't find item with url='%s' in the iconcontainer !", (*it).url.ascii() );
      }
    } else // It's not : emit event
    {
      kdebug(KDEBUG_INFO, 1205, "Dropping to desktop from other app");
      emit drop( e, 0L, m_lstDropFormats );
    }
  }
  // Drop on item
  else
  {
    kdebug(KDEBUG_INFO, 1205, "Dropping to desktop icon from other app");
    emit drop( e, item, m_lstDropFormats );
  }

  if ( !m_dragIcons.isEmpty() )
    m_dragIcons.clear();
}

void KIconContainer::viewportMousePressEvent( QMouseEvent *_ev )
{
  // TODO: What is that good for ... ?
  if ( !hasFocus() )
    setFocus();

  QPoint globalPos = mapToGlobal( _ev->pos() );
  m_pressed = false;
  
  if ( m_mouseMode == SingleClick )
  {    
    KIconContainerItem* it = itemAt( _ev->pos() );
    if ( it )
    {
      if ( m_overItem )
      {
	//reset to the standart cursor
	setCursor(m_stdCursor);
	m_overItem = 0;
      }

      // Toggle selection ?
      if ( ( _ev->state() & ControlButton ) && _ev->button() == LeftButton )
      {
	setSelected( it, !it->isSelected() );
	return;
      }
      if ( _ev->button() == RightButton && !it->isSelected() )
      {    
	unselectAll();
	setSelected( it, true );
      }
      else if ( _ev->button() == LeftButton || _ev->button() == MidButton )
      {    
	if ( !it->isSelected() )
        {      
	  unselectAll();
	  setSelected( it, true );
	}
	
	// This may be the beginning of DND
	m_pressed = true;
	m_pressedPos = _ev->pos();
	m_pressedItem = it;
	return;
      }
      
      emit mousePressed( it, globalPos, _ev->button() );
      return;
    }
    
    // Click on the background of the window
    if ( _ev->button() == LeftButton )
    {
      unselectAll();

      // Rectangular selection ?
      int x, y, dx, dy;
      x = globalPos.x();
      y = globalPos.y();
      dx = dy = 0;
      int cx, cy, rx, ry;
      int ox, oy;
      XEvent ev;
      drawSelectionRectangle(x, y, dx, dy);
  
      ox = x; oy = y; // initial position
      cx = x; cy = y; // always contain the previous position

      for(;;) {
        XMaskEvent(qt_xdisplay(), ButtonPressMask|ButtonReleaseMask|
                   PointerMotionMask, &ev);
    
        if (ev.type == MotionNotify){
          rx = ev.xmotion.x_root;
          ry = ev.xmotion.y_root;
        }
        else
          break; // mouse button released -> exit
        if (rx == cx && ry == cy) // no movement this time
          continue;
        cx = rx;
        cy = ry;
        
        drawSelectionRectangle(x, y, dx, dy);
    
        if (cx > ox){        
          x = ox;
          dx = cx - x;
        }
        else {
          x = cx;
          dx = ox - x;
        }
        if (cy > oy){        
          y = oy;
          dy = cy - y;
        }
        else {
          y = cy;
          dy = oy - y;
        }
        
        QRect r( x, y, dx, dy );
        selectIcons( r, false );
    
        drawSelectionRectangle(x, y, dx, dy);
    
        XFlush(qt_xdisplay());
      }
      
      drawSelectionRectangle(x, y, dx, dy);

      if (dx >= 5 || dy >= 5) // Did we really make a rectangular selection ?
        return;
      // If not, emit the event
    }
    emit mousePressed( 0L, globalPos, _ev->button() );
    return;
  }
  else if ( m_mouseMode == DoubleClick )
  {    
    KIconContainerItem* it = itemAt( _ev->pos() );
    if ( it )
    {
      if ( ( _ev->state() & ControlButton ) && _ev->button() == LeftButton )
      {
	setSelected( it, !it->isSelected() );
	return;
      }
      if ( _ev->button() == RightButton && !it->isSelected() )
      {    
	unselectAll();
	setSelected( it, true );

	emit mousePressed( it, globalPos, _ev->button() );
      }
      else if ( _ev->button() == LeftButton || _ev->button() == MidButton )
      {    
	unselectAll();
	setSelected( it, true );
      }
      return;
    }

    if ( _ev->button() == RightButton )
    {
      emit mousePressed( 0L, globalPos, _ev->button() );
      return;
    }
    if ( _ev->button() == LeftButton )
    {
      unselectAll();
      return;
    }
  }
  else
    assert( 0 );
}

void KIconContainer::drawSelectionRectangle( int x, int y, int dx, int dy )
{
  QPainter p;
  p.begin( viewport() );
  p.setRasterOp( NotROP );
  p.setPen( QPen( Qt::black /*, 1, Qt::DashDotLine */ ) ); // solid line looks better, no ?
  p.setBrush( Qt::NoBrush );
  p.drawRect( x, y, dx, dy );
  p.end();
  /* old krootwm code
    XDrawRectangle(qt_xdisplay(), qt_xrootwin(), gc, x, y, dx, dy);
    if (dx>2) dx-=2;
    if (dy>2) dy-=2;
    XDrawRectangle(qt_xdisplay(), qt_xrootwin(), gc, x+1, y+1, dx, dy);
  */
}

void KIconContainer::viewportMouseDoubleClickEvent( QMouseEvent *_ev )
{
  KIconContainerItem* it = itemAt( _ev->pos() );
  if ( it )
  {
    QPoint p = mapToGlobal( _ev->pos() );
    emit doubleClicked( it, p, _ev->button() );
    return;
  }
}

void KIconContainer::viewportMouseReleaseEvent( QMouseEvent *_mouse )
{
  // Defensive programming ...
  if ( !m_pressed )
    return;
  m_pressed = false;

  if ( _mouse->button() != LeftButton || ( _mouse->state() & ControlButton ) == ControlButton )
    return;

  QPoint p = mapToGlobal( m_pressedPos );

  if ( m_mouseMode == SingleClick )
    emit returnPressed( m_pressedItem, p );

  emit mouseReleased( m_pressedItem, p, LeftButton );
  return;
}

void KIconContainer::viewportMouseMoveEvent( QMouseEvent *_mouse )
{
  if ( !m_pressed )
  {
    KIconContainerItem* item = itemAt( _mouse->pos() );
    // The mouse moved over some icon ?
    if ( item )
    {
      // Are we over another icon now ?
      if ( m_overItem != item )
      {
	m_overItem = item;
	emit onItem( item );

	if ( m_mouseMode == SingleClick )
	  setCursor( m_handCursor );
      }
    }
    else
      if ( m_overItem )
      {
	emit onItem( 0L );
	
	setCursor(m_stdCursor);
	m_overItem = 0;
      }

    return;
  }
  
#warning "had to define Dnd_X_Precision (it's defined in drag.h which I don't want!) (David)"
#define Dnd_X_Precision 2
#define Dnd_Y_Precision 2

  // Do we start a DND ?
  if ( abs( _mouse->pos().x() - m_pressedPos.x() ) > Dnd_X_Precision ||
       abs( _mouse->pos().y() - m_pressedPos.y() ) > Dnd_Y_Precision )
  {
    // Collect all selected items
    QList<KIconContainerItem> lst;
    iterator it = begin();
    for( ; *it; ++it )
      if ( (*it)->isSelected() )
	lst.append( *it );
    
    // Do not handle and more mouse move or mouse release events
    m_pressed = false;

    // kdebug( KDEBUG_INFO, 1205, "Starting drag");
    
    QPoint hotspot;
    QPixmap pix;
    // Multiple icons ?
    if ( lst.count() > 1 )
    {
      pix.load( locate("icon", "kmultiple.xpm") );
      hotspot.setX( pix.width() / 2 );
      hotspot.setY( pix.height() / 2 );
    }
    else
    {
      pix = QPixmap( m_pressedItem->width(), m_pressedItem->height() );
      pix.fill();
      kdebug( KDEBUG_INFO, 1205, "==== w=%i h=%i ======", m_pressedItem->width(), m_pressedItem->height() );
      QPainter painter;
      painter.begin( &pix );
      m_pressedItem->paint( &painter, true );
      painter.end();
      pix.setMask( pix.createHeuristicMask() );

      // This is wrong I think, because of the way it looks when trying it ! (David)
      hotspot.setX( _mouse->pos().x() - ( m_pressedItem->x() - contentsX() ) );
      hotspot.setY( _mouse->pos().y() - ( m_pressedItem->y() - contentsY() ) );
      // Not good either... :(
      //hotspot.setX( m_pressedItem->pixmap().width() / 2 );
      //hotspot.setY( m_pressedItem->pixmap().height() / 2 );
      //pix = m_pressedItem->pixmap();
    }

    KIconDrag* drag = new KIconDrag( viewport() );
    
    // Pack the icons in the drag
    QListIterator<KIconContainerItem> icit( lst );
    for( ; *icit; ++icit )
    {
      drag->append( KIconDrag::Icon( (*icit)->name(), (*icit)->position() - _mouse->pos() ) );
    }
    
    drag->setPixmap( pix, hotspot );
    drag->drag();
  }
}

void KIconContainer::drawBackground( QPainter *_painter, int _xval, int _yval, int _x, int _y, int _w, int _h )
{
  if ( m_bgPixmap.isNull() )
  {
    _painter->eraseRect( _x, _y, _w, _h );
    return;
  }
  
  // if the background pixmap is transparent we must erase the bg
  // if ( m_bgPixmap.mask() )
  // _painter->eraseRect( _x, _y, _w, _h );

  int pw = m_bgPixmap.width();
  int ph = m_bgPixmap.height();

  int xOrigin = (_x/pw)*pw - _xval;
  int yOrigin = (_y/ph)*ph - _yval;

  int rx = _x%pw;
  int ry = _y%ph;
  
  for ( int yp = yOrigin; yp - yOrigin < _h + ry; yp += ph )
  {
    for ( int xp = xOrigin; xp - xOrigin < _w + rx; xp += pw )
    {
      _painter->drawPixmap( xp, yp, m_bgPixmap );
    }
  }
}

void KIconContainer::drawContentsOffset( QPainter* _painter, int _offsetx, int _offsety,
					 int _clipx, int _clipy, int _clipw, int _cliph )
{
  if ( !_painter )
    return;
  
  QColorGroup cgr = colorGroup();
  // int maxX = _clipx + _clipw;
  // int maxY = _clipy + _cliph;
  QRect clip( _clipx, _clipy, _clipw, _cliph );

  //kdebug( KDEBUG_INFO, 1205, 
  //      "!!!!!!!!!! DRAW !!!!!!!!!!!! x=%d y=%d w=%d h=%d ox=%d oy=%d )",
  //      _clipx, _clipy, _clipw, _cliph, _offsetx, _offsety);
  
  if ( m_bDirty )
  {
    // kdebug( KDEBUG_INFO, 1205, "!!!!!!!!!! DRAW SETUP !!!!!!!!!!!!");
    setup();
  }
  
  if ( m_mouseMode == SingleClick && m_underlineLink )
  {
    QFont f = _painter->font();
    f.setUnderline( true );
    _painter->setFont( f );
  }

  drawBackground( _painter, _offsetx, _offsety, _clipx, _clipy, _clipw, _cliph );
  
  // Repaint all icons which intersect the clipping area.
  iterator it = begin();
  for( ; *it; ++it )
  {
    if ( (*it)->isVisible() )
    {
      QRect icon( (*it)->x(), (*it)->y(), (*it)->height(), m_iItemWidth );
      if ( clip.intersects( icon ) )
      {
	int diffX = (*it)->x() - _offsetx;
	int diffY = (*it)->y() - _offsety;
	
	_painter->save();
	_painter->translate( diffX, diffY );
	(*it)->paint( _painter );
	_painter->restore();
      }
    }
  }

  //paint focus of selected item
  KIconContainerItem* item = currentItem();
  if ( item )
  {
    int ih = item->height();

    _painter->translate( item->x() - _offsetx, item->y() - _offsety );
    QRect r( 0, 0, m_iItemWidth, ih );
    item->paintFocus( _painter, cgr, r );
  }
    
  // kdebug( KDEBUG_INFO, 1205, "!!!!!!!!!! DRAW END !!!!!!!!!!!!");
}

void KIconContainer::setup()
{
  if ( m_displayMode == Horizontal )
  {    
    // TODO: That is slow
    // m_lstItems.sort();
    // How many items can we put in one line ?
    int cols = viewport()->width() / ( m_iItemWidth + m_iColumnSpacing );
    if ( cols == 0 )
      // ... at least one ...
      cols = 1;

    // Offset from the top of the page
    int h = m_iHeadSpacing;
    int lineheight = 0;
    int col = 0;
    
    iterator it = begin();
    for( ; *it && !(*it)->hasFixedPos(); ++it )
    {
      if ( col == cols )
      {
	col = 0;
	h += lineheight + m_iLineSpacing;
	lineheight = 0;
      }
      
      (*it)->setPos( col * ( m_iItemWidth + m_iColumnSpacing ), h );
      int ih = (*it)->height();
      if ( ih > lineheight )
	lineheight = ih;
      col++;
    }

    // HACK: TODO Fixed pos

    // Offset from the bottom
    h += lineheight + m_iFootSpacing;

    // Resize the view
    resizeContents( viewport()->width(), h );
  }
  else if ( m_displayMode == Vertical )
  {    
    // How many rows are needed in total ?
    int vcount = 0;
    iterator it = begin();
    for( ; *it && !(*it)->hasFixedPos(); ++it )
    {
      // How many rows does the item need ?
      int ih = (*it)->height();
      int mult = ih / ( m_iMinItemHeight + m_iLineSpacing );
      if ( mult == 0 )
	vcount++;
      else if ( ih % ( m_iMinItemHeight + m_iLineSpacing ) == 0 )
	vcount += mult;
      else
	vcount += mult + 1;
    }
    
    // TODO: slow ....
    // m_lstItems.sort();
    // How many columns can we have
    int cols = viewport()->width() / ( m_iItemWidth + m_iColumnSpacing );
    if ( cols == 0 )
      cols = 1;
    // So we know now how many rows each column will have
    int rows = vcount / cols;
    if ( vcount % cols != 0 )
      rows++;
    
    int h = m_iHeadSpacing;
    int maxh = 0;
    
    int col = 0;
    int row = 0;
    it = begin();
    for( ; *it && !(*it)->hasFixedPos(); ++it )
    {
      // Next column ?
      if ( row >= rows )
      {
	row = 0;
	col++;
	// Obeye offset from top and bottom
	h += m_iFootSpacing;
	if ( h > maxh )
	  maxh = h;
	h = m_iHeadSpacing;
      }
      
      // Position the icon
      (*it)->setPos( col * ( m_iItemWidth + m_iColumnSpacing ),
		    m_iHeadSpacing + row * ( m_iMinItemHeight + m_iLineSpacing ) );
      int ih = (*it)->height();
      int mult = ih / ( m_iMinItemHeight + m_iLineSpacing );
      if ( mult == 0 )
	mult = 1;
      else if ( ih % ( m_iMinItemHeight + m_iLineSpacing ) != 0 )
	mult++;

      h += mult * ( m_iMinItemHeight + m_iLineSpacing );
      row += mult;
    }

    // HACK: TODO Fixed pos

    // Resize the contents so that all icons fit in.
    if ( h > maxh )
      maxh = h;
    resizeContents( viewport()->width(), maxh );
  }
  else if ( m_displayMode == SmallVertical )
  {    
    // How many rows are needed in total ?
    int vcount = 0;
    iterator it = begin();
    for( ; *it && !(*it)->hasFixedPos(); ++it )
    {
      // How many rows does the item need ?
      int ih = (*it)->height();
      int mult = ih / ( m_iMinItemHeight + m_iLineSpacing );
      if ( mult == 0 )
	vcount++;
      else if ( ih % ( m_iMinItemHeight + m_iLineSpacing ) == 0 )
	vcount += mult;
      else
	vcount += mult + 1;
    }
    
    // TODO: slow ....
    // m_lstItems.sort();
    // How many columns can we have
    int cols = viewport()->width() / ( m_iItemWidth + m_iColumnSpacing );
    if ( cols == 0 )
      cols = 1;
    // So we know now how many rows each column will have
    int rows = vcount / cols;
    if ( vcount % cols != 0 )
      rows++;
    
    int h = m_iHeadSpacing;
    int maxh = 0;
    
    int col = 0;
    int row = 0;
    it = begin();
    for( ; *it && !(*it)->hasFixedPos(); ++it )
    {
      // Next column ?
      if ( row >= rows )
      {
	row = 0;
	col++;
	// Obeye offset from top and bottom
	h += m_iFootSpacing;
	if ( h > maxh )
	  maxh = h;
	h = m_iHeadSpacing;
      }
      
      // Position the icon
      (*it)->setPos( col * ( m_iItemWidth + m_iColumnSpacing ),
		    m_iHeadSpacing + row * ( m_iMinItemHeight + m_iLineSpacing ) );
      int ih = (*it)->height();
      int mult = ih / ( m_iMinItemHeight + m_iLineSpacing );
      if ( mult == 0 )
	mult = 1;
      else if ( ih % ( m_iMinItemHeight + m_iLineSpacing ) != 0 )
	mult++;

      h += mult * ( m_iMinItemHeight + m_iLineSpacing );
      row += mult;
    }

    // HACK: TODO Fixed pos

    // Resize the contents so that all icons fit in.
    if ( h > maxh )
      maxh = h;
    resizeContents( viewport()->width(), maxh );
  }
  else
    assert( 0 );
  
  m_bDirty = false;
}

void KIconContainer::repaintItem( KIconContainerItem* _item, bool _erase )
{
  // Defensive programming ...
  if ( !_item )
    return;

  QPainter painter;
  painter.begin( viewport() );
  QRect r( _item->x() - contentsX(), _item->y() - contentsY(), m_iItemWidth, _item->height() );
  painter.setClipRect( r );
  if ( _erase )
    drawBackground( &painter, contentsX(), contentsY(), _item->x(), _item->y(), m_iItemWidth, _item->height() );
  painter.translate( _item->x() - contentsX(), _item->y() - contentsY() );
  painter.save();
  _item->paint( &painter );
  painter.restore();

  // Does the icon have the focus ?
  if ( _item == m_currentItem )
  {
    QRect r( 0, 0, m_iItemWidth, _item->height() );
    _item->paintFocus( &painter, colorGroup(), r );
  }

  painter.end();
}

void KIconContainer::resizeEvent( QResizeEvent *_ev )
{
  // Tell papa.
  QScrollView::resizeEvent( _ev );

  // We need a new layout now.
  refreshAll();
}

void KIconContainer::setBgColor( const QColor& _color, bool _refresh )
{ 
  m_bgColor = _color;

  if ( _refresh )
    refresh();
}

void KIconContainer::setTextColor( const QColor& _color, bool _refresh )
{
  m_textColor = _color;
  
  if ( _refresh )
    refresh();
}

void KIconContainer::setLinkColor( const QColor& _color, bool _refresh )
{
  m_linkColor = _color;
  
  if ( _refresh )
    refresh();
}

void KIconContainer::setVLinkColor( const QColor& _color, bool _refresh )
{
  m_vLinkColor = _color;

  if ( _refresh )
    refresh();
}
  
void KIconContainer::setStdFontName( const QString& _name, bool _refresh )
{
  m_stdFontName = _name;

  if ( _refresh )
    refresh();
}

void KIconContainer::setFixedFontName( const QString& _name, bool _refresh )
{
  m_fixedFontName = _name;

  if ( _refresh )
    refresh();
}

void KIconContainer::setFontSize( const int _size, bool _refresh )
{
  m_fontSize = _size;

  if ( _refresh )
    refresh();
}
  
void KIconContainer::setUnderlineLink( bool _underlineLink, bool _refresh )
{
  m_underlineLink = _underlineLink;

  if ( _refresh )
    refresh();
}

void KIconContainer::setShowingDotFiles( bool _isShowingDotFiles, bool _refresh )
{
  m_isShowingDotFiles = _isShowingDotFiles;

  if ( _refresh )
    refresh();
}

void KIconContainer::setBgPixmap( const QPixmap& _pixmap, bool _refresh )
{
  m_bgPixmap = _pixmap;

  if ( m_bgPixmap.isNull() )
    viewport()->setBackgroundMode( PaletteBackground );
  else
    viewport()->setBackgroundMode( NoBackground );
  
  if ( _refresh )
    update();
}

KIconContainerItem* KIconContainer::itemAt( const QPoint& _point )
{
  int x = _point.x() + contentsX();
  int y = _point.y() + contentsY();
  
  iterator it = begin();
  for( ; *it; ++it )
    if ( (*it)->contains( x, y ) )
      return *it;

  return 0;
}

void KIconContainer::drawDragShadow( const QPoint& _offset )
{
  if ( m_dragIcons.count() < 2 )
    return;

  m_dragIconsOffset = _offset;

  QPainter painter;
  painter.begin( viewport() );

  painter.setRasterOp( Qt::XorROP );
  KIconDrag::IconList::Iterator it = m_dragIcons.begin();
  while( it != m_dragIcons.end() )
  {
    painter.drawRect( _offset.x() + (*it).pos.x(), _offset.y() + (*it).pos.y(), m_iMinItemWidth, m_iMinItemHeight );
    it++;
  }

  painter.end();
}

void KIconContainer::clearDragShadow()
{
  drawDragShadow( m_dragIconsOffset );
}

/*******************************************************
 *
 * KIconContainerItem
 *
 *******************************************************/

KIconContainerItem::KIconContainerItem( KIconContainer* _container, const QPixmap& _pixmap,
                                        const QString& _text, const QString &_name ) :
  m_pContainer ( _container ),
  m_pixmap ( _pixmap ),
  m_strText ( _text ),
  m_strName ( _name ),
  m_bFixedPos ( false ),
  m_flags ( NoFlags )
{
  breakText();
}

KIconContainerItem::KIconContainerItem( KIconContainer* _container ) :
  m_pContainer ( _container ),
  m_strText ( "" ),
  m_strBrokenText ( "" ),
  m_strName ( "" ),
  m_bFixedPos ( false ),
  m_flags ( NoFlags )
{
}

void KIconContainerItem::refresh( bool /* _display_mode_changed */ )
{
  breakText();
}

void KIconContainerItem::setText( const QString& _text )
{
  m_strText = _text;
  
  breakText();
}

void KIconContainerItem::breakText()
{
  // QPainter painter;
  // assert( painter.begin( m_pContainer ) );
  // breakText( &painter );
  // painter.end();

  QPainter painter;
  assert( painter.begin( m_pContainer->m_pDummy ) );
  breakText( &painter );
  painter.end();
}

void KIconContainerItem::breakText( QPainter *_painter )
{
  // This is complicated. Dont even try to understand it ...
  char * str = new char [ m_strText.length() + 1 ];
  
  int maxlen;
  if ( m_pContainer->displayMode() == KIconContainer::SmallVertical )
    maxlen = m_pContainer->itemWidth() - s_iPixmapTextHSpacing - m_pContainer->pixmapSize().width();
  else if ( m_pContainer->displayMode() == KIconContainer::Horizontal ||
	    m_pContainer->displayMode() == KIconContainer::Vertical )
    maxlen = m_pContainer->itemWidth();
  else
    assert( 0 );

  short j = 0, width = 0, charWidth = 0;
  char* pos = 0L;
  char* sepPos = 0L;
  char* part = 0L;
  char c = 0;
        
  if ( m_pContainer->fontMetrics().width( m_strText ) <= maxlen )
    m_strBrokenText = m_strText;
  else
  {    
    strcpy( str, m_strText );  
    m_strBrokenText = "";
  
    for ( width=0, part=str, pos=str; *pos; pos++ )
    {
      charWidth = m_pContainer->fontMetrics().width( *pos );
      if ( width + charWidth >= maxlen )
      {
	// search for a suitable separator in the previous 8 characters
	for ( sepPos=pos, j=0; j<8 && sepPos>part; j++, sepPos--)
        {
	  if (ispunct(*sepPos) || isdigit(*sepPos))
	  {
	    pos = sepPos;
	    break;
	  }
	  if (isupper(*sepPos) && !isupper(*(sepPos-1)))
	  {
	    pos = sepPos;
	    break;
	  }               
	}
	
	c = *pos;
	*pos = '\0';
	m_strBrokenText += part;
	m_strBrokenText += "\n";
	*pos = c;
	part = pos;
	width = 0;
      }
      width += charWidth;
    }
    if (*part)
      m_strBrokenText += part;         
  }
  
  assert ( _painter != 0L );
  
  if ( m_pContainer->displayMode() == KIconContainer::Horizontal ||
       m_pContainer->displayMode() == KIconContainer::Vertical )
    m_textBoundingRect = _painter->boundingRect( 0, 0, m_pContainer->itemWidth(), 0xFFFF,
						 Qt::AlignTop | Qt::AlignHCenter | Qt::WordBreak,
						 m_strBrokenText );
  else if ( m_pContainer->displayMode() == KIconContainer::SmallVertical )
  {
    m_textBoundingRect = _painter->boundingRect( 0, 0, m_pContainer->itemWidth()
						 - s_iPixmapTextHSpacing -
						 m_pContainer->pixmapSize().width(), 0xFFFF,
						 Qt::AlignTop | Qt::AlignLeft | Qt::WordBreak,
						 m_strBrokenText );
  }
  else
    assert( 0 );
  delete str;
}

void KIconContainerItem::setFixedPos( const QPoint& _pos, bool _refresh )
{
  if ( _refresh )
  {
    setVisible( false );
    m_pContainer->wipeOutItem( this );
  }

  m_x = _pos.x() + m_pContainer->contentsX();
  m_y = _pos.y() + m_pContainer->contentsY();
  m_bFixedPos = true;

  if ( _refresh )
  {
    setVisible( true );
    m_pContainer->repaintItem( this, false );
  }
}

int KIconContainerItem::width() const
{
  return m_pContainer->itemWidth();

  if ( m_pContainer->displayMode() == KIconContainer::Horizontal ||
       m_pContainer->displayMode() == KIconContainer::Vertical )
  {
    int w = m_pixmap.width();
    if ( m_textBoundingRect.width() > w )
      w = m_textBoundingRect.width();
    return w;
  }
  else if ( m_pContainer->displayMode() == KIconContainer::SmallVertical )
  {
    return m_pixmap.width() + s_iPixmapTextHSpacing + m_textBoundingRect.width();
  }
  else
    assert( 0 );
}

int KIconContainerItem::height() const
{
  if ( m_pContainer->displayMode() == KIconContainer::Horizontal ||
       m_pContainer->displayMode() == KIconContainer::Vertical )
  {
    return m_pixmap.height() + KIconContainer::m_iPixTextSpace + m_textBoundingRect.height();
  }
  else if ( m_pContainer->displayMode() == KIconContainer::SmallVertical )
  {
    int h = m_pixmap.height();
    if ( m_textBoundingRect.height() > h )
      return m_textBoundingRect.height();
    return h;
  }
  else
    assert( 0 );
}

bool KIconContainerItem::isEqual( const KIconContainerItem& _item )
{
  return ( _item.text() == m_strText );
}

bool KIconContainerItem::isSmallerThen( const KIconContainerItem& _item )
{
  // Special handling for fixed pos items
  if ( hasFixedPos() && !_item.hasFixedPos() )
    return true;
  
  if ( strcmp( text(), _item.text() ) < 0 )
    return true;
  return false;
}

void KIconContainerItem::paint( QPainter* _painter, bool _drag )
{ 
  int w = m_pContainer->itemWidth();
  if ( _drag )
    w = width();

  if ( m_pContainer->displayMode() == KIconContainer::Horizontal ||
       m_pContainer->displayMode() == KIconContainer::Vertical )
  {
    if ( isSelected() && !_drag )
    {
      QPixmap pix( m_pixmap );
      QPainter painter;
      painter.begin( &pix );
      QBrush b( QApplication::palette().normal().highlight(), Qt::Dense4Pattern );
      painter.fillRect( 0, 0, pix.width(), pix.height(), b );
      painter.end();
      _painter->drawPixmap( ( w - m_pixmap.width() ) / 2, 0, pix );
    }
    else
      _painter->drawPixmap( ( w - m_pixmap.width() ) / 2, 0, m_pixmap );

    // Some pixels space between text and pixmap
    int h = KIconContainer::m_iPixTextSpace + m_pixmap.height();
 
    _painter->setPen( color() );
    _painter->drawText( ( w - m_textBoundingRect.width() ) / 2, h,
			m_textBoundingRect.width(), /* m_textBoundingRect.height() */ 0xFFFF, 
			Qt::DontClip | Qt::AlignTop | Qt::AlignHCenter | Qt::WordBreak,
			m_strBrokenText );  
  }
  else if ( m_pContainer->displayMode() == KIconContainer::SmallVertical )
  {
    if ( isSelected() && !_drag )
    {
      _painter->fillRect( 0, 0, w, height(), QApplication::winStyleHighlightColor() );
      _painter->setPen( Qt::white ); // A hack copied from Arnt :-) If even he does not know better ...
    }
    /* else
      _painter->eraseRect( 0, 0, w, height() ); */

    _painter->drawPixmap( 0, 0, m_pixmap );
    
    _painter->setPen( color() );
    _painter->drawText( m_pContainer->pixmapSize().width() + s_iPixmapTextHSpacing, 0,
			m_textBoundingRect.width(), /* m_textBoundingRect.height() */ 0xFFFF, 
			Qt::DontClip | Qt::AlignTop | Qt::AlignLeft | Qt::WordBreak,
			m_strBrokenText );  
  }
  else
    assert( 0 );  
}

void KIconContainerItem::paintFocus( QPainter *_painter, const QColorGroup &, const QRect& _rect )
{
  _painter->drawWinFocusRect( _rect );
}

bool KIconContainerItem::contains( int _x, int _y )
{ 
  if ( m_pContainer->displayMode() == KIconContainer::Horizontal ||
       m_pContainer->displayMode() == KIconContainer::Vertical )
  {      
    int pixoffset = ( m_pContainer->itemWidth() - m_pixmap.width() ) / 2;
    
    if ( _x >= m_x + pixoffset && _x < m_x + m_pContainer->itemWidth() - pixoffset &&
	 _y >= m_y && _y < m_y + m_pixmap.height() )
      return true;

    int h = m_pixmap.height() + 3;
    int textoffset = ( m_pContainer->itemWidth() - m_textBoundingRect.width() ) / 2;
    
    if ( _x >= m_x + textoffset && _x < m_x + m_pContainer->itemWidth() - textoffset &&
	 _y >= m_y + h && _y < m_y + height() )
      return true;

    return false;
  }
  else if ( m_pContainer->displayMode() == KIconContainer::SmallVertical )
  {    
    if ( _x >= m_x && _x < m_x + m_pixmap.width() &&
	 _y >= m_y && _y < m_y + m_pixmap.height() )
      return true;

    int textoffset = m_pContainer->pixmapSize().width() + s_iPixmapTextHSpacing;
    int textwidth = m_textBoundingRect.width();
    
    if ( _x >= m_x + textoffset && _x < m_x + textoffset + textwidth &&
	 _y >= m_y && _y < m_y + m_textBoundingRect.height() )
      return true;

    return false;
  }
  else
    assert( 0 );
}

QColor KIconContainerItem::color()
{
  return m_pContainer->linkColor();
}

void KIconContainerItem::setSelected( bool _selected, bool _refresh )
{
  if ( _selected == isSelected() )
    return;
  m_flags = (Flags) (( m_flags & Invisible ) | ( _selected ? Selected : NoFlags ));

  if ( _refresh )
    m_pContainer->setSelected( this, _selected );
}

void KIconContainerItem::setVisible( bool _visible )
{
  m_flags = (Flags) (( m_flags & Selected ) | ( _visible ? NoFlags : Invisible ));
}

#include "kiconcontainer.moc"
