/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#include <qtest_kde.h>

#include <KDirLister>
#include "kitemviews/kfileitemmodel.h"
#include "testdir.h"

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

Q_DECLARE_METATYPE(KItemRangeList)
Q_DECLARE_METATYPE(QList<int>)

class KFileItemModelTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testDefaultRoles();
    void testDefaultSortRole();
    void testDefaultGroupedSorting();
    void testNewItems();
    void testRemoveItems();
    void testSetData();
    void testSetDataWithModifiedSortRole_data();
    void testSetDataWithModifiedSortRole();
    void testModelConsistencyWhenInsertingItems();
    void testItemRangeConsistencyWhenInsertingItems();
    void testExpandItems();
    void testExpandParentItems();
    void testSorting();

    void testExpansionLevelsCompare_data();
    void testExpansionLevelsCompare();

    void testIndexForKeyboardSearch();

    void testNameFilter();

private:
    bool isModelConsistent() const;
    QStringList itemsInModel() const;

private:
    KFileItemModel* m_model;
    KDirLister* m_dirLister;
    TestDir* m_testDir;
};

void KFileItemModelTest::init()
{
    // The item-model tests result in a huge number of debugging
    // output from kdelibs. Only show critical and fatal messages.
    qInstallMsgHandler(myMessageOutput);

    qRegisterMetaType<KItemRange>("KItemRange");
    qRegisterMetaType<KItemRangeList>("KItemRangeList");
    qRegisterMetaType<KFileItemList>("KFileItemList");

    m_testDir = new TestDir();
    m_dirLister = new KDirLister();
    m_model = new KFileItemModel(m_dirLister);
}

void KFileItemModelTest::cleanup()
{
    delete m_model;
    m_model = 0;

    delete m_dirLister;
    m_dirLister = 0;

    delete m_testDir;
    m_testDir = 0;
}

void KFileItemModelTest::testDefaultRoles()
{
    const QSet<QByteArray> roles = m_model->roles();
    QCOMPARE(roles.count(), 2);
    QVERIFY(roles.contains("name"));
    QVERIFY(roles.contains("isDir"));
}

void KFileItemModelTest::testDefaultSortRole()
{
    QCOMPARE(m_model->sortRole(), QByteArray("name"));

    QStringList files;
    files << "c.txt" << "a.txt" << "b.txt";

    m_testDir->createFiles(files);

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_model->data(0)["name"].toString(), QString("a.txt"));
    QCOMPARE(m_model->data(1)["name"].toString(), QString("b.txt"));
    QCOMPARE(m_model->data(2)["name"].toString(), QString("c.txt"));
}

void KFileItemModelTest::testDefaultGroupedSorting()
{
    QCOMPARE(m_model->groupedSorting(), false);
}

void KFileItemModelTest::testNewItems()
{
    QStringList files;
    files << "a.txt" << "b.txt" << "c.txt";
    m_testDir->createFiles(files);

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    QCOMPARE(m_model->count(), 3);

    QVERIFY(isModelConsistent());
}

void KFileItemModelTest::testRemoveItems()
{
    m_testDir->createFile("a.txt");
    m_testDir->createFile("b.txt");
    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 2);
    QVERIFY(isModelConsistent());

    m_testDir->removeFile("a.txt");
    m_dirLister->updateDirectory(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsRemoved(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 1);
    QVERIFY(isModelConsistent());
}

void KFileItemModelTest::testSetData()
{
    m_testDir->createFile("a.txt");

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    QHash<QByteArray, QVariant> values;
    values.insert("customRole1", "Test1");
    values.insert("customRole2", "Test2");

    QSignalSpy itemsChangedSpy(m_model, SIGNAL(itemsChanged(KItemRangeList,QSet<QByteArray>)));
    m_model->setData(0, values);
    QCOMPARE(itemsChangedSpy.count(), 1);

    values = m_model->data(0);
    QCOMPARE(values.value("customRole1").toString(), QString("Test1"));
    QCOMPARE(values.value("customRole2").toString(), QString("Test2"));
    QVERIFY(isModelConsistent());
}

void KFileItemModelTest::testSetDataWithModifiedSortRole_data()
{
    QTest::addColumn<int>("changedIndex");
    QTest::addColumn<int>("changedRating");
    QTest::addColumn<bool>("expectMoveSignal");
    QTest::addColumn<int>("ratingIndex0");
    QTest::addColumn<int>("ratingIndex1");
    QTest::addColumn<int>("ratingIndex2");

    // Default setup:
    // Index 0 = rating 2
    // Index 1 = rating 4
    // Index 2 = rating 6

    QTest::newRow("Index 0: Rating 3") << 0 << 3 << false << 3 << 4 << 6;
    QTest::newRow("Index 0: Rating 5") << 0 << 5 << true  << 4 << 5 << 6;
    QTest::newRow("Index 0: Rating 8") << 0 << 8 << true  << 4 << 6 << 8;

    QTest::newRow("Index 2: Rating 1") << 2 << 1 << true  << 1 << 2 << 4;
    QTest::newRow("Index 2: Rating 3") << 2 << 3 << true  << 2 << 3 << 4;
    QTest::newRow("Index 2: Rating 5") << 2 << 5 << false << 2 << 4 << 5;
}

void KFileItemModelTest::testSetDataWithModifiedSortRole()
{
    QFETCH(int, changedIndex);
    QFETCH(int, changedRating);
    QFETCH(bool, expectMoveSignal);
    QFETCH(int, ratingIndex0);
    QFETCH(int, ratingIndex1);
    QFETCH(int, ratingIndex2);

    // Changing the value of a sort-role must result in
    // a reordering of the items.
    QCOMPARE(m_model->sortRole(), QByteArray("name"));

    QStringList files;
    files << "a.txt" << "b.txt" << "c.txt";
    m_testDir->createFiles(files);

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    // Fill the "rating" role of each file:
    // a.txt -> 2
    // b.txt -> 4
    // c.txt -> 6

    QHash<QByteArray, QVariant> ratingA;
    ratingA.insert("rating", 2);
    m_model->setData(0, ratingA);

    QHash<QByteArray, QVariant> ratingB;
    ratingB.insert("rating", 4);
    m_model->setData(1, ratingB);

    QHash<QByteArray, QVariant> ratingC;
    ratingC.insert("rating", 6);
    m_model->setData(2, ratingC);

    m_model->setSortRole("rating");
    QCOMPARE(m_model->data(0).value("rating").toInt(), 2);
    QCOMPARE(m_model->data(1).value("rating").toInt(), 4);
    QCOMPARE(m_model->data(2).value("rating").toInt(), 6);

    // Now change the rating from a.txt. This usually results
    // in reordering of the items.
    QHash<QByteArray, QVariant> rating;
    rating.insert("rating", changedRating);
    m_model->setData(changedIndex, rating);

    if (expectMoveSignal) {
        QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsMoved(KItemRange,QList<int>)), DefaultTimeout));
    }
    
    QCOMPARE(m_model->data(0).value("rating").toInt(), ratingIndex0);
    QCOMPARE(m_model->data(1).value("rating").toInt(), ratingIndex1);
    QCOMPARE(m_model->data(2).value("rating").toInt(), ratingIndex2);
    QVERIFY(isModelConsistent());
}

void KFileItemModelTest::testModelConsistencyWhenInsertingItems()
{
    //QSKIP("Temporary disabled", SkipSingle);

    // KFileItemModel prevents that inserting a punch of items sequentially
    // results in an itemsInserted()-signal for each item. Instead internally
    // a timeout is given that collects such operations and results in only
    // one itemsInserted()-signal. However in this test we want to stress
    // KFileItemModel to do a lot of insert operation and hence decrease
    // the timeout to 1 millisecond.
    m_model->m_minimumUpdateIntervalTimer->setInterval(1);

    m_testDir->createFile("1");
    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 1);

    // Insert 10 items for 20 times. After each insert operation the model consistency
    // is checked.
    QSet<int> insertedItems;
    for (int i = 0; i < 20; ++i) {
        QSignalSpy spy(m_model, SIGNAL(itemsInserted(KItemRangeList)));

        for (int j = 0; j < 10; ++j) {
            int itemName = qrand();
            while (insertedItems.contains(itemName)) {
                itemName = qrand();
            }
            insertedItems.insert(itemName);

            m_testDir->createFile(QString::number(itemName));
        }

        m_dirLister->updateDirectory(m_testDir->url());
        if (spy.count() == 0) {
            QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
        }

        QVERIFY(isModelConsistent());
    }

    QCOMPARE(m_model->count(), 201);
}

void KFileItemModelTest::testItemRangeConsistencyWhenInsertingItems()
{
    QStringList files;
    files << "B" << "E" << "G";
    m_testDir->createFiles(files);

    // Due to inserting the 3 items one item-range with index == 0 and
    // count == 3 must be given
    QSignalSpy spy1(m_model, SIGNAL(itemsInserted(KItemRangeList)));
    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    QCOMPARE(spy1.count(), 1);
    QList<QVariant> arguments = spy1.takeFirst();
    KItemRangeList itemRangeList = arguments.at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(0, 3));

    // The indexes of the item-ranges must always be related to the model before
    // the items have been inserted. Having:
    //   0 1 2
    //   B E G
    // and inserting A, C, D, F the resulting model will be:
    //   0 1 2 3 4 5 6
    //   A B C D E F G
    // and the item-ranges must be:
    //   index: 0, count: 1 for A
    //   index: 1, count: 2 for B, C
    //   index: 2, count: 1 for G

    files.clear();
    files << "A" << "C" << "D" << "F";
    m_testDir->createFiles(files);

    QSignalSpy spy2(m_model, SIGNAL(itemsInserted(KItemRangeList)));
    m_dirLister->updateDirectory(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    QCOMPARE(spy2.count(), 1);
    arguments = spy2.takeFirst();
    itemRangeList = arguments.at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(0, 1) << KItemRange(1, 2) << KItemRange(2, 1));
}

void KFileItemModelTest::testExpandItems()
{
    // Test expanding subfolders in a folder with the items "a/", "a/a/", "a/a/1", "a/a-1/", "a/a-1/1".
    // Besides testing the basic item expansion functionality, the test makes sure that
    // KFileItemModel::expansionLevelsCompare(const KFileItem& a, const KFileItem& b)
    // yields the correct result for "a/a/1" and "a/a-1/", whis is non-trivial because they share the
    // first three characters.
    QSet<QByteArray> modelRoles = m_model->roles();
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    m_model->setRoles(modelRoles);

    QStringList files;
    files << "a/a/1" << "a/a-1/1"; // missing folders are created automatically
    m_testDir->createFiles(files);

    // Store the URLs of all folders in a set.
    QSet<KUrl> allFolders;
    allFolders << KUrl(m_testDir->name() + "a") << KUrl(m_testDir->name() + "a/a") << KUrl(m_testDir->name() + "a/a-1");

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    // So far, the model contains only "a/"
    QCOMPARE(m_model->count(), 1);
    QVERIFY(m_model->isExpandable(0));
    QVERIFY(!m_model->isExpanded(0));
    QVERIFY(m_model->expandedUrls().empty());

    QSignalSpy spyInserted(m_model, SIGNAL(itemsInserted(KItemRangeList)));

    // Expand the folder "a/" -> "a/a/" and "a/a-1/" become visible
    m_model->setExpanded(0, true);
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 3); // 3 items: "a/", "a/a/", "a/a-1/"
    QCOMPARE(m_model->expandedUrls(), QSet<KUrl>() << KUrl(m_testDir->name() + "a"));

    QCOMPARE(spyInserted.count(), 1);
    KItemRangeList itemRangeList = spyInserted.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(1, 2)); // 2 new items "a/a/" and "a/a-1/" with indices 1 and 2

    QVERIFY(m_model->isExpandable(1));
    QVERIFY(!m_model->isExpanded(1));
    QVERIFY(m_model->isExpandable(2));
    QVERIFY(!m_model->isExpanded(2));

    // Expand the folder "a/a/" -> "a/a/1" becomes visible
    m_model->setExpanded(1, true);
    QVERIFY(m_model->isExpanded(1));
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 4);  // 4 items: "a/", "a/a/", "a/a/1", "a/a-1/"
    QCOMPARE(m_model->expandedUrls(), QSet<KUrl>() << KUrl(m_testDir->name() + "a") << KUrl(m_testDir->name() + "a/a"));

    QCOMPARE(spyInserted.count(), 1);
    itemRangeList = spyInserted.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(2, 1)); // 1 new item "a/a/1" with index 2

    QVERIFY(!m_model->isExpandable(2));
    QVERIFY(!m_model->isExpanded(2));

    // Expand the folder "a/a-1/" -> "a/a-1/1" becomes visible
    m_model->setExpanded(3, true);
    QVERIFY(m_model->isExpanded(3));
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 5);  // 5 items: "a/", "a/a/", "a/a/1", "a/a-1/", "a/a-1/1"
    QCOMPARE(m_model->expandedUrls(), allFolders);

    QCOMPARE(spyInserted.count(), 1);
    itemRangeList = spyInserted.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(4, 1)); // 1 new item "a/a-1/1" with index 4

    QVERIFY(!m_model->isExpandable(4));
    QVERIFY(!m_model->isExpanded(4));

    QSignalSpy spyRemoved(m_model, SIGNAL(itemsRemoved(KItemRangeList)));

    // Collapse the top-level folder -> all other items should disappear
    m_model->setExpanded(0, false);
    QVERIFY(!m_model->isExpanded(0));
    QCOMPARE(m_model->count(), 1);
    QVERIFY(!m_model->expandedUrls().contains(KUrl(m_testDir->name() + "a"))); // TODO: Make sure that child URLs are also removed

    QCOMPARE(spyRemoved.count(), 1);
    itemRangeList = spyRemoved.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(1, 4)); // 4 items removed

    // Clear the model, reload the folder and try to restore the expanded folders.
    m_model->clear();
    QCOMPARE(m_model->count(), 0);
    QVERIFY(m_model->expandedUrls().empty());

    m_dirLister->openUrl(m_testDir->url());
    m_model->restoreExpandedUrls(allFolders);
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(loadingCompleted()), DefaultTimeout));
    QCOMPARE(m_model->count(), 5);  // 5 items: "a/", "a/a/", "a/a/1", "a/a-1/", "a/a-1/1"
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(m_model->isExpanded(1));
    QVERIFY(!m_model->isExpanded(2));
    QVERIFY(m_model->isExpanded(3));
    QVERIFY(!m_model->isExpanded(4));
    QCOMPARE(m_model->expandedUrls(), allFolders);

    // Move to a sub folder, then call restoreExpandedFolders() *before* going back.
    // This is how DolphinView restores the expanded folders when navigating in history.
    m_dirLister->openUrl(KUrl(m_testDir->name() + "a/a/"));
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(loadingCompleted()), DefaultTimeout));
    QCOMPARE(m_model->count(), 1);  // 1 item: "1"
    m_model->restoreExpandedUrls(allFolders);
    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(loadingCompleted()), DefaultTimeout));
    QCOMPARE(m_model->count(), 5);  // 5 items: "a/", "a/a/", "a/a/1", "a/a-1/", "a/a-1/1"
    QCOMPARE(m_model->expandedUrls(), allFolders);
}

void KFileItemModelTest::testExpandParentItems()
{
    // Create a tree structure of folders:
    // a 1/
    // a 1/b1/
    // a 1/b1/c1/
    // a2/
    // a2/b2/
    // a2/b2/c2/
    // a2/b2/c2/d2/
    QSet<QByteArray> modelRoles = m_model->roles();
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    m_model->setRoles(modelRoles);

    QStringList files;
    files << "a 1/b1/c1/file.txt" << "a2/b2/c2/d2/file.txt"; // missing folders are created automatically
    m_testDir->createFiles(files);

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    // So far, the model contains only "a 1/" and "a2/".
    QCOMPARE(m_model->count(), 2);
    QVERIFY(m_model->expandedUrls().empty());

    // Expand the parents of "a2/b2/c2".
    m_model->expandParentItems(KUrl(m_testDir->name() + "a2/b2/c2"));
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(loadingCompleted()), DefaultTimeout));

    // The model should now contain "a 1/", "a2/", "a2/b2/", and "a2/b2/c2/".
    // It's important that only the parents of "a1/b1/c1" are expanded.
    QCOMPARE(m_model->count(), 4);
    QVERIFY(!m_model->isExpanded(0));
    QVERIFY(m_model->isExpanded(1));
    QVERIFY(m_model->isExpanded(2));
    QVERIFY(!m_model->isExpanded(3));

    // Expand the parents of "a 1/b1".
    m_model->expandParentItems(KUrl(m_testDir->name() + "a 1/b1"));
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(loadingCompleted()), DefaultTimeout));

    // The model should now contain "a 1/", "a 1/b1/", "a2/", "a2/b2", and "a2/b2/c2/".
    // It's important that only the parents of "a 1/b1/" and "a2/b2/c2/" are expanded.
    QCOMPARE(m_model->count(), 5);
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(!m_model->isExpanded(1));
    QVERIFY(m_model->isExpanded(2));
    QVERIFY(m_model->isExpanded(3));
    QVERIFY(!m_model->isExpanded(4));
}

void KFileItemModelTest::testSorting()
{
    // Create some files with different sizes and modification times to check the different sorting options
    QDateTime now = QDateTime::currentDateTime();

    m_testDir->createFile("a", "A file", now.addDays(-3));
    m_testDir->createFile("b", "A larger file", now.addDays(0));
    m_testDir->createDir("c", now.addDays(-2));
    m_testDir->createFile("d", "The largest file in this directory", now.addDays(-1));
    m_testDir->createFile("e", "An even larger file", now.addDays(-4));
    m_testDir->createFile(".f");

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    // Default: Sort by Name, ascending
    QCOMPARE(m_model->sortRole(), QByteArray("name"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(m_model->sortFoldersFirst());
    //QVERIFY(!m_model->showHiddenFiles());
    QCOMPARE(itemsInModel(), QStringList() << "c" << "a" << "b" << "d" << "e");

    QSignalSpy spyItemsMoved(m_model, SIGNAL(itemsMoved(KItemRange,QList<int>)));

    // Sort by Name, descending
    m_model->setSortOrder(Qt::DescendingOrder);
    QCOMPARE(m_model->sortRole(), QByteArray("name"));
    QCOMPARE(m_model->sortOrder(), Qt::DescendingOrder);
    QCOMPARE(itemsInModel(), QStringList() << "c" << "e" << "d" << "b" << "a");
    QCOMPARE(spyItemsMoved.count(), 1);
    QCOMPARE(spyItemsMoved.takeFirst().at(1).value<QList<int> >(), QList<int>() << 0 << 4 << 3 << 2 << 1);

    // Sort by Date, descending
    m_model->setSortRole("date");
    QCOMPARE(m_model->sortRole(), QByteArray("date"));
    QCOMPARE(m_model->sortOrder(), Qt::DescendingOrder);
    QCOMPARE(itemsInModel(), QStringList() << "c" << "b" << "d" << "a" << "e");
    QCOMPARE(spyItemsMoved.count(), 1);
    QCOMPARE(spyItemsMoved.takeFirst().at(1).value<QList<int> >(), QList<int>() << 0 << 4 << 2 << 1 << 3);

    // Sort by Date, ascending
    m_model->setSortOrder(Qt::AscendingOrder);
    QCOMPARE(m_model->sortRole(), QByteArray("date"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QCOMPARE(itemsInModel(), QStringList() << "c" << "e" << "a" << "d" << "b");
    QCOMPARE(spyItemsMoved.count(), 1);
    QCOMPARE(spyItemsMoved.takeFirst().at(1).value<QList<int> >(), QList<int>() << 0 << 4 << 3 << 2 << 1);

    // Sort by Date, ascending, 'Sort Folders First' disabled
    m_model->setSortFoldersFirst(false);
    QCOMPARE(m_model->sortRole(), QByteArray("date"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(!m_model->sortFoldersFirst());
    QCOMPARE(itemsInModel(), QStringList() << "e" << "a" << "c" << "d" << "b");
    QCOMPARE(spyItemsMoved.count(), 1);
    QCOMPARE(spyItemsMoved.takeFirst().at(1).value<QList<int> >(), QList<int>() << 2 << 0 << 1 << 3 << 4);

    // Sort by Name, ascending, 'Sort Folders First' disabled
    m_model->setSortRole("name");
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(!m_model->sortFoldersFirst());
    QCOMPARE(itemsInModel(), QStringList() << "a" << "b" << "c" << "d" << "e");
    QCOMPARE(spyItemsMoved.count(), 1);
    QCOMPARE(spyItemsMoved.takeFirst().at(1).value<QList<int> >(), QList<int>() << 4 << 0 << 2 << 3 << 1);

    // Sort by Size, ascending, 'Sort Folders First' disabled
    m_model->setSortRole("size");
    QCOMPARE(m_model->sortRole(), QByteArray("size"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(!m_model->sortFoldersFirst());
    QCOMPARE(itemsInModel(), QStringList() << "c" << "a" << "b" << "e" << "d");
    QCOMPARE(spyItemsMoved.count(), 1);
    QCOMPARE(spyItemsMoved.takeFirst().at(1).value<QList<int> >(), QList<int>() << 1 << 2 << 0 << 4 << 3);

    QSKIP("2 tests of testSorting() are temporary deactivated as in KFileItemModel resortAllItems() "
          "always emits a itemsMoved() signal. Before adjusting the tests think about probably introducing "
          "another signal", SkipSingle);
    // Internal note: Check comment in KFileItemModel::resortAllItems() for details.

    // In 'Sort by Size' mode, folders are always first -> changing 'Sort Folders First' does not resort the model
    m_model->setSortFoldersFirst(true);
    QCOMPARE(m_model->sortRole(), QByteArray("size"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(m_model->sortFoldersFirst());
    QCOMPARE(itemsInModel(), QStringList() << "c" << "a" << "b" << "e" << "d");
    QCOMPARE(spyItemsMoved.count(), 0);

    // Sort by Size, descending, 'Sort Folders First' enabled
    m_model->setSortOrder(Qt::DescendingOrder);
    QCOMPARE(m_model->sortRole(), QByteArray("size"));
    QCOMPARE(m_model->sortOrder(), Qt::DescendingOrder);
    QVERIFY(m_model->sortFoldersFirst());
    QCOMPARE(itemsInModel(), QStringList() << "c" << "d" << "e" << "b" << "a");
    QCOMPARE(spyItemsMoved.count(), 1);
    QCOMPARE(spyItemsMoved.takeFirst().at(1).value<QList<int> >(), QList<int>() << 0 << 4 << 3 << 2 << 1);

    // TODO: Sort by other roles; show/hide hidden files
}

void KFileItemModelTest::testExpansionLevelsCompare_data()
{
    QTest::addColumn<QString>("urlA");
    QTest::addColumn<QString>("urlB");
    QTest::addColumn<int>("result");

    QTest::newRow("Equal") << "/a/b" << "/a/b" << 0;
    QTest::newRow("Sub path: A < B") << "/a/b" << "/a/b/c" << -1;
    QTest::newRow("Sub path: A > B") << "/a/b/c" << "/a/b" << +1;
    QTest::newRow("Same level: /a/1 < /a-1/1") << "/a/1" << "/a-1/1" << -1;
    QTest::newRow("Same level: /a-1/1 > /a/1") << "/a-1/1" << "/a/1" << +1;
    QTest::newRow("Different levels: /a/a/1 < /a/a-1") << "/a/a/1" << "/a/a-1" << -1;
    QTest::newRow("Different levels: /a/a-1 > /a/a/1") << "/a/a-1" << "/a/a/1" << +1;
}

void KFileItemModelTest::testExpansionLevelsCompare()
{
    QSKIP("Temporary deactivated as KFileItemModel::ItemData has been extended "
          "by a 'parent' member that is required for a correct comparison. For a "
          "successful test the item-data of all parents must be available.", SkipAll);

    QFETCH(QString, urlA);
    QFETCH(QString, urlB);
    QFETCH(int, result);

    const KFileItem itemA(KUrl(urlA), QString(), mode_t(-1));
    const KFileItem itemB(KUrl(urlB), QString(), mode_t(-1));

    KFileItemModel::ItemData a;
    a.item = itemA;
    a.parent = 0;

    KFileItemModel::ItemData b;
    b.item = itemB;
    b.parent = 0;

    QCOMPARE(m_model->expandedParentsCountCompare(&a, &b), result);
}

void KFileItemModelTest::testIndexForKeyboardSearch()
{
    QStringList files;
    files << "a" << "aa" << "Image.jpg" << "Image.png" << "Text" << "Text1" << "Text2" << "Text11";
    m_testDir->createFiles(files);

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    // Search from index 0
    QCOMPARE(m_model->indexForKeyboardSearch("a", 0), 0);
    QCOMPARE(m_model->indexForKeyboardSearch("aa", 0), 1);
    QCOMPARE(m_model->indexForKeyboardSearch("i", 0), 2);
    QCOMPARE(m_model->indexForKeyboardSearch("image", 0), 2);
    QCOMPARE(m_model->indexForKeyboardSearch("image.jpg", 0), 2);
    QCOMPARE(m_model->indexForKeyboardSearch("image.png", 0), 3);
    QCOMPARE(m_model->indexForKeyboardSearch("t", 0), 4);
    QCOMPARE(m_model->indexForKeyboardSearch("text", 0), 4);
    QCOMPARE(m_model->indexForKeyboardSearch("text1", 0), 5);
    QCOMPARE(m_model->indexForKeyboardSearch("text2", 0), 6);
    QCOMPARE(m_model->indexForKeyboardSearch("text11", 0), 7);

    // Start a search somewhere in the middle
    QCOMPARE(m_model->indexForKeyboardSearch("a", 1), 1);
    QCOMPARE(m_model->indexForKeyboardSearch("i", 3), 3);
    QCOMPARE(m_model->indexForKeyboardSearch("t", 5), 5);
    QCOMPARE(m_model->indexForKeyboardSearch("text1", 6), 7);

    // Test searches that go past the last item back to index 0
    QCOMPARE(m_model->indexForKeyboardSearch("a", 2), 0);
    QCOMPARE(m_model->indexForKeyboardSearch("i", 7), 2);
    QCOMPARE(m_model->indexForKeyboardSearch("image.jpg", 3), 2);
    QCOMPARE(m_model->indexForKeyboardSearch("text2", 7), 6);

    // Test searches that yield no result
    QCOMPARE(m_model->indexForKeyboardSearch("aaa", 0), -1);
    QCOMPARE(m_model->indexForKeyboardSearch("b", 0), -1);
    QCOMPARE(m_model->indexForKeyboardSearch("image.svg", 0), -1);
    QCOMPARE(m_model->indexForKeyboardSearch("text3", 0), -1);
    QCOMPARE(m_model->indexForKeyboardSearch("text3", 5), -1);

    // Test upper case searches (note that search is case insensitive)
    QCOMPARE(m_model->indexForKeyboardSearch("A", 0), 0);
    QCOMPARE(m_model->indexForKeyboardSearch("aA", 0), 1);
    QCOMPARE(m_model->indexForKeyboardSearch("TexT", 5), 5);
    QCOMPARE(m_model->indexForKeyboardSearch("IMAGE", 4), 2);

    // TODO: Maybe we should also test keyboard searches in directories which are not sorted by Name?
}

void KFileItemModelTest::testNameFilter()
{
    QStringList files;
    files << "A1" << "A2" << "Abc" << "Bcd" << "Cde";
    m_testDir->createFiles(files);

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    m_model->setNameFilter("A"); // Shows A1, A2 and Abc
    QCOMPARE(m_model->count(), 3);

    m_model->setNameFilter("A2"); // Shows only A2
    QCOMPARE(m_model->count(), 1);

    m_model->setNameFilter("A2"); // Shows only A1
    QCOMPARE(m_model->count(), 1);

    m_model->setNameFilter("Bc"); // Shows "Abc" and "Bcd"
    QCOMPARE(m_model->count(), 2);

    m_model->setNameFilter("bC"); // Shows "Abc" and "Bcd"
    QCOMPARE(m_model->count(), 2);

    m_model->setNameFilter(QString()); // Shows again all items
    QCOMPARE(m_model->count(), 5);
}

bool KFileItemModelTest::isModelConsistent() const
{
    if (m_model->m_items.count() != m_model->m_itemData.count()) {
        return false;
    }

    for (int i = 0; i < m_model->count(); ++i) {
        const KFileItem item = m_model->fileItem(i);
        if (item.isNull()) {
            qWarning() << "Item" << i << "is null";
            return false;
        }

        const int itemIndex = m_model->index(item);
        if (itemIndex != i) {
            qWarning() << "Item" << i << "has a wrong index:" << itemIndex;
            return false;
        }
    }

    return true;
}

QStringList KFileItemModelTest::itemsInModel() const
{
    QStringList items;

    for (int i = 0; i < m_model->count(); i++) {
        items << m_model->data(i).value("name").toString();
    }

    return items;
}

QTEST_KDEMAIN(KFileItemModelTest, NoGUI)

#include "kfileitemmodeltest.moc"
