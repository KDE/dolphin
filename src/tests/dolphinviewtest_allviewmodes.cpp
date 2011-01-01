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

#include "dolphinviewtest_allviewmodes.h"

#include <qtest_kde.h>

#include "testbase.h"

#include "views/dolphinview.h"
#include "views/dolphinmodel.h"
#include "views/dolphindirlister.h"
#include "views/dolphinsortfilterproxymodel.h"

#include <qtestmouse.h>
#include <qtestkeyboard.h>

DolphinViewTest_AllViewModes::DolphinViewTest_AllViewModes() {
    // Need to register KFileItemList for use with QSignalSpy
    qRegisterMetaType<KFileItemList>("KFileItemList");
}

void DolphinViewTest_AllViewModes::init() {
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

    index = itemView()->model()->index(45, 0);
    itemView()->scrollTo(index);
    QTest::mouseClick(itemView()->viewport(), Qt::LeftButton, Qt::ControlModifier, itemView()->visualRect(index).center());
    verifySelectedItemsCount(2);

    index = itemView()->model()->index(48, 0);
    itemView()->scrollTo(index);
    QTest::mouseClick(itemView()->viewport(), Qt::LeftButton, Qt::ShiftModifier, itemView()->visualRect(index).center());
    verifySelectedItemsCount(5);
}

/**
 * testKeyboardFocus() checks whether a view grabs the keyboard focus.
 *
 * A view may never grab the keyboard focus itself and must respect the focus-state
 * when switching the view mode.
 */

void DolphinViewTest_AllViewModes::testKeyboardFocus()
{
    const DolphinView::Mode mode = m_view->mode();

    QVERIFY(!m_view->hasFocus());
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
        if (mode() == DolphinView::ColumnView &&
            itemView()->selectionModel()->selectedIndexes().count() == 0 &&
            itemView()->selectionModel()->hasSelection()) {
            QEXPECT_FAIL("",
                         "The selection model's hasSelection() method returns true, but there are no selected indexes. Needs to be investigated.",
                         Continue);
        }

        QVERIFY(!m_view->hasSelection());
    }
}

#include "dolphinviewtest_allviewmodes.moc"
