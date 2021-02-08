/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemviews/kfileitemlistview.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/private/kfileitemmodeldirlister.h"
#include "testdir.h"

#include <QGraphicsView>
#include <QTest>
#include <QSignalSpy>

class KFileItemListViewTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();
    void testGroupedItemChanges();

private:
    KFileItemListView* m_listView;
    KFileItemModel* m_model;
    TestDir* m_testDir;
    QGraphicsView* m_graphicsView;
};

void KFileItemListViewTest::init()
{
    qRegisterMetaType<KItemRangeList>("KItemRangeList");
    qRegisterMetaType<KFileItemList>("KFileItemList");

    m_testDir = new TestDir();
    m_model = new KFileItemModel();
    m_model->m_dirLister->setAutoUpdate(false);

    m_listView = new KFileItemListView();
    m_listView->onModelChanged(m_model, nullptr);

    m_graphicsView = new QGraphicsView();
    m_graphicsView->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_graphicsView));
}

void KFileItemListViewTest::cleanup()
{
    delete m_graphicsView;
    m_graphicsView = nullptr;

    delete m_listView;
    m_listView = nullptr;

    delete m_model;
    m_model = nullptr;

    delete m_testDir;
    m_testDir = nullptr;
}

/**
 * If grouping is enabled, the group headers must be updated
 * when items have been inserted or removed. This updating
 * may only be done after all multiple ranges have been inserted
 * or removed and not after each individual range (see description
 * in #ifndef QT_NO_DEBUG-block of KItemListView::slotItemsInserted()
 * and KItemListView::slotItemsRemoved()). This test inserts and
 * removes multiple ranges and will trigger the Q_ASSERT in the
 * ifndef QT_NO_DEBUG-block in case if a group-header will be updated
 * too early.
 */
void KFileItemListViewTest::testGroupedItemChanges()
{
    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    QVERIFY(itemsInsertedSpy.isValid());
    QSignalSpy itemsRemovedSpy(m_model, &KFileItemModel::itemsRemoved);
    QVERIFY(itemsRemovedSpy.isValid());

    m_model->setGroupedSorting(true);

    m_testDir->createFiles({"1", "3", "5"});
    m_model->loadDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 3);

    m_testDir->createFiles({"2", "4"});
    m_model->m_dirLister->updateDirectory(m_testDir->url());
    QVERIFY(itemsInsertedSpy.wait());
    QCOMPARE(m_model->count(), 5);

    m_testDir->removeFiles({"1", "3", "5"});
    m_model->m_dirLister->updateDirectory(m_testDir->url());
    QVERIFY(itemsRemovedSpy.wait());
    QCOMPARE(m_model->count(), 2);
}

QTEST_MAIN(KFileItemListViewTest)

#include "kfileitemlistviewtest.moc"
