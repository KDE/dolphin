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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_selectionmodel.h"

#include "kdebug.h"

KonqSelectionModel::KonqSelectionModel( QAbstractItemModel* model )
    : QItemSelectionModel( model )
{
}

KonqSelectionModel::~KonqSelectionModel()
{
}

void KonqSelectionModel::select( const QItemSelection& selection, QItemSelectionModel::SelectionFlags command )
{
    QItemSelection newSelection;
    foreach ( QItemSelectionRange range, selection ) {
        QModelIndex index1 = model()->index( range.top(), 0, range.parent() );
        QModelIndex index2 = model()->index( range.bottom(), 0, range.parent() );
        newSelection << QItemSelectionRange( index1, index2 );
    }
    QItemSelectionModel::select( newSelection, command );
}

void KonqSelectionModel::select( const QModelIndex& index, QItemSelectionModel::SelectionFlags command )
{
    QModelIndex index0 = model()->index( index.row(), 0, index.parent() );
    QItemSelectionModel::select( index0, command );
}
