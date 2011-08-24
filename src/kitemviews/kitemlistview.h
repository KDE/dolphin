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

#ifndef KITEMLISTVIEW_H
#define KITEMLISTVIEW_H

#include <libdolphin_export.h>

#include <kitemviews/kitemliststyleoption.h>
#include <kitemviews/kitemlistviewanimation_p.h>
#include <kitemviews/kitemlistwidget.h>
#include <kitemviews/kitemmodelbase.h>
#include <QGraphicsWidget>
#include <QSet>

class KItemListController;
class KItemListWidgetCreatorBase;
class KItemListGroupHeader;
class KItemListGroupHeaderCreatorBase;
class KItemListSizeHintResolver;
class KItemListRubberBand;
class KItemListViewAnimation;
class KItemListViewLayouter;
class KItemListWidget;
class KItemListViewCreatorBase;
class QTimer;

/**
 * @brief Represents the view of an item-list.
 *
 * The view is responsible for showing the items of the model within
 * a GraphicsItem. Each visible item is represented by a KItemListWidget.
 *
 * The created view must be applied to the KItemListController with
 * KItemListController::setView(). For showing a custom model it is not
 * mandatory to derive from KItemListView, all that is necessary is
 * to set a widget-creator that is capable to create KItemListWidgets
 * showing the model items. A widget-creator can be set with
 * KItemListView::setWidgetCreator().
 *
 * @see KItemListWidget
 * @see KItemModelBase
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListView : public QGraphicsWidget
{
    Q_OBJECT

    Q_PROPERTY(qreal offset READ offset WRITE setOffset)

public:
    KItemListView(QGraphicsWidget* parent = 0);
    virtual ~KItemListView();

    void setScrollOrientation(Qt::Orientation orientation);
    Qt::Orientation scrollOrientation() const;

    void setItemSize(const QSizeF& size);
    QSizeF itemSize() const;

    // TODO: add note that offset is not checked against maximumOffset, only against 0.
    void setOffset(qreal offset);
    qreal offset() const;

    qreal maximumOffset() const;

    /**
     * Sets the visible roles to \p roles. The integer-value defines
     * the order of the visible role: Smaller values are ordered first.
     */
    void setVisibleRoles(const QHash<QByteArray, int>& roles);
    QHash<QByteArray, int> visibleRoles() const;

    /**
     * @return Controller of the item-list. The controller gets
     *         initialized by KItemListController::setView() and will
     *         result in calling KItemListController::onControllerChanged().
     */
    KItemListController* controller() const;

    /**
     * @return Model of the item-list. The model gets
     *         initialized by KItemListController::setView() and will
     *         result in calling KItemListController::onModelChanged().
     */
    KItemModelBase* model() const;

    /**
     * Sets the creator that creates a widget showing the
     * content of one model-item. Usually it is sufficient
     * to implement a custom widget X derived from KItemListWidget and
     * set the creator by:
     * <code>
     * itemListView->setWidgetCreator(new KItemListWidgetCreator<X>());
     * </code>
     * Note that the ownership of the widget creator is not transferred to
     * the item-list view: One instance of a widget creator might get shared
     * by several item-list view instances.
     **/
    void setWidgetCreator(KItemListWidgetCreatorBase* widgetCreator);
    KItemListWidgetCreatorBase* widgetCreator() const;

    void setGroupHeaderCreator(KItemListGroupHeaderCreatorBase* groupHeaderCreator);
    KItemListGroupHeaderCreatorBase* groupHeaderCreator() const;

    void setStyleOption(const KItemListStyleOption& option);
    const KItemListStyleOption& styleOption() const;

    virtual void setGeometry(const QRectF& rect);

    int itemAt(const QPointF& pos) const;
    bool isAboveSelectionToggle(int index, const QPointF& pos) const;
    bool isAboveExpansionToggle(int index, const QPointF& pos) const;

    int firstVisibleIndex() const;
    int lastVisibleIndex() const;

    virtual QSizeF itemSizeHint(int index) const;
    virtual QHash<QByteArray, QSizeF> visibleRoleSizes() const;

    /**
     * @return The bounding rectangle of the item relative to the top/left of
     *         the currently visible area (see KItemListView::offset()).
     */
    QRectF itemBoundingRect(int index) const;

    /**
     * @return The number of items that can be shown in parallel for one offset.
     *         Assuming the scrolldirection is vertical then a value of 4 means
     *         that 4 columns for items are available. Assuming the scrolldirection
     *         is horizontal then a value of 4 means that 4 lines for items are
     *         available.
     */
    int itemsPerOffset() const;

    void beginTransaction();
    void endTransaction();
    bool isTransactionActive() const;

    /**
     * @return Pixmap that is used for a drag operation based on the
     *         items given by \a indexes. The default implementation returns
     *         a null-pixmap.
     */
    virtual QPixmap createDragPixmap(const QSet<int>& indexes) const;

    /**
     * @reimp
     */
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

signals:
    void offsetChanged(qreal current, qreal previous);
    void maximumOffsetChanged(qreal current, qreal previous);
    void scrollTo(qreal newOffset);

protected:
    virtual void initializeItemListWidget(KItemListWidget* item);

    virtual void onControllerChanged(KItemListController* current, KItemListController* previous);
    virtual void onModelChanged(KItemModelBase* current, KItemModelBase* previous);

    virtual void onScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous);
    virtual void onItemSizeChanged(const QSizeF& current, const QSizeF& previous);
    virtual void onOffsetChanged(qreal current, qreal previous);
    virtual void onVisibleRolesChanged(const QHash<QByteArray, int>& current, const QHash<QByteArray, int>& previous);
    virtual void onStyleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous);

    virtual void onTransactionBegin();
    virtual void onTransactionEnd();

    virtual bool event(QEvent* event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    virtual void dragEnterEvent(QGraphicsSceneDragDropEvent* event);

    QList<KItemListWidget*> visibleItemListWidgets() const;

protected slots:
    virtual void slotItemsInserted(const KItemRangeList& itemRanges);
    virtual void slotItemsRemoved(const KItemRangeList& itemRanges);
    virtual void slotItemsChanged(const KItemRangeList& itemRanges,
                                  const QSet<QByteArray>& roles);

private slots:
    void slotCurrentChanged(int current, int previous);
    void slotSelectionChanged(const QSet<int>& current, const QSet<int>& previous);
    void slotAnimationFinished(QGraphicsWidget* widget,
                               KItemListViewAnimation::AnimationType type);
    void slotLayoutTimerFinished();

    void slotRubberBandStartPosChanged();
    void slotRubberBandEndPosChanged();
    void slotRubberBandActivationChanged(bool active);

    /**
     * Emits the signal scrollTo() with the corresponding target offset if the current
     * mouse position is above the autoscroll-margin.
     */
    void triggerAutoScrolling();

private:
    enum LayoutAnimationHint
    {
        NoAnimation,
        Animation
    };

    enum SizeType
    {
        LayouterSize,
        ItemSize
    };

    void setController(KItemListController* controller);
    void setModel(KItemModelBase* model);

    KItemListRubberBand* rubberBand() const;

    void updateLayout();
    void doLayout(LayoutAnimationHint hint, int changedIndex, int changedCount);
    void doGroupHeadersLayout(LayoutAnimationHint hint, int changedIndex, int changedCount);
    void emitOffsetChanges();

    KItemListWidget* createWidget(int index);
    void recycleWidget(KItemListWidget* widget);
    void setWidgetIndex(KItemListWidget* widget, int index);

    /**
     * Helper method for setGeometry() and setItemSize(): Calling both methods might result
     * in a changed number of visible items. To assure that currently invisible items can
     * get animated from the old position to the new position prepareLayoutForIncreasedItemCount()
     * takes care to create all item widgets that are visible with the old or the new size.
     * @param size     Size of the layouter or the item dependent on \p sizeType.
     * @param sizeType LayouterSize: KItemListLayouter::setSize() is used.
     *                 ItemSize: KItemListLayouter::setItemSize() is used.
     */
    void prepareLayoutForIncreasedItemCount(const QSizeF& size, SizeType sizeType);

    /**
     * Helper method for prepareLayoutForIncreasedItemCount().
     */
    void setLayouterSize(const QSizeF& size, SizeType sizeType);

    /**
     * Marks the visible roles as dirty so that they will get updated when doing the next
     * layout. The visible roles will only get marked as dirty if an empty item-size is
     * given.
     * @return True if the visible roles have been marked as dirty.
     */
    bool markVisibleRolesSizesAsDirty();

    /**
     * Updates the m_visibleRoleSizes property and applies the dynamic
     * size to the layouter.
     */
    void applyDynamicItemSize();

    /**
     * Helper method for createWidget() and setWidgetIndex() to update the properties
     * of the itemlist widget.
     */
    void updateWidgetProperties(KItemListWidget* widget, int index);

    /**
     * Helper function for triggerAutoScrolling(). Returns the scroll increment
     * that should be added to the offset() based on the available size \a size
     * and the current mouse position \a pos. As soon as \a pos is inside
     * the autoscroll-margin a value != 0 will be returned.
     */
    static int calculateAutoScrollingIncrement(int pos, int size);

private:
    bool m_autoScrollMarginEnabled;
    bool m_grouped;
    int m_activeTransactions; // Counter for beginTransaction()/endTransaction()

    QSizeF m_itemSize;
    KItemListController* m_controller;
    KItemModelBase* m_model;
    QHash<QByteArray, int> m_visibleRoles;
    QHash<QByteArray, QSizeF> m_visibleRolesSizes;
    KItemListWidgetCreatorBase* m_widgetCreator;
    KItemListGroupHeaderCreatorBase* m_groupHeaderCreator;
    KItemListStyleOption m_styleOption;

    QHash<int, KItemListWidget*> m_visibleItems;
    QHash<KItemListWidget*, KItemListGroupHeader*> m_visibleGroups;

    int m_scrollBarExtent;
    KItemListSizeHintResolver* m_sizeHintResolver;
    KItemListViewLayouter* m_layouter;
    KItemListViewAnimation* m_animation;

    QTimer* m_layoutTimer; // Triggers an asynchronous doLayout() call.
    qreal m_oldOffset;
    qreal m_oldMaximumOffset;

    KItemListRubberBand* m_rubberBand;

    QPointF m_mousePos;

    friend class KItemListController;
};

/**
 * Allows to do a fast logical creation and deletion of QGraphicsWidgets
 * by recycling existing QGraphicsWidgets instances. Is used by
 * KItemListWidgetCreatorBase and KItemListGroupHeaderCreatorBase.
 * @internal
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListCreatorBase
{
public:
    virtual ~KItemListCreatorBase();

protected:
    void addCreatedWidget(QGraphicsWidget* widget);
    void pushRecycleableWidget(QGraphicsWidget* widget);
    QGraphicsWidget* popRecycleableWidget();

private:
    QSet<QGraphicsWidget*> m_createdWidgets;
    QList<QGraphicsWidget*> m_recycleableWidgets;
};

/**
 * @brief Base class for creating KItemListWidgets.
 *
 * It is recommended that applications simply use the KItemListWidgetCreator-template class.
 * For a custom implementation the methods create() and recyle() must be reimplemented.
 * The intention of the widget creator is to prevent repetitive and expensive instantiations and
 * deletions of KItemListWidgets by recycling existing widget instances.
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListWidgetCreatorBase : public KItemListCreatorBase
{
public:
    virtual ~KItemListWidgetCreatorBase();
    virtual KItemListWidget* create(KItemListView* view) = 0;
    virtual void recycle(KItemListWidget* widget);
};

template <class T>
class LIBDOLPHINPRIVATE_EXPORT KItemListWidgetCreator : public KItemListWidgetCreatorBase
{
public:
    virtual ~KItemListWidgetCreator();
    virtual KItemListWidget* create(KItemListView* view);
};

template <class T>
KItemListWidgetCreator<T>::~KItemListWidgetCreator()
{
}

template <class T>
KItemListWidget* KItemListWidgetCreator<T>::create(KItemListView* view)
{
    KItemListWidget* widget = static_cast<KItemListWidget*>(popRecycleableWidget());
    if (!widget) {
        widget = new T(view);
        addCreatedWidget(widget);
    }
    return widget;
}

/**
 * @brief Base class for creating KItemListGroupHeaders.
 *
 * It is recommended that applications simply use the KItemListGroupHeaderCreator-template class.
 * For a custom implementation the methods create() and recyle() must be reimplemented.
 * The intention of the group-header creator is to prevent repetitive and expensive instantiations and
 * deletions of KItemListGroupHeaders by recycling existing header instances.
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListGroupHeaderCreatorBase : public KItemListCreatorBase
{
public:
    virtual ~KItemListGroupHeaderCreatorBase();
    virtual KItemListGroupHeader* create(QGraphicsWidget* parent) = 0;
    virtual void recycle(KItemListGroupHeader* header);
};

template <class T>
class LIBDOLPHINPRIVATE_EXPORT KItemListGroupHeaderCreator : public KItemListGroupHeaderCreatorBase
{
public:
    virtual ~KItemListGroupHeaderCreator();
    virtual KItemListGroupHeader* create(QGraphicsWidget* parent);
};

template <class T>
KItemListGroupHeaderCreator<T>::~KItemListGroupHeaderCreator()
{
}

template <class T>
KItemListGroupHeader* KItemListGroupHeaderCreator<T>::create(QGraphicsWidget* parent)
{
    KItemListGroupHeader* widget = static_cast<KItemListGroupHeader*>(popRecycleableWidget());
    if (!widget) {
        widget = new T(parent);
        addCreatedWidget(widget);
    }
    return widget;
}

#endif
