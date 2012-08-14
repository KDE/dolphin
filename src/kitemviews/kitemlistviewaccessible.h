#ifndef ACCESSIBLE_ITEMVIEWS_H
#define ACCESSIBLE_ITEMVIEWS_H

#include "QtCore/qpointer.h"
#include <QtGui/qaccessible.h>
#include <QtGui/qaccessible2.h>
#include <QtGui/qaccessiblewidget.h>
#include <QtGui/qaccessibleobject.h>

#include "kitemlistview.h"
#include "kitemlistcontainer.h"

#ifndef QT_NO_ACCESSIBILITY

class KItemListAccessibleCell;

class KItemListViewAccessible: public QAccessibleTable2Interface, public QAccessibleObjectEx
{
    Q_ACCESSIBLE_OBJECT
public:
    explicit KItemListViewAccessible(KItemListView* view);

    virtual ~KItemListViewAccessible();

    Role role(int child) const;
    State state(int child) const;
    QString text(Text t, int child) const;
    QRect rect(int child) const;

    int childAt(int x, int y) const;
    int childCount() const;
    int indexOfChild(const QAccessibleInterface* ) const;

    int navigate(RelationFlag relation, int index, QAccessibleInterface** iface) const;
    Relation relationTo(int child, const QAccessibleInterface* other, int otherChild) const;

#ifndef QT_NO_ACTION
    int userActionCount(int child) const;
    QString actionText(int action, Text t, int child) const;
    bool doAction(int action, int child, const QVariantList &params);
#endif
    QVariant invokeMethodEx(Method, int, const QVariantList &);

    // Table2 interface
    virtual QAccessibleTable2CellInterface* cellAt(int row, int column) const;
    virtual QAccessibleInterface* caption() const;
    virtual QAccessibleInterface* summary() const;
    virtual QString columnDescription(int column) const;
    virtual QString rowDescription(int row) const;
    virtual int columnCount() const;
    virtual int rowCount() const;
    virtual QAccessible2::TableModelChange modelChange() const;
    virtual void rowsInserted(const QModelIndex&, int, int) {}
    virtual void rowsRemoved(const QModelIndex&, int, int) {}
    virtual void columnsInserted(const QModelIndex&, int, int) {}
    virtual void columnsRemoved(const QModelIndex&, int, int) {}
    virtual void rowsMoved(const QModelIndex&, int, int, const QModelIndex&, int) {}
    virtual void columnsMoved(const QModelIndex&, int, int, const QModelIndex&, int) {}

    // Selection
    virtual int selectedCellCount() const;
    virtual int selectedColumnCount() const;
    virtual int selectedRowCount() const;
    virtual QList<QAccessibleTable2CellInterface*> selectedCells() const;
    virtual QList<int> selectedColumns() const;
    virtual QList<int> selectedRows() const;
    virtual bool isColumnSelected(int column) const;
    virtual bool isRowSelected(int row) const;
    virtual bool selectRow(int row);
    virtual bool selectColumn(int column);
    virtual bool unselectRow(int row);
    virtual bool unselectColumn(int column);

    KItemListView* view() const;

protected:
    virtual void modelReset();
    inline QAccessibleTable2CellInterface* cell(int index) const;
    inline QAccessible::Role cellRole() const;
};

class KItemListAccessibleCell: public QAccessibleTable2CellInterface
{
public:
    KItemListAccessibleCell(KItemListView* view, int m_index);

    QObject* object() const;
    Role role(int) const;
    State state(int) const;
    QRect rect(int) const;
    bool isValid() const;
    int childAt(int, int) const;
    int childCount() const;
    int indexOfChild(const QAccessibleInterface*) const;
    QString text(Text t, int child) const;
    void setText(Text t, int child, const QString& text);
    int navigate(RelationFlag relation, int m_index, QAccessibleInterface** iface) const;
    Relation relationTo(int child, const QAccessibleInterface* other, int otherChild) const;
    bool isExpandable() const;

#ifndef QT_NO_ACTION
    int userActionCount(int child) const;
    QString actionText(int action, Text t, int child) const;
    bool doAction(int action, int child, const QVariantList &params);
#endif

    // Cell Interface
    virtual int columnExtent() const;
    virtual QList<QAccessibleInterface* > columnHeaderCells() const;
    virtual int columnIndex() const;
    virtual int rowExtent() const;
    virtual QList<QAccessibleInterface* > rowHeaderCells() const;
    virtual int rowIndex() const;
    virtual bool isSelected() const;
    virtual void rowColumnExtents(int* row, int* column, int* rowExtents, int* columnExtents, bool* selected) const;
    virtual QAccessibleTable2Interface* table() const;

    inline int index() const;

private:
    QPointer<KItemListView> m_view;
    int m_index;

friend class KItemListViewAccessible;
};

class KItemListContainerAccessible : public QAccessibleWidgetEx
{
    Q_ACCESSIBLE_OBJECT
public:
    explicit KItemListContainerAccessible(KItemListContainer*);
    virtual ~KItemListContainerAccessible();
    int childCount () const;
    int indexOfChild ( const QAccessibleInterface* child ) const;
    int navigate ( RelationFlag relation, int entry, QAccessibleInterface** target ) const;

private:
    inline KItemListContainer* container() const
    {
        return static_cast<KItemListContainer* >(object());
    }
};

#endif // QT_NO_ACCESSIBILITY

#endif // ACCESSIBLE_ITEMVIEWS_H
