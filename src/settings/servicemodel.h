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

#ifndef SERVICEMODEL_H
#define SERVICEMODEL_H

#include <QAbstractListModel>
#include <QList>

/**
 * @brief Provides a simple model for enabling/disabling services
 *
 * The following roles are supported:
 * - Qt::DisplayRole: Name of the service
 * - Qt::DecorationRole: Icon name of the service
 * - Qt::CheckStateRole: Specifies whether the service is enabled
 * - ServiceModel::DesktopEntryNameRole: Name of the desktop-entry of the service
 * - ServiceModel::Configurable: Specifies whether the service is configurable by the user
 */
class ServiceModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        DesktopEntryNameRole = Qt::UserRole,
        ConfigurableRole
    };

    explicit ServiceModel(QObject* parent = 0);
    virtual ~ServiceModel();

    virtual bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex());
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;

 private:
    struct ServiceItem
    {
        bool checked;
        bool configurable;
        QString icon;
        QString text;
        QString desktopEntryName;
    };

    QList<ServiceItem> m_items;
};

#endif
