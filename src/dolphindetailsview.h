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
#include <libdolphin_export.h>

class DolphinController;

/**
 * @brief Represents the details view which shows the name, size,
 *        date, permissions, owner and group of an item.
 *
 * The width of the columns is automatically adjusted in a way
 * that full available width of the view is used by stretching the width
 * of the name column.
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinDetailsView : public QTreeView
{
    Q_OBJECT

public:
    explicit DolphinDetailsView(QWidget* parent, DolphinController* controller);
    virtual ~DolphinDetailsView();

protected:
    virtual bool event(QEvent* event);
    virtual QStyleOptionViewItem viewOptions() const;
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void startDrag(Qt::DropActions supportedActions);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragLeaveEvent(QDragLeaveEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void resizeEvent(QResizeEvent* event);

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

    /**
     * Is invoked when the mouse cursor has entered an item. The controller
     * gets informed to emit the itemEntered() signal if the mouse cursor
     * is above the name column. Otherwise the controller gets informed
     * to emit the itemViewportEntered() signal (all other columns should
     * behave as viewport area).
     */
    void slotEntered(const QModelIndex& index);

    /**
     * Updates the destination \a m_elasticBandDestination from
     * the elastic band to the current mouse position and triggers
     * an update.
     */
    void updateElasticBand();

    /**
     * Returns the rectangle for the elastic band dependent from the
     * origin \a m_elasticBandOrigin, the current destination
     * \a m_elasticBandDestination and the viewport position.
     */
    QRect elasticBandRect() const;

    void zoomIn();
    void zoomOut();

    /**
     * Called by QTreeView when an item is activated (clicked or double-clicked)
     */
    void triggerItem(const QModelIndex& index);

    /**
     * Opens a context menu at the position \a pos and allows to
     * configure the visibility of the header columns.
     */
    void configureColumns(const QPoint& pos);

    void updateColumnVisibility();

private:
    bool isZoomInPossible() const;
    bool isZoomOutPossible() const;

    /**
     * Updates the size of the decoration dependent on the
     * icon size of the DetailsModeSettings. The controller
     * will get informed about possible zoom in/zoom out
     * operations.
     */
    void updateDecorationSize();

    /** Return the upper left position in pixels of the viewport content. */
    QPoint contentsPos() const;

    KFileItem itemForIndex(const QModelIndex& index) const;

    KFileItemDelegate::Information infoForColumn(int columnIndex) const;

    /**
     * Resizes all columns in a way to use the whole available width of the view.
     */
    void resizeColumns();

private:
    DolphinController* m_controller;

    QFont m_font;
    QSize m_decorationSize;

    bool m_dragging;   // TODO: remove this property when the issue #160611 is solved in Qt 4.4
    QRect m_dropRect;  // TODO: remove this property when the issue #160611 is solved in Qt 4.4

    bool m_showElasticBand;
    QPoint m_elasticBandOrigin;
    QPoint m_elasticBandDestination;
};

#endif
