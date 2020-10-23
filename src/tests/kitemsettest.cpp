/*
 * SPDX-FileCopyrightText: 2013 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemviews/kitemset.h"

#include <QTest>

Q_DECLARE_METATYPE(KItemRangeList)

/**
 * Converts a KItemRangeList to a KItemSet.
 */
KItemSet KItemRangeList2KItemSet(const KItemRangeList& itemRanges)
{
    KItemSet result;
    for (const KItemRange& range : itemRanges) {
        for (int i = range.index; i < range.index + range.count; ++i) {
            result.insert(i);
        }
    }
    return result;
}

/**
 * Converts a KItemRangeList to a QSet<int>.
 */
QSet<int> KItemRangeList2QSet(const KItemRangeList& itemRanges)
{
    QSet<int> result;
    for (const KItemRange& range : itemRanges) {
        for (int i = range.index; i < range.index + range.count; ++i) {
            result.insert(i);
        }
    }
    return result;
}

/**
 * Converts a KItemRangeList to a QVector<int>.
 */
QVector<int> KItemRangeList2QVector(const KItemRangeList& itemRanges)
{
    QVector<int> result;
    for (const KItemRange& range : itemRanges) {
        for (int i = range.index; i < range.index + range.count; ++i) {
            result.append(i);
        }
    }
    return result;
}

/**
 * Converts a KItemSet to a QSet<int>.
 */
static QSet<int> KItemSet2QSet(const KItemSet& itemSet)
{
    QSet<int> result;
    for (int i : itemSet) {
        result.insert(i);
    }

    // Check that the conversion was successful.
    Q_ASSERT(itemSet.count() == result.count());

    for (int i : qAsConst(itemSet)) {
        Q_ASSERT(result.contains(i));
    }

    for (int i : qAsConst(result)) {
        Q_ASSERT(itemSet.contains(i));
    }

    return result;
}


/**
 * The main test class.
 */
class KItemSetTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testConstruction_data();
    void testConstruction();
    void testIterators_data();
    void testIterators();
    void testFind_data();
    void testFind();
    void testChangingOneItem_data();
    void testChangingOneItem();
    void testAddSets_data();
    void testAddSets();
    /*
    void testSubtractSets_data();
    void testSubtractSets();
    */
    void testSymmetricDifference_data();
    void testSymmetricDifference();

private:
    QHash<const char*, KItemRangeList> m_testCases;
};

void KItemSetTest::initTestCase()
{
    m_testCases.insert("empty", KItemRangeList());
    m_testCases.insert("[0]", KItemRangeList() << KItemRange(0, 1));
    m_testCases.insert("[1]", KItemRangeList() << KItemRange(1, 1));
    m_testCases.insert("[1, 2]", KItemRangeList() << KItemRange(1, 2));
    m_testCases.insert("[1, 2] [5]", KItemRangeList() << KItemRange(1, 2) << KItemRange(5, 1));
    m_testCases.insert("[1] [4, 5]", KItemRangeList() << KItemRange(1, 1) << KItemRange(4, 2));
    m_testCases.insert("[1, 2] [4, 5]", KItemRangeList() << KItemRange(1, 2) << KItemRange(4, 2));
    m_testCases.insert("[1, 5]", KItemRangeList() << KItemRange(1, 5));
    m_testCases.insert("[1, 2] [4, 5] [7] [9, 10] [13] [20, 25] [30]",
                       KItemRangeList() << KItemRange(1, 2) << KItemRange(4, 2) << KItemRange(7, 1) << KItemRange(9, 2) << KItemRange(20, 6) << KItemRange(30, 1));
    m_testCases.insert("[-10, -1]", KItemRangeList() << KItemRange(-10, 10));
    m_testCases.insert("[-10, 0]", KItemRangeList() << KItemRange(-10, 11));
    m_testCases.insert("[-10, 1]", KItemRangeList() << KItemRange(-10, 12));
    m_testCases.insert("[0, 9]", KItemRangeList() << KItemRange(0, 10));
    m_testCases.insert("[0, 19]", KItemRangeList() << KItemRange(0, 10));
}

void KItemSetTest::testConstruction_data()
{
    QTest::addColumn<KItemRangeList>("itemRanges");

    QHash<const char*, KItemRangeList>::const_iterator it = m_testCases.constBegin();
    const QHash<const char*, KItemRangeList>::const_iterator end = m_testCases.constEnd();

    while (it != end) {
        QTest::newRow(it.key()) << it.value();
        ++it;
    }
}

void KItemSetTest::testConstruction()
{
    QFETCH(KItemRangeList, itemRanges);

    KItemSet itemSet = KItemRangeList2KItemSet(itemRanges);
    QSet<int> itemsQSet = KItemRangeList2QSet(itemRanges);

    QVERIFY(itemSet.isValid());
    QVERIFY(itemSet.count() == itemsQSet.count());
    QCOMPARE(KItemSet2QSet(itemSet), itemsQSet);

    // Test copy constructor.
    KItemSet copy(itemSet);
    QCOMPARE(itemSet, copy);
    copy.clear();
    QVERIFY(itemSet != copy || itemSet.isEmpty());

    // Clear the set.
    itemSet.clear();
    QVERIFY(itemSet.isEmpty());
    QCOMPARE(itemSet.count(), 0);
}

void KItemSetTest::testIterators_data()
{
    QTest::addColumn<KItemRangeList>("itemRanges");

    QHash<const char*, KItemRangeList>::const_iterator it = m_testCases.constBegin();
    const QHash<const char*, KItemRangeList>::const_iterator end = m_testCases.constEnd();

    while (it != end) {
        QTest::newRow(it.key()) << it.value();
        ++it;
    }
}

/**
 * Verify that the iterators work exactly like their counterparts for the
 * equivalent QVector<int>.
 */
void KItemSetTest::testIterators()
{
    QFETCH(KItemRangeList, itemRanges);

    KItemSet itemSet = KItemRangeList2KItemSet(itemRanges);
    QVector<int> itemsQVector = KItemRangeList2QVector(itemRanges);

    QVERIFY(itemSet.isValid());
    QVERIFY(itemSet.count() == itemsQVector.count());

    if (itemSet.isEmpty()) {
        QVERIFY(itemSet.isEmpty());
        QVERIFY(itemSet.begin() == itemSet.end());
        QVERIFY(itemSet.constBegin() == itemSet.constEnd());
    } else {
        QVERIFY(!itemSet.isEmpty());
        QVERIFY(itemSet.begin() != itemSet.end());
        QVERIFY(itemSet.constBegin() != itemSet.constEnd());

        const int min = itemsQVector.first();
        const int max = itemsQVector.last();

        QCOMPARE(*itemSet.begin(), min);
        QCOMPARE(*itemSet.constBegin(), min);
        QCOMPARE(itemSet.first(), min);

        QCOMPARE(*(--itemSet.end()), max);
        QCOMPARE(*(--itemSet.constEnd()), max);
        QCOMPARE(itemSet.last(), max);
    }

    // Test iterating using the different iterators.
    QVector<int> testQVector;
    for (KItemSet::iterator it = itemSet.begin(), end = itemSet.end(); it != end; ++it) {
        testQVector.append(*it);
    }
    QCOMPARE(testQVector, itemsQVector);

    testQVector.clear();
    for (KItemSet::const_iterator it = itemSet.constBegin(), end = itemSet.constEnd(); it != end; ++it) {
        testQVector.append(*it);
    }
    QCOMPARE(testQVector, itemsQVector);

    testQVector.clear();
    for (int i : itemSet) {
        testQVector.append(i);
    }
    QCOMPARE(testQVector, itemsQVector);

    // Verify that both variants of the (const)iterator's operator++ and
    // operator-- functions behave exactly like their QVector equivalents.
    KItemSet::iterator it1 = itemSet.begin();
    KItemSet::iterator it2 = itemSet.begin();
    KItemSet::const_iterator constIt1 = itemSet.constBegin();
    KItemSet::const_iterator constIt2 = itemSet.constBegin();
    QVector<int>::iterator vectorIt1 = itemsQVector.begin();
    QVector<int>::iterator vectorIt2 = itemsQVector.begin();
    QVector<int>::const_iterator vectorConstIt1 = itemsQVector.constBegin();
    QVector<int>::const_iterator vectorConstIt2 = itemsQVector.constBegin();

    while (it1 != itemSet.end()) {
        if (it1 != --itemSet.end()) {
            QCOMPARE(*(++it1), *(++vectorIt1));
            QCOMPARE(*(++constIt1), *(++vectorConstIt1));
        } else {
            QCOMPARE(++it1, itemSet.end());
            QCOMPARE(++vectorIt1, itemsQVector.end());
            QCOMPARE(++constIt1, itemSet.constEnd());
            QCOMPARE(++vectorConstIt1, itemsQVector.constEnd());
        }

        QCOMPARE(*(it2++), *(vectorIt2++));
        QCOMPARE(*(constIt2++), *(vectorConstIt2++));

        QCOMPARE(it1, it2);
        QCOMPARE(constIt1, constIt2);
        QCOMPARE(KItemSet::const_iterator(it1), constIt1);
    }

    QCOMPARE(it1, itemSet.end());
    QCOMPARE(it2, itemSet.end());
    QCOMPARE(constIt1, itemSet.constEnd());
    QCOMPARE(constIt2, itemSet.constEnd());
    QCOMPARE(vectorIt1, itemsQVector.end());
    QCOMPARE(vectorIt2, itemsQVector.end());
    QCOMPARE(vectorConstIt1, itemsQVector.constEnd());
    QCOMPARE(vectorConstIt2, itemsQVector.constEnd());

    while (it1 != itemSet.begin()) {
        QCOMPARE(*(--it1), *(--vectorIt1));
        QCOMPARE(*(--constIt1), *(--vectorConstIt1));

        if (it2 != itemSet.end()) {
            QCOMPARE(*(it2--), *(vectorIt2--));
            QCOMPARE(*(constIt2--), *(vectorConstIt2--));
        } else {
            QCOMPARE(it2--, itemSet.end());
            QCOMPARE(vectorIt2--, itemsQVector.end());
            QCOMPARE(constIt2--, itemSet.constEnd());
            QCOMPARE(vectorConstIt2--, itemsQVector.constEnd());
        }

        QCOMPARE(it1, it2);
        QCOMPARE(constIt1, constIt2);
        QCOMPARE(KItemSet::const_iterator(it1), constIt1);
    }

    QCOMPARE(it1, itemSet.begin());
    QCOMPARE(it2, itemSet.begin());
    QCOMPARE(constIt1, itemSet.constBegin());
    QCOMPARE(constIt2, itemSet.constBegin());
    QCOMPARE(vectorIt1, itemsQVector.begin());
    QCOMPARE(vectorIt2, itemsQVector.begin());
    QCOMPARE(vectorConstIt1, itemsQVector.constBegin());
    QCOMPARE(vectorConstIt2, itemsQVector.constBegin());
}

void KItemSetTest::testFind_data()
{
    QTest::addColumn<KItemRangeList>("itemRanges");

    QHash<const char*, KItemRangeList>::const_iterator it = m_testCases.constBegin();
    const QHash<const char*, KItemRangeList>::const_iterator end = m_testCases.constEnd();

    while (it != end) {
        QTest::newRow(it.key()) << it.value();
        ++it;
    }
}

/**
 * Test all functions that find items:
 * contains(int), find(int), constFind(int)
 */
void KItemSetTest::testFind()
{
    QFETCH(KItemRangeList, itemRanges);

    KItemSet itemSet = KItemRangeList2KItemSet(itemRanges);
    QSet<int> itemsQSet = KItemRangeList2QSet(itemRanges);

    QVERIFY(itemSet.isValid());
    QVERIFY(itemSet.count() == itemsQSet.count());

    // Find the minimum and maximum items.
    int min;
    int max;

    if (itemSet.isEmpty()) {
        // Use some arbitrary values for the upcoming tests.
        min = 0;
        max = 5;
    } else {
        min = *itemSet.begin();
        max = *(--itemSet.end());
    }

    // Test contains(int), find(int), and constFind(int)
    // for items between min - 2 and max + 2.
    for (int i = min - 2; i <= max + 2; ++i) {
        const KItemSet::iterator it = itemSet.find(i);
        const KItemSet::const_iterator constIt = itemSet.constFind(i);
        QCOMPARE(KItemSet::const_iterator(it), constIt);

        if (itemsQSet.contains(i)) {
            QVERIFY(itemSet.contains(i));
            QCOMPARE(*it, i);
            QCOMPARE(*constIt, i);
        } else {
            QVERIFY(!itemSet.contains(i));
            QCOMPARE(it, itemSet.end());
            QCOMPARE(constIt, itemSet.constEnd());
        }
    }
}

void KItemSetTest::testChangingOneItem_data()
{
    QTest::addColumn<KItemRangeList>("itemRanges");

    QHash<const char*, KItemRangeList>::const_iterator it = m_testCases.constBegin();
    const QHash<const char*, KItemRangeList>::const_iterator end = m_testCases.constEnd();

    while (it != end) {
        QTest::newRow(it.key()) << it.value();
        ++it;
    }
}

/**
 * Test all functions that change a single item:
 * insert(int), remove(int), erase(KItemSet::iterator)
 */
void KItemSetTest::testChangingOneItem()
{
    QFETCH(KItemRangeList, itemRanges);

    KItemSet itemSet = KItemRangeList2KItemSet(itemRanges);
    QSet<int> itemsQSet = KItemRangeList2QSet(itemRanges);

    QVERIFY(itemSet.isValid());
    QVERIFY(itemSet.count() == itemsQSet.count());

    // Find the minimum and maximum items.
    int min;
    int max;

    if (itemSet.isEmpty()) {
        // Use some arbitrary values for the upcoming tests.
        min = 0;
        max = 5;
    } else {
        min = *itemSet.begin();
        max = *(--itemSet.end());
    }

    // Test insert(int), remove(int), and erase(KItemSet::iterator)
    // for items between min - 2 and max + 2.
    for (int i = min - 2; i <= max + 2; ++i) {

        // Test insert(int).
        {
            KItemSet tmp(itemSet);
            const KItemSet::iterator insertedIt = tmp.insert(i);
            QCOMPARE(*insertedIt, i);

            QVERIFY(tmp.isValid());
            QVERIFY(tmp.contains(i));

            QSet<int> expectedQSet = itemsQSet;
            expectedQSet.insert(i);
            QCOMPARE(KItemSet2QSet(tmp), expectedQSet);

            if (!itemSet.contains(i)) {
                QVERIFY(itemSet != tmp);
                QCOMPARE(tmp.count(), itemSet.count() + 1);
            } else {
                QCOMPARE(itemSet, tmp);
            }

            QCOMPARE(i, *tmp.find(i));
            QCOMPARE(i, *tmp.constFind(i));

            // Erase the new item and check that we get the old KItemSet back.
            tmp.erase(tmp.find(i));
            QVERIFY(tmp.isValid());
            QVERIFY(!tmp.contains(i));

            if (!itemSet.contains(i)) {
                QCOMPARE(itemSet, tmp);
            }

            expectedQSet.remove(i);
            QCOMPARE(KItemSet2QSet(tmp), expectedQSet);
        }

        // Test remove(int).
        {
            KItemSet tmp(itemSet);
            const bool removed = tmp.remove(i);

            QCOMPARE(removed, itemSet.contains(i));

            QVERIFY(tmp.isValid());
            QVERIFY(!tmp.contains(i));

            QSet<int> expectedQSet = itemsQSet;
            expectedQSet.remove(i);
            QCOMPARE(KItemSet2QSet(tmp), expectedQSet);

            if (itemSet.contains(i)) {
                QVERIFY(itemSet != tmp);
                QCOMPARE(tmp.count(), itemSet.count() - 1);
            } else {
                QCOMPARE(itemSet, tmp);
            }

            QCOMPARE(tmp.end(), tmp.find(i));
            QCOMPARE(tmp.constEnd(), tmp.constFind(i));
        }

        // Test erase(KItemSet::iterator).
        if (itemSet.contains(i)) {
            KItemSet tmp(itemSet);
            KItemSet::iterator it = tmp.find(i);
            it = tmp.erase(it);

            QVERIFY(tmp.isValid());
            QVERIFY(!tmp.contains(i));

            QSet<int> expectedQSet = itemsQSet;
            expectedQSet.remove(i);
            QCOMPARE(KItemSet2QSet(tmp), expectedQSet);

            if (itemSet.contains(i)) {
                QVERIFY(itemSet != tmp);
                QCOMPARE(tmp.count(), itemSet.count() - 1);
            } else {
                QCOMPARE(itemSet, tmp);
            }

            QCOMPARE(tmp.end(), tmp.find(i));
            QCOMPARE(tmp.constEnd(), tmp.constFind(i));

            // Check the returned value, now contained in 'it'.
            if (i == max) {
                QCOMPARE(it, tmp.end());
            } else {
                // it now points to the next item.
                QVERIFY(tmp.contains(*it));
                for (int j = i; j < *it; ++j) {
                    QVERIFY(!tmp.contains(j));
                }
            }
        }
    }

    // Clear the set.
    itemSet.clear();
    QVERIFY(itemSet.isEmpty());
    QCOMPARE(itemSet.count(), 0);
}

void KItemSetTest::testAddSets_data()
{
    QTest::addColumn<KItemRangeList>("itemRanges1");
    QTest::addColumn<KItemRangeList>("itemRanges2");

    QHash<const char*, KItemRangeList>::const_iterator it1 = m_testCases.constBegin();
    const QHash<const char*, KItemRangeList>::const_iterator end = m_testCases.constEnd();

    while (it1 != end) {
        QHash<const char*, KItemRangeList>::const_iterator it2 = m_testCases.constBegin();

        while (it2 != end) {
            QByteArray name = it1.key() + QByteArray(" + ") + it2.key();
            QTest::newRow(name) << it1.value() << it2.value();
            ++it2;
        }

        ++it1;
    }
}

void KItemSetTest::testAddSets()
{
    QFETCH(KItemRangeList, itemRanges1);
    QFETCH(KItemRangeList, itemRanges2);

    KItemSet itemSet1 = KItemRangeList2KItemSet(itemRanges1);
    QSet<int> itemsQSet1 = KItemRangeList2QSet(itemRanges1);

    KItemSet itemSet2 = KItemRangeList2KItemSet(itemRanges2);
    QSet<int> itemsQSet2 = KItemRangeList2QSet(itemRanges2);

    KItemSet sum = itemSet1 + itemSet2;
    QSet<int> sumQSet = itemsQSet1 + itemsQSet2;

    QCOMPARE(sum.count(), sumQSet.count());
    QCOMPARE(KItemSet2QSet(sum), sumQSet);
}

void KItemSetTest::testSymmetricDifference_data()
{
    QTest::addColumn<KItemRangeList>("itemRanges1");
    QTest::addColumn<KItemRangeList>("itemRanges2");

    QHash<const char*, KItemRangeList>::const_iterator it1 = m_testCases.constBegin();
    const QHash<const char*, KItemRangeList>::const_iterator end = m_testCases.constEnd();

    while (it1 != end) {
        QHash<const char*, KItemRangeList>::const_iterator it2 = m_testCases.constBegin();

        while (it2 != end) {
            QByteArray name = it1.key() + QByteArray(" ^ ") + it2.key();
            QTest::newRow(name) << it1.value() << it2.value();
            ++it2;
        }

        ++it1;
    }
}

void KItemSetTest::testSymmetricDifference()
{
    QFETCH(KItemRangeList, itemRanges1);
    QFETCH(KItemRangeList, itemRanges2);

    KItemSet itemSet1 = KItemRangeList2KItemSet(itemRanges1);
    QSet<int> itemsQSet1 = KItemRangeList2QSet(itemRanges1);

    KItemSet itemSet2 = KItemRangeList2KItemSet(itemRanges2);
    QSet<int> itemsQSet2 = KItemRangeList2QSet(itemRanges2);

    KItemSet symmetricDifference = itemSet1 ^ itemSet2;
    QSet<int> symmetricDifferenceQSet = (itemsQSet1 - itemsQSet2) + (itemsQSet2 - itemsQSet1);

    QCOMPARE(symmetricDifference.count(), symmetricDifferenceQSet.count());
    QCOMPARE(KItemSet2QSet(symmetricDifference), symmetricDifferenceQSet);

    // Check commutativity.
    QCOMPARE(itemSet2 ^ itemSet1, symmetricDifference);

    // Some more checks:
    // itemSet1 ^ symmetricDifference == itemSet2,
    // itemSet2 ^ symmetricDifference == itemSet1.
    QCOMPARE(itemSet1 ^ symmetricDifference, itemSet2);
    QCOMPARE(itemSet2 ^ symmetricDifference, itemSet1);
}


QTEST_GUILESS_MAIN(KItemSetTest)

#include "kitemsettest.moc"
