/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "servicemodel.h"

ServiceModel::ServiceModel(QObject* parent) :
    QAbstractListModel(parent),
    m_items()
{
}

ServiceModel::~ServiceModel()
{
}

bool ServiceModel::insertRows(int row, int count, const QModelIndex& parent)
{
    if (row > rowCount()) {
        return false;
    }

    if (count <= 0) {
        count = 1;
    }

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        ServiceItem item;
        item.checked = false;
        item.configurable = false;
        m_items.insert(row, item);
    }
    endInsertRows();

    return true;
}

bool ServiceModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    const int row = index.row();
    if (row >= rowCount()) {
        return false;
    }

    switch (role) {
    case Qt::CheckStateRole:
        m_items[row].checked = value.toBool();
        break;
    case ConfigurableRole:
        m_items[row].configurable = value.toBool();
        break;
    case Qt::DecorationRole:
        m_items[row].icon = value.toString();
        break;
    case Qt::DisplayRole:
        m_items[row].text = value.toString();
        break;
    case DesktopEntryNameRole:
        m_items[row].desktopEntryName = value.toString();
        break;
    default:
        return false;
    }

    emit dataChanged(index, index);
    return true;
}

QVariant ServiceModel::data(const QModelIndex& index, int role) const
{
    const int row = index.row();
    if (row < rowCount()) {
        switch (role) {
        case ConfigurableRole:     return m_items[row].configurable;
        case Qt::CheckStateRole:   return m_items[row].checked;
        case Qt::DecorationRole:   return m_items[row].icon;
        case Qt::DisplayRole:      return m_items[row].text;
        case DesktopEntryNameRole: return m_items[row].desktopEntryName;
        default: break;
        }
    }

    return QVariant();
}

int ServiceModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_items.count();
}

#include "servicemodel.moc"
