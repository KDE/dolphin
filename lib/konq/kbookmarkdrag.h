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

#ifndef __kebdrag_h
#define __kebdrag_h

#include <qdragobject.h>
#include <kbookmark.h>

// Clipboard/dnd data : URLs + XML for bookmarks
class KBookmarkDrag : public QUriDrag
{
public:
    static KBookmarkDrag * newDrag( const KBookmark & bookmark,
                                    QWidget * dragSource = 0,
                                    const char * name = 0 );
protected:
    KBookmarkDrag( const KBookmark & bookmark,
                   const QStrList & urls,
                   QWidget * dragSource,
                   const char * name );
public:
    virtual ~KBookmarkDrag() {}

    virtual const char* format( int i ) const;
    virtual QByteArray encodedData( const char* mime ) const;

    static bool canDecode( const QMimeSource * e );
    static KBookmark decode( const QMimeSource * e );

protected:
    KBookmark m_bookmark;
    QDomDocument m_doc;
    class KBookmarkDragPrivate;
    KBookmarkDragPrivate * d;
};
#endif
