/***************************************************************************
 *   Copyright (C) 2010 by Frank Reininghaus (frank78ac@googlemail.com)    *
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
#include <kdebug.h> 

#include "views/dolphintreeview.h"

#include <qtestkeyboard.h>
#include <qtestmouse.h>
#include <QtGui/QStringListModel>

class DolphinTreeViewTest : public QObject
{
    Q_OBJECT

private slots:

    void testKeyboardNavigationSelectionUpdate();

    void bug218114_visualRegionForSelection();

private:

    /** A method that simplifies checking a view's current item and selection */
    static void verifyCurrentItemAndSelection(const QAbstractItemView& view, const QModelIndex& expectedCurrent, const QModelIndexList& expectedSelection) {
        QCOMPARE(view.currentIndex(), expectedCurrent);
        const QModelIndexList selectedIndexes = view.selectionModel()->selectedIndexes();
        QCOMPARE(selectedIndexes.count(), expectedSelection.count());
        foreach(const QModelIndex& index, expectedSelection) {
            QVERIFY(selectedIndexes.contains(index));
        }
    }

    /** Use this method if only one item is selected */
    static void verifyCurrentItemAndSelection(const QAbstractItemView& view, const QModelIndex& current, const QModelIndex& selected) {
        QModelIndexList list;
        list << selected;
        verifyCurrentItemAndSelection(view, current, list);
    }

    /** Use this method if the only selected item is the current item */
    static void verifyCurrentItemAndSelection(const QAbstractItemView& view, const QModelIndex& current) {
        verifyCurrentItemAndSelection(view, current, current);
    }

};

/**
 * TestView is a simple view class derived from DolphinTreeView.
 * It makes sure that the visualRect for each index contains only the item text as
 * returned by QAbstractItemModel::data(...) for the role Qt::DisplayRole.
 *
 * We have to check that DolphinTreeView handles the case of visualRects with different widths
 * correctly because this is the case in DolphinDetailsView which is derived from DolphinTreeView.
 */

class TestView : public DolphinTreeView
{
    Q_OBJECT

public:

    TestView(QWidget* parent = 0) : DolphinTreeView(parent) {};
    ~TestView() {};

    QRect visualRect(const QModelIndex& index) const {
        QRect rect = DolphinTreeView::visualRect(index);

        const QStyleOptionViewItem option = viewOptions();
        const QFontMetrics fontMetrics(option.font);
        int width = option.decorationSize.width() + fontMetrics.width(model()->data(index).toString());

        rect.setWidth(width);
        return rect;
    }

};

/**
 * This test checks that updating the selection after key presses works as expected.
 * Qt does not handle this internally if the first letter of an item is pressed, which
 * is why DolphinTreeView has some custom code for this. The test verifies that this
 * works without unwanted side effects.
 *
 * TODO: Add test for deletion of multiple files if Shift-Delete is pressed for some time, see
 * https://bugs.kde.org/show_bug.cgi?id=259656
 */

void DolphinTreeViewTest::testKeyboardNavigationSelectionUpdate() {
    QStringList items;
    items << "a" << "b" << "c" << "d" << "e";
    QStringListModel model(items);

    QModelIndex index[5];
    for (int i = 0; i < 5; i++) {
        index[i] = model.index(i, 0);
    }

    DolphinTreeView view;
    view.setModel(&model);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.resize(400, 400);
    view.show();
    QTest::qWaitForWindowShown(&view);

    view.clearSelection();
    QVERIFY(view.selectionModel()->selectedIndexes().isEmpty());

    /**
     * Check that basic keyboard navigation with arrow keys works.
     */

    view.setCurrentIndex(index[0]);
    verifyCurrentItemAndSelection(view, index[0]);

    // Go down -> item 1 ("b") should be selected
    kDebug() << "Down";
    QTest::keyClick(view.viewport(), Qt::Key_Down);
    verifyCurrentItemAndSelection(view, index[1]);

    // Go down -> item 2 ("c") should be selected
    kDebug() << "Down";
    QTest::keyClick(view.viewport(), Qt::Key_Down);
    verifyCurrentItemAndSelection(view, index[2]);

    // Ctrl-Up -> item 2 ("c") remains selected
    kDebug() << "Ctrl-Up";
    QTest::keyClick(view.viewport(), Qt::Key_Up, Qt::ControlModifier);
    verifyCurrentItemAndSelection(view, index[1], index[2]);

    // Go up -> item 0 ("a") should be selected
    kDebug() << "Up";
    QTest::keyClick(view.viewport(), Qt::Key_Up);
    verifyCurrentItemAndSelection(view, index[0]);

    // Shift-Down -> items 0 and 1 ("a" and "b") should be selected
    kDebug() << "Shift-Down";
    QTest::keyClick(view.viewport(), Qt::Key_Down, Qt::ShiftModifier);
    QModelIndexList expectedSelection;
    expectedSelection << index[0] << index[1];
    verifyCurrentItemAndSelection(view, index[1], expectedSelection);

    /**
    * When the first letter of a file name is pressed, this file becomes the current item
    * and gets selected. If the user then Shift-clicks another item, it is expected that
    * all items between these two items get selected. Before the bug
    *
    * https://bugs.kde.org/show_bug.cgi?id=201459
    *
    * was fixed, this was not the case: the starting point for the Shift-selection was not
    * updated if an item was selected by pressing the first letter of the file name.
    */

    view.clearSelection();
    QVERIFY(view.selectionModel()->selectedIndexes().isEmpty());

    // Control-click item 0 ("a")
    kDebug() << "Ctrl-click on \"a\"";
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ControlModifier, view.visualRect(index[0]).center());
    verifyCurrentItemAndSelection(view, index[0]);

    // Press "c", such that item 2 ("c") should be the current one.
    kDebug() << "Press \"c\"";
    QTest::keyClick(view.viewport(), Qt::Key_C);
    verifyCurrentItemAndSelection(view, index[2]);

    // Now Shift-Click the last item ("e"). We expect that 3 items ("c", "d", "e") are selected.
    kDebug() << "Shift-click on \"e\"";
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, view.visualRect(index[4]).center());
    expectedSelection.clear();
    expectedSelection << index[2] << index[3] << index[4];
    verifyCurrentItemAndSelection(view, index[4], expectedSelection);

    /**
     * Starting a drag&drop operation should not clear the selection, see
     * 
     * https://bugs.kde.org/show_bug.cgi?id=158649
     */

    view.clearSelection();
    QVERIFY(view.selectionModel()->selectedIndexes().isEmpty());

    // Click item 0 ("a")
    kDebug() << "Click on \"a\"";
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, view.visualRect(index[0]).center());
    verifyCurrentItemAndSelection(view, index[0]);

    // Shift-Down -> "a" and "b" should be selected
    kDebug() << "Shift-Down";
    QTest::keyClick(view.viewport(), Qt::Key_Down, Qt::ShiftModifier);
    expectedSelection.clear();
    expectedSelection << index[0] << index[1];
    verifyCurrentItemAndSelection(view, index[1], expectedSelection);

    // Press mouse button on item 0 ("a"), but do not release it. Check that the selection is unchanged
    kDebug() << "Mouse press on \"a\"";
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::NoModifier, view.visualRect(index[0]).center());
    verifyCurrentItemAndSelection(view, index[0], expectedSelection);

    // Move mouse to item 1 ("b"), check that selection is unchanged
    kDebug() << "Move mouse to \"b\"";
    QMouseEvent moveEvent(QEvent::MouseMove, view.visualRect(index[1]).center(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    bool moveEventReceived = qApp->notify(view.viewport(), &moveEvent);
    QVERIFY(moveEventReceived);
    verifyCurrentItemAndSelection(view, index[0], expectedSelection);

    // Release mouse button on item 1 ("b"), check that selection is unchanged
    kDebug() << "Mouse release on \"b\"";
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::NoModifier, view.visualRect(index[1]).center());
    verifyCurrentItemAndSelection(view, index[0], expectedSelection);
}

/**
 * QTreeView assumes implicitly that the width of each item's visualRect is the same. This leads to painting
 * problems in Dolphin if items with different widths are in one QItemSelectionRange, see
 *
 * https://bugs.kde.org/show_bug.cgi?id=218114
 *
 * To fix this, DolphinTreeView has a custom implementation of visualRegionForSelection(). The following
 * unit test checks that.
 */

void DolphinTreeViewTest::bug218114_visualRegionForSelection()
{
    QStringList items;
    items << "a" << "an item with a long name" << "a";
    QStringListModel model(items);

    QModelIndex index0 = model.index(0, 0);
    QModelIndex index1 = model.index(1, 0);
    QModelIndex index2 = model.index(2, 0);

    TestView view;
    view.setModel(&model);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.resize(400, 400);
    view.show();
    QTest::qWaitForWindowShown(&view);

    // First check that the width of index1 is larger than that of index0 and index2 (this triggers the bug).

    QVERIFY(view.visualRect(index0).width() < view.visualRect(index1).width());
    QVERIFY(view.visualRect(index2).width() < view.visualRect(index1).width());

    // Select all items in one go.

    view.selectAll();
    const QItemSelection selection = view.selectionModel()->selection();
    QCOMPARE(selection.count(), 1);
    QCOMPARE(selection.indexes().count(), 3);

    // Verify that the visualRegionForSelection contains all visualRects.
    // We do this indirectly using QRegion::boundingRect() because
    // QRegion::contains(const QRect&) returns true even if the QRect is not
    // entirely inside the QRegion.

    const QRegion region = view.visualRegionForSelection(selection);
    const QRect boundingRect = region.boundingRect();

    QVERIFY(boundingRect.contains(view.visualRect(index0)));
    QVERIFY(boundingRect.contains(view.visualRect(index1)));
    QVERIFY(boundingRect.contains(view.visualRect(index2)));
}

QTEST_KDEMAIN(DolphinTreeViewTest, GUI)

#include "dolphintreeviewtest.moc"