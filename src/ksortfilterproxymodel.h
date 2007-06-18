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

#ifndef KSORTFILTERPROXYMODEL_H
#define KSORTFILTERPROXYMODEL_H

#include <QtGui/QSortFilterProxyModel>

#include <libdolphin_export.h>

class LIBDOLPHINPRIVATE_EXPORT KSortFilterProxyModel
    : public QSortFilterProxyModel
{
public:
    KSortFilterProxyModel(QObject *parent = 0);
    ~KSortFilterProxyModel();

    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    Qt::SortOrder sortOrder() const;

    virtual bool lessThanGeneralPurpose(const QModelIndex &left,
                                        const QModelIndex &right) const = 0;

    virtual bool lessThanCategoryPurpose(const QModelIndex &left,
                                         const QModelIndex &right) const;

private:
    Qt::SortOrder m_sortOrder;
};

#endif
