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

static const int dolphin_map_size = 3;
static int dolphin_view_to_dir_model_column[] = {
    /* SortByName */ KDirModel::Name,
    /* SortBySize */ KDirModel::Size,
    /* SortByDate */ KDirModel::ModifiedTime 
};

static DolphinView::Sorting dir_model_column_to_dolphin_view[] = {
    /* KDirModel::Name */ DolphinView::SortByName,
    /* KDirModel::Size */ DolphinView::SortBySize,
    /* KDirModel::ModifiedTime */ DolphinView::SortByDate
};


DolphinSortFilterProxyModel::DolphinSortFilterProxyModel(QObject* parent) :
    QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);

    /*
     * sort by the user visible string for now
     */
    setSortRole(Qt::DisplayRole);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    sort(KDirModel::Name, Qt::Ascending);
}

DolphinSortFilterProxyModel::~DolphinSortFilterProxyModel()
{
}

/*
 * Update the sort column by mapping DolpginView::Sorting to
 * KDirModel::ModelColumns.
 * We will keep the sortOrder
 */
void DolphinSortFilterProxyModel::setSorting(DolphinView::Sorting sorting)
{
    Q_ASSERT( static_cast<int>(sorting) >= 0 && static_cast<int>(sorting) <= dolphin_map_size );
    sort(dolphin_view_to_dir_model_column[static_cast<int>(sorting)],
         m_sortOrder );
}

/**
 * @reimplemented, @internal
 *
 * If the view 'forces' sorting order to change we will
 * notice now.
 */
void DolphinSortFilterProxyModel::sort(int column, Qt::SortOrder sortOrder)
{
    m_sortOrder = sortOrder;
    m_sorting = column >= 0 && column <= dolphin_map_size ?
                dir_model_column_to_dolphin_view[column]  :
                DolphinView::SortByName; 
    QSortFilterProxyModel::sort(column,sortOrder);
}

/*
 * change the sort order by keeping the current column
 */
void DolphinSortFilterProxyModel::setSortOrder(Qt::SortOrder sortOrder)
{
    sort(dolphin_view_to_dir_model_column[m_sorting], sortOrder);
}

bool DolphinSortFilterProxyModel::lessThan(const QModelIndex& left,
                                           const QModelIndex& right) const
{
    /*
     * We have set a SortRole and trust the ProxyModel to do
     * the right thing for now
     */
    return QSortFilterProxyModel::lessThan(left,right);
}

#include "dolphinsortfilterproxymodel.moc"
