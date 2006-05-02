/* This file is part of KDE
    Copyright (c) 2005-2006 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <qtest_kde.h>

#include <kurl.h>
#include <kdebug.h>
#include <assert.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <konqmimedata.h>
#include "konqmimedatatest.h"

#include "konqmimedatatest.moc"

QTEST_KDEMAIN( KonqMimeDataTest, NoGUI )

void KonqMimeDataTest::testPopulate()
{
    QMimeData* mimeData = new QMimeData;

    // Those URLs don't have to exist.
    KUrl mediaURL( "media:/hda1/tmp/Mat%C3%A9riel" );
    KUrl localURL( "file:///tmp/Mat%C3%A9riel" );
    //KUrl::List kdeURLs; kdeURLs << mediaURL;
    //KUrl::List mostLocalURLs; mostLocalURLs << localURL;

    KonqMimeData::populateMimeData( mimeData, mediaURL, localURL );

    QVERIFY( KUrl::List::canDecode( mimeData ) );
    const KUrl::List lst = KUrl::List::fromMimeData( mimeData );
    QCOMPARE( lst.count(), 1 );
    QCOMPARE( lst[0].url(), mediaURL.url() );

    const bool isCut = KonqMimeData::decodeIsCutSelection( mimeData );
    QVERIFY( !isCut );

    delete mimeData;
}

void KonqMimeDataTest::testCut()
{
    QMimeData* mimeData = new QMimeData;

    KUrl localURL1( "file:///tmp/Mat%C3%A9riel" );
    KUrl localURL2( "file:///tmp" );
    KUrl::List localURLs; localURLs << localURL1 << localURL2;

    KonqMimeData::populateMimeData( mimeData, KUrl::List(), localURLs, true /*cut*/ );

    QVERIFY( KUrl::List::canDecode( mimeData ) );
    const KUrl::List lst = KUrl::List::fromMimeData( mimeData );
    QCOMPARE( lst.count(), 2 );
    QCOMPARE( lst[0].url(), localURL1.url() );
    QCOMPARE( lst[1].url(), localURL2.url() );

    const bool isCut = KonqMimeData::decodeIsCutSelection( mimeData );
    QVERIFY( isCut );

    delete mimeData;
}
