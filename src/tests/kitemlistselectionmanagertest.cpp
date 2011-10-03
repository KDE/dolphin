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
    void verifySelectionChange(QSignalSpy& spy, const QSet<int>& currentSelection, const QSet<int>& previousSelection) const;

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
    QCOMPARE(m_selectionManager->m_anchorItem, -1);
}

void KItemListSelectionManagerTest::testCurrentItemAnchorItem()
{
    QSignalSpy spyCurrent(m_selectionManager, SIGNAL(currentChanged(int,int)));

    // Set current item and check that the selection manager emits the currentChanged(int,int) signal correctly.
    m_selectionManager->setCurrentItem(4);
    QCOMPARE(m_selectionManager->currentItem(), 4);
    QCOMPARE(spyCurrent.count(), 1);
    QCOMPARE(qvariant_cast<int>(spyCurrent.at(0).at(0)), 4);
    spyCurrent.takeFirst();

    // Begin an anchored selection.
    m_selectionManager->beginAnchoredSelection(5);
    QVERIFY(m_selectionManager->isAnchoredSelectionActive());
    QCOMPARE(m_selectionManager->m_anchorItem, 5);

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

    QCOMPARE(m_selectionManager->m_anchorItem, 8);

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

    QCOMPARE(m_selectionManager->m_anchorItem, 5);

    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>() << 2 << 3 << 4 << 5);
    QVERIFY(m_selectionManager->hasSelection());

    // Verify that clearSelection() also clears the anchored selection.
    m_selectionManager->clearSelection();
    QCOMPARE(m_selectionManager->selectedItems(), QSet<int>());
    QVERIFY(!m_selectionManager->hasSelection());

    m_selectionManager->endAnchoredSelection();
    QVERIFY(!m_selectionManager->isAnchoredSelectionActive());
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
    QCOMPARE(m_selectionManager->m_anchorItem, 5);

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
    QCOMPARE(m_selectionManager->m_anchorItem, 9);

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
        MoveItems,
        EndAnchoredSelection,
        SetSelected
    };
}

Q_DECLARE_METATYPE(QSet<int>);
Q_DECLARE_METATYPE(ChangeType);
Q_DECLARE_METATYPE(KItemRange);
Q_DECLARE_METATYPE(KItemRangeList);
Q_DECLARE_METATYPE(KItemListSelectionManager::SelectionMode);
Q_DECLARE_METATYPE(QList<int>);

/**
 * The following function provides a generic way to test the selection functionality.
 *
 * The test is data-driven and takes the following arguments:
 * 
 * \param initialSelection  The selection at the beginning.
 * \param anchor            This item will be the anchor item.
 * \param current           This item will be the current item.
 * \param expectedSelection Expected selection after anchor and current are set.
 * \param changeType        Type of the change that is done then:
 *                          - NoChange
 *                          - InsertItems -> data.at(0) provides the KItemRangeList. \sa KItemListSelectionManager::itemsInserted()
 *                          - RemoveItems -> data.at(0) provides the KItemRangeList. \sa KItemListSelectionManager::itemsRemoved()
 *                          - MoveItems   -> data.at(0) provides the KItemRange containing the original indices,
 *                                           data.at(1) provides the list containing the new indices
 *                                          \sa KItemListSelectionManager::itemsMoved(), KItemModelBase::itemsMoved()
 *                          - EndAnchoredSelection
 *                          - SetSelected -> data.at(0) provides the index where the selection process starts,
 *                                           data.at(1) provides the number of indices to be selected,
 *                                           data.at(2) provides the selection mode.
 *                                          \sa KItemListSelectionManager::setSelected()
 * \param data              A list of QVariants which will be cast to the arguments needed for the chosen ChangeType (see above).
 * \param finalSelection    The expected final selection.
 *
 */

void KItemListSelectionManagerTest::testChangeSelection_data()
{
    QTest::addColumn<QSet<int> >("initialSelection");
    QTest::addColumn<int>("anchor");
    QTest::addColumn<int>("current");
    QTest::addColumn<QSet<int> >("expectedSelection");
    QTest::addColumn<ChangeType>("changeType");
    QTest::addColumn<QList<QVariant> >("data");
    QTest::addColumn<QSet<int> >("finalSelection");

    QTest::newRow("No change")
        << (QSet<int>() << 5 << 6)
        << 2 << 3
        << (QSet<int>() << 2 << 3 << 5 << 6)
        << NoChange
        << QList<QVariant>()
        << (QSet<int>() << 2 << 3 << 5 << 6);

    QTest::newRow("Insert Items")
        << (QSet<int>() << 5 << 6)
        << 2 << 3
        << (QSet<int>() << 2 << 3 << 5 << 6)
        << InsertItems
        << (QList<QVariant>() << QVariant::fromValue(KItemRangeList() << KItemRange(1, 1) << KItemRange(5, 2) << KItemRange(10, 5)))
        << (QSet<int>() << 3 << 4 << 8 << 9);

    QTest::newRow("Remove Items")
        << (QSet<int>() << 5 << 6)
        << 2 << 3
        << (QSet<int>() << 2 << 3 << 5 << 6)
        << RemoveItems
        << (QList<QVariant>() << QVariant::fromValue(KItemRangeList() << KItemRange(1, 1) << KItemRange(3, 1) << KItemRange(10, 5)))
        << (QSet<int>() << 1 << 2 << 3 << 4);

    QTest::newRow("Empty Anchored Selection")
        << QSet<int>()
        << 2 << 2
        << QSet<int>()
        << EndAnchoredSelection
        << QList<QVariant>()
        << QSet<int>();

    QTest::newRow("Toggle selection")
        << (QSet<int>() << 1 << 3 << 4)
        << 6 << 8
        << (QSet<int>() << 1 << 3 << 4 << 6 << 7 << 8)
        << SetSelected
        << (QList<QVariant>() << 0 << 10 << QVariant::fromValue(KItemListSelectionManager::Toggle))
        << (QSet<int>() << 0 << 2 << 5 << 9);

    // Swap items 2, 3 and 4, 5
    QTest::newRow("Move items")
        << (QSet<int>() << 0 << 1 << 2 << 3)
        << -1 << -1
        << (QSet<int>() << 0 << 1 << 2 << 3)
        << MoveItems
        << (QList<QVariant>() << QVariant::fromValue(KItemRange(2, 4))
                              << QVariant::fromValue(QList<int>() << 4 << 5 << 2 << 3))
        << (QSet<int>() << 0 << 1 << 4 << 5);

    // Revert sort order
    QTest::newRow("Revert sort order")
        << (QSet<int>() << 0 << 1)
        << 3 << 4
        << (QSet<int>() << 0 << 1 << 3 << 4)
        << MoveItems
        << (QList<QVariant>() << QVariant::fromValue(KItemRange(0, 10))
                              << QVariant::fromValue(QList<int>() << 9 << 8 << 7 << 6 << 5 << 4 << 3 << 2 << 1 << 0))
        << (QSet<int>() << 5 << 6 << 8 << 9);
}

void KItemListSelectionManagerTest::testChangeSelection()
{
    QFETCH(QSet<int>, initialSelection);
    QFETCH(int, anchor);
    QFETCH(int, current);
    QFETCH(QSet<int>, expectedSelection);
    QFETCH(ChangeType, changeType);
    QFETCH(QList<QVariant>, data);
    QFETCH(QSet<int>, finalSelection);

    QSignalSpy spySelectionChanged(m_selectionManager, SIGNAL(selectionChanged(QSet<int>,QSet<int>)));

    // Initial selection should be empty
    QVERIFY(!m_selectionManager->hasSelection());
    QVERIFY(m_selectionManager->selectedItems().isEmpty());

    // Perform the initial selectiion
    m_selectionManager->setSelectedItems(initialSelection);

    verifySelectionChange(spySelectionChanged, initialSelection, QSet<int>());

    // Perform an anchored selection.
    // Note that current and anchor index are equal first because this is the case in typical uses of the
    // selection manager, and because this makes it easier to test the correctness of the signal's arguments.
    m_selectionManager->setCurrentItem(anchor);
    m_selectionManager->beginAnchoredSelection(anchor);
    m_selectionManager->setCurrentItem(current);
    QCOMPARE(m_selectionManager->m_anchorItem, anchor);
    QCOMPARE(m_selectionManager->currentItem(), current);

    verifySelectionChange(spySelectionChanged, expectedSelection, initialSelection);

    // Change the model by inserting or removing items.
    switch (changeType) {
    case InsertItems:
        m_selectionManager->itemsInserted(data.at(0).value<KItemRangeList>());
        break;
    case RemoveItems:
        m_selectionManager->itemsRemoved(data.at(0).value<KItemRangeList>());
        break;
    case MoveItems:
        m_selectionManager->itemsMoved(data.at(0).value<KItemRange>(),
                                       data.at(1).value<QList<int> >());
        break;
    case EndAnchoredSelection:
        m_selectionManager->endAnchoredSelection();
        QVERIFY(!m_selectionManager->isAnchoredSelectionActive());
        break;
    case SetSelected:
        m_selectionManager->setSelected(data.at(0).value<int>(), // index
                                        data.at(1).value<int>(), // count
                                        data.at(2).value<KItemListSelectionManager::SelectionMode>());
        break;
    case NoChange:
        break;
    }

    verifySelectionChange(spySelectionChanged, finalSelection, expectedSelection);

    // Finally, clear the selection
    m_selectionManager->clearSelection();

    verifySelectionChange(spySelectionChanged, QSet<int>(), finalSelection);
}

void KItemListSelectionManagerTest::verifySelectionChange(QSignalSpy& spy,
                                                          const QSet<int>& currentSelection,
                                                          const QSet<int>& previousSelection) const
{
    QCOMPARE(m_selectionManager->selectedItems(), currentSelection);
    QCOMPARE(m_selectionManager->hasSelection(), !currentSelection.isEmpty());
    for (int index = 0; index < m_selectionManager->model()->count(); ++index) {
        if (currentSelection.contains(index)) {
            QVERIFY(m_selectionManager->isSelected(index));
        }
        else {
            QVERIFY(!m_selectionManager->isSelected(index));
        }
    }

    if (currentSelection == previousSelection) {
        QCOMPARE(spy.count(), 0);
    }
    else {
        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(qvariant_cast<QSet<int> >(arguments.at(0)), currentSelection);
        QCOMPARE(qvariant_cast<QSet<int> >(arguments.at(1)), previousSelection);
    }
}

QTEST_KDEMAIN(KItemListSelectionManagerTest, NoGUI)

#include "kitemlistselectionmanagertest.moc"
