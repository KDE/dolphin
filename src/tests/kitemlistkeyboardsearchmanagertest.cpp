/*
 *   SPDX-FileCopyrightText: 2011 Frank Reininghaus <frank78ac@googlemail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemviews/private/kitemlistkeyboardsearchmanager.h"

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <source_location>

class KItemListKeyboardSearchManagerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasicKeyboardSearch();
    void testAbortedKeyboardSearch();
    void testRepeatedKeyPress();
    void testPressShift();

private:
    KItemListKeyboardSearchManager m_keyboardSearchManager;

    // Helper function to verify signal parameters
    void verifySignal(QSignalSpy &spy,
                      const QString &expectedString,
                      bool expectedSearchFromNextItem,
                      const std::source_location &location = std::source_location::current())
    {
        if (spy.count() != 1) {
            QTest::qFail(QString("Compared values are not the same\n"
                                 "   Actual   (spy.count()): %1\n"
                                 "   Expected (1)          : 1")
                             .arg(spy.count())
                             .toUtf8()
                             .constData(),
                         location.file_name(),
                         location.line());
            return;
        }

        QList<QVariant> arguments = spy.takeFirst();

        if (arguments.size() != 3) {
            QTest::qFail(QString("Compared values are not the same\n"
                                 "   Actual   (arguments.size()): %1\n"
                                 "   Expected (3)               : 3")
                             .arg(arguments.size())
                             .toUtf8()
                             .constData(),
                         location.file_name(),
                         location.line());
            return;
        }

        QString actualText = arguments.at(0).toString();
        if (actualText != expectedString) {
            QTest::qFail(QString("Compared values are not the same\n"
                                 "   Actual   (arguments.at(0).toString()): \"%1\"\n"
                                 "   Expected (QString(\"%2\"))            : \"%2\"")
                             .arg(actualText)
                             .arg(expectedString)
                             .toUtf8()
                             .constData(),
                         location.file_name(),
                         location.line());
            return;
        }

        bool actualBool = arguments.at(1).toBool();
        if (actualBool != expectedSearchFromNextItem) {
            QTest::qFail(QString("Compared values are not the same\n"
                                 "   Actual   (arguments.at(1).toBool()): %1\n"
                                 "   Expected (%2)                      : %2")
                             .arg(actualBool ? "true" : "false")
                             .arg(expectedSearchFromNextItem ? "true" : "false")
                             .toUtf8()
                             .constData(),
                         location.file_name(),
                         location.line());
            return;
        }
        // Ignore the third parameter (bool* found)
    }
};

void KItemListKeyboardSearchManagerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

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
    verifySignal(spy, "f", true);

    m_keyboardSearchManager.addKeys("i");
    verifySignal(spy, "fi", false);

    m_keyboardSearchManager.addKeys("l");
    verifySignal(spy, "fil", false);

    m_keyboardSearchManager.addKeys("e");
    verifySignal(spy, "file", false);
}

void KItemListKeyboardSearchManagerTest::testAbortedKeyboardSearch()
{
    // Set the timeout to a small value (the default is 5000 milliseconds)
    // to save time when running this test.
    m_keyboardSearchManager.setTimeout(100);

    QSignalSpy spy(&m_keyboardSearchManager, &KItemListKeyboardSearchManager::changeCurrentItem);
    QVERIFY(spy.isValid());

    m_keyboardSearchManager.addKeys("f");
    verifySignal(spy, "f", true);

    m_keyboardSearchManager.addKeys("i");
    verifySignal(spy, "fi", false);

    // If the delay between two key presses is larger than the chosen timeout,
    // a new search is started. We add a small safety margin to avoid race conditions.
    QTest::qWait(m_keyboardSearchManager.timeout() + 10);

    m_keyboardSearchManager.addKeys("l");
    verifySignal(spy, "l", true);

    m_keyboardSearchManager.addKeys("e");
    verifySignal(spy, "le", false);

    // the selection was deselected, for instance with Esc or a click outside the selection
    m_keyboardSearchManager.slotSelectionChanged(KItemSet(), KItemSet() << 1);

    m_keyboardSearchManager.addKeys("a");
    verifySignal(spy, "a", true);
}

void KItemListKeyboardSearchManagerTest::testRepeatedKeyPress()
{
    // With the new implementation, pressing the same key repeatedly will:
    // 1. First attempt a full match with the complete string
    // 2. If full match fails, fall back to rapid navigation with the first character

    QSignalSpy spy(&m_keyboardSearchManager, &KItemListKeyboardSearchManager::changeCurrentItem);
    QVERIFY(spy.isValid());

    // First key press - new search, only one signal
    m_keyboardSearchManager.addKeys("p");
    QCOMPARE(spy.count(), 1);
    verifySignal(spy, "p", true);

    // Second key press - repeating character, two signals
    m_keyboardSearchManager.addKeys("p");
    QCOMPARE(spy.count(), 2);
    // First signal: full string match attempt
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.size(), 3);
    QCOMPARE(arguments.at(0).toString(), QString("pp"));
    QCOMPARE(arguments.at(1).toBool(), false);
    // Second signal: rapid navigation fallback
    arguments = spy.takeFirst();
    QCOMPARE(arguments.size(), 3);
    QCOMPARE(arguments.at(0).toString(), QString("p"));
    QCOMPARE(arguments.at(1).toBool(), true);

    // Third key press - repeating character, two signals
    m_keyboardSearchManager.addKeys("p");
    QCOMPARE(spy.count(), 2);
    // First signal: full string match attempt
    arguments = spy.takeFirst();
    QCOMPARE(arguments.size(), 3);
    QCOMPARE(arguments.at(0).toString(), QString("ppp"));
    QCOMPARE(arguments.at(1).toBool(), false);
    // Second signal: rapid navigation fallback
    arguments = spy.takeFirst();
    QCOMPARE(arguments.size(), 3);
    QCOMPARE(arguments.at(0).toString(), QString("p"));
    QCOMPARE(arguments.at(1).toBool(), true);

    // Now press another key -> only one signal for non-repeating character
    m_keyboardSearchManager.addKeys("q");
    QCOMPARE(spy.count(), 1);
    verifySignal(spy, "pppq", false);
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
    verifySignal(spy, "a", true);

    m_keyboardSearchManager.addKeys("");
    QCOMPARE(spy.count(), 0);

    m_keyboardSearchManager.addKeys("_");
    verifySignal(spy, "a_", false);

    m_keyboardSearchManager.addKeys("b");
    verifySignal(spy, "a_b", false);
}

QTEST_GUILESS_MAIN(KItemListKeyboardSearchManagerTest)

#include "kitemlistkeyboardsearchmanagertest.moc"
