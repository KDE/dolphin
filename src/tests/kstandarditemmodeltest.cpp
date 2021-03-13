/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2011 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphindebug.h"

#include "kitemviews/kstandarditem.h"
#include "kitemviews/kstandarditemmodel.h"

#include <QStandardPaths>
#include <QTest>

class KStandardItemModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testNewItems();
    void testRemoveItems();

private:
    bool isModelConsistent() const;

private:
    KStandardItemModel* m_model;
};

void KStandardItemModelTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void KStandardItemModelTest::init()
{
    m_model = new KStandardItemModel();
}

void KStandardItemModelTest::cleanup()
{
    delete m_model;
    m_model = nullptr;
}

void KStandardItemModelTest::testNewItems()
{
    m_model->insertItem(0, new KStandardItem("item 1"));
    m_model->insertItem(0, new KStandardItem("item 2"));
    m_model->insertItem(2, new KStandardItem("item 3"));
    m_model->appendItem(new KStandardItem("item 4"));
    m_model->insertItem(-1, new KStandardItem("invalid 1"));
    m_model->insertItem(5, new KStandardItem("invalid 2"));
    QCOMPARE(m_model->count(), 4);
    QCOMPARE(m_model->item(0)->text(), QString("item 2"));
    QCOMPARE(m_model->item(1)->text(), QString("item 1"));
    QCOMPARE(m_model->item(2)->text(), QString("item 3"));
    QCOMPARE(m_model->item(3)->text(), QString("item 4"));

    QVERIFY(isModelConsistent());
}

void KStandardItemModelTest::testRemoveItems()
{
    for (int i = 1; i <= 10; ++i) {
        m_model->appendItem(new KStandardItem("item " + QString::number(i)));
    }

    m_model->removeItem(0);
    m_model->removeItem(3);
    m_model->removeItem(5);
    m_model->removeItem(-1);
    QCOMPARE(m_model->count(), 7);
    QCOMPARE(m_model->item(0)->text(), QString("item 2"));
    QCOMPARE(m_model->item(1)->text(), QString("item 3"));
    QCOMPARE(m_model->item(2)->text(), QString("item 4"));
    QCOMPARE(m_model->item(3)->text(), QString("item 6"));
    QCOMPARE(m_model->item(4)->text(), QString("item 7"));
    QCOMPARE(m_model->item(5)->text(), QString("item 9"));
    QCOMPARE(m_model->item(6)->text(), QString("item 10"));
}

bool KStandardItemModelTest::isModelConsistent() const
{
    if (m_model->m_items.count() != m_model->m_indexesForItems.count()) {
        return false;
    }

    for (int i = 0; i < m_model->count(); ++i) {
        const KStandardItem* item = m_model->item(i);
        if (!item) {
            qCWarning(DolphinDebug) << "Item" << i << "is null";
            return false;
        }

        const int itemIndex = m_model->index(item);
        if (itemIndex != i) {
            qCWarning(DolphinDebug) << "Item" << i << "has a wrong index:" << itemIndex;
            return false;
        }
    }

    return true;
}

QTEST_GUILESS_MAIN(KStandardItemModelTest)

#include "kstandarditemmodeltest.moc"
