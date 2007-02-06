/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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

#ifndef DOLPHINDETAILSVIEW_H
#define DOLPHINDETAILSVIEW_H

#include <dolphinview.h>
#include <QTreeView>

/**
 * @brief Represents the details view which shows the name, size,
 *        date, permissions, owner and group of an item.
 *
 * The width of the columns is automatically adjusted in a way
 * that full available width of the view is used by stretching the width
 * of the name column.
 */
class DolphinDetailsView : public QTreeView
{
    Q_OBJECT

public:
    explicit DolphinDetailsView(DolphinView* parent);
    virtual ~DolphinDetailsView();

protected:
    virtual bool event(QEvent* event);
    virtual QStyleOptionViewItem viewOptions() const;
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dropEvent(QDropEvent* event);

private slots:
    /**
     * Sets the sort indicator section of the header view
     * corresponding to \a sorting.
     */
    void setSortIndicatorSection(DolphinView::Sorting sorting);

    /**
     * Sets the sort indicator order of the header view
     * corresponding to \a sortOrder.
     */
    void setSortIndicatorOrder(Qt::SortOrder sortOrder);

    /**
     * Synchronizes the sorting state of the Dolphin menu 'View -> Sort'
     * with the current state of the details view.
     * @param column Index of the current sorting column.
     */
     void synchronizeSortingState(int column);

private:
    DolphinView* m_dolphinView;
};

#endif
