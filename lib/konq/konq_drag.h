/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __konqdrag_h__
#define __konqdrag_h__

#include <qdragobject.h>
#include <qrect.h>
#include <qstring.h>
#include <qiconview.h>

#include <kurl.h>

/*****************************************************************************
 *
 * Class KonqIconDrag
 *
 *****************************************************************************/

// Clipboard/dnd data for: Icons + URLS + isCut
class KonqIconDrag : public QIconDrag
{
    Q_OBJECT

public:
    KonqIconDrag( QWidget * dragSource, const char* name = 0 );
    virtual ~KonqIconDrag() {}

    const char* format( int i ) const;
    QByteArray encodedData( const char* mime ) const;

    void append( const QIconDragItem &item, const QRect &pr,
                 const QRect &tr, const QString &url );

    void setMoveSelection( bool move ) { m_bCutSelection = move; }

    static bool canDecode( const QMimeSource* e );

protected:
    QStringList urls;
    bool m_bCutSelection;
};

// Clipboard/dnd data for: URLS + isCut
class KonqDrag : public QUriDrag
{
public:
    static KonqDrag * newDrag( const KURL::List & urls, bool move, QWidget * dragSource = 0, const char* name = 0 );

protected:
    KonqDrag( const QStrList & urls, bool move, QWidget * dragSource, const char* name );

public:
    virtual ~KonqDrag() {}

    virtual const char* format( int i ) const;
    virtual QByteArray encodedData( const char* mime ) const;

    void setMoveSelection( bool move ) { m_bCutSelection = move; }

    // Returns true if the data was cut (used for KonqIconDrag too)
    static bool decodeIsCutSelection( const QMimeSource *e );

protected:
    bool m_bCutSelection;
    QStrList m_urls;
};

#endif
