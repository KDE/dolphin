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

#include "konq_itemdelegate.h"

#include <QModelIndex>
#include <QPainter>

KonqItemDelegate::KonqItemDelegate( QObject* parent ) : QItemDelegate( parent )
{
}

KonqItemDelegate::~KonqItemDelegate()
{
}

void KonqItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItem opt = option;
    opt.showDecorationSelected = false;
    if ( index.column() == 0 )
        width = -1;
    else
        width = -2;
    QItemDelegate::paint( painter, opt, index );
}

void KonqItemDelegate::drawDisplay( QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QString& text ) const
{
    QRect r = rect;
    if ( width == -1 ) {
        width = painter->fontMetrics().width( text );
        if ( width < rect.width() )
            r.setWidth( width+2 );
    }
    QItemDelegate::drawDisplay( painter, option, r, text );
}

void KonqItemDelegate::drawFocus( QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect ) const
{
    QRect r = rect;
    if ( width != -2 && width < rect.width() )
        r.setWidth( width+2 );
    QItemDelegate::drawFocus( painter, option, r );
}

#include "konq_itemdelegate.moc"
