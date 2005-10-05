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
    else if ( mimetype.lower() == "text/plain;charset=iso-8859-1")
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
    else if ( mimetype.lower() == "text/plain;charset=utf-8")
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

KonqDrag * KonqDrag::newDrag( const KURL::List & urls, bool cut, QWidget * dragSource, const char* name )
{
    // See KURLDrag::newDrag
    Q3StrList uris;
    KURL::List::ConstIterator uit = urls.begin();
    KURL::List::ConstIterator uEnd = urls.end();
    // Get each URL encoded in utf8 - and since we get it in escaped
    // form on top of that, .latin1() is fine.
    for ( ; uit != uEnd ; ++uit )
        uris.append( K3URLDrag::urlToString( *uit ).latin1() );
    return new KonqDrag( uris, cut, dragSource, name );
}

// urls must be already checked to have hostname in file URLs
KonqDrag::KonqDrag( const Q3StrList & urls, bool cut, QWidget * dragSource, const char* name )
  : Q3UriDrag( urls, dragSource, name ),
    m_bCutSelection( cut ), m_urls( urls )
{}

// urls must be already checked to have hostname in file URLs
KonqDrag::KonqDrag( const KURL::List & urls, const KURL::List& mostLocalUrls,
                    bool cut, QWidget * dragSource )
    : Q3UriDrag( dragSource ),
      m_bCutSelection( cut )
{
    QList<QByteArray> uris;
    KURL::List::ConstIterator uit = urls.begin();
    KURL::List::ConstIterator uEnd = urls.end();
    // Get each URL encoded in utf8 - and since we get it in escaped
    // form on top of that, .latin1() is fine.
    for ( ; uit != uEnd ; ++uit )
        uris.append( K3URLDrag::urlToString( *uit ).latin1() );
    setUris( uris ); // we give the KDE uris to QUriDrag. TODO: do the opposite in KDE4 and add a m_mostLocalUris member.

    uit = mostLocalUrls.begin();
    uEnd = mostLocalUrls.end();
    for ( ; uit != uEnd ; ++uit )
        m_urls.append( K3URLDrag::urlToString( *uit ).latin1() );
    // we keep the most-local-uris in m_urls for exporting those as text/plain (for xmms)
}

const char* KonqDrag::format( int i ) const
{
    if ( i == 0 )
	return "text/uri-list";
    else if ( i == 1 )
        return "application/x-kde-cutselection";
    else if ( i == 2 )
        return "text/plain";
    else if ( i == 3 )
        return "application/x-kde-urilist";
    else return 0;
}

QByteArray KonqDrag::encodedData( const char* mime ) const
{
    QByteArray a;
    Q3CString mimetype( mime );
    if ( mimetype == "text/uri-list" )
    {
        // Code taken from QUriDrag::setUris
        int c=0;
        for (Q3StrListIterator it(m_urls); *it; ++it) {
             int l = qstrlen(*it);
             a.resize(c+l+2);
             memcpy(a.data()+c,*it,l);
             memcpy(a.data()+c+l,"\r\n",2);
             c+=l+2;
        }
        a.resize(c+1);
        a[c] = 0;
    }
    else if ( mimetype == "application/x-kde-urilist" )
    {
        return Q3UriDrag::encodedData( "text/uri-list" );
    }
    else if ( mimetype == "application/x-kde-cutselection" )
    {
        Q3CString s ( m_bCutSelection ? "1" : "0" );
	a.resize( s.length() + 1 ); // trailing zero
	memcpy( a.data(), s.data(), s.length() + 1 );
    }
    else if ( mimetype == "text/plain" )
    {
        QStringList uris;
        for (Q3StrListIterator it(m_urls); *it; ++it)
            uris.append(K3URLDrag::stringToUrl(*it).prettyURL());
        Q3CString s = uris.join( "\n" ).local8Bit();
        if( uris.count() > 1 )
            s.append( "\n" );
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
    kdDebug(1203) << "KonqDrag::decodeIsCutSelection : a=" << Q3CString(a.data(), a.size() + 1) << endl;
    return (a.at(0) == '1'); // true if 1
  }
}

#include "konq_drag.moc"
