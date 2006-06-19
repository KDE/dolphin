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

#include "historymgrtest.h"
#include <qtest_kde.h>
#include <konq_historymgr.h>
#include <kio/netaccess.h>
#include <QDateTime>

#include "historymgrtest.moc"

QTEST_KDEMAIN( HistoryMgrTest, NoGUI )

void HistoryMgrTest::testGetSetMaxCount()
{
    KonqHistoryManager mgr;
    const int oldMaxCount = mgr.maxCount();
    qDebug( "oldMaxCount=%d", oldMaxCount );
    mgr.emitSetMaxCount( 4242 );
    QTest::qWait( 100 ); // ### fragile. We have no signal to wait for, so we must just wait a little bit...
    // Yes this is just a set+get test, but given that it goes via DBUS before changing the member variable
    // we do test quite a lot with it. We can't really instanciate two KonqHistoryManagers (same dbus path),
    // so we'd need two processes to test the dbus signal 'for real', if the setter changed the member var...
    QCOMPARE( mgr.maxCount(), 4242 );
    mgr.emitSetMaxCount( oldMaxCount );
    QTest::qWait( 100 ); // ### fragile. Wait again otherwise the change will be lost
    QCOMPARE( mgr.maxCount(), oldMaxCount );
}

void HistoryMgrTest::testGetSetMaxAge()
{
    KonqHistoryManager mgr;
    const int oldMaxAge = mgr.maxAge();
    qDebug( "oldMaxAge=%d", oldMaxAge );
    mgr.emitSetMaxAge( 4242 );
    QTest::qWait( 100 ); // ### fragile. We have no signal to wait for, so we must just wait a little bit...
    QCOMPARE( mgr.maxAge(), 4242 );
    mgr.emitSetMaxAge( oldMaxAge );
    QTest::qWait( 100 ); // ### fragile. Wait again otherwise the change will be lost
    QCOMPARE( mgr.maxAge(), oldMaxAge );
}

static void waitForAddedSignal( KonqHistoryManager* mgr )
{
    QEventLoop eventLoop;
    QObject::connect(mgr, SIGNAL(entryAdded(KonqHistoryEntry)), &eventLoop, SLOT(quit()));
    eventLoop.exec( QEventLoop::ExcludeUserInputEvents );
}

static void waitForRemovedSignal( KonqHistoryManager* mgr )
{
    QEventLoop eventLoop;
    QObject::connect(mgr, SIGNAL(entryRemoved(KonqHistoryEntry)), &eventLoop, SLOT(quit()));
    eventLoop.exec( QEventLoop::ExcludeUserInputEvents );
}

void HistoryMgrTest::testAddHistoryEntry()
{
    KonqHistoryManager mgr;
    qRegisterMetaType<KonqHistoryEntry>("KonqHistoryEntry");
    QSignalSpy addedSpy( &mgr, SIGNAL(entryAdded(KonqHistoryEntry)) );
    QSignalSpy removedSpy( &mgr, SIGNAL(entryRemoved(KonqHistoryEntry)) );
    const KUrl url( "http://user@historymgrtest.org/" );
    const QString typedUrl = "http://www.example.net";
    const QString title = "The Title";
    mgr.addPending( url, typedUrl, title );

    waitForAddedSignal( &mgr );

    QCOMPARE( addedSpy.count(), 1 );
    QCOMPARE( removedSpy.count(), 0 );
    QList<QVariant> args = addedSpy[0];
    QCOMPARE( args.count(), 1 );
    KonqHistoryEntry entry = qvariant_cast<KonqHistoryEntry>( args[0] );
    QCOMPARE( entry.url.url(), url.url() );
    QCOMPARE( entry.typedUrl, typedUrl );
    QCOMPARE( entry.title, QString() ); // not set yet, still pending
    QCOMPARE( (int)entry.numberOfTimesVisited, 1 );

    // Now confirm it
    mgr.confirmPending( url, typedUrl, title );
    // ## alternate code path: mgr.removePending()

    waitForAddedSignal( &mgr );

    QCOMPARE( addedSpy.count(), 2 );
    QCOMPARE( removedSpy.count(), 0 );
    args = addedSpy[1];
    QCOMPARE( args.count(), 1 );
    entry = qvariant_cast<KonqHistoryEntry>( args[0] );
    QCOMPARE( entry.url.url(), url.url() );
    QCOMPARE( entry.typedUrl, typedUrl );
    QCOMPARE( entry.title, title ); // now it's there
    QCOMPARE( (int)entry.numberOfTimesVisited, 1 );

    // Now clean it up

    mgr.emitRemoveFromHistory( url );

    waitForRemovedSignal( &mgr );

    QCOMPARE( removedSpy.count(), 1 );
    QCOMPARE( addedSpy.count(), 2 ); // unchanged
    args = removedSpy[0];
    QCOMPARE( args.count(), 1 );
    entry = qvariant_cast<KonqHistoryEntry>( args[0] );
    QCOMPARE( entry.url.url(), url.url() );
    QCOMPARE( entry.typedUrl, typedUrl );
    QCOMPARE( entry.title, title );
    QCOMPARE( (int)entry.numberOfTimesVisited, 1 );
}
