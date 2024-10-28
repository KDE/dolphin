/*
 * SPDX-FileCopyrightText: 2012 Amandeep Singh <aman.dedman@gmail.com>
 * SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTDELEGATEACCESSIBLE_H
#define KITEMLISTDELEGATEACCESSIBLE_H

#include "dolphin_export.h"

#include <QAccessibleInterface>
#include <QAccessibleTableCellInterface>
#include <QPointer>

class KItemListView;

/**
 * The accessibility class that represents singular files or folders in the main view.
 */
class DOLPHIN_EXPORT KItemListDelegateAccessible : public QAccessibleInterface, public QAccessibleTableCellInterface
{
public:
    KItemListDelegateAccessible(KItemListView *view, int m_index);

    void *interface_cast(QAccessible::InterfaceType type) override;
    QObject *object() const override;
    bool isValid() const override;
    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QRect rect() const override;
    QString text(QAccessible::Text t) const override;
    void setText(QAccessible::Text t, const QString &text) override;

    QAccessibleInterface *child(int index) const override;
    int childCount() const override;
    QAccessibleInterface *childAt(int x, int y) const override;
    int indexOfChild(const QAccessibleInterface *) const override;

    QAccessibleInterface *parent() const override;
    bool isExpandable() const;

    // Cell Interface
    int columnExtent() const override;
    QList<QAccessibleInterface *> columnHeaderCells() const override;
    int columnIndex() const override;
    int rowExtent() const override;
    QList<QAccessibleInterface *> rowHeaderCells() const override;
    int rowIndex() const override;
    bool isSelected() const override;
    QAccessibleInterface *table() const override;

    int index() const;

private:
    QPointer<KItemListView> m_view;
    int m_index;
};

#endif // KITEMLISTDELEGATEACCESSIBLE_H
