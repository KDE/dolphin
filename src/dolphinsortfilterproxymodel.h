/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz <peter.penz@gmx.at>             *
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

#ifndef DOLPHINSORTFILTERPROXYMODEL_H
#define DOLPHINSORTFILTERPROXYMODEL_H

#include <dolphinview.h>
#include <kdirsortfilterproxymodel.h>
#include <libdolphin_export.h>

/**
 * @brief Acts as proxy model for DolphinModel to sort and filter
 *        KFileItems.
 *
 * Per default a natural sorting is done. This means that items like:
 * - item_10.png
 * - item_1.png
 * - item_2.png
 * are sorted like
 * - item_1.png
 * - item_2.png
 * - item_10.png
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinSortFilterProxyModel : public KDirSortFilterProxyModel
{
    Q_OBJECT

public:
    DolphinSortFilterProxyModel(QObject* parent = 0);
    virtual ~DolphinSortFilterProxyModel();

    void setSorting(DolphinView::Sorting sorting);
    DolphinView::Sorting sorting() const;

    void setSortOrder(Qt::SortOrder sortOrder);
    Qt::SortOrder sortOrder() const;

    void setSortFoldersFirst(bool foldersFirst);

    /** @reimplemented */
    virtual void sort(int column,
                      Qt::SortOrder order = Qt::AscendingOrder);

    /**
     * Helper method to get the DolphinView::Sorting type for a given
     * column \a column. If the column is smaller 0 or greater than the
     * available columns, DolphinView::SortByName is returned.
     */
    static DolphinView::Sorting sortingForColumn(int column);

signals:
    void sortingRoleChanged();

private:
    DolphinView::Sorting m_sorting:16;
    Qt::SortOrder m_sortOrder:16;
};

inline DolphinView::Sorting DolphinSortFilterProxyModel::sorting() const
{
    return m_sorting;
}

inline Qt::SortOrder DolphinSortFilterProxyModel::sortOrder() const
{
    return m_sortOrder;
}

#endif
