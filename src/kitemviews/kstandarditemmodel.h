/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef KSTANDARDITEMMODEL_H
#define KSTANDARDITEMMODEL_H

#include <libdolphin_export.h>
#include <kitemviews/kitemmodelbase.h>
#include <QHash>
#include <QList>

class KStandardItem;

/**
 * @brief Model counterpart for KStandardItemView.
 *
 * Allows to add items to the model in an easy way by the
 * class KStandardItem.
 *
 * @see KStandardItem
 */
class LIBDOLPHINPRIVATE_EXPORT KStandardItemModel : public KItemModelBase
{
    Q_OBJECT

public:
    explicit KStandardItemModel(QObject* parent = 0);
    virtual ~KStandardItemModel();

    /**
     * Inserts the item \a item at the index \a index. If the index
     * is equal to the number of items of the model, the item
     * gets appended as last element. KStandardItemModel takes
     * the ownership of the item. If the index is invalid, the item
     * gets deleted.
     */
    void insertItem(int index, KStandardItem* item);

    /**
     * Changes the item on the index \a index to \a item.
     * KStandardItemModel takes the ownership of the item. The
     * old item gets deleted. If the index is invalid, the item
     * gets deleted.
     */
    void changeItem(int index, KStandardItem* item);

    void removeItem(int index);
    KStandardItem* item(int index) const;
    int index(const KStandardItem* item) const;

    /**
     * Convenience method for insertItem(count(), item).
     */
    void appendItem(KStandardItem* item);

    virtual int count() const;
    virtual QHash<QByteArray, QVariant> data(int index) const;
    virtual bool setData(int index, const QHash<QByteArray, QVariant>& values);
    virtual QMimeData* createMimeData(const QSet<int>& indexes) const;
    virtual int indexForKeyboardSearch(const QString& text, int startFromIndex = 0) const;
    virtual bool supportsDropping(int index) const;
    virtual QString roleDescription(const QByteArray& role) const;
    virtual QList<QPair<int, QVariant> > groups() const;

    virtual void clear();
protected:
    /**
     * Is invoked after an item has been inserted and before the signal
     * itemsInserted() gets emitted.
     */
    virtual void onItemInserted(int index);

    /**
     * Is invoked after an item or one of its roles has been changed and
     * before the signal itemsChanged() gets emitted.
     */
    virtual void onItemChanged(int index, const QSet<QByteArray>& changedRoles);

    /**
     * Is invoked after an item has been removed and before the signal
     * itemsRemoved() gets emitted. The item \a removedItem has already
     * been removed from the model and will get deleted after the
     * execution of onItemRemoved().
     */
    virtual void onItemRemoved(int index, KStandardItem* removedItem);

private:
    QList<KStandardItem*> m_items;
    QHash<const KStandardItem*, int> m_indexesForItems;

    friend class KStandardItem;
    friend class KStandardItemModelTest;  // For unit testing
};

#endif


