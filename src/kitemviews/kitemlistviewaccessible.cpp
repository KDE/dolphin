/***************************************************************************
 *   Copyright (C) 2012 by Amandeep Singh <aman.dedman@gmail.com>          *
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

#ifndef QT_NO_ACCESSIBILITY

#include "kitemlistviewaccessible.h"

#include "kitemlistcontainer.h"
#include "kitemlistcontroller.h"
#include "kitemlistselectionmanager.h"
#include "kitemlistview.h"
#include "private/kitemlistviewlayouter.h"

#include <QtGui/qaccessible2.h>
#include <qgraphicsscene.h>
#include <qgraphicsview.h>

#include <KDebug>
#include <QHash>

KItemListView* KItemListViewAccessible::view() const
{
    return qobject_cast<KItemListView*>(object());
}

KItemListViewAccessible::KItemListViewAccessible(KItemListView* view_) :
    QAccessibleObjectEx(view_)
{
    Q_ASSERT(view());
}

void KItemListViewAccessible::modelReset()
{
}

QAccessible::Role KItemListViewAccessible::cellRole() const
{
    return QAccessible::Cell;
}

QAccessibleTable2CellInterface* KItemListViewAccessible::cell(int index) const
{
    if (index < 0 || index >= view()->model()->count()) {
        return 0;
    } else {
        return new KItemListAccessibleCell(view(), index);
    }
}

QVariant KItemListViewAccessible::invokeMethodEx(Method, int, const QVariantList&)
{
    return QVariant();
}

QAccessibleTable2CellInterface* KItemListViewAccessible::cellAt(int row, int column) const
{
    return cell(columnCount() * row + column);
}

QAccessibleInterface* KItemListViewAccessible::caption() const
{
    return 0;
}

QString KItemListViewAccessible::columnDescription(int) const
{
    return QString();
}

int KItemListViewAccessible::columnCount() const
{
    return view()->m_layouter->columnCount();
}

int KItemListViewAccessible::rowCount() const
{
    if (columnCount() <= 0) {
        return 0;
    }

    int itemCount = view()->model()->count();
    int rowCount = itemCount / columnCount();

    if (rowCount <= 0) {
        return 0;
    }

    if (itemCount % columnCount()) {
        ++rowCount;
    }
    return rowCount;
}

int KItemListViewAccessible::selectedCellCount() const
{
    return view()->controller()->selectionManager()->selectedItems().size();
}

int KItemListViewAccessible::selectedColumnCount() const
{
    return 0;
}

int KItemListViewAccessible::selectedRowCount() const
{
    return 0;
}

QString KItemListViewAccessible::rowDescription(int) const
{
    return QString();
}

QList<QAccessibleTable2CellInterface*> KItemListViewAccessible::selectedCells() const
{
    QList<QAccessibleTable2CellInterface*> cells;
    Q_FOREACH (int index, view()->controller()->selectionManager()->selectedItems()) {
        cells.append(cell(index));
    }
    return cells;
}

QList<int> KItemListViewAccessible::selectedColumns() const
{
    return QList<int>();
}

QList<int> KItemListViewAccessible::selectedRows() const
{
    return QList<int>();
}

QAccessibleInterface* KItemListViewAccessible::summary() const
{
    return 0;
}

bool KItemListViewAccessible::isColumnSelected(int) const
{
    return false;
}

bool KItemListViewAccessible::isRowSelected(int) const
{
    return false;
}

bool KItemListViewAccessible::selectRow(int)
{
    return true;
}

bool KItemListViewAccessible::selectColumn(int)
{
    return true;
}

bool KItemListViewAccessible::unselectRow(int)
{
    return true;
}

bool KItemListViewAccessible::unselectColumn(int)
{
    return true;
}

QAccessible2::TableModelChange KItemListViewAccessible::modelChange() const
{
    QAccessible2::TableModelChange change;
    change.lastRow = rowCount();
    change.lastColumn = columnCount();
    return change;
}

QAccessible::Role KItemListViewAccessible::role(int child) const
{
    Q_ASSERT(child >= 0);

    if (child > 0) {
        return QAccessible::Cell;
    } else {
        return QAccessible::Table;
    }
}

QAccessible::State KItemListViewAccessible::state(int child) const
{
    if (child) {
        QAccessibleInterface* interface = 0;
        navigate(Child, child, &interface);
        if (interface) {
            return interface->state(0);
        }
    }

    return QAccessible::Normal | QAccessible::HasInvokeExtension;
}

int KItemListViewAccessible::childAt(int x, int y) const
{
    const QPointF point = QPointF(x,y);
    return view()->itemAt(view()->mapFromScene(point));
}

int KItemListViewAccessible::childCount() const
{
    return view()->model()->count();
}

int KItemListViewAccessible::indexOfChild(const QAccessibleInterface* interface) const
{
    const KItemListAccessibleCell* widget = static_cast<const KItemListAccessibleCell*>(interface);
    return widget->index() + 1;
}

QString KItemListViewAccessible::text(Text, int child) const
{
    Q_ASSERT(child == 0);
    return QString();
}

QRect KItemListViewAccessible::rect(int child) const
{
    Q_UNUSED(child)
    if (!view()->isVisible()) {
        return QRect();
    }
    const QPoint origin = view()->scene()->views()[0]->mapToGlobal(QPoint(0, 0));
    const QRect viewRect = view()->geometry().toRect();
    return viewRect.translated(origin);
}

int KItemListViewAccessible::navigate(RelationFlag relation, int index, QAccessibleInterface** interface) const
{
    *interface = 0;

    switch (relation) {
    case QAccessible::Child:
        Q_ASSERT(index > 0);
        *interface = cell(index - 1);
        if (*interface) {
            return 0;
        }
        break;

    default:
        break;
    }

    return -1;
}

QAccessible::Relation KItemListViewAccessible::relationTo(int, const QAccessibleInterface*, int) const
{
    return QAccessible::Unrelated;
}

#ifndef QT_NO_ACTION

int KItemListViewAccessible::userActionCount(int) const
{
    return 0;
}

QString KItemListViewAccessible::actionText(int, Text, int) const
{
    return QString();
}

bool KItemListViewAccessible::doAction(int, int, const QVariantList&)
{
    return false;
}

#endif

// Table Cell

KItemListAccessibleCell::KItemListAccessibleCell(KItemListView* view, int index) :
    m_view(view),
    m_index(index)
{
    Q_ASSERT(index >= 0 && index < view->model()->count());
}

int KItemListAccessibleCell::columnExtent() const
{
    return 1;
}

int KItemListAccessibleCell::rowExtent() const
{
    return 1;
}

QList<QAccessibleInterface*> KItemListAccessibleCell::rowHeaderCells() const
{
    return QList<QAccessibleInterface*>();
}

QList<QAccessibleInterface*> KItemListAccessibleCell::columnHeaderCells() const
{
    return QList<QAccessibleInterface*>();
}

int KItemListAccessibleCell::columnIndex() const
{
    return m_view->m_layouter->itemColumn(m_index);
}

int KItemListAccessibleCell::rowIndex() const
{
    return m_view->m_layouter->itemRow(m_index);
}

bool KItemListAccessibleCell::isSelected() const
{
    return m_view->controller()->selectionManager()->isSelected(m_index);
}

void KItemListAccessibleCell::rowColumnExtents(int* row, int* column, int* rowExtents, int* columnExtents, bool* selected) const
{
    const KItemListViewLayouter* layouter = m_view->m_layouter;
    *row = layouter->itemRow(m_index);
    *column = layouter->itemColumn(m_index);
    *rowExtents = 1;
    *columnExtents = 1;
    *selected = isSelected();
}

QAccessibleTable2Interface* KItemListAccessibleCell::table() const
{
    return QAccessible::queryAccessibleInterface(m_view)->table2Interface();
}

QAccessible::Role KItemListAccessibleCell::role(int child) const
{
    Q_ASSERT(child == 0);
    return QAccessible::Cell;
}

QAccessible::State KItemListAccessibleCell::state(int child) const
{
    Q_ASSERT(child == 0);
    QAccessible::State state = Normal;

    if (isSelected()) {
        state |= Selected;
    }

    if (m_view->controller()->selectionManager()->currentItem() == m_index) {
        state |= Focused;
    }

    state |= Selectable;
    state |= Focusable;

    if (m_view->controller()->selectionBehavior() == KItemListController::MultiSelection) {
        state |= MultiSelectable;
    }

    if (m_view->model()->isExpandable(m_index)) {
        if (m_view->model()->isExpanded(m_index)) {
            state |= Expanded;
        } else {
            state |= Collapsed;
        }
    }

    return state;
}

bool KItemListAccessibleCell::isExpandable() const
{
    return m_view->model()->isExpandable(m_index);
}

QRect KItemListAccessibleCell::rect(int) const
{
    QRect rect = m_view->itemRect(m_index).toRect();

    if (rect.isNull()) {
        return QRect();
    }

    rect.translate(m_view->mapToScene(QPointF(0.0, 0.0)).toPoint());
    rect.translate(m_view->scene()->views()[0]->mapToGlobal(QPoint(0, 0)));
    return rect;
}

QString KItemListAccessibleCell::text(QAccessible::Text t, int child) const
{
    Q_ASSERT(child == 0);
    Q_UNUSED(child)

    switch (t) {
    case QAccessible::Value:
    case QAccessible::Name: {
        const QHash<QByteArray, QVariant> data = m_view->model()->data(m_index);
        return data["text"].toString();
    }

    default:
        break;
    }

    return QString();
}

void KItemListAccessibleCell::setText(QAccessible::Text, int child, const QString&)
{
    Q_ASSERT(child == 0);
}

bool KItemListAccessibleCell::isValid() const
{
    return m_view && (m_index >= 0) && (m_index < m_view->model()->count());
}

int KItemListAccessibleCell::childAt(int, int) const
{
    return 0;
}

int KItemListAccessibleCell::childCount() const
{
    return 0;
}

int KItemListAccessibleCell::indexOfChild(const QAccessibleInterface* child) const
{
    Q_UNUSED(child);
    return -1;
}

int KItemListAccessibleCell::navigate(RelationFlag relation, int index, QAccessibleInterface** interface) const
{
    if (relation == Ancestor && index == 1) {
        *interface = new KItemListViewAccessible(m_view);
        return 0;
    }

    *interface = 0;
    return -1;
}

QAccessible::Relation KItemListAccessibleCell::relationTo(int child, const QAccessibleInterface* , int otherChild) const
{
    Q_ASSERT(child == 0);
    Q_ASSERT(otherChild == 0);
    return QAccessible::Unrelated;
}

#ifndef QT_NO_ACTION

int KItemListAccessibleCell::userActionCount(int) const
{
    return 0;
}

QString KItemListAccessibleCell::actionText(int, Text, int) const
{
    return QString();
}

bool KItemListAccessibleCell::doAction(int, int, const QVariantList&)
{
    return false;
}

#endif

int KItemListAccessibleCell::index() const
{
    return m_index;
}

QObject* KItemListAccessibleCell::object() const
{
    return 0;
}

// Container Interface
KItemListContainerAccessible::KItemListContainerAccessible(KItemListContainer* container) :
    QAccessibleWidgetEx(container)
{
}

KItemListContainerAccessible::~KItemListContainerAccessible()
{
}

int KItemListContainerAccessible::childCount() const
{
    return 1;
}

int KItemListContainerAccessible::indexOfChild(const QAccessibleInterface* child) const
{
    if (child->object() == container()->controller()->view()) {
        return 1;
    } else {
        return -1;
    }
}

int KItemListContainerAccessible::navigate(QAccessible::RelationFlag relation, int index, QAccessibleInterface** target) const
{
    if (relation == QAccessible::Child) {
        *target = new KItemListViewAccessible(container()->controller()->view());
        return 0;
    } else {
        return QAccessibleWidgetEx::navigate(relation, index, target);
    }
}

const KItemListContainer* KItemListContainerAccessible::container() const
{
    return qobject_cast<KItemListContainer*>(object());
}

#endif // QT_NO_ACCESSIBILITY
