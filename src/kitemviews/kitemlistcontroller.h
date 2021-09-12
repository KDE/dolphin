/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTCONTROLLER_H
#define KITEMLISTCONTROLLER_H

#include <optional>

#include "dolphin_export.h"
#include "kitemset.h"

#include <QObject>
#include <QPointF>
#include <QScroller>

class QTimer;
class KItemModelBase;
class KItemListKeyboardSearchManager;
class KItemListSelectionManager;
class KItemListView;
class KItemListWidget;
class QGestureEvent;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneDragDropEvent;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneResizeEvent;
class QGraphicsSceneWheelEvent;
class QInputMethodEvent;
class QKeyEvent;
class QTapGesture;
class QTransform;
class QTouchEvent;

/**
 * @brief Controls the view, model and selection of an item-list.
 *
 * For a working item-list it is mandatory to set a compatible view and model
 * with KItemListController::setView() and KItemListController::setModel().
 *
 * @see KItemListView
 * @see KItemModelBase
 * @see KItemListSelectionManager
 */
class DOLPHIN_EXPORT KItemListController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(KItemModelBase* model READ model WRITE setModel)
    Q_PROPERTY(KItemListView *view READ view WRITE setView)
    Q_PROPERTY(SelectionBehavior selectionBehavior READ selectionBehavior WRITE setSelectionBehavior)
    Q_PROPERTY(AutoActivationBehavior autoActivationBehavior READ autoActivationBehavior WRITE setAutoActivationBehavior)
    Q_PROPERTY(MouseDoubleClickAction mouseDoubleClickAction READ mouseDoubleClickAction WRITE setMouseDoubleClickAction)

public:
    enum SelectionBehavior {
        NoSelection,
        SingleSelection,
        MultiSelection
    };
    Q_ENUM(SelectionBehavior)

    enum AutoActivationBehavior {
        ActivationAndExpansion,
        ExpansionOnly
    };

    enum MouseDoubleClickAction {
        ActivateAndExpandItem,
        ActivateItemOnly
    };

    /**
     * @param model  Model of the controller. The ownership is passed to the controller.
     * @param view   View of the controller. The ownership is passed to the controller.
     * @param parent Optional parent object.
     */
    KItemListController(KItemModelBase* model, KItemListView* view, QObject* parent = nullptr);
    ~KItemListController() override;

    void setModel(KItemModelBase* model);
    KItemModelBase* model() const;

    void setView(KItemListView* view);
    KItemListView* view() const;

    KItemListSelectionManager* selectionManager() const;

    void setSelectionBehavior(SelectionBehavior behavior);
    SelectionBehavior selectionBehavior() const;

    void setAutoActivationBehavior(AutoActivationBehavior behavior);
    AutoActivationBehavior autoActivationBehavior() const;

    void setMouseDoubleClickAction(MouseDoubleClickAction action);
    MouseDoubleClickAction mouseDoubleClickAction() const;

    int indexCloseToMousePressedPosition() const;

    /**
     * Sets the delay in milliseconds when dragging an object above an item
     * until the item gets activated automatically. A value of -1 indicates
     * that no automatic activation will be done at all (= default value).
     *
     * The hovered item must support dropping (see KItemModelBase::supportsDropping()),
     * otherwise the automatic activation is not available.
     *
     * After activating the item the signal itemActivated() will be
     * emitted. If the view supports the expanding of items
     * (KItemListView::supportsItemExpanding() returns true) and the item
     * itself is expandable (see KItemModelBase::isExpandable()) then instead
     * of activating the item it gets expanded instead (see
     * KItemModelBase::setExpanded()).
     */
    void setAutoActivationDelay(int delay);
    int autoActivationDelay() const;

    /**
     * If set to true, the signals itemActivated() and itemsActivated() are emitted
     * after a single-click of the left mouse button. If set to false (the default),
     * the setting from style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick) is used.
     */
    void setSingleClickActivationEnforced(bool singleClick);
    bool singleClickActivationEnforced() const;

    void setSelectionMode(bool enabled);
    bool selectionMode() const;

    bool processEvent(QEvent* event, const QTransform& transform);

Q_SIGNALS:
    /**
     * Is emitted if exactly one item has been activated by e.g. a mouse-click
     * or by pressing Return/Enter.
     */
    void itemActivated(int index);

    /**
     * Is emitted if more than one item has been activated by pressing Return/Enter
     * when having a selection.
     */
    void itemsActivated(const KItemSet &indexes);

    void itemMiddleClicked(int index);

    /**
     * Emitted if a context-menu is requested for the item with
     * the index \a index. It is assured that the index is valid.
     */
    void itemContextMenuRequested(int index, const QPointF& pos);

    /**
     * Emitted if a context-menu is requested for the KItemListView.
     */
    void viewContextMenuRequested(const QPointF& pos);

    /**
     * Emitted if a context-menu is requested for the header of the KItemListView.
     */
    void headerContextMenuRequested(const QPointF& pos);

    /**
     * Is emitted if the item with the index \p index gets hovered.
     */
    void itemHovered(int index);

    /**
     * Is emitted if the item with the index \p index gets unhovered.
     * It is assured that the signal itemHovered() for this index
     * has been emitted before.
     */
    void itemUnhovered(int index);

    /**
     * Is emitted if a mouse-button has been pressed above an item.
     * If the index is smaller than 0, the mouse-button has been pressed
     * above the viewport.
     */
    void mouseButtonPressed(int itemIndex, Qt::MouseButtons buttons);

    /**
     * Is emitted if a mouse-button has been released above an item.
     * It is assured that the signal mouseButtonPressed() has been emitted before.
     * If the index is smaller than 0, the mouse-button has been pressed
     * above the viewport.
     */
    void mouseButtonReleased(int itemIndex, Qt::MouseButtons buttons);

    void itemExpansionToggleClicked(int index);

    /**
     * Is emitted if a drop event is done above the item with the index
     * \a index. If \a index is < 0 the drop event is done above an
     * empty area of the view.
     * TODO: Introduce a new signal viewDropEvent(QGraphicsSceneDragDropEvent),
     *       which is emitted if the drop event occurs on an empty area in
     *       the view, and make sure that index is always >= 0 in itemDropEvent().
     */
    void itemDropEvent(int index, QGraphicsSceneDragDropEvent* event);

    /**
     * Is emitted if a drop event is done between the item with the index
     * \a index and the previous item.
     */
    void aboveItemDropEvent(int index, QGraphicsSceneDragDropEvent* event);

    /**
     * Is emitted if the Escape key is pressed.
     */
    void escapePressed();

    /**
     * Is emitted if left click is pressed down for a long time without moving the cursor too much.
     * Moving the cursor would either trigger an item drag if the click was initiated on top of an item
     * or a selection rectangle if the click was not initiated on top of an item.
     * So long press is only emitted if there wasn't a lot of cursor movement.
     */
    void selectionModeRequested();

    void modelChanged(KItemModelBase* current, KItemModelBase* previous);
    void viewChanged(KItemListView* current, KItemListView* previous);

    void selectedItemTextPressed(int index);

    void scrollerStop();
    void increaseZoom();
    void decreaseZoom();
    void swipeUp();

public Q_SLOTS:
    void slotStateChanged(QScroller::State newState);

private Q_SLOTS:
    void slotViewScrollOffsetChanged(qreal current, qreal previous);

    /**
     * Is invoked when the rubberband boundaries have been changed and will select
     * all items that are touched by the rubberband.
     */
    void slotRubberBandChanged();

    void slotChangeCurrentItem(const QString& text, bool searchFromNextItem);

    void slotAutoActivationTimeout();

private:
    /**
     * Creates a QDrag object and initiates a drag-operation.
     */
    void startDragging();

    /**
     * @return Widget that is currently in the hovered state. 0 is returned
     *         if no widget is marked as hovered.
     */
    KItemListWidget* hoveredWidget() const;

    /**
     * @return Widget that is below the position \a pos. 0 is returned
     *         if no widget is below the position.
     */
    KItemListWidget* widgetForPos(const QPointF& pos) const;

    /**
     * @return Widget that should receive a drop event if an item is dropped at \a pos. 0 is returned
     *         if no widget should receive a drop event at the position.
     *
     * While widgetForPos() returns a widget if \a pos is anywhere inside the hover highlight area of the widget,
     * widgetForDropPos() only returns a widget if \a pos is directly above the widget (widget->contains(pos) == true).
     */
    KItemListWidget* widgetForDropPos(const QPointF& pos) const;

    /**
     * Updates m_keyboardAnchorIndex and m_keyboardAnchorPos. If no anchor is
     * set, it will be adjusted to the current item. If it is set it will be
     * checked whether it is still valid, otherwise it will be reset to the
     * current item.
     */
    void updateKeyboardAnchor();

    /**
     * @return Index for the next row based on \a index.
     *         If there is no next row \a index will be returned.
     */
    int nextRowIndex(int index) const;

    /**
     * @return Index for the previous row based on  \a index.
     *         If there is no previous row \a index will be returned.
     */
    int previousRowIndex(int index) const;

    /**
     * Helper method for updateKeyboardAnchor(), previousRowIndex() and nextRowIndex().
     * @return The position of the keyboard anchor for the item with the index \a index.
     *         If a horizontal scrolling is used the y-position of the item will be returned,
     *         for the vertical scrolling the x-position will be returned.
     */
    qreal keyboardAnchorPos(int index) const;

    /**
     * Dependent on the selection-behavior the extendedSelectionRegion-property
     * of the KItemListStyleOption from the view should be adjusted: If no
     * rubberband selection is used the property should be enabled.
     */
    void updateExtendedSelectionRegion();

    bool keyPressEvent(QKeyEvent* event);
    bool inputMethodEvent(QInputMethodEvent* event);
    bool mousePressEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform);
    bool mouseMoveEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform);
    bool mouseReleaseEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform);
    bool mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform);
    bool dragEnterEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform);
    bool dragLeaveEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform);
    bool dragMoveEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform);
    bool dropEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform);
    bool hoverEnterEvent(QGraphicsSceneHoverEvent* event, const QTransform& transform);
    bool hoverMoveEvent(QGraphicsSceneHoverEvent* event, const QTransform& transform);
    bool hoverLeaveEvent(QGraphicsSceneHoverEvent* event, const QTransform& transform);
    bool wheelEvent(QGraphicsSceneWheelEvent* event, const QTransform& transform);
    bool resizeEvent(QGraphicsSceneResizeEvent* event, const QTransform& transform);
    bool gestureEvent(QGestureEvent* event, const QTransform& transform);
    bool touchBeginEvent(QTouchEvent* event, const QTransform& transform);
    void tapTriggered(QTapGesture* tap, const QTransform& transform);
    void tapAndHoldTriggered(QGestureEvent* event, const QTransform& transform);
    void pinchTriggered(QGestureEvent* event, const QTransform& transform);
    void swipeTriggered(QGestureEvent* event, const QTransform& transform);
    void twoFingerTapTriggered(QGestureEvent* event, const QTransform& transform);
    bool onPress(const QPoint& screenPos, const QPointF& pos, const Qt::KeyboardModifiers modifiers, const Qt::MouseButtons buttons);
    bool onRelease(const QPointF& pos, const Qt::KeyboardModifiers modifiers, const Qt::MouseButtons buttons, bool touch);
    void startRubberBand();

private:
    bool m_singleClickActivationEnforced;
    bool m_selectionMode;
    bool m_selectionTogglePressed;
    bool m_clearSelectionIfItemsAreNotDragged;
    bool m_isSwipeGesture;
    bool m_dragActionOrRightClick;
    bool m_scrollerIsScrolling;
    bool m_pinchGestureInProgress;
    bool m_mousePress;
    bool m_isTouchEvent;
    SelectionBehavior m_selectionBehavior;
    AutoActivationBehavior m_autoActivationBehavior;
    MouseDoubleClickAction m_mouseDoubleClickAction;
    KItemModelBase* m_model;
    KItemListView* m_view;
    KItemListSelectionManager* m_selectionManager;
    KItemListKeyboardSearchManager* m_keyboardManager;
    std::optional<int> m_pressedIndex;
    QPointF m_pressedMousePos;

    QTimer* m_autoActivationTimer;
    QTimer* m_longPressDetectionTimer;

    Qt::GestureType m_swipeGesture;
    Qt::GestureType m_twoFingerTapGesture;

    /**
     * When starting a rubberband selection during a Shift- or Control-key has been
     * pressed the current selection should never be deleted. To be able to restore
     * the current selection it is remembered in m_oldSelection before the
     * rubberband gets activated.
     */
    KItemSet m_oldSelection;

    /**
     * Assuming a view is given with a vertical scroll-orientation, grouped items and
     * a maximum of 4 columns:
     *
     *  1  2  3  4
     *  5  6  7
     *  8  9 10 11
     * 12 13 14
     *
     * If the current index is on 4 and key-down is pressed, then item 7 gets the current
     * item. Now when pressing key-down again item 11 should get the current item and not
     * item 10. This makes it necessary to keep track of the requested column to have a
     * similar behavior like in a text editor:
     */
    int m_keyboardAnchorIndex;
    qreal m_keyboardAnchorPos;
};

#endif


