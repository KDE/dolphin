#include "kitemlistviewaccessible.h"
#include "kitemlistcontroller.h"
#include "kitemlistselectionmanager.h"
#include "private/kitemlistviewlayouter.h"

#include <QtGui/qtableview.h>
#include <QtGui/qaccessible2.h>
#include <qgraphicsscene.h>
#include <qgraphicsview.h>

#include <KDebug>
#include <QHash>

#ifndef QT_NO_ACCESSIBILITY

#ifndef QT_NO_ITEMVIEWS

KItemListView *KItemListViewAccessible::view() const
{
    return qobject_cast<KItemListView*>(object());
}

KItemListViewAccessible::KItemListViewAccessible(KItemListView *view_)
    : QAccessibleObjectEx(view_)
{
    Q_ASSERT(view());

    /*if (qobject_cast<const QTableView*>(view())) {
        m_role = QAccessible::Table;
    } else if (qobject_cast<const QTreeView*>(view())) {
        m_role = QAccessible::Tree;
    } else if (qobject_cast<const QListView*>(view())) {
        m_role = QAccessible::List;
    } else {
        // is this our best guess?
        m_role = QAccessible::Table;
    }*/
}

KItemListViewAccessible::~KItemListViewAccessible()
{
}

void KItemListViewAccessible::modelReset()
{}

QAccessibleTable2CellInterface *KItemListViewAccessible::cell(int index) const
{
    if (index > 0)
        return new KItemListAccessibleCell(view(), index);
    return 0;
}

QAccessibleTable2CellInterface *KItemListViewAccessible::cellAt(int row, int column) const
{
    /*Q_ASSERT(role(0) != QAccessible::Tree);
    QModelIndex index = view()->model()->index(row, column);
    //Q_ASSERT(index.isValid());
    if (!index.isValid()) {
        qWarning() << "QAccessibleTable2::cellAt: invalid index: " << index << " for " << view();
        return 0;
    }
    return cell(index);*/
    Q_UNUSED(row)
    Q_UNUSED(column)
    return cell(1);

}

QAccessibleInterface *KItemListViewAccessible::caption() const
{
    return 0;
}

QString KItemListViewAccessible::columnDescription(int) const
{
    // FIXME: no i18n
    return QObject::tr("No Column Description");
}

int KItemListViewAccessible::columnCount() const
{
    return view()->layouter()->columnCount();
}

int KItemListViewAccessible::rowCount() const
{
    int itemCount = view()->model()->count();
    int rowCount = itemCount / columnCount();
    if (itemCount % rowCount)
        ++rowCount;
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
    return "No Row Description";
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
    QList<int> columns;
    /*Q_FOREACH (const QModelIndex &index, view()->selectionModel()->selectedColumns()) {
        columns.append(index.column());
    }*/
    return columns;
}

QList<int> KItemListViewAccessible::selectedRows() const
{
    QList<int> rows;
    /*Q_FOREACH (const QModelIndex &index, view()->selectionModel()->selectedRows()) {
        rows.append(index.row());
    }*/
    return rows;
}

QAccessibleInterface *KItemListViewAccessible::summary() const
{
    return 0;
}

bool KItemListViewAccessible::isColumnSelected(int) const
{
    return false; //view()->selectionModel()->isColumnSelected(column, QModelIndex());
}

bool KItemListViewAccessible::isRowSelected(int) const
{
    return false; //view()->selectionModel()->isRowSelected(row, QModelIndex());
}

bool KItemListViewAccessible::selectRow(int)
{
    /*QModelIndex index = view()->model()->index(row, 0);
    if (!index.isValid() || view()->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view()->selectionModel()->select(index, QItemSelectionModel::Select);*/
    return true;
}

bool KItemListViewAccessible::selectColumn(int)
{
    /*QModelIndex index = view()->model()->index(0, column);
    if (!index.isValid() || view()->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view()->selectionModel()->select(index, QItemSelectionModel::Select);*/
    return true;
}

bool KItemListViewAccessible::unselectRow(int)
{
    /*QModelIndex index = view()->model()->index(row, 0);
    if (!index.isValid() || view()->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view()->selectionModel()->select(index, QItemSelectionModel::Deselect);*/
    return true;
}

bool KItemListViewAccessible::unselectColumn(int)
{
    /*QModelIndex index = view()->model()->index(0, column);
    if (!index.isValid() || view()->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view()->selectionModel()->select(index, QItemSelectionModel::Columns & QItemSelectionModel::Deselect);*/
    return true;
}

QAccessible2::TableModelChange KItemListViewAccessible::modelChange() const
{
    QAccessible2::TableModelChange change;
    // FIXME
    return change;
}

QAccessible::Role KItemListViewAccessible::role(int child) const
{
    Q_ASSERT(child >= 0);
    if (child > 0)
        return QAccessible::Cell;
    return QAccessible::Table;
}

QAccessible::State KItemListViewAccessible::state(int child) const
{
    Q_ASSERT(child == 0);
    return QAccessible::Normal;
}

int KItemListViewAccessible::childAt(int x, int y) const
{
    QPointF point = QPointF(x,y);
    return view()->itemAt(view()->mapFromScene(point));
}

int KItemListViewAccessible::childCount() const
{
    return rowCount() * columnCount();
}

int KItemListViewAccessible::indexOfChild(const QAccessibleInterface *iface) const
{
    /*Q_ASSERT(iface->role(0) != QAccessible::TreeItem); // should be handled by tree class
    if (iface->role(0) == QAccessible::Cell || iface->role(0) == QAccessible::ListItem) {
        const QAccessibleTable2Cell* cell = static_cast<const QAccessibleTable2Cell*>(iface);
        return logicalIndex(cell->m_index);
    } else if (iface->role(0) == QAccessible::ColumnHeader){
        const QAccessibleTable2HeaderCell* cell = static_cast<const QAccessibleTable2HeaderCell*>(iface);
        return cell->index + (verticalHeader() ? 1 : 0) + 1;
    } else if (iface->role(0) == QAccessible::RowHeader){
        const QAccessibleTable2HeaderCell* cell = static_cast<const QAccessibleTable2HeaderCell*>(iface);
        return (cell->index+1) * (view()->model()->rowCount()+1)  + 1;
    } else if (iface->role(0) == QAccessible::Pane) {
        return 1; // corner button
    } else {
        qWarning() << "WARNING QAccessibleTable2::indexOfChild Fix my children..."
                   << iface->role(0) << iface->text(QAccessible::Name, 0);
    }
    // FIXME: we are in denial of our children. this should stop.
    return -1;*/
    
    const KItemListAccessibleCell *widget = static_cast<const KItemListAccessibleCell*>(iface);
    return widget->getIndex();
}

QString KItemListViewAccessible::text(Text t, int child) const
{
    Q_ASSERT(child == 0);
    // FIXME: I don't think this is needed, but if at all it needs i18n
    if (t == QAccessible::Description)
        return QObject::tr("List of files present in the current directory");
    return QObject::tr("File List");
}

QRect KItemListViewAccessible::rect(int child) const
{
    Q_UNUSED(child)
    if (!view()->isVisible())
        return QRect();
    QPoint origin = view()->scene()->views()[0]->mapToGlobal(QPoint(0, 0));
    QRect viewRect = view()->geometry().toRect();
    return viewRect.translated(origin);
}

int KItemListViewAccessible::navigate(RelationFlag relation, int index, QAccessibleInterface **iface) const
{
    *iface = 0;
    switch (relation) {
    /*case Ancestor: {
        if (index == 1 && view()->parent()) {
            *iface = QAccessible::queryAccessibleInterface(view()->parent());
            if (*iface)
                return 0;
        }
        break;
    }*/
    case QAccessible::Child: {
        Q_ASSERT(index > 0);
        *iface = cell(index);
        if (*iface) {
            return 0;
        }
        break;
    }
    default:
        break;
    }
    return -1;
}

QAccessible::Relation KItemListViewAccessible::relationTo(int, const QAccessibleInterface *, int) const
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
bool KItemListViewAccessible::doAction(int, int, const QVariantList &)
{
    return false;
}
#endif

// TABLE CELL

KItemListAccessibleCell::KItemListAccessibleCell(KItemListView *view_, int index_)
    : /* QAccessibleSimpleEditableTextInterface(this), */ view(view_)
    , index(index_)
{
    Q_ASSERT(index_ > 0);
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
    QList<QAccessibleInterface*> headerCell;
    /*if (verticalHeader()) {
        headerCell.append(new QAccessibleTable2HeaderCell(view, m_index.row(), Qt::Vertical));
    }*/
    return headerCell;
}

QList<QAccessibleInterface*> KItemListAccessibleCell::columnHeaderCells() const
{
    QList<QAccessibleInterface*> headerCell;
    /*if (horizontalHeader()) {
        headerCell.append(new QAccessibleTable2HeaderCell(view, m_index.column(), Qt::Horizontal));
    }*/
    return headerCell;
}

int KItemListAccessibleCell::columnIndex() const
{
    return view->layouter()->itemColumn(index);
}

int KItemListAccessibleCell::rowIndex() const
{
    /*if (role(0) == QAccessible::TreeItem) {
       const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
       Q_ASSERT(treeView);
       int row = treeView->d_func()->viewIndex(m_index);
       return row;
    }*/
    return view->layouter()->itemRow(index);
}

//Done
bool KItemListAccessibleCell::isSelected() const
{
    //return widget->isSelected();
    // FIXME
    return false;
}

void KItemListAccessibleCell::rowColumnExtents(int *row, int *column, int *rowExtents, int *columnExtents, bool *selected) const
{
    KItemListViewLayouter* layouter = view->layouter();
    *row = layouter->itemRow(index);
    *column = layouter->itemColumn(index);
    *rowExtents = 1;
    *columnExtents = 1;
    *selected = isSelected();
}

QAccessibleTable2Interface* KItemListAccessibleCell::table() const
{
    return QAccessible::queryAccessibleInterface(view)->table2Interface();
}

QAccessible::Role KItemListAccessibleCell::role(int child) const
{
    Q_ASSERT(child == 0);
    return QAccessible::Cell;
}

QAccessible::State KItemListAccessibleCell::state(int child) const
{
    Q_ASSERT(child == 0);
    QAccessible::State st = Normal;

    //QRect globalRect = view->rect();
    //globalRect.translate(view->mapToGlobal(QPoint(0,0)));
    //if (!globalRect.intersects(rect(0)))
    //    st |= Invisible;

//     if (widget->isSelected())
//         st |= Selected;
    if (view->controller()->selectionManager()->currentItem() == index)
        st |= Focused;

    //if (m_index.model()->data(m_index, Qt::CheckStateRole).toInt() == Qt::Checked)
    //    st |= Checked;
    //if (flags & Qt::ItemIsSelectable) {
    st |= Selectable;
    st |= Focusable;
    if (view->controller()->selectionBehavior() == KItemListController::MultiSelection)
        st |= MultiSelectable;

        //if (view->selectionMode() == QAbstractItemView::ExtendedSelection)
            //st |= ExtSelectable;
    //}
    //if (m_role == QAccessible::TreeItem) {
    //    const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
    //    if (treeView->isExpanded(m_index))
    //        st |= Expanded;
    //}

    return st;
}

//Done
bool KItemListAccessibleCell::isExpandable() const
{
    return false; //view->model()->hasChildren(m_index);
}

//Done
QRect KItemListAccessibleCell::rect(int) const
{
    QRect r = view->itemRect(index).toRect();
    if (r.isNull())
        return QRect();
    r.translate(view->mapToScene(QPointF(0.0, 0.0)).toPoint());
    r.translate(view->scene()->views()[0]->mapToGlobal(QPoint(0, 0)));
    return r;
}

QString KItemListAccessibleCell::text(QAccessible::Text t, int child) const
{
    Q_ASSERT(child == 0);
    QHash<QByteArray, QVariant> data = view->model()->data(index);
    switch (t) {
    case QAccessible::Value:
    case QAccessible::Name:
        return data["text"].toString();
    case QAccessible::Description:
        return data["text"].toString() + " : " + data["group"].toString();
    default:
        break;
    }
    return QString();
}

void KItemListAccessibleCell::setText(QAccessible::Text /*t*/, int child, const QString &text)
{
    Q_ASSERT(child == 0);
    // FIXME - is this even allowed on the KItemListWidget?
}

bool KItemListAccessibleCell::isValid() const
{
    return view && (index > 0);
}

int KItemListAccessibleCell::navigate(RelationFlag relation, int index, QAccessibleInterface **iface) const
{
    if (relation == Ancestor && index == 1) {
        //if (m_role == QAccessible::TreeItem) {
        //    *iface = new QAccessibleTree(view);
        //} else {
        *iface = new KItemListViewAccessible(view);
        return 0;
    }

    *iface = 0;
    if (!view)
        return -1;

    switch (relation) {

    case Child: {
        return -1;
    }
    case Sibling:
        if (index > 0) {
            QAccessibleInterface *parent = queryAccessibleInterface(view);
            int ret = parent->navigate(QAccessible::Child, index, iface);
            delete parent;
            if (*iface)
                return ret;
        }
        return -1;
    default:
        break;
    }

    return -1;
}

QAccessible::Relation KItemListAccessibleCell::relationTo(int child, const QAccessibleInterface *, int otherChild) const
{
    Q_ASSERT(child == 0);
    Q_ASSERT(otherChild == 0);
    /* we only check for parent-child relationships in trees
    if (m_role == QAccessible::TreeItem && other->role(0) == QAccessible::TreeItem) {
        QModelIndex otherIndex = static_cast<const QAccessibleTable2Cell*>(other)->m_index;
        // is the other our parent?
        if (otherIndex.parent() == m_index)
            return QAccessible::Ancestor;
        // are we the other's child?
        if (m_index.parent() == otherIndex)
            return QAccessible::Child;
    }*/
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

bool KItemListAccessibleCell::doAction(int, int, const QVariantList &)
{
    return false;
}

#endif

KItemListContainerAccessible::KItemListContainerAccessible(KItemListContainer *container)
    : QAccessibleWidgetEx(container)
{}

KItemListContainerAccessible::~KItemListContainerAccessible ()
{}

int KItemListContainerAccessible::childCount () const
{
    return 1;
}

int KItemListContainerAccessible::indexOfChild ( const QAccessibleInterface * child ) const
{
    if(child->object() == container()->controller()->view())
        return 1;
    return -1;
}

int KItemListContainerAccessible::navigate ( QAccessible::RelationFlag relation, int index, QAccessibleInterface ** target ) const
{
    if (relation == QAccessible::Child) {
        *target = new KItemListViewAccessible(container()->controller()->view());
        return 0;
    }
    return QAccessibleWidgetEx::navigate(relation, index, target);
}

#endif // QT_NO_ITEMVIEWS

#endif // QT_NO_ACCESSIBILITY
