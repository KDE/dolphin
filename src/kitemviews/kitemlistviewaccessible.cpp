#include "kitemlistviewaccessible.h"
#include "kitemlistcontroller.h"
#include "kitemlistselectionmanager.h"
#include "private/kitemlistviewlayouter.h"

#include <QtGui/qaccessible2.h>
#include <qgraphicsscene.h>
#include <qgraphicsview.h>

#include <KDebug>
#include <QHash>

#ifndef QT_NO_ACCESSIBILITY

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
{}

QAccessible::Role KItemListViewAccessible::cellRole() const
{
    return QAccessible::Cell;
}

QAccessibleTable2CellInterface* KItemListViewAccessible::cell(int index) const
{
    Q_ASSERT(index >= 0 && index < view()->model()->count());
    if (index < 0 || index >= view()->model()->count())
        return 0;
    return new KItemListAccessibleCell(view(), index);
}

QVariant KItemListViewAccessible::invokeMethodEx(Method, int, const QVariantList &)
{
    return QVariant();
}

QAccessibleTable2CellInterface* KItemListViewAccessible::cellAt(int row, int column) const
{
    qDebug() << "cellAt: " << row << column << " is: " << column*row + column;
    return cell(columnCount()*row + column);
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
    return view()->layouter()->columnCount();
}

int KItemListViewAccessible::rowCount() const
{
    if(columnCount()<=0) {
        return 0;
    }
    int itemCount = view()->model()->count();
    int rowCount = itemCount / columnCount();
    if(rowCount <= 0){
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
    return change;
}

QAccessible::Role KItemListViewAccessible::role(int child) const
{
    Q_ASSERT(child >= 0);
    if (child > 0) {
        return QAccessible::Cell;
    }
    return QAccessible::Table;
}

QAccessible::State KItemListViewAccessible::state(int child) const
{
    if (child) {
        QAccessibleInterface* iface = 0;
        navigate(Child, child, &iface);
        if (iface) {
            return iface->state(0);
        }
    }
    return QAccessible::Normal | QAccessible::HasInvokeExtension;
}

int KItemListViewAccessible::childAt(int x, int y) const
{
    QPointF point = QPointF(x,y);
    return view()->itemAt(view()->mapFromScene(point));
}

int KItemListViewAccessible::childCount() const
{
    return view()->model()->count();
}

int KItemListViewAccessible::indexOfChild(const QAccessibleInterface* iface) const
{
    const KItemListAccessibleCell* widget = static_cast<const KItemListAccessibleCell*>(iface);
    return widget->index() + 1;
}

QString KItemListViewAccessible::text(Text , int child) const
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
    QPoint origin = view()->scene()->views()[0]->mapToGlobal(QPoint(0, 0));
    QRect viewRect = view()->geometry().toRect();
    return viewRect.translated(origin);
}

int KItemListViewAccessible::navigate(RelationFlag relation, int index, QAccessibleInterface* *iface) const
{
    *iface = 0;
    switch (relation) {
        case QAccessible::Child: {
            Q_ASSERT(index > 0);
            *iface = cell(index - 1);
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

QAccessible::Relation KItemListViewAccessible::relationTo(int, const QAccessibleInterface* , int) const
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
    return m_view->layouter()->itemColumn(m_index);
}

int KItemListAccessibleCell::rowIndex() const
{
    return m_view->layouter()->itemRow(m_index);
}

bool KItemListAccessibleCell::isSelected() const
{
    return m_view->controller()->selectionManager()->isSelected(m_index);
}

void KItemListAccessibleCell::rowColumnExtents(int* row, int* column, int* rowExtents, int* columnExtents, bool* selected) const
{
    KItemListViewLayouter* layouter = m_view->layouter();
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

    if (m_view->controller()->selectionBehavior() == KItemListController::MultiSelection){
        state |= MultiSelectable;
    }

    return state;
}

bool KItemListAccessibleCell::isExpandable() const
{
    return false;
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
    const QHash<QByteArray, QVariant> data = m_view->model()->data(m_index);
    switch (t) {
    case QAccessible::Value:
    case QAccessible::Name:
        return data["text"].toString();
    default:
        break;
    }
    return QString();
}

void KItemListAccessibleCell::setText(QAccessible::Text /*t*/, int child, const QString &/*text*/)
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
    return -1;
}

int KItemListAccessibleCell::navigate(RelationFlag relation, int index, QAccessibleInterface* *iface) const
{
    if (relation == Ancestor && index == 1) {
        *iface = new KItemListViewAccessible(m_view);
        return 0;
    }

    *iface = 0;
    if (!m_view) {
        return -1;
    }

    switch (relation) {

    case Child: {
        return -1;
    }
    case Sibling:
        if (index > 0) {
            QAccessibleInterface* parent = queryAccessibleInterface(m_view);
            int ret = parent->navigate(QAccessible::Child, index, iface);
            delete parent;
            if (*iface) {
                return ret;
            }
        }
        return -1;
    default:
        break;
    }

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

bool KItemListAccessibleCell::doAction(int, int, const QVariantList &)
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
{}

KItemListContainerAccessible::~KItemListContainerAccessible ()
{}

int KItemListContainerAccessible::childCount () const
{
    return 1;
}

int KItemListContainerAccessible::indexOfChild ( const QAccessibleInterface*  child ) const
{
    if (child->object() == container()->controller()->view()) {
        return 1;
    }
    return -1;
}

int KItemListContainerAccessible::navigate ( QAccessible::RelationFlag relation, int index, QAccessibleInterface** target ) const
{
    if (relation == QAccessible::Child) {
        *target = new KItemListViewAccessible(container()->controller()->view());
        return 0;
    }
    return QAccessibleWidgetEx::navigate(relation, index, target);
}

#endif // QT_NO_ACCESSIBILITY
