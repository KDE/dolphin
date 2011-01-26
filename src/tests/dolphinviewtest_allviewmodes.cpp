/*****************************************************************************
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

#include "dolphinviewtest_allviewmodes.h"

#include <qtest_kde.h>

#include "testbase.h"

#include "views/dolphinview.h"
#include "views/dolphinmodel.h"
#include "views/dolphindirlister.h"
#include "views/dolphinsortfilterproxymodel.h"
#include "views/zoomlevelinfo.h"

#include <QtGui/QScrollBar>

#include <qtestmouse.h>
#include <qtestkeyboard.h>

DolphinViewTest_AllViewModes::DolphinViewTest_AllViewModes() {
    // Need to register KFileItemList for use with QSignalSpy
    qRegisterMetaType<KFileItemList>("KFileItemList");
}

void DolphinViewTest_AllViewModes::init() {
    if (mode() == DolphinView::ColumnView) {
        // In Columns View mode, we need to create a new DolphinView after each
        // test to make the file-related tests after the first one pass.
        // TODO: Try to find out if there is a hidden bug in the Columns View that causes this.
        delete m_view;
        m_view = new DolphinView(KUrl(m_path), 0);
    }
    m_view->setMode(mode());
    QVERIFY(verifyCorrectViewMode());
    m_view->resize(200, 300);
    m_view->show();
    QTest::qWaitForWindowShown(m_view);
}

void DolphinViewTest_AllViewModes::cleanup() {
    m_view->hide();
    cleanupTestDir();
}

/**
 * testSelection() checks the basic selection funtionality of DolphinView, including:
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
    const int totalItems = 50;

    for (int i = 0; i < totalItems; i++) {
        createFile(QString("%1").arg(i));
    }
    reloadViewAndWait();

    // Start with an empty selection
    m_view->clearSelection();

    QCOMPARE(m_view->selectedItems().count(), 0);
    QCOMPARE(m_view->selectedItemsCount(), 0);
    QVERIFY(!m_view->hasSelection());

    // First some simple tests where either all or no items are selected
    m_view->selectAll();
    verifySelectedItemsCount(totalItems);

    m_view->invertSelection();
    verifySelectedItemsCount(0);

    m_view->invertSelection();
    verifySelectedItemsCount(totalItems);

    m_view->clearSelection();
    verifySelectedItemsCount(0);

    // Now we select individual items using mouse clicks
    QModelIndex index = itemView()->model()->index(2, 0);
    itemView()->scrollTo(index);
    QTest::mouseClick(itemView()->viewport(), Qt::LeftButton, Qt::ControlModifier, itemView()->visualRect(index).center());
    verifySelectedItemsCount(1);

    index = itemView()->model()->index(totalItems - 5, 0);
    itemView()->scrollTo(index);
    QTest::mouseClick(itemView()->viewport(), Qt::LeftButton, Qt::ControlModifier, itemView()->visualRect(index).center());
    verifySelectedItemsCount(2);

    index = itemView()->model()->index(totalItems - 2, 0);
    itemView()->scrollTo(index);
    QTest::mouseClick(itemView()->viewport(), Qt::LeftButton, Qt::ShiftModifier, itemView()->visualRect(index).center());
    verifySelectedItemsCount(5);

    m_view->invertSelection();
    verifySelectedItemsCount(totalItems - 5);

    // Pressing Esc should clear the selection
    QTest::keyClick(itemView()->viewport(), Qt::Key_Escape);
    verifySelectedItemsCount(0);
}

/**
 * Check that setting the directory view properties works.
 */

void DolphinViewTest_AllViewModes::testViewPropertySettings()
{
    // Create some files with different sizes and modification times to check the different sorting options
    QDateTime now = QDateTime::currentDateTime();

    createFile("a", "A file", now.addDays(-3));
    createFile("b", "A larger file", now.addDays(0));
    createDir("c", now.addDays(-2));
    createFile("d", "The largest file in this directory", now.addDays(-1));
    createFile("e", "An even larger file", now.addDays(-4));
    createFile(".f");

    reloadViewAndWait();

    // First set all settings to the default.
    m_view->setSorting(DolphinView::SortByName);
    QCOMPARE(m_view->sorting(), DolphinView::SortByName);

    m_view->setSortOrder(Qt::AscendingOrder);
    QCOMPARE(m_view->sortOrder(), Qt::AscendingOrder);

    m_view->setSortFoldersFirst(true);
    QVERIFY(m_view->sortFoldersFirst());

    m_view->setShowPreview(false);
    QVERIFY(!m_view->showPreview());

    m_view->setShowHiddenFiles(false);
    QVERIFY(!m_view->showHiddenFiles());

    /** Check that the sort order is correct for different kinds of settings */

    // Sort by Name, ascending
    QCOMPARE(m_view->sorting(), DolphinView::SortByName);
    QCOMPARE(m_view->sortOrder(), Qt::AscendingOrder);
    QCOMPARE(viewItems(), QStringList() << "c" << "a" << "b" << "d" << "e");

    // Sort by Name, descending
    m_view->setSortOrder(Qt::DescendingOrder);
    QCOMPARE(m_view->sorting(), DolphinView::SortByName);
    QCOMPARE(m_view->sortOrder(), Qt::DescendingOrder);
    QCOMPARE(viewItems(), QStringList() << "c" << "e" << "d" << "b" << "a");

    // Sort by Size, descending
    m_view->setSorting(DolphinView::SortBySize);
    QCOMPARE(m_view->sorting(), DolphinView::SortBySize);
    QCOMPARE(m_view->sortOrder(), Qt::DescendingOrder);
    QCOMPARE(viewItems(), QStringList() << "c" << "d" << "e" << "b" << "a");

    // Sort by Size, ascending
    m_view->setSortOrder(Qt::AscendingOrder);
    QCOMPARE(m_view->sorting(), DolphinView::SortBySize);
    QCOMPARE(m_view->sortOrder(), Qt::AscendingOrder);
    QCOMPARE(viewItems(), QStringList() << "c" << "a" << "b" << "e" << "d");

    // Sort by Date, ascending
    m_view->setSorting(DolphinView::SortByDate);
    QCOMPARE(m_view->sorting(), DolphinView::SortByDate);
    QCOMPARE(m_view->sortOrder(), Qt::AscendingOrder);
    QCOMPARE(viewItems(), QStringList() << "c" << "e" << "a" << "d" << "b");

    // Sort by Date, descending
    m_view->setSortOrder(Qt::DescendingOrder);
    QCOMPARE(m_view->sorting(), DolphinView::SortByDate);
    QCOMPARE(m_view->sortOrder(), Qt::DescendingOrder);
    QCOMPARE(viewItems(), QStringList() << "c" << "b" << "d" << "a" << "e");

    // Disable "Sort Folders First"
    m_view->setSortFoldersFirst(false);
    QVERIFY(!m_view->sortFoldersFirst());
    QCOMPARE(viewItems(), QStringList()<< "b" << "d" << "c"  << "a" << "e");

    // Try again with Sort by Name, ascending
    m_view->setSorting(DolphinView::SortByName);
    m_view->setSortOrder(Qt::AscendingOrder);
    QCOMPARE(m_view->sorting(), DolphinView::SortByName);
    QCOMPARE(m_view->sortOrder(), Qt::AscendingOrder);
    QCOMPARE(viewItems(), QStringList() << "a" << "b" << "c" << "d" << "e");

    // Show hidden files. This triggers the dir lister
    // -> we have to wait until loading the hidden files is finished
    m_view->setShowHiddenFiles(true);
    QVERIFY(QTest::kWaitForSignal(m_view, SIGNAL(finishedPathLoading(const KUrl&)), 2000));
    QVERIFY(m_view->showHiddenFiles());
    QCOMPARE(viewItems(), QStringList() << ".f" << "a" << "b" << "c" << "d" << "e");

    // Previews
    m_view->setShowPreview(true);
    QVERIFY(m_view->showPreview());

    // TODO: Check that the view properties are restored correctly when changing the folder and then going back.
}

/**
 * testZoomLevel() checks that setting the zoom level works, both using DolphinView's API and using Ctrl+mouse wheel.
 */

void DolphinViewTest_AllViewModes::testZoomLevel()
{
    createFiles(QStringList() << "a" << "b");
    reloadViewAndWait();

    m_view->setShowPreview(false);
    QVERIFY(!m_view->showPreview());

    int zoomLevelBackup = m_view->zoomLevel();

    int zoomLevel = ZoomLevelInfo::minimumLevel();
    m_view->setZoomLevel(zoomLevel);
    QCOMPARE(m_view->zoomLevel(), zoomLevel);

    // Increase the zoom level successively to the maximum.
    while(zoomLevel < ZoomLevelInfo::maximumLevel()) {
        zoomLevel++;
        m_view->setZoomLevel(zoomLevel);
        QCOMPARE(m_view->zoomLevel(), zoomLevel);
    }

    // Try setting a zoom level larger than the maximum
    m_view->setZoomLevel(ZoomLevelInfo::maximumLevel() + 1);
    QCOMPARE(m_view->zoomLevel(), ZoomLevelInfo::maximumLevel());

    // Turn previews on and try setting a zoom level smaller than the minimum
    m_view->setShowPreview(true);
    QVERIFY(m_view->showPreview());
    m_view->setZoomLevel(ZoomLevelInfo::minimumLevel() - 1);
    QCOMPARE(m_view->zoomLevel(), ZoomLevelInfo::minimumLevel());

    // Turn previews off again and check that the zoom level is restored
    m_view->setShowPreview(false);
    QVERIFY(!m_view->showPreview());
    QCOMPARE(m_view->zoomLevel(), ZoomLevelInfo::maximumLevel());

    // Change the zoom level using Ctrl+mouse wheel
    QModelIndex index = itemView()->model()->index(0, 0);
    itemView()->scrollTo(index);

    while (m_view->zoomLevel() > ZoomLevelInfo::minimumLevel()) {
        int oldZoomLevel = m_view->zoomLevel();
        QWheelEvent wheelEvent(itemView()->visualRect(index).center(), -1, Qt::NoButton, Qt::ControlModifier);
        bool wheelEventReceived = qApp->notify(itemView()->viewport(), &wheelEvent);
        QVERIFY(wheelEventReceived);
        QVERIFY(m_view->zoomLevel() < oldZoomLevel);
    }
    QCOMPARE(m_view->zoomLevel(), ZoomLevelInfo::minimumLevel());

    while (m_view->zoomLevel() < ZoomLevelInfo::maximumLevel()) {
        int oldZoomLevel = m_view->zoomLevel();
        QWheelEvent wheelEvent(itemView()->visualRect(index).center(), 1, Qt::NoButton, Qt::ControlModifier);
        bool wheelEventReceived = qApp->notify(itemView()->viewport(), &wheelEvent);
        QVERIFY(wheelEventReceived);
        QVERIFY(m_view->zoomLevel() > oldZoomLevel);
    }
    QCOMPARE(m_view->zoomLevel(), ZoomLevelInfo::maximumLevel());

    // Turn previews on again and check that the zoom level is restored
    m_view->setShowPreview(true);
    QVERIFY(m_view->showPreview());
    QCOMPARE(m_view->zoomLevel(), ZoomLevelInfo::minimumLevel());

    // Restore the initial state
    m_view->setZoomLevel(zoomLevelBackup);
    m_view->setShowPreview(false);
    m_view->setZoomLevel(zoomLevelBackup);
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

    for (int i = 0; i < totalItems; i++) {
        createFile(QString("%1").arg(i));
    }
    createDir("51");
    reloadViewAndWait();

    // Set sorting settings to the default to make sure that the item positions are reproducible.
    m_view->setSorting(DolphinView::SortByName);
    QCOMPARE(m_view->sorting(), DolphinView::SortByName);
    m_view->setSortOrder(Qt::AscendingOrder);
    QCOMPARE(m_view->sortOrder(), Qt::AscendingOrder);

    const QModelIndex index45 = itemView()->model()->index(45, 0);
    itemView()->scrollTo(index45);
    itemView()->setCurrentIndex(index45);
    const int scrollPosX = itemView()->horizontalScrollBar()->value();
    const int scrollPosY = itemView()->verticalScrollBar()->value();

    // Save the view state
    QByteArray viewState;
    QDataStream saveStream(&viewState, QIODevice::WriteOnly);
    m_view->saveState(saveStream);

    // Change the URL and then go back
    m_view->setUrl(m_path + "/51");
    QVERIFY(QTest::kWaitForSignal(m_view, SIGNAL(finishedPathLoading(const KUrl&)), 2000));
    qApp->sendPostedEvents();

    m_view->setUrl(m_path);
    QVERIFY(QTest::kWaitForSignal(m_view, SIGNAL(finishedPathLoading(const KUrl&)), 2000));
    qApp->sendPostedEvents();

    // Verify that the view is scrolled to top and that item 45 is not the current item
    QVERIFY(itemView()->currentIndex() != index45);
    QCOMPARE(itemView()->horizontalScrollBar()->value(), 0);
    QCOMPARE(itemView()->verticalScrollBar()->value(), 0);

    // Change the URL again
    m_view->setUrl(m_path + "/51");
    QVERIFY(QTest::kWaitForSignal(m_view, SIGNAL(finishedPathLoading(const KUrl&)), 2000));
    qApp->sendPostedEvents();

    // Check that the current item and scroll position are correct if DolphinView::restoreState()
    // is called after the URL change
    m_view->setUrl(m_path);
    QDataStream restoreStream(viewState);
    m_view->restoreState(restoreStream);
    QVERIFY(QTest::kWaitForSignal(m_view, SIGNAL(finishedPathLoading(const KUrl&)), 2000));
    qApp->sendPostedEvents();

    QCOMPARE(itemView()->currentIndex(), index45);
    QCOMPARE(itemView()->horizontalScrollBar()->value(), scrollPosX);
    QCOMPARE(itemView()->verticalScrollBar()->value(), scrollPosY);
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
    const DolphinView::Mode mode = m_view->mode();

    // Move keyboard focus to another widget. To see that this is needed, run only this test,
    // i.e., pass 'testKeyboardFocus' as a parameter on the command line.
    QWidget widget;
    widget.show();
    QTest::qWaitForWindowShown(&widget);
    widget.setFocus();

    QVERIFY(!m_view->hasFocus());

    // Switch view modes and verify that the view does not get the focus back
    for (int i = 0; i <= DolphinView::MaxModeEnum; ++i) {
        m_view->setMode(static_cast<DolphinView::Mode>(i));
        QVERIFY(!m_view->hasFocus());
    }

    m_view->setMode(mode);
}

/**
 * verifySelectedItemsCount(int) waits until the DolphinView's selectionChanged(const KFileItemList&)
 * signal is received and checks that the selection state of the view is as expected.
 */

void DolphinViewTest_AllViewModes::verifySelectedItemsCount(int itemsCount) const
{
    QSignalSpy spySelectionChanged(m_view, SIGNAL(selectionChanged(const KFileItemList&)));
    QVERIFY(QTest::kWaitForSignal(m_view, SIGNAL(selectionChanged(const KFileItemList&)), 500));

    QCOMPARE(m_view->selectedItems().count(), itemsCount);
    QCOMPARE(m_view->selectedItemsCount(), itemsCount);
    QCOMPARE(spySelectionChanged.count(), 1);
    QCOMPARE(qvariant_cast<KFileItemList>(spySelectionChanged.at(0).at(0)).count(), itemsCount);
    if (itemsCount != 0) {
        QVERIFY(m_view->hasSelection());
    }
    else {
        QVERIFY(!m_view->hasSelection());
    }
}

#include "dolphinviewtest_allviewmodes.moc"
