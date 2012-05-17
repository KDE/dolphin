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

#include "kitemviews/kfileitemlistview.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/private/kfileitemmodeldirlister.h"
#include "testdir.h"

#include <QGraphicsView>

namespace {
    const int DefaultTimeout = 2000;
};

class KFileItemListViewTest : public QObject
{
    Q_OBJECT

private slots:
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
    m_listView->onModelChanged(m_model, 0);

    m_graphicsView = new QGraphicsView();
    m_graphicsView->show();
    QTest::qWaitForWindowShown(m_graphicsView);
}

void KFileItemListViewTest::cleanup()
{
    delete m_graphicsView;
    m_graphicsView = 0;

    delete m_listView;
    m_listView = 0;

    delete m_model;
    m_model = 0;

    delete m_testDir;
    m_testDir = 0;
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
    m_model->setGroupedSorting(true);

    m_testDir->createFiles(QStringList() << "1" << "3" << "5");

    m_model->loadDirectory(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 3);

    m_testDir->createFiles(QStringList() << "2" << "4");
    m_model->m_dirLister->updateDirectory(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 5);

    m_testDir->removeFile("1");
    m_testDir->removeFile("3");
    m_testDir->removeFile("5");
    m_model->m_dirLister->updateDirectory(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsRemoved(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 2);
}

QTEST_KDEMAIN(KFileItemListViewTest, GUI)

#include "kfileitemlistviewtest.moc"
