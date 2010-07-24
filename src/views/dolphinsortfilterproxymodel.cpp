/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz <peter.penz@gmx.at>             *
 *   Copyright (C) 2006 by Dominic Battre <dominic@battre.de>              *
 *   Copyright (C) 2006 by Martin Pool <mbp@canonical.com>                 *
 *   Copyright (C) 2007 by Rafael Fernández López <ereslibre@kde.org>      *
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

DolphinSortFilterProxyModel::DolphinSortFilterProxyModel(QObject* parent) :
    KDirSortFilterProxyModel(parent),
    m_sorting(DolphinView::SortByName),
    m_sortOrder(Qt::AscendingOrder)
{
}

DolphinSortFilterProxyModel::~DolphinSortFilterProxyModel()
{
}

void DolphinSortFilterProxyModel::setSorting(DolphinView::Sorting sorting)
{
    m_sorting = sorting;
    KDirSortFilterProxyModel::sort(static_cast<int>(m_sorting), m_sortOrder);
}

void DolphinSortFilterProxyModel::setSortOrder(Qt::SortOrder sortOrder)
{
    m_sortOrder = sortOrder;
    KDirSortFilterProxyModel::sort(static_cast<int>(m_sorting), m_sortOrder);
}

void DolphinSortFilterProxyModel::setSortFoldersFirst(bool foldersFirst)
{
    if (foldersFirst != sortFoldersFirst()) {
        KDirSortFilterProxyModel::setSortFoldersFirst(foldersFirst);

        // We need to make sure that the files and folders are really resorted.
        // Without the following two lines, QSortFilterProxyModel::sort(int column, Qt::SortOrder order)
        // would do nothing because neither the column nor the sort order have changed.
        // TODO: remove this hack if we find a better way to force the ProxyModel to re-sort the data.
        const Qt::SortOrder tmpSortOrder = (m_sortOrder == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder);
        KDirSortFilterProxyModel::sort(static_cast<int>(m_sorting), tmpSortOrder);

        // Now comes the real sorting with the old column and sort order
        KDirSortFilterProxyModel::sort(static_cast<int>(m_sorting), m_sortOrder);
    }
}

void DolphinSortFilterProxyModel::sort(int column, Qt::SortOrder sortOrder)
{
    m_sorting = sortingForColumn(column);
    m_sortOrder = sortOrder;

    emit sortingRoleChanged();
    KDirSortFilterProxyModel::sort(static_cast<int>(m_sorting), sortOrder);
}

DolphinView::Sorting DolphinSortFilterProxyModel::sortingForColumn(int column)
{
    Q_ASSERT(column >= 0);
    Q_ASSERT(column <= DolphinView::MaxSortingEnum);
    return static_cast<DolphinView::Sorting>(column);
}

#include "dolphinsortfilterproxymodel.moc"
