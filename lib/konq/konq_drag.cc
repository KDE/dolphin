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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_drag.h"
#include <kdebug.h>
#include <k3urldrag.h>
//Added by qt3to4:
#include <Q3StrList>
#include <Q3CString>

KonqIconDrag::KonqIconDrag( QWidget * dragSource, const char* name )
  : Q3IconDrag( dragSource, name ),
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
    else if ( i == 4 ) //These two are imporant because they may end up being format 0,
                       //which is what KonqDirPart::updatePasteAction() checks
        return "text/plain;charset=ISO-8859-1";
    else if ( i == 5 ) //..as well as potentially for interoperability
        return "text/plain;charset=UTF-8";
    // warning, don't add anything here without checking KonqIconDrag2

    else return 0;
}

QByteArray KonqIconDrag::encodedData( const char* mime ) const
{
    QByteArray a;
    Q3CString mimetype( mime );
    if ( mimetype == "application/x-qiconlist" )
        a = Q3IconDrag::encodedData( mime );
    else if ( mimetype == "text/uri-list" ) {
        Q3CString s = urls.join( "\r\n" ).latin1();
        if( urls.count() > 0 )
            s.append( "\r\n" );
        a.resize( s.length() + 1 ); // trailing zero
        memcpy( a.data(), s.data(), s.length() + 1 );
    }
    else if ( mimetype == "application/x-kde-cutselection" ) {
        Q3CString s ( m_bCutSelection ? "1" : "0" );
        a.resize( s.length() + 1 ); // trailing zero
        memcpy( a.data(), s.data(), s.length() + 1 );
    }
    else if ( mimetype == "text/plain" ) {
        if (!urls.isEmpty())
        {
            QStringList uris;
            for (QStringList::ConstIterator it = urls.begin(); it != urls.end(); ++it)
                uris.append(K3URLDrag::stringToUrl((*it).latin1()).prettyURL());
            Q3CString s = uris.join( "\n" ).local8Bit();
            if( uris.count() > 1 )
                s.append( "\n" );
            a.resize( s.length()); // no trailing zero in clipboard text
            memcpy( a.data(), s.data(), s.length());
        }
    }
    else if ( mimetype.toLower() == "text/plain;charset=iso-8859-1")
    {
        if (!urls.isEmpty())
        {
            QStringList uris;

            for (QStringList::ConstIterator it = urls.begin(); it != urls.end(); ++it)
               uris.append(K3URLDrag::stringToUrl((*it).latin1()).url(0, 4)); // 4 for latin1

            Q3CString s = uris.join( "\n" ).latin1();
            if( uris.count() > 1 )
                s.append( "\n" );
            a.resize( s.length());
            memcpy( a.data(), s.data(), s.length());
        }
    }
    else if ( mimetype.toLower() == "text/plain;charset=utf-8")
    {
        if (!urls.isEmpty())
        {
            QStringList uris;
            for (QStringList::ConstIterator it = urls.begin(); it != urls.end(); ++it)
                uris.append(K3URLDrag::stringToUrl((*it).latin1()).prettyURL());
            Q3CString s = uris.join( "\n" ).utf8();
            if( uris.count() > 1 )
                s.append( "\n" );
            a.resize( s.length());
            memcpy( a.data(), s.data(), s.length());
        }
    }
    return a;
}

bool KonqIconDrag::canDecode( const QMimeSource* e )
{
    return  e->provides( "application/x-qiconlist" ) ||
      e->provides( "text/uri-list" ) ||
      e->provides( "application/x-kde-cutselection" );
}

void KonqIconDrag::append( const Q3IconDragItem &item, const QRect &pr,
                             const QRect &tr, const QString &url )
{
    Q3IconDrag::append( item, pr, tr );
    urls.append( url );
}

KonqIconDrag2::KonqIconDrag2( QWidget * dragSource )
    : KonqIconDrag( dragSource )
{
}

void KonqIconDrag2::append( const Q3IconDragItem &item, const QRect &pr,
                            const QRect &tr, const QString& url, const KURL &mostLocalURL )
{
    QString mostLocalURLStr = K3URLDrag::urlToString(mostLocalURL);
    m_kdeURLs.append( url );
    KonqIconDrag::append( item, pr, tr, mostLocalURLStr );
}

const char* KonqIconDrag2::format( int i ) const
{
    if ( i == 6 )
        return "application/x-kde-urilist";
    return KonqIconDrag::format( i );
}

QByteArray KonqIconDrag2::encodedData( const char* mime ) const
{
    QByteArray mimetype( mime );
    if ( mimetype == "application/x-kde-urilist" )
    {
        QByteArray a;
        int c=0;
        for (QStringList::ConstIterator it = m_kdeURLs.begin(); it != m_kdeURLs.end(); ++it) {
            QByteArray url = (*it).utf8();
            int l = url.length();
            a.resize(c+l+2);
            memcpy(a.data()+c, url.data(), l);
            memcpy(a.data()+c+l,"\r\n",2);
            c += l+2;
        }
        a.resize(c+1);
        a[c] = 0;
        return a;
    }
    return KonqIconDrag::encodedData( mime );
}

//

#if 0
// Used for KonqIconDrag too

#endif

#include "konq_drag.moc"
