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
#include <stdlib.h> // for atoi()

// For doDrop
#include <qpopupmenu.h>
#include <kio/paste.h>
#include <kio/job.h>
#include <klocale.h>
#include <kdebug.h>
#include <kuserpaths.h>
#include <X11/Xlib.h>

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
    k = k.arg( pixmapRect().x() ).arg( pixmapRect().y() ).arg( pixmapRect().width() ).
	arg( pixmapRect().height() ).arg( textRect().x() ).arg( textRect().y() ).
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
	    k = k.arg( (*it).pixmapRect().x() ).arg( (*it).pixmapRect().y() ).arg( (*it).pixmapRect().width() ).
		arg( (*it).pixmapRect().height() ).arg( (*it).textRect().x() ).arg( (*it).textRect().y() ).
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
	    icon.setPixmapRect( ir );
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

/////////////////

//static
void KonqDrag::doDrop( const KURL & dest, QDropEvent * ev, QObject * receiver )
{
    QStringList lst;
    if ( QUriDrag::decodeToUnicodeUris( ev, lst ) ) // Are they urls ?
    {
	if( lst.count() == 0 )
	{
	    kDebugWarning(1202,"Oooops, no data ....");
	    return;
	}
        // Check if we dropped something on itself
        QStringList::Iterator it = lst.begin();
        for ( ; it != lst.end() ; it++ )
            if ( dest.cmp( KURL(*it), true /*ignore trailing slashes*/ ) )
                return; // do nothing instead of diaplying kfm's annoying error box

        // Check the state of the modifiers key at the time of the drop
        Window root;
        Window child;
        int root_x, root_y, win_x, win_y;
        uint keybstate;
        XQueryPointer( qt_xdisplay(), qt_xrootwin(), &root, &child,
                       &root_x, &root_y, &win_x, &win_y, &keybstate );

        if ( dest.path( 1 ) == KUserPaths::trashPath() )
            ev->setAction( QDropEvent::Move );
        else if ( ((keybstate & ControlMask) == 0) && ((keybstate & ShiftMask) == 0) )
        {
            // Nor control nor shift are pressed => show popup menu
            QPopupMenu popup;
            popup.insertItem( i18n( "&Copy Here" ), 1 );
            popup.insertItem( i18n( "&Move Here" ), 2 );
            popup.insertItem( i18n( "&Link Here" ), 3 );

            int result = popup.exec( QPoint( win_x, win_y ) );
            switch (result) {
                case 1 : ev->setAction( QDropEvent::Copy ); break;
                case 2 : ev->setAction( QDropEvent::Move ); break;
                case 3 : ev->setAction( QDropEvent::Link ); break;
                default : return;
            }
        }

        KIO::Job * job = 0L;
	switch ( ev->action() ) {
            case QDropEvent::Move : job = KIO::move( lst, dest ); break;
            case QDropEvent::Copy : job = KIO::copy( lst, dest ); break;
            case QDropEvent::Link : KIO::link( lst, dest ); break;
            default : kDebugError( 1202, "Unknown action %d", ev->action() ); return;
	}
        connect( job, SIGNAL( result( KIO::Job * ) ),
                 receiver, SLOT( slotResult( KIO::Job * ) ) );
        ev->acceptAction(TRUE);
        ev->accept();
    }
    else
    {
        QStringList formats;

        for ( int i = 0; ev->format( i ); i++ )
            if ( *( ev->format( i ) ) )
                formats.append( ev->format( i ) );
        if ( formats.count() >= 1 )
        {
            kDebugInfo(1202,"Pasting to %s", dest.url().ascii());
            KIO::pasteData( dest, ev->data( formats.first() ) );
        }
    }
}

#include "konqdrag.moc"
