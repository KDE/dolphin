/* This file is part of the KDE libraries
    Copyright (c) 2005 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <kurl.h>
#include <kdebug.h>
#include <kurldrag.h>
#include <assert.h>
#include <qiconview.h>
#include <kapplication.h>
#include <kcmdlineargs.h>

#include <konq_drag.h>

// for future qttestlib porting :)
#define VERIFY assert
#define COMPARE( a, b ) assert( (a) == (b) )

void testKonqIconDrag2()
{
    // Those URLs don't have to exist.
    KURL mediaURL = "media:/hda1/tmp/Mat%C3%A9riel";
    KURL localURL = "file:///tmp/Mat%C3%A9riel";
    KonqIconDrag2 iconDrag( 0 );
    QIconDragItem item;
    iconDrag.append( item, QRect( 1, 2, 3, 4 ), QRect( 5, 6, 7, 8 ),
                     mediaURL.url(), localURL.url() );


    VERIFY( KURLDrag::canDecode( &iconDrag ) );
    KURL::List lst;
    KURLDrag::decode( &iconDrag, lst );
    VERIFY( !lst.isEmpty() );
    COMPARE( lst.count(), 1 );
    kdDebug() << "lst[0]=" << lst << endl;
    kdDebug() << "mediaURL=" << mediaURL << endl;
    COMPARE( lst[0].url(), mediaURL.url() );
}

int main(int argc, char *argv[])
{
  KApplication::disableAutoDcopRegistration();
  KCmdLineArgs::init( argc, argv, "kurltest", 0, 0, 0, 0 );
  KApplication app; // GUI needed for QPixmaps

  testKonqIconDrag2();
  return 0;
}
