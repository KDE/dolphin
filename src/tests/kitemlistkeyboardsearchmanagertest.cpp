/***************************************************************************
 *   Copyright (C) 2011 by Frank Reininghaus <frank78ac@googlemail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "kitemviews/private/kitemlistkeyboardsearchmanager.h"

#include <QTest>
#include <QSignalSpy>

class KItemListKeyboardSearchManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void testBasicKeyboardSearch();
    void testAbortedKeyboardSearch();
    void testRepeatedKeyPress();
    void testPressShift();

private:
    KItemListKeyboardSearchManager m_keyboardSearchManager;
};

void KItemListKeyboardSearchManagerTest::init()
{
    // Make sure that the previous search string is cleared
    m_keyboardSearchManager.cancelSearch();
}

void KItemListKeyboardSearchManagerTest::testBasicKeyboardSearch()
{
    QSignalSpy spy(&m_keyboardSearchManager, &KItemListKeyboardSearchManager::changeCurrentItem);
    QVERIFY(spy.isValid());

    m_keyboardSearchManager.addKeys("f");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "f" << true);

    m_keyboardSearchManager.addKeys("i");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "fi" << false);

    m_keyboardSearchManager.addKeys("l");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "fil" << false);

    m_keyboardSearchManager.addKeys("e");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "file" << false);
}

void KItemListKeyboardSearchManagerTest::testAbortedKeyboardSearch()
{
    // Set the timeout to a small value (the default is 5000 milliseconds)
    // to save time when running this test.
    m_keyboardSearchManager.setTimeout(100);

    QSignalSpy spy(&m_keyboardSearchManager, &KItemListKeyboardSearchManager::changeCurrentItem);
    QVERIFY(spy.isValid());

    m_keyboardSearchManager.addKeys("f");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "f" << true);

    m_keyboardSearchManager.addKeys("i");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "fi" << false);

    // If the delay between two key presses is larger than the chosen timeout,
    // a new search is started. We add a small safety margin to avoid race conditions.
    QTest::qWait(m_keyboardSearchManager.timeout() + 10);

    m_keyboardSearchManager.addKeys("l");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "l" << true);

    m_keyboardSearchManager.addKeys("e");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "le" << false);
}

void KItemListKeyboardSearchManagerTest::testRepeatedKeyPress()
{
    // If the same key is pressed repeatedly, the next matching item should be highlighted after
    // each key press. To achieve, that, the manager emits the changeCurrentItem(QString,bool)
    // signal, where
    // 1. the string contains the repeated key only once, and
    // 2. the bool searchFromNextItem is true.

    QSignalSpy spy(&m_keyboardSearchManager, &KItemListKeyboardSearchManager::changeCurrentItem);
    QVERIFY(spy.isValid());

    m_keyboardSearchManager.addKeys("p");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "p" << true);

    m_keyboardSearchManager.addKeys("p");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "p" << true);

    m_keyboardSearchManager.addKeys("p");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "p" << true);

    // Now press another key -> the search string contains all pressed keys
    m_keyboardSearchManager.addKeys("q");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "pppq" << false);
}

void KItemListKeyboardSearchManagerTest::testPressShift()
{
    // If the user presses Shift, i.e., to get a character like '_',
    // KItemListController calls the addKeys(QString) method with an empty
    // string. Make sure that this does not reset the current search. See
    // https://bugs.kde.org/show_bug.cgi?id=321286

    QSignalSpy spy(&m_keyboardSearchManager, &KItemListKeyboardSearchManager::changeCurrentItem);
    QVERIFY(spy.isValid());

    // Simulate that the user enters "a_b".
    m_keyboardSearchManager.addKeys("a");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "a" << true);

    m_keyboardSearchManager.addKeys("");
    QCOMPARE(spy.count(), 0);

    m_keyboardSearchManager.addKeys("_");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "a_" << false);

    m_keyboardSearchManager.addKeys("b");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "a_b" << false);
}

QTEST_GUILESS_MAIN(KItemListKeyboardSearchManagerTest)

#include "kitemlistkeyboardsearchmanagertest.moc"
