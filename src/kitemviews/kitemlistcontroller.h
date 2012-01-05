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

#ifndef KITEMLISTCONTROLLER_H
#define KITEMLISTCONTROLLER_H

#include <libdolphin_export.h>

#include <QObject>
#include <QPixmap>
#include <QPointF>
#include <QSet>

class KItemModelBase;
class KItemListKeyboardSearchManager;
class KItemListSelectionManager;
class KItemListView;
class KItemListWidget;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneDragDropEvent;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneResizeEvent;
class QGraphicsSceneWheelEvent;
class QHideEvent;
class QInputMethodEvent;
class QKeyEvent;
class QMimeData;
class QShowEvent;
class QTransform;

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
class LIBDOLPHINPRIVATE_EXPORT KItemListController : public QObject
{
    Q_OBJECT
    Q_ENUMS(SelectionBehavior)
    Q_PROPERTY(KItemModelBase* model READ model WRITE setModel)
    Q_PROPERTY(KItemListView *view READ view WRITE setView)
    Q_PROPERTY(SelectionBehavior selectionBehavior READ selectionBehavior WRITE setSelectionBehavior)

public:
    enum SelectionBehavior {
        NoSelection,
        SingleSelection,
        MultiSelection
    };

    KItemListController(QObject* parent = 0);
    virtual ~KItemListController();

    void setModel(KItemModelBase* model);
    KItemModelBase* model() const;

    void setView(KItemListView* view);
    KItemListView* view() const;

    KItemListSelectionManager* selectionManager() const;

    void setSelectionBehavior(SelectionBehavior behavior);
    SelectionBehavior selectionBehavior() const;

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
     * after a single-click of the left mouse button. If set to false, a double-click
     * is required. Per default the setting from KGlobalSettings::singleClick() is
     * used.
     */
    void setSingleClickActivation(bool singleClick);
    bool singleClickActivation() const;

    virtual bool showEvent(QShowEvent* event);
    virtual bool hideEvent(QHideEvent* event);
    virtual bool keyPressEvent(QKeyEvent* event);
    virtual bool inputMethodEvent(QInputMethodEvent* event);
    virtual bool mousePressEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform);
    virtual bool mouseMoveEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform);
    virtual bool mouseReleaseEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform);
    virtual bool mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event, const QTransform& transform);
    virtual bool dragEnterEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform);
    virtual bool dragLeaveEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform);
    virtual bool dragMoveEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform);
    virtual bool dropEvent(QGraphicsSceneDragDropEvent* event, const QTransform& transform);
    virtual bool hoverEnterEvent(QGraphicsSceneHoverEvent* event, const QTransform& transform);
    virtual bool hoverMoveEvent(QGraphicsSceneHoverEvent* event, const QTransform& transform);
    virtual bool hoverLeaveEvent(QGraphicsSceneHoverEvent* event, const QTransform& transform);
    virtual bool wheelEvent(QGraphicsSceneWheelEvent* event, const QTransform& transform);
    virtual bool resizeEvent(QGraphicsSceneResizeEvent* event, const QTransform& transform);
    virtual bool processEvent(QEvent* event, const QTransform& transform);

signals:
    /**
     * Is emitted if exactly one item has been activated by e.g. a mouse-click
     * or by pressing Return/Enter.
     */
    void itemActivated(int index);

    /**
     * Is emitted if more than one item has been activated by pressing Return/Enter
     * when having a selection.
     */
    void itemsActivated(const QSet<int>& indexes);

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
     */
    void itemPressed(int index, Qt::MouseButton button);

    /**
     * Is emitted if a mouse-button has been released above an item.
     * It is assured that the signal itemPressed() has been emitted before.
     */
    void itemReleased(int index, Qt::MouseButton button);

    void itemExpansionToggleClicked(int index);

    /**
     * Is emitted if a drop event is done above the item with the index
     * \a index. If \a index is < 0 the drop event is done above an
     * empty area of the view.
     */
    void itemDropEvent(int index, QGraphicsSceneDragDropEvent* event);

    void modelChanged(KItemModelBase* current, KItemModelBase* previous);
    void viewChanged(KItemListView* current, KItemListView* previous);

private slots:
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
     * Updates m_keyboardAnchorIndex and m_keyboardAnchorPos. If no anchor is
     * set, it will be adjusted to the current item. If it is set it will be
     * checked whether it is still valid, otherwise it will be reset to the
     * current item.
     */
    void updateKeyboardAnchor();

    /**
     * @return Index for the next row based on the current index.
     *         If there is no next row the current index will be returned.
     */
    int nextRowIndex() const;

    /**
     * @return Index for the previous row based on the current index.
     *         If there is no previous row the current index will be returned.
     */
    int previousRowIndex() const;

    /**
     * Helper method for updateKeyboardAnchor(), previousRowIndex() and nextRowIndex().
     * @return The position of the keyboard anchor for the item with the index \a index.
     *         If a horizontal scrolling is used the y-position of the item will be returned,
     *         for the vertical scrolling the x-position will be returned.
     */
    qreal keyboardAnchorPos(int index) const;

private:
    bool m_singleClickActivation;
    bool m_selectionTogglePressed;
    SelectionBehavior m_selectionBehavior;
    KItemModelBase* m_model;
    KItemListView* m_view;
    KItemListSelectionManager* m_selectionManager;
    KItemListKeyboardSearchManager* m_keyboardManager;
    int m_pressedIndex;
    QPointF m_pressedMousePos;

    QTimer* m_autoActivationTimer;

    /**
     * When starting a rubberband selection during a Shift- or Control-key has been
     * pressed the current selection should never be deleted. To be able to restore
     * the current selection it is remembered in m_oldSelection before the
     * rubberband gets activated.
     */
    QSet<int> m_oldSelection;

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


