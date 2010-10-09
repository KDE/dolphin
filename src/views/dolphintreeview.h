/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2008 by Simon St. James <kdedevel@etotheipiplusone.com> *
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

#ifndef DOLPHINTREEVIEW_H
#define DOLPHINTREEVIEW_H

#include <QTreeView>
#include <libdolphin_export.h>

/**
 * @brief Extends QTreeView by a custom selection- and hover-mechanism.
 *
 * QTreeView does not respect the width of a cell for the hover-feedback
 * and when selecting items. DolphinTreeView improves this by respecting the
 * content-width of the first column. The selection-mechanism also
 * respects the content-width.
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit DolphinTreeView(QWidget* parent = 0);
    virtual ~DolphinTreeView();

    virtual QRegion visualRegionForSelection(const QItemSelection& selection) const;

protected:   
    /**
     * @return True, if the item with the index \p index accepts a drop. In this
     *         case a visual feedback for the user is given during dragging. Per
     *         default false is returned.
     */
    virtual bool acceptsDrop(const QModelIndex& index) const;

    virtual bool event(QEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void startDrag(Qt::DropActions supportedActions);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dragLeaveEvent(QDragLeaveEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);
    virtual void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    virtual QModelIndex indexAt (const QPoint& point) const;
    virtual void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command);
    virtual void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible);

private slots:
    /**
     * If the elastic band is currently shown, update the elastic band based on
     * the current mouse position and ensure that the selection is the set of items
     * intersecting it.
     */
    void updateElasticBandSelection();

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

private:
    /**
     * Returns true, if \a pos is within the expanding toggle of a tree.
     */
    bool isAboveExpandingToggle(const QPoint& pos) const;

private:
    bool m_keyPressed;        // true if a key is pressed currently; info used by currentChanged()
    bool m_expandingTogglePressed;
    bool m_useDefaultIndexAt; // true, if QTreeView::indexAt() should be used
    bool m_ignoreScrollTo;    // true if calls to scrollTo(...) should do nothing.

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
