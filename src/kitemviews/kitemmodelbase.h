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

struct KItemRange
{
    KItemRange(int index, int count);
    int index;
    int count;
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

signals:
    void itemsInserted(const KItemRangeList& itemRanges);
    void itemsRemoved(const KItemRangeList& itemRanges);
    void itemsMoved(const KItemRangeList& itemRanges);
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


