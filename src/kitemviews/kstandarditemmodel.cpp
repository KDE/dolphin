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

#include "kstandarditemmodel.h"

#include <KDebug>
#include "kstandarditem.h"

KStandardItemModel::KStandardItemModel(QObject* parent) :
    KItemModelBase(parent),
    m_items(),
    m_indexesForItems()
{
}

KStandardItemModel::~KStandardItemModel()
{
    qDeleteAll(m_items);
    m_items.clear();
    m_indexesForItems.clear();
}

void KStandardItemModel::insertItem(int index, KStandardItem* item)
{
    if (index < 0 || index > count() || !item) {
        delete item;
        return;
    }

    if (!m_indexesForItems.contains(item)) {
        item->m_model = this;
        m_items.insert(index, item);
        m_indexesForItems.insert(item, index);

        // Inserting an item requires to update the indexes
        // afterwards from m_indexesForItems.
        for (int i = index + 1; i < m_items.count(); ++i) {
            m_indexesForItems.insert(m_items[i], i);
        }

        // TODO: no hierarchical items are handled yet

        onItemInserted(index);
        emit itemsInserted(KItemRangeList() << KItemRange(index, 1));
    }
}

void KStandardItemModel::changeItem(int index, KStandardItem* item)
{
    if (index < 0 || index >= count() || !item) {
        delete item;
        return;
    }

    item->m_model = this;

    QSet<QByteArray> changedRoles;

    KStandardItem* oldItem = m_items[index];
    const QHash<QByteArray, QVariant> oldData = oldItem->data();
    const QHash<QByteArray, QVariant> newData = item->data();

    // Determine which roles have been changed
    QHashIterator<QByteArray, QVariant> it(oldData);
    while (it.hasNext()) {
        it.next();
        const QByteArray role = it.key();
        const QVariant oldValue = it.value();
        if (newData.contains(role) && newData.value(role) != oldValue) {
            changedRoles.insert(role);
        }
    }

    m_indexesForItems.remove(oldItem);
    delete oldItem;
    oldItem = 0;

    m_items[index] = item;
    m_indexesForItems.insert(item, index);

    onItemChanged(index, changedRoles);
    emit itemsChanged(KItemRangeList() << KItemRange(index, 1), changedRoles);
}

void KStandardItemModel::removeItem(int index)
{
    if (index >= 0 && index < count()) {
        KStandardItem* item = m_items[index];
        m_indexesForItems.remove(item);
        m_items.removeAt(index);

        // Removing an item requires to update the indexes
        // afterwards from m_indexesForItems.
        for (int i = index; i < m_items.count(); ++i) {
            m_indexesForItems.insert(m_items[i], i);
        }

        onItemRemoved(index, item);

        delete item;
        item = 0;

        emit itemsRemoved(KItemRangeList() << KItemRange(index, 1));

        // TODO: no hierarchical items are handled yet
    }
}

KStandardItem* KStandardItemModel::item(int index) const
{
    if (index < 0 || index >= m_items.count()) {
        return 0;
    }
    return m_items[index];
}

int KStandardItemModel::index(const KStandardItem* item) const
{
    return m_indexesForItems.value(item, -1);
}

void KStandardItemModel::appendItem(KStandardItem *item)
{
    insertItem(m_items.count(), item);
}

int KStandardItemModel::count() const
{
    return m_items.count();
}

QHash<QByteArray, QVariant> KStandardItemModel::data(int index) const
{
    if (index >= 0 && index < count()) {
        const KStandardItem* item = m_items[index];
        if (item) {
            return item->data();
        }
    }
    return QHash<QByteArray, QVariant>();
}

bool KStandardItemModel::setData(int index, const QHash<QByteArray, QVariant>& values)
{
    Q_UNUSED(values);
    if (index < 0 || index >= count()) {
        return false;
    }

    return true;
}

QMimeData* KStandardItemModel::createMimeData(const QSet<int>& indexes) const
{
    Q_UNUSED(indexes);
    return 0;
}

int KStandardItemModel::indexForKeyboardSearch(const QString& text, int startFromIndex) const
{
    Q_UNUSED(text);
    Q_UNUSED(startFromIndex);
    return -1;
}

bool KStandardItemModel::supportsDropping(int index) const
{
    Q_UNUSED(index);
    return false;
}

QString KStandardItemModel::roleDescription(const QByteArray& role) const
{
    Q_UNUSED(role);
    return QString();
}

QList<QPair<int, QVariant> > KStandardItemModel::groups() const
{
    QList<QPair<int, QVariant> > groups;

    const QByteArray role = sortRole();
    bool isFirstGroupValue = true;
    QString groupValue;
    const int maxIndex = count() - 1;
    for (int i = 0; i <= maxIndex; ++i) {
        const QString newGroupValue = m_items.at(i)->dataValue(role).toString();
        if (newGroupValue != groupValue || isFirstGroupValue) {
            groupValue = newGroupValue;
            groups.append(QPair<int, QVariant>(i, newGroupValue));
            isFirstGroupValue = false;
        }
    }

    return groups;
}

void KStandardItemModel::onItemInserted(int index)
{
    Q_UNUSED(index);
}

void KStandardItemModel::onItemChanged(int index, const QSet<QByteArray>& changedRoles)
{
    Q_UNUSED(index);
    Q_UNUSED(changedRoles);
}

void KStandardItemModel::onItemRemoved(int index, KStandardItem* removedItem)
{
    Q_UNUSED(index);
    Q_UNUSED(removedItem);
}


#include "kstandarditemmodel.moc"
