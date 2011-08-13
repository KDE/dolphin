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

#include "kitemviews/kitemmodelbase.h"
#include "kitemviews/kitemlistselectionmanager.h"

class DummyModel : public KItemModelBase
{
public:
    DummyModel();
    virtual int count() const;
    virtual QHash<QByteArray, QVariant> data(int index) const;
};

DummyModel::DummyModel() :
    KItemModelBase()
{
}

int DummyModel::count() const
{
    return 100;
}

QHash<QByteArray, QVariant> DummyModel::data(int index) const
{
    Q_UNUSED(index);
    return QHash<QByteArray, QVariant>();
}



class KItemListSelectionManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testConstructor();

    void testCurrentItemAnchorItem();
    void testSetSelected_data();
    void testSetSelected();
    void testItemsInserted();
    void testItemsRemoved();
    void testAnchoredSelection();
    void testChangeSelection_data();
    void testChangeSelection();

private:
    KItemListSelectionManager* m_selectionManager;
};

void KItemListSelectionManagerTest::init()
{
    m_selectionManager = new KItemListSelectionManager();
    m_selectionManager->setModel(new DummyModel());
}

void KItemListSelectionManagerTest::cleanup()
{
    delete m_selectionManager->model();
    delete m_selectionManager;
    m_selectionManager = 0;
}

void KItemListSelectionManagerTest::testConstructor()
{
    QVERIFY(!m_selectionManager->hasSelection());
    QCOMPARE(m_selectionManager->selectedItems().count(), 0);
    QCOMPARE(m_selectionManager->currentItem(), 0);
    QCOMPARE(m_selectionManager->anchorItem(), -1);
}

void KItemListSelectionManagerTest::testCurrentItemAnchorItem()
{
    QSignalSpy spyCurrent(m_selectionManager, SIGNAL(currentChanged(int,int)));
    QSignalSpy spyAnchor(m_selectionManager, SIGNAL(anchorChanged(int,int)));

    m_selectionManager->setAnchoredSelectionActive(true);
    QVERIFY(m_selectionManager->isAnchoredSelectionActive());

    // Set current item and check that the selection manager emits the currentChanged(int,int) signal correctly.
    m_selectionManager->setCurrentItem(4);
    QCOMPARE(m_selectionManager->currentItem(), 4);
    QCOMPARE(spyCurrent.count(), 1);
    QCOMPARE(qvariant_cast<int>(spyCurrent.at(0).at(0)), 4);
    spyCurrent.takeFirst();

    // Set anchor item and check that the selection manager emits the anchorChanged(int,int) signal correctly.
    m_selectionManager->setAnchorItem(3);
    QCOMPARE(m_selectionManager->anchorItem(), 3);
    QCOMPARE(spyAnchor.count(), 1);
    QCOMPARE(qvariant_cast<int>(spyAnchor.at(0).at(0)), 3);
    spyAnchor.takeFirst();

    m_selectionManager->setAnchorItem(5);
    QCOMPARE(m_selectionManager->anchorItem(), 5);
    QCOMPARE(spyAnchor.count(), 1);
    QCOMPARE(qvariant_cast<int>(spyAnchor.at(0).at(0)), 5);
    QCOMPARE(qvariant_cast<int>(spyAnchor.at(0).at(1)), 3);
    spyAnchor.takeFirst();

    // Items between current and anchor should be selected now
    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>() << 4 << 5);
    QVERIFY(m_selectionManager->hasSelection());

    // Change current item again and check the selection
    m_selectionManager->setCurrentItem(2);
    QCOMPARE(m_selectionManager->currentItem(), 2);
    QCOMPARE(spyCurrent.count(), 1);
    QCOMPARE(qvariant_cast<int>(spyCurrent.at(0).at(0)), 2);
    QCOMPARE(qvariant_cast<int>(spyCurrent.at(0).at(1)), 4);
    spyCurrent.takeFirst();

    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>() << 2 << 3 << 4 << 5);
    QVERIFY(m_selectionManager->hasSelection());

    // Inserting items should update current item and anchor item.
    m_selectionManager->itemsInserted(KItemRangeList() <<
                                      KItemRange(0, 1) <<
                                      KItemRange(2, 2) <<
                                      KItemRange(6, 3));

    QCOMPARE(m_selectionManager->currentItem(), 5);
    QCOMPARE(spyCurrent.count(), 1);
    QCOMPARE(qvariant_cast<int>(spyCurrent.at(0).at(0)), 5);
    QCOMPARE(qvariant_cast<int>(spyCurrent.at(0).at(1)), 2);
    spyCurrent.takeFirst();

    QCOMPARE(m_selectionManager->anchorItem(), 8);
    QCOMPARE(spyAnchor.count(), 1);
    QCOMPARE(qvariant_cast<int>(spyAnchor.at(0).at(0)), 8);
    QCOMPARE(qvariant_cast<int>(spyAnchor.at(0).at(1)), 5);
    spyAnchor.takeFirst();

    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>() << 5 << 6 << 7 << 8);
    QVERIFY(m_selectionManager->hasSelection());

    // Removing items should update current item and anchor item.
    m_selectionManager->itemsRemoved(KItemRangeList() <<
                                     KItemRange(0, 2) <<
                                     KItemRange(2, 1) <<
                                     KItemRange(9, 2));

    QCOMPARE(m_selectionManager->currentItem(), 2);
    QCOMPARE(spyCurrent.count(), 1);
    QCOMPARE(qvariant_cast<int>(spyCurrent.at(0).at(0)), 2);
    QCOMPARE(qvariant_cast<int>(spyCurrent.at(0).at(1)), 5);
    spyCurrent.takeFirst();

    QCOMPARE(m_selectionManager->anchorItem(), 5);
    QCOMPARE(spyAnchor.count(), 1);
    QCOMPARE(qvariant_cast<int>(spyAnchor.at(0).at(0)), 5);
    QCOMPARE(qvariant_cast<int>(spyAnchor.at(0).at(1)), 8);
    spyAnchor.takeFirst();

    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>() << 2 << 3 << 4 << 5);
    QVERIFY(m_selectionManager->hasSelection());

    // Verify that clearSelection() also clears the anchored selection.
    m_selectionManager->clearSelection();
    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>());
    QVERIFY(!m_selectionManager->hasSelection());
}

void KItemListSelectionManagerTest::testSetSelected_data()
{
    QTest::addColumn<int>("index");
    QTest::addColumn<int>("count");
    QTest::addColumn<int>("expectedSelectionCount");

    QTest::newRow("Select all") << 0 << 100 << 100;
    QTest::newRow("Sub selection 15 items") << 20 << 15 << 15;
    QTest::newRow("Sub selection 1 item") << 20 << 1 << 1;
    QTest::newRow("Too small index") << -1 << 100 << 0;
    QTest::newRow("Too large index") << 100 << 100 << 0;
    QTest::newRow("Too large count") << 0 << 100000 << 100;
    QTest::newRow("Too small count") << 0 << 0 << 0;
}

void KItemListSelectionManagerTest::testSetSelected()
{
    QFETCH(int, index);
    QFETCH(int, count);
    QFETCH(int, expectedSelectionCount);
    m_selectionManager->setSelected(index, count);
    QCOMPARE(m_selectionManager->selectedItems().count(), expectedSelectionCount);
}

void KItemListSelectionManagerTest::testItemsInserted()
{
    // Select items 10 to 12
    m_selectionManager->setSelected(10, 3);
    QSet<int> selectedItems = m_selectionManager->selectedItems();
    QCOMPARE(selectedItems.count(), 3);
    QVERIFY(selectedItems.contains(10));
    QVERIFY(selectedItems.contains(11));
    QVERIFY(selectedItems.contains(12));

    // Insert items 0 to 4 -> selection must be 15 to 17
    m_selectionManager->itemsInserted(KItemRangeList() << KItemRange(0, 5));
    selectedItems = m_selectionManager->selectedItems();
    QCOMPARE(selectedItems.count(), 3);
    QVERIFY(selectedItems.contains(15));
    QVERIFY(selectedItems.contains(16));
    QVERIFY(selectedItems.contains(17));

    // Insert 3 items between the selections
    m_selectionManager->itemsInserted(KItemRangeList() <<
                                      KItemRange(15, 1) <<
                                      KItemRange(16, 1) <<
                                      KItemRange(17, 1));
    selectedItems = m_selectionManager->selectedItems();
    QCOMPARE(selectedItems.count(), 3);
    QVERIFY(selectedItems.contains(16));
    QVERIFY(selectedItems.contains(18));
    QVERIFY(selectedItems.contains(20));
}

void KItemListSelectionManagerTest::testItemsRemoved()
{
    // Select items 10 to 15
    m_selectionManager->setSelected(10, 6);
    QSet<int> selectedItems = m_selectionManager->selectedItems();
    QCOMPARE(selectedItems.count(), 6);
    for (int i = 10; i <= 15; ++i) {
        QVERIFY(selectedItems.contains(i));
    }

    // Remove items 0 to 4 -> selection must be 5 to 10
    m_selectionManager->itemsRemoved(KItemRangeList() << KItemRange(0, 5));
    selectedItems = m_selectionManager->selectedItems();
    QCOMPARE(selectedItems.count(), 6);
    for (int i = 5; i <= 10; ++i) {
        QVERIFY(selectedItems.contains(i));
    }

    // Remove the items 6 , 8 and 10
    m_selectionManager->itemsRemoved(KItemRangeList() <<
                                     KItemRange(6, 1) <<
                                     KItemRange(8, 1) <<
                                     KItemRange(10, 1));
    selectedItems = m_selectionManager->selectedItems();
    QCOMPARE(selectedItems.count(), 3);
    QVERIFY(selectedItems.contains(5));
    QVERIFY(selectedItems.contains(6));
    QVERIFY(selectedItems.contains(7));
}

void KItemListSelectionManagerTest::testAnchoredSelection()
{
    m_selectionManager->beginAnchoredSelection(5);
    QVERIFY(m_selectionManager->isAnchoredSelectionActive());
    QCOMPARE(m_selectionManager->anchorItem(), 5);

    m_selectionManager->setCurrentItem(6);
    QCOMPARE(m_selectionManager->currentItem(), 6);
    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>() << 5 << 6);

    m_selectionManager->setCurrentItem(4);
    QCOMPARE(m_selectionManager->currentItem(), 4);
    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>() << 4 << 5);

    m_selectionManager->setCurrentItem(7);
    QCOMPARE(m_selectionManager->currentItem(), 7);
    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>() << 5 << 6 << 7);

    // Ending the anchored selection should not change the selected items.
    m_selectionManager->endAnchoredSelection();
    QVERIFY(!m_selectionManager->isAnchoredSelectionActive());
    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>() << 5 << 6 << 7);

    // Start a new anchored selection that overlaps the previous one
    m_selectionManager->beginAnchoredSelection(9);
    QVERIFY(m_selectionManager->isAnchoredSelectionActive());
    QCOMPARE(m_selectionManager->anchorItem(), 9);

    m_selectionManager->setCurrentItem(6);
    QCOMPARE(m_selectionManager->currentItem(), 6);
    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>() << 5 << 6 << 7 << 8 << 9);

    m_selectionManager->setCurrentItem(10);
    QCOMPARE(m_selectionManager->currentItem(), 10);
    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>() << 5 << 6 << 7 << 9 << 10);

    m_selectionManager->endAnchoredSelection();
    QVERIFY(!m_selectionManager->isAnchoredSelectionActive());
    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>() << 5 << 6 << 7 << 9 << 10);
}

namespace {
    enum ChangeType {
        NoChange,
        InsertItems,
        RemoveItems,
        EndAnchoredSelection
    };
}

Q_DECLARE_METATYPE(QSet<int>);
Q_DECLARE_METATYPE(ChangeType);
Q_DECLARE_METATYPE(KItemRangeList);

void KItemListSelectionManagerTest::testChangeSelection_data()
{
    QTest::addColumn<QSet<int> >("initialSelection");
    QTest::addColumn<int>("anchor");
    QTest::addColumn<int>("current");
    QTest::addColumn<QSet<int> >("expectedSelection");
    QTest::addColumn<ChangeType>("changeType");
    QTest::addColumn<KItemRangeList>("changedItems");
    QTest::addColumn<QSet<int> >("finalSelection");

    QTest::newRow("No change")
        << (QSet<int>() << 5 << 6)
        << 2 << 3
        << (QSet<int>() << 2 << 3 << 5 << 6)
        << NoChange << KItemRangeList()
        << (QSet<int>() << 2 << 3 << 5 << 6);    

    QTest::newRow("Insert Items")
        << (QSet<int>() << 5 << 6)
        << 2 << 3
        << (QSet<int>() << 2 << 3 << 5 << 6)
        << InsertItems << (KItemRangeList() << KItemRange(1, 1) << KItemRange(5, 2) << KItemRange(10, 5))
        << (QSet<int>() << 3 << 4 << 8 << 9);

    QTest::newRow("Remove Items")
        << (QSet<int>() << 5 << 6)
        << 2 << 3
        << (QSet<int>() << 2 << 3 << 5 << 6)
        << RemoveItems << (KItemRangeList() << KItemRange(1, 1) << KItemRange(3, 1) << KItemRange(10, 5))
        << (QSet<int>() << 1 << 2 << 3 << 4);

    QTest::newRow("Empty Anchored Selection")
        << QSet<int>()
        << 2 << 2
        << QSet<int>()
        << EndAnchoredSelection << KItemRangeList()
        << QSet<int>();
}

void KItemListSelectionManagerTest::testChangeSelection()
{
    QFETCH(QSet<int>, initialSelection);
    QFETCH(int, anchor);
    QFETCH(int, current);
    QFETCH(QSet<int> , expectedSelection);
    QFETCH(ChangeType, changeType);
    QFETCH(KItemRangeList, changedItems);
    QFETCH(QSet<int> , finalSelection);

    QSignalSpy spySelectionChanged(m_selectionManager, SIGNAL(selectionChanged(QSet<int>,QSet<int>)));

    // Initial selection should be empty
    QVERIFY(!m_selectionManager->hasSelection());
    QVERIFY(m_selectionManager->selectedItems().isEmpty());

    // Perform the initial selectiion
    m_selectionManager->setSelectedItems(initialSelection);
    QCOMPARE(m_selectionManager->selectedItems(), initialSelection);
    if (initialSelection.isEmpty()) {
        QVERIFY(!m_selectionManager->hasSelection());
        QCOMPARE(spySelectionChanged.count(), 0);
    }
    else {
        QVERIFY(m_selectionManager->hasSelection());
        QCOMPARE(spySelectionChanged.count(), 1);
        QList<QVariant> arguments = spySelectionChanged.takeFirst();
        QCOMPARE(qvariant_cast<QSet<int> >(arguments.at(0)), initialSelection);
        QCOMPARE(qvariant_cast<QSet<int> >(arguments.at(1)), QSet<int>());
    }

    // Perform an anchored selection.
    // Note that current and anchor index are equal first because this is the case in typical uses of the
    // selection manager, and because this makes it easier to test the correctness of the signal's arguments.
    m_selectionManager->setCurrentItem(anchor);
    m_selectionManager->beginAnchoredSelection(anchor);
    m_selectionManager->setCurrentItem(current);
    QCOMPARE(m_selectionManager->selectedItems(), expectedSelection);
    QCOMPARE(m_selectionManager->hasSelection(), !expectedSelection.isEmpty());
    if (expectedSelection == initialSelection) {
        QCOMPARE(spySelectionChanged.count(), 0);
    }
    else {
        QCOMPARE(spySelectionChanged.count(), 1);
        QList<QVariant> arguments = spySelectionChanged.takeFirst();
        QCOMPARE(qvariant_cast<QSet<int> >(arguments.at(0)), expectedSelection);
        QCOMPARE(qvariant_cast<QSet<int> >(arguments.at(1)), initialSelection);
    }

    // Change the model by inserting or removing items.
    switch (changeType) {
    case InsertItems:
        m_selectionManager->itemsInserted(changedItems);
        break;
    case RemoveItems:
        m_selectionManager->itemsRemoved(changedItems);
        break;
    case EndAnchoredSelection:
        m_selectionManager->endAnchoredSelection();
        QVERIFY(!m_selectionManager->isAnchoredSelectionActive());
        break;
    case NoChange:
        break;
    }

    QCOMPARE(m_selectionManager->selectedItems(), finalSelection);
    QCOMPARE(m_selectionManager->hasSelection(), !finalSelection.isEmpty());
    if (finalSelection == expectedSelection) {
        QCOMPARE(spySelectionChanged.count(), 0);
    }
    else {
        QCOMPARE(spySelectionChanged.count(), 1);
        QList<QVariant> arguments = spySelectionChanged.takeFirst();
        QCOMPARE(qvariant_cast<QSet<int> >(arguments.at(0)), finalSelection);
        QCOMPARE(qvariant_cast<QSet<int> >(arguments.at(1)), expectedSelection);
    }

    // Finally, clear the selection
    m_selectionManager->clearSelection();
    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>());
    QVERIFY(!m_selectionManager->hasSelection());
    if (finalSelection.isEmpty()) {
        // Selection has been empty already
        QCOMPARE(spySelectionChanged.count(), 0);
    }
    else {
        QCOMPARE(spySelectionChanged.count(), 1);
        QList<QVariant> arguments = spySelectionChanged.takeFirst();
        QCOMPARE(qvariant_cast<QSet<int> >(arguments.at(0)), QSet<int>());
        QCOMPARE(qvariant_cast<QSet<int> >(arguments.at(1)), finalSelection);
    }
}

QTEST_KDEMAIN(KItemListSelectionManagerTest, NoGUI)

#include "kitemlistselectionmanagertest.moc"
