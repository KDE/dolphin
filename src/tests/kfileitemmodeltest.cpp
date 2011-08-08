/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

namespace {
    const int DefaultTimeout = 5000;
};

Q_DECLARE_METATYPE(KItemRangeList)

class KFileItemModelTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testDefaultRoles();
    void testDefaultSortRole();
    void testDefaultGroupRole();
    void testNewItems();
    void testModelConsistencyWhenInsertingItems();
    void testItemRangeConsistencyWhenInsertingItems();

    void testExpansionLevelsCompare_data();
    void testExpansionLevelsCompare();

private:
    bool isModelConsistent() const;

private:
    KFileItemModel* m_model;
    KDirLister* m_dirLister;
    TestDir* m_testDir;
};

void KFileItemModelTest::init()
{
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

void KFileItemModelTest::testDefaultGroupRole()
{
    QVERIFY(m_model->groupRole().isEmpty());
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

void KFileItemModelTest::testModelConsistencyWhenInsertingItems()
{
    QSKIP("Temporary disabled", SkipSingle);

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
    QCOMPARE(itemRangeList.count(), 1);
    KItemRange itemRange = itemRangeList.first();
    QCOMPARE(itemRange.index, 0);
    QCOMPARE(itemRange.count, 3);

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
    QCOMPARE(itemRangeList.count(), 3);
    QCOMPARE(itemRangeList.at(0).index, 0);
    QCOMPARE(itemRangeList.at(0).count, 1);
    QCOMPARE(itemRangeList.at(1).index, 1);
    QCOMPARE(itemRangeList.at(1).count, 2);
    QCOMPARE(itemRangeList.at(2).index, 2);
    QCOMPARE(itemRangeList.at(2).count, 1);
}

void KFileItemModelTest::testExpansionLevelsCompare_data()
{
    QTest::addColumn<QString>("urlA");
    QTest::addColumn<QString>("urlB");
    QTest::addColumn<int>("result");

    QTest::newRow("Equal") << "/a/b" << "/a/b" << 0;
    QTest::newRow("Sub path: A < B") << "/a/b" << "/a/b/c" << -1;
    QTest::newRow("Sub path: A > B") << "/a/b/c" << "/a/b" << +1;
}

void KFileItemModelTest::testExpansionLevelsCompare()
{
    QFETCH(QString, urlA);
    QFETCH(QString, urlB);
    QFETCH(int, result);

    const KFileItem a(KUrl(urlA), QString(), mode_t(-1));
    const KFileItem b(KUrl(urlB), QString(), mode_t(-1));
    QCOMPARE(m_model->expansionLevelsCompare(a, b), result);
}

bool KFileItemModelTest::isModelConsistent() const
{
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

QTEST_KDEMAIN(KFileItemModelTest, NoGUI)

#include "kfileitemmodeltest.moc"
