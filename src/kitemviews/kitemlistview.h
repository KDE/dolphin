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
class KItemListGroupHeader;
class KItemListGroupHeaderCreatorBase;
class KItemListHeader;
class KItemListSizeHintResolver;
class KItemListRubberBand;
class KItemListViewAnimation;
class KItemListViewLayouter;
class KItemListWidget;
class KItemListWidgetCreatorBase;
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

    Q_PROPERTY(qreal scrollOffset READ scrollOffset WRITE setScrollOffset)

public:
    KItemListView(QGraphicsWidget* parent = 0);
    virtual ~KItemListView();

    void setScrollOrientation(Qt::Orientation orientation);
    Qt::Orientation scrollOrientation() const;

    void setItemSize(const QSizeF& size);
    QSizeF itemSize() const;

    // TODO: add note that offset is not checked against maximumOffset, only against 0.
    void setScrollOffset(qreal offset);
    qreal scrollOffset() const;

    qreal maximumScrollOffset() const;

    void setItemOffset(qreal scrollOffset);
    qreal itemOffset() const;

    qreal maximumItemOffset() const;

    void setVisibleRoles(const QList<QByteArray>& roles);
    QList<QByteArray> visibleRoles() const;

    /**
     * If set to true an automatic scrolling is done as soon as the
     * mouse is moved near the borders of the view. Per default
     * the automatic scrolling is turned off.
     */
    void setAutoScroll(bool enabled);
    bool autoScroll() const;

    /**
     * Turns on the header if \p show is true. Per default the
     * header is not shown.
     */
    void setHeaderShown(bool show);
    bool isHeaderShown() const;

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

    /** @reimp */
    virtual void setGeometry(const QRectF& rect);

    int itemAt(const QPointF& pos) const;
    bool isAboveSelectionToggle(int index, const QPointF& pos) const;
    bool isAboveExpansionToggle(int index, const QPointF& pos) const;

    int firstVisibleIndex() const;
    int lastVisibleIndex() const;

    /**
     * @return Required size for the item with the index \p index.
     *         Per default KItemListView::itemSize() is returned.
     *         When reimplementing this method it is recommended to
     *         also reimplement KItemListView::itemSizeHintUpdateRequired().
     */
    virtual QSizeF itemSizeHint(int index) const;

    /**
     * @param itemRanges Items that must be checked for getting the visible roles sizes.
     * @return The size of each visible role in case if KItemListView::itemSize()
     *         is empty. This allows to have dynamic but equal role sizes between
     *         all items. Per default an empty hash is returned.
     */
    virtual QHash<QByteArray, QSizeF> visibleRolesSizes(const KItemRangeList& itemRanges) const;

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
    void scrollOffsetChanged(qreal current, qreal previous);
    void maximumScrollOffsetChanged(qreal current, qreal previous);
    void itemOffsetChanged(qreal current, qreal previous);
    void maximumItemOffsetChanged(qreal current, qreal previous);
    void scrollTo(qreal newOffset);

protected:
    virtual void initializeItemListWidget(KItemListWidget* item);

    /**
     * @return True if at least one of the changed roles \p changedRoles might result
     *         in the need to update the item-size hint (see KItemListView::itemSizeHint()).
     *         Per default true is returned which means on each role-change of existing items
     *         the item-size hints are recalculated. For performance reasons it is recommended
     *         to return false in case if a role-change will not result in a changed
     *         item-size hint.
     */
    virtual bool itemSizeHintUpdateRequired(const QSet<QByteArray>& changedRoles) const;

    virtual void onControllerChanged(KItemListController* current, KItemListController* previous);
    virtual void onModelChanged(KItemModelBase* current, KItemModelBase* previous);

    virtual void onScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous);
    virtual void onItemSizeChanged(const QSizeF& current, const QSizeF& previous);
    virtual void onScrollOffsetChanged(qreal current, qreal previous);
    virtual void onVisibleRolesChanged(const QList<QByteArray>& current, const QList<QByteArray>& previous);
    virtual void onStyleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous);

    virtual void onTransactionBegin();
    virtual void onTransactionEnd();

    virtual bool event(QEvent* event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    virtual void dragEnterEvent(QGraphicsSceneDragDropEvent* event);
    virtual void dragMoveEvent(QGraphicsSceneDragDropEvent* event);
    virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent* event);
    virtual void dropEvent(QGraphicsSceneDragDropEvent* event);

    QList<KItemListWidget*> visibleItemListWidgets() const;

    /** @reimp */
    virtual void resizeEvent(QGraphicsSceneResizeEvent* event);

protected slots:
    virtual void slotItemsInserted(const KItemRangeList& itemRanges);
    virtual void slotItemsRemoved(const KItemRangeList& itemRanges);
    virtual void slotItemsMoved(const KItemRange& itemRange, const QList<int>& movedToIndexes);
    virtual void slotItemsChanged(const KItemRangeList& itemRanges,
                                  const QSet<QByteArray>& roles);

private slots:
    void slotCurrentChanged(int current, int previous);
    void slotSelectionChanged(const QSet<int>& current, const QSet<int>& previous);
    void slotAnimationFinished(QGraphicsWidget* widget,
                               KItemListViewAnimation::AnimationType type);
    void slotLayoutTimerFinished();

    void slotRubberBandPosChanged();
    void slotRubberBandActivationChanged(bool active);

    /**
     * Is invoked if the visible role-width of one role in the header has
     * been changed by the user. It is remembered that the user has modified
     * the role-width, so that it won't be changed anymore automatically to
     * calculate an optimized width.
     */
    void slotVisibleRoleWidthChanged(const QByteArray& role,
                                     qreal currentWidth,
                                     qreal previousWidth);

    /**
     * Triggers the autoscrolling if autoScroll() is enabled by checking the
     * current mouse position. If the mouse position is within the autoscroll
     * margins a timer will be started that periodically triggers the autoscrolling.
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
     * Helper method for createWidget() and setWidgetIndex() to update the properties
     * of the itemlist widget.
     */
    void updateWidgetProperties(KItemListWidget* widget, int index);

    /**
     * Updates the width of the KItemListHeader corresponding to the required width of
     * the roles.
     */
    void updateHeaderWidth();

    /**
     * @return The widths of each visible role that is shown in the KItemListHeader.
     */
    QHash<QByteArray, qreal> headerRolesWidths() const;

    /**
     * Updates m_visibleRolesSizes by calling KItemListView::visibleRolesSizes().
     * Nothing will be done if m_itemRect is not empty or custom header-widths
     * are used (see m_useHeaderWidths). Also m_strechedVisibleRolesSizes will be adjusted
     * to respect the available view-size.
     */
    void updateVisibleRolesSizes(const KItemRangeList& itemRanges);

    /**
     * Convenience method for updateVisibleRoleSizes(KItemRangeList() << KItemRange(0, m_model->count()).
     */
    void updateVisibleRolesSizes();

    /**
     * Updates m_stretchedVisibleRolesSizes based on m_visibleRolesSizes and the available
     * view-size. Nothing will be done if m_itemRect is not empty or custom header-widths
     * are used (see m_useHeaderWidths).
     */
    void updateStretchedVisibleRolesSizes();

    /**
     * Helper function for triggerAutoScrolling().
     * @param pos    Logical position of the mouse relative to the range.
     * @param range  Range of the visible area.
     * @param oldInc Previous increment. Is used to assure that the increment
     *               increases only gradually.
     * @return Scroll increment that should be added to the offset().
     *         As soon as \a pos is inside the autoscroll-margin a
     *         value != 0 will be returned.
     */
    static int calculateAutoScrollingIncrement(int pos, int range, int oldInc);

private:
    bool m_grouped;
    int m_activeTransactions; // Counter for beginTransaction()/endTransaction()

    QSizeF m_itemSize;
    KItemListController* m_controller;
    KItemModelBase* m_model;
    QList<QByteArray> m_visibleRoles;
    QHash<QByteArray, QSizeF> m_visibleRolesSizes;
    QHash<QByteArray, QSizeF> m_stretchedVisibleRolesSizes;
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
    qreal m_oldScrollOffset;
    qreal m_oldMaximumScrollOffset;
    qreal m_oldItemOffset;
    qreal m_oldMaximumItemOffset;

    bool m_skipAutoScrollForRubberBand;
    KItemListRubberBand* m_rubberBand;

    QPointF m_mousePos;
    int m_autoScrollIncrement;
    QTimer* m_autoScrollTimer;

    KItemListHeader* m_header;
    bool m_useHeaderWidths;

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
