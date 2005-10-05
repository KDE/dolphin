/* This file is part of the KDE project
   Copyright (c) 2005 Pascal Létourneau <pascal.letourneau@kdemail.net>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_listview.h"

#include <QHelpEvent>
#include <QHeaderView>

#include "konq_itemdelegate.h"

KonqListView::KonqListView( QWidget* parent ) : QTreeView( parent )
{
    setAlternatingRowColors( true );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectItems );
    setDragEnabled( true );
    setAcceptDrops( true );
    setEditTriggers( QAbstractItemView::EditKeyPressed );
    setItemDelegate( new KonqItemDelegate( this ) );
    header()->setClickable( true );
    header()->setSortIndicatorShown( true );
}

KonqListView::~KonqListView()
{
}

void KonqListView::setSelectionModel( QItemSelectionModel* model )
{
    QTreeView::setSelectionModel( model );
    connect( selectionModel(), SIGNAL( currentChanged(const QModelIndex&,const QModelIndex&) ),
             SLOT( slotCurrentChanged(const QModelIndex&,const QModelIndex&) ) );
}

bool KonqListView::viewportEvent( QEvent* ev )
{
    if ( ev->type() == QEvent::ToolTip ) {
        QHelpEvent* hev = static_cast<QHelpEvent*>(ev);
        if ( isExecutableArea( hev->pos() ) )
            emit toolTip( indexAt( hev->pos() ) );
        return true;
    }
    return QTreeView::viewportEvent( ev );
}

void KonqListView::contextMenuEvent( QContextMenuEvent* ev )
{
    if ( ev->reason() == QContextMenuEvent::Keyboard )
        emit contextMenu( viewport()->mapToGlobal( visualRect( currentIndex() ).center() ), selectedIndexes() );
    else
        emit contextMenu( ev->globalPos(), selectedIndexes() ); 
}

void KonqListView::mouseReleaseEvent( QMouseEvent* ev )
{
    if ( isExecutableArea( ev->pos() ) )
        emit execute( indexAt( ev->pos() ), ev->button() );
    QTreeView::mouseReleaseEvent( ev );
}

void KonqListView::keyPressEvent( QKeyEvent* ev )
{
    if ( ev->key() == Qt::Key_Return )
        emit execute( currentIndex(), Qt::NoButton );
    QTreeView::keyPressEvent( ev );
}

void KonqListView::slotCurrentChanged( const QModelIndex& index, const QModelIndex& )
{
    if ( index.column() != 0 )
        selectionModel()->setCurrentIndex( model()->index( index.row(), 0, index.parent() ), QItemSelectionModel::NoUpdate );
}

bool KonqListView::isExecutableArea( const QPoint& p )
{
    QModelIndex index = indexAt( p );
    if ( index.column() != 0 )
        return false;

    QFontMetrics fm = fontMetrics();
    QVariant value = model()->data( index, Qt::FontRole );
    if ( value.isValid() )
        fm = QFontMetrics( qvariant_cast<QFont>( value ) );

    int width = fm.width( model()->data( index, Qt::DisplayRole ).toString() );
    width += qvariant_cast<QPixmap>( model()->data( index, Qt::DecorationRole ) ).width() + 2;
    return p.x() > indentation() && p.x() < width + indentation();
}
