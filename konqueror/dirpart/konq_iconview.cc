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

#include "konq_iconview.h"

#include "konq_iconviewitemdelegate.h"

#include <QHelpEvent>

KonqIconView::KonqIconView( QWidget* parent ) : QListView( parent )
{
    setViewMode( QListView::IconMode );        
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setGridSize( QSize( 150, 150 ) );
    setMovement( QListView::Static );
    setResizeMode( QListView::Adjust );
    setDragEnabled( true );
    setAcceptDrops( true );
//    setItemDelegate( new KonqIconViewItemDelegate( this ) );
    setEditTriggers( QAbstractItemView::EditKeyPressed );
}

KonqIconView::~KonqIconView()
{
}

bool KonqIconView::viewportEvent( QEvent* ev )
{
    if ( ev->type() == QEvent::ToolTip ) {
        QHelpEvent *hev = static_cast<QHelpEvent*>(ev);
        emit toolTip( indexAt( hev->pos() ) );
        return true;
    }
    return QListView::viewportEvent( ev );
}

void KonqIconView::contextMenuEvent( QContextMenuEvent* ev )
{
    if ( ev->reason() == QContextMenuEvent::Keyboard )
        emit contextMenu( viewport()->mapToGlobal( visualRect( currentIndex() ).center() ), selectedIndexes() );
    else
        emit contextMenu( ev->globalPos(), selectedIndexes() );
}

void KonqIconView::mouseReleaseEvent( QMouseEvent* ev )
{
    if ( state() == QAbstractItemView::NoState )
        emit execute( indexAt( ev->pos() ), ev->button() );
    QListView::mouseReleaseEvent( ev );
}

void KonqIconView::keyPressEvent( QKeyEvent* ev )
{
    if ( ev->key() == Qt::Key_Return )
        emit execute( currentIndex(), Qt::NoButton );
    QListView::keyPressEvent( ev );
}

QStyleOptionViewItem KonqIconView::viewOptions() const
{
    QStyleOptionViewItem option = QListView::viewOptions();
    option.decorationSize = QSize( 128, 128 );
    option.displayAlignment = Qt::AlignHCenter;
    return option;
}
