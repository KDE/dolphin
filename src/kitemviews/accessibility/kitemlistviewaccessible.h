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
     * Called by KItemListContainer when it passes on focus to the view. Accessible focus is then meant to go towards this accessible interface and a detailed
     * announcement of the current view state (current item and overall location state) should be triggered.
     */
    void setAccessibleFocusAndAnnounceAll();

    /**
     * Called multiple times while a new location is loading. A timer is restarted, and if this method has not been called for a split second, the newly loaded
     * location is finally announced.
     * Either the @p placeholderMessage is announced when there are no items in the view (yet), or the current item is announced together with the view state.
     *
     * @param placeholderMessage    The message that should be announced when no items are visible (yet). This message is mostly identical to
     *                              DolphinView::m_placeholderLabel in both content and purpose. @see DolphinView::updatePlaceHolderLabel().
     *
     * If there are items in the view and the placeholderMessage is therefore not visible, the current item and location is announced instead.
     */
    void announceNewlyLoadedLocation(const QString &placeholderMessage);

    /**
     * Starts a timer that will trigger an announcement of the current item. The timer makes sure that quick changes to the current item will only lead to a
     * singular announcement. This way when a new folder is loaded we only trigger a single announcement even if the items quickly change.
     *
     * When m_shouldAnnounceLocation is true, the current location will be announced following the announcement of the current item.
     *
     * If the current item is invalid, only the current location is announced, which has the responsibility of then telling why there is no valid item in the
     * view.
     */
    void announceCurrentItem();

private:
    /**
     * @returns a KItemListDelegateAccessible representing the file or folder at the @index. Returns nullptr for invalid indices.
     * If a KItemListDelegateAccessible for an index does not yet exist, it will be created.
     * Index is 0-based.
     */
    inline QAccessibleInterface *accessibleDelegate(int index) const;

    KItemListSelectionManager *selectionManager() const;

private Q_SLOTS:
    /**
     * Is run in response to announceCurrentItem(). If the current item exists, it is announced. Otherwise the view is announced.
     * Also announces some general information about the current location if it has changed recently.
     */
    void slotAnnounceCurrentItemTimerTimeout();

private:
    /** @see setPlaceholderMessage(). */
    QString m_placeholderMessage;

    /**
     * Is started by announceCurrentItem().
     * If we announce the current item as soon as it changes, we would announce multiple items while loading a folder.
     * This timer makes sure we only announce the singular currently focused item when things have settled down.
     */
    QTimer *m_announceCurrentItemTimer;

    /**
     * If we want announceCurrentItem() to always announce the current item, we must be aware if this is equal to the previous current item, because
     * - if the accessibility focus moves to a new item, it is automatically announced, but
     * - if the focus is still on the item at the same index, the focus does not technically move to a new item even if the file at that index changed, so we
     *   need to instead send change events for the accessible name and accessible description.
     */
    int m_lastAnnouncedIndex = -1;

    /**
     * Is set to true in response to announceDescriptionChange(). When true, the next time slotAnnounceCurrentItemTimerTimeout() is called the description is
     * also announced. Then this bool is set to false.
     */
    bool m_shouldAnnounceLocation = true;

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
