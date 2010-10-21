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

#include "views/dolphintreeview.h"

#include <qtestkeyboard.h>
#include <QtGui/QStringListModel>

class DolphinTreeViewTest : public QObject
{
    Q_OBJECT

private slots:

    void bug218114_visualRegionForSelection();

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