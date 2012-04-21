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
#include "kstandarditem.h"

KStandardItemModel::KStandardItemModel(QObject* parent) :
    KItemModelBase(parent),
    m_items(),
    m_indexesForItems()
{
}

KStandardItemModel::~KStandardItemModel()
{
}

void KStandardItemModel::insertItem(int index, KStandardItem* item)
{
    if (!m_indexesForItems.contains(item) && !item->m_model) {
        m_items.insert(index, item);
        m_indexesForItems.insert(item, index);
        item->m_model = this;
        // TODO: no hierarchical items are handled yet
    }
}

void KStandardItemModel::appendItem(KStandardItem *item)
{
    insertItem(m_items.count(), item);
}

void KStandardItemModel::removeItem(KStandardItem* item)
{
    const int index = m_indexesForItems.value(item, -1);
    if (index >= 0) {
        m_items.removeAt(index);
        m_indexesForItems.remove(item);
        delete item;
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

int KStandardItemModel::count() const
{
    return m_items.count();
}

QHash<QByteArray, QVariant> KStandardItemModel::data(int index) const
{
    // TODO: Ugly hack
    QHash<QByteArray, QVariant> values;
    const KStandardItem* item = m_items[index];
    values.insert("text", item->text());
    values.insert("iconName", item->icon().name());
    return values;
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
    return QList<QPair<int, QVariant> >();
}

#include "kstandarditemmodel.moc"
