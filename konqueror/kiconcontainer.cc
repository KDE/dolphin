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

#include <qpainter.h>
#include <qkeycode.h>
#include <kapp.h>
#include <kiconloader.h>
#include <qdragobject.h>
#include <kcursor.h>

#include <assert.h>
#include <string.h>

/*******************************************************
 *
 * KIconContainer
 *
 *******************************************************/

int KIconContainer::s_id = 0;

KIconContainer::KIconContainer( QWidget *_parent, const char *_name, WFlags _flags ) :
  QScrollView( _parent, _name, _flags ), QDropSite( this )
{
  m_pDummy         = new QWidget( 0L );

  m_bDirty         = true;
  m_iMinItemWidth  = 90;
  m_pixmapSize     = QSize( 16, 16 );
  m_iMinItemHeight = fontMetrics().lineSpacing();
  m_displayMode    = Horizontal;
  m_currentItemId  = -1;

  m_stdCursor      = KCursor().arrowCursor();
  m_handCursor     = KCursor().handCursor();

  m_dragOverItem   = end();
  m_pressed        = false;
  m_onItem         = false;
  m_overItem       = "";

  int x = viewport()->width() / m_iMinItemWidth;
  if ( x != 0 )
    m_iItemWidth = viewport()->width() / x;
  else
    m_iItemWidth = m_iMinItemWidth;
  if ( m_pixmapSize.height() > m_iMinItemHeight )
    m_iMinItemHeight = m_pixmapSize.height();

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

  return QScrollView::eventFilter( o, e );
}

void KIconContainer::clear()
{
  iterator it = begin();
  for( ; it != end(); it++ )
    it->free();

  m_lstIcons.clear();
}

void KIconContainer::setDisplayMode( DisplayMode _m )
{
  if ( m_displayMode == _m )
    return;

  m_displayMode = _m;

  if ( m_displayMode == Horizontal )
    m_iMinItemWidth = 90;
  else
    m_iMinItemWidth = 180;

  refreshAll();
}

void KIconContainer::refreshAll()
{
  if ( m_displayMode == Vertical )
  {
    m_iMinItemHeight = fontMetrics().lineSpacing();
    if ( m_pixmapSize.height() > m_iMinItemHeight )
      m_iMinItemHeight = m_pixmapSize.height();
  }
  else if ( m_displayMode == Horizontal )
    m_iMinItemHeight = fontMetrics().lineSpacing();
  else
    assert( 0 );

  int x = viewport()->width() / m_iMinItemWidth;
  if ( x != 0 )
    m_iItemWidth = viewport()->width() / x - m_iColumnSpacing;
  else
    m_iItemWidth = m_iMinItemWidth;

  iterator it = begin();
  for( ; it != end(); it++ )
    (*it)->refresh();

  refresh();
}

void KIconContainer::insert( QPixmap& _pixmap, const char *_txt, int _x, int _y, bool _refresh )
{
  KIconContainerItem *item = new KIconContainerItem( this, _pixmap, _txt );
  m_lstIcons.push_back( item );

  if ( _x != -1 && _y != -1 )
    item->setFixedPos( _x, _y );

  if ( _refresh )
    refresh();
}

void KIconContainer::insert( KIconContainerItem* _item, int _x, int _y, bool _refresh )
{
  m_lstIcons.push_back( _item );

  if ( _x != -1 && _y != -1 )
    _item->setFixedPos( _x, _y );

  if ( _refresh )
    refresh();
}

void KIconContainer::erase( iterator _it, bool _refresh )
{
  if ( _it->id() == m_currentItemId )
    m_currentItemId = -1;

  if ( !_refresh )
  {
    QPainter painter;
    assert( painter.begin( viewport() ) );
    if ( m_bgPixmap.isNull() )
    {
      painter.translate( (*_it)->x() - contentsX(), (*_it)->y() - contentsY() );
      painter.eraseRect( 0, 0, m_iItemWidth, (*_it)->height() );
    }
    else
    {
      QRect r( (*_it)->x() - contentsX(), (*_it)->y() - contentsY(), m_iItemWidth, (*_it)->height() );
      painter.setClipRect( r );
      drawBackground( &painter, contentsX(), contentsY(), (*_it)->x(), (*_it)->y(), m_iItemWidth, (*_it)->height() );
    }

    painter.end();
  }

  _it->free();
  m_lstIcons.erase( _it );

  if ( _refresh )
    refresh();
}

void KIconContainer::remove( KIconContainerItem* _item, bool _refresh )
{
  iterator it = begin();
  bool match = false;
  for( ; it != end() && !match; ++it )
    if ( it->item() == _item )
    {
      erase( it, _refresh );
      return;
    }
}

KIconContainerItem* KIconContainer::currentItem()
{
  iterator it = idToIterator( m_currentItemId );
  if ( it == end() )
    return 0L;

  return it->item();
}

void KIconContainer::setCurrentItem( KIconContainerItem *_item )
{
  iterator it = itemToIterator( _item );

  setCurrentItem( it );
}

void KIconContainer::setCurrentItem( iterator _item )
{
  iterator old = idToIterator( m_currentItemId );
  m_currentItemId = _item->id();

  if ( old != end() )
    repaintItem( old );

  if ( _item != end() )
  {
    repaintItem( _item );
    int dx = 0;
    int dy = 0;
    int x = (*_item)->x();
    int y = (*_item)->y();
    int ih = (*_item)->height();
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

    emit currentChanged( &**_item );
    emit onItem( &**_item );
  }

  emit currentChanged( 0L );
}

void KIconContainer::unselectAll()
{
  iterator it = begin();
  for( ; it != end(); ++it )
    if ( (*it)->isSelected() )
      setSelected( &**it, false );
}

void KIconContainer::setSelected( KIconContainerItem* _item, bool _selected )
{
  _item->setSelected( _selected );
  iterator it = itemToIterator( _item );
  repaintItem( it );

  emit selectionChanged();
  emit selectionChanged( _item );
}

KIconContainer::iterator KIconContainer::idToIterator( int _id )
{
  iterator it = begin();
  for( ; it != end(); ++it )
    if ( it->id() == _id )
      return it;

  return it;
}

KIconContainer::iterator KIconContainer::itemToIterator( KIconContainerItem* _item )
{
  iterator it = begin();
  for( ; it != end(); ++it )
    if ( it->item() == _item )
      return it;

  return it;
}

void KIconContainer::keyPressEvent( QKeyEvent *_ev )
{
  switch( _ev->key() )
    {
    case Key_Return:
      _ev->accept();
      {
	iterator it = idToIterator( m_currentItemId );
	if ( it == end() )
	  return;
	QPoint p( (*it)->x() - contentsX(), (*it)->y() - contentsY() );
	p = mapToGlobal( p );
	emit returnPressed( &**it, p );
	return;
      }
    case Key_Down:
      _ev->accept();
      if ( m_displayMode == Horizontal )
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
      else if ( m_displayMode == Vertical )
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
	assert( 0 );
    case Key_Up:
      _ev->accept();
      if ( m_displayMode == Horizontal )
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
      else if ( m_displayMode == Vertical )
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
	assert( 0 );
    case Key_Left:
      _ev->accept();
      if ( m_displayMode == Horizontal )
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
      else if ( m_displayMode == Vertical )
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
	assert( 0 );
    case Key_Right:
      _ev->accept();
      if ( m_displayMode == Horizontal )
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
      else if ( m_displayMode == Vertical )
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
	assert( 0 );
    case Key_Space:
      _ev->accept();
      {
	iterator it = idToIterator( m_currentItemId );
	if ( it == end() )
	  return;

	setSelected( &**it, !(*it)->isSelected() );
	return;
      }
    }
}

void KIconContainer::dragMoveEvent( QDragMoveEvent *_ev )
{
  int x = _ev->pos().x() + contentsX();
  int y = _ev->pos().y() + contentsY();

  // Find the item
  iterator it = itemAt( x, y );
  if ( it != end() )
  {
    if ( m_dragOverItem == it )
      return;
    if ( m_dragOverItem != end() )
      setSelected( &**m_dragOverItem, false );

    if ( (*it)->acceptsDrops( m_lstDropFormats ) )
    {
      _ev->accept();
      setSelected( &**it, true );
      m_dragOverItem = it;
    }
    else
      {
	_ev->ignore();
	m_dragOverItem = end();
      }
    return;
  }

  if ( m_dragOverItem != end() )
    setSelected( &**m_dragOverItem, false );
  m_dragOverItem = end();
  _ev->accept();
}

void KIconContainer::dragEnterEvent( QDragEnterEvent *_ev )
{
  m_dragOverItem = end();

  // Save the available formats
  m_lstDropFormats.clear();

  for( int i = 0; _ev->format( i ); i++ )
  {
    if ( *( _ev->format( i ) ) )
      m_lstDropFormats.append( _ev->format( i ) );
  }

 kdebug(0,1202,"GOT DRAG");

  // By default we accept any format
  _ev->accept();
}

void KIconContainer::dragLeaveEvent( QDragLeaveEvent * )
{
  if ( m_dragOverItem != end() )
    setSelected( &**m_dragOverItem, false );
  m_dragOverItem = end();

  /** DEBUG CODE */
  // Give the user some feedback...
 kdebug(0,1202,"Left!");
  /** End DEBUG CODE */
}

void KIconContainer::dropEvent( QDropEvent * e )
{
 kdebug(0,1202,"YappaDappaDu");

  if ( m_dragOverItem != end() )
    setSelected( &**m_dragOverItem, false );
  m_dragOverItem = end();


  int x = e->pos().x() + contentsX();
  int y = e->pos().y() + contentsY();

  iterator it = itemAt( x, y );

  // Background drop
  if ( it == end() )
  {
   kdebug(0,1202,"Background drop");
    emit drop( e, 0L, m_lstDropFormats );
  }
  else
  {
   kdebug(0,1202,"Item drop");
    emit drop( e, &**it, m_lstDropFormats );
  }

    /*

  // Try to decode to the data you understand...
  if ( QUrlDrag::decode( e, m_lstDropURLs ) )
  {
    if( m_lstDropURLs.count() == 0 )
      return;

    m_dropFilePos = e->pos();

    m_pPopupMenu->clear();

    char *s;
    for ( s = m_lstDropURLs.first(); s != 0L; s = m_lstDropURLs.next() )
    {
     kdebug(0,1202,"Testing URL " << s);

      KURL u( s );
      if ( u.isMalformed() )
      {
	QString tmp;
	ksprintf( &tmp, i18n( "The URL\n%s\nis malformed" ), s );
	QMessageBox::critical( 0L, i18n("Error"), tmp, i18n("OK") );
	return;
      }
    }

    if ( m_dndStartPos.x() != -1 && m_dndStartPos.y() != -1 )
    {
      moveIcons( m_lstDropURLs, m_dropFilePos );
      return;
    }

    m_pPopupMenu->insertItem( klocale->getAlias( ID_STRING_COPY ), this, SLOT( slotDropCopy() ) );
    m_pPopupMenu->insertItem( klocale->getAlias( ID_STRING_MOVE ), this, SLOT( slotDropMove() ) );
    m_pPopupMenu->insertItem( klocale->getAlias( ID_STRING_LINK ), this, SLOT( slotDropLink() ) );

    m_pPopupMenu->popup( m_dropFilePos );
  } */
}

void KIconContainer::viewportMousePressEvent( QMouseEvent *_ev )
{
  // kdebug(0,1202,"void KIconContainer::viewportMousePressEvent( QMouseEvent *_ev )");

  if ( !hasFocus() )
    setFocus();

  m_pressed = false;

  int x = _ev->pos().x();
  int y = _ev->pos().y();
  x += contentsX();
  y += contentsY();

  if ( m_mouseMode == SingleClick )
  {
    iterator it = itemAt( x, y );
    if ( it != end() )
    {
      if ( !m_overItem.isEmpty() )
      {
	//reset to the standart cursor
	setCursor(m_stdCursor);
	m_overItem = "";
      }

      if ( ( _ev->state() & ControlButton ) && _ev->button() == LeftButton )
      {
	setSelected( &**it, !(*it)->isSelected() );
	
	return;
      }
      else if ( _ev->button() == RightButton && (*it)->isSelected() == false )
      {
	unselectAll();
	setSelected( &**it, true );
      }
      else if ( _ev->button() == LeftButton || _ev->button() == MidButton )
      {
	if ( (*it)->isSelected() == false )
        {
	  unselectAll();
	  setSelected( &**it, true );
	}
	
	// This may be the beginning of DND
	m_pressed = true;
	m_pressedPos = _ev->pos();
	m_pressedItem = it;
	return;
      }

      QPoint p = mapToGlobal( _ev->pos() );
      emit mousePressed( &**it, p, _ev->button() );
      return;
    }

    // Click on the background of the window
    if ( _ev->button() == RightButton )
    {
      QPoint p = mapToGlobal( _ev->pos() );
      emit mousePressed( 0L, p, _ev->button() );
      return;
    }
  }
  else if ( m_mouseMode == DoubleClick )
  {
    iterator it = itemAt( x, y );
    if ( it != end() )
    {
      if ( ( _ev->state() & ControlButton ) && _ev->button() == LeftButton )
      {
	setSelected( &**it, !(*it)->isSelected() );
	return;
      }
      if ( _ev->button() == RightButton && !(*it)->isSelected() )
      {
	unselectAll();
	setSelected( &**it, true );

	QPoint p = mapToGlobal( _ev->pos() );
	emit mousePressed( &**it, p, _ev->button() );
      }
      else if ( _ev->button() == LeftButton || _ev->button() == MidButton )
      {
	unselectAll();
	setSelected( &**it, true );
      }

      return;
    }

    if ( _ev->button() == RightButton )
    {
      QPoint p = mapToGlobal( _ev->pos() );
      emit mousePressed( 0L, p, _ev->button() );
      return;
    }
  }
  else
    assert( 0 );
}

void KIconContainer::viewportMouseDoubleClickEvent( QMouseEvent *_ev )
{
  int x = _ev->pos().x();
  int y = _ev->pos().y();
  x += contentsX();
  y += contentsY();

  iterator it = itemAt( x, y );
  if ( it != end() )
  {
    QPoint p = mapToGlobal( _ev->pos() );
    emit doubleClicked( &**it, p, _ev->button() );
    return;
  }
}

void KIconContainer::viewportMouseReleaseEvent( QMouseEvent *_mouse )
{
  if ( !m_pressed )
    return;
  m_pressed = false;

  if ( _mouse->button() != LeftButton || ( _mouse->state() & ControlButton ) == ControlButton )
    return;

  QPoint p = mapToGlobal( m_pressedPos );

  if ( m_mouseMode == SingleClick )
    emit returnPressed( &**m_pressedItem, p );

  emit mouseReleased( &**m_pressedItem, p, LeftButton );
  return;
}

void KIconContainer::viewportMouseMoveEvent( QMouseEvent *_mouse )
{
  int x = _mouse->pos().x();
  int y = _mouse->pos().y();

  if ( !m_pressed )
  {
    iterator it;

    it = itemAt( x + contentsX(), y + contentsY() );

    if ( it != end() )
    {
      KIconContainerItem* item = it->item();

      if ( m_overItem != item->text() )
      {
	emit onItem( item );
	m_overItem = item->text();

	if ( m_mouseMode == SingleClick )
	  setCursor( m_handCursor );
      }
    }
    else
      if ( !m_overItem.isEmpty() )
      {
	emit onItem( 0L );
	
	setCursor(m_stdCursor);
	m_overItem = "";
      }

    return;
  }

  if ( abs( x - m_pressedPos.x() ) > Dnd_X_Precision || abs( y - m_pressedPos.y() ) > Dnd_Y_Precision )
  {
    // Collect all selected items
    QList<KIconContainerItem> lst;
    iterator it = begin();
    for( ; it != end(); it++ )
      if ( (*it)->isSelected() )
	lst.append( &**it );

    QPoint hotspot;

    // Do not handle and more mouse move or mouse release events
    m_pressed = false;

   kdebug(0,1202,"Starting drag");

    // Multiple URLs ?
    if ( lst.count() > 1 )
    {
      QPixmap pixmap2 = ICON("kmultiple.xpm");

      hotspot.setX( pixmap2.width() / 2 );
      hotspot.setY( pixmap2.height() / 2 );
      emit dragStart( hotspot, lst, pixmap2 );
    }
    else
    {
      hotspot.setX( (*m_pressedItem)->pixmap().width() / 2 );
      hotspot.setY( (*m_pressedItem)->pixmap().height() / 2 );
      emit dragStart( hotspot, lst, (*m_pressedItem)->pixmap() );
    }
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
  KIconContainerItem* item = currentItem();
  int maxX = _clipx + _clipw;
  int maxY = _clipy + _cliph;

  //kdebug(0,1202,"!!!!!!!!!! DRAW !!!!!!!!!!!!" <<  " x=" << _clipx << " y=" << _clipy << " w=" << _clipw << " h=" << _cliph << " ox=" << _offsetx << " oy=" << _offsety);

  if ( m_bDirty )
  {
   kdebug(0,1202,"!!!!!!!!!! DRAW SETUP !!!!!!!!!!!!");
    setup();
  }

  if ( m_mouseMode == SingleClick && m_underlineLink )
  {
    QFont f = _painter->font();
    f.setUnderline( true );
    _painter->setFont( f );
  }

  drawBackground( _painter, _offsetx, _offsety, _clipx, _clipy, _clipw, _cliph );

  iterator it = begin();

  for( ; it != end(); it++ )
  {
    int x = (*it)->x();
    int y = (*it)->y();
    int ih = (*it)->height();
    int iMaxX = x + m_iItemWidth;
    int iMaxY = y + ih;

    if ( iMaxX < _clipx )
    {
      // icon is left from the clipping area
    }
    else if ( iMaxY < _clipy )
    {
      // if we iterating from end then jump out of the loop,
      // because the icon is before clipping area
    }
    else if ( x > maxX )
    {
      // icon is right from the clipping area
    }
    else if ( y > maxY )
    {
      // if we iterating from the beginnig and are in the
      // horizontal mode, then jump out of the loop, because
      // the icon is after the clipping area
      if ( m_displayMode == Horizontal )
	break;
    }
    // check if icons are in the clipping area
    else if ( ( x >= _clipx || iMaxX <= maxX ) &&
	      ( y >= _clipy || iMaxY <= maxY ) ||
	      x <= _clipx && iMaxX >= maxX ||
	      y <= _clipy && iMaxY >= maxY )
    {
      int diffX = x - _offsetx;
      int diffY = y - _offsety;

      _painter->save();
      _painter->translate( diffX, diffY );
      (*it)->paint( _painter, cgr );
      _painter->restore();
    }
  }

  //paint focus of selected item
  if ( item )
  {
    int ih = item->height();

    _painter->translate( item->x() - _offsetx, item->y() - _offsety );
    QRect r( 0, 0, m_iItemWidth, ih );
    item->paintFocus( _painter, cgr, r );
  }

  //kdebug(0,1202,"!!!!!!!!!! DRAW END !!!!!!!!!!!!");
}

void KIconContainer::setup()
{
  if ( m_displayMode == Horizontal )
  {
    m_lstIcons.sort();
    int cols = viewport()->width() / ( m_iItemWidth + m_iColumnSpacing );
    if ( cols == 0 )
      cols = 1;

    int h = m_iHeadSpacing;

    int lineheight = 0;
    int col = 0;

    iterator it = begin();
    for( ; it != end() && !(*it)->hasFixedPos(); ++it )
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

    h += lineheight + m_iFootSpacing;
    resizeContents( viewport()->width(), h );
  }
  else if ( m_displayMode == Vertical )
  {
    int vcount = 0;
    iterator it = begin();
    for( ; it != end() && !(*it)->hasFixedPos(); ++it )
    {
      int ih = (*it)->height();
      int mult = ih / ( m_iMinItemHeight + m_iLineSpacing );
      if ( mult == 0 )
	vcount++;
      else if ( ih % ( m_iMinItemHeight + m_iLineSpacing ) == 0 )
	vcount += mult;
      else
	vcount += mult + 1;
    }

    m_lstIcons.sort();
    int cols = viewport()->width() / ( m_iItemWidth + m_iColumnSpacing );
    if ( cols == 0 )
      cols = 1;
    int rows = vcount / cols;
    if ( vcount % cols != 0 )
      rows++;

    int h = m_iHeadSpacing;
    int maxh = 0;

    int col = 0;
    int row = 0;
    it = begin();
    for( ; it != end() && !(*it)->hasFixedPos(); ++it )
    {
      if ( row >= rows )
      {
	row = 0;
	col++;
	h += m_iFootSpacing;
	if ( h > maxh )
	  maxh = h;
	h = m_iHeadSpacing;
      }

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

    if ( h > maxh )
      maxh = h;

    resizeContents( viewport()->width(), maxh );
  }
  else
    assert( 0 );

  m_bDirty = false;
}

void KIconContainer::repaintItem( iterator _item )
{
  QPainter painter;
  assert( painter.begin( viewport() ) );
  QRect r( (*_item)->x() - contentsX(), (*_item)->y() - contentsY(), m_iItemWidth, (*_item)->height() );
  painter.setClipRect( r );
  drawBackground( &painter, contentsX(), contentsY(), (*_item)->x(), (*_item)->y(), m_iItemWidth, (*_item)->height() );
  painter.translate( (*_item)->x() - contentsX(), (*_item)->y() - contentsY() );
  painter.save();
  (*_item)->paint( &painter, colorGroup() );
  painter.restore();

  if ( _item->id() == m_currentItemId )
  {
    QRect r( 0, 0, m_iItemWidth, (*_item)->height() );
    (*_item)->paintFocus( &painter, colorGroup(), r );
  }

  painter.end();
}

void KIconContainer::resizeEvent( QResizeEvent *_ev )
{
  QScrollView::resizeEvent( _ev );

  refreshAll();
}

void KIconContainer::setBgColor( const QColor& _color )
{
  m_bgColor = _color;

  refresh();
}

void KIconContainer::setTextColor( const QColor& _color )
{
  m_textColor = _color;

  refresh();
}

void KIconContainer::setLinkColor( const QColor& _color )
{
  m_linkColor = _color;

  refresh();
}

void KIconContainer::setVLinkColor( const QColor& _color )
{
  m_vLinkColor = _color;

  refresh();
}

void KIconContainer::setStdFontName( const char *_name )
{
  m_stdFontName = _name;

  refresh();
}

void KIconContainer::setFixedFontName( const char *_name )
{
  m_fixedFontName = _name;

  refresh();
}

void KIconContainer::setFontSize( const int _size )
{
  m_fontSize = _size;

  refresh();
}

void KIconContainer::setUnderlineLink( bool _underlineLink )
{
  m_underlineLink = _underlineLink;

  refresh();
}

void KIconContainer::setShowingDotFiles( bool _isShowingDotFiles )
{
  m_isShowingDotFiles = _isShowingDotFiles;

  refresh();
}

void KIconContainer::setBgPixmap( const QPixmap& _pixmap )
{
  m_bgPixmap = _pixmap;

  if ( m_bgPixmap.isNull() )
    viewport()->setBackgroundMode( PaletteBackground );
  else
    viewport()->setBackgroundMode( NoBackground );

  update();
}

KIconContainer::iterator KIconContainer::itemAt( int _x, int _y )
{
  iterator it = begin();
  for( ; it != end(); it++ )
    if ( (*it)->contains( _x, _y ) )
      break;

  return it;
}

/*******************************************************
 *
 * KIconContainerItem
 *
 *******************************************************/

KIconContainerItem::KIconContainerItem( KIconContainer* _container, QPixmap& _pixmap, const char *_text )
{
  m_pContainer = _container;
  m_bFixedPos = false;
  m_bSelected = false;
  m_pixmap = _pixmap;
  m_strText = _text;

  breakText();
}

KIconContainerItem::KIconContainerItem( KIconContainer* _container )
{
  m_pContainer = _container;
  m_bFixedPos = false;
  m_bSelected = false;
  m_strText = "";
  m_strBrokenText = "";
}

void KIconContainerItem::refresh()
{
  breakText();
}

void KIconContainerItem::setText( const char *_text )
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
  char str[ m_strText.size() + 1 ];

  int maxlen;
  if ( m_pContainer->displayMode() == KIconContainer::Vertical )
    maxlen = m_pContainer->itemWidth() - m_iPixmapTextHSpacing - m_pContainer->pixmapSize().width();
  else if ( m_pContainer->displayMode() == KIconContainer::Horizontal )
    maxlen = m_pContainer->itemWidth();
  else
    assert( 0 );

  short j = 0, width = 0, charWidth = 0;
  char* pos = 0L;
  char* sepPos = 0L;
  char* part = 0L;
  char c = 0;

  if ( m_pContainer->fontMetrics().width( m_strText.c_str() ) <= maxlen )
    m_strBrokenText = m_strText;
  else
  {
    strcpy( str, m_strText.c_str() );
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

  if ( m_pContainer->displayMode() == KIconContainer::Horizontal )
    m_textBoundingRect = _painter->boundingRect( 0, 0, m_pContainer->itemWidth(), 0xFFFF,
					       Qt::AlignTop | Qt::AlignHCenter | Qt::WordBreak, m_strBrokenText.c_str() );
  else if ( m_pContainer->displayMode() == KIconContainer::Vertical )
  {
    m_textBoundingRect = _painter->boundingRect( 0, 0, m_pContainer->itemWidth() - m_iPixmapTextHSpacing -
					       m_pContainer->pixmapSize().width(), 0xFFFF,
					       Qt::AlignTop | Qt::AlignLeft | Qt::WordBreak, m_strBrokenText.c_str() );
  }
  else
    assert( 0 );
}

void KIconContainerItem::setFixedPos( int _x, int _y )
{
  if ( _x == -1 && _y == -1 )
  {
    m_bFixedPos = false;
    return;
  }

  m_x = _x + m_pContainer->contentsX();
  m_y = _y + m_pContainer->contentsY();
  m_bFixedPos = true;
}

int KIconContainerItem::height()
{
  if ( m_pContainer->displayMode() == KIconContainer::Horizontal )
  {
    int h = m_pixmap.height();
    h += 3;
    h += m_textBoundingRect.height();
    return h;
  }
  else if ( m_pContainer->displayMode() == KIconContainer::Vertical )
  {
    int h = m_pixmap.height();
    if ( m_textBoundingRect.height() > h )
      return m_textBoundingRect.height();
    return h;
  }
  else
    assert( 0 );
}

bool KIconContainerItem::isSmallerThen( KIconContainerItem* _item )
{
  if ( hasFixedPos() && !_item->hasFixedPos() )
    return true;

  if ( strcmp( text(), _item->text() ) < 0 )
    return true;
  return false;
}

void KIconContainerItem::paint( QPainter* _painter, const QColorGroup _grp )
{
  if ( m_pContainer->displayMode() == KIconContainer::Horizontal )
  {
    if ( m_bSelected )
    {
      _painter->fillRect( 0, 0, m_pContainer->itemWidth(), height(), QApplication::winStyleHighlightColor() );
      _painter->setPen( Qt::white ); // A hack copied from Arnt :-) If even he does not know better ...
    }
    /* else
      _painter->eraseRect( 0, 0, m_pContainer->itemWidth(), height() ); */

    _painter->drawPixmap( ( m_pContainer->itemWidth() - m_pixmap.width() ) / 2, 0, m_pixmap );

    int h = m_pixmap.height() + 3;

    _painter->setPen( color() );
    _painter->drawText( ( m_pContainer->itemWidth() - m_textBoundingRect.width() ) / 2, h,
			m_textBoundingRect.width(), /* m_textBoundingRect.height() */ 0xFFFF,
			Qt::DontClip | Qt::AlignTop | Qt::AlignHCenter | Qt::WordBreak, m_strBrokenText.c_str() );
  }
  else if ( m_pContainer->displayMode() == KIconContainer::Vertical )
  {
    if ( m_bSelected )
    {
      _painter->fillRect( 0, 0, m_pContainer->itemWidth(), height(), QApplication::winStyleHighlightColor() );
      _painter->setPen( Qt::white ); // A hack copied from Arnt :-) If even he does not know better ...
    }
    /* else
      _painter->eraseRect( 0, 0, m_pContainer->itemWidth(), height() ); */

    _painter->drawPixmap( 0, 0, m_pixmap );

    _painter->setPen( color() );
    _painter->drawText( m_pContainer->pixmapSize().width() + m_iPixmapTextHSpacing, 0,
			m_textBoundingRect.width(), /* m_textBoundingRect.height() */ 0xFFFF,
			Qt::DontClip | Qt::AlignTop | Qt::AlignLeft | Qt::WordBreak, m_strBrokenText.c_str() );
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
  if ( m_pContainer->displayMode() == KIconContainer::Horizontal )
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
  else if ( m_pContainer->displayMode() == KIconContainer::Vertical )
  {
    if ( _x >= m_x && _x < m_x + m_pixmap.width() &&
	 _y >= m_y && _y < m_y + m_pixmap.height() )
      return true;

    int textoffset = m_pContainer->pixmapSize().width() + m_iPixmapTextHSpacing;
    int textwidth = m_textBoundingRect.width();

    if ( _x >= m_x + textoffset && _x < m_x + textoffset + textwidth &&
	 _y >= m_y && _y < m_y + m_textBoundingRect.height() )
      return true;

    return false;
  }
  else
    assert( 0 );
}

const QColor& KIconContainerItem::color()
{
  return m_pContainer->linkColor();
}

#include "kiconcontainer.moc"
