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

#include "dolphintreeview.h"

#include "dolphinmodel.h"

#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

DolphinTreeView::DolphinTreeView(QWidget* parent) :
    QTreeView(parent),
    m_updateCurrentIndex(false),
    m_expandingTogglePressed(false),
    m_useDefaultIndexAt(true),
    m_ignoreScrollTo(false),
    m_dropRect(),
    m_band()
{
}

DolphinTreeView::~DolphinTreeView()
{
}

QRegion DolphinTreeView::visualRegionForSelection(const QItemSelection& selection) const
{
    // We have to make sure that the visualRect of each model index is inside the region.
    // QTreeView::visualRegionForSelection does not do it right because it assumes implicitly
    // that all visualRects have the same width, which is in general not the case here.
    QRegion selectionRegion;
    const QModelIndexList indexes = selection.indexes();

    foreach(const QModelIndex& index, indexes) {
        selectionRegion += visualRect(index);
    }

    return selectionRegion;
}

bool DolphinTreeView::acceptsDrop(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return false;
}

bool DolphinTreeView::event(QEvent* event)
{
    switch (event->type()) {
    case QEvent::Polish:
        m_useDefaultIndexAt = false;
        break;
    case QEvent::FocusOut:
        // If a key-press triggers an action that e. g. opens a dialog, the
        // widget gets no key-release event. Assure that the pressed state
        // is reset to prevent accidently setting the current index during a selection.
        m_updateCurrentIndex = false;
        break;
    default:
        break;
    }
    return QTreeView::event(event);
}

void DolphinTreeView::mousePressEvent(QMouseEvent* event)
{
    const QModelIndex current = currentIndex();
    QTreeView::mousePressEvent(event);

    m_expandingTogglePressed = isAboveExpandingToggle(event->pos());

    const QModelIndex index = indexAt(event->pos());
    const bool updateState = index.isValid() &&
                             (index.column() == DolphinModel::Name) &&
                             (event->button() == Qt::LeftButton);
    if (updateState) {
        setState(QAbstractItemView::DraggingState);
    }

    if (!index.isValid() || (index.column() != DolphinModel::Name)) {
        const Qt::KeyboardModifiers mod = QApplication::keyboardModifiers();
        if (!m_expandingTogglePressed && !(mod & Qt::ShiftModifier) && !(mod & Qt::ControlModifier)) {
            clearSelection();
        }

        // Restore the current index, other columns are handled as viewport area.
        // setCurrentIndex(...) implicitly calls scrollTo(...), which we want to ignore.
        m_ignoreScrollTo = true;
        selectionModel()->setCurrentIndex(current, QItemSelectionModel::Current);
        m_ignoreScrollTo = false;

        if ((event->button() == Qt::LeftButton) && !m_expandingTogglePressed) {
            // Inform Qt about what we are doing - otherwise it starts dragging items around!
            setState(DragSelectingState);
            m_band.show = true;
            // Incremental update data will not be useful - start from scratch.
            m_band.ignoreOldInfo = true;
            const QPoint scrollPos(horizontalScrollBar()->value(), verticalScrollBar()->value());
            m_band.origin = event->pos()  + scrollPos;
            m_band.destination = m_band.origin;
            m_band.originalSelection = selectionModel()->selection();
        }
    }
}

void DolphinTreeView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_expandingTogglePressed) {
        // Per default QTreeView starts either a selection or a drag operation when dragging
        // the expanding toggle button (Qt-issue - see TODO comment in DolphinIconsView::mousePressEvent()).
        // Turn off this behavior in Dolphin to stay predictable:
        setState(QAbstractItemView::NoState);
        return;
    }

    if (m_band.show) {
        const QPoint mousePos = event->pos();
        const QModelIndex index = indexAt(mousePos);
        if (!index.isValid()) {
            // The destination of the selection rectangle is above the viewport. In this
            // case QTreeView does no selection at all, which is not the wanted behavior
            // in Dolphin. Select all items within the elastic band rectangle.
            updateElasticBandSelection();
        }

        // TODO: Enable QTreeView::mouseMoveEvent(event) again, as soon
        // as the Qt-issue #199631 has been fixed.
        // QTreeView::mouseMoveEvent(event);
        QAbstractItemView::mouseMoveEvent(event);
        updateElasticBand();
    } else {
        // TODO: Enable QTreeView::mouseMoveEvent(event) again, as soon
        // as the Qt-issue #199631 has been fixed.
        // QTreeView::mouseMoveEvent(event);
        QAbstractItemView::mouseMoveEvent(event);
    }
}

void DolphinTreeView::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_expandingTogglePressed) {
        const QModelIndex index = indexAt(event->pos());
        if (index.isValid() && (index.column() == DolphinModel::Name)) {
            QTreeView::mouseReleaseEvent(event);
        } else {
            // don't change the current index if the cursor is released
            // above any other column than the name column, as the other
            // columns act as viewport
            const QModelIndex current = currentIndex();
            QTreeView::mouseReleaseEvent(event);
            selectionModel()->setCurrentIndex(current, QItemSelectionModel::Current);
        }
    }
    m_expandingTogglePressed = false;

    if (m_band.show) {
        setState(NoState);
        updateElasticBand();
        m_band.show = false;
    }
}

void DolphinTreeView::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions);
    m_band.show = false;
}

void DolphinTreeView::dragEnterEvent(QDragEnterEvent* event)
{
    Q_UNUSED(event);
    if (m_band.show) {
        updateElasticBand();
        m_band.show = false;
    }
}

void DolphinTreeView::dragMoveEvent(QDragMoveEvent* event)
{
    QTreeView::dragMoveEvent(event);

    setDirtyRegion(m_dropRect);

    const QModelIndex index = indexAt(event->pos());
    if (acceptsDrop(index)) {
        m_dropRect = visualRect(index);
    } else {
        m_dropRect.setSize(QSize()); // set invalid
    }
    setDirtyRegion(m_dropRect);
}

void DolphinTreeView::dragLeaveEvent(QDragLeaveEvent* event)
{
    QTreeView::dragLeaveEvent(event);
    setDirtyRegion(m_dropRect);
}

void DolphinTreeView::paintEvent(QPaintEvent* event)
{
    QTreeView::paintEvent(event);
    if (m_band.show) {
        // The following code has been taken from QListView
        // and adapted to DolphinDetailsView.
        // (C) 1992-2007 Trolltech ASA
        QStyleOptionRubberBand opt;
        opt.initFrom(this);
        opt.shape = QRubberBand::Rectangle;
        opt.opaque = false;
        opt.rect = elasticBandRect();

        QPainter painter(viewport());
        painter.save();
        style()->drawControl(QStyle::CE_RubberBand, &opt, &painter);
        painter.restore();
    }
}

void DolphinTreeView::keyPressEvent(QKeyEvent* event)
{
    // See DolphinTreeView::currentChanged() for more information about m_updateCurrentIndex
    m_updateCurrentIndex = (event->modifiers() == Qt::NoModifier);
    QTreeView::keyPressEvent(event);
}

void DolphinTreeView::keyReleaseEvent(QKeyEvent* event)
{
    QTreeView::keyReleaseEvent(event);
    m_updateCurrentIndex = false;
}

void DolphinTreeView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    QTreeView::currentChanged(current, previous);

    // Stay consistent with QListView: When changing the current index by key presses
    // without modifiers, also change the selection.
    if (m_updateCurrentIndex) {
        setCurrentIndex(current);
    }
}

QModelIndex DolphinTreeView::indexAt(const QPoint& point) const
{
    // The blank portion of the name column counts as empty space
    const QModelIndex index = QTreeView::indexAt(point);
    const bool isAboveEmptySpace  = !m_useDefaultIndexAt &&
                                    (index.column() == KDirModel::Name) &&
                                    !visualRect(index).contains(point);
    return isAboveEmptySpace ? QModelIndex() : index;
}

void DolphinTreeView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command)
{
    // We must override setSelection() as Qt calls it internally and when this happens
    // we must ensure that the default indexAt() is used.
    if (!m_band.show) {
        m_useDefaultIndexAt = true;
        QTreeView::setSelection(rect, command);
        m_useDefaultIndexAt = false;
    } else {
        // Use our own elastic band selection algorithm
        updateElasticBandSelection();
    }
}


void DolphinTreeView::scrollTo(const QModelIndex & index, ScrollHint hint)
{
    if (!m_ignoreScrollTo) {
        QTreeView::scrollTo(index, hint);
    }
}

void DolphinTreeView::updateElasticBandSelection()
{
    if (!m_band.show) {
        return;
    }

    // Ensure the elastic band itself is up-to-date, in
    // case we are being called due to e.g. a drag event.
    updateElasticBand();

    // Clip horizontally to the name column, as some filenames will be
    // longer than the column.  We don't clip vertically as origin
    // may be above or below the current viewport area.
    const int nameColumnX = header()->sectionPosition(DolphinModel::Name);
    const int nameColumnWidth = header()->sectionSize(DolphinModel::Name);
    QRect selRect = elasticBandRect().normalized();
    QRect nameColumnArea(nameColumnX, selRect.y(), nameColumnWidth, selRect.height());
    selRect = nameColumnArea.intersect(selRect).normalized();
    // Get the last elastic band rectangle, expressed in viewpoint coordinates.
    const QPoint scrollPos(horizontalScrollBar()->value(), verticalScrollBar()->value());
    QRect oldSelRect = QRect(m_band.lastSelectionOrigin - scrollPos, m_band.lastSelectionDestination - scrollPos).normalized();

    if (selRect.isNull()) {
        selectionModel()->select(m_band.originalSelection, QItemSelectionModel::ClearAndSelect);
        m_band.ignoreOldInfo = true;
        return;
    }

    if (!m_band.ignoreOldInfo) {
        // Do some quick checks to see if we can rule out the need to
        // update the selection.
        Q_ASSERT(uniformRowHeights());
        QModelIndex dummyIndex = model()->index(0, 0);
        if (!dummyIndex.isValid()) {
            // No items in the model presumably.
            return;
        }

        // If the elastic band does not cover the same rows as before, we'll
        // need to re-check, and also invalidate the old item distances.
        const int rowHeight = QTreeView::rowHeight(dummyIndex);
        const bool coveringSameRows =
            (selRect.top()    / rowHeight == oldSelRect.top()    / rowHeight) &&
            (selRect.bottom() / rowHeight == oldSelRect.bottom() / rowHeight);
        if (coveringSameRows) {
            // Covering the same rows, but have we moved far enough horizontally
            // that we might have (de)selected some other items?
            const bool itemSelectionChanged =
                ((selRect.left() > oldSelRect.left()) &&
                 (selRect.left() > m_band.insideNearestLeftEdge)) ||
                ((selRect.left() < oldSelRect.left()) &&
                 (selRect.left() <= m_band.outsideNearestLeftEdge)) ||
                ((selRect.right() < oldSelRect.right()) &&
                 (selRect.left() >= m_band.insideNearestRightEdge)) ||
                ((selRect.right() > oldSelRect.right()) &&
                 (selRect.right() >= m_band.outsideNearestRightEdge));

            if (!itemSelectionChanged) {
                return;
            }
        }
    } else {
        // This is the only piece of optimization data that needs to be explicitly
        // discarded.
        m_band.lastSelectionOrigin = QPoint();
        m_band.lastSelectionDestination = QPoint();
        oldSelRect = selRect;
    }

    // Do the selection from scratch. Force a update of the horizontal distances info.
    m_band.insideNearestLeftEdge   = nameColumnX + nameColumnWidth + 1;
    m_band.insideNearestRightEdge  = nameColumnX - 1;
    m_band.outsideNearestLeftEdge  = nameColumnX - 1;
    m_band.outsideNearestRightEdge = nameColumnX + nameColumnWidth + 1;

    // Include the old selection rect as well, so we can deselect
    // items that were inside it but not in the new selRect.
    const QRect boundingRect = selRect.united(oldSelRect).normalized();
    if (boundingRect.isNull()) {
        return;
    }

    // Get the index of the item in this row in the name column.
    // TODO - would this still work if the columns could be re-ordered?
    QModelIndex startIndex = QTreeView::indexAt(boundingRect.topLeft());
    if (startIndex.parent().isValid()) {
        startIndex = startIndex.parent().child(startIndex.row(), KDirModel::Name);
    } else {
        startIndex = model()->index(startIndex.row(), KDirModel::Name);
    }
    if (!startIndex.isValid()) {
        selectionModel()->select(m_band.originalSelection, QItemSelectionModel::ClearAndSelect);
        m_band.ignoreOldInfo = true;
        return;
    }

   // Go through all indexes between the top and bottom of boundingRect, and
   // update the selection.
   const int verticalCutoff = boundingRect.bottom();
   QModelIndex currIndex = startIndex;
   QModelIndex lastIndex;
   bool allItemsInBoundDone = false;

   // Calling selectionModel()->select(...) for each item that needs to be
   // toggled is slow as each call emits selectionChanged(...) so store them
   // and do the selection toggle in one batch.
   QItemSelection itemsToToggle;
   // QItemSelection's deal with continuous ranges of indexes better than
   // single indexes, so try to portion items that need to be toggled into ranges.
   bool formingToggleIndexRange = false;
   QModelIndex toggleIndexRangeBegin = QModelIndex();

   do {
       QRect currIndexRect = visualRect(currIndex);

        // Update some optimization info as we go.
       const int cr = currIndexRect.right();
       const int cl = currIndexRect.left();
       const int sl = selRect.left();
       const int sr = selRect.right();
       // "The right edge of the name is outside of the rect but nearer than m_outsideNearestLeft", etc
       if ((cr < sl && cr > m_band.outsideNearestLeftEdge)) {
           m_band.outsideNearestLeftEdge = cr;
       }
       if ((cl > sr && cl < m_band.outsideNearestRightEdge)) {
           m_band.outsideNearestRightEdge = cl;
       }
       if ((cl >= sl && cl <= sr && cl > m_band.insideNearestRightEdge)) {
           m_band.insideNearestRightEdge = cl;
       }
       if ((cr >= sl && cr <= sr && cr < m_band.insideNearestLeftEdge)) {
           m_band.insideNearestLeftEdge = cr;
       }

       bool currentlySelected = selectionModel()->isSelected(currIndex);
       bool originallySelected = m_band.originalSelection.contains(currIndex);
       bool intersectsSelectedRect = currIndexRect.intersects(selRect);
       bool shouldBeSelected = (intersectsSelectedRect && !originallySelected) || (!intersectsSelectedRect && originallySelected);
       bool needToToggleItem = (currentlySelected && !shouldBeSelected) || (!currentlySelected && shouldBeSelected);
       if (needToToggleItem && !formingToggleIndexRange) {
            toggleIndexRangeBegin = currIndex;
            formingToggleIndexRange = true;
       }

       // NOTE: indexBelow actually walks up and down expanded trees for us.
       QModelIndex nextIndex = indexBelow(currIndex);
       allItemsInBoundDone = !nextIndex.isValid() || currIndexRect.top() > verticalCutoff;

       const bool commitToggleIndexRange = formingToggleIndexRange &&
                                           (!needToToggleItem ||
                                            allItemsInBoundDone ||
                                            currIndex.parent() != toggleIndexRangeBegin.parent());
       if (commitToggleIndexRange) {
           formingToggleIndexRange = false;
            // If this is the last item in the bounds and it is also the beginning of a range,
            // don't toggle lastIndex - it will already have been dealt with.
           if (!allItemsInBoundDone || toggleIndexRangeBegin != currIndex) {
               itemsToToggle.select(toggleIndexRangeBegin, lastIndex);
           }
            // Need to start a new range immediately with currIndex?
           if (needToToggleItem) {
               toggleIndexRangeBegin = currIndex;
               formingToggleIndexRange = true;
           }
           if (allItemsInBoundDone && needToToggleItem) {
                // Toggle the very last item in the bounds.
               itemsToToggle.select(currIndex, currIndex);
           }
       }

       // Next item
       lastIndex = currIndex;
       currIndex = nextIndex;
    } while (!allItemsInBoundDone);


    selectionModel()->select(itemsToToggle, QItemSelectionModel::Toggle);

    m_band.lastSelectionOrigin = m_band.origin;
    m_band.lastSelectionDestination = m_band.destination;
    m_band.ignoreOldInfo = false;
}

void DolphinTreeView::updateElasticBand()
{
    if (m_band.show) {
        QRect dirtyRegion(elasticBandRect());
        const QPoint scrollPos(horizontalScrollBar()->value(), verticalScrollBar()->value());
        m_band.destination = viewport()->mapFromGlobal(QCursor::pos()) + scrollPos;
        // Going above the (logical) top-left of the view causes complications during selection;
        // we may as well prevent it.
        if (m_band.destination.y() < 0) {
            m_band.destination.setY(0);
        }
        if (m_band.destination.x() < 0) {
            m_band.destination.setX(0);
        }
        dirtyRegion = dirtyRegion.united(elasticBandRect());
        setDirtyRegion(dirtyRegion);
    }
}

QRect DolphinTreeView::elasticBandRect() const
{
    const QPoint scrollPos(horizontalScrollBar()->value(), verticalScrollBar()->value());

    const QPoint topLeft = m_band.origin - scrollPos;
    const QPoint bottomRight = m_band.destination - scrollPos;
    return QRect(topLeft, bottomRight).normalized();
}

bool DolphinTreeView::isAboveExpandingToggle(const QPoint& pos) const
{
    // QTreeView offers no public API to get the information whether an index has an
    // expanding toggle and what boundaries the toggle has. The following approach
    // also assumes a toggle for file items.
    if (itemsExpandable()) {
        const QModelIndex index = QTreeView::indexAt(pos);
        if (index.isValid() && (index.column() == KDirModel::Name)) {
            QRect rect = visualRect(index);
            const int toggleSize = rect.height();
            if (isRightToLeft()) {
                rect.moveRight(rect.right());
            } else {
                rect.moveLeft(rect.x() - toggleSize);
            }
            rect.setWidth(toggleSize);

            QStyleOption opt;
            opt.initFrom(this);
            opt.rect = rect;
            rect = style()->subElementRect(QStyle::SE_TreeViewDisclosureItem, &opt, this);

            return rect.contains(pos);
        }
    }
    return false;
}

DolphinTreeView::ElasticBand::ElasticBand() :
    show(false),
    origin(),
    destination(),
    lastSelectionOrigin(),
    lastSelectionDestination(),
    ignoreOldInfo(true),
    outsideNearestLeftEdge(0),
    outsideNearestRightEdge(0),
    insideNearestLeftEdge(0),
    insideNearestRightEdge(0)
{
}

#include "dolphintreeview.moc"
