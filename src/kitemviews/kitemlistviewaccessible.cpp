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
}

KItemListViewAccessible::~KItemListViewAccessible()
{
}

void KItemListViewAccessible::modelReset()
{}

QAccessibleTable2CellInterface *KItemListViewAccessible::cell(int index) const
{
    if (index > 0) {
        return new KItemListAccessibleCell(view(), index);
    }
    return 0;
}

QAccessibleTable2CellInterface *KItemListViewAccessible::cellAt(int row, int column) const
{
    return cell(column * (row - 1) + column) ;
}

QAccessibleInterface *KItemListViewAccessible::caption() const
{
    return 0;
}

QString KItemListViewAccessible::columnDescription(int) const
{
    return "";
}

int KItemListViewAccessible::columnCount() const
{
    return view()->layouter()->columnCount();
}

int KItemListViewAccessible::rowCount() const
{
    int itemCount = view()->model()->count();
    int rowCount = itemCount / columnCount();
    if (itemCount % rowCount) {
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
    return "";
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

QAccessibleInterface *KItemListViewAccessible::summary() const
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

bool KItemListViewAccessible::selectRow(int row)
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
    // FIXME
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
        QAccessibleInterface *iface;
        navigate(Child,child,&iface);
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
    return rowCount() * columnCount();
}

int KItemListViewAccessible::indexOfChild(const QAccessibleInterface *iface) const
{
    const KItemListAccessibleCell *widget = static_cast<const KItemListAccessibleCell*>(iface);
    return widget->getIndex();
}

QString KItemListViewAccessible::text(Text t, int child) const
{
    Q_ASSERT(child == 0);
    if (t == QAccessible::Description) {
        return QObject::tr("List of files present in the current directory");
    }
    return QObject::tr("File List");
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

int KItemListViewAccessible::navigate(RelationFlag relation, int index, QAccessibleInterface **iface) const
{
    *iface = 0;
    switch (relation) {
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
    : view(view_)
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
    return QList<QAccessibleInterface*>();
}

QList<QAccessibleInterface*> KItemListAccessibleCell::columnHeaderCells() const
{
    return QList<QAccessibleInterface*>();
}

int KItemListAccessibleCell::columnIndex() const
{
    return view->layouter()->itemColumn(index);
}

int KItemListAccessibleCell::rowIndex() const
{
    return view->layouter()->itemRow(index);
}

bool KItemListAccessibleCell::isSelected() const
{
    return view->controller()->selectionManager()->isSelected(index-1);
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

    if (isSelected()) {
         st |= Selected;
    }
    if (view->controller()->selectionManager()->currentItem() == index) {
        st |= Focused;
    }

    st |= Selectable;
    st |= Focusable;

    if (view->controller()->selectionBehavior() == KItemListController::MultiSelection){
        st |= MultiSelectable;
    }

    return st;
}

bool KItemListAccessibleCell::isExpandable() const
{
    return false;
}

QRect KItemListAccessibleCell::rect(int) const
{
    QRect r = view->itemRect(index-1).toRect();
    if (r.isNull()) {
        return QRect();
    }
    r.translate(view->mapToScene(QPointF(0.0, 0.0)).toPoint());
    r.translate(view->scene()->views()[0]->mapToGlobal(QPoint(0, 0)));
    return r;
}

QString KItemListAccessibleCell::text(QAccessible::Text t, int child) const
{
    Q_ASSERT(child == 0);
    Q_UNUSED(child)
    const QHash<QByteArray, QVariant> data = view->model()->data(index-1);
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
    // FIXME - is this even allowed on the KItemListWidget?
}

bool KItemListAccessibleCell::isValid() const
{
    return view && (index > 0);
}

int KItemListAccessibleCell::navigate(RelationFlag relation, int index, QAccessibleInterface **iface) const
{
    if (relation == Ancestor && index == 1) {
        *iface = new KItemListViewAccessible(view);
        return 0;
    }

    *iface = 0;
    if (!view) {
        return -1;
    }

    switch (relation) {

    case Child: {
        return -1;
    }
    case Sibling:
        if (index > 0) {
            QAccessibleInterface *parent = queryAccessibleInterface(view);
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

QAccessible::Relation KItemListAccessibleCell::relationTo(int child, const QAccessibleInterface *, int otherChild) const
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
    if (child->object() == container()->controller()->view()) {
        return 1;
    }
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
