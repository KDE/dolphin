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
//    else if ( i == 2 )
//	return "text/uri-iconlist";
    else return 0;
}

QByteArray KonqDrag::encodedData( const char* mime ) const
{
    QByteArray a;
    if ( QCString( mime ) == "application/x-qiconlist" )
	a = QIconDrag::encodedData( mime );
#if 0
    else if ( QCString( mime ) == "text/uri-iconlist" )
    {
      // Encode all the icon positions - reusing rules !
      QByteArray parentArray = QIconDrag::encodedData( mime );
      // Encode all urls
      QString s = urls.join( "\r\n" );

      // Append both - QIconDrag uses $@@$, let's use $@@@$ :-)
      QString complete( "%1$@@@$%2" );
      complete.arg( QString(parentArray) ).arg( s );

      a.resize( complete.length() );
      memcpy( a.data(), complete.latin1(), complete.length() );
    }
#endif
    else if ( QCString( mime ) == "text/uri-list" ) {
        QString s = urls.join( "\r\n" );
        a.resize( s.length() );
        memcpy( a.data(), s.latin1(), s.length() );
    }
    return a;
}

bool KonqDrag::canDecode( QMimeSource* e )
{
    return  e->provides( "application/x-qiconlist" ) ||
      // e->provides( "text/uri-iconlist" ) ||
      e->provides( "text/uri-list" );
}

#if 0
bool KonqDrag::decode( QMimeSource* e, QValueList<KonqDragItem> &lst )
{
    QByteArray ba = e->encodedData( "text/uri-iconlist" );
    if ( ba.size() ) {
	lst.clear();
	uint c = 0;
	QString s = ba.data();
        QStringList l = QStringList::split( "$@@@$", s );
        // We should have two items
        if ( l.count() != 2 )
        {
          kDebugError("Data has %d toplevel items instead of 2, KonqDrag can't decode !", l.count());
          return false;
        }

        if (!QIconDrag
        QStringList urllist = QStringList::split( l.last(), "\r\n" );

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
	    icon.setURL( s.latin1() + pos + 1 );
	    list_.append( icon );
	}
	return TRUE;
    }

    return FALSE;
}
#endif

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

void KonqDrag::append( const QIconDragItem &item, const QRect &pr,
                             const QRect &tr, const QString &url )
{
    QIconDrag::append( item, pr, tr );
    urls.append(url);
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
