/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2013 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <random>

#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/private/kfileitemmodelsortalgorithm.h"

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)

    switch (type) {
    case QtDebugMsg:
        break;
    case QtWarningMsg:
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", msg.toLocal8Bit().data());
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", msg.toLocal8Bit().data());
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

private Q_SLOTS:
    void initTestCase();
    void insertAndRemoveManyItems_data();
    void insertAndRemoveManyItems();

private:
    static KFileItemList createFileItemList(const QStringList &fileNames, const QString &urlPrefix = QLatin1String("file:///"));
};

KFileItemModelBenchmark::KFileItemModelBenchmark()
{
}

void KFileItemModelBenchmark::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
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
    sizes << 100000;

    for (int n : std::as_const(sizes)) {
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
    model.setRoles({"text"});

    QSignalSpy spyItemsInserted(&model, &KFileItemModel::itemsInserted);
    QSignalSpy spyItemsRemoved(&model, &KFileItemModel::itemsRemoved);

    QBENCHMARK {
        model.slotClear();
        model.slotItemsAdded(model.directory(), initialItems);
        model.slotCompleted();
        QCOMPARE(model.count(), initialItems.count());

        if (!newItems.isEmpty()) {
            model.slotItemsAdded(model.directory(), newItems);
            model.slotCompleted();
        }
        QCOMPARE(model.count(), initialItems.count() + newItems.count());

        if (!removedItems.isEmpty()) {
            model.slotItemsDeleted(removedItems);
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

KFileItemList KFileItemModelBenchmark::createFileItemList(const QStringList &fileNames, const QString &prefix)
{
    // Suppress 'file does not exist anymore' messages from KFileItemPrivate::init().
    qInstallMessageHandler(myMessageOutput);

    KFileItemList result;
    for (const QString &name : fileNames) {
        const KFileItem item(QUrl::fromLocalFile(prefix + name), QString(), KFileItem::Unknown);
        result << item;
    }
    return result;
}

QTEST_MAIN(KFileItemModelBenchmark)

#include "kfileitemmodelbenchmark.moc"
