/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_textviewitem.h"
#include "konq_propsview.h"

#include <kio/global.h>
#include <assert.h>
#include <stdio.h>

QString KonqTextViewItem::key( int _column, bool asc) const
{
   if (_column==0) return key(1,asc);
   QString tmp=sortChar;
   if (!asc && (sortChar=='0')) tmp=QChar('2');
   tmp+=text(_column);
   return tmp;
};

void KonqTextViewItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
   if (!m_pTextView->props()->bgPixmap().isNull())
      _painter->drawTiledPixmap( 0, 0, _width, height(),m_pTextView->props()->bgPixmap(),0, 0 );
   QColorGroup cg( _cg );
   cg.setColor(QColorGroup::Text, m_pTextView->colors[type]);
   cg.setColor(QColorGroup::HighlightedText, m_pTextView->highlight[type]);
   cg.setColor(QColorGroup::Highlight, Qt::darkGray);
   
   //         cg.setColor( QColorGroup::Base, Qt::color0 );
   QListViewItem::paintCell( _painter, cg, _column, _width, _alignment );
};

void KonqTextViewItem::setup()
{
   widthChanged();
   int h(listView()->fontMetrics().height());
   if ( h % 2 > 0 ) h++;
   setHeight(h);
};
