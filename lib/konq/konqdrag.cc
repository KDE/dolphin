/* This file is part of the KDE projects
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

#include "konqdrag.h"

KonqDrag::KonqDrag( QWidget * dragSource, const char* name )
    : QIconDrag( dragSource, name )
{
}

const char* KonqDrag::format( int i ) const
{
    if ( i == 0 )
	return "application/x-qiconlist";
    else if ( i == 1 )
	return "text/uri-list";
    else return 0;
}

QByteArray KonqDrag::encodedData( const char* mime ) const
{
    QByteArray a;
    if ( QCString( mime ) == "application/x-qiconlist" )
	a = QIconDrag::encodedData( mime );
    else if ( QCString( mime ) == "text/uri-list" ) {
        QString s = urls.join( "\r\n" );
        a = QCString(s.latin1()); // perhaps use QUriDrag::unicodeUriToUri here
    }
    return a;
}

bool KonqDrag::canDecode( const QMimeSource* e )
{
    return  e->provides( "application/x-qiconlist" ) ||
      e->provides( "text/uri-list" );
}

bool KonqDrag::decode( const QMimeSource *e, KURL::List &uris )
{
    QStrList lst;
    bool ret = QUriDrag::decode( e, lst );
    for (QStrListIterator it(lst); *it; ++it)
      uris.append(KURL(*it)); // *it is encoded already
    return ret;
}

void KonqDrag::append( const QIconDragItem &item, const QRect &pr,
                             const QRect &tr, const QString &url )
{
    QIconDrag::append( item, pr, tr );
    urls.append(url);
}

#include "konqdrag.moc"
