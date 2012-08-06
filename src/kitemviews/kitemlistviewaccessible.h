#ifndef ACCESSIBLE_ITEMVIEWS_H
#define ACCESSIBLE_ITEMVIEWS_H

#include "QtCore/qpointer.h"
#include <QtGui/qabstractitemview.h>
#include <QtGui/qheaderview.h>
#include <QtGui/qaccessible.h>
#include <QtGui/qaccessible2.h>
#include <QtGui/qaccessiblewidget.h>

#include "kitemlistview.h"
#include "kitemlistcontainer.h"

#ifndef QT_NO_ACCESSIBILITY

#ifndef QT_NO_ITEMVIEWS

class KItemListWidgetAccessible;

class KItemListViewAccessible: public QAccessibleTable2Interface, public QAccessibleObjectEx
{
    Q_ACCESSIBLE_OBJECT
public:
    explicit KItemListViewAccessible(KItemListView *view);

    virtual ~KItemListViewAccessible();

    Role role(int child) const;
    State state(int child) const;
    QString text(Text t, int child) const;
    QRect rect(int child) const;

    int childAt(int x, int y) const;
    int childCount() const;
    int indexOfChild(const QAccessibleInterface *) const;

    int navigate(RelationFlag relation, int index, QAccessibleInterface **iface) const;
    Relation relationTo(int child, const QAccessibleInterface *other, int otherChild) const;

#ifndef QT_NO_ACTION
    int userActionCount(int child) const;
    QString actionText(int action, Text t, int child) const;
    bool doAction(int action, int child, const QVariantList &params);
#endif
    QVariant invokeMethodEx(Method, int, const QVariantList &) { return QVariant(); }

    // table2 interface
    virtual QAccessibleTable2CellInterface *cellAt(int row, int column) const;
    virtual QAccessibleInterface *caption() const;
    virtual QAccessibleInterface *summary() const;
    virtual QString columnDescription(int column) const;
    virtual QString rowDescription(int row) const;
    virtual int columnCount() const;
    virtual int rowCount() const;
    virtual QAccessible2::TableModelChange modelChange() const;

    //Table
    virtual void rowsInserted(const QModelIndex&, int, int) {}
    virtual void rowsRemoved(const QModelIndex&, int, int) {}
    virtual void columnsInserted(const QModelIndex&, int, int) {}
    virtual void columnsRemoved(const QModelIndex&, int, int) {}
    virtual void rowsMoved(const QModelIndex&, int, int, const QModelIndex&, int) {}
    virtual void columnsMoved(const QModelIndex&, int, int, const QModelIndex&, int) {}

    // selection
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

    KItemListView *view() const;

protected:
    virtual void modelReset();

protected:
    inline QAccessibleTable2CellInterface *cell(int index) const;
    inline QAccessible::Role cellRole() const {
    /*
        switch (m_role) {
        case QAccessible::List:
            return QAccessible::ListItem;
        case QAccessible::Table:
            return QAccessible::Cell;
        case QAccessible::Tree:
            return QAccessible::TreeItem;
        default:
            Q_ASSERT(0);
        }
        return QAccessible::NoRole;
    */
    return QAccessible::Cell;
    }

private:
    //QAccessible::Role m_role;
    // the child index for a model index
    //inline int logicalIndex(const QModelIndex &index) const;
    // the model index from the child index
    //QAccessibleInterface *childFromLogical(int logicalIndex) const;
};

class KItemListWidgetAccessible: public QAccessibleTable2CellInterface
{
public:
    KItemListWidgetAccessible(KItemListView *view, int m_index);

    QObject *object() const { return 0; }
    Role role(int child) const;
    State state(int child) const;
    QRect rect(int child) const;
    bool isValid() const;

    int childAt(int, int) const { return 0; }
    int childCount() const { return 0; }
    int indexOfChild(const QAccessibleInterface *) const  { return -1; }

    QString text(Text t, int child) const;
    void setText(Text t, int child, const QString &text);

    int navigate(RelationFlag relation, int m_index, QAccessibleInterface **iface) const;
    Relation relationTo(int child, const QAccessibleInterface *other, int otherChild) const;

    bool isExpandable() const;

#ifndef QT_NO_ACTION
    int userActionCount(int child) const;
    QString actionText(int action, Text t, int child) const;
    bool doAction(int action, int child, const QVariantList &params);
#endif

    // cell interface
    virtual int columnExtent() const;
    virtual QList<QAccessibleInterface*> columnHeaderCells() const;
    virtual int columnIndex() const;
    virtual int rowExtent() const;
    virtual QList<QAccessibleInterface*> rowHeaderCells() const;
    virtual int rowIndex() const;
    virtual bool isSelected() const;
    virtual void rowColumnExtents(int *row, int *column, int *rowExtents, int *columnExtents, bool *selected) const;
    virtual QAccessibleTable2Interface* table() const;

    inline int getIndex() const
    { return index; }

private:
    QPointer<KItemListView > view;
    int index;
    KItemListWidget *widget;

friend class KItemListViewAccessible;
//friend class QAccessibleTree;
};

class KItemListContainerAccessible : public QAccessibleWidgetEx
{
    Q_ACCESSIBLE_OBJECT
public:
    explicit KItemListContainerAccessible(KItemListContainer*);
    virtual  ~KItemListContainerAccessible ();
    int  childCount () const ;
    int  indexOfChild ( const QAccessibleInterface * child ) const ;
    bool isValid () const ;
    int  navigate ( RelationFlag relation, int entry, QAccessibleInterface ** target ) const ;
    QObject * object () const ;
    QRect rect ( int child ) const ;
    QAccessible::Relation relationTo ( int child, const QAccessibleInterface * other, int otherChild ) const ;
    QAccessible::Role role ( int child ) const ;
    QAccessible::State state ( int child ) const ;
private:
    KItemListContainer *m_container ;
};

#endif // QT_NO_ITEMVIEWS

#endif // QT_NO_ACCESSIBILITY

#endif // ACCESSIBLE_ITEMVIEWS_H
