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

#ifndef QICONVIEW_H
#define QICONVIEW_H

#include <qscrollview.h>
#include <qiconset.h>
#include <qstring.h>
#include <qrect.h>
#include <qpoint.h>
#include <qsize.h>
#include <qfont.h>
#include <qlist.h>
#include <qdragobject.h>
#include <qstringlist.h>

class QIconView;
class QFontMetrics;
class QPainter;
class QMimeSource;
class QDragObject;
class QMouseEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QKeyEvent;
class QFocusEvent;
class QShowEvent;

struct QIconViewPrivate;
class QIconViewItem;
class QIconViewItemLineEdit;

/*****************************************************************************
 *
 * Class QIconDragItem
 *
 *****************************************************************************/

class Q_EXPORT QIconDragItem
{
public:
    QIconDragItem();
    QIconDragItem( const QRect &ir, const QRect &tr );
    virtual ~QIconDragItem();

    virtual bool operator<( const QIconDragItem &icon )  const;
    virtual bool operator==( const QIconDragItem &icon ) const;
	
    virtual QRect iconRect() const;
    virtual QRect textRect() const;
    virtual QString key() const;

    virtual void setIconRect( const QRect &r );
    virtual void setTextRect( const QRect &r );

protected:
    virtual void makeKey();

    QRect iconRect_, textRect_;
    QString key_;
	
};

/*****************************************************************************
 *
 * Class QIconDrag
 *
 *****************************************************************************/

typedef QValueList<QIconDragItem> QIconList;

class Q_EXPORT QIconDrag : public QDragObject
{
    Q_OBJECT

public:
    QIconDrag( const QIconList &icons_, QWidget * dragSource, const char* name = 0 );
    QIconDrag( QWidget * dragSource, const char* name = 0 );
    virtual ~QIconDrag();

    void setIcons( const QIconList &list_ );
    void append( const QIconDragItem &icon_ );

    virtual const char* format( int i ) const;
    virtual QByteArray encodedData( const char* mime ) const;

    static bool canDecode( QMimeSource* e );

    static bool decode( QMimeSource* e, QIconList &list_ );

protected:
    QIconList icons;

};

/*****************************************************************************
 *
 * Class QIconViewItem
 *
 *****************************************************************************/

class Q_EXPORT QIconViewItem : public QObject
{
    friend class QIconView;
    friend class QIconViewItemLineEdit;

    Q_OBJECT

public:
    QIconViewItem( QIconView *parent );
    QIconViewItem( QIconView *parent, QIconViewItem *after );
    QIconViewItem( QIconView *parent, const QString &text );
    QIconViewItem( QIconView *parent, QIconViewItem *after, const QString &text );
    QIconViewItem( QIconView *parent, const QString &text, const QIconSet &icon );
    QIconViewItem( QIconView *parent, QIconViewItem *after, const QString &text, const QIconSet &icon );
    virtual ~QIconViewItem();

    virtual void setRenameEnabled( bool allow );
    virtual void setDragEnabled( bool allow );
    virtual void setDropEnabled( bool allow );

    virtual QString text() const;
    virtual QIconSet icon() const;
    virtual QString key() const;

    bool renameEnabled() const;
    bool dragEnabled() const;
    bool dropEnabled() const;

    QIconView *iconView() const;
    QIconViewItem *prevItem() const;
    QIconViewItem *nextItem() const;

    int index() const;

    virtual void setSelected( bool s, bool cb = FALSE );
    virtual void setSelectable( bool s );

    bool isSelected() const;
    bool isSelectable() const;

    virtual void repaint();

    virtual void move( int x, int y );
    virtual void moveBy( int dx, int dy );
    virtual void move( const QPoint &pnt );
    virtual void moveBy( const QPoint &pnt );

    QRect rect() const;
    int x() const;
    int y() const;
    int width() const;
    int height() const;
    QSize size() const;
    QPoint pos() const;
    QRect textRect( bool relative = TRUE ) const;
    QRect iconRect( bool relative = TRUE ) const;
    bool contains( QPoint pnt ) const;
    bool intersects( QRect r ) const;

    virtual void setFont( const QFont &font );
    QFont font() const;

    virtual void setViewMode( QIconSet::Size mode );
    QIconSet::Size viewMode() const;

    virtual bool acceptDrop( const QMimeSource *mime ) const;

    void rename();

signals:
    void renamed( const QString &text );
    void renamed();

public slots:
    virtual void setText( const QString &text );
    virtual void setIcon( const QIconSet &icon );
    virtual void setText( const QString &text, bool recalc );
    virtual void setIcon( const QIconSet &icon, bool recalc );
    virtual void setKey( const QString &k );

protected slots:
    virtual void renameItem();
    virtual void cancelRenameItem();

protected:
    virtual void removeRenameBox();
    virtual void calcRect( const QString &text_ = QString::null );
    virtual void paintItem( QPainter *p );
    virtual void paintFocus( QPainter *p );
    virtual void dropped( QDropEvent *e );
    virtual void dragEntered();
    virtual void dragLeft();
    virtual void init();
    void setView( QIconView* v );
    void setItemRect( const QRect &r );
    void setTextRect( const QRect &r );
    void setIconRect( const QRect &r );

private:
    QIconView *view;
    QString itemText, itemKey;
    QIconSet itemIcon;
    QIconViewItem *prev, *next;
    bool allow_rename, allow_drag, allow_drop;
    bool selected, selectable;
    QRect itemRect, itemTextRect, itemIconRect;
    QFontMetrics *fm;
    QFont f;
    QIconSet::Size itemViewMode;
    QIconViewItemLineEdit *renameBox;
    bool isReady;
    QRect oldRect;
    bool dirty;

};


/*****************************************************************************
 *
 * Class QIconView
 *
 *****************************************************************************/

class Q_EXPORT QIconView : public QScrollView
{
    friend class QIconViewItem;
    friend struct QIconViewPrivate;

    Q_OBJECT

public:
    enum SelectionMode {
	Single = 0,
	Multi,
	StrictMulti
    };
    enum AlignMode {
	East = 0,
	South
    };
    enum ResizeMode {
	Fixed = 0,
	Adjust
    };
    enum ItemTextPos {
	Bottom = 0,
	Right
    };

    QIconView( QWidget *parent = 0, const char *name = 0, WFlags f=0 );
    virtual ~QIconView();

    virtual void insertItem( QIconViewItem *item, QIconViewItem *after = 0L );
    virtual void removeItem( QIconViewItem *item );

    int index( const QIconViewItem *item ) const;

    QIconViewItem *firstItem() const;
    QIconViewItem *lastItem() const;
    QIconViewItem *selectedItem() const;
    QIconViewItem *currentItem() const;
    virtual void setCurrentItem( QIconViewItem *item );

    unsigned int count() const;

    virtual void setViewMode( QIconSet::Size mode );
    QIconSet::Size viewMode() const;

    virtual void showEvent( QShowEvent * );

    virtual void setSelectionMode( SelectionMode m );
    SelectionMode selectionMode() const;

    virtual void setSingleClickConfiguration( QFont *normalText, QColor *normalTextCol,
					      QFont *highlightedText, QColor *highlightedTextCol,
					      QCursor *highlightedCursor, int setCurrentInterval );
    void singleClickConfiguration( QFont *normalText, QColor *normalTextCol,
				   QFont *highlightedText, QColor *highlightedTextCol,
				   QCursor *highlightedCursor, int &setCurrentInterval ) const;
    virtual void setUseSingleClickMode( bool b );
    bool useSingleClickMode() const;

    QIconViewItem *findItem( const QPoint &pos ) const;
    QIconViewItem *findItem( const QString &text ) const;
    void selectAll( bool select );

    virtual void repaintItem( QIconViewItem *item );

    void ensureItemVisible( QIconViewItem *item );
    QIconViewItem* findFirstVisibleItem() const;

    virtual void clear();

    virtual void setGridX( int rx );
    virtual void setGridY( int ry );
    int gridX() const;
    int gridY() const;
    virtual void setSpacing( int sp );
    int spacing() const;
    virtual void setItemTextPos( ItemTextPos pos );
    ItemTextPos itemTextPos() const;
    virtual void setAlignMode( AlignMode am );
    AlignMode alignMode() const;
    virtual void setResizeMode( ResizeMode am );
    ResizeMode resizeMode() const;
    virtual void setMaxItemWidth( int w );
    int maxItemWidth() const;
    virtual void setMaxItemTextLength( int w );
    int maxItemTextLength() const;
    void setReorderItemsWhenInsert( bool b );
    bool reorderItemsWhenInsert() const;
    void setResortItemsWhenInsert( bool sort, bool ascending = TRUE );
    bool resortItemsWhenInsert() const;
    bool sortOrder() const;

    virtual void setRearrangeEnabled( bool b );
    bool rearrangeEnabled() const;

    bool eventFilter( QObject * o, QEvent * );

    QSize minimumSizeHint() const;
    QSizePolicy sizePolicy() const;

    virtual void sortItems( bool ascending = TRUE );

public slots:
    virtual void orderItemsInGrid();
    virtual void alignItemsInGrid( const QSize &grid );
    virtual void alignItemsInGrid();

signals:
    void dropped( QDropEvent *e );
    void moved();
    void doubleClicked( QIconViewItem *item );
    void itemRightClicked( QIconViewItem *item );
    void viewportRightClicked();
    void selectionChanged();
    void selectionChanged( int numItems );
    void currentChanged();
    void currentChanged( QIconViewItem *item );
    void onItem( QIconViewItem *item );
    void onViewport();
    // ###### HACK for builder. Torben will remove it!
    void rightButtonPressed( QIconViewItem* item, const QPoint& pos );
    void itemRenamed( QIconViewItem *item, const QString & );
    void itemRenamed( QIconViewItem *item );

protected slots:
    virtual void doAutoScroll();
    virtual void adjustItems();
    virtual void slotUpdate();
    virtual void selectHighlightedItem();

private slots:
    void clearInputString();

protected:
    virtual void drawContents( QPainter *p, int cx, int cy, int cw, int ch );
    virtual void contentsMousePressEvent( QMouseEvent *e );
    virtual void contentsMouseReleaseEvent( QMouseEvent *e );
    virtual void contentsMouseMoveEvent( QMouseEvent *e );
    virtual void contentsMouseDoubleClickEvent( QMouseEvent *e );
    virtual void contentsDragEnterEvent( QDragEnterEvent *e );
    virtual void contentsDragMoveEvent( QDragMoveEvent *e );
    virtual void contentsDragLeaveEvent( QDragLeaveEvent *e );
    virtual void contentsDropEvent( QDropEvent *e );
    virtual void resizeEvent( QResizeEvent* e );
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void focusInEvent( QFocusEvent *e );
    virtual void focusOutEvent( QFocusEvent *e );

    virtual void selectByRubber( QRect oldRubber );
    virtual void drawRubber( QPainter *p );
    virtual QDragObject *dragObject();
    virtual void startDrag();
    virtual void insertInGrid( QIconViewItem *item );
    virtual void drawDragShade( const QPoint &pnt );
    virtual void initDrag( QDropEvent *e );
    virtual void drawBackground( QPainter *p, const QRect &r );

    void emitSelectionChanged();
    void emitNewSelectionNumber();
    void emitRenamed( QIconViewItem *item );

    void setDragObjectIsKnown( bool b );
    void setIconDragData( const QValueList<QIconDragItem> &lst );
    void setNumDragItems( int num );
    QIconViewItem *makeRowLayout( QIconViewItem *begin, int &y );

private:
    void findItemByName( const QString text );
    int calcGridNum( int w, int x ) const;
    QIconViewItem *rowBegin( QIconViewItem *item ) const;
    void clearSingleClickConfig();

    QIconViewPrivate *d;

};

#endif
