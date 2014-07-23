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

#ifndef KITEMLISTVIEWACCESSIBLE_H
#define KITEMLISTVIEWACCESSIBLE_H

#ifndef QT_NO_ACCESSIBILITY

#include <libdolphin_export.h>

#include <QtCore/qpointer.h>
#include <QtGui/qaccessible.h>
#include <QtGui/qaccessibleobject.h>
#include <QtWidgets/qaccessiblewidget.h>

class KItemListView;
class KItemListContainer;

class LIBDOLPHINPRIVATE_EXPORT KItemListViewAccessible: public QAccessibleObject, public QAccessibleTableInterface
{
public:
    explicit KItemListViewAccessible(KItemListView* view);
    ~KItemListViewAccessible();

    void* interface_cast(QAccessible::InterfaceType type) Q_DECL_OVERRIDE;

    QAccessible::Role role() const Q_DECL_OVERRIDE;
    QAccessible::State state() const Q_DECL_OVERRIDE;
    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;
    QRect rect() const Q_DECL_OVERRIDE;

    QAccessibleInterface* child(int index) const Q_DECL_OVERRIDE;
    int childCount() const Q_DECL_OVERRIDE;
    int indexOfChild(const QAccessibleInterface*) const Q_DECL_OVERRIDE;
    QAccessibleInterface* childAt(int x, int y) const Q_DECL_OVERRIDE;
    QAccessibleInterface* parent() const Q_DECL_OVERRIDE;

    // Table interface
    virtual QAccessibleInterface* cellAt(int row, int column) const Q_DECL_OVERRIDE;
    virtual QAccessibleInterface* caption() const Q_DECL_OVERRIDE;
    virtual QAccessibleInterface* summary() const Q_DECL_OVERRIDE;
    virtual QString columnDescription(int column) const Q_DECL_OVERRIDE;
    virtual QString rowDescription(int row) const Q_DECL_OVERRIDE;
    virtual int columnCount() const Q_DECL_OVERRIDE;
    virtual int rowCount() const Q_DECL_OVERRIDE;

    // Selection
    virtual int selectedCellCount() const Q_DECL_OVERRIDE;
    virtual int selectedColumnCount() const Q_DECL_OVERRIDE;
    virtual int selectedRowCount() const Q_DECL_OVERRIDE;
    virtual QList<QAccessibleInterface*> selectedCells() const Q_DECL_OVERRIDE;
    virtual QList<int> selectedColumns() const Q_DECL_OVERRIDE;
    virtual QList<int> selectedRows() const Q_DECL_OVERRIDE;
    virtual bool isColumnSelected(int column) const Q_DECL_OVERRIDE;
    virtual bool isRowSelected(int row) const Q_DECL_OVERRIDE;
    virtual bool selectRow(int row) Q_DECL_OVERRIDE;
    virtual bool selectColumn(int column) Q_DECL_OVERRIDE;
    virtual bool unselectRow(int row) Q_DECL_OVERRIDE;
    virtual bool unselectColumn(int column) Q_DECL_OVERRIDE;
    virtual void modelChange(QAccessibleTableModelChangeEvent*) Q_DECL_OVERRIDE;

    KItemListView* view() const;

protected:
    virtual void modelReset();
    /**
     * Create an QAccessibleTableCellInterface representing the table
     * cell at the @index. Index is 0-based.
     */
    inline QAccessibleInterface* cell(int index) const;

private:
    mutable QVector<QAccessibleInterface*> m_cells;
};

class LIBDOLPHINPRIVATE_EXPORT KItemListAccessibleCell: public QAccessibleInterface, public QAccessibleTableCellInterface
{
public:
    KItemListAccessibleCell(KItemListView* view, int m_index);

    void* interface_cast(QAccessible::InterfaceType type) Q_DECL_OVERRIDE;
    QObject* object() const Q_DECL_OVERRIDE;
    bool isValid() const Q_DECL_OVERRIDE;
    QAccessible::Role role() const Q_DECL_OVERRIDE;
    QAccessible::State state() const Q_DECL_OVERRIDE;
    QRect rect() const Q_DECL_OVERRIDE;
    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;
    void setText(QAccessible::Text t, const QString& text) Q_DECL_OVERRIDE;

    QAccessibleInterface* child(int index) const Q_DECL_OVERRIDE;
    int childCount() const Q_DECL_OVERRIDE;
    QAccessibleInterface* childAt(int x, int y) const Q_DECL_OVERRIDE;
    int indexOfChild(const QAccessibleInterface*) const Q_DECL_OVERRIDE;

    QAccessibleInterface* parent() const Q_DECL_OVERRIDE;
    bool isExpandable() const;

    // Cell Interface
    virtual int columnExtent() const Q_DECL_OVERRIDE;
    virtual QList<QAccessibleInterface*> columnHeaderCells() const Q_DECL_OVERRIDE;
    virtual int columnIndex() const Q_DECL_OVERRIDE;
    virtual int rowExtent() const Q_DECL_OVERRIDE;
    virtual QList<QAccessibleInterface*> rowHeaderCells() const Q_DECL_OVERRIDE;
    virtual int rowIndex() const Q_DECL_OVERRIDE;
    virtual bool isSelected() const Q_DECL_OVERRIDE;
    virtual QAccessibleInterface* table() const Q_DECL_OVERRIDE;

    inline int index() const;

private:
    QPointer<KItemListView> m_view;
    int m_index;
};

class LIBDOLPHINPRIVATE_EXPORT KItemListContainerAccessible : public QAccessibleWidget
{
public:
    explicit KItemListContainerAccessible(KItemListContainer* container);
    virtual ~KItemListContainerAccessible();

    QAccessibleInterface* child(int index) const Q_DECL_OVERRIDE;
    int childCount() const Q_DECL_OVERRIDE;
    int indexOfChild(const QAccessibleInterface* child) const Q_DECL_OVERRIDE;

private:
    const KItemListContainer* container() const;
};

#endif // QT_NO_ACCESSIBILITY

#endif
