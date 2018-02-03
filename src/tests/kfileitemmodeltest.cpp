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

#include <QTest>
#include <QSignalSpy>
#include <QTimer>
#include <QMimeData>

#include <kio/job.h>

#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/private/kfileitemmodeldirlister.h"
#include "testdir.h"

void myMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);

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

Q_DECLARE_METATYPE(KItemRange)
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
    void testDirLoadingCompleted();
    void testSetData();
    void testSetDataWithModifiedSortRole_data();
    void testSetDataWithModifiedSortRole();
    void testChangeSortRole();
    void testResortAfterChangingName();
    void testModelConsistencyWhenInsertingItems();
    void testItemRangeConsistencyWhenInsertingItems();
    void testExpandItems();
    void testExpandParentItems();
    void testMakeExpandedItemHidden();
    void testRemoveFilteredExpandedItems();
    void testSorting();
    void testIndexForKeyboardSearch();
    void testNameFilter();
    void testEmptyPath();
    void testRefreshExpandedItem();
    void testRemoveHiddenItems();
    void collapseParentOfHiddenItems();
    void removeParentOfHiddenItems();
    void testGeneralParentChildRelationships();
    void testNameRoleGroups();
    void testNameRoleGroupsWithExpandedItems();
    void testInconsistentModel();
    void testChangeRolesForFilteredItems();
    void testChangeSortRoleWhileFiltering();
    void testRefreshFilteredItems();
    void testCollapseFolderWhileLoading();
    void testCreateMimeData();
    void testDeleteFileMoreThanOnce();

private:
    QStringList itemsInModel() const;

private:
    KFileItemModel* m_model;
    TestDir* m_testDir;
};

void KFileItemModelTest::init()
{
    // The item-model tests result in a huge number of debugging
    // output from kdelibs. Only show critical and fatal messages.
    qInstallMessageHandler(myMessageOutput);

    qRegisterMetaType<KItemRange>("KItemRange");
    qRegisterMetaType<KItemRangeList>("KItemRangeList");
    qRegisterMetaType<KFileItemList>("KFileItemList");

    m_testDir = new TestDir();
    m_model = new KFileItemModel();
    m_model->m_dirLister->setAutoUpdate(false);

    // Reduce the timer interval to make the test run faster.
    m_model->m_resortAllItemsTimer->setInterval(0);
}

void KFileItemModelTest::cleanup()
{
    delete m_model;
    m_model = nullptr;

    delete m_testDir;
    m_testDir = nullptr;
}

void KFileItemModelTest::testDefaultRoles()
{
    const QSet<QByteArray> roles = m_model->roles();
    QCOMPARE(roles.count(), 4);
    QVERIFY(roles.contains("text"));
    QVERIFY(roles.contains("isDir"));
    QVERIFY(roles.contains("isLink"));
    QVERIFY(roles.contains("isHidden"));
}

void KFileItemModelTest::testDefaultSortRole()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QVERIFY(itemsInsertedSpy.isValid());

    QCOMPARE(m_model->sortRole(), QByteArray("text"));

    m_testDir->createFiles({"c.txt", "a.txt", "b.txt"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());

    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_model->data(0).value("text").toString(), QString("a.txt"));
    QCOMPARE(m_model->data(1).value("text").toString(), QString("b.txt"));
    QCOMPARE(m_model->data(2).value("text").toString(), QString("c.txt"));
}

void KFileItemModelTest::testDefaultGroupedSorting()
{
    QCOMPARE(m_model->groupedSorting(), false);
}

void KFileItemModelTest::testNewItems()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);

    m_testDir->createFiles({"a.txt", "b.txt", "c.txt"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());

    QCOMPARE(m_model->count(), 3);

    QVERIFY(m_model->isConsistent());
}

void KFileItemModelTest::testRemoveItems()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QSignalSpy itemsRemovedSpy(m_model, &KFileItemModel::itemsRemoved);

    m_testDir->createFiles({"a.txt", "b.txt"});
    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 2);
    QVERIFY(m_model->isConsistent());

    m_testDir->removeFile("a.txt");
    m_model->m_dirLister->updateDirectory(m_testDir->url());
    QVERIFY(itemsRemovedSpy.wait());
    QCOMPARE(m_model->count(), 1);
    QVERIFY(m_model->isConsistent());
}

void KFileItemModelTest::testDirLoadingCompleted()
{
    QSignalSpy loadingCompletedSpy(m_model, &KFileItemModel::directoryLoadingCompleted);
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QSignalSpy itemsRemovedSpy(m_model, &KFileItemModel::itemsRemoved);

    m_testDir->createFiles({"a.txt", "b.txt", "c.txt"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(loadingCompletedSpy.wait());
    QCOMPARE(loadingCompletedSpy.count(), 1);
    QCOMPARE(itemsInsertedSpy.count(), 1);
    QCOMPARE(itemsRemovedSpy.count(), 0);
    QCOMPARE(m_model->count(), 3);

    m_testDir->createFiles({"d.txt", "e.txt"});
    m_model->m_dirLister->updateDirectory(m_testDir->url());
    QVERIFY(loadingCompletedSpy.wait());
    QCOMPARE(loadingCompletedSpy.count(), 2);
    QCOMPARE(itemsInsertedSpy.count(), 2);
    QCOMPARE(itemsRemovedSpy.count(), 0);
    QCOMPARE(m_model->count(), 5);

    m_testDir->removeFile("a.txt");
    m_testDir->createFile("f.txt");
    m_model->m_dirLister->updateDirectory(m_testDir->url());
    QVERIFY(loadingCompletedSpy.wait());
    QCOMPARE(loadingCompletedSpy.count(), 3);
    QCOMPARE(itemsInsertedSpy.count(), 3);
    QCOMPARE(itemsRemovedSpy.count(), 1);
    QCOMPARE(m_model->count(), 5);

    m_testDir->removeFile("b.txt");
    m_model->m_dirLister->updateDirectory(m_testDir->url());
    QVERIFY(itemsRemovedSpy.wait());
    QCOMPARE(loadingCompletedSpy.count(), 4);
    QCOMPARE(itemsInsertedSpy.count(), 3);
    QCOMPARE(itemsRemovedSpy.count(), 2);
    QCOMPARE(m_model->count(), 4);

    QVERIFY(m_model->isConsistent());
}

void KFileItemModelTest::testSetData()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QVERIFY(itemsInsertedSpy.isValid());
    QSignalSpy itemsChangedSpy(m_model, &KFileItemModel::itemsChanged);
    QVERIFY(itemsChangedSpy.isValid());

    m_testDir->createFile("a.txt");

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());

    QHash<QByteArray, QVariant> values;
    values.insert("customRole1", "Test1");
    values.insert("customRole2", "Test2");

    m_model->setData(0, values);
    QCOMPARE(itemsChangedSpy.count(), 1);

    values = m_model->data(0);
    QCOMPARE(values.value("customRole1").toString(), QString("Test1"));
    QCOMPARE(values.value("customRole2").toString(), QString("Test2"));
    QVERIFY(m_model->isConsistent());
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

    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QVERIFY(itemsInsertedSpy.isValid());
    QSignalSpy itemsMovedSpy(m_model, &KFileItemModel::itemsMoved);
    QVERIFY(itemsMovedSpy.isValid());

    // Changing the value of a sort-role must result in
    // a reordering of the items.
    QCOMPARE(m_model->sortRole(), QByteArray("text"));
    m_model->setSortRole("rating");
    QCOMPARE(m_model->sortRole(), QByteArray("rating"));

    m_testDir->createFiles({"a.txt", "b.txt", "c.txt"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());

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

    QCOMPARE(m_model->data(0).value("rating").toInt(), 2);
    QCOMPARE(m_model->data(1).value("rating").toInt(), 4);
    QCOMPARE(m_model->data(2).value("rating").toInt(), 6);

    // Now change the rating from a.txt. This usually results
    // in reordering of the items.
    QHash<QByteArray, QVariant> rating;
    rating.insert("rating", changedRating);
    m_model->setData(changedIndex, rating);

    if (expectMoveSignal) {
        QVERIFY(itemsMovedSpy.wait());
    }

    QCOMPARE(m_model->data(0).value("rating").toInt(), ratingIndex0);
    QCOMPARE(m_model->data(1).value("rating").toInt(), ratingIndex1);
    QCOMPARE(m_model->data(2).value("rating").toInt(), ratingIndex2);
    QVERIFY(m_model->isConsistent());
}

void KFileItemModelTest::testChangeSortRole()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QSignalSpy itemsMovedSpy(m_model, &KFileItemModel::itemsMoved);
    QVERIFY(itemsMovedSpy.isValid());

    QCOMPARE(m_model->sortRole(), QByteArray("text"));

    m_testDir->createFiles({"a.txt", "b.jpg", "c.txt"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "b.jpg" << "c.txt");

    // Simulate that KFileItemModelRolesUpdater determines the mime type.
    // Resorting the files by 'type' will only work immediately if their
    // mime types are known.
    for (int index = 0; index < m_model->count(); ++index) {
        m_model->fileItem(index).determineMimeType();
    }

    // Now: sort by type.
    m_model->setSortRole("type");
    QCOMPARE(m_model->sortRole(), QByteArray("type"));
    QVERIFY(!itemsMovedSpy.isEmpty());

    // The actual order of the files might depend on the translation of the
    // result of KFileItem::mimeComment() in the user's language.
    QStringList version1;
    version1 << "b.jpg" << "a.txt" << "c.txt";

    QStringList version2;
    version2 << "a.txt" << "c.txt" << "b.jpg";

    const bool ok1 = (itemsInModel() == version1);
    const bool ok2 = (itemsInModel() == version2);

    QVERIFY(ok1 || ok2);
}

void KFileItemModelTest::testResortAfterChangingName()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QSignalSpy itemsMovedSpy(m_model, &KFileItemModel::itemsMoved);
    QVERIFY(itemsMovedSpy.isValid());

    // We sort by size in a directory where all files have the same size.
    // Therefore, the files are sorted by their names.
    m_model->setSortRole("size");

    m_testDir->createFiles({"a.txt", "b.txt", "c.txt"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "b.txt" << "c.txt");

    // We rename a.txt to d.txt. Even though the size has not changed at all,
    // the model must re-sort the items.
    QHash<QByteArray, QVariant> data;
    data.insert("text", "d.txt");
    m_model->setData(0, data);

    QVERIFY(itemsMovedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "b.txt" << "c.txt" << "d.txt");

    // We rename d.txt back to a.txt using the dir lister's refreshItems() signal.
    const KFileItem fileItemD = m_model->fileItem(2);
    KFileItem fileItemA = fileItemD;
    QUrl urlA = fileItemA.url().adjusted(QUrl::RemoveFilename);
    urlA.setPath(urlA.path() + "a.txt");
    fileItemA.setUrl(urlA);

    m_model->slotRefreshItems({qMakePair(fileItemD, fileItemA)});

    QVERIFY(itemsMovedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "b.txt" << "c.txt");
}

void KFileItemModelTest::testModelConsistencyWhenInsertingItems()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);

    // KFileItemModel prevents that inserting a punch of items sequentially
    // results in an itemsInserted()-signal for each item. Instead internally
    // a timeout is given that collects such operations and results in only
    // one itemsInserted()-signal. However in this test we want to stress
    // KFileItemModel to do a lot of insert operation and hence decrease
    // the timeout to 1 millisecond.
    m_testDir->createFile("1");
    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 1);

    // Insert 10 items for 20 times. After each insert operation the model consistency
    // is checked.
    QSet<int> insertedItems;
    for (int i = 0; i < 20; ++i) {
        itemsInsertedSpy.clear();

        for (int j = 0; j < 10; ++j) {
            int itemName = qrand();
            while (insertedItems.contains(itemName)) {
                itemName = qrand();
            }
            insertedItems.insert(itemName);

            m_testDir->createFile(QString::number(itemName));
        }

        m_model->m_dirLister->updateDirectory(m_testDir->url());
        if (itemsInsertedSpy.count() == 0) {
            QVERIFY(itemsInsertedSpy.wait());
        }

        QVERIFY(m_model->isConsistent());
    }

    QCOMPARE(m_model->count(), 201);
}

void KFileItemModelTest::testItemRangeConsistencyWhenInsertingItems()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);

    m_testDir->createFiles({"B", "E", "G"});

    // Due to inserting the 3 items one item-range with index == 0 and
    // count == 3 must be given
    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());

    QCOMPARE(itemsInsertedSpy.count(), 1);
    QList<QVariant> arguments = itemsInsertedSpy.takeFirst();
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

    m_testDir->createFiles({"A", "C", "D", "F"});

    m_model->m_dirLister->updateDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());

    QCOMPARE(itemsInsertedSpy.count(), 1);
    arguments = itemsInsertedSpy.takeFirst();
    itemRangeList = arguments.at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(0, 1) << KItemRange(1, 2) << KItemRange(2, 1));
}

void KFileItemModelTest::testExpandItems()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QVERIFY(itemsInsertedSpy.isValid());
    QSignalSpy itemsRemovedSpy(m_model, &KFileItemModel::itemsRemoved);
    QVERIFY(itemsRemovedSpy.isValid());
    QSignalSpy loadingCompletedSpy(m_model, &KFileItemModel::directoryLoadingCompleted);
    QVERIFY(loadingCompletedSpy.isValid());

    // Test expanding subfolders in a folder with the items "a/", "a/a/", "a/a/1", "a/a-1/", "a/a-1/1".
    // Besides testing the basic item expansion functionality, the test makes sure that
    // KFileItemModel::expansionLevelsCompare(const KFileItem& a, const KFileItem& b)
    // yields the correct result for "a/a/1" and "a/a-1/", whis is non-trivial because they share the
    // first three characters.
    QSet<QByteArray> originalModelRoles = m_model->roles();
    QSet<QByteArray> modelRoles = originalModelRoles;
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    m_model->setRoles(modelRoles);

    m_testDir->createFiles({"a/a/1", "a/a-1/1"});

    // Store the URLs of all folders in a set.
    QSet<QUrl> allFolders;
    allFolders << QUrl::fromLocalFile(m_testDir->path() + "/a")
               << QUrl::fromLocalFile(m_testDir->path() + "/a/a")
               << QUrl::fromLocalFile(m_testDir->path() + "/a/a-1");

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());

    // So far, the model contains only "a/"
    QCOMPARE(m_model->count(), 1);
    QVERIFY(m_model->isExpandable(0));
    QVERIFY(!m_model->isExpanded(0));
    QVERIFY(m_model->expandedDirectories().empty());

    QCOMPARE(itemsInsertedSpy.count(), 1);
    KItemRangeList itemRangeList = itemsInsertedSpy.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(0, 1)); // 1 new item "a/" with index 0

    // Expand the folder "a/" -> "a/a/" and "a/a-1/" become visible
    m_model->setExpanded(0, true);
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 3); // 3 items: "a/", "a/a/", "a/a-1/"
    QCOMPARE(m_model->expandedDirectories(), QSet<QUrl>() << QUrl::fromLocalFile(m_testDir->path() + "/a"));

    QCOMPARE(itemsInsertedSpy.count(), 1);
    itemRangeList = itemsInsertedSpy.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(1, 2)); // 2 new items "a/a/" and "a/a-1/" with indices 1 and 2

    QVERIFY(m_model->isExpandable(1));
    QVERIFY(!m_model->isExpanded(1));
    QVERIFY(m_model->isExpandable(2));
    QVERIFY(!m_model->isExpanded(2));

    // Expand the folder "a/a/" -> "a/a/1" becomes visible
    m_model->setExpanded(1, true);
    QVERIFY(m_model->isExpanded(1));
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 4);  // 4 items: "a/", "a/a/", "a/a/1", "a/a-1/"
    QCOMPARE(m_model->expandedDirectories(), QSet<QUrl>() << QUrl::fromLocalFile(m_testDir->path() + "/a")
                                                          << QUrl::fromLocalFile(m_testDir->path() + "/a/a"));

    QCOMPARE(itemsInsertedSpy.count(), 1);
    itemRangeList = itemsInsertedSpy.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(2, 1)); // 1 new item "a/a/1" with index 2

    QVERIFY(!m_model->isExpandable(2));
    QVERIFY(!m_model->isExpanded(2));

    // Expand the folder "a/a-1/" -> "a/a-1/1" becomes visible
    m_model->setExpanded(3, true);
    QVERIFY(m_model->isExpanded(3));
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 5);  // 5 items: "a/", "a/a/", "a/a/1", "a/a-1/", "a/a-1/1"
    QCOMPARE(m_model->expandedDirectories(), allFolders);

    QCOMPARE(itemsInsertedSpy.count(), 1);
    itemRangeList = itemsInsertedSpy.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(4, 1)); // 1 new item "a/a-1/1" with index 4

    QVERIFY(!m_model->isExpandable(4));
    QVERIFY(!m_model->isExpanded(4));

    // Collapse the top-level folder -> all other items should disappear
    m_model->setExpanded(0, false);
    QVERIFY(!m_model->isExpanded(0));
    QCOMPARE(m_model->count(), 1);
    QVERIFY(!m_model->expandedDirectories().contains(QUrl::fromLocalFile(m_testDir->path() + "/a"))); // TODO: Make sure that child URLs are also removed

    QCOMPARE(itemsRemovedSpy.count(), 1);
    itemRangeList = itemsRemovedSpy.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(1, 4)); // 4 items removed
    QVERIFY(m_model->isConsistent());

    // Clear the model, reload the folder and try to restore the expanded folders.
    m_model->clear();
    QCOMPARE(m_model->count(), 0);
    QVERIFY(m_model->expandedDirectories().empty());

    m_model->loadDirectory(m_testDir->url());
    m_model->restoreExpandedDirectories(allFolders);
    QVERIFY(loadingCompletedSpy.wait());
    QCOMPARE(m_model->count(), 5);  // 5 items: "a/", "a/a/", "a/a/1", "a/a-1/", "a/a-1/1"
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(m_model->isExpanded(1));
    QVERIFY(!m_model->isExpanded(2));
    QVERIFY(m_model->isExpanded(3));
    QVERIFY(!m_model->isExpanded(4));
    QCOMPARE(m_model->expandedDirectories(), allFolders);
    QVERIFY(m_model->isConsistent());

    // Move to a sub folder, then call restoreExpandedFolders() *before* going back.
    // This is how DolphinView restores the expanded folders when navigating in history.
    m_model->loadDirectory(QUrl::fromLocalFile(m_testDir->path() + "/a/a/"));
    QVERIFY(loadingCompletedSpy.wait());
    QCOMPARE(m_model->count(), 1);  // 1 item: "1"
    m_model->restoreExpandedDirectories(allFolders);
    m_model->loadDirectory(m_testDir->url());
    QVERIFY(loadingCompletedSpy.wait());
    QCOMPARE(m_model->count(), 5);  // 5 items: "a/", "a/a/", "a/a/1", "a/a-1/", "a/a-1/1"
    QCOMPARE(m_model->expandedDirectories(), allFolders);

    // Remove all expanded items by changing the roles
    itemsRemovedSpy.clear();
    m_model->setRoles(originalModelRoles);
    QVERIFY(!m_model->isExpanded(0));
    QCOMPARE(m_model->count(), 1);
    QVERIFY(!m_model->expandedDirectories().contains(QUrl::fromLocalFile(m_testDir->path() + "/a")));

    QCOMPARE(itemsRemovedSpy.count(), 1);
    itemRangeList = itemsRemovedSpy.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(1, 4)); // 4 items removed
    QVERIFY(m_model->isConsistent());
}

void KFileItemModelTest::testExpandParentItems()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QSignalSpy loadingCompletedSpy(m_model, &KFileItemModel::directoryLoadingCompleted);
    QVERIFY(loadingCompletedSpy.isValid());

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

    m_testDir->createFiles({"a 1/b1/c1/file.txt", "a2/b2/c2/d2/file.txt"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());

    // So far, the model contains only "a 1/" and "a2/".
    QCOMPARE(m_model->count(), 2);
    QVERIFY(m_model->expandedDirectories().empty());

    // Expand the parents of "a2/b2/c2".
    m_model->expandParentDirectories(QUrl::fromLocalFile(m_testDir->path() + "a2/b2/c2"));
    QVERIFY(loadingCompletedSpy.wait());

    // The model should now contain "a 1/", "a2/", "a2/b2/", and "a2/b2/c2/".
    // It's important that only the parents of "a1/b1/c1" are expanded.
    QCOMPARE(m_model->count(), 4);
    QVERIFY(!m_model->isExpanded(0));
    QVERIFY(m_model->isExpanded(1));
    QVERIFY(m_model->isExpanded(2));
    QVERIFY(!m_model->isExpanded(3));

    // Expand the parents of "a 1/b1".
    m_model->expandParentDirectories(QUrl::fromLocalFile(m_testDir->path() + "a 1/b1"));
    QVERIFY(loadingCompletedSpy.wait());

    // The model should now contain "a 1/", "a 1/b1/", "a2/", "a2/b2", and "a2/b2/c2/".
    // It's important that only the parents of "a 1/b1/" and "a2/b2/c2/" are expanded.
    QCOMPARE(m_model->count(), 5);
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(!m_model->isExpanded(1));
    QVERIFY(m_model->isExpanded(2));
    QVERIFY(m_model->isExpanded(3));
    QVERIFY(!m_model->isExpanded(4));
    QVERIFY(m_model->isConsistent());

    // Expand "a 1/b1/".
    m_model->setExpanded(1, true);
    QVERIFY(loadingCompletedSpy.wait());
    QCOMPARE(m_model->count(), 6);
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(m_model->isExpanded(1));
    QVERIFY(!m_model->isExpanded(2));
    QVERIFY(m_model->isExpanded(3));
    QVERIFY(m_model->isExpanded(4));
    QVERIFY(!m_model->isExpanded(5));
    QVERIFY(m_model->isConsistent());

    // Collapse "a 1/b1/" again, and verify that the previous state is restored.
    m_model->setExpanded(1, false);
    QCOMPARE(m_model->count(), 5);
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(!m_model->isExpanded(1));
    QVERIFY(m_model->isExpanded(2));
    QVERIFY(m_model->isExpanded(3));
    QVERIFY(!m_model->isExpanded(4));
    QVERIFY(m_model->isConsistent());
}

/**
 * Renaming an expanded folder by prepending its name with a dot makes it
 * hidden. Verify that this does not cause an inconsistent model state and
 * a crash later on, see https://bugs.kde.org/show_bug.cgi?id=311947
 */
void KFileItemModelTest::testMakeExpandedItemHidden()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QSignalSpy itemsRemovedSpy(m_model, &KFileItemModel::itemsRemoved);

    QSet<QByteArray> modelRoles = m_model->roles();
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    m_model->setRoles(modelRoles);

    m_testDir->createFiles({"1a/2a/3a", "1a/2a/3b", "1a/2b", "1b"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());

    // So far, the model contains only "1a/" and "1b".
    QCOMPARE(m_model->count(), 2);
    m_model->setExpanded(0, true);
    QVERIFY(itemsInsertedSpy.wait());

    // Now "1a/2a" and "1a/2b" have appeared.
    QCOMPARE(m_model->count(), 4);
    m_model->setExpanded(1, true);
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 6);

    // Rename "1a/2" and make it hidden.
    const QUrl oldUrl = QUrl::fromLocalFile(m_model->fileItem(0).url().path() + "/2a");
    const QUrl newUrl = QUrl::fromLocalFile(m_model->fileItem(0).url().path() + "/.2a");

    KIO::SimpleJob* job = KIO::rename(oldUrl, newUrl, KIO::HideProgressInfo);
    bool ok = job->exec();
    QVERIFY(ok);
    QVERIFY(itemsRemovedSpy.wait());

    // "1a/2" and its subfolders have disappeared now.
    QVERIFY(m_model->isConsistent());
    QCOMPARE(m_model->count(), 3);

    m_model->setExpanded(0, false);
    QCOMPARE(m_model->count(), 2);

}

void KFileItemModelTest::testRemoveFilteredExpandedItems()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);

    QSet<QByteArray> originalModelRoles = m_model->roles();
    QSet<QByteArray> modelRoles = originalModelRoles;
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    m_model->setRoles(modelRoles);

    m_testDir->createFiles({"folder/child", "file"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());

    // So far, the model contains only "folder/" and "file".
    QCOMPARE(m_model->count(), 2);
    QVERIFY(m_model->isExpandable(0));
    QVERIFY(!m_model->isExpandable(1));
    QVERIFY(!m_model->isExpanded(0));
    QVERIFY(!m_model->isExpanded(1));
    QCOMPARE(itemsInModel(), QStringList() << "folder" << "file");

    // Expand "folder" -> "folder/child" becomes visible.
    m_model->setExpanded(0, true);
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "folder" << "child" << "file");

    // Add a name filter.
    m_model->setNameFilter("f");
    QCOMPARE(itemsInModel(), QStringList() << "folder" << "file");

    m_model->setNameFilter("fo");
    QCOMPARE(itemsInModel(), QStringList() << "folder");

    // Remove all expanded items by changing the roles
    m_model->setRoles(originalModelRoles);
    QVERIFY(!m_model->isExpanded(0));
    QCOMPARE(itemsInModel(), QStringList() << "folder");

    // Remove the name filter and verify that "folder/child" does not reappear.
    m_model->setNameFilter(QString());
    QCOMPARE(itemsInModel(), QStringList() << "folder" << "file");
}

void KFileItemModelTest::testSorting()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QSignalSpy itemsMovedSpy(m_model, &KFileItemModel::itemsMoved);
    QVERIFY(itemsMovedSpy.isValid());

    // Create some files with different sizes and modification times to check the different sorting options
    QDateTime now = QDateTime::currentDateTime();

    QSet<QByteArray> roles;
    roles.insert("text");
    roles.insert("isExpanded");
    roles.insert("isExpandable");
    roles.insert("expandedParentsCount");
    m_model->setRoles(roles);

    m_testDir->createDir("c/c-2");
    m_testDir->createFile("c/c-2/c-3");
    m_testDir->createFile("c/c-1");

    m_testDir->createFile("a", "A file", now.addDays(-3));
    m_testDir->createFile("b", "A larger file", now.addDays(0));
    m_testDir->createDir("c", now.addDays(-2));
    m_testDir->createFile("d", "The largest file in this directory", now.addDays(-1));
    m_testDir->createFile("e", "An even larger file", now.addDays(-4));
    m_testDir->createFile(".f");

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());

    int index = m_model->index(QUrl(m_testDir->url().url() + "/c"));
    m_model->setExpanded(index, true);
    QVERIFY(itemsInsertedSpy.wait());

    index = m_model->index(QUrl(m_testDir->url().url() + "/c/c-2"));
    m_model->setExpanded(index, true);
    QVERIFY(itemsInsertedSpy.wait());

    // Default: Sort by Name, ascending
    QCOMPARE(m_model->sortRole(), QByteArray("text"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(m_model->sortDirectoriesFirst());
    QVERIFY(!m_model->showHiddenFiles());
    QCOMPARE(itemsInModel(), QStringList() << "c" << "c-2" << "c-3" << "c-1" << "a" << "b" << "d" << "e");

    // Sort by Name, ascending, 'Sort Folders First' disabled
    m_model->setSortDirectoriesFirst(false);
    QCOMPARE(m_model->sortRole(), QByteArray("text"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QCOMPARE(itemsInModel(), QStringList() << "a" << "b" << "c" << "c-1" << "c-2" << "c-3" << "d" << "e");
    QCOMPARE(itemsMovedSpy.count(), 1);
    QCOMPARE(itemsMovedSpy.first().at(0).value<KItemRange>(), KItemRange(0, 6));
    QCOMPARE(itemsMovedSpy.takeFirst().at(1).value<QList<int> >(), QList<int>() << 2 << 4 << 5 << 3 << 0 << 1);

    // Sort by Name, descending
    m_model->setSortDirectoriesFirst(true);
    m_model->setSortOrder(Qt::DescendingOrder);
    QCOMPARE(m_model->sortRole(), QByteArray("text"));
    QCOMPARE(m_model->sortOrder(), Qt::DescendingOrder);
    QCOMPARE(itemsInModel(), QStringList() << "c" << "c-2" << "c-3" << "c-1" << "e" << "d" << "b" << "a");
    QCOMPARE(itemsMovedSpy.count(), 2);
    QCOMPARE(itemsMovedSpy.first().at(0).value<KItemRange>(), KItemRange(0, 6));
    QCOMPARE(itemsMovedSpy.takeFirst().at(1).value<QList<int> >(), QList<int>() << 4 << 5 << 0 << 3 << 1 << 2);
    QCOMPARE(itemsMovedSpy.first().at(0).value<KItemRange>(), KItemRange(4, 4));
    QCOMPARE(itemsMovedSpy.takeFirst().at(1).value<QList<int> >(), QList<int>() << 7 << 6 << 5 << 4);

    // Sort by Date, descending
    m_model->setSortDirectoriesFirst(true);
    m_model->setSortRole("modificationtime");
    QCOMPARE(m_model->sortRole(), QByteArray("modificationtime"));
    QCOMPARE(m_model->sortOrder(), Qt::DescendingOrder);
    QCOMPARE(itemsInModel(), QStringList() << "c" << "c-2" << "c-3" << "c-1" << "b" << "d" << "a" << "e");
    QCOMPARE(itemsMovedSpy.count(), 1);
    QCOMPARE(itemsMovedSpy.first().at(0).value<KItemRange>(), KItemRange(4, 4));
    QCOMPARE(itemsMovedSpy.takeFirst().at(1).value<QList<int> >(), QList<int>() << 7 << 5 << 4 << 6);

    // Sort by Date, ascending
    m_model->setSortOrder(Qt::AscendingOrder);
    QCOMPARE(m_model->sortRole(), QByteArray("modificationtime"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QCOMPARE(itemsInModel(), QStringList() << "c" << "c-2" << "c-3" << "c-1" << "e" << "a" << "d" << "b");
    QCOMPARE(itemsMovedSpy.count(), 1);
    QCOMPARE(itemsMovedSpy.first().at(0).value<KItemRange>(), KItemRange(4, 4));
    QCOMPARE(itemsMovedSpy.takeFirst().at(1).value<QList<int> >(), QList<int>() << 7 << 6 << 5 << 4);

    // Sort by Date, ascending, 'Sort Folders First' disabled
    m_model->setSortDirectoriesFirst(false);
    QCOMPARE(m_model->sortRole(), QByteArray("modificationtime"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(!m_model->sortDirectoriesFirst());
    QCOMPARE(itemsInModel(), QStringList() << "e" << "a" << "c" << "c-1" << "c-2" << "c-3" << "d" << "b");
    QCOMPARE(itemsMovedSpy.count(), 1);
    QCOMPARE(itemsMovedSpy.first().at(0).value<KItemRange>(), KItemRange(0, 6));
    QCOMPARE(itemsMovedSpy.takeFirst().at(1).value<QList<int> >(), QList<int>() << 2 << 4 << 5 << 3 << 0 << 1);

    // Sort by Name, ascending, 'Sort Folders First' disabled
    m_model->setSortRole("text");
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(!m_model->sortDirectoriesFirst());
    QCOMPARE(itemsInModel(), QStringList() << "a" << "b" << "c" << "c-1" << "c-2" << "c-3" << "d" << "e");
    QCOMPARE(itemsMovedSpy.count(), 1);
    QCOMPARE(itemsMovedSpy.first().at(0).value<KItemRange>(), KItemRange(0, 8));
    QCOMPARE(itemsMovedSpy.takeFirst().at(1).value<QList<int> >(), QList<int>() << 7 << 0 << 2 << 3 << 4 << 5 << 6 << 1);

    // Sort by Size, ascending, 'Sort Folders First' disabled
    m_model->setSortRole("size");
    QCOMPARE(m_model->sortRole(), QByteArray("size"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(!m_model->sortDirectoriesFirst());
    QCOMPARE(itemsInModel(), QStringList() << "c" << "c-2" << "c-3" << "c-1" << "a" << "b" << "e" << "d");
    QCOMPARE(itemsMovedSpy.count(), 1);
    QCOMPARE(itemsMovedSpy.first().at(0).value<KItemRange>(), KItemRange(0, 8));
    QCOMPARE(itemsMovedSpy.takeFirst().at(1).value<QList<int> >(), QList<int>() << 4 << 5 << 0 << 3 << 1 << 2 << 7 << 6);

    // In 'Sort by Size' mode, folders are always first -> changing 'Sort Folders First' does not resort the model
    m_model->setSortDirectoriesFirst(true);
    QCOMPARE(m_model->sortRole(), QByteArray("size"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(m_model->sortDirectoriesFirst());
    QCOMPARE(itemsInModel(), QStringList() << "c" << "c-2" << "c-3" << "c-1" << "a" << "b" << "e" << "d");
    QCOMPARE(itemsMovedSpy.count(), 0);

    // Sort by Size, descending, 'Sort Folders First' enabled
    m_model->setSortOrder(Qt::DescendingOrder);
    QCOMPARE(m_model->sortRole(), QByteArray("size"));
    QCOMPARE(m_model->sortOrder(), Qt::DescendingOrder);
    QVERIFY(m_model->sortDirectoriesFirst());
    QCOMPARE(itemsInModel(), QStringList() << "c" << "c-2" << "c-3" << "c-1" << "d" << "e" << "b" << "a");
    QCOMPARE(itemsMovedSpy.count(), 1);
    QCOMPARE(itemsMovedSpy.first().at(0).value<KItemRange>(), KItemRange(4, 4));
    QCOMPARE(itemsMovedSpy.takeFirst().at(1).value<QList<int> >(), QList<int>() << 7 << 6 << 5 << 4);

    // TODO: Sort by other roles; show/hide hidden files
}

void KFileItemModelTest::testIndexForKeyboardSearch()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);

    m_testDir->createFiles({"a", "aa", "Image.jpg", "Image.png", "Text", "Text1", "Text2", "Text11"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());

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
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);

    m_testDir->createFiles({"A1", "A2", "Abc", "Bcd", "Cde"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());

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

/**
 * Verifies that we do not crash when adding a KFileItem with an empty path.
 * Before this issue was fixed, KFileItemModel::expandedParentsCountCompare()
 * tried to always read the first character of the path, even if the path is empty.
 */
void KFileItemModelTest::testEmptyPath()
{
    QSet<QByteArray> roles;
    roles.insert("text");
    roles.insert("isExpanded");
    roles.insert("isExpandable");
    roles.insert("expandedParentsCount");
    m_model->setRoles(roles);

    const QUrl emptyUrl;
    QVERIFY(emptyUrl.path().isEmpty());

    const QUrl url("file:///test/");

    KFileItemList items;
    items << KFileItem(emptyUrl, QString(), KFileItem::Unknown) << KFileItem(url, QString(), KFileItem::Unknown);
    m_model->slotItemsAdded(emptyUrl, items);
    m_model->slotCompleted();
}

/**
 * Verifies that the 'isExpanded' state of folders does not change when the
 * 'refreshItems' signal is received, see https://bugs.kde.org/show_bug.cgi?id=299675.
 */
void KFileItemModelTest::testRefreshExpandedItem()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QSignalSpy itemsChangedSpy(m_model, &KFileItemModel::itemsChanged);
    QVERIFY(itemsChangedSpy.isValid());

    QSet<QByteArray> modelRoles = m_model->roles();
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    m_model->setRoles(modelRoles);

    m_testDir->createFiles({"a/1", "a/2", "3", "4"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 3); // "a/", "3", "4"

    m_model->setExpanded(0, true);
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 5); // "a/", "a/1", "a/2", "3", "4"
    QVERIFY(m_model->isExpanded(0));

    const KFileItem item = m_model->fileItem(0);
    m_model->slotRefreshItems({qMakePair(item, item)});
    QVERIFY(!itemsChangedSpy.isEmpty());

    QCOMPARE(m_model->count(), 5); // "a/", "a/1", "a/2", "3", "4"
    QVERIFY(m_model->isExpanded(0));
}

/**
 * Verify that removing hidden files and folders from the model does not
 * result in a crash, see https://bugs.kde.org/show_bug.cgi?id=314046
 */
void KFileItemModelTest::testRemoveHiddenItems()
{
    m_testDir->createDir(".a");
    m_testDir->createDir(".b");
    m_testDir->createDir("c");
    m_testDir->createDir("d");
    m_testDir->createFiles({".f", ".g", "h", "i"});

    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QSignalSpy itemsRemovedSpy(m_model, &KFileItemModel::itemsRemoved);

    m_model->setShowHiddenFiles(true);
    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << ".a" << ".b" << "c" << "d" <<".f" << ".g" << "h" << "i");
    QCOMPARE(itemsInsertedSpy.count(), 1);
    QCOMPARE(itemsRemovedSpy.count(), 0);
    KItemRangeList itemRangeList = itemsInsertedSpy.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(0, 8));

    m_model->setShowHiddenFiles(false);
    QCOMPARE(itemsInModel(), QStringList() << "c" << "d" << "h" << "i");
    QCOMPARE(itemsInsertedSpy.count(), 0);
    QCOMPARE(itemsRemovedSpy.count(), 1);
    itemRangeList = itemsRemovedSpy.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(0, 2) << KItemRange(4, 2));

    m_model->setShowHiddenFiles(true);
    QCOMPARE(itemsInModel(), QStringList() << ".a" << ".b" << "c" << "d" <<".f" << ".g" << "h" << "i");
    QCOMPARE(itemsInsertedSpy.count(), 1);
    QCOMPARE(itemsRemovedSpy.count(), 0);
    itemRangeList = itemsInsertedSpy.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(0, 2) << KItemRange(2, 2));

    m_model->clear();
    QCOMPARE(itemsInModel(), QStringList());
    QCOMPARE(itemsInsertedSpy.count(), 0);
    QCOMPARE(itemsRemovedSpy.count(), 1);
    itemRangeList = itemsRemovedSpy.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(0, 8));

    // Hiding hidden files makes the dir lister emit its itemsDeleted signal.
    // Verify that this does not make the model crash.
    m_model->setShowHiddenFiles(false);
}

/**
 * Verify that filtered items are removed when their parent is collapsed.
 */
void KFileItemModelTest::collapseParentOfHiddenItems()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QSignalSpy itemsRemovedSpy(m_model, &KFileItemModel::itemsRemoved);

    QSet<QByteArray> modelRoles = m_model->roles();
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    m_model->setRoles(modelRoles);

    m_testDir->createFiles({"a/1", "a/b/1", "a/b/c/1", "a/b/c/d/1"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 1); // Only "a/"

    // Expand "a/".
    m_model->setExpanded(0, true);
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 3); // 3 items: "a/", "a/b/", "a/1"

    // Expand "a/b/".
    m_model->setExpanded(1, true);
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 5); // 5 items: "a/", "a/b/", "a/b/c", "a/b/1", "a/1"

    // Expand "a/b/c/".
    m_model->setExpanded(2, true);
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 7); // 7 items: "a/", "a/b/", "a/b/c", "a/b/c/d/", "a/b/c/1", "a/b/1", "a/1"

    // Set a name filter that matches nothing -> only the expanded folders remain.
    m_model->setNameFilter("xyz");
    QCOMPARE(itemsRemovedSpy.count(), 1);
    QCOMPARE(m_model->count(), 3);
    QCOMPARE(itemsInModel(), QStringList() << "a" << "b" << "c");

    // Collapse the folder "a/".
    m_model->setExpanded(0, false);
    QCOMPARE(itemsRemovedSpy.count(), 2);
    QCOMPARE(m_model->count(), 1);
    QCOMPARE(itemsInModel(), QStringList() << "a");

    // Remove the filter -> no files should appear (and we should not get a crash).
    m_model->setNameFilter(QString());
    QCOMPARE(m_model->count(), 1);
}

/**
 * Verify that filtered items are removed when their parent is deleted.
 */
void KFileItemModelTest::removeParentOfHiddenItems()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QSignalSpy itemsRemovedSpy(m_model, &KFileItemModel::itemsRemoved);

    QSet<QByteArray> modelRoles = m_model->roles();
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    m_model->setRoles(modelRoles);

    m_testDir->createFiles({"a/1", "a/b/1", "a/b/c/1", "a/b/c/d/1"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 1); // Only "a/"

    // Expand "a/".
    m_model->setExpanded(0, true);
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 3); // 3 items: "a/", "a/b/", "a/1"

    // Expand "a/b/".
    m_model->setExpanded(1, true);
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 5); // 5 items: "a/", "a/b/", "a/b/c", "a/b/1", "a/1"

    // Expand "a/b/c/".
    m_model->setExpanded(2, true);
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 7); // 7 items: "a/", "a/b/", "a/b/c", "a/b/c/d/", "a/b/c/1", "a/b/1", "a/1"

    // Set a name filter that matches nothing -> only the expanded folders remain.
    m_model->setNameFilter("xyz");
    QCOMPARE(itemsRemovedSpy.count(), 1);
    QCOMPARE(m_model->count(), 3);
    QCOMPARE(itemsInModel(), QStringList() << "a" << "b" << "c");

    // Simulate the deletion of the directory "a/b/".
    m_model->slotItemsDeleted(KFileItemList() << m_model->fileItem(1));
    QCOMPARE(itemsRemovedSpy.count(), 2);
    QCOMPARE(m_model->count(), 1);
    QCOMPARE(itemsInModel(), QStringList() << "a");

    // Remove the filter -> only the file "a/1" should appear.
    m_model->setNameFilter(QString());
    QCOMPARE(m_model->count(), 2);
    QCOMPARE(itemsInModel(), QStringList() << "a" << "1");
}

/**
 * Create a tree structure where parent-child relationships can not be
 * determined by parsing the URLs, and verify that KFileItemModel
 * handles them correctly.
 */
void KFileItemModelTest::testGeneralParentChildRelationships()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QSignalSpy itemsRemovedSpy(m_model, &KFileItemModel::itemsRemoved);

    QSet<QByteArray> modelRoles = m_model->roles();
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    m_model->setRoles(modelRoles);

    m_testDir->createFiles({"parent1/realChild1/realGrandChild1", "parent2/realChild2/realGrandChild2"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "parent1" << "parent2");

    // Expand all folders.
    m_model->setExpanded(0, true);
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "parent1" << "realChild1" << "parent2");

    m_model->setExpanded(1, true);
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "parent1" << "realChild1" << "realGrandChild1" << "parent2");

    m_model->setExpanded(3, true);
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "parent1" << "realChild1" << "realGrandChild1" << "parent2" << "realChild2");

    m_model->setExpanded(4, true);
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "parent1" << "realChild1" << "realGrandChild1" << "parent2" << "realChild2" << "realGrandChild2");

    // Add some more children and grand-children.
    const QUrl parent1 = m_model->fileItem(0).url();
    const QUrl parent2 = m_model->fileItem(3).url();
    const QUrl realChild1 = m_model->fileItem(1).url();
    const QUrl realChild2 = m_model->fileItem(4).url();

    m_model->slotItemsAdded(parent1, KFileItemList() << KFileItem(QUrl("child1"), QString(), KFileItem::Unknown));
    m_model->slotCompleted();
    QCOMPARE(itemsInModel(), QStringList() << "parent1" << "realChild1" << "realGrandChild1" << "child1" << "parent2" << "realChild2" << "realGrandChild2");

    m_model->slotItemsAdded(parent2, KFileItemList() << KFileItem(QUrl("child2"), QString(), KFileItem::Unknown));
    m_model->slotCompleted();
    QCOMPARE(itemsInModel(), QStringList() << "parent1" << "realChild1" << "realGrandChild1" << "child1" << "parent2" << "realChild2" << "realGrandChild2" << "child2");

    m_model->slotItemsAdded(realChild1, KFileItemList() << KFileItem(QUrl("grandChild1"), QString(), KFileItem::Unknown));
    m_model->slotCompleted();
    QCOMPARE(itemsInModel(), QStringList() << "parent1" << "realChild1" << "grandChild1" << "realGrandChild1" << "child1" << "parent2" << "realChild2" << "realGrandChild2" << "child2");

    m_model->slotItemsAdded(realChild1, KFileItemList() << KFileItem(QUrl("grandChild1"), QString(), KFileItem::Unknown));
    m_model->slotCompleted();
    QCOMPARE(itemsInModel(), QStringList() << "parent1" << "realChild1" << "grandChild1" << "realGrandChild1" << "child1" << "parent2" << "realChild2" << "realGrandChild2" << "child2");

    m_model->slotItemsAdded(realChild2, KFileItemList() << KFileItem(QUrl("grandChild2"), QString(), KFileItem::Unknown));
    m_model->slotCompleted();
    QCOMPARE(itemsInModel(), QStringList() << "parent1" << "realChild1" << "grandChild1" << "realGrandChild1" << "child1" << "parent2" << "realChild2" << "grandChild2" << "realGrandChild2" << "child2");

    // Set a name filter that matches nothing -> only expanded folders remain.
    m_model->setNameFilter("xyz");
    QCOMPARE(itemsInModel(), QStringList() << "parent1" << "realChild1" << "parent2" << "realChild2");
    QCOMPARE(itemsRemovedSpy.count(), 1);
    QList<QVariant> arguments = itemsRemovedSpy.takeFirst();
    KItemRangeList itemRangeList = arguments.at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(2, 3) << KItemRange(7, 3));

    // Collapse "parent1".
    m_model->setExpanded(0, false);
    QCOMPARE(itemsInModel(), QStringList() << "parent1" << "parent2" << "realChild2");
    QCOMPARE(itemsRemovedSpy.count(), 1);
    arguments = itemsRemovedSpy.takeFirst();
    itemRangeList = arguments.at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(1, 1));

    // Remove "parent2".
    m_model->slotItemsDeleted(KFileItemList() << m_model->fileItem(1));
    QCOMPARE(itemsInModel(), QStringList() << "parent1");
    QCOMPARE(itemsRemovedSpy.count(), 1);
    arguments = itemsRemovedSpy.takeFirst();
    itemRangeList = arguments.at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(1, 2));

    // Clear filter, verify that no items reappear.
    m_model->setNameFilter(QString());
    QCOMPARE(itemsInModel(), QStringList() << "parent1");
}

void KFileItemModelTest::testNameRoleGroups()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QSignalSpy itemsMovedSpy(m_model, &KFileItemModel::itemsMoved);
    QVERIFY(itemsMovedSpy.isValid());
    QSignalSpy groupsChangedSpy(m_model, &KFileItemModel::groupsChanged);
    QVERIFY(groupsChangedSpy.isValid());

    m_testDir->createFiles({"b.txt", "c.txt", "d.txt", "e.txt"});

    m_model->setGroupedSorting(true);
    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "b.txt" << "c.txt" << "d.txt" << "e.txt");

    QList<QPair<int, QVariant> > expectedGroups;
    expectedGroups << QPair<int, QVariant>(0, QLatin1String("B"));
    expectedGroups << QPair<int, QVariant>(1, QLatin1String("C"));
    expectedGroups << QPair<int, QVariant>(2, QLatin1String("D"));
    expectedGroups << QPair<int, QVariant>(3, QLatin1String("E"));
    QCOMPARE(m_model->groups(), expectedGroups);

    // Rename d.txt to a.txt.
    QHash<QByteArray, QVariant> data;
    data.insert("text", "a.txt");
    m_model->setData(2, data);
    QVERIFY(itemsMovedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "b.txt" << "c.txt" << "e.txt");

    expectedGroups.clear();
    expectedGroups << QPair<int, QVariant>(0, QLatin1String("A"));
    expectedGroups << QPair<int, QVariant>(1, QLatin1String("B"));
    expectedGroups << QPair<int, QVariant>(2, QLatin1String("C"));
    expectedGroups << QPair<int, QVariant>(3, QLatin1String("E"));
    QCOMPARE(m_model->groups(), expectedGroups);

    // Rename c.txt to d.txt.
    data.insert("text", "d.txt");
    m_model->setData(2, data);
    QVERIFY(groupsChangedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "b.txt" << "d.txt" << "e.txt");

    expectedGroups.clear();
    expectedGroups << QPair<int, QVariant>(0, QLatin1String("A"));
    expectedGroups << QPair<int, QVariant>(1, QLatin1String("B"));
    expectedGroups << QPair<int, QVariant>(2, QLatin1String("D"));
    expectedGroups << QPair<int, QVariant>(3, QLatin1String("E"));
    QCOMPARE(m_model->groups(), expectedGroups);

    // Change d.txt back to c.txt, but this time using the dir lister's refreshItems() signal.
    const KFileItem fileItemD = m_model->fileItem(2);
    KFileItem fileItemC = fileItemD;
    QUrl urlC = fileItemC.url().adjusted(QUrl::RemoveFilename);
    urlC.setPath(urlC.path() + "c.txt");
    fileItemC.setUrl(urlC);

    m_model->slotRefreshItems({qMakePair(fileItemD, fileItemC)});
    QVERIFY(groupsChangedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "b.txt" << "c.txt" << "e.txt");

    expectedGroups.clear();
    expectedGroups << QPair<int, QVariant>(0, QLatin1String("A"));
    expectedGroups << QPair<int, QVariant>(1, QLatin1String("B"));
    expectedGroups << QPair<int, QVariant>(2, QLatin1String("C"));
    expectedGroups << QPair<int, QVariant>(3, QLatin1String("E"));
    QCOMPARE(m_model->groups(), expectedGroups);
}

void KFileItemModelTest::testNameRoleGroupsWithExpandedItems()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);

    QSet<QByteArray> modelRoles = m_model->roles();
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    m_model->setRoles(modelRoles);

    m_testDir->createFiles({"a/b.txt", "a/c.txt", "d/e.txt", "d/f.txt"});

    m_model->setGroupedSorting(true);
    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a" << "d");

    QList<QPair<int, QVariant> > expectedGroups;
    expectedGroups << QPair<int, QVariant>(0, QLatin1String("A"));
    expectedGroups << QPair<int, QVariant>(1, QLatin1String("D"));
    QCOMPARE(m_model->groups(), expectedGroups);

    // Verify that expanding "a" and "d" will not change the groups (except for the index of "D").
    expectedGroups.clear();
    expectedGroups << QPair<int, QVariant>(0, QLatin1String("A"));
    expectedGroups << QPair<int, QVariant>(3, QLatin1String("D"));

    m_model->setExpanded(0, true);
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a" << "b.txt" << "c.txt" << "d");
    QCOMPARE(m_model->groups(), expectedGroups);

    m_model->setExpanded(3, true);
    QVERIFY(m_model->isExpanded(3));
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a" << "b.txt" << "c.txt" << "d" << "e.txt" << "f.txt");
    QCOMPARE(m_model->groups(), expectedGroups);
}

void KFileItemModelTest::testInconsistentModel()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);

    QSet<QByteArray> modelRoles = m_model->roles();
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    m_model->setRoles(modelRoles);

    m_testDir->createFiles({"a/b/c1.txt", "a/b/c2.txt"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a");

    // Expand "a/" and "a/b/".
    m_model->setExpanded(0, true);
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a" << "b");

    m_model->setExpanded(1, true);
    QVERIFY(m_model->isExpanded(1));
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a" << "b" << "c1.txt" << "c2.txt");

    // Add the files "c1.txt" and "c2.txt" to the model also as top-level items.
    // Such a thing can in principle happen when performing a search, and there
    // are files which
    // (a) match the search string, and
    // (b) are children of a folder that matches the search string and is expanded.
    //
    // Note that the first item in the list of added items must be new (i.e., not
    // in the model yet). Otherwise, KFileItemModel::slotItemsAdded() will see that
    // it receives items that are in the model already and ignore them.
    QUrl url(m_model->directory().url() + "/a2");
    KFileItem newItem(url);

    KFileItemList items;
    items << newItem << m_model->fileItem(2) << m_model->fileItem(3);
    m_model->slotItemsAdded(m_model->directory(), items);
    m_model->slotCompleted();
    QCOMPARE(itemsInModel(), QStringList() << "a" << "b" << "c1.txt" << "c2.txt" << "a2" << "c1.txt" << "c2.txt");

    m_model->setExpanded(0, false);

    // Test that the right items have been removed, see
    // https://bugs.kde.org/show_bug.cgi?id=324371
    QCOMPARE(itemsInModel(), QStringList() << "a" << "a2" << "c1.txt" << "c2.txt");

    // Test that resorting does not cause a crash, see
    // https://bugs.kde.org/show_bug.cgi?id=325359
    // The crash is not 100% reproducible, but Valgrind will report an invalid memory access.
    m_model->resortAllItems();

}

void KFileItemModelTest::testChangeRolesForFilteredItems()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);

    QSet<QByteArray> modelRoles = m_model->roles();
    modelRoles << "owner";
    m_model->setRoles(modelRoles);

    m_testDir->createFiles({"a.txt", "aa.txt", "aaa.txt"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "aa.txt" << "aaa.txt");

    for (int index = 0; index < m_model->count(); ++index) {
        // All items should have the "text" and "owner" roles, but not "group".
        QVERIFY(m_model->data(index).contains("text"));
        QVERIFY(m_model->data(index).contains("owner"));
        QVERIFY(!m_model->data(index).contains("group"));
    }

    // Add a filter, such that only "aaa.txt" remains in the model.
    m_model->setNameFilter("aaa");
    QCOMPARE(itemsInModel(), QStringList() << "aaa.txt");

    // Add the "group" role.
    modelRoles << "group";
    m_model->setRoles(modelRoles);

    // Modify the filter, such that "aa.txt" reappears, and verify that all items have the expected roles.
    m_model->setNameFilter("aa");
    QCOMPARE(itemsInModel(), QStringList() << "aa.txt" << "aaa.txt");

    for (int index = 0; index < m_model->count(); ++index) {
        // All items should have the "text", "owner", and "group" roles.
        QVERIFY(m_model->data(index).contains("text"));
        QVERIFY(m_model->data(index).contains("owner"));
        QVERIFY(m_model->data(index).contains("group"));
    }

    // Remove the "owner" role.
    modelRoles.remove("owner");
    m_model->setRoles(modelRoles);

    // Clear the filter, and verify that all items have the expected roles
    m_model->setNameFilter(QString());
    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "aa.txt" << "aaa.txt");

    for (int index = 0; index < m_model->count(); ++index) {
        // All items should have the "text" and "group" roles, but now "owner".
        QVERIFY(m_model->data(index).contains("text"));
        QVERIFY(!m_model->data(index).contains("owner"));
        QVERIFY(m_model->data(index).contains("group"));
    }
}

void KFileItemModelTest::testChangeSortRoleWhileFiltering()
{
    KFileItemList items;

    KIO::UDSEntry entry;
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, 0100000);    // S_IFREG might not be defined on non-Unix platforms.
    entry.insert(KIO::UDSEntry::UDS_ACCESS, 07777);
    entry.insert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, 0);
    entry.insert(KIO::UDSEntry::UDS_GROUP, "group");
    entry.insert(KIO::UDSEntry::UDS_ACCESS_TIME, 0);

    entry.insert(KIO::UDSEntry::UDS_NAME, "a.txt");
    entry.insert(KIO::UDSEntry::UDS_USER, "user-b");
    items.append(KFileItem(entry, m_testDir->url(), false, true));

    entry.insert(KIO::UDSEntry::UDS_NAME, "b.txt");
    entry.insert(KIO::UDSEntry::UDS_USER, "user-c");
    items.append(KFileItem(entry, m_testDir->url(), false, true));

    entry.insert(KIO::UDSEntry::UDS_NAME, "c.txt");
    entry.insert(KIO::UDSEntry::UDS_USER, "user-a");
    items.append(KFileItem(entry, m_testDir->url(), false, true));

    m_model->slotItemsAdded(m_testDir->url(), items);
    m_model->slotCompleted();

    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "b.txt" << "c.txt");

    // Add a filter.
    m_model->setNameFilter("a");
    QCOMPARE(itemsInModel(), QStringList() << "a.txt");

    // Sort by "owner".
    m_model->setSortRole("owner");

    // Clear the filter, and verify that the items are sorted correctly.
    m_model->setNameFilter(QString());
    QCOMPARE(itemsInModel(), QStringList() << "c.txt" << "a.txt" << "b.txt");
}

void KFileItemModelTest::testRefreshFilteredItems()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);

    m_testDir->createFiles({"a.txt", "b.txt", "c.jpg", "d.jpg"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "b.txt" << "c.jpg" << "d.jpg");

    const KFileItem fileItemC = m_model->fileItem(2);

    // Show only the .txt files.
    m_model->setNameFilter(".txt");
    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "b.txt");

    // Rename one of the .jpg files.
    KFileItem fileItemE = fileItemC;
    QUrl urlE = fileItemE.url().adjusted(QUrl::RemoveFilename);
    urlE.setPath(urlE.path() + "/e.jpg");
    fileItemE.setUrl(urlE);

    m_model->slotRefreshItems({qMakePair(fileItemC, fileItemE)});

    // Show all files again, and verify that the model has updated the file name.
    m_model->setNameFilter(QString());
    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "b.txt" << "d.jpg" << "e.jpg");
}

void KFileItemModelTest::testCreateMimeData()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);

    QSet<QByteArray> modelRoles = m_model->roles();
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    m_model->setRoles(modelRoles);

    m_testDir->createFile("a/1");

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a");

    // Expand "a/".
    m_model->setExpanded(0, true);
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a" << "1");

    // Verify that creating the MIME data for a child of an expanded folder does
    // not cause a crash, see https://bugs.kde.org/show_bug.cgi?id=329119
    KItemSet selection;
    selection.insert(1);
    QMimeData* mimeData = m_model->createMimeData(selection);
    delete mimeData;
}

void KFileItemModelTest::testCollapseFolderWhileLoading()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);

    QSet<QByteArray> modelRoles = m_model->roles();
    modelRoles << "isExpanded" << "isExpandable" << "expandedParentsCount";
    m_model->setRoles(modelRoles);

    m_testDir->createFile("a2/b/c1.txt");

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a2");

    // Expand "a2/".
    m_model->setExpanded(0, true);
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a2" << "b");

    // Expand "a2/b/".
    m_model->setExpanded(1, true);
    QVERIFY(m_model->isExpanded(1));
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a2" << "b" << "c1.txt");

    // Simulate that a new item "c2.txt" appears, but that the dir lister's completed()
    // signal is not emitted yet.
    const KFileItem fileItemC1 = m_model->fileItem(2);
    KFileItem fileItemC2 = fileItemC1;
    QUrl urlC2 = fileItemC2.url();
    urlC2 = urlC2.adjusted(QUrl::RemoveFilename);
    urlC2.setPath(urlC2.path() + "c2.txt");
    fileItemC2.setUrl(urlC2);

    const QUrl urlB = m_model->fileItem(1).url();
    m_model->slotItemsAdded(urlB, KFileItemList() << fileItemC2);
    QCOMPARE(itemsInModel(), QStringList() << "a2" << "b" << "c1.txt");

    // Collapse "a2/". This should also remove all its (indirect) children from
    // the model and from the model's m_pendingItemsToInsert member.
    m_model->setExpanded(0, false);
    QCOMPARE(itemsInModel(), QStringList() << "a2");

    // Simulate that the dir lister's completed() signal is emitted. If "c2.txt"
    // is still in m_pendingItemsToInsert, then we might get a crash, see
    // https://bugs.kde.org/show_bug.cgi?id=332102. Even if the crash is not
    // reproducible here, Valgrind will complain, and the item "c2.txt" will appear
    // without parent in the model.
    m_model->slotCompleted();
    QCOMPARE(itemsInModel(), QStringList() << "a2");

    // Expand "a2/" again.
    m_model->setExpanded(0, true);
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a2" << "b");

    // Now simulate that a new folder "a1/" is appears, but that the dir lister's
    // completed() signal is not emitted yet.
    const KFileItem fileItemA2 = m_model->fileItem(0);
    KFileItem fileItemA1 = fileItemA2;
    QUrl urlA1 = fileItemA1.url().adjusted(QUrl::RemoveFilename);
    urlA1.setPath(urlA1.path() + "a1");
    fileItemA1.setUrl(urlA1);

    m_model->slotItemsAdded(m_model->directory(), KFileItemList() << fileItemA1);
    QCOMPARE(itemsInModel(), QStringList() << "a2" << "b");

    // Collapse "a2/". Note that this will cause "a1/" to be added to the model,
    // i.e., the index of "a2/" will change from 0 to 1. Check that this does not
    // confuse the code which collapses the folder.
    m_model->setExpanded(0, false);
    QCOMPARE(itemsInModel(), QStringList() << "a1" << "a2");
    QVERIFY(!m_model->isExpanded(0));
    QVERIFY(!m_model->isExpanded(1));
}

void KFileItemModelTest::testDeleteFileMoreThanOnce()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);

    m_testDir->createFiles({"a.txt", "b.txt", "c.txt", "d.txt"});

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "b.txt" << "c.txt" << "d.txt");

    const KFileItem fileItemB = m_model->fileItem(1);

    // Tell the model that a list of items has been deleted, where "b.txt" appears twice in the list.
    KFileItemList list;
    list << fileItemB << fileItemB;
    m_model->slotItemsDeleted(list);

    QVERIFY(m_model->isConsistent());
    QCOMPARE(itemsInModel(), QStringList() << "a.txt" << "c.txt" << "d.txt");
}

QStringList KFileItemModelTest::itemsInModel() const
{
    QStringList items;
    for (int i = 0; i < m_model->count(); i++) {
        items << m_model->fileItem(i).text();
    }
    return items;
}

QTEST_MAIN(KFileItemModelTest)

#include "kfileitemmodeltest.moc"
