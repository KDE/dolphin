/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "drag.h"
#include <kurl.h>
#include <kurldrag.h>
#include <kdebug.h>

KEBDrag * KEBDrag::newDrag( const KBookmark & bookmark, QWidget * dragSource, const char * name )
{
    KURL::List urls;
    urls.append( bookmark.url() );

    // See KURLDrag::newDrag
    QStrList uris;
    KURL::List::ConstIterator uit = urls.begin();
    KURL::List::ConstIterator uEnd = urls.end();
    // Get each URL encoded in utf8 - and since we get it in escaped
    // form on top of that, .latin1() is fine.
    for ( ; uit != uEnd ; ++uit )
        uris.append( (*uit).url(0, QFont::Unicode).latin1() );
    return new KEBDrag( bookmark, uris, dragSource, name );
}

KEBDrag::KEBDrag( const KBookmark & bookmark, const QStrList & urls,
                  QWidget * dragSource, const char * name )
    : QUriDrag( urls, dragSource, name ), m_bookmark( bookmark )
{

}

const char* KEBDrag::format( int i ) const
{
    if ( i == 0 )
        return "application/x-xbel";
    else if ( i == 1 )
	return "text/uri-list";
    else return 0;
}

QByteArray KEBDrag::encodedData( const char* mime ) const
{
    QByteArray a;
    QCString mimetype( mime );
    if ( mimetype == "text/uri-list" )
        return QUriDrag::encodedData( mime );
    else if ( mimetype == "application/x-xbel" )
    {
        QDomDocument doc("xbel"); // plan for the future :)
        QDomElement elem = doc.createElement("xbel");
        doc.appendChild( elem );
        elem.appendChild( m_bookmark.internalElement().cloneNode( true /* deep */ ) );
        kdDebug() << "KEBDrag::encodedData " << doc.toCString() << endl;
        a = doc.toCString();
    }
    return a;
}

bool KEBDrag::canDecode( const QMimeSource * e )
{
    return e->provides("text/uri-list") || e->provides("application/x-xbel");
}

KBookmark KEBDrag::decode( const QMimeSource * e )
{
    if ( e->provides("application/x-xbel") )
    {
        QCString s( e->encodedData("application/x-xbel") );
        kdDebug() << "KEBDrag::decode s=" << s << endl;
        QDomDocument doc;
        doc.setContent( s );
        QDomElement elem = doc.documentElement();
        QDomNodeList children = elem.childNodes();
        ASSERT(children.count()==1);
        return KBookmark( children.item(0).toElement() );
    }
    if ( e->provides("text/uri-list") )
    {
        KURL::List m_lstDragURLs;
        if ( KURLDrag::decode( e, m_lstDragURLs ) )
        {
            QDomDocument doc("xbel");
            QDomElement elem = doc.createElement("xbel");
            doc.appendChild( elem );
            if ( m_lstDragURLs.count() > 1 )
                kdWarning() << "Only first URL inserted, known limitation" << endl;
            //kdDebug() << "KEBDrag::decode url=" << m_lstDragURLs.first().url() << endl;
            KBookmarkGroup grp( elem );
            grp.addBookmark( m_lstDragURLs.first().fileName(), m_lstDragURLs.first().url() );
            return grp.first();
        }
    }
    return KBookmark(QDomElement());
}
