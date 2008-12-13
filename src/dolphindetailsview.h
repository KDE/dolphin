/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
 *   Copyright (C) 2008 by Simon St. James (kdedevel@etotheipiplusone.com) *
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
class SelectionManager;
class DolphinViewAutoScroller;

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
    virtual void keyReleaseEvent(QKeyEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    virtual bool eventFilter(QObject* watched, QEvent* event);
    virtual QModelIndex indexAt (const QPoint& point) const;
    virtual void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command);
    virtual void scrollTo (const QModelIndex & index, ScrollHint hint = EnsureVisible);

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
     * Updates the destination \a destination from
     * the elastic band to the current mouse position and triggers
     * an update.
     */
    void updateElasticBand();

    /**
     * Returns the rectangle for the elastic band dependent from the
     * origin \a origin, the current destination
     * \a destination and the viewport position.
     */
    QRect elasticBandRect() const;

    void setZoomLevel(int level);

    void slotShowPreviewChanged();

    /**
     * Opens a context menu at the position \a pos and allows to
     * configure the visibility of the header columns.
     */
    void configureColumns(const QPoint& pos);

    void updateColumnVisibility();

    /**
     * Disables the automatical resizing of columns, if the user has resized the columns
     * with the mouse.
     */
    void slotHeaderSectionResized(int logicalIndex, int oldSize, int newSize);

    /**
     * Changes the alternating row colors setting depending from
     * the activation state \a active.
     */
    void slotActivationChanged(bool active);

    /**
     * Disables the automatical resizing of the columns. Per default all columns
     * are resized to use the maximum available width of the view as good as possible.
     */
    void disableAutoResizing();

    void requestActivation();

    void updateFont();

    /**
     * If the elastic band is currently shown, update the elastic band based on
     * the current mouse position and ensure that the selection is the set of items
     * intersecting it.
     */
    void updateElasticBandSelection();

private:
    /**
     * Updates the size of the decoration dependent on the
     * icon size of the DetailsModeSettings. The controller
     * will get informed about possible zoom in/zoom out
     * operations.
     */
    void updateDecorationSize(bool showPreview);

    /** Return the upper left position in pixels of the viewport content. */
    QPoint contentsPos() const;

    KFileItemDelegate::Information infoForColumn(int columnIndex) const;

    /**
     * Resizes all columns in a way to use the whole available width of the view.
     */
    void resizeColumns();

    QRect nameColumnRect(const QModelIndex& index) const;

private:
    bool m_autoResize : 1;        // if true, the columns are resized automatically to the available width
    bool m_expandingTogglePressed : 1;
    bool m_keyPressed : 1;        // true if a key is pressed currently; info used by currentChanged()
    bool m_useDefaultIndexAt : 1; // true, if QTreeView::indexAt() should be used
    bool m_ignoreScrollTo : 1;    // true if calls to scrollTo(...) should do nothing.

    DolphinController* m_controller;
    SelectionManager* m_selectionManager;
    DolphinViewAutoScroller* m_autoScroller;

    QFont m_font;
    QSize m_decorationSize;

    QRect m_dropRect;

    struct ElasticBand
    {
        ElasticBand();

        // Elastic band origin and destination coordinates are relative to t
        // he origin of the view, not the viewport.
        bool show;
        QPoint origin;
        QPoint destination;

        // Optimization mechanisms for use with elastic band selection.
        // Basically, allow "incremental" updates to the selection based
        // on the last elastic band shape.
        QPoint lastSelectionOrigin;
        QPoint lastSelectionDestination;

        // If true, compute the set of selected elements from scratch (slower)
        bool ignoreOldInfo;

        // Edges of the filenames that are closest to the edges of oldSelectionRect.
        // Used to decide whether horizontal changes in the elastic band are likely
        // to require us to re-check which items are selected.
        int outsideNearestLeftEdge;
        int outsideNearestRightEdge;
        int insideNearestLeftEdge;
        int insideNearestRightEdge;
        // The set of items that were selected at the time this band was shown.
        // NOTE: Unless CTRL was pressed when the band was created, this is always empty.
        QItemSelection originalSelection;
    } m_band;
};

#endif
