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

#include <QtGui/QSortFilterProxyModel>

DolphinItemCategorizer::DolphinItemCategorizer() :
    KItemCategorizer()
{
}

DolphinItemCategorizer::~DolphinItemCategorizer()
{
}

QString DolphinItemCategorizer::categoryForItem(const QModelIndex& index,
                                                int sortRole)
{
    QString retString;

    if (!index.isValid())
    {
        return retString;
    }

    int indexColumn;

    switch (sortRole)
    {
        case DolphinView::SortByName:
            indexColumn = KDirModel::Name;
            break;
        case DolphinView::SortBySize:
            indexColumn = KDirModel::Size;
            break;
        default:
        return retString;
    }

    // KDirModel checks columns to know to which role are
    // we talking about
    QModelIndex theIndex = index.model()->index(index.row(),
                                                indexColumn,
                           index.parent());

    if (!theIndex.isValid()) {
        return retString;
    }

    QVariant data = theIndex.model()->data(theIndex, Qt::DisplayRole);

    const KDirModel *dirModel = qobject_cast<const KDirModel*>(index.model());
    KFileItem* item = dirModel->itemForIndex(index);

    switch (sortRole)
    {
    case DolphinView::SortByName:
            if (data.toString().size())
            {
                if (!item->isHidden() && data.toString().at(0).isLetter())
                    retString = data.toString().toUpper().at(0);
                else if (item->isHidden() && data.toString().at(0) == '.' &&
                         data.toString().at(1).isLetter())
                    retString = data.toString().toUpper().at(1);
                else if (item->isHidden() && data.toString().at(0) == '.' &&
                         !data.toString().at(1).isLetter())
                    retString = i18n("Others");
                else if (item->isHidden() && data.toString().at(0) != '.')
                    retString = data.toString().toUpper().at(0);
                else if (item->isHidden())
                    retString = data.toString().toUpper().at(0);
                else
                    retString = i18n("Others");
            }
        break;
    case DolphinView::SortBySize:
        int fileSize = (item) ? item->size() : -1;
        if (item && item->isDir()) {
            retString = i18n("Folders");
        } else if (fileSize < 5242880) {
            retString = i18nc("Size", "Small");
        } else if (fileSize < 10485760) {
            retString = i18nc("Size", "Medium");
        } else {
            retString = i18nc("Size", "Big");
        }
        break;
    }

    return retString;
}
