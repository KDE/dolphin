/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMMODELBASE_H
#define KITEMMODELBASE_H

#include <optional>

#include "dolphin_export.h"
#include "kitemviews/kitemrange.h"
#include "kitemviews/kitemset.h"

#include <QHash>
#include <QObject>
#include <QUrl>
#include <QVariant>

class QMimeData;

/**
 * @brief Base class for model implementations used by KItemListView and KItemListController.
 *
 * An item-model consists of a variable number of items. The number of items
 * is given by KItemModelBase::count(). The data of an item is accessed by a unique index
 * with KItemModelBase::data(). The indexes are integer-values counting from 0 to the
 * KItemModelBase::count() - 1.
 *
 * One item consists of a variable number of role/value-pairs.
 *
 * A model can optionally provide sorting- and grouping-capabilities.
 *
 * Also optionally it is possible to provide a tree of items by implementing the methods
 * setExpanded(), isExpanded(), isExpandable() and expandedParentsCount().
 */
class DOLPHIN_EXPORT KItemModelBase : public QObject
{
    Q_OBJECT

public:
    explicit KItemModelBase(QObject* parent = nullptr);
    explicit KItemModelBase(const QByteArray& sortRole, QObject* parent = nullptr);
    ~KItemModelBase() override;

    /** @return The number of items. */
    virtual int count() const = 0;

    virtual QHash<QByteArray, QVariant> data(int index) const = 0;

    /**
     * Sets the data for the item at \a index to the given \a values. Returns true
     * if the data was set on the item; returns false otherwise.
     *
     * The default implementation does not set the data, and will always return
     * false.
     */
    virtual bool setData(int index, const QHash<QByteArray, QVariant>& values);

    /**
     * Enables/disables the grouped sorting. The method KItemModelBase::onGroupedSortingChanged() will be
     * called so that model-implementations can react on the grouped-sorting change. Afterwards the
     * signal groupedSortingChanged() will be emitted. If the grouped sorting is enabled, the method
     * KItemModelBase::groups() must be implemented.
     */
    void setGroupedSorting(bool grouped);
    bool groupedSorting() const;

    /**
     * Sets the sort-role to \a role. The method KItemModelBase::onSortRoleChanged() will be
     * called so that model-implementations can react on the sort-role change. Afterwards the
     * signal sortRoleChanged() will be emitted.
     * The implementation should resort only if \a resortItems is true.
     */
    void setSortRole(const QByteArray& role, bool resortItems = true);
    QByteArray sortRole() const;

    /**
     * Sets the sort order to \a order. The method KItemModelBase::onSortOrderChanged() will be
     * called so that model-implementations can react on the sort order change. Afterwards the
     * signal sortOrderChanged() will be emitted.
     */
    void setSortOrder(Qt::SortOrder order);
    Qt::SortOrder sortOrder() const;

    /**
     * @return Translated description for the \p role. The description is e.g. used
     *         for the header in KItemListView.
     */
    virtual QString roleDescription(const QByteArray& role) const;

    /**
     * @return List of group headers. Each list-item consists of the index of the item
     *         that represents the first item of a group and a value represented
     *         as QVariant. The value is shown by an instance of KItemListGroupHeader.
     *         Per default an empty list is returned.
     */
    virtual QList<QPair<int, QVariant> > groups() const;

    /**
     * Expands the item with the index \a index if \a expanded is true.
     * If \a expanded is false the item will be collapsed.
     *
     * Per default no expanding of items is implemented. When implementing
     * this method it is mandatory to overwrite KItemModelBase::isExpandable()
     * and KItemListView::supportsExpandableItems() to return true.
     *
     * @return True if the operation has been successful.
     */
    virtual bool setExpanded(int index, bool expanded);

    /**
     * @return True if the item with the index \a index is expanded.
     *         Per default no expanding of items is implemented. When implementing
     *         this method it is mandatory to overwrite KItemModelBase::isExpandable()
     *         and KItemListView::supportsExpandableItems() to return true.
     */
    virtual bool isExpanded(int index) const;

    /**
     * @return True if expanding and collapsing of the item with the index \a index
     *         is supported. Per default false is returned.
     */
    virtual bool isExpandable(int index) const;

    /**
     * @return Number of expanded parent items for the item with the given index.
     *         Per default 0 is returned.
     */
    virtual int expandedParentsCount(int index) const;

    /**
     * @return MIME-data for the items given by \a indexes. The default implementation
     *         returns 0. The ownership of the returned instance is in the hand of the
     *         caller of this method. The method must be implemented if dragging of
     *         items should be possible.
     */
    virtual QMimeData* createMimeData(const KItemSet& indexes) const;

    /**
     * @return Reimplement this to return the index for the first item
     * beginning with string typed in through the keyboard, -1 if not found.
     * @param text              the text which has been typed in through the keyboard
     * @param startFromIndex    the index from which to start searching from
     */
    virtual int indexForKeyboardSearch(const QString& text, int startFromIndex = 0) const;

    /**
     * @return True, if the item with the index \a index basically supports dropping.
     *         Per default false is returned.
     *
     *         The information is used only to give a visual feedback during a drag operation
     *         and not to decide whether a drop event gets emitted. It is it is still up to
     *         the receiver of KItemListController::itemDropEvent() to decide how to handle
     *         the drop event.
     */
    // TODO: Should the MIME-data be passed too so that the model can do a more specific
    // decision whether it accepts the drop?
    virtual bool supportsDropping(int index) const;

    /**
     * @return An internal mimetype to signal that an itemDropEvent() should be rejected by
     *         the receiving model.
     *
     *         This mimeType can be used in createMimeData() to notify that the
     *         drop-onto-items events should be ignored, while the drop-between-items
     *         ones should be still accepted.
     */
    QString blacklistItemDropEventMimeType() const;

    /**
     * @return URL of the item at the specified index
     */
    virtual QUrl url(int index) const;

    /**
     * @return True, if item at specified index is a directory
     */
    virtual bool isDir(int index) const;

    /**
     * @return Parent directory of the items that are shown
     */
    virtual QUrl directory() const;
Q_SIGNALS:
    /**
     * Is emitted if one or more items have been inserted. Each item-range consists
     * of:
     * - an index where items have been inserted
     * - the number of inserted items.
     * The index of each item-range represents the index of the model
     * before the items have been inserted.
     *
     * For the item-ranges it is assured that:
     * - They don't overlap
     * - The index of item-range n is smaller than the index of item-range n + 1.
     */
    void itemsInserted(const KItemRangeList& itemRanges);

    /**
     * Is emitted if one or more items have been removed. Each item-range consists
     * of:
     * - an index where items have been inserted
     * - the number of inserted items.
     * The index of each item-range represents the index of the model
     * before the items have been removed.
     *
     * For the item-ranges it is assured that:
     * - They don't overlap
     * - The index of item-range n is smaller than the index of item-range n + 1.
     */
    void itemsRemoved(const KItemRangeList& itemRanges);

    /**
     * Is emitted if one ore more items get moved.
     * @param itemRange      Item-range that gets moved to a new position.
     * @param movedToIndexes New positions for each element of the item-range.
     *
     * For example if the model has 10 items and the items 0 and 1 get exchanged
     * with the items 5 and 6 then the parameters look like this:
     * - itemRange: has the index 0 and a count of 7.
     * - movedToIndexes: Contains the seven values 5, 6, 2, 3, 4, 0, 1
     *
     * This signal implies that the groups might have changed. Therefore,
     * gropusChanged() is not emitted if this signal is emitted.
     */
    void itemsMoved(const KItemRange& itemRange, const QList<int>& movedToIndexes);

    void itemsChanged(const KItemRangeList& itemRanges, const QSet<QByteArray>& roles);

    /**
     * Is emitted if the groups have changed, even though the order of the
     * items has not been modified.
     */
    void groupsChanged();

    void groupedSortingChanged(bool current);
    void sortRoleChanged(const QByteArray& current, const QByteArray& previous);
    void sortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous);

protected:
    /**
     * Is invoked if the grouped sorting has been changed by KItemModelBase::setGroupedSorting(). Allows
     * to react on the changed grouped sorting  before the signal groupedSortingChanged() will be emitted.
     */
    virtual void onGroupedSortingChanged(bool current);

    /**
     * Is invoked if the sort role has been changed by KItemModelBase::setSortRole(). Allows
     * to react on the changed sort role before the signal sortRoleChanged() will be emitted.
     * The implementation must assure that the items are sorted by the role given by \a current.
     * Usually the most efficient way is to emit a
     * itemsRemoved() signal for all items, reorder the items internally and to emit a
     * itemsInserted() signal afterwards.
     * The implementation should resort only if \a resortItems is true.
     */
    virtual void onSortRoleChanged(const QByteArray& current, const QByteArray& previous, bool resortItems = true);

    /**
     * Is invoked if the sort order has been changed by KItemModelBase::setSortOrder(). Allows
     * to react on the changed sort order before the signal sortOrderChanged() will be emitted.
     * The implementation must assure that the items are sorted by the order given by \a current.
     * Usually the most efficient way is to emit a
     * itemsRemoved() signal for all items, reorder the items internally and to emit a
     * itemsInserted() signal afterwards.
     */
    virtual void onSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous);

private:
    bool m_groupedSorting;
    QByteArray m_sortRole;
    Qt::SortOrder m_sortOrder;
};

inline Qt::SortOrder KItemModelBase::sortOrder() const
{
    return m_sortOrder;
}

#endif


