/*
 * SPDX-FileCopyrightText: 2012 Amandeep Singh <aman.dedman@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef QT_NO_ACCESSIBILITY
#include "kitemlistviewaccessible.h"

#include "kitemlistcontainer.h"
#include "kitemlistcontroller.h"
#include "kitemlistselectionmanager.h"
#include "kitemlistview.h"
#include "private/kitemlistviewlayouter.h"

#include <QGraphicsScene>
#include <QGraphicsView>

KItemListView *KItemListViewAccessible::view() const
{
    return qobject_cast<KItemListView *>(object());
}

KItemListViewAccessible::KItemListViewAccessible(KItemListView *view_, KItemListContainerAccessible *parent)
    : QAccessibleObject(view_)
    , m_parent(parent)
{
    Q_ASSERT(view());
    Q_CHECK_PTR(parent);
    m_cells.resize(childCount());
}

KItemListViewAccessible::~KItemListViewAccessible()
{
    for (AccessibleIdWrapper idWrapper : std::as_const(m_cells)) {
        if (idWrapper.isValid) {
            QAccessible::deleteAccessibleInterface(idWrapper.id);
        }
    }
}

void *KItemListViewAccessible::interface_cast(QAccessible::InterfaceType type)
{
    if (type == QAccessible::TableInterface) {
        return static_cast<QAccessibleTableInterface *>(this);
    }
    return nullptr;
}

void KItemListViewAccessible::modelReset()
{
}

QAccessibleInterface *KItemListViewAccessible::cell(int index) const
{
    if (index < 0 || index >= view()->model()->count()) {
        return nullptr;
    }

    if (m_cells.size() <= index) {
        m_cells.resize(childCount());
    }
    Q_ASSERT(index < m_cells.size());

    AccessibleIdWrapper idWrapper = m_cells.at(index);
    if (!idWrapper.isValid) {
        idWrapper.id = QAccessible::registerAccessibleInterface(new KItemListAccessibleCell(view(), index));
        idWrapper.isValid = true;
        m_cells.insert(index, idWrapper);
    }
    return QAccessible::accessibleInterface(idWrapper.id);
}

QAccessibleInterface *KItemListViewAccessible::cellAt(int row, int column) const
{
    return cell(columnCount() * row + column);
}

QAccessibleInterface *KItemListViewAccessible::caption() const
{
    return nullptr;
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
    return view()->controller()->selectionManager()->selectedItems().count();
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

QList<QAccessibleInterface *> KItemListViewAccessible::selectedCells() const
{
    QList<QAccessibleInterface *> cells;
    const auto items = view()->controller()->selectionManager()->selectedItems();
    cells.reserve(items.count());
    for (int index : items) {
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

QAccessibleInterface *KItemListViewAccessible::summary() const
{
    return nullptr;
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

void KItemListViewAccessible::modelChange(QAccessibleTableModelChangeEvent * /*event*/)
{
}

QAccessible::Role KItemListViewAccessible::role() const
{
    return QAccessible::Table;
}

QAccessible::State KItemListViewAccessible::state() const
{
    QAccessible::State s;
    return s;
}

QAccessibleInterface *KItemListViewAccessible::childAt(int x, int y) const
{
    const QPointF point = QPointF(x, y);
    const std::optional<int> itemIndex = view()->itemAt(view()->mapFromScene(point));
    return child(itemIndex.value_or(-1));
}

QAccessibleInterface *KItemListViewAccessible::parent() const
{
    return m_parent;
}

int KItemListViewAccessible::childCount() const
{
    return view()->model()->count();
}

int KItemListViewAccessible::indexOfChild(const QAccessibleInterface *interface) const
{
    const KItemListAccessibleCell *widget = static_cast<const KItemListAccessibleCell *>(interface);
    return widget->index();
}

QString KItemListViewAccessible::text(QAccessible::Text) const
{
    return QString();
}

QRect KItemListViewAccessible::rect() const
{
    if (!view()->isVisible()) {
        return QRect();
    }

    const QGraphicsScene *scene = view()->scene();
    if (scene) {
        const QPoint origin = scene->views().at(0)->mapToGlobal(QPoint(0, 0));
        const QRect viewRect = view()->geometry().toRect();
        return viewRect.translated(origin);
    } else {
        return QRect();
    }
}

QAccessibleInterface *KItemListViewAccessible::child(int index) const
{
    if (index >= 0 && index < childCount()) {
        return cell(index);
    }
    return nullptr;
}

KItemListViewAccessible::AccessibleIdWrapper::AccessibleIdWrapper()
    : isValid(false)
    , id(0)
{
}

// Table Cell

KItemListAccessibleCell::KItemListAccessibleCell(KItemListView *view, int index)
    : m_view(view)
    , m_index(index)
{
    Q_ASSERT(index >= 0 && index < view->model()->count());
}

void *KItemListAccessibleCell::interface_cast(QAccessible::InterfaceType type)
{
    if (type == QAccessible::TableCellInterface) {
        return static_cast<QAccessibleTableCellInterface *>(this);
    }
    return nullptr;
}

int KItemListAccessibleCell::columnExtent() const
{
    return 1;
}

int KItemListAccessibleCell::rowExtent() const
{
    return 1;
}

QList<QAccessibleInterface *> KItemListAccessibleCell::rowHeaderCells() const
{
    return QList<QAccessibleInterface *>();
}

QList<QAccessibleInterface *> KItemListAccessibleCell::columnHeaderCells() const
{
    return QList<QAccessibleInterface *>();
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

QAccessibleInterface *KItemListAccessibleCell::table() const
{
    return QAccessible::queryAccessibleInterface(m_view);
}

QAccessible::Role KItemListAccessibleCell::role() const
{
    return QAccessible::Cell;
}

QAccessible::State KItemListAccessibleCell::state() const
{
    QAccessible::State state;

    state.selectable = true;
    if (isSelected()) {
        state.selected = true;
    }

    state.focusable = true;
    if (m_view->controller()->selectionManager()->currentItem() == m_index) {
        state.focused = true;
    }

    if (m_view->controller()->selectionBehavior() == KItemListController::MultiSelection) {
        state.multiSelectable = true;
    }

    if (m_view->model()->isExpandable(m_index)) {
        if (m_view->model()->isExpanded(m_index)) {
            state.expanded = true;
        } else {
            state.collapsed = true;
        }
    }

    return state;
}

bool KItemListAccessibleCell::isExpandable() const
{
    return m_view->model()->isExpandable(m_index);
}

QRect KItemListAccessibleCell::rect() const
{
    QRect rect = m_view->itemRect(m_index).toRect();

    if (rect.isNull()) {
        return QRect();
    }

    rect.translate(m_view->mapToScene(QPointF(0.0, 0.0)).toPoint());
    rect.translate(m_view->scene()->views()[0]->mapToGlobal(QPoint(0, 0)));
    return rect;
}

QString KItemListAccessibleCell::text(QAccessible::Text t) const
{
    switch (t) {
    case QAccessible::Name: {
        const QHash<QByteArray, QVariant> data = m_view->model()->data(m_index);
        return data["text"].toString();
    }

    default:
        break;
    }

    return QString();
}

void KItemListAccessibleCell::setText(QAccessible::Text, const QString &)
{
}

QAccessibleInterface *KItemListAccessibleCell::child(int) const
{
    return nullptr;
}

bool KItemListAccessibleCell::isValid() const
{
    return m_view && (m_index >= 0) && (m_index < m_view->model()->count());
}

QAccessibleInterface *KItemListAccessibleCell::childAt(int, int) const
{
    return nullptr;
}

int KItemListAccessibleCell::childCount() const
{
    return 0;
}

int KItemListAccessibleCell::indexOfChild(const QAccessibleInterface *child) const
{
    Q_UNUSED(child)
    return -1;
}

QAccessibleInterface *KItemListAccessibleCell::parent() const
{
    return QAccessible::queryAccessibleInterface(m_view);
}

int KItemListAccessibleCell::index() const
{
    return m_index;
}

QObject *KItemListAccessibleCell::object() const
{
    return nullptr;
}

// Container Interface
KItemListContainerAccessible::KItemListContainerAccessible(KItemListContainer *container)
    : QAccessibleWidget(container)
{
}

KItemListContainerAccessible::~KItemListContainerAccessible()
{
}

int KItemListContainerAccessible::childCount() const
{
    return 1;
}

int KItemListContainerAccessible::indexOfChild(const QAccessibleInterface *child) const
{
    if (child->object() == container()->controller()->view()) {
        return 0;
    }
    return -1;
}

QAccessibleInterface *KItemListContainerAccessible::child(int index) const
{
    if (index == 0) {
        return QAccessible::queryAccessibleInterface(container()->controller()->view());
    }
    return nullptr;
}

const KItemListContainer *KItemListContainerAccessible::container() const
{
    return qobject_cast<KItemListContainer *>(object());
}

#endif // QT_NO_ACCESSIBILITY
