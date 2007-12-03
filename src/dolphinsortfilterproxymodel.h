/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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
 * A natural sorting is done. This means that items like:
 * - item_10.png
 * - item_1.png
 * - item_2.png
 * are sorted like
 * - item_1.png
 * - item_2.png
 * - item_10.png
 *
 * @note It is NOT assured that directories are always sorted before files.
 *       For example, on a Nepomuk based sorting, it is possible to have a file
 *       rated with 10 stars, and a directory rated with 5 stars. The file will
 *       be shown before the directory.
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

    /**
     * @reimplemented, @internal
     *
     * If the view 'forces' sorting order to change we will
     * notice now.
     */
    virtual void sort(int column,
                      Qt::SortOrder order = Qt::AscendingOrder);

    /**
     * Helper method to get the DolphinView::Sorting type for a given
     * column \a column. If the column is smaller 0 or greater than the
     * available columns, DolphinView::SortByName is returned.
     */
    static DolphinView::Sorting sortingForColumn(int column);

    /**
     * This method is essential for the categorized view.
     * It does a basic sorting for finding out categories
     * and their order. The lessThan() method will be applied for
     * each category.
     *
     * The easy explanation is that not always folders go first.
     * Imagine we sort by rating. Categories will be created by 10 stars,
     * 9 stars, 8 stars, ... but a category with only a file rated by 10
     * will go before a category with a folder with rating 8.
     * That's the main reason for having the lessThanGeneralPurpose() method.
     */
    virtual int compareCategories(const QModelIndex &left,
                                  const QModelIndex &right) const;

signals:
    void sortingRoleChanged();

protected:
    virtual bool subSortLessThan(const QModelIndex& left,
                                 const QModelIndex& right) const;

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
