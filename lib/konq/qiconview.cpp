/****************************************************************************
** $Id$
**
** Definition of QIconView widget class
**
** Created : 990707
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/

#include "qiconview.h"

#include <qpixmap.h>
#include <qfontmetrics.h>
#include <qpainter.h>
#include <qevent.h>
#include <qpalette.h>
#include <qmime.h>
#include <qimage.h>
#include <qpen.h>
#include <qbrush.h>
#include <qtimer.h>
#include <qcursor.h>
#include <qkeycode.h>
#include <qapplication.h>
#include <qmultilineedit.h>
#include <qmap.h>
#include <qarray.h>

#include <stdlib.h>
#include <math.h>

static const char *unknown[] = {
    "32 32 11 1",
    "c c #ffffff",
    "g c #c0c0c0",
    "a c #c0ffc0",
    "h c #a0a0a4",
    "d c #585858",
    "f c #303030",
    "i c #400000",
    "b c #00c000",
    "e c #000000",
    "# c #000000",
    ". c None",
    "...###..........................",
    "...#aa##........................",
    ".###baaa##......................",
    ".#cde#baaa##....................",
    ".#cccdeebaaa##..##f.............",
    ".#cccccdeebaaa##aaa##...........",
    ".#cccccccdeebaaaaaaaa##.........",
    ".#cccccccccdeebaaaaaaa#.........",
    ".#cccccgcgghhebbbbaaaaa#........",
    ".#ccccccgcgggdebbbbbbaa#........",
    ".#cccgcgcgcgghdeebiebbba#.......",
    ".#ccccgcggggggghdeddeeba#.......",
    ".#cgcgcgcggggggggghghdebb#......",
    ".#ccgcggggggggghghghghd#b#......",
    ".#cgcgcggggggggghghghhd#b#......",
    ".#gcggggggggghghghhhhhd#b#......",
    ".#cgcggggggggghghghhhhd#b#......",
    ".#ggggggggghghghhhhhhhdib#......",
    ".#gggggggggghghghhhhhhd#b#......",
    ".#hhggggghghghhhhhhhhhd#b#......",
    ".#ddhhgggghghghhhhhhhhd#b#......",
    "..##ddhhghghhhhhhhhhhhdeb#......",
    "....##ddhhhghhhhhhhhhhd#b#......",
    "......##ddhhhhhhhhhhhhd#b#......",
    "........##ddhhhhhhhhhhd#b#......",
    "..........##ddhhhhhhhhd#b#......",
    "............##ddhhhhhhd#b###....",
    "..............##ddhhhhd#b#####..",
    "................##ddhhd#b######.",
    "..................##dddeb#####..",
    "....................##d#b###....",
    "......................####......"};

static QIconSet *unknown_icon = 0;

/*****************************************************************************
 *
 * Struct QIconViewPrivate
 *
 *****************************************************************************/

struct QIconViewPrivate
{
    QIconViewItem *firstItem, *lastItem;
    unsigned int count;
    QIconSet::Size mode;
    bool mousePressed, startDrag;
    QIconView::SelectionMode selectionMode;
    QIconViewItem *currentItem, *tmpCurrentItem, *highlightedItem;
    QRect *rubber;
    QTimer *scrollTimer, *adjustTimer, *updateTimer, *inputTimer,
	*singleClickSelectTimer;
    int rastX, rastY, spacing;
    bool cleared, dropped;
    int dragItems;
    int numSelectedItems;
    QPoint oldDragPos;
    QIconView::AlignMode alignMode;
    QIconView::ResizeMode resizeMode;
    QSize oldSize;
    bool firstAdjust;
    QValueList<QIconDragItem> iconDragData;
    bool isIconDrag;
    int numDragItems, cachedW, cachedH;
    int maxItemWidth, maxItemTextLength;
    QPoint dragStart;
    bool drawDragShade;
    QString currInputString;
    bool dirty, rearrangeEnabled;
    QIconView::ItemTextPos itemTextPos;
    bool reorderItemsWhenInsert;
    bool singleClickMode;
    QCursor oldCursor;
    bool resortItemsWhenInsert, sortOrder;

    struct SingleClickConfig {
	SingleClickConfig()
	    : normalText( 0 ), normalTextCol( 0 ),
	      highlightedText( 0 ), highlightedTextCol( 0 ),
	      highlightedCursor( 0 ), setCurrentInterval( -1 ) {
	}
	
	QFont *normalText;
	QColor *normalTextCol;
	QFont *highlightedText;
	QColor *highlightedTextCol;
	QCursor *highlightedCursor;
	int setCurrentInterval;
    } singleClickConfig;

    struct SortableItem {
	QIconViewItem *item;
    };
};

/*****************************************************************************
 *
 * Class QIconViewItemLineEdit
 *
 *****************************************************************************/

class QIconViewItemLineEdit : public QMultiLineEdit
{
    friend class QIconViewItem;

    Q_OBJECT

public:
    QIconViewItemLineEdit( const QString &text, QWidget *parent, QIconViewItem *theItem, const char *name = 0 );

signals:
    void escapePressed();

protected:
    void keyPressEvent( QKeyEvent *e );
    void focusOutEvent( QFocusEvent *e );

protected:
    QIconViewItem *item;
    QString startText;

};

QIconViewItemLineEdit::QIconViewItemLineEdit( const QString &text, QWidget *parent,
					      QIconViewItem *theItem, const char *name )
    : QMultiLineEdit( parent, name ), item( theItem ), startText( text )
{
//     setWordWrap( QMultiLineEdit::FixedWidthWrap | QMultiLineEdit::BreakWithinWords );
//     setWrapColumnOrWidth( item->iconView()->maxItemWidth() - ( item->iconView()->itemTextPos() == QIconView::Bottom ?
// 			  0 : item->iconRect().width() ) );
//     setMaxLength( item->iconView()->maxItemTextLength() );
    setAlignment( Qt::AlignCenter );
    setText( text );
    clearTableFlags();
}

void QIconViewItemLineEdit::keyPressEvent( QKeyEvent *e )
{
    if ( e->key()  == Key_Escape ) {
	item->QIconViewItem::setText( startText );
	emit escapePressed();
    } else if ( e->key() == Key_Enter ||
	      e->key() == Key_Return )
	emit returnPressed();
    else {
	QMultiLineEdit::keyPressEvent( e );
	int w = width();
	int h = height();
	w = totalWidth();
	h = totalHeight();
	QSize s( size() );
	resize( w, h );
	if ( s != QSize( w, h ) ) {
	    item->calcRect( text() );
	    item->repaint();
	}
    }
}

void QIconViewItemLineEdit::focusOutEvent( QFocusEvent * )
{
    emit escapePressed();
}

/*****************************************************************************
 *
 * Class QIconDragItem
 *
 *****************************************************************************/

/*!
  \class QIconDragItem qiconview.h
  \brief The QIconDragItem is the internal data structure of a QIconDrag


*/

/*!
 */

QIconDragItem::QIconDragItem()
    : iconRect_(), textRect_()
{
    makeKey();
}

/*!
 */

QIconDragItem::QIconDragItem( const QRect &ir, const QRect &tr )
    : iconRect_( ir ), textRect_( tr )
{
    makeKey();
}

/*!
 */

QIconDragItem::~QIconDragItem()
{
}

/*!
 */

bool QIconDragItem::operator<( const QIconDragItem &icon ) const
{
    return key_ < icon.key_;
}

/*!
 */

bool QIconDragItem::operator==( const QIconDragItem &icon ) const
{
    return key_ == icon.key_;
}

/*!
 */

void QIconDragItem::makeKey()
{
    QString k( "%1 %2 %3 %4 %5 %6 %7 %8" );
    k = k.arg( iconRect().x() ).arg( iconRect().y() ).arg( iconRect().width() ).
	arg( iconRect().height() ).arg( textRect().x() ).arg( textRect().y() ).
	arg( textRect().width() ).arg( textRect().height() );
}

/*!
 */

QRect QIconDragItem::textRect() const
{
    return textRect_;
}

/*!
 */

QRect QIconDragItem::iconRect() const
{
    return iconRect_;
}

/*!
 */

QString QIconDragItem::key() const
{
    return key_;
}

/*!
 */

void QIconDragItem::setIconRect( const QRect &r )
{
    iconRect_ = r;
}

/*!
 */

void QIconDragItem::setTextRect( const QRect &r )
{
    textRect_ = r;
}

/*****************************************************************************
 *
 * Class QIconDrag
 *
 *****************************************************************************/

/*!
  \class QIconDrag qiconview.h
  \brief The QIconDrag is the drag object which is used for moving items in the iconview

  The QIconDrag is the drag object which is used for moving items in the
  iconview. The QIconDrag stores exact informations about the positions of
  the items, which are dragged, so that each iconview is able to draw drag
  shades in correct positions.

  It's suggested that, if you write a drag object for own QIconViewItems,
  you derive the drag object class from QIconDrag and just implement the
  methods which are needed for encoding/decoding your data and the mimetype
  handling. Because if you do this, the position informations will be stored
  in the drag object too.

  An example, how to implement this, is in the QtFileIconView example.
*/

/*!
 */

QIconDrag::QIconDrag( const QIconList &icons_, QWidget * dragSource, const char* name )
    : QDragObject( dragSource, name ), icons( icons_ )
{
}

/*!
 */

QIconDrag::QIconDrag( QWidget * dragSource, const char* name )
    : QDragObject( dragSource, name )
{
}

/*!
 */

QIconDrag::~QIconDrag()
{
}

/*!
 */

void QIconDrag::setIcons( const QIconList &list_ )
{
    icons = list_;
}

/*!
 */

void QIconDrag::append( const QIconDragItem &icon_ )
{
    icons.append( icon_ );
}

/*!
 */

const char* QIconDrag::format( int i ) const
{
    if ( i == 0 )
	return "application/x-qiconlist";
    return 0;
}

/*!
 */

QByteArray QIconDrag::encodedData( const char* mime ) const
{
    QByteArray a;
    if ( QString( mime ) == "application/x-qiconlist" ) {
	int c = 0;
	QIconList::ConstIterator it = icons.begin();
	for ( ; it != icons.end(); ++it ) {
	    QString k( "%1 %2 %3 %4 %5 %6 %7 %8" );
	    k = k.arg( (*it).iconRect().x() ).arg( (*it).iconRect().y() ).arg( (*it).iconRect().width() ).
		arg( (*it).iconRect().height() ).arg( (*it).textRect().x() ).arg( (*it).textRect().y() ).
		arg( (*it).textRect().width() ).arg( (*it).textRect().height() );
	    int l = k.length();
	    a.resize( c + l + 1 );
	    memcpy( a.data() + c , k.latin1(), l );
	    a[ c + l ] = 0;
	    c += l + 1;
	}
	a.resize( c - 1 );
    }

    return a;
}

/*!
 */

bool QIconDrag::canDecode( QMimeSource* e )
{
    if ( e->provides( "application/x-qiconlist" ) )
	return TRUE;
    return FALSE;
}

/*!
 */

bool QIconDrag::decode( QMimeSource* e, QIconList &list_ )
{
    QByteArray ba = e->encodedData( "application/x-qiconlist" );
    if ( ba.size() ) {
	list_.clear();
	uint c = 0;
	
	char* d = ba.data();
	
	while ( c < ba.size() ) {
	    uint f = c;
	    while ( c < ba.size() && d[ c ] )
		c++;
	    QString s;
	    if ( c < ba.size() ) {
		s = d + f ;
		c++;
	    } else  {
		QString tmp( QString(d + f).left( c - f + 1 ) );
		s = tmp;
	    }

	    QIconDragItem icon;
	    QRect ir, tr;
	
	    ir.setX( atoi( s.latin1() ) );
	    int pos = s.find( ' ' );
	    if ( pos == -1 )
		return FALSE;
	    ir.setY( atoi( s.latin1() + pos + 1 ) );
	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    ir.setWidth( atoi( s.latin1() + pos + 1 ) );
	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    ir.setHeight( atoi( s.latin1() + pos + 1 ) );

	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    tr.setX( atoi( s.latin1() + pos + 1 ) );
	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    tr.setY( atoi( s.latin1() + pos + 1 ) );
	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    tr.setWidth( atoi( s.latin1() + pos + 1 ) );
	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    tr.setHeight( atoi( s.latin1() + pos + 1 ) );
	
	    icon.setIconRect( ir );
	    icon.setTextRect( tr );
	    list_.append( icon );
	}
	return TRUE;
    }

    return FALSE;
}

/*****************************************************************************
 *
 * Class QIconViewItem
 *
 *****************************************************************************/

/*!
  \class QIconViewItem qiconview.h
  \brief The QIconViewItem class implements a iconview item.

  An iconview item is a object containing an icon and a text, which can
  display itself in an iconview.

  The most simple way to create an iconview item and insert it into an
  iconview is, to construct it with a pointer to the iconview, a string
  and an icon:

  \code
    \/ parent is a pointer to our iconview, pixmap a QPixmap,
    \/ which we want to use as icon
    (void) new QIconViewItem( parent,
			      "This is the text of the item",
			      QIconSet(pixmap) );
  \endcode

  When the iconview is deleted, all items of it are deleted automatically,
  so the programmer must not delete the iconview items itself.

  To iterate over all items of an iconview do something like

  \code
    QIconViewItem *item;
    for ( item = iconview->firstItem(); item; item = item->nextItem() )
      do_something_with( item );
  \endcode

  As the iconview is designed to use DnD, the iconview has methods for DnD
  too.

  Subclassing QIconViewItem will provide a big flexibility.
*/

/*!
  \fn void QIconViewItem::renamed ()
  This signal is emitted when the item has been renamed using
  in-place renaming.
*/

/*!
  \fn void QIconViewItem::renamed (const QString & text)
  This signal is emitted when the item has been renamed using
  in-place renaming. \a text is the new item-text.
*/

/*!
 Constructs an iconview item with no text and a default icon, and
 inserts it into the iconview \a parent.
*/

QIconViewItem::QIconViewItem( QIconView *parent )
    : view( parent ), itemText(), itemIcon( *unknown_icon ),
      prev( 0 ), next( 0 ), allow_rename( TRUE ), allow_drag( TRUE ), allow_drop( TRUE ),
      selected( FALSE ), selectable( TRUE ), fm( 0 ), renameBox( 0 )
{
    init();
}

/*!
 Constructs an iconview item with no text and a default icon, and inserts
 it into the iconview \a parent after the iconview item \a after.
*/

QIconViewItem::QIconViewItem( QIconView *parent, QIconViewItem *after )
    : view( parent ), itemText(), itemIcon( *unknown_icon ),
      prev( 0 ), next( after ), allow_rename( TRUE ), allow_drag( TRUE ), allow_drop( TRUE ),
      selected( FALSE ), selectable( TRUE ), fm( 0 ), renameBox( 0 )
{
    init();
}

/*!
  Constructs an iconview item using \a text as text and a default icon,
  and inserts it into the iconview \a parent.
*/

QIconViewItem::QIconViewItem( QIconView *parent, const QString &text )
    : view( parent ), itemText( text ), itemIcon( *unknown_icon ),
      prev( 0 ), next( 0 ), allow_rename( TRUE ), allow_drag( TRUE ), allow_drop( TRUE ),
      selected( FALSE ), selectable( TRUE ), fm( 0 ), renameBox( 0 )
{
    init();
}

/*!
  Constructs an iconview item using \a text as text and a default icon, and
  inserts it into the iconview \a parent after the iconview item \a after.
*/

QIconViewItem::QIconViewItem( QIconView *parent, QIconViewItem *after,
			      const QString &text )
    : view( parent ), itemText( text ), itemIcon( *unknown_icon ),
      prev( 0 ), next( after ), allow_rename( TRUE ), allow_drag( TRUE ), allow_drop( TRUE ),
      selected( FALSE ), selectable( TRUE ), fm( 0 ), renameBox( 0 )
{
    init();
}

/*!
  Constructs an iconview item using \a text as text and a \icon as icon,
  and inserts it into the iconview \a parent.
*/

QIconViewItem::QIconViewItem( QIconView *parent, const QString &text,
			      const QIconSet &icon )
    : view( parent ), itemText( text ), itemIcon( icon ),
      prev( 0 ), next( 0 ), allow_rename( TRUE ), allow_drag( TRUE ), allow_drop( TRUE ),
      selected( FALSE ), selectable( TRUE ), fm( 0 ), renameBox( 0 )
{
    init();
}

/*!
  Constructs an iconview item using \a text as text and a \icon as icon,
  and inserts it into the iconview \a parent after the iconview item \a
  after.
*/

QIconViewItem::QIconViewItem( QIconView *parent, QIconViewItem *after, const QString &text, const QIconSet &icon )
    : view( parent ), itemText( text ), itemIcon( icon ),
      prev( 0 ), next( after ), allow_rename( TRUE ), allow_drag( TRUE ), allow_drop( TRUE ),
      selected( FALSE ), selectable( TRUE ), fm( 0 ), renameBox( 0 )
{
    init();
}

/*!
  Initializes the iconview item and inserts it into the iconview.
*/

void QIconViewItem::init()
{
    f = 0;
    if ( view ) {
	itemKey = itemText;
	dirty = TRUE;
	fm = new QFontMetrics( view->font() );
	itemViewMode = view->viewMode();

	calcRect();
	view->insertItem( this );
    }
}

/*!
  Destructs the iconview item and tells the iconview about it.
*/

QIconViewItem::~QIconViewItem()
{
    if ( f )
	delete f;
    view->removeItem( this );
    removeRenameBox();
}

/*!
  Sets \a text as text of the iconview item.
*/

void QIconViewItem::setText( const QString &text )
{
    if ( text == itemText )
	return;

    itemText = text;
    if ( itemKey.isEmpty() )
	itemKey = itemText;
    calcRect();
    repaint();
}

/*!
  Sets \a k as key of the iconview item. This is
  used for sorting.
*/

void QIconViewItem::setKey( const QString &k )
{
    if ( k == itemKey )
	return;

    itemKey = k;
}

/*!
  Sets \a icon as item icon of the iconview item.
*/

void QIconViewItem::setIcon( const QIconSet &icon )
{
    itemIcon = icon;
    calcRect();
    repaint();
}

/*!
  Sets \a text as text of the iconview item. Using \a recalc you can specify,
  if the iconview should be recalculated and repainted or not.
*/

void QIconViewItem::setText( const QString &text, bool recalc )
{
    if ( text == itemText )
	return;

    itemText = text;

    if ( recalc ) {
	calcRect();
	repaint();
    }
}

/*!
  Sets \a icon as item icon of the iconview item. Using \a recalc you can
  specify, if the iconview should be recalculated and repainted or not.
*/

void QIconViewItem::setIcon( const QIconSet &icon, bool recalc )
{
    itemIcon = icon;

    if ( recalc ) {
	calcRect();
	repaint();
    }
}

/*!
  If \allow is TRUE, the user can rename the iconview item, when clicking
  on the text after the item has been selected (in-place renaming). If
  \a allow is FALSE, no inplace-renaming is done.
*/

void QIconViewItem::setRenameEnabled( bool allow )
{
    allow_rename = allow;
}

/*!
  If \a allow is TRUE, the iconview lets the user to drag the iconview
  item (inside the iconview and outside of it). if \a allow is FALSE, no
  dragging is possible with this item.
*/

void QIconViewItem::setDragEnabled( bool allow )
{
    allow_drag = allow;
}

/*!
  If \a allow is TRUE, the iconview lets the use drop something on this
  iconview item.
*/

void QIconViewItem::setDropEnabled( bool allow )
{
    allow_drop = allow;
}

/*!
  Returns the text of the iconview item.
*/

QString QIconViewItem::text() const
{
    return itemText;
}

/*!
  Returns the key of the iconview item.
*/

QString QIconViewItem::key() const
{
    return itemKey;
}

/*!
  Returns the icon of the iconview item.
*/

QIconSet QIconViewItem::icon() const
{
    return itemIcon;
}

/*!
  Returns TRUE, if the item can be renamed ny the user with in-place renaming,
  else FALSE.
*/

bool QIconViewItem::renameEnabled() const
{
    return allow_rename;
}

/*!
  Returns TRUE, if the user is allowed to drag the iconview item, else FALSE.
*/

bool QIconViewItem::dragEnabled() const
{
    return allow_drag;
}

/*!
  Returns TRUE, if the user is allowed to drop something onto the item,
  otherwise FALSE.
*/

bool QIconViewItem::dropEnabled() const
{
    return allow_drop;
}

/*!
  Returns a pointer to the iconview of this item.
*/

QIconView *QIconViewItem::iconView() const
{
    return view;
}

/*!
  Returns a pointer to the previous item, or NULL if this is the first item
  of the iconview.
*/

QIconViewItem *QIconViewItem::prevItem() const
{
    return prev;
}

/*!
  Returns a pointer to the next item, or NULL if this is the last item
  of the iconview.
*/

QIconViewItem *QIconViewItem::nextItem() const
{
    return next;
}

/*!
  Returns the index of this item in the iconview, of -1 if something went
  wrong.
*/

int QIconViewItem::index() const
{
    if ( view )
	return view->index( this );

    return -1;
}

/*!
  Selects / Unselects the item depending on the selectionMode of the iconview.

  If \a s is FALSE, the item gets unselected. If \a s is TRUE
  <li> and QIconView::selectionMode is Single, the item gets selected and the
  item which was selected, gets unselected
  <li> and QIconView::selectionMode is StrictMulti and \a cb is TRUE, the
  item gets selected
  <li> and QIconView::selectionMode is Multi the item gets selected.

  The item redraws itself if the selection changed.
*/

void QIconViewItem::setSelected( bool s, bool cb )
{
    if ( selectable && s != selected ) {
	if ( !s )
	    selected = FALSE;
	else {
	    if ( view->d->selectionMode == QIconView::Single && view->d->currentItem )
		view->d->currentItem->setSelected( FALSE );
	    else if ( view->d->selectionMode == QIconView::StrictMulti && !cb )
		view->selectAll( FALSE );
	}
	selected = s;

	repaint();
	view->emitSelectionChanged();
    }
}

/*!
  If \a s is TRUE, this item can be selected, else it can't be selected.
*/

void QIconViewItem::setSelectable( bool s )
{
    selectable = s;
    if ( selected )
	selected = FALSE;
}

/*!
  Returns TRUE, if the item is selected, else FALSE.
*/

bool QIconViewItem::isSelected() const
{
    return selected;
}

/*!
  Returns TRUE, of the item is selectable, else FALSE.
*/

bool QIconViewItem::isSelectable() const
{
    return selectable;
}

/*!
  Repaints the item.
*/

void QIconViewItem::repaint()
{
    if ( view )
	view->repaintItem( this );
}

/*!
  Moves the item to \a x and \a y in the iconview (these are contents
  coordinates)
*/

void QIconViewItem::move( int x, int y )
{
    itemRect.setRect( x, y, itemRect.width(), itemRect.height() );
}

/*!
  Moves the item by the distance \a dx and \a dy.
*/

void QIconViewItem::moveBy( int dx, int dy )
{
    itemRect.moveBy( dx, dy );
}

/*!
  \overload void QIconViewItem::move( const QPoint &pnt  )
*/

void QIconViewItem::move( const QPoint &pnt )
{
    move( pnt.x(), pnt.y() );
}

/*!
  \overload void QIconViewItem::moveBy( const QPoint &pnt )
*/

void QIconViewItem::moveBy( const QPoint &pnt )
{
    moveBy( pnt.x(), pnt.y() );
}

/*!
  Returns the bounding rect of the item (in contents coordinates).
*/

QRect QIconViewItem::rect() const
{
    return itemRect;
}

/*!
  Returns the X-Coordinate of the item (in contents coordinates).
*/

int QIconViewItem::x() const
{
    return itemRect.x();
}

/*!
  Returns the Y-Coordinate of the item (in contents coordinates).
*/

int QIconViewItem::y() const
{
    return itemRect.y();
}

/*!
  Returns the  width of the item.
*/

int QIconViewItem::width() const
{
    return itemRect.width();
}

/*!
  Returns the height of the item.
*/

int QIconViewItem::height() const
{
    return itemRect.height();
}

/*!
  Returns the size of the item.
*/

QSize QIconViewItem::size() const
{
    return QSize( itemRect.width(), itemRect.height() );
}

/*!
  Returns the position of the item (in contents coordinates).
*/

QPoint QIconViewItem::pos() const
{
    return QPoint( itemRect.x(), itemRect.y() );
}

/*!
  Returns the rectangle of the text of the item.
  If relative is FALSE, the returned rectangle is relative to the origin of
  the contents' coordinate system, else the rectangle is relative to the
  origin of the item's rectangle
*/

QRect QIconViewItem::textRect( bool relative ) const
{
    if ( relative )
	return itemTextRect;
    else
	return QRect( x() + itemTextRect.x(), y() + itemTextRect.y(), itemTextRect.width(), itemTextRect.height() );
}

/*!
  Returns the rectangle of the icon of the item.
  If relative is FALSE, the returned rectangle is relative to the origin of
  the contents' coordinate system, else the rectangle is relative to the
  origin of the item's rectangle
*/

QRect QIconViewItem::iconRect( bool relative ) const
{
    if ( relative )
	return itemIconRect;
    else
	return QRect( x() + itemIconRect.x(), y() + itemIconRect.y(), itemIconRect.width(), itemIconRect.height() );
}

/*!
  Returns TRUE, if the item contains the point \a pnt (in contents
  coordinates).
*/

bool QIconViewItem::contains( QPoint pnt ) const
{
    return ( textRect( FALSE ).contains( pnt ) ||
	     iconRect( FALSE ).contains( pnt ) );
}

/*!
  Returns TRUE, if the item intersects the rectangle \a r (in contents
  coordinates).
*/

bool QIconViewItem::intersects( QRect r ) const
{
    return ( textRect( FALSE ).intersects( r ) ||
	     iconRect( FALSE ).intersects( r ) );
}

/*!
  Changes font of this item to \a font.
*/

void QIconViewItem::setFont( const QFont &font )
{
    if ( !f )
	f = new QFont;
    *f = font;

    if ( fm )
	delete fm;

    fm = new QFontMetrics( *f );
    calcRect();
    repaint();
}

/*!
  Returns the font of this item.
*/

QFont QIconViewItem::font() const
{
    if ( !f )
	return view->font();
    return *f;
}

/*!
  Sets the viewmode of the item. This can be QIconSet::Automatic,
  QIconSet::Small, QIconSet::Large.  QIconSet::Automatic is default. Use
  QIconView::setViewMode() instead to set the viewmode of all items.

  \sa QIconView::setViewMode()
*/

void QIconViewItem::setViewMode( QIconSet::Size mode )
{
    itemViewMode = mode;
    calcRect();
}

/*!
  Returns the viewmode of the item;

  \sa QIconView::viewMode()
*/

QIconSet::Size QIconViewItem::viewMode() const
{
    return itemViewMode;
}

/*!
  Returns TRUE, of the item accepts the QMimeSource \a mime (so it could
  be dropped on the item), else FALSE is returned.

  The default implementation does nothing and returns always TRUE. A
  subclass should reimplement this to allow dropping something on an item.

  \sa QFileIconViewItem::acceptDrop()
*/

bool QIconViewItem::acceptDrop( const QMimeSource *mime ) const
{
    if ( mime )
	;
    return FALSE;
}

/*!
  Starts renaming an icon.
*/

void QIconViewItem::rename()
{
    oldRect = rect();
    renameBox = new QIconViewItemLineEdit( itemText, view->viewport(), this );
    renameBox->resize( textRect().width() + fm->width( ' ' ), textRect().height() );
    view->addChild( renameBox, textRect( FALSE ).x(), textRect( FALSE ).y() );
    renameBox->setFrameStyle( QFrame::Plain | QFrame::Box );
    renameBox->setLineWidth( 1 );
    renameBox->selectAll();
    renameBox->setFocus();
    renameBox->show();
    view->viewport()->setFocusProxy( renameBox );
    connect( renameBox, SIGNAL( returnPressed() ),
	     this, SLOT( renameItem() ) );
    connect( renameBox, SIGNAL( escapePressed() ),
	     this, SLOT( cancelRenameItem() ) );
}

/*!
  This function is called when the user pressed RETURN in the in-place
  renaming.
*/

void QIconViewItem::renameItem()
{
    if ( !renameBox )
	return;

    QRect r = itemRect;
    setText( renameBox->text() );
    view->repaintContents( oldRect.x() - 1, oldRect.y() - 1, oldRect.width() + 2, oldRect.height() + 2, FALSE );
    view->repaintContents( r.x() - 1, r.y() - 1, r.width() + 2, r.height() + 2, FALSE );
    removeRenameBox();

    emit renamed( text() );
    emit renamed();
    view->emitRenamed( this );
}

/*!
  Cancels in-place renaming.
*/

void QIconViewItem::cancelRenameItem()
{
    QRect r = itemRect;
    calcRect();
    view->repaintContents( oldRect.x() - 1, oldRect.y() - 1, oldRect.width() + 2, oldRect.height() + 2, FALSE );
    view->repaintContents( r.x() - 1, r.y() - 1, r.width() + 2, r.height() + 2, FALSE );

    if ( !renameBox )
	return;

    removeRenameBox();
}

/*!
  Removed the editbox which was used for in-place renaming.
*/

void QIconViewItem::removeRenameBox()
{
    if ( !renameBox )
	return;

    delete renameBox;
    renameBox = 0;
    view->viewport()->setFocusProxy( view );
    view->setFocus();
}

/*!
  Recalculates the bounding rect, text- and iconrect of the item.
*/

void QIconViewItem::calcRect( const QString &text_ )
{
    if ( !fm )
	return;

    int pw = 0;
    int ph = 0;

    pw = itemIcon.pixmap( itemViewMode, QIconSet::Normal ).width() + 2;
    ph = itemIcon.pixmap( itemViewMode, QIconSet::Normal ).height() + 2;

    itemIconRect.setWidth( pw );
    itemIconRect.setHeight( ph );

    QString t = text_;
    if ( t.isEmpty() )
	t = itemText;

    int tw = 0;
    int th = 0;
    QRect r( fm->boundingRect( 0, 0, iconView()->maxItemWidth() - ( iconView()->itemTextPos() == QIconView::Bottom ? 0 :
			       iconRect().width() ),
			       0xFFFFFFFF, Qt::AlignCenter | Qt::WordBreak, t ) );
    tw = r.width();
    th = r.height();
    if ( tw < fm->width( "X" ) )
	tw = fm->width( "X" );

    itemTextRect.setWidth( tw );
    itemTextRect.setHeight( th );

    int w = 0;
    int h = 0;
    if ( view->itemTextPos() == QIconView::Bottom ) {
	w = QMAX( itemTextRect.width(), itemIconRect.width() );
	h = itemTextRect.height() + itemIconRect.height() + 1;

	itemRect.setWidth( w );
	itemRect.setHeight( h );

	itemTextRect = QRect( ( width() - itemTextRect.width() ) / 2, height() - itemTextRect.height(),
			      itemTextRect.width(), itemTextRect.height() );
	itemIconRect = QRect( ( width() - itemIconRect.width() ) / 2, 0,
			      itemIconRect.width(), itemIconRect.height() );
    } else {
	h = QMAX( itemTextRect.height(), itemIconRect.height() );
	w = itemTextRect.width() + itemIconRect.width() + 1;

	itemRect.setWidth( w );
	itemRect.setHeight( h );

	itemTextRect = QRect( width() - itemTextRect.width(), ( height() - itemTextRect.height() ) / 2,
			      itemTextRect.width(), itemTextRect.height() );
	itemIconRect = QRect( 0, ( height() - itemIconRect.height() ) / 2,
			      itemIconRect.width(), itemIconRect.height() );
    }
}

/*!
  Paints the item using the painter \a p.
*/

void QIconViewItem::paintItem( QPainter *p )
{
    if ( f )
	p->setFont( *f );
    else
	p->setFont( view->font() );

    if ( view->d->singleClickMode ) {
	if ( view->d->highlightedItem == this ) {
	    if ( view->d->singleClickConfig.highlightedText && !f )
		p->setFont( *view->d->singleClickConfig.highlightedText );
	    if ( view->d->singleClickConfig.highlightedTextCol )
		p->setPen( *view->d->singleClickConfig.highlightedTextCol );
	} else {
	    if ( view->d->singleClickConfig.normalText && !f )
		p->setFont( *view->d->singleClickConfig.normalText );
	    if ( view->d->singleClickConfig.normalTextCol )
		p->setPen( *view->d->singleClickConfig.normalTextCol );
	}
    }

    QIconSet::Mode m = QIconSet::Normal;
    if ( isSelected() )
	m = QIconSet::Active;
    else if ( !isSelectable() )
	m = QIconSet::Disabled;

    if ( view->itemTextPos() == QIconView::Bottom ) {
	int w = itemIcon.pixmap( itemViewMode, QIconSet::Normal ).width();

	if ( isSelected() )
	    p->fillRect( iconRect( FALSE ), view->colorGroup().highlight() );
	p->drawPixmap( x() + ( width() - w ) / 2, y(), itemIcon.pixmap( itemViewMode, m ) );

	p->save();
	if ( isSelected() ) {
	    p->fillRect( textRect( FALSE ), view->colorGroup().highlight() );
	    p->setPen( QPen( view->colorGroup().highlightedText() ) );
	} /*else
	    p->setPen( view->colorGroup().text() );*/

	p->drawText( textRect( FALSE ), Qt::AlignCenter | Qt::WordBreak, itemText );

	p->restore();
    } else {
	int h = itemIcon.pixmap( itemViewMode, QIconSet::Normal ).height();

	if ( isSelected() )
	    p->fillRect( iconRect( FALSE ), view->colorGroup().highlight() );
	p->drawPixmap( x() , y() + ( height() - h ) / 2, itemIcon.pixmap( itemViewMode, m ) );

	p->save();
	if ( isSelected() ) {
	    p->fillRect( textRect( FALSE ), view->colorGroup().highlight() );
	    p->setPen( QPen( view->colorGroup().highlightedText() ) );
	} /*else
	    p->setPen( view->colorGroup().text() );*/

	p->drawText( textRect( FALSE ), Qt::AlignCenter | Qt::WordBreak, itemText );

	p->restore();
    }
}

/*!
  Paints the focus rect of the item using the painter \a.
*/

void QIconViewItem::paintFocus( QPainter *p )
{
    view->style().drawFocusRect( p, QRect( textRect( FALSE ).x(), textRect( FALSE ).y(),
					   textRect( FALSE ).width(), textRect( FALSE ).height() ),
				 view->colorGroup(), isSelected() ? &view->colorGroup().highlight() :
				 &view->colorGroup().base(), isSelected() );
    view->style().drawFocusRect( p, QRect( iconRect( FALSE ).x(), iconRect( FALSE ).y(), iconRect( FALSE ).width(),
					   iconRect( FALSE ).height() ),
				 view->colorGroup(), isSelected() ? &view->colorGroup().highlight() :
				 &view->colorGroup().base(), isSelected() );
}

/*!
  This method is called, when something was dropped on the item. \a e
  gives you all information about the drop.

  The default implementation does nothing, subclasses should reimplement this
  method.

  \sa QFileIconViewItem::dropped()
*/

void QIconViewItem::dropped( QDropEvent *e )
{
    if ( e )
	;
}

/*!
  This method is called, when a drag entered the item's bounding rect.

  The default implementation does nothing, subcasses should reimplement
  this method.
*/

void QIconViewItem::dragEntered()
{
}

/*!
  This method is called, when a drag left the item's bounding rect.

  The default implementation does nothing, subcasses should reimplement
  this method.
*/

void QIconViewItem::dragLeft()
{
}

/*!
  Sets the iconview of the item ot \a v.
*/

void QIconViewItem::setView( QIconView *v )
{
    view = v;
}

/*!
  Sets the bounding rectangle of the whole item. This should
  be only used in subclasses, which reimplement calcRect() to be able
  to set the calcualted rectangle
*/

void QIconViewItem::setItemRect( const QRect &r )
{
    itemRect = r;
}

/*!
  Sets the bounding rectangle of the item text. This should
  be only used in subclasses, which reimplement calcRect() to be able
  to set the calcualted rectangle
*/

void QIconViewItem::setTextRect( const QRect &r )
{
    itemTextRect = r;
}

/*!
  Sets the bounding rectangle of the item icon. This should
  be only used in subclasses, which reimplement calcRect() to be able
  to set the calcualted rectangle
*/

void QIconViewItem::setIconRect( const QRect &r )
{
    itemIconRect = r;
}


/*****************************************************************************
 *
 * Class QIconView
 *
 *****************************************************************************/

/*!
  \class QIconView qiconview.h
  \brief The QIconView class

  The QIconView provides a widget which can contain lots of iconview items which can
  be selected, dragged and so on. The iconview can be used in double click mode
  (default) or single click mode (behaves like a web page). For the programmer which
  uses the iconview these modes are transparent, this means the iconview does all the
  work for that.

  Items can be inserted in a grid and can flow from top to bottom (South) or from
  left to right (East). The text can be either displayed at the bottom of the icons
  or the the right of the icons.

  The QIconView is designed for Drag'n'Drop, as the icons are also moved inside
  the iconview itself using DnD. So the QIconView provides some methods for
  extended DnD too.

  There can be specified different selection modes, which describe if more that one item
  can be selected and under which conditions.

  Items can of course be selected. When multiple items may be selected, the iconview
  provides a rubberband too.

  Items can also be in-place renamed.
*/

/*! \enum QIconView::ResizeMode

  This enum type decides how QIconView should treat the positions of
  its icons when the widget is resized.  The currently defined modes
  are: <ul>

  <li> \c Fixed - the icons' positions are not changed.
  <li> \c Adjust - the icons' positions are adjusted to be within the
  new geometry, if possible.

  </ul>
*/


/*! \enum QIconView::SelectionMode

  This enum type specifies the different selection modes of the
  iconview.
  <ul>
  <li>\c Single (only one item can be selected)
  <li>\c Multi (multiple items can be selected)
  <li>\c StrictMulti (multiple items can be selected, but only if the user pressed CTRL while selecting them)
  </ul>
*/

/*! \enum QIconView::AlignMode

   This enum type descides in which direction the items, which do not fit onto the
   screen anymore flow.

   <ul>
   <li> \c East (Items, which don't fit onto the view, go further down (you get a vertical scrollbar)
   <li> \c South (Items, which don't fit onto the view, go further right (you get a horizontal scrollbar)
   </ul>
*/

/*! \enum QIconView::ItemTextPos

   This enum type specifies the position of the item text in relation to the icon.

   <ul>
   <li> \c Bottom (The text is drawn at the bottom of the icon)
   <li> \c Right (The text is drawn at the right of the icon)
   </ul>
*/

/*! \fn void  QIconView::dropped (QDropEvent * e)
  This signal is emitted, when a drop event occured onto the viewport (not onto an icon),
  which the iconview itself can't handle
*/

/*! \fn void  QIconView::moved ()
  This signal is emitted after successfully dropping an (or some) item(s) of the iconview
  somewhere and if they should be removed now.
*/

/*! \fn void  QIconView::doubleClicked (QIconViewItem * item)
  This signal is emitted, if the user doubleclicked on the item \a item.
*/

/*! \fn void  QIconView::itemRightPressed (QIconViewItem * item)
  This signal is emitted, if the user pressed on the item \a item using the right mouse button.
*/

/*! \fn void  QIconView::viewportRightPressed ()
  This signal is emitted, when the user pressed with the right mouse button onto the viewport
  (not on an item)
*/

/*! \fn void  QIconView::itemRightClicked (QIconViewItem * item)
  This signal is emitted, if the user clicked (pressed + released) on the
  item \a item using the right mouse button.
*/

/*! \fn void  QIconView::viewportRightClicked ()
  This signal is emitted, when the user clicked (pressed + released) with
  the right mouse button onto the viewport (not on an item)
*/

/*! \fn void  QIconView::selectionChanged ()
  This signal is emitted when the selection has been changed.
*/

/*! \fn void  QIconView::selectionChanged (int numItems)
  This signal is emitted when the selection has been changed.
  \a numItems specifies the number of selected items.
*/

/*! \fn void QIconView::currentChanged ()
  This signal is emitted, when the different items got current.
*/

/*! \fn void QIconView::currentChanged ( QIconViewItem *item )
  This signal is emitted, when the different items got current.
  \a item is the new current item of 0, if no items is current now.
*/

/*! \fn void  QIconView::onItem( QIconViewItem *i )
  This signal is emitted, when the user moves the mouse cursor onto an item.
  It´s only emitted once per item.
*/

/*! \fn void  QIconView::onViewport()
  This signal is emitted, when the user moves the mouse cursor, which was
  on an item away from the item onto the viewport.
*/

/*!
  \fn void QIconView::itemRenamed (QIconViewItem * item)
  If the \a item has been renamed (e.g. by in-place renaming),
  this signal is emitted.
*/

/*!
  \fn void QIconView::itemRenamed (QIconViewItem * item, const QString &name)
  If the \a item has been renamed (e.g. by in-place renaming),
  this signal is emitted. \a name is the new text (name) of the item.
*/

/*!
  \fn void QIconView::rightButtonPressed (QIconViewItem * item, const QPoint & pos)
  This signal is emitted wher the user pressed with the right mouse button on
  either and item (then \a item is the item under the mouse cursor) or
  somewhere else (then \a item is NULL). \a pos the position of the mouse cursor.
*/

/*!
  \fn void QIconView::rightButtonClicked (QIconViewItem * item, const QPoint & pos)
  This signal is emitted wher the user clicked (pressed + released) with the right
  mouse button on either and item (then \a item is the item under the mouse cursor) or
  somewhere else (then \a item is NULL). \a pos the position of the mouse cursor.
*/

/*!
  \fn void QIconView:mouseButtonPressed (int button, QIconViewItem * item, const QPoint & pos)
  This signal is emitted wher the user pressed with any mouse button on
  either and item (then \a item is the item under the mouse cursor) or
  somewhere else (then \a item is NULL). \a button is the number of the mouse button which
  the user pressed, and \a pos the position of the mouse cursor.
*/

/*!
  \fn void QIconView:mouseButtonClicked (int button, QIconViewItem * item, const QPoint & pos)
  This signal is emitted wher the user clicked (pressed + released) with any mouse button on
  either and item (then \a item is the item under the mouse cursor) or
  somewhere else (then \a item is NULL). \a button is the number of the mouse button which
  the user clicked, and \a pos the position of the mouse cursor.
*/

/*!
  Constructs an empty icon view
*/

QIconView::QIconView( QWidget *parent, const char *name, WFlags f )
    : QScrollView( parent, name, WNorthWestGravity | f )
{
    if ( !unknown_icon ) {
	QPixmap pix = QPixmap( unknown );
	unknown_icon = new QIconSet( pix );
    }

    d = new QIconViewPrivate;
    d->firstItem = 0;
    d->lastItem = 0;
    d->count = 0;
    d->mode = QIconSet::Automatic;
    d->mousePressed = FALSE;
    d->selectionMode = Single;
    d->currentItem = 0;
    d->highlightedItem = 0;
    d->rubber = 0;
    d->scrollTimer = 0;
    d->startDrag = FALSE;
    d->tmpCurrentItem = 0;
    d->rastX = d->rastY = -1;
    d->spacing = 5;
    d->cleared = FALSE;
    d->numSelectedItems = 0;
    d->alignMode = East;
    d->resizeMode = Fixed;
    d->dropped = FALSE;
    d->adjustTimer = new QTimer( this );
    d->firstAdjust = TRUE;
    d->isIconDrag = FALSE;
    d->iconDragData.clear();
    d->numDragItems = 0;
    d->updateTimer = new QTimer( this );
    d->cachedW = d->cachedH = 0;
    d->maxItemWidth = 200;
    d->maxItemTextLength = 255;
    d->inputTimer = new QTimer( this );
    d->currInputString = QString::null;
    d->dirty = FALSE;
    d->rearrangeEnabled = TRUE;
    d->itemTextPos = Bottom;
    d->reorderItemsWhenInsert = TRUE;
    d->singleClickMode = FALSE;
    d->singleClickConfig = QIconViewPrivate::SingleClickConfig();
    d->oldCursor = Qt::arrowCursor;
    d->singleClickSelectTimer = new QTimer( this );
    d->resortItemsWhenInsert = FALSE;
    d->sortOrder = TRUE;

    connect( d->adjustTimer, SIGNAL( timeout() ),
	     this, SLOT( adjustItems() ) );
    connect( d->updateTimer, SIGNAL( timeout() ),
	     this, SLOT( slotUpdate() ) );
    connect( d->inputTimer, SIGNAL( timeout() ),
	     this, SLOT( clearInputString() ) );
    connect( d->singleClickSelectTimer, SIGNAL( timeout() ),
	     this, SLOT( selectHighlightedItem() ) );

    setAcceptDrops( TRUE );
    viewport()->setAcceptDrops( TRUE );

    setMouseTracking( TRUE );
    viewport()->setMouseTracking( TRUE );

    viewport()->setBackgroundMode( NoBackground );
    viewport()->setFocusProxy( this );
    viewport()->setFocusPolicy( QWidget::WheelFocus );
}

/*!
  Destructs the iconview and deletes all items.
*/

QIconView::~QIconView()
{
    QIconViewItem *tmp, *item = d->firstItem;
    while ( item ) {
	tmp = item->next;
	delete item;
	item = tmp;
    }

    if ( d->scrollTimer )
	delete d->scrollTimer;
    if ( d->adjustTimer )
	delete d->adjustTimer;
    if ( d->updateTimer )
	delete d->updateTimer;
    if ( d->inputTimer )
	delete d->inputTimer;
    if ( d->singleClickSelectTimer )
	delete d->singleClickSelectTimer;

    clearSingleClickConfig();

    delete d;
}

/*!
  Inserts the iconview item \a item after \a after. If \a after is NULL,
  \a item is appended.

  You should never need to call this method yourself, you should rather do

  \code
    (void) new QIconViewItem( iconview, "This is the text of the item", QIconSet( pixmap ) );
  \endcode

  This does everything required for inserting an item.
*/

void QIconView::insertItem( QIconViewItem *item, QIconViewItem *after )
{
    if ( !item )
	return;

    if ( !d->firstItem ) {
	d->firstItem = d->lastItem = item;
	item->prev = 0;
	item->next = 0;
    } else {
	if ( !after || after == d->lastItem ) {
	    d->lastItem->next = item;
	    item->prev = d->lastItem;
	    d->lastItem = item;
	} else {
	    QIconViewItem *i = d->firstItem;
	    while ( i != after )
		i = i->next;

	    if ( i ) {
		QIconViewItem *next = i->next;
		item->next = next;
		item->prev = i;
		i->next = item;
		next->prev = item;
	    }
	}
    }

    if ( isVisible() ) {
	if ( d->reorderItemsWhenInsert ) {
	    if ( d->updateTimer->isActive() )
		d->updateTimer->stop();
	    // #### uncomment this ASA insertInGrid uses cached values and is efficient
	    //insertInGrid( item );
	
	    d->cachedW = QMAX( d->cachedW, item->x() + item->width() );
	    d->cachedH= QMAX( d->cachedH, item->y() + item->height() );

	    d->updateTimer->start( 100, TRUE );
	} else {
	    insertInGrid( item );
	}
    }

    d->count++;
    d->dirty = TRUE;
}

/*!
  Because of efficiency, the iconview is not redrawn immediately after
  inserting a new item, but with a very small delay using a QTimer. The
  result of this is, that if lots of items are inserted in a short time
  (e.g. in a loop), the iconview is not redrawn after each inserted item,
  but after inserting all of them, which makes the operation much faster.
*/

void QIconView::slotUpdate()
{
    d->updateTimer->stop();

    // #### remove that ASA insertInGrid uses cached values and is efficient
    if ( d->resortItemsWhenInsert )
	sortItems( d->sortOrder );
    else {
	int y = d->spacing;
	QIconViewItem *item = d->firstItem;
	int w = 0, h = 0;
	while ( item ) {
	    item = makeRowLayout( item, y );

	    if ( !item || !item->next )
		break;

	    w = QMAX( w, item->x() + item->width() );
	    h = QMAX( h, item->y() + item->height() );
	
	    item = item->next;
	}

	w = QMAX( QMAX( d->cachedW, w ), d->lastItem->x() + d->lastItem->width() );
	h = QMAX( QMAX( d->cachedH, h ), d->lastItem->y() + d->lastItem->height() );

	resizeContents( QMAX( contentsWidth(), w ),
			QMAX( contentsHeight(), h ) );
    }
    viewport()->repaint( FALSE );

    d->cachedW = d->cachedH = 0;
}

/*!
  Removes the iconview item \a item from the iconview. You should never
  need to call this method yourself, just delete an item to get rid of it. The
  destructor of QIconViewItem does everything, which is required for removing
  and item.
*/

void QIconView::removeItem( QIconViewItem *item )
{
    if ( !item )
	return;

    if ( item == d->firstItem )
	d->firstItem = d->firstItem->next;
    else if ( item == d->lastItem )
	d->lastItem = d->lastItem->prev;
    else {
	QIconViewItem *i = d->firstItem;
	while ( i != item )
	    i = i->next;

	if ( i ) {
	    i->prev->next = i->next;
	    i->next->prev = i->prev;
	}
    }

    d->count--;
}

/*!
  Returns the index of \a item or -1 if there is something wrong.
*/

int QIconView::index( const QIconViewItem *item ) const
{
    if ( !item )
	return -1;

    if ( item == d->firstItem )
	return 0;
    else if ( item == d->lastItem )
	return d->count - 1;
    else {
	QIconViewItem *i = d->firstItem;
	unsigned int j = 0;
	while ( i != item ) {
	    i = i->next;
	    ++j;
	}

	return j;
    }
}

/*!
  Returns the selected item. This only works in single selection mode.
*/

QIconViewItem *QIconView::selectedItem() const
{
    if ( selectionMode() != Single )
	return 0;

    QIconViewItem* item = firstItem();
    for( ; item; item = item->nextItem() )
	if ( item->isSelected() )
	    return item;

    return 0;
}

/*!
  Returns a pointer to the first item fo the iconview, or NULL, if there
  are no items in the iconview.
*/

QIconViewItem *QIconView::firstItem() const
{
    return d->firstItem;
}

/*!
  Returns a pointer to the last item fo the iconview, or NULL, if there
  are no items in the iconview.
*/

QIconViewItem *QIconView::lastItem() const
{
    return d->lastItem;
}

/*!
  Returns a pointer to the current item fo the iconview, or NULL, if no
  item is current.
*/

QIconViewItem *QIconView::currentItem() const
{
    return d->currentItem;
}

/*!
  Makes \a item the new current item of the iconview.
*/

void QIconView::setCurrentItem( QIconViewItem *item )
{
    d->currentItem = item;
    emit currentChanged();
    emit currentChanged( d->currentItem );
    emitNewSelectionNumber();
}

/*!
  Returns the number of inserted items.
*/

unsigned int QIconView::count() const
{
    return d->count;
}

/*!
  Sets all icons of the iconview to the viewmode \a mode. This can be
  QIconSet::Automatic, QIconSet::Small, QIconSet::Large.
  QIconSet::Automatic is default.
*/

void QIconView::setViewMode( QIconSet::Size mode )
{
    d->mode = mode;

    if ( !d->firstItem )
	return;

    QIconViewItem *item = d->firstItem;
    for ( ; item; item = item->next )
	item->setViewMode( mode );

    orderItemsInGrid();
}

/*!
  Returns the viewmode of the iconview.
*/

QIconSet::Size QIconView::viewMode() const
{
    return d->mode;
}

/*!
  Does autoscrolling when selecting multiple icons with the rubber band.
*/

void QIconView::doAutoScroll()
{
    QPoint pos = QCursor::pos();
    pos = viewport()->mapFromGlobal( pos );
    pos = viewportToContents( pos );

    QPainter p;
    p.begin( viewport() );
    p.setRasterOp( NotROP );
    p.setPen( QPen( Qt::black, 1 ) );
    p.setBrush( Qt::NoBrush );
    drawRubber( &p );
    p.end();

    ensureVisible( pos.x(), pos.y() );

    pos = QCursor::pos();
    pos = viewport()->mapFromGlobal( pos );
    pos = viewportToContents( pos );

    QRect oldRubber = QRect( *d->rubber );

    d->rubber->setRight( pos.x() );
    d->rubber->setBottom( pos.y() );

    selectByRubber( oldRubber );

    p.begin( viewport() );
    p.setRasterOp( NotROP );
    p.setPen( QPen( Qt::black, 1 ) );
    p.setBrush( Qt::NoBrush );
    drawRubber( &p );

    p.end();

    pos = QCursor::pos();
    pos = mapFromGlobal( pos );

    if ( !QRect( 0, 0, viewport()->width(), viewport()->height() ).contains( pos ) &&
	 !d->scrollTimer ) {
	d->scrollTimer = new QTimer( this );

	connect( d->scrollTimer, SIGNAL( timeout() ),
		 this, SLOT( doAutoScroll() ) );
	d->scrollTimer->start( 100, FALSE );
    } else if ( QRect( 0, 0, viewport()->width(), viewport()->height() ).contains( pos ) &&
		d->scrollTimer ) {
	disconnect( d->scrollTimer, SIGNAL( timeout() ),
		    this, SLOT( doAutoScroll() ) );
	d->scrollTimer->stop();
	delete d->scrollTimer;
	d->scrollTimer = 0;
    }

}

/*!
  \reimp
*/

void QIconView::drawContents( QPainter *p, int cx, int cy, int cw, int ch )
{
    p->save();
    p->resetXForm();
    QRect r( contentsToViewport( QPoint( cx, cy ) ), QSize( cw, ch ) );
    p->setClipRect( r );
    drawBackground( p, r );
    p->restore();

    if ( !d->firstItem )
	return;

    QIconViewItem *item = d->firstItem;
    for ( ; item; item = item->next )
	if ( item->rect().intersects( QRect( cx, cy, cw, ch ) ) && !item->dirty )
	    item->paintItem( p );

    if ( ( hasFocus() || viewport()->hasFocus() ) && d->currentItem &&
	 d->currentItem->rect().intersects( QRect( cx, cy, cw, ch ) ) )
	d->currentItem->paintFocus( p );
}

/*!
  Orders all items in the grid.
*/

void QIconView::orderItemsInGrid()
{
    if ( !d->firstItem || !d->lastItem )
	return;

    int w = 0, h = 0, y = d->spacing;

    QIconViewItem *item = d->firstItem;
    while ( item ) {
	item = makeRowLayout( item, y );
	w = QMAX( w, item->x() + item->width() );
	h = QMAX( h, item->y() + item->height() );

	if ( !item || !item->next )
	    break;

	item = item->next;
    }
    resizeContents( w, h );
    d->dirty = FALSE;
}

/*!
  Aligns all items in the \a grid;
*/

void QIconView::alignItemsInGrid( const QSize &grid )
{
    int w = 0, h = 0;
    QIconViewItem *item = d->firstItem;
    for ( ; item; item = item->next ) {
	int nx = item->x() / grid.width();
	int ny = item->y() / grid.height();
	item->move( nx * grid.width(), // + ( grid.width() - item->width() / 2 ),
		    ny * grid.height() );// + ( grid.height() - item->height() / 2 ) );
	w = QMAX( w, item->x() + item->width() );
	h = QMAX( h, item->y() + item->height() );
	item->dirty = FALSE;
    }

    resizeContents( w, h );
}

/*!
  Aligns the items in the best fitting grid.
*/

void QIconView::alignItemsInGrid()
{
    int w = 0, h = 0;
    QIconViewItem *item = d->firstItem;
    for ( ; item; item = item->next ) {
	w = QMAX( w, item->width() );
	h = QMAX( h, item->height() );
    }

    alignItemsInGrid( QSize( QMAX( d->rastX + d->spacing, w ),
			     QMAX( d->rastY + d->spacing, h ) ) );
}

/*!
  \reimp
*/

void QIconView::showEvent( QShowEvent * )
{
    resizeContents( viewport()->width(), viewport()->height() );
    if ( d->dirty )
	orderItemsInGrid();
    QScrollView::show();
}

/*!
  Sets the selection mode of the iconview to \a m. This can be
  <li>Single (only one item can be selected)
  <li>Multi (multiple items can be selected)
  <li>StrictMulti (multiple items can be selected, but only if the user pressed
  CTRL while selecting them)
*/

void QIconView::setSelectionMode( SelectionMode m )
{
    d->selectionMode = m;
}

/*!
  Returns the selection mode of the iconview.
*/

QIconView::SelectionMode QIconView::selectionMode() const
{
    return d->selectionMode;
}

/*!
  Sets the configuration for the single click mode. \a normalText
  is the font and \a normalTextCol the color which should be used
  for the normal icon text. \a highlightedText is the font and
  \a highlightedTextCol the color which should be used for the icon
  text, when the icon is highlighted (means, when the mouse cursor is
  over the item). \a highlightedCursor is the mouse cursor which should
  be used when the mouse is over an item. Passing NULL for one of these
  parameters lets the iconview use the default value.
  \a setCurrentInterval specifies the time interval (in ms) after which
  a highlighted item (item over which the mouse is) gets the current
  item and gets selected. Passing -1 here disables this feature.

  The QIconView takes the ownership of the passed pointers. This means
  you must not delete them! The QIconView cares about deleting them.
*/

void QIconView::setSingleClickConfiguration( QFont *normalText, QColor *normalTextCol,
					     QFont *highlightedText, QColor *highlightedTextCol,
					     QCursor *highlightedCursor, int setCurrentInterval )
{
    clearSingleClickConfig();

    d->singleClickConfig.normalText = normalText;
    d->singleClickConfig.normalTextCol = normalTextCol;
    d->singleClickConfig.highlightedText = highlightedText;
    d->singleClickConfig.highlightedTextCol = highlightedTextCol;
    d->singleClickConfig.highlightedCursor = highlightedCursor;
    d->singleClickConfig.setCurrentInterval = setCurrentInterval;
}

/*!
  This methode gives back the current single click configuration
  using the parameters.

  \sa QIconView::setSingleClickConfiguration(), QIconView::setUseSingleClickMode()
*/

void QIconView::singleClickConfiguration( QFont *normalText, QColor *normalTextCol,
					  QFont *highlightedText, QColor *highlightedTextCol,
					  QCursor *highlightedCursor, int &setCurrentInterval ) const
{
    normalText = d->singleClickConfig.normalText;
    normalTextCol = d->singleClickConfig.normalTextCol;
    highlightedText = d->singleClickConfig.highlightedText;
    highlightedTextCol = d->singleClickConfig.highlightedTextCol;
    highlightedCursor = d->singleClickConfig.highlightedCursor;
    setCurrentInterval = d->singleClickConfig.setCurrentInterval;
}

/*!
  If \a b is TRUE, the iconview is switched to single click mode. This means
  it behaves quite like a web page. E.g. a single click on an item lets the
  iconview emit a doubleClicked signal, and the items behave a bit like links
  (can be configures in QIconView::setSingleClickConfiguration()).
  If \b is FALSE, the normal mode of the iconview is used, with double clicks
  and so on.

  If you use the QIconView you don't have to care about the difference of single
  click or double click modes. For you this is transparent (the QIconView translates
  the important signals and evets).
  So you can offer the user to switch between single and doubleclick mode, but
  you don't need to care about the handling of that at all.

  \sa QIconView::setSingleClickConfiguration()
*/

void QIconView::setUseSingleClickMode( bool b )
{
    d->singleClickMode = b;
}

/*!
  Returns TRUE if the iconview is in single click mode, or
  FALSE if it is in double click mode.

  \sa QIconView::setUseSingleClickMode(), QIconView::setSingleClickConfiguration()
*/

bool QIconView::useSingleClickMode() const
{
    return d->singleClickMode;
}

/*!
  Returns a pointer to the item which contains \a pos, which is given
  on contents coordinates.
*/

QIconViewItem *QIconView::findItem( const QPoint &pos ) const
{
    if ( !d->firstItem )
	return 0;

    QIconViewItem *item = d->firstItem;
    for ( ; item; item = item->next )
	if ( item->contains( pos ) )
	    return item;

    return 0;
}

/*!
  Returns a pointer to the first item which could be found that contains
  \a text, or 0 if no such item could be found.
*/

QIconViewItem *QIconView::findItem( const QString &text ) const
{
    if ( !d->firstItem )
	return 0;

    QIconViewItem *item = d->firstItem;
    for ( ; item; item = item->next )
	if ( item->text().left( text.length() ) == text )
	    return item;

    return 0;
}

/*!
  If \a select is TRUE, all items are selected, else all are unselected.
*/

void QIconView::selectAll( bool select )
{
    QIconViewItem *item = d->firstItem;
    for ( ; item; item = item->next )
	item->setSelected( select );
}

/*!
  Repaints the \a item.
*/

void QIconView::repaintItem( QIconViewItem *item )
{
    if ( !item || item->dirty )
	return;

    if ( QRect( contentsX(), contentsY(), contentsWidth(), contentsHeight() ).
	 intersects( QRect( item->x() - 1, item->y() - 1, item->width() + 2, item->height() + 2 ) ) )
	repaintContents( item->x() - 1, item->y() - 1, item->width() + 2, item->height() + 2, FALSE );
}

/*!
  Makes sure, that \a item is visible, and scrolls the view if required.
*/

void QIconView::ensureItemVisible( QIconViewItem *item )
{
    if ( !item )
	return;

    ensureVisible( item->x(), item->y(),
		   item->width(), item->height() );
}

/*!
  Finds the first item which is visible on the viewport. If no items is
  visible at all, 0 is returned.
*/

QIconViewItem* QIconView::findFirstVisibleItem() const
{
    QRect r( contentsX(), contentsY(), visibleWidth(), visibleHeight() );
    QIconViewItem *item = d->firstItem, *i = 0;
    for ( ; item; item = item->next ) {
	if ( r.intersects( item->rect() ) ) {
	    if ( !i )
		i = item;
	    else {
		QRect r2 = item->rect();
		QRect r3 = i->rect();
		if ( r2.y() < r3.y() )
		    i = item;
		else if ( r2.y() == r3.y() &&
			  r2.x() < r3.x() )
		    i = item;
	    }
	}
    }

    return i;
}

/*!
  Cleares the iconview.
*/

void QIconView::clear()
{
    resizeContents( visibleWidth() - verticalScrollBar()->width(),
		    visibleHeight() - horizontalScrollBar()->width() );
    setContentsPos( 0, 0 );

    if ( !d->firstItem )
	return;

    QIconViewItem *item = d->firstItem, *tmp;
    while ( item ) {
	tmp = item->next;
	delete item;
	item = tmp;
    }

    d->count = 0;
    d->firstItem = 0;
    d->lastItem = 0;
    setCurrentItem( 0 );
    d->highlightedItem = 0;
    d->tmpCurrentItem = 0;
    d->drawDragShade = FALSE;

    viewport()->repaint( FALSE );

    d->cleared = TRUE;
}

/*!
  Sets the horizontal grid to \a rx.  If \a rx is -1, there is no
  horizontal grid used for aligning items.
*/

void QIconView::setGridX( int rx )
{
    d->rastX = rx;
}

/*!
  Sets the vertical grid to \a ry.  If \a ry is -1, there is no
  vertical grid used for aligning items.
*/

void QIconView::setGridY( int ry )
{
    d->rastY = ry;
}

/*!
  Returns the horizonal grid.

  \sa setGridX
*/

int QIconView::gridX() const
{
    return d->rastX;
}

/*!
  Returns the vertica grid.

  \sa setGridY
*/

int QIconView::gridY() const
{
    return d->rastY;
}

/*!
  Sets the space between iconview items to \a sp.
*/

void QIconView::setSpacing( int sp )
{
    d->spacing = sp;
}

/*!
  Returns the spacing between iconview items.
*/

int QIconView::spacing() const
{
    return d->spacing;
}

/*!
  Sets the position, where the text of the items is drawn. This can be Bottom
  or Right.
*/

void QIconView::setItemTextPos( ItemTextPos pos )
{
    if ( pos == d->itemTextPos )
	return;

    d->itemTextPos = pos;

    QIconViewItem *item = d->firstItem;
    for ( ; item; item = item->next )
	item->calcRect();

    orderItemsInGrid();
    repaintContents( contentsX(), contentsY(), contentsWidth(), contentsHeight(), FALSE );
}

/*!
  Returns the position, at which the text of the items are drawn.
*/

QIconView::ItemTextPos QIconView::itemTextPos() const
{
    return d->itemTextPos;
}

/*!
  Sets the alignment mode of the iconview to \a am. This can be
  <li> East (Items, which don't fit onto the view, go further down (you get a
  vertical scrollbar)
  <li> South (Items, which don't fit onto the view, go further right (you get a
  horizontal scrollbar)
*/

void QIconView::setAlignMode( AlignMode am )
{
    if ( d->alignMode == am )
	return;

//     if ( d->resizeMode != Adjust ) {
// 	setHScrollBarMode( Auto );
// 	setVScrollBarMode( Auto );
//     } else {
// 	if ( d->alignMode == East ) {
// 	    setHScrollBarMode( AlwaysOff );
// 	    setVScrollBarMode( Auto );
// 	} else {
// 	    setVScrollBarMode( AlwaysOff );
// 	    setHScrollBarMode( Auto );
// 	}		
//     }

    d->alignMode = am;
    resizeContents( viewport()->width(), viewport()->height() );
    orderItemsInGrid();
}

/*!
  Returns the alignment mode of the iconview.
*/

QIconView::AlignMode QIconView::alignMode() const
{
    return d->alignMode;
}

/*!
  Sets the resize mode of the iconview. This can be
  <li> Fixed (when resizing the view, the items stay as they are.
  <li> Adjust (when resizing the view, the items are reordered to
  fit as good as possible into the view.
*/

void QIconView::setResizeMode( ResizeMode rm )
{
    if ( d->resizeMode == rm )
	return;

    d->resizeMode = rm;

//     if ( d->resizeMode != Adjust ) {
// 	setHScrollBarMode( Auto );
// 	setVScrollBarMode( Auto );
//     } else {
// 	if ( d->alignMode == East ) {
// 	    setHScrollBarMode( AlwaysOff );
// 	    setVScrollBarMode( Auto );
// 	} else {
// 	    setVScrollBarMode( AlwaysOff );
// 	    setHScrollBarMode( Auto );
// 	}		
//     }
}

/*!
  Returns the resizemode of the iconview.
*/

QIconView::ResizeMode QIconView::resizeMode() const
{
    return d->resizeMode;
}

/*!
  Sets the maximum width, which an item may have. If a raster X is set,
  this value is ignoredm, and the raster X value is used.
*/

void QIconView::setMaxItemWidth( int w )
{
    d->maxItemWidth = w;
}

/*!
  Sets the maximum length (in characters), which an item text may have.
*/

void QIconView::setMaxItemTextLength( int w )
{
    d->maxItemTextLength = w;
}

/*!
  Returns the maximum width, which an item may have.
*/

int QIconView::maxItemWidth() const
{
    if ( d->rastX != -1 )
	return d->rastX - 2;
    else
	return d->maxItemWidth;
}

/*!
  Returns the maximum length (in characters), which the
  text of an icon may have.
*/

int QIconView::maxItemTextLength() const
{
    return d->maxItemTextLength;
}

/*!
  If \a b is TRUE, the user is allowed to rearrange icons in the iconview
  (moving them with the mouse),
  if \a b is FALSE, the user is not allowed to do that.
*/

void QIconView::setRearrangeEnabled( bool b )
{
    d->rearrangeEnabled = b;
}

/*!
  Returns TRUE, if the user is allowed to rearrange icons in the iconview,
  else FALSE;
*/

bool QIconView::rearrangeEnabled() const
{
    return d->rearrangeEnabled;
}

/*!
  If \a b is TRUE, all items are reordered in the grid if a new one is
  inserted. Else, the best fitting place for the new item is searched and
  the other ones are not touched.

  This setting only applies if the iconview is visible. If you insert
  items and the iconview is not visible, the icons are reordered whe it
  gets visible.
*/

void QIconView::setReorderItemsWhenInsert( bool b )
{
    d->reorderItemsWhenInsert = b;
}

/*!
  Returns TRUE if all items are reordered in the grid if a new one is
  inserted, else FALSE.
*/

bool QIconView::reorderItemsWhenInsert() const
{
    return d->reorderItemsWhenInsert;
}

/*!
  If \a sort is TRUE, new items are inserted sorted. The sort
  direction is specified using \a ascending.

  Inserting items sorted only works when reordering items is
  set to TRUE as well.

  \sa QIconView::setReorderItemsWhenInsert(), QIconView::reorderItemsWhenInsert()
*/

void QIconView::setResortItemsWhenInsert( bool sort, bool ascending )
{
    d->resortItemsWhenInsert = sort;
    d->sortOrder = ascending;
}

/*!
  Returns TRUE if new items are inserted sorted, else FALSE.

  \sa QIconView::setResortItemsWhenInsert()
*/

bool QIconView::resortItemsWhenInsert() const
{
    return d->resortItemsWhenInsert;
}

/*!
  Returns TRUE if the sortorder for inserting new items is ascending,
  FALSE means descending. This sortorder has only a meaning if resorting
  and reordering of new inserted items is enabled.

  \sa QIconView::setResortItemsWhenInsert(), QIconView::setReorderItemsWhenInsert()
*/

bool QIconView::sortOrder() const
{
    return d->sortOrder;
}

/*!
  \reimp
*/

void QIconView::contentsMousePressEvent( QMouseEvent *e )
{
    QIconViewItem *item = findItem( e->pos() );
    emit mouseButtonPressed( e->button(), item, e->globalPos() );

    if ( d->currentItem )
	d->currentItem->renameItem();

    if ( e->button() == LeftButton ) {
	d->startDrag = FALSE;

	QIconViewItem *oldCurrent = d->currentItem;
	
	if ( item && item->isSelected() &&
	     item->textRect( FALSE ).contains( e->pos() ) ) {

	    if ( !item->renameEnabled() )
		return;

	    ensureItemVisible( item );
	    setCurrentItem( item );
	    repaintItem( item );
	    item->rename();
	    return;
	}

	if ( item && item->isSelectable() )
	    if ( d->selectionMode == Single )
		item->setSelected( TRUE, e->state() & ControlButton );
	    else
		item->setSelected( !item->isSelected(), e->state() & ControlButton );
	else if ( d->selectionMode == Single || !( e->state() & ControlButton ) )
	    selectAll( FALSE );

	setCurrentItem( item );

	repaintItem( oldCurrent );
	repaintItem( d->currentItem );

	if ( !d->currentItem && d->selectionMode != Single ) {
	    if ( d->rubber )
		delete d->rubber;
	    d->rubber = 0;
	    d->rubber = new QRect( e->x(), e->y(), 0, 0 );

	    if ( d->selectionMode == StrictMulti && !( e->state() & ControlButton ) )
		selectAll( FALSE );
	}

	d->mousePressed = TRUE;
    } else if ( e->button() == RightButton ) {
	emit rightButtonPressed( item, e->globalPos() );
	if ( item )
	    emit itemRightPressed( item );
	else
	    emit viewportRightPressed();
    } else if ( e->button() == MidButton ) {
	emit mouseButtonPressed( 1, item, e->globalPos() );
    }
}

/*!
  \reimp
*/

void QIconView::contentsMouseReleaseEvent( QMouseEvent *e )
{
    QIconViewItem *item = findItem( e->pos() );
    emit mouseButtonClicked( e->button(), item, e->globalPos() );

    d->mousePressed = FALSE;
    d->startDrag = FALSE;

    if ( d->rubber ) {
	QPainter p;
	p.begin( viewport() );
	p.setRasterOp( NotROP );
	p.setPen( QPen( Qt::black, 1 ) );
	p.setBrush( Qt::NoBrush );

	drawRubber( &p );

	p.end();

	delete d->rubber;
	d->rubber = 0;
    } else if ( !d->startDrag && d->singleClickMode ) {
	if ( item && !item->renameBox ) {
	    selectAll( FALSE );
	    item->setSelected( TRUE, TRUE );
	    emit doubleClicked( item );
	}
    }

    if ( d->scrollTimer ) {
	disconnect( d->scrollTimer, SIGNAL( timeout() ), this, SLOT( doAutoScroll() ) );
	d->scrollTimer->stop();
	delete d->scrollTimer;
	d->scrollTimer = 0;
    }

    if ( e->button() == RightButton ) {
	emit rightButtonClicked( item, e->globalPos() );
	if ( item )
	    emit itemRightClicked( item );
	else
	    emit viewportRightClicked();
    }
}

/*!
  \reimp
*/

void QIconView::contentsMouseMoveEvent( QMouseEvent *e )
{
    if ( d->singleClickMode ) {
	if ( ( d->currentItem && d->currentItem->renameBox ) ||
	     d->rubber ) {
	    if ( d->singleClickSelectTimer->isActive() )
		d->singleClickSelectTimer->stop();
	} else {
	    QIconViewItem *item = findItem( e->pos() );
	    if ( item ) {
		if ( item != d->highlightedItem ) {
		    if ( d->singleClickSelectTimer->isActive() )
			d->singleClickSelectTimer->stop();

		    QIconViewItem *old = d->highlightedItem;
		    d->highlightedItem = item;
		    emit onItem( item );
		    if ( d->singleClickConfig.highlightedCursor )
			viewport()->setCursor( *d->singleClickConfig.highlightedCursor );
		    repaintItem( old );
		    repaintItem( d->highlightedItem );
		    if ( d->singleClickConfig.setCurrentInterval >= 0 )
			d->singleClickSelectTimer->start( d->singleClickConfig.setCurrentInterval, TRUE );
		}
	    } else if ( d->highlightedItem ) {
		if ( d->singleClickSelectTimer->isActive() )
		    d->singleClickSelectTimer->stop();

		viewport()->setCursor( d->oldCursor );
		QIconViewItem *old = d->highlightedItem;
		d->highlightedItem = 0;
		repaintItem( old );
		emit onViewport();
	    }
	}
    } else {
	QIconViewItem *item = findItem( e->pos() );
	if ( d->highlightedItem != item ) {
	    if ( item )
		emit onItem( item );
	    else
		emit onViewport();
	    d->highlightedItem = item;
	}
    }



    if ( d->mousePressed && d->currentItem && d->currentItem->dragEnabled() ) {
	if ( !d->startDrag ) {
	    d->currentItem->setSelected( TRUE, TRUE );
	    d->startDrag = TRUE;
	}
	else {
	    d->mousePressed = FALSE;
	    d->cleared = FALSE;
	    startDrag();
	    if ( d->tmpCurrentItem )
		repaintItem( d->tmpCurrentItem );
	}
    } else if ( d->mousePressed && !d->currentItem && d->rubber )
	doAutoScroll();
}

/*!
  \reimp
*/

void QIconView::contentsMouseDoubleClickEvent( QMouseEvent *e )
{
    QIconViewItem *item = findItem( e->pos() );
    if ( item ) {
	selectAll( FALSE );
	item->setSelected( TRUE, TRUE );
	emit doubleClicked( item );
    }
}

/*!
  \reimp
*/

void QIconView::contentsDragEnterEvent( QDragEnterEvent *e )
{
    d->drawDragShade = TRUE;
    d->tmpCurrentItem = 0;
    initDrag( e );
    d->oldDragPos = e->pos();
    drawDragShade( e->pos() );
    d->dropped = FALSE;
}

/*!
  \reimp
*/

void QIconView::contentsDragMoveEvent( QDragMoveEvent *e )
{
    drawDragShade( d->oldDragPos );

    QIconViewItem *old = d->tmpCurrentItem;
    d->tmpCurrentItem = 0;

    QIconViewItem *item = findItem( e->pos() );

    if ( item ) {
	if ( item != old ) {
	    if ( old ) {
		old->dragLeft();
		repaintItem( old );
	    }
	    item->dragEntered();
	}

	if ( item->acceptDrop( e ) )
	    e->acceptAction();
	else
	    e->ignore();

	d->tmpCurrentItem = item;
	if ( d->tmpCurrentItem != old ) {
	    QPainter p;
	    p.begin( viewport() );
	    p.translate( -contentsX(), -contentsY() );
	    item->paintFocus( &p );
	    p.end();
	}
    } else {
	e->acceptAction();
	if ( old ) {
	    old->dragLeft();
	    repaintItem( old );
	}
    }

    d->oldDragPos = e->pos();
    drawDragShade( e->pos() );
}

/*!
  \reimp
*/

void QIconView::contentsDragLeaveEvent( QDragLeaveEvent * )
{
    if ( !d->dropped )
	drawDragShade( d->oldDragPos );

    if ( d->tmpCurrentItem ) {
	repaintItem( d->tmpCurrentItem );
	d->tmpCurrentItem->dragLeft();
    }

    d->tmpCurrentItem = 0;
    d->isIconDrag = FALSE;
    d->iconDragData.clear();
}

/*!
  \reimp
*/

void QIconView::contentsDropEvent( QDropEvent *e )
{
    d->dropped = TRUE;
    drawDragShade( d->oldDragPos );

    if ( d->tmpCurrentItem )
	repaintItem( d->tmpCurrentItem );

    QIconViewItem *i = findItem( e->pos() );

    if ( !i && e->source() == viewport() && d->currentItem && !d->cleared ) {
	if ( !d->rearrangeEnabled )
	    return;
	QRect r = d->currentItem->rect();

	d->currentItem->move( e->pos() - d->dragStart );

	int w = d->currentItem->x() + d->currentItem->width() + 1;
	int h = d->currentItem->y() + d->currentItem->height() + 1;

	repaintItem( d->currentItem );
	repaintContents( r.x(), r.y(), r.width(), r.height(), FALSE );

	if ( d->selectionMode != Single ) {
	    int dx = d->currentItem->x() - r.x();
	    int dy = d->currentItem->y() - r.y();

	    QIconViewItem *item = d->firstItem;
	    for ( ; item; item = item->next )
		if ( item->isSelected() && item != d->currentItem ) {
		    QRect pr = item->rect();
		    item->moveBy( dx, dy );
		    repaintItem( item );
		    repaintContents( pr.x(), pr.y(), pr.width(), pr.height(), FALSE );
		    w = QMAX( w, item->x() + item->width() + 1 );
		    h = QMAX( h, item->y() + item->height() + 1 );
		}
	}
	bool fullRepaint = FALSE;
	if ( w > contentsWidth() ||
	     h > contentsHeight() )
	    fullRepaint = TRUE;

	int oldw = contentsWidth();
	int oldh = contentsHeight();

	resizeContents( QMAX( contentsWidth(), w ), QMAX( contentsHeight(), h ) );

	if ( fullRepaint ) {
	    repaintContents( oldw, 0, contentsWidth() - oldw, contentsHeight(), FALSE );
	    repaintContents( 0, oldh, contentsWidth(), contentsHeight() - oldh, FALSE );
	}
	e->acceptAction();
    } else if ( !i && e->source() != viewport() || d->cleared )
	emit dropped( e );
    else if ( i )
	i->dropped( e );
}

/*!
  \reimp
*/

void QIconView::resizeEvent( QResizeEvent* e )
{
    if ( d->resizeMode == Adjust )
	d->oldSize = e->oldSize();
    QScrollView::resizeEvent( e );
    if ( d->resizeMode == Adjust ) {
	if ( d->adjustTimer->isActive() )
	    d->adjustTimer->stop();
	d->adjustTimer->start( 400, TRUE );
    }
}

/*!
  Adjusts the positions of the items to the geometry of the iconview.
*/

void QIconView::adjustItems()
{
    d->adjustTimer->stop();
    if ( d->resizeMode == Adjust ) {
	if ( size() != d->oldSize ) {
 	    if ( d->firstAdjust ) {
 		d->firstAdjust = FALSE;
 		return;
 	    }
	    orderItemsInGrid();
	    viewport()->repaint( FALSE );
	}
    }
}

/*!
  \reimp
*/

void QIconView::keyPressEvent( QKeyEvent *e )
{
    if ( !d->firstItem )
	return;

    if ( !d->currentItem ) {
	if ( !e->text().isEmpty() && e->text()[ 0 ].isPrint() ) {
	    findItemByName( e->text() );
	    return;
	}
	
	if ( e->key() == Key_Control )
	    return;

	setCurrentItem( d->firstItem );

	if ( d->selectionMode == Single )
	    d->currentItem->setSelected( TRUE, TRUE );

	repaintItem( d->currentItem );
	return;
    }

    switch ( e->key() )
    {
    case Key_Home: {
	if ( !d->firstItem )
	    return;

	QIconViewItem *item = d->currentItem;
	setCurrentItem( d->firstItem );

	if ( d->selectionMode == Single ) {
	    item->setSelected( FALSE );
	    d->currentItem->setSelected( TRUE, TRUE );
	} else {
	    if ( e->state() & ShiftButton )
		d->currentItem->setSelected( !d->currentItem->isSelected(), TRUE );
	}

	repaintItem( item );
	repaintItem( d->currentItem );
    } break;
    case Key_End: {
	if ( !d->lastItem )
	    return;

	QIconViewItem *item = d->currentItem;
	setCurrentItem( d->lastItem );

	if ( d->selectionMode == Single ) {
	    item->setSelected( FALSE );
	    d->currentItem->setSelected( TRUE, TRUE );
	} else {
	    if ( e->state() & ShiftButton )
		d->currentItem->setSelected( !d->currentItem->isSelected(), TRUE );
	}

	repaintItem( item );
	repaintItem( d->currentItem );
    } break;
    case Key_Right: {	
	QIconViewItem *item;
	if ( d->alignMode == East ) {
	    if ( !d->currentItem->next )
		return;

	    item = d->currentItem;
	    setCurrentItem( d->currentItem->next );

	    if ( d->selectionMode == Single ) {
		item->setSelected( FALSE );
		d->currentItem->setSelected( TRUE, TRUE );
	    } else {
		if ( e->state() & ShiftButton )
		    d->currentItem->setSelected( !d->currentItem->isSelected(), TRUE );
	    }
	} else {
	    item = d->firstItem;
	    QRect r( 0, d->currentItem->y(), contentsWidth(), d->currentItem->height() );
	    for ( ; item; item = item->next ) {
		if ( item->x() > d->currentItem->x() && r.intersects( item->rect() ) ) {
		    QRect ir = r.intersect( item->rect() );
		    if ( item->next && r.intersects( item->next->rect() ) ) {
			QRect irn = r.intersect( item->next->rect() );
			if ( irn.height() > ir.height() )
			    item = item->next;
		    }
		    QIconViewItem *i = d->currentItem;
		    d->currentItem = item;
		    item = i;
		    if ( d->selectionMode == Single ) {
			i->setSelected( FALSE );
			d->currentItem->setSelected( TRUE, TRUE );
		    } else {
			if ( e->state() & ShiftButton )
			d->currentItem->setSelected( !d->currentItem->isSelected(), TRUE );
		    }
		    break;
		}
	    }
	}
	
	repaintItem( item );
	repaintItem( d->currentItem );
    } break;
    case Key_Left: {
	QIconViewItem *item;
	if ( d->alignMode == East ) {
	    if ( !d->currentItem->prev )
		return;

	    item = d->currentItem;
	    setCurrentItem( d->currentItem->prev );

	    if ( d->selectionMode == Single ) {
		item->setSelected( FALSE );
		d->currentItem->setSelected( TRUE, TRUE );
	    } else {
		if ( e->state() & ShiftButton )
		    d->currentItem->setSelected( !d->currentItem->isSelected(), TRUE );
	    }
	} else {
	    item = d->lastItem;
	    QRect r( 0, d->currentItem->y(), contentsWidth(), d->currentItem->height() );
	    for ( ; item; item = item->prev ) {
		if ( item->x() < d->currentItem->x() && r.intersects( item->rect() ) ) {
		    QRect ir = r.intersect( item->rect() );
		    if ( item->prev && r.intersects( item->prev->rect() ) ) {
			QRect irn = r.intersect( item->prev->rect() );
			if ( irn.height() > ir.height() )
			    item = item->prev;
		    }
		    QIconViewItem *i = d->currentItem;
		    d->currentItem = item;
		    item = i;
		    if ( d->selectionMode == Single ) {
			i->setSelected( FALSE );
			d->currentItem->setSelected( TRUE, TRUE );
		    } else {
			if ( e->state() & ShiftButton )
			    d->currentItem->setSelected( !d->currentItem->isSelected(), TRUE );
		    }
		    break;
		}
	    }
	}
	
	repaintItem( item );
	repaintItem( d->currentItem );
    } break;
    case Key_Space: {
	if ( d->selectionMode == Single )
	    return;

	d->currentItem->setSelected( !d->currentItem->isSelected(), TRUE );
    } break;
    case Key_Enter: case Key_Return:
	emit doubleClicked( d->currentItem );
	break;
    case Key_Down: {
	QIconViewItem *item;
	
	if ( d->alignMode == East ) {
	    item = d->firstItem;
	    QRect r( d->currentItem->x(), 0, d->currentItem->width(), contentsHeight() );
	    for ( ; item; item = item->next ) {
		if ( item->y() > d->currentItem->y() && r.intersects( item->rect() ) ) {
		    QRect ir = r.intersect( item->rect() );
		    if ( item->next && r.intersects( item->next->rect() ) ) {
			QRect irn = r.intersect( item->next->rect() );
			if ( irn.width() > ir.width() )
			    item = item->next;
		    }
		    QIconViewItem *i = d->currentItem;
		    d->currentItem = item;
		    item = i;
		    if ( d->selectionMode == Single ) {
			i->setSelected( FALSE );
			d->currentItem->setSelected( TRUE, TRUE );
		    } else {
			if ( e->state() & ShiftButton )
			    d->currentItem->setSelected( !d->currentItem->isSelected(), TRUE );
		    }
		    break;
		}
	    }
	} else {
	    if ( !d->currentItem->next )
		return;

	    item = d->currentItem;
	    setCurrentItem( d->currentItem->next );

	    if ( d->selectionMode == Single ) {
		item->setSelected( FALSE );
		d->currentItem->setSelected( TRUE, TRUE );
	    } else {
		if ( e->state() & ShiftButton )
		    d->currentItem->setSelected( !d->currentItem->isSelected(), TRUE );
	    }
	}
	repaintItem( item );
	repaintItem( d->currentItem );
    } break;
    case Key_Up: {
	QIconViewItem *item;
	
	if ( d->alignMode == East ) {
	    item = d->lastItem;
	    QRect r( d->currentItem->x(), 0, d->currentItem->width(), contentsHeight() );
	    for ( ; item; item = item->prev ) {
		if ( item->y() < d->currentItem->y() && r.intersects( item->rect() ) ) {
		    QRect ir = r.intersect( item->rect() );
		    if ( item->prev && r.intersects( item->prev->rect() ) ) {
			QRect irn = r.intersect( item->prev->rect() );
			if ( irn.width() > ir.width() )
			    item = item->prev;
		    }
		    QIconViewItem *i = d->currentItem;
		    d->currentItem = item;
		    item = i;
		    if ( d->selectionMode == Single ) {
			i->setSelected( FALSE );
			d->currentItem->setSelected( TRUE, TRUE );
		    } else {
			if ( e->state() & ShiftButton )
			    d->currentItem->setSelected( !d->currentItem->isSelected(), TRUE );
		    }
		    break;
		}
	    }
	} else {
	    if ( !d->currentItem->prev )
		return;

	    item = d->currentItem;
	    setCurrentItem( d->currentItem->prev );

	    if ( d->selectionMode == Single ) {
		item->setSelected( FALSE );
		d->currentItem->setSelected( TRUE, TRUE );
	    } else {
		if ( e->state() & ShiftButton )
		    d->currentItem->setSelected( !d->currentItem->isSelected(), TRUE );
	    }
	}
	
	repaintItem( item );
	repaintItem( d->currentItem );
    } break;
    case Key_Next:
	scrollBy( 0, visibleHeight() );
	setCurrentItem( findFirstVisibleItem() );
	break;
    case Key_Prior:
	scrollBy( 0, -visibleHeight() );
	setCurrentItem( findFirstVisibleItem() );
	break;
    default:
	if ( !e->text().isEmpty() && e->text()[ 0 ].isPrint() )
	    findItemByName( e->text() );
    }

    ensureItemVisible( d->currentItem );
}

/*!
  \reimp
*/

void QIconView::focusInEvent( QFocusEvent * )
{
    if ( d->currentItem )
	repaintItem( d->currentItem );
    else {
	d->currentItem = d->firstItem;
	repaintItem( d->currentItem );
    }
}

/*!
  \reimp
*/

void QIconView::focusOutEvent( QFocusEvent * )
{
    if ( d->currentItem )
	repaintItem( d->currentItem );
}

/*!
  Selects items by the rubber band.
*/

void QIconView::selectByRubber( QRect oldRubber )
{
    if ( !d->rubber )
	return;

    oldRubber = oldRubber.normalize();
    int minx = contentsWidth(), miny = contentsHeight();
    int maxx = 0, maxy = 0;
    int selected = 0;

    QIconViewItem *item = d->firstItem;
    for ( ; item; item = item->next ) {
	if ( item->intersects( oldRubber ) &&
	     !item->intersects( d->rubber->normalize() ) )
	    item->setSelected( FALSE );
	else if ( item->intersects( d->rubber->normalize() ) ) {
	    item->setSelected( TRUE, TRUE );
	    ++selected;
	    minx = QMIN( minx, item->x() - 1 );
	    miny = QMIN( miny, item->y() - 1 );
	    maxx = QMAX( maxx, item->x() + item->width() + 1 );
	    maxy = QMAX( maxy, item->y() + item->height() + 1 );
	}
    }
    emit selectionChanged();
    emit selectionChanged( selected );
}

/*!
  Draws the rubber band using the painter \a p.
*/

void QIconView::drawRubber( QPainter *p )
{
    if ( !p || !d->rubber )
	return;

    QPoint pnt( d->rubber->x(), d->rubber->y() );
    pnt = contentsToViewport( pnt );
    p->drawRect( QRect( pnt.x(), pnt.y(), d->rubber->width(), d->rubber->height() ) );
}

/*!
  Returns the QDragObject which should be used for DnD.  Subclasses may reimplement this.

  \sa QIconViewItem::dragObject()
*/

QDragObject *QIconView::dragObject()
{
    if ( !d->currentItem )
	return 0;

    QPoint orig = viewportToContents( viewport()->mapFromGlobal( QCursor::pos() ) );

    QIconDrag *drag = new QIconDrag( viewport() );
    drag->setPixmap( QPixmap( d->currentItem->icon().pixmap( d->mode, QIconSet::Normal ) ),
 		     QPoint( d->currentItem->iconRect().width() / 2, d->currentItem->iconRect().height() / 2 ) );
    for ( QIconViewItem *item = d->firstItem; item; item = item->next )
	if ( item->isSelected() )
	    drag->append( QIconDragItem( QRect( item->iconRect( FALSE ).x() - orig.x(),
						item->iconRect( FALSE ).y() - orig.y(),
						item->iconRect().width(), item->iconRect().height() ),
					 QRect( item->textRect( FALSE ).x() - orig.x(),
						item->textRect( FALSE ).y() - orig.y(), 	
						item->textRect().width(), item->textRect().height() ) ) );

    return drag;
}

/*!
  Starts a drag.
*/

void QIconView::startDrag()
{
    QPoint orig = viewportToContents( viewport()->mapFromGlobal( QCursor::pos() ) );
    d->dragStart = QPoint( orig.x() - d->currentItem->x(),
			   orig.y() - d->currentItem->y() );

    QDragObject *drag = dragObject();
    if ( !drag )
	return;

    if ( drag->drag() )
	if ( drag->target() != viewport() )
	    emit moved();
}

/*!
  Inserts an item in the grid of the iconview. You should never need
  to call this manually.
*/

void QIconView::insertInGrid( QIconViewItem *item )
{
    if ( !item )
	return;

    if ( d->reorderItemsWhenInsert ) {
	// #### make this efficient - but it's not too dramatic
	int y = d->spacing;

	item->dirty = FALSE;
	if ( item == d->firstItem ) {
	    makeRowLayout( item, y );
	    return;
	}

	QIconViewItem *begin = rowBegin( item );
	y = begin->y();
	while ( begin ) {
	    begin = makeRowLayout( begin, y );

	    if ( !begin || !begin->next )
		break;

	    begin = begin->next;
	}
	item->dirty = FALSE;
    } else {
	QRegion r( QRect( 0, 0, QMAX( contentsWidth(), visibleWidth() ),
			  QMAX( contentsHeight(), visibleHeight() ) ) );
	
	QIconViewItem *i = d->firstItem;
	int y = -1;
	for ( ; i; i = i->next ) {
	    r = r.subtract( i->rect() );
	    y = QMAX( y, i->y() + i->height() );
	}
	
	QArray<QRect> rects = r.rects();
	QArray<QRect>::Iterator it = rects.begin();
	bool foundPlace = FALSE;
	for ( ; it != rects.end(); ++it ) {
	    QRect rect = *it;
	    if ( rect.width() >= item->width() &&
		 rect.height() >= item->height() ) {
		int sx = 0, sy = 0;
		if ( rect.width() >= item->width() + d->spacing )
		    sx = d->spacing;
		if ( rect.height() >= item->height() + d->spacing )
		    sy = d->spacing;
		item->move( rect.x() + sx, rect.y() + sy );
		foundPlace = TRUE;
		break;
	    }
	}
	
	if ( !foundPlace )
	    item->move( d->spacing, y + d->spacing );
	
	resizeContents( QMAX( contentsWidth(), item->x() + item->width() ),
			QMAX( contentsHeight(), item->y() + item->height() ) );	
	item->dirty = FALSE;
    }
}

/*!
  Emits signals, that indciate selection changes.
*/

void QIconView::emitSelectionChanged()
{
    emit selectionChanged();
    emitNewSelectionNumber();
}

/*!
  Emits signals, that indciate the number of selected items
*/

void QIconView::emitNewSelectionNumber()
{
    int num = 0;
    QIconViewItem *item = d->firstItem;
    for ( ; item; item = item->next )
	if ( item->isSelected() )
	    ++num;

    emit selectionChanged( num );
    d->numSelectedItems = num;
}

/*!
  \internal
*/

void QIconView::emitRenamed( QIconViewItem *item )
{
    if ( !item )
	return;

    emit itemRenamed( item, item->text() );
    emit itemRenamed( item );
}

/*!
  If a drag enters the iconview, shades of the objects, which the drag
  contains are drawn, usnig \a pos as origin.
*/

void QIconView::drawDragShade( const QPoint &pos )
{
    if ( !d->drawDragShade ) {
	d->drawDragShade = TRUE;
	return;
    }

    if ( d->isIconDrag ) {
	QPainter p;
	p.begin( viewport() );
	p.translate( -contentsX(), -contentsY() );
	p.setRasterOp( NotROP );

	QValueList<QIconDragItem>::Iterator it = d->iconDragData.begin();
	for ( ; it != d->iconDragData.end(); ++it ) {
	    QRect ir = (*it).iconRect();
	    QRect tr = (*it).textRect();
	    tr.moveBy( pos.x(), pos.y() );
	    ir.moveBy( pos.x(), pos.y() );
	    style().drawFocusRect( &p, ir, colorGroup() );
	    style().drawFocusRect( &p, tr, colorGroup() );
	}
	
	p.end();
    } else if ( d->numDragItems > 0 ) {
	QPainter p;
	p.begin( viewport() );
	p.setRasterOp( NotROP );

	for ( int i = 0; i < d->numDragItems; ++i ) {
	    QRect r( pos.x() + i * 40, pos.y(), 35, 35 );
	    style().drawFocusRect( &p, r, colorGroup() );
	}
	
	p.end();
    }
}

/*!
  This method is called before a drag is started to initialize
  everything. Subclasses may reimplement this method to gain more
  flexibility.

  \sa QFileIconView::initDrag()
*/

void QIconView::initDrag( QDropEvent *e )
{
    if ( QIconDrag::canDecode( e ) ) {
	QIconDrag::decode( e, d->iconDragData );
	d->isIconDrag = TRUE;
    } else
	d->numDragItems = 0;

}

/*!
  Using this function it's possible to tell the iconview, that a
  drag contains known data, which may be encoded to display
  correct drag shades.
*/

void QIconView::setDragObjectIsKnown( bool b )
{
    d->isIconDrag = b;
}

/*!
  Sets a list of icondrag items. They are used to draw a correct drag shade.
*/

void QIconView::setIconDragData( const QValueList<QIconDragItem> &lst )
{
    d->iconDragData = lst;
}

/*!
  If a drag is unknown (you have no information about how to draw the drag
  shades), it's possible to specify using the function the number of items
  of the drag (e.g. the number of URLs), so that the iconview can draw
  some (not totally correcty) drag shades.
*/

void QIconView::setNumDragItems( int num )
{
    d->numDragItems = num;
}

/*!
  This method is called to draw the rectangle \a r of the background using
  the painter \a p. xOffset and yOffset are known using the methods
  contentsX() and contentsY().

  The default implementation only fills \a r with colorGroup().base(). Subclasses
  may reimplement this to draw fency backgrounds.

  \sa QFileIconView::drawBackground()
*/

void QIconView::drawBackground( QPainter *p, const QRect &r )
{
    p->fillRect( r, QBrush( colorGroup().base() ) );
}

/*!
  \reimp
*/

bool QIconView::eventFilter( QObject * o, QEvent * e )
{
    if ( !o || !e )
	return FALSE;

    QFocusEvent * fe = (QFocusEvent *)e;

    switch( e->type() ) {
    case QEvent::FocusIn:
	focusInEvent( fe );
	return TRUE;
    case QEvent::FocusOut:
	focusOutEvent( fe );
	return TRUE;
    default:
	// nothing
	break;
    }

    return QScrollView::eventFilter( o, e );
}


/*!
  \reimp
*/

QSize QIconView::minimumSizeHint() const
{
    return QSize( 100, 100 );
}

/*!
  \reimp
*/

QSizePolicy QIconView::sizePolicy() const
{
    return QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
}

/*!
  \internal
  Clears string which is used for setting the current item
  when the user types something in
*/

void QIconView::clearInputString()
{
    d->currInputString = QString::null;
}

/*!
  \internal
  Finds the first item beginning with \a text and makes
  it the current one
*/

void QIconView::findItemByName( const QString text )
{
    if ( d->inputTimer->isActive() )
	d->inputTimer->stop();
    d->inputTimer->start( 500, TRUE );
    d->currInputString += text;
    QIconViewItem *item = findItem( d->currInputString );
    if ( item ) {
	QIconViewItem *i = d->currentItem;
	setCurrentItem( item );
	repaintItem( i );
	repaintItem( d->currentItem );
    }
}

/*!
  Layouts a row of icons (in AlignMode == South this is a column). Starts
  layouting with the item \a begin.\a y is the starting coordinate.
  Returns the last item of the row and sets the new starting coordinate to \a y.
*/

QIconViewItem *QIconView::makeRowLayout( QIconViewItem *begin, int &y )
{
    QIconViewItem *end = 0;
	
    if ( d->alignMode == East ) {
	
	if ( d->rastX == -1 ) {
	    // first calculate the row height
	    int h = 0;
	    int x = 0;
	    int ih = 0;
	    QIconViewItem *item = begin;
	    while ( TRUE ) {
		x += d->spacing + item->width();
		if ( x > visibleWidth() && item != begin ) {
		    item = item->prev;
		    break;
		}
		h = QMAX( h, item->height() );
		ih = QMAX( ih, item->iconRect().height() );
		QIconViewItem *old = item;
		item = item->next;
		if ( !item ) {
		    item = old;
		    break;
		}
	    }
	    end = item;
	
	    if ( d->rastY != -1 )
		h = QMAX( h, d->rastY );
	
	    // now move the items
	    item = begin;
	    while ( TRUE ) {
		item->dirty = FALSE;
		if ( item == begin )
		    item->move( d->spacing, y + ih - item->iconRect().height() );
		else
		    item->move( item->prev->x() + item->prev->width() + d->spacing,
				y + ih - item->iconRect().height() );
		if ( item == end )
		    break;
		item = item->next;
	    }
	    y += h + d->spacing;
	} else {
	    // first calculate the row height
	    int h = begin->height();
	    int x = d->spacing;
	    int ih = begin->iconRect().height();
	    QIconViewItem *item = begin;
	    int i = 0;
	    int sp = 0;
	    while ( TRUE ) {
		int r = calcGridNum( item->width(), d->rastX );
		if ( item == begin ) {
		    i += r;
		    sp += r;
		    x = d->spacing + d->rastX * r;
		} else {
		    sp += r;
		    i += r;
		    x = i * d->rastX + sp * d->spacing;
		}
		if ( x > visibleWidth() && item != begin ) {
		    item = item->prev;
		    break;
		}
		h = QMAX( h, item->height() );
		ih = QMAX( ih, item->iconRect().height() );
		QIconViewItem *old = item;
		item = item->next;
		if ( !item ) {
		    item = old;
		    break;
		}
	    }
	    end = item;
	
	    if ( d->rastY != -1 )
		h = QMAX( h, d->rastY );

	    // now move the items
	    item = begin;
	    i = 0;
	    sp = 0;
	    while ( TRUE ) {
		item->dirty = FALSE;
		int r = calcGridNum( item->width(), d->rastX );
		if ( item == begin ) {
		    if ( d->itemTextPos == Bottom )
			item->move( d->spacing + ( r * d->rastX - item->width() ) / 2,
				    y + ih - item->iconRect().height() );
		    else
			item->move( d->spacing, y + ih - item->iconRect().height() );
		    i += r;
		    sp += r;
		} else {
		    sp += r;
		    int x = i * d->rastX + sp * d->spacing;
		    if ( d->itemTextPos == Bottom )
			item->move( x + ( r * d->rastX - item->width() ) / 2,
				    y + ih - item->iconRect().height() );
		    else
			item->move( x, y + ih - item->iconRect().height() );
		    i += r;
		}
		if ( item == end )
		    break;
		item = item->next;
	    }
	    y += h + d->spacing;
	}


    } else { // -------------------------------- SOUTH ------------------------------
	
	int x = y;
	
	{
	    int w = 0;
	    int y = 0;
	    QIconViewItem *item = begin;
	    while ( TRUE ) {
		y += d->spacing + item->height();
		if ( y > visibleHeight() && item != begin ) {
		    item = item->prev;
		    break;
		}
		w = QMAX( w, item->width() );
		QIconViewItem *old = item;
		item = item->next;
		if ( !item ) {
		    item = old;
		    break;
		}
	    }
	    end = item;
	
	    if ( d->rastX != -1 )
		w = QMAX( w, d->rastX );
	
	    // now move the items
	    item = begin;
	    while ( TRUE ) {
		item->dirty = FALSE;
		if ( d->itemTextPos == Bottom ) {
		    if ( item == begin )
			item->move( x + ( w - item->width() ) / 2, d->spacing );
		    else
			item->move( x + ( w - item->width() ) / 2,
				    item->prev->y() + item->prev->height() + d->spacing );
		} else {
		    if ( item == begin )
			item->move( x, d->spacing );
		    else
			item->move( x, item->prev->y() + item->prev->height() + d->spacing );
		}
		if ( item == end )
		    break;
		item = item->next;
	    }
	    x += w + d->spacing;
	}

	y = x;
    }	

    return end;
}

/*!
  \internal
  Calculates how many cells and item of the width \a w needs in a grid with of
  \a x and returns the result.
*/

int QIconView::calcGridNum( int w, int x ) const
{
    float r = (float)w / (float)x;
    if ( ( w / x ) * x != w )
	r += 1.0;
    return (int)r;
}

/*!
  \internal
  Returns the first item of the row which cntains \a item.
*/

QIconViewItem *QIconView::rowBegin( QIconViewItem * ) const
{
    // #### todo
    return d->firstItem;
}

/*!
  This slot selects the currentle highlighed item (if there is one).
  This is used in the single click mode.

  \sa QIconView::setSingleClickConfiguration(), QIconView::setUseSingleClickMode()
*/

void QIconView::selectHighlightedItem()
{
    if ( d->highlightedItem ) {
	QIconViewItem *old = d->currentItem;
	setCurrentItem( d->highlightedItem );
	repaintItem( old );
	d->highlightedItem->setSelected( TRUE );
    }
}

/*!
  \internal
*/

void QIconView::clearSingleClickConfig()
{
    if ( d->singleClickConfig.normalText )
	delete d->singleClickConfig.normalText;
    if ( d->singleClickConfig.normalTextCol )
	delete d->singleClickConfig.normalTextCol;
    if ( d->singleClickConfig.highlightedText )
	delete d->singleClickConfig.highlightedText;
    if ( d->singleClickConfig.highlightedTextCol )
	delete d->singleClickConfig.highlightedTextCol;
    if ( d->singleClickConfig.highlightedCursor )
	delete d->singleClickConfig.highlightedCursor;
    d->singleClickConfig.normalText = 0;
    d->singleClickConfig.normalTextCol = 0;
    d->singleClickConfig.highlightedText = 0;
    d->singleClickConfig.highlightedTextCol = 0;
    d->singleClickConfig.highlightedCursor = 0;
    d->singleClickConfig.setCurrentInterval = -1;
}

static int cmpIconViewItems( const void *n1, const void *n2 )
{
    if ( !n1 || !n2 )
	return 0;

    QIconViewPrivate::SortableItem *i1 = (QIconViewPrivate::SortableItem *)n1;
    QIconViewPrivate::SortableItem *i2 = (QIconViewPrivate::SortableItem *)n2;

    return i1->item->key().compare( i2->item->key() );
}

/*!
  Sorts the items of the listview. If \a ascending is TRUE, the items
  are sorted in increasing order, else in decreasing order. For sorting
  the key of the items is used.

  \sa QIconViewItem::key(), QIconViewItem::setKey()
*/

void QIconView::sortItems( bool ascending )
{
    if ( count() == 0 )
	return;

    QIconViewPrivate::SortableItem *items = new QIconViewPrivate::SortableItem[ count() ];

    QIconViewItem *item = d->firstItem;
    int i = 0;
    for ( ; item; item = item->next )
	items[ i++ ].item = item;

    qsort( items, count(), sizeof( QIconViewPrivate::SortableItem ), cmpIconViewItems );

    QIconViewItem *prev = 0;
    item = 0;
    if ( ascending ) {
	for ( i = 0; i < (int)count(); ++i ) {
	    item = items[ i ].item;
	    if ( item ) {
		item->prev = prev;
		if ( item->prev )
		    item->prev->next = item;
		item->next = 0;
	    }
	    if ( i == 0 )
		d->firstItem = item;
	    if ( i == (int)count() - 1 )
		d->lastItem = item;
	    prev = item;
	}
    } else {
	for ( i = (int)count() - 1; i >= 0 ; --i ) {
	    item = items[ i ].item;
	    if ( item ) {
		item->prev = prev;
		if ( item->prev )
		    item->prev->next = item;
		item->next = 0;
	    }
	    if ( i == (int)count() - 1 )
		d->firstItem = item;
	    if ( i == 0 )
		d->lastItem = item;
	    prev = item;
	}
    }

    delete [] items;

    orderItemsInGrid();
    viewport()->repaint( FALSE );
}

#include "qiconview.moc"

