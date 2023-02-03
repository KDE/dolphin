/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
    enum Role { DesktopEntryNameRole = Qt::UserRole, ConfigurableRole };

    explicit ServiceModel(QObject *parent = nullptr);
    ~ServiceModel() override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    void clear();

private:
    struct ServiceItem {
        bool checked;
        bool configurable;
        QString icon;
        QString text;
        QString desktopEntryName;
    };

    QList<ServiceItem> m_items;
};

#endif
