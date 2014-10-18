/***************************************************************************
 *   Copyright (C) 2014 by Frank Reininghaus <frank78ac@googlemail.com>    *
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

#include <qtest.h>

#include "kitemviews/kitemrange.h"

#include <QVector>

Q_DECLARE_METATYPE(QVector<int>);
Q_DECLARE_METATYPE(KItemRangeList);

class KItemRangeTest : public QObject
{
    Q_OBJECT

private slots:
    void testFromSortedContainer_data();
    void testFromSortedContainer();
};

void KItemRangeTest::testFromSortedContainer_data()
{
    QTest::addColumn<QVector<int> >("sortedNumbers");
    QTest::addColumn<KItemRangeList>("expected");

    QTest::newRow("empty") << QVector<int>() << KItemRangeList();
    QTest::newRow("[1]") << (QVector<int>() << 1) << (KItemRangeList() << KItemRange(1, 1));
    QTest::newRow("[9]") << (QVector<int>() << 9) << (KItemRangeList() << KItemRange(9, 1));
    QTest::newRow("[1-2]") << (QVector<int>() << 1 << 2) << (KItemRangeList() << KItemRange(1, 2));
    QTest::newRow("[1-3]") << (QVector<int>() << 1 << 2 << 3) << (KItemRangeList() << KItemRange(1, 3));
    QTest::newRow("[1] [4]") << (QVector<int>() << 1 << 4) << (KItemRangeList() << KItemRange(1, 1) << KItemRange(4, 1));
    QTest::newRow("[1-3] [5]") << (QVector<int>() << 1 << 2 << 3 << 5) << (KItemRangeList() << KItemRange(1, 3) << KItemRange(5, 1));
    QTest::newRow("[1] [5-6]") << (QVector<int>() << 1 << 5 << 6) << (KItemRangeList() << KItemRange(1, 1) << KItemRange(5, 2));
    QTest::newRow("duplicates: 1 1") << (QVector<int>() << 1 << 1) << (KItemRangeList() << KItemRange(1, 1));
    QTest::newRow("duplicates: 1 1 1") << (QVector<int>() << 1 << 1 << 1) << (KItemRangeList() << KItemRange(1, 1));
    QTest::newRow("duplicates: 1 1 5") << (QVector<int>() << 1 << 1 << 5) << (KItemRangeList() << KItemRange(1, 1) << KItemRange(5, 1));
    QTest::newRow("duplicates: 1 5 5") << (QVector<int>() << 1 << 5 << 5) << (KItemRangeList() << KItemRange(1, 1) << KItemRange(5, 1));
    QTest::newRow("duplicates: 1 1 1 5") << (QVector<int>() << 1 << 1 << 1 << 5) << (KItemRangeList() << KItemRange(1, 1) << KItemRange(5, 1));
    QTest::newRow("duplicates: 1 5 5 5") << (QVector<int>() << 1 << 5 << 5 << 5) << (KItemRangeList() << KItemRange(1, 1) << KItemRange(5, 1));
    QTest::newRow("duplicates: 1 1 2") << (QVector<int>() << 1 << 1 << 2) << (KItemRangeList() << KItemRange(1, 2));
    QTest::newRow("duplicates: 1 2 2") << (QVector<int>() << 1 << 2 << 2) << (KItemRangeList() << KItemRange(1, 2));
    QTest::newRow("duplicates: 1 1 2 3") << (QVector<int>() << 1 << 1 << 2 << 3) << (KItemRangeList() << KItemRange(1, 3));
    QTest::newRow("duplicates: 1 2 2 3") << (QVector<int>() << 1 << 2 << 2 << 3) << (KItemRangeList() << KItemRange(1, 3));
    QTest::newRow("duplicates: 1 2 3 3") << (QVector<int>() << 1 << 2 << 3 << 3) << (KItemRangeList() << KItemRange(1, 3));
}

void KItemRangeTest::testFromSortedContainer()
{
    QFETCH(QVector<int>, sortedNumbers);
    QFETCH(KItemRangeList, expected);

    const KItemRangeList result = KItemRangeList::fromSortedContainer(sortedNumbers);
    QCOMPARE(expected, result);
}

QTEST_MAIN(KItemRangeTest)

#include "kitemrangetest.moc"
