/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kbookmark.h"
#include <kdebug.h>
#include <kglobal.h>
#include <kmimetype.h>
#include <krun.h>
#include <kstddirs.h>
#include <kstringhandler.h>
#include <kurl.h>
#include <klineeditdlg.h>
#include <qtextstream.h>
#include <klocale.h>
#include <assert.h>

#include "konq_faviconmgr.h"

KBookmarkGroup::KBookmarkGroup()
 : KBookmark( QDomElement() )
{
}

KBookmarkGroup::KBookmarkGroup( QDomElement elem )
 : KBookmark(elem)
{
}

QString KBookmarkGroup::groupAddress() const
{
    if (m_address.isEmpty())
        m_address = address();
    return m_address;
}

bool KBookmarkGroup::isOpen() const
{
    return element.attribute("folded") == "no"; // default is: folded
}

KBookmark KBookmarkGroup::first() const
{
    return KBookmark( nextKnownTag( element.firstChild().toElement(), true ) );
}

KBookmark KBookmarkGroup::previous( const KBookmark & current ) const
{
    return KBookmark( nextKnownTag( current.element.previousSibling().toElement(), false ) );
}

KBookmark KBookmarkGroup::next( const KBookmark & current ) const
{
    return KBookmark( nextKnownTag( current.element.nextSibling().toElement(), true ) );
}

QDomElement KBookmarkGroup::nextKnownTag( QDomElement start, bool goNext ) const
{
    static const QString & bookmark = KGlobal::staticQString("bookmark");
    static const QString & folder = KGlobal::staticQString("folder");
    static const QString & separator = KGlobal::staticQString("separator");
    QDomElement elem = start;
    while ( !elem.isNull() )
    {
        QString tag = elem.tagName();
        if (tag == folder || tag == bookmark || tag == separator)
            break;
        if (goNext)
            elem = elem.nextSibling().toElement();
        else
            elem = elem.previousSibling().toElement();
    }
    return elem;
}

KBookmarkGroup KBookmarkGroup::createNewFolder( const QString & text )
{
    QString txt( text );
    if ( text.isEmpty() )
    {
        KLineEditDlg l( i18n("New Folder:"), "", 0L );
        l.setCaption( i18n("Create new bookmark folder in %1").arg( parentGroup().text() ) );
        if ( l.exec() )
            txt = l.text();
        else
            return KBookmarkGroup();
    }

    ASSERT(!element.isNull());
    QDomDocument doc = element.ownerDocument();
    QDomElement groupElem = doc.createElement( "folder" );
    element.appendChild( groupElem );
    QDomElement textElem = doc.createElement( "title" );
    groupElem.appendChild( textElem );
    textElem.appendChild( doc.createTextNode( txt ) );
    return KBookmarkGroup(groupElem);
}

KBookmark KBookmarkGroup::createNewSeparator()
{
    ASSERT(!element.isNull());
    QDomDocument doc = element.ownerDocument();
    ASSERT(!doc.isNull());
    QDomElement sepElem = doc.createElement( "separator" );
    element.appendChild( sepElem );
    return KBookmark(sepElem);
}

bool KBookmarkGroup::moveItem( const KBookmark & item, const KBookmark & after )
{
    QDomNode n;
    if ( !after.isNull() )
        n = element.insertAfter( item.element, after.element );
    else // first child
    {
        if ( element.firstChild().isNull() ) // Empty element -> set as real first child
            n = element.insertBefore( item.element, QDomElement() );

        // we have to skip everything up to the first valid child
        QDomElement firstChild = nextKnownTag(element.firstChild().toElement(), true);
        if ( !firstChild.isNull() )
            n = element.insertBefore( item.element, firstChild );
        else
        {
            // No real first child -> append after the <title> etc.
            n = element.appendChild( item.element );
        }
    }
    return (!n.isNull());
}

KBookmark KBookmarkGroup::addBookmark( const QString & text, const KURL & url, const QString & icon )
{
    //kdDebug(1203) << "KBookmarkGroup::addBookmark " << text << " into " << m_address << endl;
    QDomDocument doc = element.ownerDocument();
    QDomElement elem = doc.createElement( "bookmark" );
    element.appendChild( elem );
    elem.setAttribute( "href", url.url( 0, QFont::Unicode ) ); // write utf8 URL
    QString _icon = icon;
    if ( _icon.isEmpty() )
        _icon = KonqFavIconMgr::iconForURL( url.url() );
    if ( _icon.isEmpty() )
        _icon = KMimeType::iconForURL( url );
    elem.setAttribute( "icon", _icon );

    QDomElement textElem = doc.createElement( "title" );
    elem.appendChild( textElem );
    textElem.appendChild( doc.createTextNode( text ) );

    return KBookmark(elem);
}

void KBookmarkGroup::deleteBookmark( KBookmark bk )
{
    element.removeChild( bk.element );
}

bool KBookmarkGroup::isToolbarGroup() const
{
    return ( element.attribute("toolbar") == "yes" );
}

QDomElement KBookmarkGroup::findToolbar() const
{
    if ( element.attribute("toolbar") == "yes" )
        return element;
    QDomElement e = element.firstChild().toElement();
    for ( ; !e.isNull() ; e = e.nextSibling().toElement() )
    {
        // Search among the "folder" children only
        if ( e.tagName() == "folder" )
        {
            if ( e.attribute("toolbar") == "yes" )
                return e;
            else
            {
                QDomElement result = KBookmarkGroup(e).findToolbar();
                if (!result.isNull())
                    return result;
            }
        }
    }
    return QDomElement();
}

//////

bool KBookmark::isGroup() const
{
    QString tag = element.tagName();
    return ( tag == "folder"
             || tag == "xbel" ); // don't forget the toplevel group
}

bool KBookmark::isSeparator() const
{
    return (element.tagName() == "separator");
}

QString KBookmark::text() const
{
    return KStringHandler::csqueeze( fullText() );
}

QString KBookmark::fullText() const
{
    if (isSeparator())
        return i18n("--- separator ---");

    return element.namedItem("title").toElement().text();
}

KURL KBookmark::url() const
{
    return KURL(element.attribute("href"), QFont::Unicode); // Decode it from utf8
}

QString KBookmark::icon() const
{
    QString icon = element.attribute("icon");
    if ( icon.isEmpty() )
        // Default icon depends on URL for bookmarks, and is default directory
        // icon for groups.
        if ( isGroup() )
            icon = "bookmark_folder";
        else
            if ( isSeparator() )
                icon = "eraser"; // whatever
            else
                icon = KMimeType::iconForURL( url() );
    return icon;
}

KBookmarkGroup KBookmark::parentGroup() const
{
    return KBookmarkGroup( element.parentNode().toElement() );
}

KBookmarkGroup KBookmark::toGroup() const
{
    ASSERT( isGroup() );
    return KBookmarkGroup(element);
}

QString KBookmark::address() const
{
    if ( element.tagName() == "xbel" )
        return ""; // not QString::null !
    else
    {
        // Use keditbookmarks's DEBUG_ADDRESSES flag to debug this code :)
        QDomElement parent = element.parentNode().toElement();
        if(parent.isNull())
        {
          ASSERT(!parent.isNull());
          return "ERROR"; // Avoid an infinite loop
        }
        KBookmarkGroup group( parent );
        QString parentAddress = group.address();
        uint counter = 0;
        // Implementation note: we don't use QDomNode's childNode list because we
        // would have to skip "TEXT", which KBookmarkGroup already does for us.
        for ( KBookmark bk = group.first() ; !bk.isNull() ; bk = group.next(bk), ++counter )
        {
            if ( bk.element == element )
                return parentAddress + "/" + QString::number(counter);
        }
        kdWarning() << "KBookmark::address : this can't happen!  " << parentAddress << endl;
        return "ERROR";
    }
}

KBookmark KBookmark::standaloneBookmark( const QString & text, const KURL & url, const QString & icon )
{
    QDomDocument doc("xbel");
    QDomElement elem = doc.createElement("xbel");
    doc.appendChild( elem );
    KBookmarkGroup grp( elem );
    grp.addBookmark( text, url, icon );
    return grp.first();
}
