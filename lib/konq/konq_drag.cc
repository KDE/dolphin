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
#include <kdebug.h>

KonqIconDrag::KonqIconDrag( QWidget * dragSource, const char* name )
  : QIconDrag( dragSource, name ),
    m_bCutSelection( false )
{
}

const char* KonqIconDrag::format( int i ) const
{
    if ( i == 0 )
	return "application/x-qiconlist";
    else if ( i == 1 )
	return "text/uri-list";
    else if ( i == 2 )
        return "application/x-kde-cutselection";
    else if ( i == 3 )
        return "text/plain";
    else return 0;
}

QByteArray KonqIconDrag::encodedData( const char* mime ) const
{
    QByteArray a;
    QCString mimetype( mime );
    if ( mimetype == "application/x-qiconlist" )
        a = QIconDrag::encodedData( mime );
    else if ( mimetype == "text/uri-list" ) {
        QCString s = urls.join( "\r\n" ).latin1();
        a.resize( s.length() + 1 ); // trailing zero
        memcpy( a.data(), s.data(), s.length() + 1 );
    }
    else if ( mimetype == "application/x-kde-cutselection" ) {
        QCString s ( m_bCutSelection ? "1" : "0" );
        a.resize( s.length() + 1 ); // trailing zero
        memcpy( a.data(), s.data(), s.length() + 1 );
    }
    else if ( mimetype == "text/plain" ) {
        QStringList uris;
        for (QStringList::ConstIterator it = urls.begin(); it != urls.end(); ++it)
            uris.append(KURL((*it).latin1(), QFont::Unicode).prettyURL());
        QCString s = uris.join( "\n" ).local8Bit();
        a.resize( s.length() + 1 ); // trailing zero
        memcpy( a.data(), s.data(), s.length() + 1 );
    }
    return a;
}

bool KonqIconDrag::canDecode( const QMimeSource* e )
{
    return  e->provides( "application/x-qiconlist" ) ||
      e->provides( "text/uri-list" ) ||
      e->provides( "application/x-kde-cutselection" );
}

void KonqIconDrag::append( const QIconDragItem &item, const QRect &pr,
                             const QRect &tr, const QString &url )
{
    QIconDrag::append( item, pr, tr );
    urls.append(url);
}

//

KonqDrag * KonqDrag::newDrag( const KURL::List & urls, bool move, QWidget * dragSource, const char* name )
{
    // See KURLDrag::newDrag
    QStrList uris;
    KURL::List::ConstIterator uit = urls.begin();
    KURL::List::ConstIterator uEnd = urls.end();
    // Get each URL encoded in utf8 - and since we get it in escaped
    // form on top of that, .latin1() is fine.
    for ( ; uit != uEnd ; ++uit )
        uris.append( (*uit).url(0, QFont::Unicode).latin1() );
    return new KonqDrag( uris, move, dragSource, name );
}

KonqDrag::KonqDrag( const QStrList & urls, bool move, QWidget * dragSource, const char* name )
  : QUriDrag( urls, dragSource, name ),
    m_bCutSelection( move ), m_urls( urls )
{}

const char* KonqDrag::format( int i ) const
{
    if ( i == 0 )
	return "text/uri-list";
    else if ( i == 1 )
        return "application/x-kde-cutselection";
    else if ( i == 2 )
        return "text/plain";
    else return 0;
}

QByteArray KonqDrag::encodedData( const char* mime ) const
{
    QByteArray a;
    QCString mimetype( mime );
    if ( mimetype == "text/uri-list" )
        return QUriDrag::encodedData( mime );
    else if ( mimetype == "application/x-kde-cutselection" ) {
        QCString s ( m_bCutSelection ? "1" : "0" );
	a.resize( s.length() + 1 ); // trailing zero
	memcpy( a.data(), s.data(), s.length() + 1 );
    }
    else if ( mimetype == "text/plain" )
    {
        QStringList uris;
        for (QStrListIterator it(m_urls); *it; ++it)
            uris.append(KURL(*it, QFont::Unicode).prettyURL());
        QCString s = uris.join( "\n" ).local8Bit();
        a.resize( s.length() + 1 ); // trailing zero
        memcpy( a.data(), s.data(), s.length() + 1 );
    }
    return a;
}

//

// Used for KonqIconDrag too

bool KonqDrag::decodeIsCutSelection( const QMimeSource *e )
{
  QByteArray a = e->encodedData( "application/x-kde-cutselection" );
  if ( a.isEmpty() )
    return false;
  else
  {
    kdDebug(1203) << "KonqDrag::decodeIsCutSelection : a=" << QCString(a) << endl;
    return !strcmp(QCString( a ).data(), "1"); // true if 1
  }
}

#include "konq_drag.moc"
