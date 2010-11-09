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

#include "testbase.h"

#include "views/dolphindetailsview.h"
#include "views/dolphinview.h"
#include "views/dolphinmodel.h"
#include "views/dolphindirlister.h"
#include "views/dolphinsortfilterproxymodel.h"
#include "views/zoomlevelinfo.h"

#include <KTempDir>

#include <QtCore/QDir>
#include <qtestmouse.h>
#include <qtestkeyboard.h>

class DolphinDetailsViewTest : public TestBase
{
    Q_OBJECT

private slots:

    void bug217447_shiftArrowSelection();
    void bug234600_overlappingIconsWhenZooming();

};

/**
 * When the first item in the view is active and Shift is held while the "arrow down"
 * key is pressed repeatedly, the selection should grow by one item for each key press.
 * A change in Qt 4.6 revealed a bug in DolphinDetailsView which broke this, see
 *
 * https://bugs.kde.org/show_bug.cgi?id=217447
 *
 * The problem was that DolphinDetailsView, which uses not the full width of the "Name"
 * column for an item, but only the width of the actual file name, did not reimplement
 * QTreeView::visualRect(). This caused item selection to fail because QAbstractItemView
 * uses the center of the visualRect of an item internally. If the width of the file name
 * is less than half the width of the "Name" column, the center of an item's visualRect
 * was therefore outside the space that DolphinDetailsView actually assigned to the
 * item, and this led to unexpected deselection of items.
 *
 * TODO: To make the test more reliable, one could adjust the width of the "Name"
 * column before the test in order to really make sure that the column is more than twice
 * as wide as the space actually occupied by the file names (this triggers the bug).
 */

void DolphinDetailsViewTest::bug217447_shiftArrowSelection()
{
    for (int i = 0; i < 100; i++) {
        createFile(QString("%1").arg(i));
    }

    m_view->setMode(DolphinView::DetailsView);
    DolphinDetailsView* detailsView = qobject_cast<DolphinDetailsView*>(itemView());
    QVERIFY(detailsView);
    m_view->resize(1000, 400);
    m_view->show();
    QTest::qWaitForWindowShown(m_view);
    reloadViewAndWait();

    // Select the first item
    QModelIndex index0 = detailsView->model()->index(0, 0);
    detailsView->setCurrentIndex(index0);
    QCOMPARE(detailsView->currentIndex(), index0);

    // Before we test Shift-selection, we verify that the root cause is fixed a bit more
    // directly: we check that passing the corners or the center of an item's visualRect
    // to itemAt() returns the item (and not an invalid model index).
    QRect rect = detailsView->visualRect(index0);
    QCOMPARE(detailsView->indexAt(rect.center()), index0);
    QCOMPARE(detailsView->indexAt(rect.topLeft()), index0);
    QCOMPARE(detailsView->indexAt(rect.topRight()), index0);
    QCOMPARE(detailsView->indexAt(rect.bottomLeft()), index0);
    QCOMPARE(detailsView->indexAt(rect.bottomRight()), index0);

    // Another way to test this is to Ctrl-click the center of the visualRect.
    // The selection state of the item should be toggled.
    detailsView->clearSelection();
    QItemSelectionModel* selectionModel = detailsView->selectionModel();
    QCOMPARE(selectionModel->selectedIndexes().count(), 0);

    QTest::mouseClick(detailsView->viewport(), Qt::LeftButton, Qt::ControlModifier, rect.center());
    QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
    QCOMPARE(selectedIndexes.count(), 1);
    QVERIFY(selectedIndexes.contains(index0));

    // Now we go down item by item using Shift+Down. In each step, we check that the current item
    // is added to the selection and that the size of the selection grows by one.

    int current = 1;

    while (current < 100) {
        QTest::keyClick(detailsView->viewport(), Qt::Key_Down, Qt::ShiftModifier);
        QModelIndex currentIndex = detailsView->model()->index(current, 0);
        QCOMPARE(detailsView->currentIndex(), currentIndex);

        selectedIndexes = selectionModel->selectedIndexes();
        QCOMPARE(selectedIndexes.count(), current + 1);
        QVERIFY(selectedIndexes.contains(currentIndex));

        current++;
    }

    m_view->hide();
    cleanupTestDir();
}

/**
 * When the icon size is changed, we have to make sure that the maximumSize given
 * to KFileItemDelegate for rendering each item is updated correctly. If this is not
 * done, the visualRects are clipped by the incorrect maximum size, and the icons
 * may overlap, see
 *
 * https://bugs.kde.org/show_bug.cgi?id=234600
 */

void DolphinDetailsViewTest::bug234600_overlappingIconsWhenZooming()
{
    QStringList files;
    files << "a" << "b" << "c" << "d";
    createFiles(files);

    m_view->setMode(DolphinView::DetailsView);
    DolphinDetailsView* detailsView = qobject_cast<DolphinDetailsView*>(itemView());
    QVERIFY(detailsView);
    m_view->resize(400, 400);
    m_view->show();
    QTest::qWaitForWindowShown(m_view);
    reloadViewAndWait();

    QModelIndex index0 = detailsView->model()->index(0, 0);
    detailsView->setCurrentIndex(index0);
    QCOMPARE(detailsView->currentIndex(), index0);

    // Setting the zoom level to the minimum value and triggering DolphinDetailsView::currentChanged(...)
    // should make sure that the bug is triggered.
    int zoomLevel = ZoomLevelInfo::minimumLevel();
    m_view->setZoomLevel(zoomLevel);

    QModelIndex index1 = detailsView->model()->index(1, 0);
    detailsView->setCurrentIndex(index1);
    QCOMPARE(detailsView->currentIndex(), index1);

    // Increase the zoom level successively to the maximum.
    while(zoomLevel < ZoomLevelInfo::maximumLevel()) {
        zoomLevel++;
        m_view->setZoomLevel(zoomLevel);

        //Check for each zoom level that the height of each item is at least the icon size.
        QVERIFY(detailsView->visualRect(index1).height() >= ZoomLevelInfo::iconSizeForZoomLevel(zoomLevel));
    }

    m_view->hide();
    cleanupTestDir();
}

QTEST_KDEMAIN(DolphinDetailsViewTest, GUI)

#include "dolphindetailsviewtest.moc"