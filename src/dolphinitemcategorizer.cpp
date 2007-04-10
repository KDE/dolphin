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

#include "dolphinitemcategorizer.h"

#include "dolphinview.h"

#include <klocale.h>
#include <kdirmodel.h>

#include <QSortFilterProxyModel>

DolphinItemCategorizer::DolphinItemCategorizer() :
        KItemCategorizer()
{}

DolphinItemCategorizer::~DolphinItemCategorizer()
{}

QString DolphinItemCategorizer::categoryForItem(const QModelIndex& index,
                                                int sortRole)
{
    QString retString;

    if (!index.isValid()) {
        return retString;
    }

    // KDirModel checks columns to know to which role are
    // we talking about
    QModelIndex theIndex = index.model()->index(index.row(),
                           sortRole,
                           index.parent());

    const QSortFilterProxyModel* proxyModel = static_cast<const QSortFilterProxyModel*>(index.model());
    const KDirModel* dirModel = static_cast<const KDirModel*>(proxyModel->sourceModel());

    QVariant data = theIndex.model()->data(theIndex, Qt::DisplayRole);

    QModelIndex mappedIndex = proxyModel->mapToSource(theIndex);
    KFileItem* item = dirModel->itemForIndex(mappedIndex);

    switch (sortRole) {
    case DolphinView::SortByName:
        retString = data.toString().toUpper().at(0);
        break;
    case DolphinView::SortBySize:
        int fileSize = (item) ? item->size() : -1;
        if (item && item->isDir()) {
            retString = i18n("Unknown");
        } else if (fileSize < 5242880) {
            retString = i18n("Small");
        } else if (fileSize < 10485760) {
            retString = i18n("Medium");
        } else {
            retString = i18n("Big");
        }
        break;
    }

    return retString;
}
