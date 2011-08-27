/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs:                *
 *   http://qt.gitorious.org/qt-labs/itemviews-ng                          *
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

#include "kitemlistcontroller.h"

#include "kitemlistview.h"
#include "kitemlistrubberband_p.h"
#include "kitemlistselectionmanager.h"

#include <QApplication>
#include <QDrag>
#include <QEvent>
#include <QGraphicsSceneEvent>
#include <QMimeData>

#include <KGlobalSettings>
#include <KDebug>

KItemListController::KItemListController(QObject* parent) :
    QObject(parent),
    m_dragging(false),
    m_selectionBehavior(NoSelection),
    m_model(0),
    m_view(0),
    m_selectionManager(new KItemListSelectionManager(this)),
    m_pressedIndex(-1),
    m_pressedMousePos(),
    m_oldSelection()
{
}

KItemListController::~KItemListController()
{
}

void KItemListController::setModel(KItemModelBase* model)
{
    if (m_model == model) {
        return;
    }

    KItemModelBase* oldModel = m_model;
    m_model = model;

    if (m_view) {
        m_view->setModel(m_model);
    }

    m_selectionManager->setModel(m_model);

    emit modelChanged(m_model, oldModel);
}

KItemModelBase* KItemListController::model() const
{
    return m_model;
}

KItemListSelectionManager* KItemListController::selectionManager() const
{
    return m_selectionManager;
}

void KItemListController::setView(KItemListView* view)
{
    if (m_view == view) {
        return;
    }

    KItemListView* oldView = m_view;
    if (oldView) {
        disconnect(oldView, SIGNAL(offsetChanged(qreal,qreal)), this, SLOT(slotViewOffsetChanged(qreal,qreal)));
    }

    m_view = view;

    if (m_view) {
        m_view->setController(this);
        m_view->setModel(m_model);
        connect(m_view, SIGNAL(offsetChanged(qreal,qreal)), this, SLOT(slotViewOffsetChanged(qreal,qreal)));
    }

    emit viewChanged(m_view, oldView);
}

KItemListView* KItemListController::view() const
{
    return m_view;
}

void KItemListController::setSelectionBehavior(SelectionBehavior behavior)
{
    m_selectionBehavior = behavior;
}

KItemListController::SelectionBehavior KItemListController::selectionBehavior() const
{
    return m_selectionBehavior;
}

bool KItemListController::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    return false;
}

bool KItemListController::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    return false;
}

bool KItemListController::keyPressEvent(QKeyEvent* event)
{
    const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
    const bool controlPressed = event->modifiers() & Qt::ControlModifier;
    const bool shiftOrControlPressed = shiftPressed || controlPressed;

    int index = m_selectionManager->currentItem();
    const int itemCount = m_model->count();
    const int itemsPerRow = m_view->itemsPerOffset();

    // For horizontal scroll orientation, transform
    // the arrow keys to simplify the event handling.
    int key = event->key();
    if (m_view->scrollOrientation() == Qt::Horizontal) {
        switch (key) {
        case Qt::Key_Up:    key = Qt::Key_Left; break;
        case Qt::Key_Down:  key = Qt::Key_Right; break;
        case Qt::Key_Left:  key = Qt::Key_Up; break;
        case Qt::Key_Right: key = Qt::Key_Down; break;
        default:            break;
        }
    }

    switch (key) {
    case Qt::Key_Home:
        index = 0;
        break;

    case Qt::Key_End:
        index = itemCount - 1;
        break;

    case Qt::Key_Left:
        if (index > 0) {
            index--;
        }
        break;

    case Qt::Key_Right:
        if (index < itemCount - 1) {
            index++;
        }
        break;

    case Qt::Key_Up:
        if (index >= itemsPerRow) {
            index -= itemsPerRow;
        }
        break;

    case Qt::Key_Down:
        if (index + itemsPerRow < itemCount) {
            // We are not in the last row yet.
            index += itemsPerRow;
        }
        else {
            // We are either in the last row already, or we are in the second-last row,
            // and there is no item below the current item.
            // In the latter case, we jump to the very last item.
            const int currentColumn = index % itemsPerRow;
            const int lastItemColumn = (itemCount - 1) % itemsPerRow;
            const bool inLastRow = currentColumn < lastItemColumn;
            if (!inLastRow) {
                index = itemCount - 1;
            }
        }
        break;

    case Qt::Key_Space:
        if (controlPressed) {
            m_selectionManager->endAnchoredSelection();
            m_selectionManager->setSelected(index, 1, KItemListSelectionManager::Toggle);
            m_selectionManager->beginAnchoredSelection(index);
        }

    default:
        break;
    }

    if (m_selectionManager->currentItem() != index) {
        if (controlPressed) {
            m_selectionManager->endAnchoredSelection();
        }

        m_selectionManager->setCurrentItem(index);

        if (!shiftOrControlPressed || m_selectionBehavior == SingleSelection) {
            m_selectionManager->clearSelection();
            m_selectionManager->setSelected(index, 1);
        }

        if (!shiftPressed) {
            m_selectionManager->beginAnchoredSelection(index);
        }
    }
    return true;
}

bool KItemListController::inputMethodEvent(QInputMethodEvent* event)
{
    Q_UNUSED(event);
    return false;
}

bool KItemListController::mousePressEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform)
{
    if (!m_view) {
        return false;
    }

    m_dragging = false;
    m_pressedMousePos = transform.map(event->pos());
    m_pressedIndex = m_view->itemAt(m_pressedMousePos);

    if (m_view->isAboveExpansionToggle(m_pressedIndex, m_pressedMousePos)) {
        return true;
    }

    const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
    const bool controlPressed = event->modifiers() & Qt::ControlModifier;

    if (m_selectionBehavior == SingleSelection) {
        m_selectionManager->clearSelection();
    }

    if (!shiftPressed) {
        // Finish the anchored selection before the current index is changed
        m_selectionManager->endAnchoredSelection();
    }

    if (m_pressedIndex >= 0) {
        m_selectionManager->setCurrentItem(m_pressedIndex);

        switch (m_selectionBehavior) {
        case NoSelection:
            break;

        case SingleSelection:
            m_selectionManager->setSelected(m_pressedIndex);
            break;

        case MultiSelection:
            if (controlPressed) {
                m_selectionManager->setSelected(m_pressedIndex, 1, KItemListSelectionManager::Toggle);
                m_selectionManager->beginAnchoredSelection(m_pressedIndex);
            } else if (!shiftPressed || !m_selectionManager->isAnchoredSelectionActive()) {
                // Select the pressed item and start a new anchored selection
                m_selectionManager->setSelected(m_pressedIndex, 1, KItemListSelectionManager::Select);
                m_selectionManager->beginAnchoredSelection(m_pressedIndex);
            }
            break;

        default:
            Q_ASSERT(false);
            break;
        }

        return true;
    } else {
        KItemListRubberBand* rubberBand = m_view->rubberBand();
        QPointF startPos = m_pressedMousePos;
        if (m_view->scrollOrientation() == Qt::Vertical) {
            startPos.ry() += m_view->offset();
            if (m_view->itemSize().width() < 0) {
                // Use a special rubberband for views that have only one column and
                // expand the rubberband to use the whole width of the view.
                startPos.setX(0);
            }
        } else {
            startPos.rx() += m_view->offset();
        }

        m_oldSelection = m_selectionManager->selectedItems();
        rubberBand->setStartPosition(startPos);
        rubberBand->setEndPosition(startPos);
        rubberBand->setActive(true);
        connect(rubberBand, SIGNAL(endPositionChanged(QPointF,QPointF)), this, SLOT(slotRubberBandChanged()));
        m_view->setAutoScroll(true);
    }

    return false;
}

bool KItemListController::mouseMoveEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform)
{
    if (!m_view) {
        return false;
    }

    if (m_pressedIndex >= 0) {
        // Check whether a dragging should be started
        if (!m_dragging && (event->buttons() & Qt::LeftButton)) {
            const QPointF pos = transform.map(event->pos());
            const qreal minDragDiff = 4;
            const bool hasMinDragDiff = qAbs(pos.x() - m_pressedMousePos.x()) >= minDragDiff ||
                                        qAbs(pos.y() - m_pressedMousePos.y()) >= minDragDiff;
            if (hasMinDragDiff && startDragging()) {
                m_dragging = true;
            }
        }
    } else {
        KItemListRubberBand* rubberBand = m_view->rubberBand();
        if (rubberBand->isActive()) {
            QPointF endPos = transform.map(event->pos());
            if (m_view->scrollOrientation() == Qt::Vertical) {
                endPos.ry() += m_view->offset();
                if (m_view->itemSize().width() < 0) {
                    // Use a special rubberband for views that have only one column and
                    // expand the rubberband to use the whole width of the view.
                    endPos.setX(m_view->size().width());
                }
            } else {
                endPos.rx() += m_view->offset();
            }
            rubberBand->setEndPosition(endPos);
        }
    }

    return false;
}

bool KItemListController::mouseReleaseEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform)
{
    if (!m_view) {
        return false;
    }

    const bool shiftOrControlPressed = event->modifiers() & Qt::ShiftModifier ||
                                       event->modifiers() & Qt::ControlModifier;

    bool clearSelection = !shiftOrControlPressed && !m_dragging && !(event->button() == Qt::RightButton);

    KItemListRubberBand* rubberBand = m_view->rubberBand();
    if (rubberBand->isActive()) {
        disconnect(rubberBand, SIGNAL(endPositionChanged(QPointF,QPointF)), this, SLOT(slotRubberBandChanged()));
        rubberBand->setActive(false);
        m_oldSelection.clear();
        m_view->setAutoScroll(false);

        if (rubberBand->startPosition() != rubberBand->endPosition()) {
            clearSelection = false;
        }
    }

    const QPointF pos = transform.map(event->pos());
    const int index = m_view->itemAt(pos);

    if (index >= 0 && index == m_pressedIndex) {
        // The release event is done above the same item as the press event

        if (clearSelection) {
            // Clear the previous selection but reselect the current index
            m_selectionManager->setSelectedItems(QSet<int>() << index);
        }

        bool emitItemClicked = true;
        if (event->button() & Qt::LeftButton) {
            if (m_view->isAboveExpansionToggle(index, pos)) {
                emit itemExpansionToggleClicked(index);
                emitItemClicked = false;
            } else if (shiftOrControlPressed || !KGlobalSettings::singleClick()) {
                emitItemClicked = false;
            }
        }

        if (emitItemClicked) {
            emit itemClicked(index, event->button());
        }
    } else if (clearSelection) {
        m_selectionManager->clearSelection();
    }

    m_dragging = false;
    m_pressedMousePos = QPointF();
    m_pressedIndex = -1;
    return false;
}

bool KItemListController::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform)
{
    const QPointF pos = transform.map(event->pos());
    const int index = m_view->itemAt(pos);

    bool emitItemClicked = !KGlobalSettings::singleClick() &&
                           (event->button() & Qt::LeftButton) &&
                           index >= 0 && index < m_model->count();
    if (emitItemClicked) {
        emit itemClicked(index, event->button());
    }
    return false;
}

bool KItemListController::dragEnterEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform)
{
    Q_UNUSED(event);
    Q_UNUSED(transform);
    return false;
}

bool KItemListController::dragLeaveEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform)
{
    Q_UNUSED(event);
    Q_UNUSED(transform);
    return false;
}

bool KItemListController::dragMoveEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform)
{
    Q_UNUSED(event);
    Q_UNUSED(transform);
    return false;
}

bool KItemListController::dropEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform)
{
    Q_UNUSED(event);
    Q_UNUSED(transform);

    m_dragging = false;
    return false;
}

bool KItemListController::hoverEnterEvent(QGraphicsSceneHoverEvent* event, const QTransform& transform)
{
    Q_UNUSED(event);
    Q_UNUSED(transform);
    return false;
}

bool KItemListController::hoverMoveEvent(QGraphicsSceneHoverEvent* event, const QTransform& transform)
{
    // The implementation assumes that only one item can get hovered no matter
    // whether they overlap or not.

    Q_UNUSED(transform);
    if (!m_model || !m_view) {
        return false;
    }

    // Search the previously hovered item that might get unhovered
    KItemListWidget* unhoveredWidget = 0;
    foreach (KItemListWidget* widget, m_view->visibleItemListWidgets()) {
        if (widget->isHovered()) {
            unhoveredWidget = widget;
            break;
        }
    }

    // Search the currently hovered item
    KItemListWidget* hoveredWidget = 0;
    foreach (KItemListWidget* widget, m_view->visibleItemListWidgets()) {
        const QPointF mappedPos = widget->mapFromItem(m_view, event->pos());

        const bool hovered = widget->contains(mappedPos) &&
                             !widget->expansionToggleRect().contains(mappedPos) &&
                             !widget->selectionToggleRect().contains(mappedPos);
        if (hovered) {
            hoveredWidget = widget;
            break;
        }
    }

    if (unhoveredWidget != hoveredWidget) {
        if (unhoveredWidget) {
            unhoveredWidget->setHovered(false);
            emit itemUnhovered(unhoveredWidget->index());
        }

        if (hoveredWidget) {
            hoveredWidget->setHovered(true);
            emit itemHovered(hoveredWidget->index());
        }
    }

    return false;
}

bool KItemListController::hoverLeaveEvent(QGraphicsSceneHoverEvent* event, const QTransform& transform)
{
    Q_UNUSED(event);
    Q_UNUSED(transform);

    if (!m_model || !m_view) {
        return false;
    }

    foreach (KItemListWidget* widget, m_view->visibleItemListWidgets()) {
        if (widget->isHovered()) {
            widget->setHovered(false);
            emit itemUnhovered(widget->index());
        }
    }
    return false;
}

bool KItemListController::wheelEvent(QGraphicsSceneWheelEvent* event, const QTransform& transform)
{
    Q_UNUSED(event);
    Q_UNUSED(transform);
    return false;
}

bool KItemListController::resizeEvent(QGraphicsSceneResizeEvent* event, const QTransform& transform)
{
    Q_UNUSED(event);
    Q_UNUSED(transform);
    return false;
}

bool KItemListController::processEvent(QEvent* event, const QTransform& transform)
{
    if (!event) {
        return false;
    }

    switch (event->type()) {
    case QEvent::KeyPress:
        return keyPressEvent(static_cast<QKeyEvent*>(event));
    case QEvent::InputMethod:
        return inputMethodEvent(static_cast<QInputMethodEvent*>(event));
    case QEvent::GraphicsSceneMousePress:
        return mousePressEvent(static_cast<QGraphicsSceneMouseEvent*>(event), QTransform());
    case QEvent::GraphicsSceneMouseMove:
        return mouseMoveEvent(static_cast<QGraphicsSceneMouseEvent*>(event), QTransform());
    case QEvent::GraphicsSceneMouseRelease:
        return mouseReleaseEvent(static_cast<QGraphicsSceneMouseEvent*>(event), QTransform());
    case QEvent::GraphicsSceneMouseDoubleClick:
        return mouseDoubleClickEvent(static_cast<QGraphicsSceneMouseEvent*>(event), QTransform());
    case QEvent::GraphicsSceneWheel:
         return wheelEvent(static_cast<QGraphicsSceneWheelEvent*>(event), QTransform());
    case QEvent::GraphicsSceneDragEnter:
        return dragEnterEvent(static_cast<QGraphicsSceneDragDropEvent*>(event), QTransform());
    case QEvent::GraphicsSceneDragLeave:
        return dragLeaveEvent(static_cast<QGraphicsSceneDragDropEvent*>(event), QTransform());
    case QEvent::GraphicsSceneDragMove:
        return dragMoveEvent(static_cast<QGraphicsSceneDragDropEvent*>(event), QTransform());
    case QEvent::GraphicsSceneDrop:
        return dropEvent(static_cast<QGraphicsSceneDragDropEvent*>(event), QTransform());
    case QEvent::GraphicsSceneHoverEnter:
        return hoverEnterEvent(static_cast<QGraphicsSceneHoverEvent*>(event), QTransform());
    case QEvent::GraphicsSceneHoverMove:
        return hoverMoveEvent(static_cast<QGraphicsSceneHoverEvent*>(event), QTransform());
    case QEvent::GraphicsSceneHoverLeave:
        return hoverLeaveEvent(static_cast<QGraphicsSceneHoverEvent*>(event), QTransform());
    case QEvent::GraphicsSceneResize:
        return resizeEvent(static_cast<QGraphicsSceneResizeEvent*>(event), transform);
    default:
        break;
    }

    return false;
}

void KItemListController::slotViewOffsetChanged(qreal current, qreal previous)
{
    if (!m_view) {
        return;
    }

    KItemListRubberBand* rubberBand = m_view->rubberBand();
    if (rubberBand->isActive()) {
        const qreal diff = current - previous;
        // TODO: Ideally just QCursor::pos() should be used as
        // new end-position but it seems there is no easy way
        // to have something like QWidget::mapFromGlobal() for QGraphicsWidget
        // (... or I just missed an easy way to do the mapping)
        QPointF endPos = rubberBand->endPosition();
        if (m_view->scrollOrientation() == Qt::Vertical) {
            endPos.ry() += diff;
        } else {
            endPos.rx() += diff;
        }

        rubberBand->setEndPosition(endPos);
    }
}

void KItemListController::slotRubberBandChanged()
{
    if (!m_view || !m_model || m_model->count() <= 0) {
        return;
    }

    const KItemListRubberBand* rubberBand = m_view->rubberBand();
    const QPointF startPos = rubberBand->startPosition();
    const QPointF endPos = rubberBand->endPosition();
    QRectF rubberBandRect = QRectF(startPos, endPos).normalized();

    const bool scrollVertical = (m_view->scrollOrientation() == Qt::Vertical);
    if (scrollVertical) {
        rubberBandRect.translate(0, -m_view->offset());
    } else {
        rubberBandRect.translate(-m_view->offset(), 0);
    }

    if (!m_oldSelection.isEmpty()) {
        // Clear the old selection that was available before the rubberband has
        // been activated in case if no Shift- or Control-key are pressed
        const bool shiftOrControlPressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ||
                                           QApplication::keyboardModifiers() & Qt::ControlModifier;
        if (!shiftOrControlPressed) {
            m_oldSelection.clear();
        }
    }

    QSet<int> selectedItems;

    // Select all visible items that intersect with the rubberband
    foreach (const KItemListWidget* widget, m_view->visibleItemListWidgets()) {
        const int index = widget->index();

        const QRectF widgetRect = m_view->itemBoundingRect(index);
        if (widgetRect.intersects(rubberBandRect)) {
            const QRectF iconRect = widget->iconBoundingRect().translated(widgetRect.topLeft());
            const QRectF textRect = widget->textBoundingRect().translated(widgetRect.topLeft());
            if (iconRect.intersects(rubberBandRect) || textRect.intersects(rubberBandRect)) {
                selectedItems.insert(index);
            }
        }
    }

    // Select all invisible items that intersect with the rubberband. Instead of
    // iterating all items only the area which might be touched by the rubberband
    // will be checked.
    const bool increaseIndex = scrollVertical ?
                               startPos.y() > endPos.y(): startPos.x() > endPos.x();

    int index = increaseIndex ? m_view->lastVisibleIndex() + 1 : m_view->firstVisibleIndex() - 1;
    bool selectionFinished = false;
    do {
        const QRectF widgetRect = m_view->itemBoundingRect(index);
        if (widgetRect.intersects(rubberBandRect)) {
            selectedItems.insert(index);
        }

        if (increaseIndex) {
            ++index;
            selectionFinished = (index >= m_model->count()) ||
                                ( scrollVertical && widgetRect.top()  > rubberBandRect.bottom()) ||
                                (!scrollVertical && widgetRect.left() > rubberBandRect.right());
        } else {
            --index;
            selectionFinished = (index < 0) ||
                                ( scrollVertical && widgetRect.bottom() < rubberBandRect.top()) ||
                                (!scrollVertical && widgetRect.right()  < rubberBandRect.left());
        }
    } while (!selectionFinished);

    m_selectionManager->setSelectedItems(selectedItems + m_oldSelection);
}

bool KItemListController::startDragging()
{
    if (!m_view || !m_model) {
        return false;
    }

    const QSet<int> selectedItems = m_selectionManager->selectedItems();
    QMimeData* data = m_model->createMimeData(selectedItems);
    if (!data) {
        return false;
    }

    // The created drag object will be owned and deleted
    // by QApplication::activeWindow().
    QDrag* drag = new QDrag(QApplication::activeWindow());
    drag->setMimeData(data);

    const QPixmap pixmap = m_view->createDragPixmap(selectedItems);
    drag->setPixmap(pixmap);

    drag->exec(Qt::MoveAction | Qt::CopyAction | Qt::LinkAction, Qt::IgnoreAction);
    return true;
}

#include "kitemlistcontroller.moc"
