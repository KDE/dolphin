/*
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
*/

#ifndef DOLPHINMODEL_H
#define DOLPHINMODEL_H

#include <kdirmodel.h>
#include <libdolphin_export.h>

#include <QHash>
#include <QPersistentModelIndex>

class LIBDOLPHINPRIVATE_EXPORT DolphinModel : public KDirModel
{
    Q_OBJECT

public:
    enum AdditionalColumns {
        Revision = KDirModel::ColumnCount,
        ExtraColumnCount
    };

    enum RevisionState {
        LocalRevision,
        LatestRevision
        // TODO...
    };

    DolphinModel(QObject* parent = 0);
    virtual ~DolphinModel();

    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;    
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;

    bool hasRevisionData() const;

private:
    QVariant displayRoleData(const QModelIndex& index) const;
    QVariant sortRoleData(const QModelIndex& index) const;

private:
    bool m_hasRevisionData;
    QHash<QPersistentModelIndex, RevisionState> m_revisionHash;

    static const char* m_others;
};

#endif // DOLPHINMODEL_H
