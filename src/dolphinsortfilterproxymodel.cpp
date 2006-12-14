/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Gregor Kališnik <gregor@podnapisi.net>          *
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

#include "dolphinsortfilterproxymodel.h"

#include <kdirmodel.h>
#include <kfileitem.h>

DolphinSortFilterProxyModel::DolphinSortFilterProxyModel(QObject* parent) :
    QSortFilterProxyModel(parent),
    m_sorting(DolphinView::SortByName),
    m_sortOrder(Qt::Ascending)
{
}

DolphinSortFilterProxyModel::~DolphinSortFilterProxyModel()
{
}

void DolphinSortFilterProxyModel::setSorting(DolphinView::Sorting sorting)
{
    if (sorting != m_sorting) {
        m_sorting = sorting;
        // TODO: how to trigger an update?
    }
}

void DolphinSortFilterProxyModel::setSortOrder(Qt::SortOrder sortOrder)
{
    if (sortOrder != m_sortOrder) {
        m_sortOrder = sortOrder;
        // TODO: how to trigger an update?
    }
}

bool DolphinSortFilterProxyModel::lessThan(const QModelIndex& left,
                                           const QModelIndex& right) const
{
    // TODO: this is just a test implementation
    KDirModel* model = static_cast<KDirModel*>(sourceModel());

    KFileItem* leftItem = model->itemForIndex(left);
    if (leftItem == 0) {
        return true;
    }

    KFileItem* rightItem = model->itemForIndex(right);
    if (rightItem == 0) {
        return false;
    }

    return leftItem->name() > rightItem->name();
}

#include "dolphinsortfilterproxymodel.moc"
