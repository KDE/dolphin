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
#include "kitemlistselectionmanager.h"

#include <QEvent>
#include <QGraphicsSceneEvent>
#include <QTransform>

#include <KDebug>

KItemListController::KItemListController(QObject* parent) :
    QObject(parent),
    m_selectionBehavior(NoSelection),
    m_model(0),
    m_view(0),
    m_selectionManager(new KItemListSelectionManager(this)),
    m_pressedIndex(-1)
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
    m_view = view;

    if (m_view) {
        m_view->setController(this);
        m_view->setModel(m_model);
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
    Q_UNUSED(event);
    return false;
}

bool KItemListController::inputMethodEvent(QInputMethodEvent* event)
{
    Q_UNUSED(event);
    return false;
}

bool KItemListController::mousePressEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform)
{
    const QPointF pos = transform.map(event->pos());
    m_pressedIndex = m_view->itemAt(pos);

    const bool shiftOrControlPressed = event->modifiers() & Qt::ShiftModifier ||
                                       event->modifiers() & Qt::ControlModifier;
    if (!shiftOrControlPressed) {
        m_selectionManager->clearSelection();
        m_selectionManager->setAnchorItem(m_pressedIndex);
    }

    if (m_pressedIndex >= 0) {
        m_selectionManager->setCurrentItem(m_pressedIndex);
        if (!m_view->isAboveExpansionToggle(m_pressedIndex, pos)) {
            m_selectionManager->setSelected(m_pressedIndex);
        }
    }

    return false;
}

bool KItemListController::mouseMoveEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform)
{
    Q_UNUSED(event);
    Q_UNUSED(transform);
    return false;
}

bool KItemListController::mouseReleaseEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform)
{
    if (m_view) {
        const QPointF pos = transform.map(event->pos());
        const int index = m_view->itemAt(pos);
        const bool shiftOrControlPressed = event->modifiers() & Qt::ShiftModifier || event->modifiers() & Qt::ControlModifier;

        if (index >= 0 && index == m_pressedIndex) {
            // The release event is done above the same item as the press event
            bool emitItemClicked = true;
            if (event->button() & Qt::LeftButton) {
                if (m_view->isAboveExpansionToggle(index, pos)) {
                    emit itemExpansionToggleClicked(index);
                    emitItemClicked = false;
                }
                else if (shiftOrControlPressed) {
                    // The mouse click should only update the selection, not trigger the item
                    emitItemClicked = false;
                }
            }

            if (emitItemClicked) {
                emit itemClicked(index, event->button());
            }
        } else if (!shiftOrControlPressed) {
            m_selectionManager->clearSelection();
        }
    }

    m_pressedIndex = -1;
    return false;
}

bool KItemListController::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform)
{
    Q_UNUSED(event);
    Q_UNUSED(transform);
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

#include "kitemlistcontroller.moc"
