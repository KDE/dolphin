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
    void itemActivated(int index);
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

private:
    bool m_selectionTogglePressed;
    SelectionBehavior m_selectionBehavior;
    KItemModelBase* m_model;
    KItemListView* m_view;
    KItemListSelectionManager* m_selectionManager;
    KItemListKeyboardSearchManager* m_keyboardManager;
    int m_pressedIndex;
    QPointF m_pressedMousePos;

    /**
     * When starting a rubberband selection during a Shift- or Control-key has been
     * pressed the current selection should never be deleted. To be able to restore
     * the current selection it is remembered in m_oldSelection before
     * rubberband gets activated.
     */
    QSet<int> m_oldSelection;
};

#endif


