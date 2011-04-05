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
    QVERIFY(waitForFinishedPathLoading(&view));
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
    view.setUrl(dir.name() + "51");
    QVERIFY(waitForFinishedPathLoading(&view));
    qApp->sendPostedEvents();

    // Go back, but do not call DolphinView::restoreState()
    view.setUrl(dir.url());
    QVERIFY(waitForFinishedPathLoading(&view));
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
    view.setUrl(dir.name() + "51");
    QVERIFY(waitForFinishedPathLoading(&view));
    qApp->sendPostedEvents();

    // Check that the current item and scroll position are correct if DolphinView::restoreState()
    // is called after the URL change
    view.setUrl(dir.url());
    QDataStream restoreStream(viewState);
    view.restoreState(restoreStream);
    QVERIFY(waitForFinishedPathLoading(&view));
    qApp->sendPostedEvents();

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

QAbstractItemView* DolphinViewTest_AllViewModes::initView(DolphinView* view) const
{
    view->setMode(mode());
    view->resize(200, 300);
    view->show();
    QTest::qWaitForWindowShown(view);
    Q_ASSERT(verifyCorrectViewMode(view));
    reloadViewAndWait(view);
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
