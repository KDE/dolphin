/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@gmail.com>
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

class LIBDOLPHINPRIVATE_EXPORT DolphinModel
    : public KDirModel
{
public:
    enum AdditionalColumns {
        Rating = ColumnCount, // ColumnCount defined at KDirModel
        Tags,
        ExtraColumnCount
    };

    DolphinModel(QObject *parent = 0);
    virtual ~DolphinModel();

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

    /**
     * Returns the rating for the item with the index \a index. 0 is
     * returned if no item could be found.
     */
    static quint32 ratingForIndex(const QModelIndex& index);

    /**
     * Returns the tags for the item with the index \a index. If no
     * tag is applied, a predefined string will be returned.
     */
    static QString tagsForIndex(const QModelIndex& index);
};

#endif // DOLPHINMODEL_H
