/* This file is part of KDE
    Copyright (c) 2006 David Faure <faure@kde.org>

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

#include "favicontest.h"
#include <qtest_kde.h>
#include <konq_faviconmgr.h>
#include <kio/netaccess.h>
#include <QDateTime>
#include <kmimetype.h>

#include "favicontest.moc"

QTEST_KDEMAIN( FavIconTest, NoGUI )

static const char* s_hostUrl = "http://www.kde.org";
static const char* s_iconUrl = "http://www.kde.org/favicon.ico";
static const char* s_altIconUrl = "http://www.koffice.org/favicon.ico";

static int s_downloadTime; // in ms

enum NetworkAccess { Unknown, Yes, No } s_networkAccess = Unknown;
static bool checkNetworkAccess() {
    if ( s_networkAccess == Unknown ) {
        QString tmpFile;
        QTime tm;
        tm.start();
        if( KIO::NetAccess::download( KUrl( s_iconUrl ), tmpFile, 0 ) ) {
            KIO::NetAccess::removeTempFile( tmpFile );
            s_networkAccess = Yes;
            s_downloadTime = tm.elapsed();
	    qDebug( "Network access OK. Download time %d", s_downloadTime );
        } else {
            qWarning( "%s", qPrintable( KIO::NetAccess::lastErrorString() ) );
            s_networkAccess = No;
        }
    }
    return s_networkAccess;
}

class TestFavIconMgr : public KonqFavIconMgr
{
public:
    TestFavIconMgr() : KonqFavIconMgr( 0 ) {}

    virtual void notifyChange( bool isHost, QString hostOrURL, QString iconName ) {
        m_isHost = isHost;
        m_hostOrURL = hostOrURL;
        m_iconName = iconName;
        emit changed();
    }

    bool m_isHost;
    QString m_hostOrURL;
    QString m_iconName;
};

static void waitForSignal( TestFavIconMgr* mgr )
{
    // Wait for signals - indefinitely
    QEventLoop eventLoop;
    QObject::connect(mgr, SIGNAL(changed()), &eventLoop, SLOT(quit()));
    eventLoop.exec( QEventLoop::ExcludeUserInputEvents );

    // or: wait for signals for a certain amount of time...
    // QTest::qWait( s_downloadTime * 2 + 1000 );
    // qDebug() << QDateTime::currentDateTime() << " waiting done";
}

// To avoid hitting the cache, we first set the icon to s_altIconUrl (koffice.org),
// then back to s_iconUrl (kde.org) (to avoid messing up the favicons for the user ;)
void FavIconTest::testSetIconForURL()
{
    if ( !checkNetworkAccess() )
        QSKIP( "no network access", SkipAll );

    TestFavIconMgr mgr;
    QSignalSpy spy( &mgr, SIGNAL( changed() ) );
    QVERIFY( spy.isValid() );
    QCOMPARE( spy.count(), 0 );

    KonqFavIconMgr::setIconForURL( KUrl( s_hostUrl ), KUrl( s_altIconUrl ) );

    qDebug( "called first setIconForURL, waiting" );
    waitForSignal( &mgr );

    QCOMPARE( spy.count(), 1 );
    QCOMPARE( mgr.m_isHost, false );
    QCOMPARE( mgr.m_hostOrURL, QString( s_hostUrl ) );
    QCOMPARE( mgr.m_iconName, QString( "favicons/www.koffice.org" ) );

    KonqFavIconMgr::setIconForURL( KUrl( s_hostUrl ), KUrl( s_iconUrl ) );

    qDebug( "called setIconForURL again, waiting" );
    waitForSignal( &mgr );

    QCOMPARE( spy.count(), 2 );
    QCOMPARE( mgr.m_isHost, false );
    QCOMPARE( mgr.m_hostOrURL, QString( s_hostUrl ) );
    QCOMPARE( mgr.m_iconName, QString( "favicons/www.kde.org" ) );
}

void FavIconTest::testIconForURL()
{
    QString icon = KMimeType::favIconForURL( KUrl( s_hostUrl ) );
    QCOMPARE( icon, QString( "favicons/www.kde.org" ) );
}

#if 0
// downloadHostIcon does nothing if the icon is available already, so how to test this?
// We could delete the icon first, but given that we have a different KDEHOME than the kded module,
// that's not really easy...
void FavIconTest::testDownloadHostIcon()
{
    if ( !checkNetworkAccess() )
        QSKIP( "no network access", SkipAll );

    TestFavIconMgr mgr;
    QSignalSpy spy( &mgr, SIGNAL( changed() ) );
    QVERIFY( spy.isValid() );
    QCOMPARE( spy.count(), 0 );

    qDebug( "called downloadHostIcon, waiting" );
    KonqFavIconMgr::downloadHostIcon( KUrl( s_hostUrl ) );
    waitForSignal( &mgr );

    QCOMPARE( spy.count(), 1 );
    QCOMPARE( mgr.m_isHost, true );
    QCOMPARE( mgr.m_hostOrURL, KUrl( s_hostUrl ).host() );
    qDebug( "icon: %s", qPrintable( mgr.m_iconName ) );
}
#endif
