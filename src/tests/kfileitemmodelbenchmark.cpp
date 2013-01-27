/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2013 by Frank Reininghaus <frank78ac@googlemail.com>    *
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

#include <qtest_kde.h>

#include <KGlobalSettings>

#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/private/kfileitemmodelsortalgorithm.h"

void myMessageOutput(QtMsgType type, const char* msg)
{
    switch (type) {
    case QtDebugMsg:
        break;
    case QtWarningMsg:
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", msg);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", msg);
        abort();
    default:
       break;
    }
}

Q_DECLARE_METATYPE(KFileItemList)
Q_DECLARE_METATYPE(KItemRangeList)

class KFileItemModelBenchmark : public QObject
{
    Q_OBJECT

public:
    KFileItemModelBenchmark();

private slots:
    void insertAndRemoveManyItems_data();
    void insertAndRemoveManyItems();

private:
    static KFileItemList createFileItemList(const QStringList& fileNames, const QString& urlPrefix = QLatin1String("file:///"));
};

KFileItemModelBenchmark::KFileItemModelBenchmark()
{
}

void KFileItemModelBenchmark::insertAndRemoveManyItems_data()
{
    QTest::addColumn<KFileItemList>("initialItems");
    QTest::addColumn<KFileItemList>("newItems");
    QTest::addColumn<KFileItemList>("removedItems");
    QTest::addColumn<KFileItemList>("expectedFinalItems");
    QTest::addColumn<KItemRangeList>("expectedItemsInserted");
    QTest::addColumn<KItemRangeList>("expectedItemsRemoved");

    QList<int> sizes;
    sizes << 1000 << 4000 << 16000 << 64000 << 256000;
    //sizes << 50000 << 100000 << 150000 << 200000 << 250000;

    foreach (int n, sizes) {
        QStringList allStrings;
        for (int i = 0; i < n; ++i) {
            allStrings << QString::number(i);
        }

        // We want to keep the sorting overhead in the benchmark low.
        // Therefore, we do not use natural sorting. However, this
        // means that our list is currently not sorted.
        allStrings.sort();

        KFileItemList all = createFileItemList(allStrings);

        KFileItemList firstHalf, secondHalf, even, odd;
        for (int i = 0; i < n; ++i) {
            if (i < n / 2) {
                firstHalf << all.at(i);
            } else {
                secondHalf << all.at(i);
            }

            if (i % 2 == 0) {
                even << all.at(i);
            } else {
                odd << all.at(i);
            }
        }

        KItemRangeList itemRangeListFirstHalf;
        itemRangeListFirstHalf << KItemRange(0, firstHalf.count());

        KItemRangeList itemRangeListSecondHalf;
        itemRangeListSecondHalf << KItemRange(firstHalf.count(), secondHalf.count());

        KItemRangeList itemRangeListOddInserted, itemRangeListOddRemoved;
        for (int i = 0; i < odd.count(); ++i) {
            // Note that the index in the KItemRange is the index of
            // the model *before* the items have been inserted.
            itemRangeListOddInserted << KItemRange(i + 1, 1);
            itemRangeListOddRemoved << KItemRange(2 * i + 1, 1);
        }

        const int bufferSize = 128;
        char buffer[bufferSize];

        snprintf(buffer, bufferSize, "all--n=%i", n);
        QTest::newRow(buffer) << all << KFileItemList() << KFileItemList() << all << KItemRangeList() << KItemRangeList();

        snprintf(buffer, bufferSize, "1st half + 2nd half--n=%i", n);
        QTest::newRow(buffer) << firstHalf << secondHalf << KFileItemList() << all << itemRangeListSecondHalf << KItemRangeList();

        snprintf(buffer, bufferSize, "2nd half + 1st half--n=%i", n);
        QTest::newRow(buffer) << secondHalf << firstHalf << KFileItemList() << all << itemRangeListFirstHalf << KItemRangeList();

        snprintf(buffer, bufferSize, "even + odd--n=%i", n);
        QTest::newRow(buffer) << even << odd << KFileItemList() << all << itemRangeListOddInserted << KItemRangeList();

        snprintf(buffer, bufferSize, "all - 2nd half--n=%i", n);
        QTest::newRow(buffer) << all << KFileItemList() << secondHalf << firstHalf << KItemRangeList() << itemRangeListSecondHalf;

        snprintf(buffer, bufferSize, "all - 1st half--n=%i", n);
        QTest::newRow(buffer) << all << KFileItemList() << firstHalf << secondHalf << KItemRangeList() << itemRangeListFirstHalf;

        snprintf(buffer, bufferSize, "all - odd--n=%i", n);
        QTest::newRow(buffer) << all << KFileItemList() << odd << even << KItemRangeList() << itemRangeListOddRemoved;
    }
}

void KFileItemModelBenchmark::insertAndRemoveManyItems()
{
    QFETCH(KFileItemList, initialItems);
    QFETCH(KFileItemList, newItems);
    QFETCH(KFileItemList, removedItems);
    QFETCH(KFileItemList, expectedFinalItems);
    QFETCH(KItemRangeList, expectedItemsInserted);
    QFETCH(KItemRangeList, expectedItemsRemoved);

    KFileItemModel model;

    // Avoid overhead caused by natural sorting
    // and determining the isDir/isLink roles.
    model.m_naturalSorting = false;
    model.setRoles(QSet<QByteArray>() << "text");

    QSignalSpy spyItemsInserted(&model, SIGNAL(itemsInserted(KItemRangeList)));
    QSignalSpy spyItemsRemoved(&model, SIGNAL(itemsRemoved(KItemRangeList)));

    QBENCHMARK {
        model.slotClear();
        model.slotNewItems(initialItems);
        model.slotCompleted();
        QCOMPARE(model.count(), initialItems.count());

        if (!newItems.isEmpty()) {
            model.slotNewItems(newItems);
            model.slotCompleted();
        }
        QCOMPARE(model.count(), initialItems.count() + newItems.count());

        if (!removedItems.isEmpty()) {
            model.removeItems(removedItems);
        }
        QCOMPARE(model.count(), initialItems.count() + newItems.count() - removedItems.count());
    }

    QVERIFY(model.isConsistent());

    for (int i = 0; i < model.count(); ++i) {
        QCOMPARE(model.fileItem(i), expectedFinalItems.at(i));
    }

    if (!expectedItemsInserted.empty()) {
        QVERIFY(!spyItemsInserted.empty());
        const KItemRangeList actualItemsInserted = spyItemsInserted.last().first().value<KItemRangeList>();
        QCOMPARE(actualItemsInserted, expectedItemsInserted);
    }

    if (!expectedItemsRemoved.empty()) {
        QVERIFY(!spyItemsRemoved.empty());
        const KItemRangeList actualItemsRemoved = spyItemsRemoved.last().first().value<KItemRangeList>();
        QCOMPARE(actualItemsRemoved, expectedItemsRemoved);
    }
}

KFileItemList KFileItemModelBenchmark::createFileItemList(const QStringList& fileNames, const QString& prefix)
{
    // Suppress 'file does not exist anymore' messages from KFileItemPrivate::init().
    qInstallMsgHandler(myMessageOutput);

    KFileItemList result;
    foreach (const QString& name, fileNames) {
        const KUrl url(prefix + name);
        const KFileItem item(url, QString(), KFileItem::Unknown);
        result << item;
    }
    return result;
}

QTEST_KDEMAIN(KFileItemModelBenchmark, NoGUI)

#include "kfileitemmodelbenchmark.moc"
