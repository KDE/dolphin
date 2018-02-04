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

#include "dolphin_export.h"

#include <QAccessible>
#include <QAccessibleObject>
#include <QAccessibleWidget>
#include <QPointer>

class KItemListView;
class KItemListContainer;

class DOLPHIN_EXPORT KItemListViewAccessible: public QAccessibleObject, public QAccessibleTableInterface
{
public:
    explicit KItemListViewAccessible(KItemListView* view);
    ~KItemListViewAccessible() override;

    void* interface_cast(QAccessible::InterfaceType type) override;

    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QString text(QAccessible::Text t) const override;
    QRect rect() const override;

    QAccessibleInterface* child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface*) const override;
    QAccessibleInterface* childAt(int x, int y) const override;
    QAccessibleInterface* parent() const override;

    // Table interface
    QAccessibleInterface* cellAt(int row, int column) const override;
    QAccessibleInterface* caption() const override;
    QAccessibleInterface* summary() const override;
    QString columnDescription(int column) const override;
    QString rowDescription(int row) const override;
    int columnCount() const override;
    int rowCount() const override;

    // Selection
    int selectedCellCount() const override;
    int selectedColumnCount() const override;
    int selectedRowCount() const override;
    QList<QAccessibleInterface*> selectedCells() const override;
    QList<int> selectedColumns() const override;
    QList<int> selectedRows() const override;
    bool isColumnSelected(int column) const override;
    bool isRowSelected(int row) const override;
    bool selectRow(int row) override;
    bool selectColumn(int column) override;
    bool unselectRow(int row) override;
    bool unselectColumn(int column) override;
    void modelChange(QAccessibleTableModelChangeEvent*) override;

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

class DOLPHIN_EXPORT KItemListAccessibleCell: public QAccessibleInterface, public QAccessibleTableCellInterface
{
public:
    KItemListAccessibleCell(KItemListView* view, int m_index);

    void* interface_cast(QAccessible::InterfaceType type) override;
    QObject* object() const override;
    bool isValid() const override;
    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QRect rect() const override;
    QString text(QAccessible::Text t) const override;
    void setText(QAccessible::Text t, const QString& text) override;

    QAccessibleInterface* child(int index) const override;
    int childCount() const override;
    QAccessibleInterface* childAt(int x, int y) const override;
    int indexOfChild(const QAccessibleInterface*) const override;

    QAccessibleInterface* parent() const override;
    bool isExpandable() const;

    // Cell Interface
    int columnExtent() const override;
    QList<QAccessibleInterface*> columnHeaderCells() const override;
    int columnIndex() const override;
    int rowExtent() const override;
    QList<QAccessibleInterface*> rowHeaderCells() const override;
    int rowIndex() const override;
    bool isSelected() const override;
    QAccessibleInterface* table() const override;

    inline int index() const;

private:
    QPointer<KItemListView> m_view;
    int m_index;
};

class DOLPHIN_EXPORT KItemListContainerAccessible : public QAccessibleWidget
{
public:
    explicit KItemListContainerAccessible(KItemListContainer* container);
    ~KItemListContainerAccessible() override;

    QAccessibleInterface* child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface* child) const override;

private:
    const KItemListContainer* container() const;
};

#endif // QT_NO_ACCESSIBILITY

#endif
