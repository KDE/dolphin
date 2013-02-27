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

#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/private/kfileitemmodelsortalgorithm.h"

#include "testdir.h"

#include <KRandomSequence>

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

namespace {
    const int DefaultTimeout = 5000;
};

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
    void insertManyChildItems();

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
        model.slotItemsAdded(model.directory(), initialItems);
        model.slotCompleted();
        QCOMPARE(model.count(), initialItems.count());

        if (!newItems.isEmpty()) {
            model.slotItemsAdded(model.directory(), newItems);
            model.slotCompleted();
        }
        QCOMPARE(model.count(), initialItems.count() + newItems.count());

        if (!removedItems.isEmpty()) {
            model.removeItems(removedItems, KFileItemModel::DeleteItemData);
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

void KFileItemModelBenchmark::insertManyChildItems()
{
    // TODO: this function needs to be adjusted to the changes in KFileItemModel
    // (replacement of slotNewItems(KFileItemList) by slotItemsAdded(KUrl,KFileItemList))
    // Currently, this function tries to insert child items of multiple
    // directories by invoking the slot only once.
#if 0
    qInstallMsgHandler(myMessageOutput);

    KFileItemModel model;

    // Avoid overhead caused by natural sorting.
    model.m_naturalSorting = false;

    QSet<QByteArray> modelRoles = model.roles();
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    model.setRoles(modelRoles);
    model.setSortDirectoriesFirst(false);

    // Create a test folder with a 3-level tree structure of folders.
    TestDir testFolder;
    int numberOfFolders = 0;

    QStringList subFolderNames;
    subFolderNames << "a/" << "b/" << "c/" << "d/";

    foreach (const QString& s1, subFolderNames) {
        ++numberOfFolders;
        foreach (const QString& s2, subFolderNames) {
            ++numberOfFolders;
            foreach (const QString& s3, subFolderNames) {
                testFolder.createDir("level-1-" + s1 + "level-2-" + s2 + "level-3-" + s3);
                ++numberOfFolders;
            }
        }
    }

    // Open the folder in the model and expand all subfolders.
    model.loadDirectory(testFolder.url());
    QVERIFY(QTest::kWaitForSignal(&model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    int index = 0;
    while (index < model.count()) {
        if (model.isExpandable(index)) {
            model.setExpanded(index, true);

            if (!model.data(index).value("text").toString().startsWith("level-3")) {
                // New subfolders will appear unless we are on the final level already.
                QVERIFY(QTest::kWaitForSignal(&model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
            }

            QVERIFY(model.isExpanded(index));
        }
        ++index;
    }

    QCOMPARE(model.count(), numberOfFolders);

    // Create a list of many file items, which will be added to each of the
    // "level 1", "level 2", and "level 3" folders.
    const int filesPerDirectory = 500;
    QStringList allStrings;
    for (int i = 0; i < filesPerDirectory; ++i) {
        allStrings << QString::number(i);
    }
    allStrings.sort();

    KFileItemList newItems;

    // Also keep track of all expected items, including the existing
    // folders, to verify the final state of the model.
    KFileItemList allExpectedItems;

    for (int i = 0; i < model.count(); ++i) {
        const KFileItem folderItem = model.fileItem(i);
        allExpectedItems << folderItem;

        const KUrl folderUrl = folderItem.url();
        KFileItemList itemsInFolder = createFileItemList(allStrings, folderUrl.url(KUrl::AddTrailingSlash));

        newItems.append(itemsInFolder);
        allExpectedItems.append(itemsInFolder);
    }

    // Bring the items into random order.
    KRandomSequence randomSequence(0);
    randomSequence.randomize(newItems);

    // Measure how long it takes to insert and then remove all files.
    QBENCHMARK {
        model.slotNewItems(newItems);
        model.slotCompleted();

        QCOMPARE(model.count(), allExpectedItems.count());
        QVERIFY(model.isConsistent());
        for (int i = 0; i < model.count(); ++i) {
            QCOMPARE(model.fileItem(i), allExpectedItems.at(i));
        }

        model.slotItemsDeleted(newItems);
        QCOMPARE(model.count(), numberOfFolders);
        QVERIFY(model.isConsistent());
    }
#endif
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
