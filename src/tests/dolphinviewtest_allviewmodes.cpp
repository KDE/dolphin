/****************************************************************************
 *   Copyright (C) 2010-2011 by Frank Reininghaus (frank78ac@googlemail.com) *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, write to the                           *
 *   Free Software Foundation, Inc.,                                         *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA              *
 *****************************************************************************/

#include <kdebug.h>

#include "dolphinviewtest_allviewmodes.h"

#include <qtest_kde.h>

#include "testdir.h"

#include "views/dolphinview.h"
#include "views/dolphinmodel.h"
#include "views/dolphindirlister.h"
#include "views/dolphinsortfilterproxymodel.h"
#include "views/zoomlevelinfo.h"

#include <QScrollBar>

#include <qtestmouse.h>
#include <qtestkeyboard.h>

DolphinViewTest_AllViewModes::DolphinViewTest_AllViewModes() {
    // Need to register KFileItemList for use with QSignalSpy
    qRegisterMetaType<KFileItemList>("KFileItemList");
}

/**
 * testSelection() checks the basic selection functionality of DolphinView, including:
 *
 * selectedItems()
 * selectedItemsCount()
 * selectAll()
 * invertSelection()
 * clearSelection()
 * hasSelection()
 *
 * and the signal selectionChanged(const KFileItemList& selection)
 */

Q_DECLARE_METATYPE(KFileItemList)

void DolphinViewTest_AllViewModes::testSelection() {
    TestDir dir;
    const int totalItems = 50;
    for (int i = 0; i < totalItems; i++) {
        dir.createFile(QString("%1").arg(i));
    }

    DolphinView view(dir.url(), 0);
    QAbstractItemView* itemView = initView(&view);

    // Start with an empty selection
    view.clearSelection();

    QCOMPARE(view.selectedItems().count(), 0);
    QCOMPARE(view.selectedItemsCount(), 0);
    QVERIFY(!view.hasSelection());

    // First some simple tests where either all or no items are selected
    view.selectAll();
    verifySelectedItemsCount(&view, totalItems);

    view.invertSelection();
    verifySelectedItemsCount(&view, 0);

    view.invertSelection();
    verifySelectedItemsCount(&view, totalItems);

    view.clearSelection();
    verifySelectedItemsCount(&view, 0);

    // Now we select individual items using mouse clicks
    QModelIndex index = itemView->model()->index(2, 0);
    itemView->scrollTo(index);
    QTest::mouseClick(itemView->viewport(), Qt::LeftButton, Qt::ControlModifier, itemView->visualRect(index).center());
    verifySelectedItemsCount(&view, 1);

    index = itemView->model()->index(totalItems - 5, 0);
    itemView->scrollTo(index);
    QTest::mouseClick(itemView->viewport(), Qt::LeftButton, Qt::ControlModifier, itemView->visualRect(index).center());
    verifySelectedItemsCount(&view, 2);

    index = itemView->model()->index(totalItems - 2, 0);
    itemView->scrollTo(index);
    QTest::mouseClick(itemView->viewport(), Qt::LeftButton, Qt::ShiftModifier, itemView->visualRect(index).center());
    verifySelectedItemsCount(&view, 5);

    view.invertSelection();
    verifySelectedItemsCount(&view, totalItems - 5);

    // Pressing Esc should clear the selection
    QTest::keyClick(itemView->viewport(), Qt::Key_Escape);
    verifySelectedItemsCount(&view, 0);
}

/**
 * Check that setting the directory view properties works.
 */

void DolphinViewTest_AllViewModes::testViewPropertySettings()
{
    // Create some files with different sizes and modification times to check the different sorting options
    QDateTime now = QDateTime::currentDateTime();

    TestDir dir;
    dir.createFile("a", "A file", now.addDays(-3));
    dir.createFile("b", "A larger file", now.addDays(0));
    dir.createDir("c", now.addDays(-2));
    dir.createFile("d", "The largest file in this directory", now.addDays(-1));
    dir.createFile("e", "An even larger file", now.addDays(-4));
    dir.createFile(".f");

    DolphinView view(dir.url(), 0);
    initView(&view);

    // First set all settings to the default.
    view.setSorting(DolphinView::SortByName);
    QCOMPARE(view.sorting(), DolphinView::SortByName);

    view.setSortOrder(Qt::AscendingOrder);
    QCOMPARE(view.sortOrder(), Qt::AscendingOrder);

    view.setSortFoldersFirst(true);
    QVERIFY(view.sortFoldersFirst());

    view.setShowPreview(false);
    QVERIFY(!view.showPreview());

    view.setShowHiddenFiles(false);
    QVERIFY(!view.showHiddenFiles());

    /** Check that the sort order is correct for different kinds of settings */

    // Sort by Name, ascending
    QCOMPARE(view.sorting(), DolphinView::SortByName);
    QCOMPARE(view.sortOrder(), Qt::AscendingOrder);
    QCOMPARE(viewItems(&view), QStringList() << "c" << "a" << "b" << "d" << "e");

    // Sort by Name, descending
    view.setSortOrder(Qt::DescendingOrder);
    QCOMPARE(view.sorting(), DolphinView::SortByName);
    QCOMPARE(view.sortOrder(), Qt::DescendingOrder);
    QCOMPARE(viewItems(&view), QStringList() << "c" << "e" << "d" << "b" << "a");

    // Sort by Size, descending
    view.setSorting(DolphinView::SortBySize);
    QCOMPARE(view.sorting(), DolphinView::SortBySize);
    QCOMPARE(view.sortOrder(), Qt::DescendingOrder);
    QCOMPARE(viewItems(&view), QStringList() << "c" << "d" << "e" << "b" << "a");

    // Sort by Size, ascending
    view.setSortOrder(Qt::AscendingOrder);
    QCOMPARE(view.sorting(), DolphinView::SortBySize);
    QCOMPARE(view.sortOrder(), Qt::AscendingOrder);
    QCOMPARE(viewItems(&view), QStringList() << "c" << "a" << "b" << "e" << "d");

    // Sort by Date, ascending
    view.setSorting(DolphinView::SortByDate);
    QCOMPARE(view.sorting(), DolphinView::SortByDate);
    QCOMPARE(view.sortOrder(), Qt::AscendingOrder);
    QCOMPARE(viewItems(&view), QStringList() << "c" << "e" << "a" << "d" << "b");

    // Sort by Date, descending
    view.setSortOrder(Qt::DescendingOrder);
    QCOMPARE(view.sorting(), DolphinView::SortByDate);
    QCOMPARE(view.sortOrder(), Qt::DescendingOrder);
    QCOMPARE(viewItems(&view), QStringList() << "c" << "b" << "d" << "a" << "e");

    // Disable "Sort Folders First"
    view.setSortFoldersFirst(false);
    QVERIFY(!view.sortFoldersFirst());
    QCOMPARE(viewItems(&view), QStringList()<< "b" << "d" << "c"  << "a" << "e");

    // Try again with Sort by Name, ascending
    view.setSorting(DolphinView::SortByName);
    view.setSortOrder(Qt::AscendingOrder);
    QCOMPARE(view.sorting(), DolphinView::SortByName);
    QCOMPARE(view.sortOrder(), Qt::AscendingOrder);
    QCOMPARE(viewItems(&view), QStringList() << "a" << "b" << "c" << "d" << "e");

    // Show hidden files. This triggers the dir lister
    // -> we have to wait until loading the hidden files is finished
    view.setShowHiddenFiles(true);
    waitForFinishedPathLoading(&view);
    QVERIFY(view.showHiddenFiles());
    QCOMPARE(viewItems(&view), QStringList() << ".f" << "a" << "b" << "c" << "d" << "e");

    // Previews
    view.setShowPreview(true);
    QVERIFY(view.showPreview());

    // TODO: Check that the view properties are restored correctly when changing the folder and then going back.
}

/**
 * testZoomLevel() checks that setting the zoom level works, both using DolphinView's API and using Ctrl+mouse wheel.
 */

void DolphinViewTest_AllViewModes::testZoomLevel()
{
    TestDir dir;
    dir.createFiles(QStringList() << "a" << "b");
    DolphinView view(dir.url(), 0);
    QAbstractItemView* itemView = initView(&view);

    view.setShowPreview(false);
    QVERIFY(!view.showPreview());

    int zoomLevelBackup = view.zoomLevel();

    int zoomLevel = ZoomLevelInfo::minimumLevel();
    view.setZoomLevel(zoomLevel);
    QCOMPARE(view.zoomLevel(), zoomLevel);

    // Increase the zoom level successively to the maximum.
    while(zoomLevel < ZoomLevelInfo::maximumLevel()) {
        zoomLevel++;
        view.setZoomLevel(zoomLevel);
        QCOMPARE(view.zoomLevel(), zoomLevel);
    }

    // Try setting a zoom level larger than the maximum
    view.setZoomLevel(ZoomLevelInfo::maximumLevel() + 1);
    QCOMPARE(view.zoomLevel(), ZoomLevelInfo::maximumLevel());

    // Turn previews on and try setting a zoom level smaller than the minimum
    view.setShowPreview(true);
    QVERIFY(view.showPreview());
    view.setZoomLevel(ZoomLevelInfo::minimumLevel() - 1);
    QCOMPARE(view.zoomLevel(), ZoomLevelInfo::minimumLevel());

    // Turn previews off again and check that the zoom level is restored
    view.setShowPreview(false);
    QVERIFY(!view.showPreview());
    QCOMPARE(view.zoomLevel(), ZoomLevelInfo::maximumLevel());

    // Change the zoom level using Ctrl+mouse wheel
    QModelIndex index = itemView->model()->index(0, 0);
    itemView->scrollTo(index);

    while (view.zoomLevel() > ZoomLevelInfo::minimumLevel()) {
        int oldZoomLevel = view.zoomLevel();
        QWheelEvent wheelEvent(itemView->visualRect(index).center(), -1, Qt::NoButton, Qt::ControlModifier);
        bool wheelEventReceived = qApp->notify(itemView->viewport(), &wheelEvent);
        QVERIFY(wheelEventReceived);
        QVERIFY(view.zoomLevel() < oldZoomLevel);
    }
    QCOMPARE(view.zoomLevel(), ZoomLevelInfo::minimumLevel());

    while (view.zoomLevel() < ZoomLevelInfo::maximumLevel()) {
        int oldZoomLevel = view.zoomLevel();
        QWheelEvent wheelEvent(itemView->visualRect(index).center(), 1, Qt::NoButton, Qt::ControlModifier);
        bool wheelEventReceived = qApp->notify(itemView->viewport(), &wheelEvent);
        QVERIFY(wheelEventReceived);
        QVERIFY(view.zoomLevel() > oldZoomLevel);
    }
    QCOMPARE(view.zoomLevel(), ZoomLevelInfo::maximumLevel());

    // Turn previews on again and check that the zoom level is restored
    view.setShowPreview(true);
    QVERIFY(view.showPreview());
    QCOMPARE(view.zoomLevel(), ZoomLevelInfo::minimumLevel());

    // Restore the initial state
    view.setZoomLevel(zoomLevelBackup);
    view.setShowPreview(false);
    view.setZoomLevel(zoomLevelBackup);
}

/**
 * testSaveAndRestoreState() checks if saving and restoring the view state (current item, scroll position).
 *
 * Note that we call qApp->sendPostedEvents() every time the view's finishedPathLoading(const KUrl&) signal
 * is received. The reason is that the scroll position is restored in the slot restoreContentsPosition(),
 * which is been invoked using a queued connection in DolphinView::slotLoadingCompleted(). To make sure
 * that this slot is really executed before we proceed, we have to empty the event queue using qApp->sendPostedEvents().
 */

void DolphinViewTest_AllViewModes::testSaveAndRestoreState()
{
    const int totalItems = 50;
    TestDir dir;
    for (int i = 0; i < totalItems; i++) {
        dir.createFile(QString("%1").arg(i));
    }
    dir.createDir("51");
    DolphinView view(dir.url(), 0);
    initView(&view);

    // Set sorting settings to the default to make sure that the item positions are reproducible.
    view.setSorting(DolphinView::SortByName);
    QCOMPARE(view.sorting(), DolphinView::SortByName);
    view.setSortOrder(Qt::AscendingOrder);
    QCOMPARE(view.sortOrder(), Qt::AscendingOrder);

    // Make sure that previews are off and that the icon size does not depend on the preview setting.
    // This is needed for the test for bug 270437, see below.
    view.setShowPreview(false);
    int zoomLevel = view.zoomLevel();
    view.setShowPreview(true);
    view.setZoomLevel(zoomLevel);
    view.setShowPreview(false);

    // Select item 45
    const QModelIndex index45 = itemView(&view)->model()->index(45, 0);
    itemView(&view)->scrollTo(index45);
    itemView(&view)->setCurrentIndex(index45);
    const int scrollPosX = itemView(&view)->horizontalScrollBar()->value();
    const int scrollPosY = itemView(&view)->verticalScrollBar()->value();

    // Save the view state
    QByteArray viewState;
    QDataStream saveStream(&viewState, QIODevice::WriteOnly);
    view.saveState(saveStream);

    // Change the URL
    view.setUrl(KUrl(dir.name() + "51"));
    waitForFinishedPathLoading(&view);
    qApp->sendPostedEvents();

    // Go back, but do not call DolphinView::restoreState()
    view.setUrl(dir.url());
    waitForFinishedPathLoading(&view);
    qApp->sendPostedEvents();

    // Verify that the view is scrolled to top-left corner and that item 45 is not the current item.
    // Note that the vertical position of the columns view might not be zero -> skip that part
    // of the check in this case.
    QVERIFY(itemView(&view)->currentIndex() != index45);
    QCOMPARE(itemView(&view)->horizontalScrollBar()->value(), 0);
    if (mode() != DolphinView::ColumnView) {
        QCOMPARE(itemView(&view)->verticalScrollBar()->value(), 0);
    }

    // Change the URL again
    view.setUrl(KUrl(dir.name() + "51"));
    waitForFinishedPathLoading(&view);
    qApp->sendPostedEvents();

    // Check that the current item and scroll position are correct if DolphinView::restoreState()
    // is called after the URL change
    view.setUrl(dir.url());
    QDataStream restoreStream(viewState);
    view.restoreState(restoreStream);
    waitForFinishedPathLoading(&view);
    qApp->sendPostedEvents();

    QCOMPARE(itemView(&view)->currentIndex(), index45);
    QCOMPARE(itemView(&view)->horizontalScrollBar()->value(), scrollPosX);
    QCOMPARE(itemView(&view)->verticalScrollBar()->value(), scrollPosY);

    /**
     * Additionally, we verify the fix for the bug https://bugs.kde.org/show_bug.cgi?id=270437
     * Actually, it's a bug in KFilePreviewGenerator, but it is easier to test it here.
     */

    // Turn previews on.
    view.setShowPreview(true);
    QVERIFY(view.showPreview());

    // We have to process all events in the queue to make sure that previews are really on.
    qApp->sendPostedEvents();

    // Current item and scroll position should not change.
    QCOMPARE(itemView(&view)->currentIndex(), index45);
    QCOMPARE(itemView(&view)->horizontalScrollBar()->value(), scrollPosX);
    QCOMPARE(itemView(&view)->verticalScrollBar()->value(), scrollPosY);

    // Turn previews off again. Before bug 270437, this triggered the dir lister's openUrl() method
    // -> we check that by listening to the view's startedPathLoading() signal and wait until the loading is finished in that case.
    QSignalSpy spy(&view, SIGNAL(startedPathLoading(const KUrl&)));
    view.setShowPreview(false);
    QVERIFY(!view.showPreview());
    qApp->sendPostedEvents();
    if (!spy.isEmpty()) {
        // The dir lister reloads the directory. We wait until the loading is finished.
        waitForFinishedPathLoading(&view);
    }

    // Current item and scroll position should not change.
    QCOMPARE(itemView(&view)->currentIndex(), index45);
    QCOMPARE(itemView(&view)->horizontalScrollBar()->value(), scrollPosX);
    QCOMPARE(itemView(&view)->verticalScrollBar()->value(), scrollPosY);
}

/**
 * testKeyboardFocus() checks whether a view grabs the keyboard focus.
 *
 * A view may never grab the keyboard focus itself and must respect the focus-state
 * when switching the view mode, see
 *
 * https://bugs.kde.org/show_bug.cgi?id=261147
 */

void DolphinViewTest_AllViewModes::testKeyboardFocus()
{
    TestDir dir;
    dir.createFiles(QStringList() << "a" << "b");
    DolphinView view(dir.url(), 0);
    initView(&view);

    // Move the keyboard focus to another widget.
    QWidget widget;
    widget.show();
    QTest::qWaitForWindowShown(&widget);
    widget.setFocus();

    QVERIFY(!view.hasFocus());

    // Switch view modes and verify that the view does not get the focus back
    for (int i = 0; i <= DolphinView::MaxModeEnum; ++i) {
        view.setMode(static_cast<DolphinView::Mode>(i));
        QVERIFY(!view.hasFocus());
    }
}

/**
 * testCutCopyPaste() checks if cutting or copying items in one view and pasting
 * them in another one works.
 */

void DolphinViewTest_AllViewModes::testCutCopyPaste()
{
    TestDir dir1;
    dir1.createFiles(QStringList() << "a" << "b" << "c" << "d");
    DolphinView view1(dir1.url(), 0);
    QAbstractItemView* itemView1 = initView(&view1);

    TestDir dir2;
    dir2.createFiles(QStringList() << "1" << "2" << "3" << "4");
    dir2.createDir("subfolder");
    DolphinView view2(dir2.url(), 0);
    QAbstractItemView* itemView2 = initView(&view2);

    // Make sure that both views are sorted by name in ascending order
    // TODO: Maybe that should be done in initView(), such all tests can rely on it...?
    view1.setSorting(DolphinView::SortByName);
    view1.setSortOrder(Qt::AscendingOrder);
    view2.setSorting(DolphinView::SortByName);
    view2.setSortOrder(Qt::AscendingOrder);
    view2.setSortFoldersFirst(true);

    QCOMPARE(viewItems(&view1), QStringList() << "a" << "b" << "c" << "d");
    QCOMPARE(viewItems(&view2), QStringList() << "subfolder" << "1" << "2" << "3" << "4");

    /** Copy and paste */
    // Select an item ("d") n view1, copy it and paste it in view2.
    // Note that we have to wait for view2's finishedPathLoading() signal because the pasting is done in the background.
    QModelIndex index = itemView1->model()->index(3, 0);
    itemView1->scrollTo(index);
    QTest::mouseClick(itemView1->viewport(), Qt::LeftButton, Qt::ControlModifier, itemView1->visualRect(index).center());
    verifySelectedItemsCount(&view1, 1);
    QCOMPARE(selectedItems(&view1), QStringList() << "d");
    view1.copySelectedItems();
    view2.paste();
    waitForFinishedPathLoading(&view2);
    QCOMPARE(viewItems(&view1), QStringList() << "a" << "b" << "c" << "d");
    QCOMPARE(viewItems(&view2), QStringList() << "subfolder" << "1" << "2" << "3" << "4" << "d");
    // The pasted item should be selected
    QCOMPARE(selectedItems(&view2), QStringList() << "d");

    /** Cut and paste */
    // Select two items ("3", "4") in view2, cut and paste in view1.
    view2.clearSelection();
    index = itemView2->model()->index(3, 0);
    itemView2->scrollTo(index);
    QTest::mouseClick(itemView2->viewport(), Qt::LeftButton, Qt::ControlModifier, itemView2->visualRect(index).center());
    verifySelectedItemsCount(&view2, 1);
    index = itemView2->model()->index(4, 0);
    itemView2->scrollTo(index);
    QTest::mouseClick(itemView2->viewport(), Qt::LeftButton, Qt::ShiftModifier, itemView2->visualRect(index).center());
    verifySelectedItemsCount(&view2, 2);
    QCOMPARE(selectedItems(&view2), QStringList() << "3" << "4");
    view2.cutSelectedItems();
    // In view1, "d" is still selected
    QCOMPARE(selectedItems(&view1), QStringList() << "d");
    // Paste "3" and "4"
    view1.paste();
    waitForFinishedPathLoading(&view1);
    // In principle, KIO could implement copy&paste such that the pasted items are already there, but the cut items
    // have not been removed yet. Therefore, we check the number of items in view2 and also wait for that view's
    // finishedPathLoading() signal if the cut items are still there.
    if (viewItems(&view2).count() > 4) {
        waitForFinishedPathLoading(&view2);
    }
    QCOMPARE(viewItems(&view1), QStringList() << "3" << "4" << "a" << "b" << "c" << "d");
    QCOMPARE(viewItems(&view2), QStringList() << "subfolder" << "1" << "2" << "d");
    // The pasted items ("3", "4") should be selected now, and the previous selection ("d") should be cleared.
    QCOMPARE(selectedItems(&view1), QStringList() << "3" << "4");

    /** Copy and paste into subfolder */
    view1.clearSelection();
    index = itemView1->model()->index(3, 0);
    itemView1->scrollTo(index);
    QTest::mouseClick(itemView1->viewport(), Qt::LeftButton, Qt::ControlModifier, itemView1->visualRect(index).center());
    verifySelectedItemsCount(&view1, 1);
    QCOMPARE(selectedItems(&view1), QStringList() << "b");
    view1.copySelectedItems();
    // Now we use view1 to display the subfolder, which is still empty.
    view1.setUrl(KUrl(dir2.name() + "subfolder"));
    waitForFinishedPathLoading(&view1);
    QCOMPARE(viewItems(&view1), QStringList());
    // Select the subfolder.in view2
    view2.clearSelection();
    index = itemView2->model()->index(0, 0);
    itemView2->scrollTo(index);
    QTest::mouseClick(itemView2->viewport(), Qt::LeftButton, Qt::ControlModifier, itemView2->visualRect(index).center());
    verifySelectedItemsCount(&view2, 1);
    // Paste into the subfolder
    view2.pasteIntoFolder();
    waitForFinishedPathLoading(&view1);
    QCOMPARE(viewItems(&view1), QStringList() << "b");
    // The pasted items in view1 are *not* selected now (because the pasting was done indirectly using view2.pasteIntoFolder()).
}

// Private member functions which are used by the tests

/**
 * initView(DolphinView*) sets the correct view mode, shows the view on the screen, and waits until loading the
 * folder in the view is finished.
 *
 * Many unit tests need access to DolphinView's internal item view (icons, details, or columns).
 * Therefore, a pointer to the item view is returned by initView(DolphinView*).
 */

QAbstractItemView* DolphinViewTest_AllViewModes::initView(DolphinView* view) const
{
    QSignalSpy spyFinishedPathLoading(view, SIGNAL(finishedPathLoading(const KUrl&)));
    view->setMode(mode());
    Q_ASSERT(verifyCorrectViewMode(view));
    view->resize(200, 300);
    view->show();
    QTest::qWaitForWindowShown(view);

    // If the DolphinView's finishedPathLoading(const KUrl&) signal has not been received yet,
    // we have to wait a bit more.
    // The reason why the if-statement is needed here is that the signal might have been emitted
    // while we were waiting in QTest::qWaitForWindowShown(view)
    // -> waitForFinishedPathLoading(view) would fail in that case.
    if (spyFinishedPathLoading.isEmpty()) {
        waitForFinishedPathLoading(view);
    }

    return itemView(view);
}

/**
 * verifySelectedItemsCount(int) waits until the DolphinView's selectionChanged(const KFileItemList&)
 * signal is received and checks that the selection state of the view is as expected.
 */

void DolphinViewTest_AllViewModes::verifySelectedItemsCount(DolphinView* view, int itemsCount) const
{
    QSignalSpy spySelectionChanged(view, SIGNAL(selectionChanged(const KFileItemList&)));
    QVERIFY(QTest::kWaitForSignal(view, SIGNAL(selectionChanged(const KFileItemList&)), 500));

    QCOMPARE(view->selectedItems().count(), itemsCount);
    QCOMPARE(view->selectedItemsCount(), itemsCount);
    QCOMPARE(spySelectionChanged.count(), 1);
    QCOMPARE(qvariant_cast<KFileItemList>(spySelectionChanged.at(0).at(0)).count(), itemsCount);
    if (itemsCount) {
        QVERIFY(view->hasSelection());
    }
    else {
        QVERIFY(!view->hasSelection());
    }
}

#include "dolphinviewtest_allviewmodes.moc"
