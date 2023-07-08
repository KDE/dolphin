/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "servicemodel.h"

#include <QIcon>

ServiceModel::ServiceModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_items()
{
}

ServiceModel::~ServiceModel()
{
}

bool ServiceModel::insertRows(int row, int count, const QModelIndex &parent)
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
        item.checked = Qt::Unchecked;
        m_items.insert(row, item);
    }
    endInsertRows();

    return true;
}

bool ServiceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    const int row = index.row();
    if (row >= rowCount()) {
        return false;
    }

    switch (role) {
    case Qt::CheckStateRole:
        m_items[row].checked = value.value<Qt::CheckState>();
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

    Q_EMIT dataChanged(index, index);
    return true;
}

QVariant ServiceModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row < rowCount()) {
        switch (role) {
        case Qt::CheckStateRole:
            return m_items[row].checked;
        case Qt::DecorationRole:
            return QIcon::fromTheme(m_items[row].icon);
        case Qt::DisplayRole:
            return m_items[row].text;
        case DesktopEntryNameRole:
            return m_items[row].desktopEntryName;
        default:
            break;
        }
    }

    return QVariant();
}

int ServiceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_items.count();
}

void ServiceModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, m_items.count());
    m_items.clear();
    endRemoveRows();
}

Qt::ItemFlags ServiceModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
}

#include "moc_servicemodel.cpp"
