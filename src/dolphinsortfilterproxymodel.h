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

#include <QSortFilterProxyModel>
#include <dolphinview.h>

/**
 * @brief Acts as proxy model for KDirModel to sort and filter
 *        KFileItems.
 *
 * TODO: implementation does not work yet (the method lessThan is
 * not invoked)
 */
class DolphinSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    DolphinSortFilterProxyModel(QObject* parent = 0);
    virtual ~DolphinSortFilterProxyModel();

    void setSorting(DolphinView::Sorting sorting);
    DolphinView::Sorting sorting() const { return m_sorting; }

    void setSortOrder(Qt::SortOrder sortOrder);
    Qt::SortOrder sortOrder() const { return m_sortOrder; }

protected:
    virtual bool lessThan(const QModelIndex& left,
                          const QModelIndex& right) const;

private:
    DolphinView::Sorting m_sorting;
    Qt::SortOrder m_sortOrder;
};

#endif
