/*
 *   SPDX-FileCopyrightText: 2011 Frank Reininghaus <frank78ac@googlemail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

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
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "f" << false);

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
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "f" << false);

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

    // the selection was deselected, for instance with Esc or a click outside the selection
    m_keyboardSearchManager.slotSelectionChanged(KItemSet(), KItemSet() << 1);

    m_keyboardSearchManager.addKeys("a");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "a" << false);
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
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "p" << false);

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
    QCOMPARE(spy.takeFirst(), QList<QVariant>() << "a" << false);

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
