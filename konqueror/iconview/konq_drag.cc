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

#include "konq_drag.h"
#include <stdlib.h> // for atoi()

KonqDragItem::KonqDragItem()
    : QIconDragItem()
{
    makeKey();
}

KonqDragItem::KonqDragItem( const QRect &ir, const QRect &tr, const QString &u )
    : QIconDragItem( ir, tr ), url_( u )
{
    makeKey();
}

KonqDragItem::~KonqDragItem()
{
}

QString KonqDragItem::url() const
{
    return url_;
}

void KonqDragItem::setURL( const QString &u )
{
    url_ = u;
}

void KonqDragItem::makeKey()
{	
    QString k( "%1 %2 %3 %4 %5 %6 %7 %8 %9");
    k = k.arg( iconRect().x() ).arg( iconRect().y() ).arg( iconRect().width() ).
	arg( iconRect().height() ).arg( textRect().x() ).arg( textRect().y() ).
	arg( textRect().width() ).arg( textRect().height() ).arg( url_ );
    key_ = k;
}

KonqDrag::KonqDrag( QWidget * dragSource, const char* name )
    : QIconDrag( dragSource, name )
{
}

KonqDrag::~KonqDrag()
{
}

const char* KonqDrag::format( int i ) const
{
    if ( i == 0 )
	return "application/x-qiconlist";
    else if ( i == 1 )
	return "text/uri-iconlist";
    else if ( i == 2 )
	return "text/uri-list";
    else return 0;
}

void KonqDrag::append( const KonqDragItem &icon_ )
{
    icons.append( icon_ );
    QIconDrag::icons.append( icon_ );
}

QByteArray KonqDrag::encodedData( const char* mime ) const
{
    QByteArray a;
    if ( QString( mime ) == "application/x-qiconlist" )
	a = QIconDrag::encodedData( mime );
    else if ( QString( mime ) == "text/uri-iconlist" ) {
	int c = 0;
	KonqList::ConstIterator it = icons.begin();
	for ( ; it != icons.end(); ++it ) {
	    QString k( "%1 %2 %3 %4 %5 %6 %7 %8 %9" );
	    k = k.arg( (*it).iconRect().x() ).arg( (*it).iconRect().y() ).arg( (*it).iconRect().width() ).
		arg( (*it).iconRect().height() ).arg( (*it).textRect().x() ).arg( (*it).textRect().y() ).
		arg( (*it).textRect().width() ).arg( (*it).textRect().height() ).arg( (*it).url() );
	    int l = k.length();
	    a.resize(c + l + 1 );
	    memcpy( a.data() + c , k.latin1(), l );
	    a[ c + l ] = 0;
	    c += l + 1;
	}
	a.resize( c - 1 );
    } else if ( QString( mime ) == "text/uri-list" ) {
	int c = 0;
	KonqList::ConstIterator it = icons.begin();
	for ( ; it != icons.end(); ++it ) {
	    QString k( "%1" );
	    k = k.arg( (*it).url() );
	    int l = k.length();
	    a.resize(c + l + 2 );
	    memcpy( a.data() + c , k.latin1(), l );
	    memcpy(a.data() + c + l, "\r\n" ,2);
	    c += l + 2;
	}
	a.resize( c - 1 );
    }

    return a;
}

bool KonqDrag::canDecode( QMimeSource* e )
{
    return e->provides( "text/uri-iconlist" ) ||
	e->provides( "text/uri-list" );
}

bool KonqDrag::decode( QMimeSource* e, QValueList<KonqDragItem> &list_ )
{
    QByteArray ba = e->encodedData( "text/uri-iconlist" );
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

	    KonqDragItem icon;
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

	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    icon.setIconRect( ir );
	    icon.setTextRect( tr );
	    icon.setURL( s.latin1() + pos + 1 );
	    list_.append( icon );
	}
	return TRUE;
    }

    return FALSE;
}

bool KonqDrag::decode( QMimeSource *e, QStringList &uris )
{
    QByteArray ba = e->encodedData( "text/uri-list" );
    if ( ba.size() ) {
	uris.clear();
	uint c = 0;
	char* d = ba.data();
	while ( c < ba.size() ) {
	    uint f = c;
	    while ( c < ba.size() && d[ c ] )
		c++;
	    if ( c < ba.size() ) {
		uris.append( d + f );
		c++;
	    } else {
		QString s( QString(d + f).left(c - f + 1) );
		uris.append( s );
	    }
	}
	return TRUE;
    }
    return FALSE;
}

#include "konq_drag.moc"
