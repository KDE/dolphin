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

#include <kitemviews/kstandarditemlistgroupheader.h>
#include <kitemviews/kitemliststyleoption.h>
#include <kitemviews/kitemlistwidget.h>
#include <kitemviews/kitemmodelbase.h>
#include <kitemviews/private/kitemlistviewanimation.h>
#include <QGraphicsWidget>
#include <QSet>

class KItemListController;
class KItemListGroupHeaderCreatorBase;
class KItemListHeader;
class KItemListHeaderWidget;
class KItemListSizeHintResolver;
class KItemListRubberBand;
class KItemListViewAnimation;
class KItemListViewLayouter;
class KItemListWidget;
class KItemListWidgetInformant;
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
 * KItemListController::setView() or with the constructor of
 * KItemListController.
 *
 * @see KItemListWidget
 * @see KItemModelBase
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListView : public QGraphicsWidget
{
    Q_OBJECT

    Q_PROPERTY(qreal scrollOffset READ scrollOffset WRITE setScrollOffset)
    Q_PROPERTY(qreal itemOffset READ itemOffset WRITE setItemOffset)

public:
    KItemListView(QGraphicsWidget* parent = 0);
    virtual ~KItemListView();

    /**
     * Offset of the scrollbar that represents the scroll-orientation
     * (see setScrollOrientation()).
     */
    void setScrollOffset(qreal offset);
    qreal scrollOffset() const;

    qreal maximumScrollOffset() const;

    /**
     * Offset related to an item, that does not fit into the available
     * size of the listview. If the scroll-orientation is vertical
     * the item-offset describes the offset of the horizontal axe, if
     * the scroll-orientation is horizontal the item-offset describes
     * the offset of the vertical axe.
     */
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
     * If set to true selection-toggles will be shown when hovering
     * an item. Per default the selection-toggles are disabled.
     */
    void setEnabledSelectionToggles(bool enabled);
    bool enabledSelectionToggles() const;

    /**
     * @return Controller of the item-list. The controller gets
     *         initialized by KItemListController::setView() and will
     *         result in calling KItemListController::onControllerChanged().
     */
    KItemListController* controller() const;

    /**
     * @return Model of the item-list. The model gets
     *         initialized by KItemListController::setModel() and will
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
     * The ownership of the widget creator is transferred to
     * the item-list view.
     **/
    void setWidgetCreator(KItemListWidgetCreatorBase* widgetCreator);
    KItemListWidgetCreatorBase* widgetCreator() const;

    /**
     * Sets the creator that creates a group header. Usually it is sufficient
     * to implement a custom header widget X derived from KItemListGroupHeader and
     * set the creator by:
     * <code>
     * itemListView->setGroupHeaderCreator(new KItemListGroupHeaderCreator<X>());
     * </code>
     * The ownership of the gropup header creator is transferred to
     * the item-list view.
     **/
    void setGroupHeaderCreator(KItemListGroupHeaderCreatorBase* groupHeaderCreator);
    KItemListGroupHeaderCreatorBase* groupHeaderCreator() const;

    /**
     * @return The basic size of all items. The size of an item may be larger than
     *         the basic size (see KItemListView::itemSizeHint() and KItemListView::itemRect()).
     */
    QSizeF itemSize() const;

    const KItemListStyleOption& styleOption() const;

    /** @reimp */
    virtual void setGeometry(const QRectF& rect);

    /**
     * @return Index of the item that is below the point \a pos.
     *         The position is relative to the upper right of
     *         the visible area. Only (at least partly) visible
     *         items are considered. -1 is returned if no item is
     *         below the position.
     */
    int itemAt(const QPointF& pos) const;
    bool isAboveSelectionToggle(int index, const QPointF& pos) const;
    bool isAboveExpansionToggle(int index, const QPointF& pos) const;

    /**
     * @return Index of the first item that is at least partly visible.
     *         -1 is returned if the model contains no items.
     */
    int firstVisibleIndex() const;

    /**
     * @return Index of the last item that is at least partly visible.
     *         -1 is returned if the model contains no items.
     */
    int lastVisibleIndex() const;

    /**
     * @return Required size for the item with the index \p index.
     *         The returned value might be larger than KItemListView::itemSize().
     *         In this case the layout grid will be stretched to assure an
     *         unclipped item.
     */
    QSizeF itemSizeHint(int index) const;

    /**
     * If set to true, items having child-items can be expanded to show the child-items as
     * part of the view. Per default the expanding of items is is disabled. If expanding of
     * items is enabled, the methods KItemModelBase::setExpanded(), KItemModelBase::isExpanded(),
     * KItemModelBase::isExpandable() and KItemModelBase::expandedParentsCount()
     * must be reimplemented. The view-implementation
     * has to take care itself how to visually represent the expanded items provided
     * by the model.
     */
    void setSupportsItemExpanding(bool supportsExpanding);
    bool supportsItemExpanding() const;

    /**
     * @return The rectangle of the item relative to the top/left of
     *         the currently visible area (see KItemListView::offset()).
     */
    QRectF itemRect(int index) const;

    /**
     * @return The context rectangle of the item relative to the top/left of
     *         the currently visible area (see KItemListView::offset()). The
     *         context rectangle is defined by by the united rectangle of
     *         the icon rectangle and the text rectangle (see KItemListWidget::iconRect()
     *         and KItemListWidget::textRect()) and is useful as reference for e.g. aligning
     *         a tooltip or a context-menu for an item. Note that a context rectangle will
     *         only be returned for (at least partly) visible items. An empty rectangle will
     *         be returned for fully invisible items.
     */
    QRectF itemContextRect(int index) const;

    /**
     * Scrolls to the item with the index \a index so that the item
     * will be fully visible.
     */
    void scrollToItem(int index);

    /**
     * If several properties of KItemListView are changed synchronously, it is
     * recommended to encapsulate the calls between beginTransaction() and endTransaction().
     * This prevents unnecessary and expensive layout-calculations.
     */
    void beginTransaction();

    /**
     * Counterpart to beginTransaction(). The layout changes will only be animated if
     * all property changes between beginTransaction() and endTransaction() support
     * animations.
     */
    void endTransaction();

    bool isTransactionActive() const;

    /**
     * Turns on the header if \p visible is true. Per default the
     * header is not visible. Usually the header is turned on when
     * showing a classic "table-view" to describe the shown columns.
     */
    void setHeaderVisible(bool visible);
    bool isHeaderVisible() const;

    /**
     * @return Header of the list. The header is also available if it is not shown
     *         (see KItemListView::setHeaderShown()).
     */
    KItemListHeader* header() const;

    /**
     * @return Pixmap that is used for a drag operation based on the
     *         items given by \a indexes.
     */
    virtual QPixmap createDragPixmap(const QSet<int>& indexes) const;

    /**
     * Lets the user edit the role \a role for item with the index \a index.
     */
    void editRole(int index, const QByteArray& role);

    /**
     * @reimp
     */
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

signals:
    void scrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous);
    void scrollOffsetChanged(qreal current, qreal previous);
    void maximumScrollOffsetChanged(qreal current, qreal previous);
    void itemOffsetChanged(qreal current, qreal previous);
    void maximumItemOffsetChanged(qreal current, qreal previous);
    void scrollTo(qreal newOffset);

    /**
     * Is emitted if the user has changed the sort order by clicking on a
     * header item (see KItemListView::setHeaderShown()). The sort order
     * of the model has already been adjusted to
     * the current sort order. Note that no signal will be emitted if the
     * sort order of the model has been changed without user interaction.
     */
    void sortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous);

    /**
     * Is emitted if the user has changed the sort role by clicking on a
     * header item (see KItemListView::setHeaderShown()). The sort role
     * of the model has already been adjusted to
     * the current sort role. Note that no signal will be emitted if the
     * sort role of the model has been changed without user interaction.
     */
    void sortRoleChanged(const QByteArray& current, const QByteArray& previous);

    /**
     * Is emitted if the user has changed the visible roles by moving a header
     * item (see KItemListView::setHeaderShown()). Note that no signal will be
     * emitted if the roles have been changed without user interaction by
     * KItemListView::setVisibleRoles().
     */
    void visibleRolesChanged(const QList<QByteArray>& current, const QList<QByteArray>& previous);

    void roleEditingCanceled(int index, const QByteArray& role, const QVariant& value);
    void roleEditingFinished(int index, const QByteArray& role, const QVariant& value);

protected:
    void setItemSize(const QSizeF& size);
    void setStyleOption(const KItemListStyleOption& option);

    /**
     * If the scroll-orientation is vertical, the items are ordered
     * from top to bottom (= default setting). If the scroll-orientation
     * is horizontal, the items are ordered from left to right.
     */
    void setScrollOrientation(Qt::Orientation orientation);
    Qt::Orientation scrollOrientation() const;

    /**
     * Factory method for creating a default widget-creator. The method will be used
     * in case if setWidgetCreator() has not been set by the application.
     * @return New instance of the widget-creator that should be used per
     *         default.
     */
    virtual KItemListWidgetCreatorBase* defaultWidgetCreator() const;

    /**
     * Factory method for creating a default group-header-creator. The method will be used
     * in case if setGroupHeaderCreator() has not been set by the application.
     * @return New instance of the group-header-creator that should be used per
     *         default.
     */
    virtual KItemListGroupHeaderCreatorBase* defaultGroupHeaderCreator() const;

    /**
     * Is called when creating a new KItemListWidget instance and allows derived
     * classes to do a custom initialization.
     */
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
    virtual void onSupportsItemExpandingChanged(bool supportsExpanding);

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

protected slots:
    virtual void slotItemsInserted(const KItemRangeList& itemRanges);
    virtual void slotItemsRemoved(const KItemRangeList& itemRanges);
    virtual void slotItemsMoved(const KItemRange& itemRange, const QList<int>& movedToIndexes);
    virtual void slotItemsChanged(const KItemRangeList& itemRanges,
                                  const QSet<QByteArray>& roles);

    virtual void slotGroupedSortingChanged(bool current);
    virtual void slotSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous);
    virtual void slotSortRoleChanged(const QByteArray& current, const QByteArray& previous);
    virtual void slotCurrentChanged(int current, int previous);
    virtual void slotSelectionChanged(const QSet<int>& current, const QSet<int>& previous);

private slots:
    void slotAnimationFinished(QGraphicsWidget* widget,
                               KItemListViewAnimation::AnimationType type);
    void slotLayoutTimerFinished();

    void slotRubberBandPosChanged();
    void slotRubberBandActivationChanged(bool active);

    /**
     * Is invoked if the column-width of one role in the header has
     * been changed by the user. The automatic resizing of columns
     * will be turned off as soon as this method has been called at
     * least once.
     */
    void slotHeaderColumnWidthChanged(const QByteArray& role,
                                      qreal currentWidth,
                                      qreal previousWidth);

    /**
     * Is invoked if a column has been moved by the user. Applies
     * the moved role to the view.
     */
    void slotHeaderColumnMoved(const QByteArray& role,
                               int currentIndex,
                               int previousIndex);

    /**
     * Triggers the autoscrolling if autoScroll() is enabled by checking the
     * current mouse position. If the mouse position is within the autoscroll
     * margins a timer will be started that periodically triggers the autoscrolling.
     */
    void triggerAutoScrolling();

    /**
     * Is invoked if the geometry of the parent-widget from a group-header has been
     * changed. The x-position and width of the group-header gets adjusted to assure
     * that it always spans the whole width even during temporary transitions of the
     * parent widget.
     */
    void slotGeometryOfGroupHeaderParentChanged();

    void slotRoleEditingCanceled(int index, const QByteArray& role, const QVariant& value);
    void slotRoleEditingFinished(int index, const QByteArray& role, const QVariant& value);

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

    void doLayout(LayoutAnimationHint hint, int changedIndex = 0, int changedCount = 0);

    /**
     * Helper method for doLayout: Returns a list of items that can be reused for the visible
     * area. Invisible group headers get recycled. The reusable items are items that are
     * invisible. If the animation hint is 'Animation' then items that are currently animated
     * won't be reused. Reusing items is faster in comparison to deleting invisible
     * items and creating a new instance for visible items.
     */
    QList<int> recycleInvisibleItems(int firstVisibleIndex,
                                     int lastVisibleIndex,
                                     LayoutAnimationHint hint);

    /**
     * Helper method for doLayout: Starts a moving-animation for the widget to the given
     * new position. The moving-animation is only started if the new position is within
     * the same row or column, otherwise the create-animation is used instead.
     * @return True if the moving-animation has been applied.
     */
    bool moveWidget(KItemListWidget* widget, const QPointF& newPos);

    void emitOffsetChanges();

    KItemListWidget* createWidget(int index);
    void recycleWidget(KItemListWidget* widget);

    /**
     * Changes the index of the widget to \a index and assures a consistent
     * update for m_visibleItems and m_visibleCells. The cell-information
     * for the new index will not be updated and be initialized as empty cell.
     */
    void setWidgetIndex(KItemListWidget* widget, int index);

    /**
     * Changes the index of the widget to \a index. In opposite to
     * setWidgetIndex() the cell-information for the widget gets updated.
     * This update gives doLayout() the chance to animate the moving
     * of the item visually (see moveWidget()).
     */
    void moveWidgetToIndex(KItemListWidget* widget, int index);

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
     * Helper method for updateWidgetPropertes() to create or update
     * the itemlist group-header.
     */
    void updateGroupHeaderForWidget(KItemListWidget* widget);

    /**
     * Updates the position and size of the group-header that belongs
     * to the itemlist widget \a widget. The given widget must represent
     * the first item of a group.
     */
    void updateGroupHeaderLayout(KItemListWidget* widget);

    /**
     * Recycles the group-header for the widget.
     */
    void recycleGroupHeaderForWidget(KItemListWidget* widget);

    /**
     * Helper method for slotGroupedSortingChanged(), slotSortOrderChanged()
     * and slotSortRoleChanged(): Iterates through all visible items and updates
     * the group-header widgets.
     */
    void updateVisibleGroupHeaders();

    /**
     * @return Index for the item in the list returned by KItemModelBase::groups()
     *         that represents the group where the item with the index \a index
     *         belongs to. -1 is returned if no groups are available.
     */
    int groupIndexForItem(int index) const;

    /**
     * Updates the alternate background for all visible items.
     * @see updateAlternateBackgroundForWidget()
     */
    void updateAlternateBackgrounds();

    /**
     * Updates the alternateBackground-property of the widget dependent
     * on the state of useAlternateBackgrounds() and the grouping state.
     */
    void updateAlternateBackgroundForWidget(KItemListWidget* widget);

    /**
     * @return True if alternate backgrounds should be used for the items.
     *         This is the case if an empty item-size is given and if there
     *         is more than one visible role.
     */
    bool useAlternateBackgrounds() const;

    /**
     * @param itemRanges Items that must be checked for getting the widths of columns.
     * @return           The preferred width of the column of each visible role. The width will
     *                   be respected if the width of the item size is <= 0 (see
     *                   KItemListView::setItemSize()). Per default an empty hash
     *                   is returned.
     */
    QHash<QByteArray, qreal> preferredColumnWidths(const KItemRangeList& itemRanges) const;

    /**
     * Applies the column-widths from m_headerWidget to the layout
     * of the view.
     */
    void applyColumnWidthsFromHeader();

    /**
     * Applies the column-widths from m_headerWidget to \a widget.
     */
    void updateWidgetColumnWidths(KItemListWidget* widget);

    /**
     * Updates the preferred column-widths of m_groupHeaderWidget by
     * invoking KItemListView::columnWidths().
     */
    void updatePreferredColumnWidths(const KItemRangeList& itemRanges);

    /**
     * Convenience method for
     * updatePreferredColumnWidths(KItemRangeList() << KItemRange(0, m_model->count()).
     */
    void updatePreferredColumnWidths();

    /**
     * Resizes the column-widths of m_headerWidget based on the preferred widths
     * and the vailable view-size.
     */
    void applyAutomaticColumnWidths();

    /**
     * @return Sum of the widths of all columns.
     */
    qreal columnWidthsSum() const;

    /**
     * @return Boundaries of the header. An empty rectangle is returned
     *         if no header is shown.
     */
    QRectF headerBoundaries() const;

    /**
     * @return True if the number of columns or rows will be changed when applying
     *         the new grid- and item-size. Used to determine whether an animation
     *         should be done when applying the new layout.
     */
    bool changesItemGridLayout(const QSizeF& newGridSize,
                               const QSizeF& newItemSize,
                               const QSizeF& newItemMargin) const;

    /**
     * @param changedItemCount Number of inserted  or removed items.
     * @return                 True if the inserting or removing of items should be animated.
     *                         No animation should be done if the number of items is too large
     *                         to provide a pleasant animation.
     */
    bool animateChangedItemCount(int changedItemCount) const;

    /**
     * @return True if a scrollbar for the given scroll-orientation is required
     *         when using a size of \p size for the view. Calling the method is rather
     *         expansive as a temporary relayout needs to be done.
     */
    bool scrollBarRequired(const QSizeF& size) const;

    /**
     * Shows a drop-indicator between items dependent on the given
     * cursor position. The cursor position is relative the the upper left
     * edge of the view.
     * @return Index of the item where the dropping is done. An index of -1
     *         indicates that the item has been dropped after the last item.
     */
    int showDropIndicator(const QPointF& pos);
    void hideDropIndicator();

    /**
     * Applies the height of the group header to the layouter. The height
     * depends on the used scroll orientation.
     */
    void updateGroupHeaderHeight();

    /**
     * Updates the siblings-information for all visible items that are inside
     * the range of \p firstIndex and \p lastIndex. If firstIndex or lastIndex
     * is smaller than 0, the siblings-information for all visible items gets
     * updated.
     * @see KItemListWidget::setSiblingsInformation()
     */
    void updateSiblingsInformation(int firstIndex = -1, int lastIndex = -1);

    /**
     * Helper method for updateExpansionIndicators().
     * @return True if the item with the index \a index has a sibling successor
     *         (= the item is not the last item of the current hierarchy).
     */
    bool hasSiblingSuccessor(int index) const;

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

    /**
     * Helper functions for changesItemCount().
     * @return The number of items that fit into the available size by
     *         respecting the size of the item and the margin between the items.
     */
    static int itemsPerSize(qreal size, qreal itemSize, qreal itemMargin);

private:
    bool m_enabledSelectionToggles;
    bool m_grouped;
    bool m_supportsItemExpanding;
    bool m_editingRole;
    int m_activeTransactions; // Counter for beginTransaction()/endTransaction()
    LayoutAnimationHint m_endTransactionAnimationHint;

    QSizeF m_itemSize;
    KItemListController* m_controller;
    KItemModelBase* m_model;
    QList<QByteArray> m_visibleRoles;
    mutable KItemListWidgetCreatorBase* m_widgetCreator;
    mutable KItemListGroupHeaderCreatorBase* m_groupHeaderCreator;
    KItemListStyleOption m_styleOption;

    QHash<int, KItemListWidget*> m_visibleItems;
    QHash<KItemListWidget*, KItemListGroupHeader*> m_visibleGroups;

    struct Cell
    {
        Cell() : column(-1), row(-1) {}
        Cell(int c, int r) : column(c), row(r) {}
        int column;
        int row;
    };
    QHash<int, Cell> m_visibleCells;

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
    KItemListHeaderWidget* m_headerWidget;

    // When dragging items into the view where the sort-role of the model
    // is empty, a visual indicator should be shown during dragging where
    // the dropping will happen. This indicator is specified by an index
    // of the item. -1 means that no indicator will be shown at all.
    // The m_dropIndicator is set by the KItemListController
    // by KItemListView::showDropIndicator() and KItemListView::hideDropIndicator().
    QRectF m_dropIndicator;

    friend class KItemListContainer; // Accesses scrollBarRequired()
    friend class KItemListHeader;    // Accesses m_headerWidget
    friend class KItemListController;
    friend class KItemListControllerTest;
    friend class KItemListViewAccessible;
    friend class KItemListAccessibleCell;
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
 * For a custom implementation the methods create(), itemSizeHint() and preferredColumnWith()
 * must be reimplemented. The intention of the widget creator is to prevent repetitive and
 * expensive instantiations and deletions of KItemListWidgets by recycling existing widget
 * instances.
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListWidgetCreatorBase : public KItemListCreatorBase
{
public:
    virtual ~KItemListWidgetCreatorBase();

    virtual KItemListWidget* create(KItemListView* view) = 0;

    virtual void recycle(KItemListWidget* widget);

    virtual QSizeF itemSizeHint(int index, const KItemListView* view) const = 0;

    virtual qreal preferredRoleColumnWidth(const QByteArray& role,
                                           int index,
                                           const KItemListView* view) const = 0;
};

/**
 * @brief Template class for creating KItemListWidgets.
 */
template <class T>
class KItemListWidgetCreator : public KItemListWidgetCreatorBase
{
public:
    KItemListWidgetCreator();
    virtual ~KItemListWidgetCreator();

    virtual KItemListWidget* create(KItemListView* view);

    virtual QSizeF itemSizeHint(int index, const KItemListView* view) const;

    virtual qreal preferredRoleColumnWidth(const QByteArray& role,
                                           int index,
                                           const KItemListView* view) const;
private:
    KItemListWidgetInformant* m_informant;
};

template <class T>
KItemListWidgetCreator<T>::KItemListWidgetCreator() :
    m_informant(T::createInformant())
{
}

template <class T>
KItemListWidgetCreator<T>::~KItemListWidgetCreator()
{
    delete m_informant;
}

template <class T>
KItemListWidget* KItemListWidgetCreator<T>::create(KItemListView* view)
{
    KItemListWidget* widget = static_cast<KItemListWidget*>(popRecycleableWidget());
    if (!widget) {
        widget = new T(m_informant, view);
        addCreatedWidget(widget);
    }
    return widget;
}

template<class T>
QSizeF KItemListWidgetCreator<T>::itemSizeHint(int index, const KItemListView* view) const
{
    return m_informant->itemSizeHint(index, view);
}

template<class T>
qreal KItemListWidgetCreator<T>::preferredRoleColumnWidth(const QByteArray& role,
                                                          int index,
                                                          const KItemListView* view) const
{
    return m_informant->preferredRoleColumnWidth(role, index, view);
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
    virtual KItemListGroupHeader* create(KItemListView* view) = 0;
    virtual void recycle(KItemListGroupHeader* header);
};

template <class T>
class KItemListGroupHeaderCreator : public KItemListGroupHeaderCreatorBase
{
public:
    virtual ~KItemListGroupHeaderCreator();
    virtual KItemListGroupHeader* create(KItemListView* view);
};

template <class T>
KItemListGroupHeaderCreator<T>::~KItemListGroupHeaderCreator()
{
}

template <class T>
KItemListGroupHeader* KItemListGroupHeaderCreator<T>::create(KItemListView* view)
{
    KItemListGroupHeader* widget = static_cast<KItemListGroupHeader*>(popRecycleableWidget());
    if (!widget) {
        widget = new T(view);
        addCreatedWidget(widget);
    }
    return widget;
}

#endif
