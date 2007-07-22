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
#include <ksortfilterproxymodel.h>
#include <libdolphin_export.h>

/**
 * @brief Acts as proxy model for KDirModel to sort and filter
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
 * It is assured that directories are always sorted before files.
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinSortFilterProxyModel : public KSortFilterProxyModel
{
    Q_OBJECT

public:
    DolphinSortFilterProxyModel(QObject* parent = 0);
    virtual ~DolphinSortFilterProxyModel();

    void setSorting(DolphinView::Sorting sorting);
    inline DolphinView::Sorting sorting() const;

    void setSortOrder(Qt::SortOrder sortOrder);
    inline Qt::SortOrder sortOrder() const;

    /**
     * @reimplemented, @internal
     *
     * If the view 'forces' sorting order to change we will
     * notice now.
     */
    virtual void sort(int column,
                      Qt::SortOrder order = Qt::AscendingOrder);

    /** Reimplemented from QAbstractItemModel. Returns true for directories. */
    virtual bool hasChildren(const QModelIndex& parent = QModelIndex()) const;

    /** Reimplemented from QAbstractItemModel. Returns true for empty directories. */
    virtual bool canFetchMore(const QModelIndex& parent) const;

    /**
     * Helper method to get the DolphinView::Sorting type for a given
     * column \a column. If the column is smaller 0 or greater than the
     * available columns, DolphinView::SortByName is returned.
     */
    static DolphinView::Sorting sortingForColumn(int column);

    /** @see KSortFilterProxyModel::lessThanGeneralPurpose() */
    virtual bool lessThanGeneralPurpose(const QModelIndex &left,
                                        const QModelIndex &right) const;

protected:
    virtual bool lessThan(const QModelIndex& left,
                          const QModelIndex& right) const;

private:
    inline int naturalCompare(const QString& a, const QString& b) const;

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

private:
    DolphinView::Sorting m_sorting;
    Qt::SortOrder m_sortOrder;

    friend class DolphinItemCategorizer;
};

DolphinView::Sorting DolphinSortFilterProxyModel::sorting() const
{
    return m_sorting;
}

Qt::SortOrder DolphinSortFilterProxyModel::sortOrder() const
{
    return m_sortOrder;
}

int DolphinSortFilterProxyModel::naturalCompare(const QString& a, const QString& b) const
{
    return KDirSortFilterProxyModel::naturalCompare(a, b);
}

#endif
