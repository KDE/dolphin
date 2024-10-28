/*
 * SPDX-FileCopyrightText: 2012 Amandeep Singh <aman.dedman@gmail.com>
 * SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTVIEWACCESSIBLE_H
#define KITEMLISTVIEWACCESSIBLE_H

#include "dolphin_export.h"

#include <QAccessible>
#include <QAccessibleObject>
#include <QAccessibleWidget>
#include <QPointer>

class KItemListView;
class KItemListContainer;
class KItemListContainerAccessible;
class KItemListSelectionManager;

/**
 * The main class for making the main view accessible.
 *
 * Such a class is necessary because the KItemListView is a mostly custom entity. This class provides a lot of the functionality to make it possible to
 * interact with the view using accessibility tools. It implements various interfaces mostly to generally allow working with the view as a whole. However,
 * actually interacting with singular items within the view is implemented in KItemListDelegateAccessible.
 *
 * @note For documentation of most of the methods within this class, check out the documentation of the methods which are being overriden here.
 */
class DOLPHIN_EXPORT KItemListViewAccessible : public QAccessibleObject,
                                               public QAccessibleSelectionInterface,
                                               public QAccessibleTableInterface,
                                               public QAccessibleActionInterface
{
public:
    explicit KItemListViewAccessible(KItemListView *view, KItemListContainerAccessible *parent);
    ~KItemListViewAccessible() override;

    // QAccessibleObject
    void *interface_cast(QAccessible::InterfaceType type) override;

    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QString text(QAccessible::Text t) const override;
    QRect rect() const override;

    QAccessibleInterface *child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *) const override;
    QAccessibleInterface *childAt(int x, int y) const override;
    QAccessibleInterface *parent() const override;

    // Table interface
    QAccessibleInterface *cellAt(int row, int column) const override;
    QAccessibleInterface *caption() const override;
    QAccessibleInterface *summary() const override;
    QString columnDescription(int column) const override;
    QString rowDescription(int row) const override;
    int columnCount() const override;
    int rowCount() const override;

    // Selection
    int selectedCellCount() const override;
    int selectedColumnCount() const override;
    int selectedRowCount() const override;
    QList<QAccessibleInterface *> selectedCells() const override;
    QList<int> selectedColumns() const override;
    QList<int> selectedRows() const override;
    bool isColumnSelected(int column) const override;
    bool isRowSelected(int row) const override;
    bool selectRow(int row) override;
    bool selectColumn(int column) override;
    bool unselectRow(int row) override;
    bool unselectColumn(int column) override;
    void modelChange(QAccessibleTableModelChangeEvent *) override;

    // Selection interface
    /** Clear selection */
    bool clear() override;
    bool isSelected(QAccessibleInterface *childItem) const override;
    bool select(QAccessibleInterface *childItem) override;
    bool selectAll() override;
    QAccessibleInterface *selectedItem(int selectionIndex) const override;
    int selectedItemCount() const override;
    QList<QAccessibleInterface *> selectedItems() const override;
    bool unselect(QAccessibleInterface *childItem) override;

    // Action interface
    QStringList actionNames() const override;
    void doAction(const QString &actionName) override;
    QStringList keyBindingsForAction(const QString &actionName) const override;

    // Custom non-interface methods
    KItemListView *view() const;

    /**
     * Moves the focus to the list view itself so an overview over the state can be given.
     * @param placeholderMessage   the message that should be announced when no items are visible (yet). This message is mostly identical to
     *                              DolphinView::m_placeholderLabel in both content and purpose. @see DolphinView::updatePlaceHolderLabel().
     */
    void announceOverallViewState(const QString &placeholderMessage);

    /**
     * Announces that the description of the view has changed. The changed description will only be announced if the view has focus (from an accessibility
     * point of view). This method ensures that multiple calls to this method within a small time frame will only lead to a singular announcement instead of
     * multiple or already outdated ones, so calling this method instead of manually sending accessibility events for this view is preferred.
     */
    void announceDescriptionChange();

protected:
    virtual void modelReset();
    /**
     * @returns a KItemListDelegateAccessible representing the file or folder at the @index. Returns nullptr for invalid indices.
     * If a KItemListDelegateAccessible for an index does not yet exist, it will be created.
     * Index is 0-based.
     */
    inline QAccessibleInterface *accessibleDelegate(int index) const;

    KItemListSelectionManager *selectionManager() const;

private:
    /** @see setPlaceholderMessage(). */
    QString m_placeholderMessage;

    QTimer *m_announceDescriptionChangeTimer;

    class AccessibleIdWrapper
    {
    public:
        AccessibleIdWrapper();
        bool isValid;
        QAccessible::Id id;
    };
    /**
     * A list that maps the indices of the children of this KItemListViewAccessible to the accessible ids of the matching KItemListDelegateAccessible objects.
     * For example: m_accessibleDelegates.at(2) would be the AccessibleIdWrapper with an id which can be used to retrieve the QAccessibleObject that represents
     *              the third file in this view.
     */
    mutable QVector<AccessibleIdWrapper> m_accessibleDelegates;

    KItemListContainerAccessible *m_parent;
};

#endif
