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

#include "testbase.h"

#include <qtest_kde.h>

#include "views/dolphinview.h"
#include "views/dolphinmodel.h"
#include "views/dolphindirlister.h"
#include "views/dolphinsortfilterproxymodel.h"

#include <QAbstractItemView>

QAbstractItemView* TestBase::itemView(const DolphinView* view)
{
    return view->m_viewAccessor.itemView();
}

void TestBase::waitForFinishedPathLoading(DolphinView* view, int milliseconds)
{
    // If the signal is not received, somthing is going seriously wrong.
    // -> assert here rather than continuing, which might result in test failures which are hard to unterstand.
    bool viewHasFinishedLoading = QTest::kWaitForSignal(view, SIGNAL(finishedPathLoading(const KUrl&)), milliseconds);
    Q_ASSERT(viewHasFinishedLoading);
    Q_UNUSED(viewHasFinishedLoading) // suppress compiler warining is asserts are disabled
}

void TestBase::reloadViewAndWait(DolphinView* view)
{
    view->reload();
    waitForFinishedPathLoading(view);
}

QStringList TestBase::viewItems(const DolphinView* view)
{
    QStringList itemList;
    const QAbstractItemModel* model = itemView(view)->model();

    for (int row = 0; row < model->rowCount(); row++) {
        itemList << model->data(model->index(row, 0), Qt::DisplayRole).toString();
    }

    return itemList;
}

QStringList TestBase::selectedItems(const DolphinView* view)
{
    QStringList itemList;
    const QAbstractItemModel* model = itemView(view)->model();
    const QModelIndexList selectedIndexes = itemView(view)->selectionModel()->selectedIndexes();

    for (int row = 0; row < model->rowCount(); row++) {
        const QModelIndex index = model->index(row, 0);
        if (selectedIndexes.contains(index)) {
            itemList << model->data(model->index(row, 0), Qt::DisplayRole).toString();
        }
    }

    return itemList;
}
