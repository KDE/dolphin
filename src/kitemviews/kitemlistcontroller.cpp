/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2012 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistcontroller.h"

#include "kitemlistselectionmanager.h"
#include "kitemlistview.h"
#include "private/kitemlistkeyboardsearchmanager.h"
#include "private/kitemlistrubberband.h"
#include "private/ktwofingerswipe.h"
#include "private/ktwofingertap.h"
#include "views/draganddrophelper.h"

#include <QAccessible>
#include <QApplication>
#include <QDrag>
#include <QGesture>
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QGraphicsView>
#include <QMimeData>
#include <QTimer>
#include <QTouchEvent>

KItemListController::KItemListController(KItemModelBase* model, KItemListView* view, QObject* parent) :
    QObject(parent),
    m_singleClickActivationEnforced(false),
    m_selectionTogglePressed(false),
    m_clearSelectionIfItemsAreNotDragged(false),
    m_isSwipeGesture(false),
    m_dragActionOrRightClick(false),
    m_scrollerIsScrolling(false),
    m_pinchGestureInProgress(false),
    m_mousePress(false),
    m_isTouchEvent(false),
    m_selectionBehavior(NoSelection),
    m_autoActivationBehavior(ActivationAndExpansion),
    m_mouseDoubleClickAction(ActivateItemOnly),
    m_model(nullptr),
    m_view(nullptr),
    m_selectionManager(new KItemListSelectionManager(this)),
    m_keyboardManager(new KItemListKeyboardSearchManager(this)),
    m_pressedIndex(-1),
    m_pressedMousePos(),
    m_autoActivationTimer(nullptr),
    m_swipeGesture(Qt::CustomGesture),
    m_twoFingerTapGesture(Qt::CustomGesture),
    m_oldSelection(),
    m_keyboardAnchorIndex(-1),
    m_keyboardAnchorPos(0)
{
    connect(m_keyboardManager, &KItemListKeyboardSearchManager::changeCurrentItem,
            this, &KItemListController::slotChangeCurrentItem);
    connect(m_selectionManager, &KItemListSelectionManager::currentChanged,
            m_keyboardManager, &KItemListKeyboardSearchManager::slotCurrentChanged);
    connect(m_selectionManager, &KItemListSelectionManager::selectionChanged,
            m_keyboardManager, &KItemListKeyboardSearchManager::slotSelectionChanged);

    m_autoActivationTimer = new QTimer(this);
    m_autoActivationTimer->setSingleShot(true);
    m_autoActivationTimer->setInterval(-1);
    connect(m_autoActivationTimer, &QTimer::timeout, this, &KItemListController::slotAutoActivationTimeout);

    setModel(model);
    setView(view);

    m_swipeGesture = QGestureRecognizer::registerRecognizer(new KTwoFingerSwipeRecognizer());
    m_twoFingerTapGesture = QGestureRecognizer::registerRecognizer(new KTwoFingerTapRecognizer());
    view->grabGesture(m_swipeGesture);
    view->grabGesture(m_twoFingerTapGesture);
    view->grabGesture(Qt::TapGesture);
    view->grabGesture(Qt::TapAndHoldGesture);
    view->grabGesture(Qt::PinchGesture);
}

KItemListController::~KItemListController()
{
    setView(nullptr);
    Q_ASSERT(!m_view);

    setModel(nullptr);
    Q_ASSERT(!m_model);
}

void KItemListController::setModel(KItemModelBase* model)
{
    if (m_model == model) {
        return;
    }

    KItemModelBase* oldModel = m_model;
    if (oldModel) {
        oldModel->deleteLater();
    }

    m_model = model;
    if (m_model) {
        m_model->setParent(this);
    }

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
        disconnect(oldView, &KItemListView::scrollOffsetChanged, this, &KItemListController::slotViewScrollOffsetChanged);
        oldView->deleteLater();
    }

    m_view = view;

    if (m_view) {
        m_view->setParent(this);
        m_view->setController(this);
        m_view->setModel(m_model);
        connect(m_view, &KItemListView::scrollOffsetChanged, this, &KItemListController::slotViewScrollOffsetChanged);
        updateExtendedSelectionRegion();
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
    updateExtendedSelectionRegion();
}

KItemListController::SelectionBehavior KItemListController::selectionBehavior() const
{
    return m_selectionBehavior;
}

void KItemListController::setAutoActivationBehavior(AutoActivationBehavior behavior)
{
    m_autoActivationBehavior = behavior;
}

KItemListController::AutoActivationBehavior KItemListController::autoActivationBehavior() const
{
    return m_autoActivationBehavior;
}

void KItemListController::setMouseDoubleClickAction(MouseDoubleClickAction action)
{
    m_mouseDoubleClickAction = action;
}

KItemListController::MouseDoubleClickAction KItemListController::mouseDoubleClickAction() const
{
    return m_mouseDoubleClickAction;
}

int KItemListController::indexCloseToMousePressedPosition() const
{
    QHashIterator<KItemListWidget*, KItemListGroupHeader*> it(m_view->m_visibleGroups);
    while (it.hasNext()) {
        it.next();
        KItemListGroupHeader *groupHeader = it.value();
        const QPointF mappedToGroup = groupHeader->mapFromItem(nullptr, m_pressedMousePos);
        if (groupHeader->contains(mappedToGroup)) {
            return it.key()->index();
        }
    }
    return -1;
}

void KItemListController::setAutoActivationDelay(int delay)
{
    m_autoActivationTimer->setInterval(delay);
}

int KItemListController::autoActivationDelay() const
{
    return m_autoActivationTimer->interval();
}

void KItemListController::setSingleClickActivationEnforced(bool singleClick)
{
    m_singleClickActivationEnforced = singleClick;
}

bool KItemListController::singleClickActivationEnforced() const
{
    return m_singleClickActivationEnforced;
}

bool KItemListController::keyPressEvent(QKeyEvent* event)
{
    int index = m_selectionManager->currentItem();
    int key = event->key();

    // Handle the expanding/collapsing of items
    if (m_view->supportsItemExpanding() && m_model->isExpandable(index)) {
        if (key == Qt::Key_Right) {
            if (m_model->setExpanded(index, true)) {
                return true;
            }
        } else if (key == Qt::Key_Left) {
            if (m_model->setExpanded(index, false)) {
                return true;
            }
        }
    }

    const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
    const bool controlPressed = event->modifiers() & Qt::ControlModifier;
    const bool shiftOrControlPressed = shiftPressed || controlPressed;
    const bool navigationPressed = key == Qt::Key_Home || key == Qt::Key_End  ||
                                   key == Qt::Key_PageUp || key == Qt::Key_PageDown ||
                                   key == Qt::Key_Up || key == Qt::Key_Down ||
                                   key == Qt::Key_Left || key == Qt::Key_Right;

    const int itemCount = m_model->count();

    // For horizontal scroll orientation, transform
    // the arrow keys to simplify the event handling.
    if (m_view->scrollOrientation() == Qt::Horizontal) {
        switch (key) {
        case Qt::Key_Up:    key = Qt::Key_Left; break;
        case Qt::Key_Down:  key = Qt::Key_Right; break;
        case Qt::Key_Left:  key = Qt::Key_Up; break;
        case Qt::Key_Right: key = Qt::Key_Down; break;
        default:            break;
        }
    }

    const bool selectSingleItem = m_selectionBehavior != NoSelection && itemCount == 1 && navigationPressed;

    if (selectSingleItem) {
        const int current = m_selectionManager->currentItem();
        m_selectionManager->setSelected(current);
        return true;
    }

    switch (key) {
    case Qt::Key_Home:
        index = 0;
        m_keyboardAnchorIndex = index;
        m_keyboardAnchorPos = keyboardAnchorPos(index);
        break;

    case Qt::Key_End:
        index = itemCount - 1;
        m_keyboardAnchorIndex = index;
        m_keyboardAnchorPos = keyboardAnchorPos(index);
        break;

    case Qt::Key_Left:
        if (index > 0) {
            const int expandedParentsCount = m_model->expandedParentsCount(index);
            if (expandedParentsCount == 0) {
                --index;
            } else {
                // Go to the parent of the current item.
                do {
                    --index;
                } while (index > 0 && m_model->expandedParentsCount(index) == expandedParentsCount);
            }
            m_keyboardAnchorIndex = index;
            m_keyboardAnchorPos = keyboardAnchorPos(index);
        }
        break;

    case Qt::Key_Right:
        if (index < itemCount - 1) {
            ++index;
            m_keyboardAnchorIndex = index;
            m_keyboardAnchorPos = keyboardAnchorPos(index);
        }
        break;

    case Qt::Key_Up:
        updateKeyboardAnchor();
        index = previousRowIndex(index);
        break;

    case Qt::Key_Down:
        updateKeyboardAnchor();
        index = nextRowIndex(index);
        break;

    case Qt::Key_PageUp:
        if (m_view->scrollOrientation() == Qt::Horizontal) {
            // The new current index should correspond to the first item in the current column.
            int newIndex = qMax(index - 1, 0);
            while (newIndex != index && m_view->itemRect(newIndex).topLeft().y() < m_view->itemRect(index).topLeft().y()) {
                index = newIndex;
                newIndex = qMax(index - 1, 0);
            }
            m_keyboardAnchorIndex = index;
            m_keyboardAnchorPos = keyboardAnchorPos(index);
        } else {
            const qreal currentItemBottom = m_view->itemRect(index).bottomLeft().y();
            const qreal height = m_view->geometry().height();

            // The new current item should be the first item in the current
            // column whose itemRect's top coordinate is larger than targetY.
            const qreal targetY = currentItemBottom - height;

            updateKeyboardAnchor();
            int newIndex = previousRowIndex(index);
            do {
                index = newIndex;
                updateKeyboardAnchor();
                newIndex = previousRowIndex(index);
            } while (m_view->itemRect(newIndex).topLeft().y() > targetY && newIndex != index);
        }
        break;

    case Qt::Key_PageDown:
        if (m_view->scrollOrientation() == Qt::Horizontal) {
            // The new current index should correspond to the last item in the current column.
            int newIndex = qMin(index + 1, m_model->count() - 1);
            while (newIndex != index && m_view->itemRect(newIndex).topLeft().y() > m_view->itemRect(index).topLeft().y()) {
                index = newIndex;
                newIndex = qMin(index + 1, m_model->count() - 1);
            }
            m_keyboardAnchorIndex = index;
            m_keyboardAnchorPos = keyboardAnchorPos(index);
        } else {
            const qreal currentItemTop = m_view->itemRect(index).topLeft().y();
            const qreal height = m_view->geometry().height();

            // The new current item should be the last item in the current
            // column whose itemRect's bottom coordinate is smaller than targetY.
            const qreal targetY = currentItemTop + height;

            updateKeyboardAnchor();
            int newIndex = nextRowIndex(index);
            do {
                index = newIndex;
                updateKeyboardAnchor();
                newIndex = nextRowIndex(index);
            } while (m_view->itemRect(newIndex).bottomLeft().y() < targetY && newIndex != index);
        }
        break;

    case Qt::Key_Enter:
    case Qt::Key_Return: {
        const KItemSet selectedItems = m_selectionManager->selectedItems();
        if (selectedItems.count() >= 2) {
            emit itemsActivated(selectedItems);
        } else if (selectedItems.count() == 1) {
            emit itemActivated(selectedItems.first());
        } else {
            emit itemActivated(index);
        }
        break;
    }

    case Qt::Key_Menu: {
        // Emit the signal itemContextMenuRequested() in case if at least one
        // item is selected. Otherwise the signal viewContextMenuRequested() will be emitted.
        const KItemSet selectedItems = m_selectionManager->selectedItems();
        int index = -1;
        if (selectedItems.count() >= 2) {
            const int currentItemIndex = m_selectionManager->currentItem();
            index = selectedItems.contains(currentItemIndex)
                    ? currentItemIndex : selectedItems.first();
        } else if (selectedItems.count() == 1) {
            index = selectedItems.first();
        }

        if (index >= 0) {
            const QRectF contextRect = m_view->itemContextRect(index);
            const QPointF pos(m_view->scene()->views().first()->mapToGlobal(contextRect.bottomRight().toPoint()));
            emit itemContextMenuRequested(index, pos);
        } else {
            emit viewContextMenuRequested(QCursor::pos());
        }
        break;
    }

    case Qt::Key_Escape:
        if (m_selectionBehavior != SingleSelection) {
            m_selectionManager->clearSelection();
        }
        m_keyboardManager->cancelSearch();
        emit escapePressed();
        break;

    case Qt::Key_Space:
        if (m_selectionBehavior == MultiSelection) {
            if (controlPressed) {
                // Toggle the selection state of the current item.
                m_selectionManager->endAnchoredSelection();
                m_selectionManager->setSelected(index, 1, KItemListSelectionManager::Toggle);
                m_selectionManager->beginAnchoredSelection(index);
                break;
            } else {
                // Select the current item if it is not selected yet.
                const int current = m_selectionManager->currentItem();
                if (!m_selectionManager->isSelected(current)) {
                    m_selectionManager->setSelected(current);
                    break;
                }
            }
        }
        Q_FALLTHROUGH();  // fall through to the default case and add the Space to the current search string.
    default:
        m_keyboardManager->addKeys(event->text());
        // Make sure unconsumed events get propagated up the chain. #302329
        event->ignore();
        return false;
    }

    if (m_selectionManager->currentItem() != index) {
        switch (m_selectionBehavior) {
        case NoSelection:
            m_selectionManager->setCurrentItem(index);
            break;

        case SingleSelection:
            m_selectionManager->setCurrentItem(index);
            m_selectionManager->clearSelection();
            m_selectionManager->setSelected(index, 1);
            break;

        case MultiSelection:
            if (controlPressed) {
                m_selectionManager->endAnchoredSelection();
            }

            m_selectionManager->setCurrentItem(index);

            if (!shiftOrControlPressed) {
                m_selectionManager->clearSelection();
                m_selectionManager->setSelected(index, 1);
            }

            if (!shiftPressed) {
                m_selectionManager->beginAnchoredSelection(index);
            }
            break;
        }
    }

    if (navigationPressed) {
        m_view->scrollToItem(index);
    }
    return true;
}

void KItemListController::slotChangeCurrentItem(const QString& text, bool searchFromNextItem)
{
    if (!m_model || m_model->count() == 0) {
        return;
    }
    int index;
    if (searchFromNextItem) {
        const int currentIndex = m_selectionManager->currentItem();
        index = m_model->indexForKeyboardSearch(text, (currentIndex + 1) % m_model->count());
    } else {
        index = m_model->indexForKeyboardSearch(text, 0);
    }
    if (index >= 0) {
        m_selectionManager->setCurrentItem(index);

        if (m_selectionBehavior != NoSelection) {
            m_selectionManager->replaceSelection(index);
            m_selectionManager->beginAnchoredSelection(index);
        }

        m_view->scrollToItem(index);
    }
}

void KItemListController::slotAutoActivationTimeout()
{
    if (!m_model || !m_view) {
        return;
    }

    const int index = m_autoActivationTimer->property("index").toInt();
    if (index < 0 || index >= m_model->count()) {
        return;
    }

    /* m_view->isUnderMouse() fixes a bug in the Folder-View-Panel and in the
     * Places-Panel.
     *
     * Bug: When you drag a file onto a Folder-View-Item or a Places-Item and
     * then move away before the auto-activation timeout triggers, than the
     * item still becomes activated/expanded.
     *
     * See Bug 293200 and 305783
     */
    if (m_model->supportsDropping(index) && m_view->isUnderMouse()) {
        if (m_view->supportsItemExpanding() && m_model->isExpandable(index)) {
            const bool expanded = m_model->isExpanded(index);
            m_model->setExpanded(index, !expanded);
        } else if (m_autoActivationBehavior != ExpansionOnly) {
            emit itemActivated(index);
        }
    }
}

bool KItemListController::inputMethodEvent(QInputMethodEvent* event)
{
    Q_UNUSED(event)
    return false;
}

bool KItemListController::mousePressEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform)
{
    m_mousePress = true;

    if (event->source() == Qt::MouseEventSynthesizedByQt && m_isTouchEvent) {
        return false;
    }

    if (!m_view) {
        return false;
    }

    m_pressedMousePos = transform.map(event->pos());
    m_pressedIndex = m_view->itemAt(m_pressedMousePos);

    const Qt::MouseButtons buttons = event->buttons();

    if (!onPress(event->screenPos(), event->pos(), event->modifiers(), buttons)) {
        startRubberBand();
        return false;
    }

    return true;
}

bool KItemListController::mouseMoveEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform)
{
    if (!m_view) {
        return false;
    }

    if (m_view->m_tapAndHoldIndicator->isActive()) {
        m_view->m_tapAndHoldIndicator->setActive(false);
    }

    if (event->source() == Qt::MouseEventSynthesizedByQt && !m_dragActionOrRightClick && m_isTouchEvent) {
        return false;
    }

    if (m_pressedIndex >= 0) {
        // Check whether a dragging should be started
        if (event->buttons() & Qt::LeftButton) {
            const QPointF pos = transform.map(event->pos());
            if ((pos - m_pressedMousePos).manhattanLength() >= QApplication::startDragDistance()) {
                if (!m_selectionManager->isSelected(m_pressedIndex)) {
                    // Always assure that the dragged item gets selected. Usually this is already
                    // done on the mouse-press event, but when using the selection-toggle on a
                    // selected item the dragged item is not selected yet.
                    m_selectionManager->setSelected(m_pressedIndex, 1, KItemListSelectionManager::Toggle);
                } else {
                    // A selected item has been clicked to drag all selected items
                    // -> the selection should not be cleared when the mouse button is released.
                    m_clearSelectionIfItemsAreNotDragged = false;
                }
                startDragging();
                m_mousePress = false;
            }
        }
    } else {
        KItemListRubberBand* rubberBand = m_view->rubberBand();
        if (rubberBand->isActive()) {
            QPointF endPos = transform.map(event->pos());

            // Update the current item.
            const int newCurrent = m_view->itemAt(endPos);
            if (newCurrent >= 0) {
                // It's expected that the new current index is also the new anchor (bug 163451).
                m_selectionManager->endAnchoredSelection();
                m_selectionManager->setCurrentItem(newCurrent);
                m_selectionManager->beginAnchoredSelection(newCurrent);
            }

            if (m_view->scrollOrientation() == Qt::Vertical) {
                endPos.ry() += m_view->scrollOffset();
                if (m_view->itemSize().width() < 0) {
                    // Use a special rubberband for views that have only one column and
                    // expand the rubberband to use the whole width of the view.
                    endPos.setX(m_view->size().width());
                }
            } else {
                endPos.rx() += m_view->scrollOffset();
            }
            rubberBand->setEndPosition(endPos);
        }
    }

    return false;
}

bool KItemListController::mouseReleaseEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform)
{
    m_mousePress = false;
    m_isTouchEvent = false;

    if (!m_view) {
        return false;
    }

    if (m_view->m_tapAndHoldIndicator->isActive()) {
        m_view->m_tapAndHoldIndicator->setActive(false);
    }

    KItemListRubberBand* rubberBand = m_view->rubberBand();
    if (event->source() == Qt::MouseEventSynthesizedByQt && !rubberBand->isActive() && m_isTouchEvent) {
        return false;
    }

    emit mouseButtonReleased(m_pressedIndex, event->buttons());

    return onRelease(transform.map(event->pos()), event->modifiers(), event->button(), false);
}

bool KItemListController::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform)
{
    const QPointF pos = transform.map(event->pos());
    const int index = m_view->itemAt(pos);

    // Expand item if desired - See Bug 295573
    if (m_mouseDoubleClickAction != ActivateItemOnly) {
        if (m_view && m_model && m_view->supportsItemExpanding() && m_model->isExpandable(index)) {
            const bool expanded = m_model->isExpanded(index);
            m_model->setExpanded(index, !expanded);
        }
    }

    if (event->button() & Qt::RightButton) {
        m_selectionManager->clearSelection();
        if (index >= 0) {
            m_selectionManager->setSelected(index);
            emit itemContextMenuRequested(index, event->screenPos());
        } else {
            const QRectF headerBounds = m_view->headerBoundaries();
            if (headerBounds.contains(event->pos())) {
                emit headerContextMenuRequested(event->screenPos());
            } else {
                emit viewContextMenuRequested(event->screenPos());
            }
        }
        return true;
    }

    bool emitItemActivated = !(m_view->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick) || m_singleClickActivationEnforced) &&
                             (event->button() & Qt::LeftButton) &&
                             index >= 0 && index < m_model->count();
    if (emitItemActivated) {
        emit itemActivated(index);
    }
    return false;
}

bool KItemListController::dragEnterEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform)
{
    Q_UNUSED(event)
    Q_UNUSED(transform)

    DragAndDropHelper::clearUrlListMatchesUrlCache();

    return false;
}

bool KItemListController::dragLeaveEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform)
{
    Q_UNUSED(event)
    Q_UNUSED(transform)

    m_autoActivationTimer->stop();
    m_view->setAutoScroll(false);
    m_view->hideDropIndicator();

    KItemListWidget* widget = hoveredWidget();
    if (widget) {
        widget->setHovered(false);
        emit itemUnhovered(widget->index());
    }
    return false;
}

bool KItemListController::dragMoveEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform)
{
    if (!m_model || !m_view) {
        return false;
    }


    QUrl hoveredDir = m_model->directory();
    KItemListWidget* oldHoveredWidget = hoveredWidget();

    const QPointF pos = transform.map(event->pos());
    KItemListWidget* newHoveredWidget = widgetForPos(pos);

    if (oldHoveredWidget != newHoveredWidget) {
        m_autoActivationTimer->stop();

        if (oldHoveredWidget) {
            oldHoveredWidget->setHovered(false);
            emit itemUnhovered(oldHoveredWidget->index());
        }
    }

    if (newHoveredWidget) {
        bool droppingBetweenItems = false;
        if (m_model->sortRole().isEmpty()) {
            // The model supports inserting items between other items.
            droppingBetweenItems = (m_view->showDropIndicator(pos) >= 0);
        }

        const int index = newHoveredWidget->index();

        if (m_model->isDir(index)) {
            hoveredDir = m_model->url(index);
        }

        if (!droppingBetweenItems) {
            if (m_model->supportsDropping(index)) {
                // Something has been dragged on an item.
                m_view->hideDropIndicator();
                if (!newHoveredWidget->isHovered()) {
                    newHoveredWidget->setHovered(true);
                    emit itemHovered(index);
                }

                if (!m_autoActivationTimer->isActive() && m_autoActivationTimer->interval() >= 0) {
                    m_autoActivationTimer->setProperty("index", index);
                    m_autoActivationTimer->start();
                }
            }
        } else {
            m_autoActivationTimer->stop();
            if (newHoveredWidget && newHoveredWidget->isHovered()) {
                newHoveredWidget->setHovered(false);
                emit itemUnhovered(index);
            }
        }
    } else {
        m_view->hideDropIndicator();
    }

    if (DragAndDropHelper::urlListMatchesUrl(event->mimeData()->urls(), hoveredDir)) {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
    } else {
        event->setDropAction(event->proposedAction());
        event->accept();
    }
    return false;
}

bool KItemListController::dropEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform)
{
    if (!m_view) {
        return false;
    }

    m_autoActivationTimer->stop();
    m_view->setAutoScroll(false);

    const QPointF pos = transform.map(event->pos());

    int dropAboveIndex = -1;
    if (m_model->sortRole().isEmpty()) {
        // The model supports inserting of items between other items.
        dropAboveIndex = m_view->showDropIndicator(pos);
    }

    if (dropAboveIndex >= 0) {
        // Something has been dropped between two items.
        m_view->hideDropIndicator();
        emit aboveItemDropEvent(dropAboveIndex, event);
    } else if (!event->mimeData()->hasFormat(m_model->blacklistItemDropEventMimeType())) {
        // Something has been dropped on an item or on an empty part of the view.
        emit itemDropEvent(m_view->itemAt(pos), event);
    }

    QAccessibleEvent accessibilityEvent(view(), QAccessible::DragDropEnd);
    QAccessible::updateAccessibility(&accessibilityEvent);

    return true;
}

bool KItemListController::hoverEnterEvent(QGraphicsSceneHoverEvent* event, const QTransform& transform)
{
    Q_UNUSED(event)
    Q_UNUSED(transform)
    return false;
}

bool KItemListController::hoverMoveEvent(QGraphicsSceneHoverEvent* event, const QTransform& transform)
{
    Q_UNUSED(transform)
    if (!m_model || !m_view) {
        return false;
    }

    KItemListWidget* oldHoveredWidget = hoveredWidget();
    const QPointF pos = transform.map(event->pos());
    KItemListWidget* newHoveredWidget = widgetForPos(pos);

    if (oldHoveredWidget != newHoveredWidget) {
        if (oldHoveredWidget) {
            oldHoveredWidget->setHovered(false);
            emit itemUnhovered(oldHoveredWidget->index());
        }

        if (newHoveredWidget) {
            newHoveredWidget->setHovered(true);
            const QPointF mappedPos = newHoveredWidget->mapFromItem(m_view, pos);
            newHoveredWidget->setHoverPosition(mappedPos);
            emit itemHovered(newHoveredWidget->index());
        }
    } else if (oldHoveredWidget) {
        const QPointF mappedPos = oldHoveredWidget->mapFromItem(m_view, pos);
        oldHoveredWidget->setHoverPosition(mappedPos);
    }

    return false;
}

bool KItemListController::hoverLeaveEvent(QGraphicsSceneHoverEvent* event, const QTransform& transform)
{
    Q_UNUSED(event)
    Q_UNUSED(transform)

    m_mousePress = false;
    m_isTouchEvent = false;

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
    Q_UNUSED(event)
    Q_UNUSED(transform)
    return false;
}

bool KItemListController::resizeEvent(QGraphicsSceneResizeEvent* event, const QTransform& transform)
{
    Q_UNUSED(event)
    Q_UNUSED(transform)
    return false;
}

bool KItemListController::gestureEvent(QGestureEvent* event, const QTransform& transform)
{
    if (!m_view) {
        return false;
    }

    //you can touch on different views at the same time, but only one QWidget gets a mousePressEvent
    //we use this to get the right QWidget
    //the only exception is a tap gesture with state GestureStarted, we need to reset some variable
    if (!m_mousePress) {
        if (QGesture* tap = event->gesture(Qt::TapGesture)) {
            QTapGesture* tapGesture = static_cast<QTapGesture*>(tap);
            if (tapGesture->state() == Qt::GestureStarted) {
                tapTriggered(tapGesture, transform);
            }
        }
        return false;
    }

    bool accepted = false;

    if (QGesture* tap = event->gesture(Qt::TapGesture)) {
        tapTriggered(static_cast<QTapGesture*>(tap), transform);
        accepted = true;
    }
    if (event->gesture(Qt::TapAndHoldGesture)) {
        tapAndHoldTriggered(event, transform);
        accepted = true;
    }
    if (event->gesture(Qt::PinchGesture)) {
        pinchTriggered(event, transform);
        accepted = true;
    }
    if (event->gesture(m_swipeGesture)) {
        swipeTriggered(event, transform);
        accepted = true;
    }
    if (event->gesture(m_twoFingerTapGesture)) {
        twoFingerTapTriggered(event, transform);
        accepted = true;
    }
    return accepted;
}

bool KItemListController::touchBeginEvent(QTouchEvent* event, const QTransform& transform)
{
    Q_UNUSED(event)
    Q_UNUSED(transform)

    m_isTouchEvent = true;
    return false;
}

void KItemListController::tapTriggered(QTapGesture* tap, const QTransform& transform)
{
    static bool scrollerWasActive = false;

    if (tap->state() == Qt::GestureStarted) {
        m_dragActionOrRightClick = false;
        m_isSwipeGesture = false;
        m_pinchGestureInProgress = false;
        scrollerWasActive = m_scrollerIsScrolling;
    }

    if (tap->state() == Qt::GestureFinished) {
        m_mousePress = false;

        //if at the moment of the gesture start the QScroller was active, the user made the tap
        //to stop the QScroller and not to tap on an item
        if (scrollerWasActive) {
            return;
        }

        if (m_view->m_tapAndHoldIndicator->isActive()) {
            m_view->m_tapAndHoldIndicator->setActive(false);
        }

        m_pressedMousePos = transform.map(tap->position());
        m_pressedIndex = m_view->itemAt(m_pressedMousePos);

        if (m_dragActionOrRightClick) {
            onPress(tap->hotSpot().toPoint(), tap->position().toPoint(), Qt::NoModifier, Qt::RightButton);
            onRelease(transform.map(tap->position()), Qt::NoModifier, Qt::RightButton, false);
            m_dragActionOrRightClick = false;
        }
        else {
            onPress(tap->hotSpot().toPoint(), tap->position().toPoint(), Qt::NoModifier, Qt::LeftButton);
            onRelease(transform.map(tap->position()), Qt::NoModifier, Qt::LeftButton, true);
        }
        m_isTouchEvent = false;
    }
}

void KItemListController::tapAndHoldTriggered(QGestureEvent* event, const QTransform& transform)
{

    //the Qt TabAndHold gesture is triggerable with a mouse click, we don't want this
    if (!m_isTouchEvent) {
        return;
    }

    const QTapAndHoldGesture* tap = static_cast<QTapAndHoldGesture*>(event->gesture(Qt::TapAndHoldGesture));
    if (tap->state() == Qt::GestureFinished) {
        //if a pinch gesture is in progress we don't want a TabAndHold gesture
        if (m_pinchGestureInProgress) {
            return;
        }
        m_pressedMousePos = transform.map(event->mapToGraphicsScene(tap->position()));
        m_pressedIndex = m_view->itemAt(m_pressedMousePos);

        if (m_pressedIndex >= 0 && !m_selectionManager->isSelected(m_pressedIndex)) {
            m_selectionManager->clearSelection();
            m_selectionManager->setSelected(m_pressedIndex);
        } else if (m_pressedIndex == -1) {
            m_selectionManager->clearSelection();
            startRubberBand();
        }

        emit scrollerStop();

        m_view->m_tapAndHoldIndicator->setStartPosition(m_pressedMousePos);
        m_view->m_tapAndHoldIndicator->setActive(true);

        m_dragActionOrRightClick = true;
    }
}

void KItemListController::pinchTriggered(QGestureEvent* event, const QTransform& transform)
{
    Q_UNUSED(transform)

    const QPinchGesture* pinch = static_cast<QPinchGesture*>(event->gesture(Qt::PinchGesture));
    const qreal sensitivityModifier = 0.2;
    static qreal counter = 0;

    if (pinch->state() == Qt::GestureStarted) {
        m_pinchGestureInProgress = true;
        counter = 0;
    }
    if (pinch->state() == Qt::GestureUpdated) {
        //if a swipe gesture was recognized or in progress, we don't want a pinch gesture to change the zoom
        if (m_isSwipeGesture) {
            return;
        }
        counter = counter + (pinch->scaleFactor() - 1);
        if (counter >= sensitivityModifier) {
            emit increaseZoom();
            counter = 0;
        } else if (counter <= -sensitivityModifier) {
            emit decreaseZoom();
            counter = 0;
        }
    }
}

void KItemListController::swipeTriggered(QGestureEvent* event, const QTransform& transform)
{
    Q_UNUSED(transform)

    const KTwoFingerSwipe* swipe = static_cast<KTwoFingerSwipe*>(event->gesture(m_swipeGesture));

    if (!swipe) {
        return;
    }
    if (swipe->state() == Qt::GestureStarted) {
        m_isSwipeGesture = true;
    }

    if (swipe->state() == Qt::GestureCanceled) {
        m_isSwipeGesture = false;
    }

    if (swipe->state() == Qt::GestureFinished) {
        emit scrollerStop();

        if (swipe->swipeAngle() <= 20 || swipe->swipeAngle() >= 340) {
            emit mouseButtonPressed(m_pressedIndex, Qt::BackButton);
        } else if (swipe->swipeAngle() <= 200 && swipe->swipeAngle() >= 160) {
            emit mouseButtonPressed(m_pressedIndex, Qt::ForwardButton);
        } else if (swipe->swipeAngle() <= 110 && swipe->swipeAngle() >= 60) {
            emit swipeUp();
        }
        m_isSwipeGesture = true;
    }
}

void KItemListController::twoFingerTapTriggered(QGestureEvent* event, const QTransform& transform)
{
    const KTwoFingerTap* twoTap = static_cast<KTwoFingerTap*>(event->gesture(m_twoFingerTapGesture));

    if (!twoTap) {
        return;
    }

    if (twoTap->state() == Qt::GestureStarted) {
        m_pressedMousePos = transform.map(twoTap->pos());
        m_pressedIndex = m_view->itemAt(m_pressedMousePos);
        if (m_pressedIndex >= 0) {
            onPress(twoTap->screenPos().toPoint(), twoTap->pos().toPoint(), Qt::ControlModifier, Qt::LeftButton);
            onRelease(transform.map(twoTap->pos()), Qt::ControlModifier, Qt::LeftButton, false);
        }

    }
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
    case QEvent::Gesture:
        return gestureEvent(static_cast<QGestureEvent*>(event), transform);
    case QEvent::TouchBegin:
        return touchBeginEvent(static_cast<QTouchEvent*>(event), transform);
    default:
        break;
    }

    return false;
}

void KItemListController::slotViewScrollOffsetChanged(qreal current, qreal previous)
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
        rubberBandRect.translate(0, -m_view->scrollOffset());
    } else {
        rubberBandRect.translate(-m_view->scrollOffset(), 0);
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

    KItemSet selectedItems;

    // Select all visible items that intersect with the rubberband
    foreach (const KItemListWidget* widget, m_view->visibleItemListWidgets()) {
        const int index = widget->index();

        const QRectF widgetRect = m_view->itemRect(index);
        if (widgetRect.intersects(rubberBandRect)) {
            const QRectF iconRect = widget->iconRect().translated(widgetRect.topLeft());
            const QRectF textRect = widget->textRect().translated(widgetRect.topLeft());
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
        const QRectF widgetRect = m_view->itemRect(index);
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

    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        // If Control is pressed, the selection state of all items in the rubberband is toggled.
        // Therefore, the new selection contains:
        // 1. All previously selected items which are not inside the rubberband, and
        // 2. all items inside the rubberband which have not been selected previously.
        m_selectionManager->setSelectedItems(m_oldSelection ^ selectedItems);
    }
    else {
        m_selectionManager->setSelectedItems(selectedItems + m_oldSelection);
    }
}

void KItemListController::startDragging()
{
    if (!m_view || !m_model) {
        return;
    }

    const KItemSet selectedItems = m_selectionManager->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QMimeData* data = m_model->createMimeData(selectedItems);
    if (!data) {
        return;
    }

    // The created drag object will be owned and deleted
    // by QApplication::activeWindow().
    QDrag* drag = new QDrag(QApplication::activeWindow());
    drag->setMimeData(data);

    const QPixmap pixmap = m_view->createDragPixmap(selectedItems);
    drag->setPixmap(pixmap);

    const QPoint hotSpot((pixmap.width() / pixmap.devicePixelRatio()) / 2, 0);
    drag->setHotSpot(hotSpot);

    drag->exec(Qt::MoveAction | Qt::CopyAction | Qt::LinkAction, Qt::CopyAction);

    QAccessibleEvent accessibilityEvent(view(), QAccessible::DragDropStart);
    QAccessible::updateAccessibility(&accessibilityEvent);
}

KItemListWidget* KItemListController::hoveredWidget() const
{
    Q_ASSERT(m_view);

    foreach (KItemListWidget* widget, m_view->visibleItemListWidgets()) {
        if (widget->isHovered()) {
            return widget;
        }
    }

    return nullptr;
}

KItemListWidget* KItemListController::widgetForPos(const QPointF& pos) const
{
    Q_ASSERT(m_view);

    foreach (KItemListWidget* widget, m_view->visibleItemListWidgets()) {
        const QPointF mappedPos = widget->mapFromItem(m_view, pos);

        const bool hovered = widget->contains(mappedPos) &&
                             !widget->expansionToggleRect().contains(mappedPos);
        if (hovered) {
            return widget;
        }
    }

    return nullptr;
}

void KItemListController::updateKeyboardAnchor()
{
    const bool validAnchor = m_keyboardAnchorIndex >= 0 &&
                             m_keyboardAnchorIndex < m_model->count() &&
                             keyboardAnchorPos(m_keyboardAnchorIndex) == m_keyboardAnchorPos;
    if (!validAnchor) {
        const int index = m_selectionManager->currentItem();
        m_keyboardAnchorIndex = index;
        m_keyboardAnchorPos = keyboardAnchorPos(index);
    }
}

int KItemListController::nextRowIndex(int index) const
{
    if (m_keyboardAnchorIndex < 0) {
        return index;
    }

    const int maxIndex = m_model->count() - 1;
    if (index == maxIndex) {
        return index;
    }

    // Calculate the index of the last column inside the row of the current index
    int lastColumnIndex = index;
    while (keyboardAnchorPos(lastColumnIndex + 1) > keyboardAnchorPos(lastColumnIndex)) {
        ++lastColumnIndex;
        if (lastColumnIndex >= maxIndex) {
            return index;
        }
    }

    // Based on the last column index go to the next row and calculate the nearest index
    // that is below the current index
    int nextRowIndex = lastColumnIndex + 1;
    int searchIndex = nextRowIndex;
    qreal minDiff = qAbs(m_keyboardAnchorPos - keyboardAnchorPos(nextRowIndex));
    while (searchIndex < maxIndex && keyboardAnchorPos(searchIndex + 1) > keyboardAnchorPos(searchIndex)) {
        ++searchIndex;
        const qreal searchDiff = qAbs(m_keyboardAnchorPos - keyboardAnchorPos(searchIndex));
        if (searchDiff < minDiff) {
            minDiff = searchDiff;
            nextRowIndex = searchIndex;
        }
    }

    return nextRowIndex;
}

int KItemListController::previousRowIndex(int index) const
{
    if (m_keyboardAnchorIndex < 0 || index == 0) {
        return index;
    }

    // Calculate the index of the first column inside the row of the current index
    int firstColumnIndex = index;
    while (keyboardAnchorPos(firstColumnIndex - 1) < keyboardAnchorPos(firstColumnIndex)) {
        --firstColumnIndex;
        if (firstColumnIndex <= 0) {
            return index;
        }
    }

    // Based on the first column index go to the previous row and calculate the nearest index
    // that is above the current index
    int previousRowIndex = firstColumnIndex - 1;
    int searchIndex = previousRowIndex;
    qreal minDiff = qAbs(m_keyboardAnchorPos - keyboardAnchorPos(previousRowIndex));
    while (searchIndex > 0 && keyboardAnchorPos(searchIndex - 1) < keyboardAnchorPos(searchIndex)) {
        --searchIndex;
        const qreal searchDiff = qAbs(m_keyboardAnchorPos - keyboardAnchorPos(searchIndex));
        if (searchDiff < minDiff) {
            minDiff = searchDiff;
            previousRowIndex = searchIndex;
        }
    }

    return previousRowIndex;
}

qreal KItemListController::keyboardAnchorPos(int index) const
{
    const QRectF itemRect = m_view->itemRect(index);
    if (!itemRect.isEmpty()) {
        return (m_view->scrollOrientation() == Qt::Vertical) ? itemRect.x() : itemRect.y();
    }

    return 0;
}

void KItemListController::updateExtendedSelectionRegion()
{
    if (m_view) {
        const bool extend = (m_selectionBehavior != MultiSelection);
        KItemListStyleOption option = m_view->styleOption();
        if (option.extendedSelectionRegion != extend) {
            option.extendedSelectionRegion = extend;
            m_view->setStyleOption(option);
        }
    }
}

bool KItemListController::onPress(const QPoint& screenPos, const QPointF& pos, const Qt::KeyboardModifiers modifiers, const Qt::MouseButtons buttons)
{
    emit mouseButtonPressed(m_pressedIndex, buttons);

    if (buttons & (Qt::BackButton | Qt::ForwardButton)) {
        // Do not select items when clicking the back/forward buttons, see
        // https://bugs.kde.org/show_bug.cgi?id=327412.
        return true;
    }

    if (m_view->isAboveExpansionToggle(m_pressedIndex, m_pressedMousePos)) {
        m_selectionManager->endAnchoredSelection();
        m_selectionManager->setCurrentItem(m_pressedIndex);
        m_selectionManager->beginAnchoredSelection(m_pressedIndex);
        return true;
    }

    m_selectionTogglePressed = m_view->isAboveSelectionToggle(m_pressedIndex, m_pressedMousePos);
    if (m_selectionTogglePressed) {
        m_selectionManager->setSelected(m_pressedIndex, 1, KItemListSelectionManager::Toggle);
        // The previous anchored selection has been finished already in
        // KItemListSelectionManager::setSelected(). We can safely change
        // the current item and start a new anchored selection now.
        m_selectionManager->setCurrentItem(m_pressedIndex);
        m_selectionManager->beginAnchoredSelection(m_pressedIndex);
        return true;
    }

    const bool shiftPressed = modifiers & Qt::ShiftModifier;
    const bool controlPressed = modifiers & Qt::ControlModifier;

    // The previous selection is cleared if either
    // 1. The selection mode is SingleSelection, or
    // 2. the selection mode is MultiSelection, and *none* of the following conditions are met:
    //    a) Shift or Control are pressed.
    //    b) The clicked item is selected already. In that case, the user might want to:
    //       - start dragging multiple items, or
    //       - open the context menu and perform an action for all selected items.
    const bool shiftOrControlPressed = shiftPressed || controlPressed;
    const bool pressedItemAlreadySelected = m_pressedIndex >= 0 && m_selectionManager->isSelected(m_pressedIndex);
    const bool clearSelection = m_selectionBehavior == SingleSelection ||
                                (!shiftOrControlPressed && !pressedItemAlreadySelected);
    if (clearSelection) {
        m_selectionManager->clearSelection();
    } else if (pressedItemAlreadySelected && !shiftOrControlPressed && (buttons & Qt::LeftButton)) {
        // The user might want to start dragging multiple items, but if he clicks the item
        // in order to trigger it instead, the other selected items must be deselected.
        // However, we do not know yet what the user is going to do.
        // -> remember that the user pressed an item which had been selected already and
        //    clear the selection in mouseReleaseEvent(), unless the items are dragged.
        m_clearSelectionIfItemsAreNotDragged = true;

        if (m_selectionManager->selectedItems().count() == 1 && m_view->isAboveText(m_pressedIndex, m_pressedMousePos)) {
            emit selectedItemTextPressed(m_pressedIndex);
        }
    }

    if (!shiftPressed) {
        // Finish the anchored selection before the current index is changed
        m_selectionManager->endAnchoredSelection();
    }

    if (buttons & Qt::RightButton) {
        // Stop rubber band from persisting after right-clicks
        KItemListRubberBand* rubberBand = m_view->rubberBand();
        if (rubberBand->isActive()) {
            disconnect(rubberBand, &KItemListRubberBand::endPositionChanged, this, &KItemListController::slotRubberBandChanged);
            rubberBand->setActive(false);
            m_view->setAutoScroll(false);
        }
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
            if (controlPressed && !shiftPressed) {
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

        if (buttons & Qt::RightButton) {
            emit itemContextMenuRequested(m_pressedIndex, screenPos);
        }

        return true;
    }

    if (buttons & Qt::RightButton) {
        const QRectF headerBounds = m_view->headerBoundaries();
        if (headerBounds.contains(pos)) {
            emit headerContextMenuRequested(screenPos);
        } else {
            emit viewContextMenuRequested(screenPos);
        }
        return true;
    }

    return false;
}

bool KItemListController::onRelease(const QPointF& pos, const Qt::KeyboardModifiers modifiers, const Qt::MouseButtons buttons, bool touch)
{
    const bool isAboveSelectionToggle = m_view->isAboveSelectionToggle(m_pressedIndex, m_pressedMousePos);
    if (isAboveSelectionToggle) {
        m_selectionTogglePressed = false;
        return true;
    }

    if (!isAboveSelectionToggle && m_selectionTogglePressed) {
        m_selectionManager->setSelected(m_pressedIndex, 1, KItemListSelectionManager::Toggle);
        m_selectionTogglePressed = false;
        return true;
    }

    const bool shiftOrControlPressed = modifiers & Qt::ShiftModifier ||
                                       modifiers & Qt::ControlModifier;

    KItemListRubberBand* rubberBand = m_view->rubberBand();
    if (rubberBand->isActive()) {
        disconnect(rubberBand, &KItemListRubberBand::endPositionChanged, this, &KItemListController::slotRubberBandChanged);
        rubberBand->setActive(false);
        m_oldSelection.clear();
        m_view->setAutoScroll(false);
    }

    const int index = m_view->itemAt(pos);

    if (index >= 0 && index == m_pressedIndex) {
        // The release event is done above the same item as the press event

        if (m_clearSelectionIfItemsAreNotDragged) {
            // A selected item has been clicked, but no drag operation has been started
            // -> clear the rest of the selection.
            m_selectionManager->clearSelection();
            m_selectionManager->setSelected(m_pressedIndex, 1, KItemListSelectionManager::Select);
            m_selectionManager->beginAnchoredSelection(m_pressedIndex);
        }

        if (buttons & Qt::LeftButton) {
            bool emitItemActivated = true;
            if (m_view->isAboveExpansionToggle(index, pos)) {
                const bool expanded = m_model->isExpanded(index);
                m_model->setExpanded(index, !expanded);

                emit itemExpansionToggleClicked(index);
                emitItemActivated = false;
            } else if (shiftOrControlPressed) {
                // The mouse click should only update the selection, not trigger the item
                emitItemActivated = false;
            } else if (!(m_view->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick) || m_singleClickActivationEnforced)) {
                if (touch) {
                emitItemActivated = true;
                } else {
                emitItemActivated = false;
                }
            }
            if (emitItemActivated) {
                emit itemActivated(index);
            }
        } else if (buttons & Qt::MiddleButton) {
            emit itemMiddleClicked(index);
        }
    }

    m_pressedMousePos = QPointF();
    m_pressedIndex = -1;
    m_clearSelectionIfItemsAreNotDragged = false;
    return false;
}

void KItemListController::startRubberBand()
{
    if (m_selectionBehavior == MultiSelection) {
        QPointF startPos = m_pressedMousePos;
        if (m_view->scrollOrientation() == Qt::Vertical) {
            startPos.ry() += m_view->scrollOffset();
            if (m_view->itemSize().width() < 0) {
                // Use a special rubberband for views that have only one column and
                // expand the rubberband to use the whole width of the view.
                startPos.setX(0);
            }
        } else {
            startPos.rx() += m_view->scrollOffset();
        }

        m_oldSelection = m_selectionManager->selectedItems();
        KItemListRubberBand* rubberBand = m_view->rubberBand();
        rubberBand->setStartPosition(startPos);
        rubberBand->setEndPosition(startPos);
        rubberBand->setActive(true);
        connect(rubberBand, &KItemListRubberBand::endPositionChanged, this, &KItemListController::slotRubberBandChanged);
        m_view->setAutoScroll(true);
    }
}

void KItemListController::slotStateChanged(QScroller::State newState)
{
    if (newState == QScroller::Scrolling) {
        m_scrollerIsScrolling = true;
    } else if (newState == QScroller::Inactive) {
        m_scrollerIsScrolling = false;
    }
}
