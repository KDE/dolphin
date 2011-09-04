/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs:                *
 *   http://qt.gitorious.org/qt-labs/itemviews-ng                          *
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

#ifndef KITEMMODELBASE_H
#define KITEMMODELBASE_H

#include <libdolphin_export.h>

#include <QHash>
#include <QObject>
#include <QSet>
#include <QVariant>

class QMimeData;

struct KItemRange
{
    KItemRange(int index, int count);
    int index;
    int count;

    bool operator == (const KItemRange& other) const;
};
typedef QList<KItemRange> KItemRangeList;

/**
 * @brief Base class for model implementations used by KItemListView and KItemListController.
 *
 * A item-model consists of a variable number of items. The number of items
 * is given by KItemModelBase::count(). The data of an item is accessed by a unique index
 * with KItemModelBase::data(). The indexes are integer-values counting from 0 to the
 * KItemModelBase::count() - 1.
 *
 * One item consists of a variable number of role/value-pairs.
 *
 * A model can optionally provide sorting- and/or grouping-capabilities.
 */
class LIBDOLPHINPRIVATE_EXPORT KItemModelBase : public QObject
{
    Q_OBJECT

public:
    KItemModelBase(QObject* parent = 0);
    KItemModelBase(const QByteArray& groupRole, const QByteArray& sortRole, QObject* parent = 0);
    virtual ~KItemModelBase();

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
     * @return True if the model supports grouping of data. Per default false is returned.
     *         If the model should support grouping it is necessary to overwrite
     *         this method to return true and to implement KItemModelBase::onGroupRoleChanged().
     */
    virtual bool supportsGrouping() const;

    /**
     * Sets the group-role to \a role. The method KItemModelBase::onGroupRoleChanged() will be
     * called so that model-implementations can react on the group-role change. Afterwards the
     * signal groupRoleChanged() will be emitted.
     */
    void setGroupRole(const QByteArray& role);
    QByteArray groupRole() const;

    /**
     * @return True if the model supports sorting of data. Per default false is returned.
     *         If the model should support sorting it is necessary to overwrite
     *         this method to return true and to implement KItemModelBase::onSortRoleChanged().
     */
    virtual bool supportsSorting() const;

    /**
     * Sets the sor-role to \a role. The method KItemModelBase::onSortRoleChanged() will be
     * called so that model-implementations can react on the sort-role change. Afterwards the
     * signal sortRoleChanged() will be emitted.
     */
    void setSortRole(const QByteArray& role);
    QByteArray sortRole() const;

    virtual QString roleDescription(const QByteArray& role) const;

    /**
     * @return MIME-data for the items given by \a indexes. The default implementation
     *         returns 0. The ownership of the returned instance is in the hand of the
     *         caller of this method.
     */
    virtual QMimeData* createMimeData(const QSet<int>& indexes) const;

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

signals:
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
     * @param itemRanges     Item-ranges that get moved to a new position.
     * @param movedToIndexes New position of the ranges.
     * It is assured that the itemRanges list has the same size as the movedToIndexes list.
     *
     * For the item-ranges it is assured that:
     * - They don't overlap
     * - The index of item-range n is smaller than the index of item-range n + 1.
     */
    void itemsMoved(const KItemRangeList& itemRanges, const QList<int> movedToIndexes);

    void itemsChanged(const KItemRangeList& itemRanges, const QSet<QByteArray>& roles);

    void groupRoleChanged(const QByteArray& current, const QByteArray& previous);
    void sortRoleChanged(const QByteArray& current, const QByteArray& previous);

protected:
    /**
     * Is invoked if the group role has been changed by KItemModelBase::setGroupRole(). Allows
     * to react on the changed group role before the signal groupRoleChanged() will be emitted.
     * The implementation must assure that the items are sorted in a way that they are grouped
     * by the role given by \a current. Usually the most efficient way is to emit a
     * itemsRemoved() signal for all items, reorder the items internally and to emit a
     * itemsInserted() signal afterwards.
     */
    virtual void onGroupRoleChanged(const QByteArray& current, const QByteArray& previous);

    /**
     * Is invoked if the sort role has been changed by KItemModelBase::setSortRole(). Allows
     * to react on the changed sort role before the signal sortRoleChanged() will be emitted.
     * The implementation must assure that the items are sorted by the role given by \a current.
     * Usually the most efficient way is to emit a
     * itemsRemoved() signal for all items, reorder the items internally and to emit a
     * itemsInserted() signal afterwards.
     */
    virtual void onSortRoleChanged(const QByteArray& current, const QByteArray& previous);

private:
    QByteArray m_groupRole;
    QByteArray m_sortRole;
};

#endif


