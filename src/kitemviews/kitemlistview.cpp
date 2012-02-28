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

#include "kitemlistview.h"

#include "kitemlistcontroller.h"
#include "kitemlistheader_p.h"
#include "kitemlistrubberband_p.h"
#include "kitemlistselectionmanager.h"
#include "kitemlistsizehintresolver_p.h"
#include "kitemlistviewlayouter_p.h"
#include "kitemlistviewanimation_p.h"
#include "kitemlistwidget.h"

#include <KDebug>

#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyle>
#include <QStyleOptionRubberBand>
#include <QTimer>

namespace {
    // Time in ms until reaching the autoscroll margin triggers
    // an initial autoscrolling
    const int InitialAutoScrollDelay = 700;

    // Delay in ms for triggering the next autoscroll
    const int RepeatingAutoScrollDelay = 1000 / 60;
}

KItemListView::KItemListView(QGraphicsWidget* parent) :
    QGraphicsWidget(parent),
    m_enabledSelectionToggles(false),
    m_grouped(false),
    m_supportsItemExpanding(false),
    m_activeTransactions(0),
    m_endTransactionAnimationHint(Animation),
    m_itemSize(),
    m_controller(0),
    m_model(0),
    m_visibleRoles(),
    m_visibleRolesSizes(),
    m_stretchedVisibleRolesSizes(),
    m_widgetCreator(0),
    m_groupHeaderCreator(0),
    m_styleOption(),
    m_visibleItems(),
    m_visibleGroups(),
    m_visibleCells(),
    m_sizeHintResolver(0),
    m_layouter(0),
    m_animation(0),
    m_layoutTimer(0),
    m_oldScrollOffset(0),
    m_oldMaximumScrollOffset(0),
    m_oldItemOffset(0),
    m_oldMaximumItemOffset(0),
    m_skipAutoScrollForRubberBand(false),
    m_rubberBand(0),
    m_mousePos(),
    m_autoScrollIncrement(0),
    m_autoScrollTimer(0),
    m_header(0),
    m_useHeaderWidths(false)
{
    setAcceptHoverEvents(true);

    m_sizeHintResolver = new KItemListSizeHintResolver(this);

    m_layouter = new KItemListViewLayouter(this);
    m_layouter->setSizeHintResolver(m_sizeHintResolver);

    m_animation = new KItemListViewAnimation(this);
    connect(m_animation, SIGNAL(finished(QGraphicsWidget*,KItemListViewAnimation::AnimationType)),
            this, SLOT(slotAnimationFinished(QGraphicsWidget*,KItemListViewAnimation::AnimationType)));

    m_layoutTimer = new QTimer(this);
    m_layoutTimer->setInterval(300);
    m_layoutTimer->setSingleShot(true);
    connect(m_layoutTimer, SIGNAL(timeout()), this, SLOT(slotLayoutTimerFinished()));

    m_rubberBand = new KItemListRubberBand(this);
    connect(m_rubberBand, SIGNAL(activationChanged(bool)), this, SLOT(slotRubberBandActivationChanged(bool)));
}

KItemListView::~KItemListView()
{
    delete m_sizeHintResolver;
    m_sizeHintResolver = 0;
}

void KItemListView::setScrollOrientation(Qt::Orientation orientation)
{
    const Qt::Orientation previousOrientation = m_layouter->scrollOrientation();
    if (orientation == previousOrientation) {
        return;
    }

    m_layouter->setScrollOrientation(orientation);
    m_animation->setScrollOrientation(orientation);
    m_sizeHintResolver->clearCache();

    if (m_grouped) {
        QMutableHashIterator<KItemListWidget*, KItemListGroupHeader*> it (m_visibleGroups);
        while (it.hasNext()) {
            it.next();
            it.value()->setScrollOrientation(orientation);
        }
        updateGroupHeaderHeight();

    }

    doLayout(NoAnimation);

    onScrollOrientationChanged(orientation, previousOrientation);
    emit scrollOrientationChanged(orientation, previousOrientation);
}

Qt::Orientation KItemListView::scrollOrientation() const
{
    return m_layouter->scrollOrientation();
}

void KItemListView::setItemSize(const QSizeF& itemSize)
{
    const QSizeF previousSize = m_itemSize;
    if (itemSize == previousSize) {
        return;
    }

    // Skip animations when the number of rows or columns
    // are changed in the grid layout. Although the animation
    // engine can handle this usecase, it looks obtrusive.
    const bool animate = !changesItemGridLayout(m_layouter->size(),
                                                itemSize,
                                                m_layouter->itemMargin());

    const bool updateAlternateBackgrounds = (m_visibleRoles.count() > 1) &&
                                            (( m_itemSize.isEmpty() && !itemSize.isEmpty()) ||
                                             (!m_itemSize.isEmpty() && itemSize.isEmpty()));

    m_itemSize = itemSize;

    if (updateAlternateBackgrounds) {
        // For an empty item size alternate backgrounds are drawn if more than
        // one role is shown. Assure that the backgrounds for visible items are
        // updated when changing the size in this context.
        QHashIterator<int, KItemListWidget*> it(m_visibleItems);
        while (it.hasNext()) {
            it.next();
            updateAlternateBackgroundForWidget(it.value());
        }
    }

    if (itemSize.isEmpty()) {
        updateVisibleRolesSizes();
    } else {
        m_layouter->setItemSize(itemSize);
    }

    m_sizeHintResolver->clearCache();
    doLayout(animate ? Animation : NoAnimation);
    onItemSizeChanged(itemSize, previousSize);
}

QSizeF KItemListView::itemSize() const
{
    return m_itemSize;
}

void KItemListView::setScrollOffset(qreal offset)
{
    if (offset < 0) {
        offset = 0;
    }

    const qreal previousOffset = m_layouter->scrollOffset();
    if (offset == previousOffset) {
        return;
    }

    m_layouter->setScrollOffset(offset);
    m_animation->setScrollOffset(offset);

    // Don't check whether the m_layoutTimer is active: Changing the
    // scroll offset must always trigger a synchronous layout, otherwise
    // the smooth-scrolling might get jerky.
    doLayout(NoAnimation);
    onScrollOffsetChanged(offset, previousOffset);
}

qreal KItemListView::scrollOffset() const
{
    return m_layouter->scrollOffset();
}

qreal KItemListView::maximumScrollOffset() const
{
    return m_layouter->maximumScrollOffset();
}

void KItemListView::setItemOffset(qreal offset)
{
    if (m_layouter->itemOffset() == offset) {
        return;
    }

    m_layouter->setItemOffset(offset);
    if (m_header) {
        m_header->setPos(-offset, 0);
    }

    // Don't check whether the m_layoutTimer is active: Changing the
    // item offset must always trigger a synchronous layout, otherwise
    // the smooth-scrolling might get jerky.
    doLayout(NoAnimation);
}

qreal KItemListView::itemOffset() const
{
    return m_layouter->itemOffset();
}

qreal KItemListView::maximumItemOffset() const
{
    return m_layouter->maximumItemOffset();
}

void KItemListView::setVisibleRoles(const QList<QByteArray>& roles)
{
    const QList<QByteArray> previousRoles = m_visibleRoles;
    m_visibleRoles = roles;

    const bool updateAlternateBackgrounds = m_itemSize.isEmpty() &&
                                            ((roles.count() > 1 && previousRoles.count() <= 1) ||
                                             (roles.count() <= 1 && previousRoles.count() > 1));

    QHashIterator<int, KItemListWidget*> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        KItemListWidget* widget = it.value();
        widget->setVisibleRoles(roles);
        widget->setVisibleRolesSizes(m_stretchedVisibleRolesSizes);
        if (updateAlternateBackgrounds) {
            updateAlternateBackgroundForWidget(widget);
        }
    }

    m_sizeHintResolver->clearCache();
    m_layouter->markAsDirty();

    if (m_header) {
        m_header->setVisibleRoles(roles);
        m_header->setVisibleRolesWidths(headerRolesWidths());
        m_useHeaderWidths = false;
    }

    updateVisibleRolesSizes();
    doLayout(NoAnimation);

    onVisibleRolesChanged(roles, previousRoles);
}

QList<QByteArray> KItemListView::visibleRoles() const
{
    return m_visibleRoles;
}

void KItemListView::setAutoScroll(bool enabled)
{
    if (enabled && !m_autoScrollTimer) {
        m_autoScrollTimer = new QTimer(this);
        m_autoScrollTimer->setSingleShot(true);
        connect(m_autoScrollTimer, SIGNAL(timeout()), this, SLOT(triggerAutoScrolling()));
        m_autoScrollTimer->start(InitialAutoScrollDelay);
    } else if (!enabled && m_autoScrollTimer) {
        delete m_autoScrollTimer;
        m_autoScrollTimer = 0;
    }

}

bool KItemListView::autoScroll() const
{
    return m_autoScrollTimer != 0;
}

void KItemListView::setEnabledSelectionToggles(bool enabled)
{
    if (m_enabledSelectionToggles != enabled) {
        m_enabledSelectionToggles = enabled;

        QHashIterator<int, KItemListWidget*> it(m_visibleItems);
        while (it.hasNext()) {
            it.next();
            it.value()->setEnabledSelectionToggle(enabled);
        }
    }
}

bool KItemListView::enabledSelectionToggles() const
{
    return m_enabledSelectionToggles;
}

KItemListController* KItemListView::controller() const
{
    return m_controller;
}

KItemModelBase* KItemListView::model() const
{
    return m_model;
}

void KItemListView::setWidgetCreator(KItemListWidgetCreatorBase* widgetCreator)
{
    m_widgetCreator = widgetCreator;
}

KItemListWidgetCreatorBase* KItemListView::widgetCreator() const
{
    return m_widgetCreator;
}

void KItemListView::setGroupHeaderCreator(KItemListGroupHeaderCreatorBase* groupHeaderCreator)
{
    m_groupHeaderCreator = groupHeaderCreator;
}

KItemListGroupHeaderCreatorBase* KItemListView::groupHeaderCreator() const
{
    return m_groupHeaderCreator;
}

void KItemListView::setStyleOption(const KItemListStyleOption& option)
{
    const KItemListStyleOption previousOption = m_styleOption;
    m_styleOption = option;

    bool animate = true;
    const QSizeF margin(option.horizontalMargin, option.verticalMargin);
    if (margin != m_layouter->itemMargin()) {
        // Skip animations when the number of rows or columns
        // are changed in the grid layout. Although the animation
        // engine can handle this usecase, it looks obtrusive.
        animate = !changesItemGridLayout(m_layouter->size(),
                                         m_layouter->itemSize(),
                                         margin);
        m_layouter->setItemMargin(margin);
    }

    if (m_grouped) {
        updateGroupHeaderHeight();
    }

    QHashIterator<int, KItemListWidget*> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        it.value()->setStyleOption(option);
    }

    m_sizeHintResolver->clearCache();
    doLayout(animate ? Animation : NoAnimation);

    onStyleOptionChanged(option, previousOption);
}

const KItemListStyleOption& KItemListView::styleOption() const
{
    return m_styleOption;
}

void KItemListView::setGeometry(const QRectF& rect)
{
    QGraphicsWidget::setGeometry(rect);

    if (!m_model) {
        return;
    }

    const QSizeF newSize = rect.size();
    if (m_itemSize.isEmpty()) {
        // The item size is dynamic:
        // Changing the geometry does not require to do an expensive
        // update of the visible-roles sizes, only the stretched sizes
        // need to be adjusted to the new size.
        updateStretchedVisibleRolesSizes();

        if (m_useHeaderWidths) {
            QSizeF dynamicItemSize = m_layouter->itemSize();

            if (m_itemSize.width() < 0) {
                const qreal requiredWidth = visibleRolesSizesWidthSum();
                if (newSize.width() > requiredWidth) {
                    dynamicItemSize.setWidth(newSize.width());
                }
                const qreal headerWidth = qMax(newSize.width(), requiredWidth);
                m_header->resize(headerWidth, m_header->size().height());
            }

            if (m_itemSize.height() < 0) {
                const qreal requiredHeight = visibleRolesSizesHeightSum();
                if (newSize.height() > requiredHeight) {
                    dynamicItemSize.setHeight(newSize.height());
                }
                // TODO: KItemListHeader is not prepared for vertical alignment
            }

            m_layouter->setItemSize(dynamicItemSize);
        }

        // Triggering a synchronous layout is fine from a performance point of view,
        // as with dynamic item sizes no moving animation must be done.
        m_layouter->setSize(newSize);
        doLayout(Animation);
    } else {
        const bool animate = !changesItemGridLayout(newSize,
                                                    m_layouter->itemSize(),
                                                    m_layouter->itemMargin());
        m_layouter->setSize(newSize);

        if (animate) {
            // Trigger an asynchronous relayout with m_layoutTimer to prevent
            // performance bottlenecks. If the timer is exceeded, an animated layout
            // will be triggered.
            if (!m_layoutTimer->isActive()) {
                m_layoutTimer->start();
            }
        } else {
            m_layoutTimer->stop();
            doLayout(NoAnimation);
        }
    }
}

int KItemListView::itemAt(const QPointF& pos) const
{
    QHashIterator<int, KItemListWidget*> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();

        const KItemListWidget* widget = it.value();
        const QPointF mappedPos = widget->mapFromItem(this, pos);
        if (widget->contains(mappedPos)) {
            return it.key();
        }
    }

    return -1;
}

bool KItemListView::isAboveSelectionToggle(int index, const QPointF& pos) const
{
    if (!m_enabledSelectionToggles) {
        return false;
    }

    const KItemListWidget* widget = m_visibleItems.value(index);
    if (widget) {
        const QRectF selectionToggleRect = widget->selectionToggleRect();
        if (!selectionToggleRect.isEmpty()) {
            const QPointF mappedPos = widget->mapFromItem(this, pos);
            return selectionToggleRect.contains(mappedPos);
        }
    }
    return false;
}

bool KItemListView::isAboveExpansionToggle(int index, const QPointF& pos) const
{
    const KItemListWidget* widget = m_visibleItems.value(index);
    if (widget) {
        const QRectF expansionToggleRect = widget->expansionToggleRect();
        if (!expansionToggleRect.isEmpty()) {
            const QPointF mappedPos = widget->mapFromItem(this, pos);
            return expansionToggleRect.contains(mappedPos);
        }
    }
    return false;
}

int KItemListView::firstVisibleIndex() const
{
    return m_layouter->firstVisibleIndex();
}

int KItemListView::lastVisibleIndex() const
{
    return m_layouter->lastVisibleIndex();
}

QSizeF KItemListView::itemSizeHint(int index) const
{
    Q_UNUSED(index);
    return itemSize();
}

QHash<QByteArray, QSizeF> KItemListView::visibleRolesSizes(const KItemRangeList& itemRanges) const
{
    Q_UNUSED(itemRanges);
    return QHash<QByteArray, QSizeF>();
}

bool KItemListView::supportsItemExpanding() const
{
    return m_supportsItemExpanding;
}

QRectF KItemListView::itemRect(int index) const
{
    return m_layouter->itemRect(index);
}

QRectF KItemListView::itemContextRect(int index) const
{
    QRectF contextRect;

    const KItemListWidget* widget = m_visibleItems.value(index);
    if (widget) {
        contextRect = widget->iconRect() | widget->textRect();
        contextRect.translate(itemRect(index).topLeft());
    }

    return contextRect;
}

void KItemListView::scrollToItem(int index)
{
    QRectF viewGeometry = geometry();
    if (m_header) {
        const qreal headerHeight = m_header->size().height();
        viewGeometry.adjust(0, headerHeight, 0, 0);
    }
    const QRectF currentRect = itemRect(index);

    if (!viewGeometry.contains(currentRect)) {
        qreal newOffset = scrollOffset();
        if (scrollOrientation() == Qt::Vertical) {
            if (currentRect.top() < viewGeometry.top()) {
                newOffset += currentRect.top() - viewGeometry.top();
            } else if (currentRect.bottom() > viewGeometry.bottom()) {
                newOffset += currentRect.bottom() - viewGeometry.bottom();
            }
        } else {
            if (currentRect.left() < viewGeometry.left()) {
                newOffset += currentRect.left() - viewGeometry.left();
            } else if (currentRect.right() > viewGeometry.right()) {
                newOffset += currentRect.right() - viewGeometry.right();
            }
        }

        if (newOffset != scrollOffset()) {
            emit scrollTo(newOffset);
        }
    }
}

void KItemListView::beginTransaction()
{
    ++m_activeTransactions;
    if (m_activeTransactions == 1) {
        onTransactionBegin();
    }
}

void KItemListView::endTransaction()
{
    --m_activeTransactions;
    if (m_activeTransactions < 0) {
        m_activeTransactions = 0;
        kWarning() << "Mismatch between beginTransaction()/endTransaction()";
    }

    if (m_activeTransactions == 0) {
        onTransactionEnd();
        doLayout(m_endTransactionAnimationHint);
        m_endTransactionAnimationHint = Animation;
    }
}

bool KItemListView::isTransactionActive() const
{
    return m_activeTransactions > 0;
}

void KItemListView::setHeaderShown(bool show)
{

    if (show && !m_header) {
        m_header = new KItemListHeader(this);
        m_header->setPos(0, 0);
        m_header->setModel(m_model);
        m_header->setVisibleRoles(m_visibleRoles);
        m_header->setVisibleRolesWidths(headerRolesWidths());
        m_header->setZValue(1);

        connect(m_header, SIGNAL(visibleRoleWidthChanged(QByteArray,qreal,qreal)),
                this, SLOT(slotVisibleRoleWidthChanged(QByteArray,qreal,qreal)));
        connect(m_header, SIGNAL(sortOrderChanged(Qt::SortOrder,Qt::SortOrder)),
                this, SIGNAL(sortOrderChanged(Qt::SortOrder,Qt::SortOrder)));
        connect(m_header, SIGNAL(sortRoleChanged(QByteArray,QByteArray)),
                this, SIGNAL(sortRoleChanged(QByteArray,QByteArray)));

        m_useHeaderWidths = false;

        m_layouter->setHeaderHeight(m_header->size().height());
    } else if (!show && m_header) {
        delete m_header;
        m_header = 0;
        m_useHeaderWidths = false;
        m_layouter->setHeaderHeight(0);
    }
}

bool KItemListView::isHeaderShown() const
{
    return m_header != 0;
}

QPixmap KItemListView::createDragPixmap(const QSet<int>& indexes) const
{
    Q_UNUSED(indexes);
    return QPixmap();
}

void KItemListView::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    QGraphicsWidget::paint(painter, option, widget);

    if (m_rubberBand->isActive()) {
        QRectF rubberBandRect = QRectF(m_rubberBand->startPosition(),
                                       m_rubberBand->endPosition()).normalized();

        const QPointF topLeft = rubberBandRect.topLeft();
        if (scrollOrientation() == Qt::Vertical) {
            rubberBandRect.moveTo(topLeft.x(), topLeft.y() - scrollOffset());
        } else {
            rubberBandRect.moveTo(topLeft.x() - scrollOffset(), topLeft.y());
        }

        QStyleOptionRubberBand opt;
        opt.initFrom(widget);
        opt.shape = QRubberBand::Rectangle;
        opt.opaque = false;
        opt.rect = rubberBandRect.toRect();
        style()->drawControl(QStyle::CE_RubberBand, &opt, painter);
    }
}

void KItemListView::initializeItemListWidget(KItemListWidget* item)
{
    Q_UNUSED(item);
}

bool KItemListView::itemSizeHintUpdateRequired(const QSet<QByteArray>& changedRoles) const
{
    Q_UNUSED(changedRoles);
    return true;
}

void KItemListView::onControllerChanged(KItemListController* current, KItemListController* previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListView::onModelChanged(KItemModelBase* current, KItemModelBase* previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListView::onScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListView::onItemSizeChanged(const QSizeF& current, const QSizeF& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListView::onScrollOffsetChanged(qreal current, qreal previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListView::onVisibleRolesChanged(const QList<QByteArray>& current, const QList<QByteArray>& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListView::onStyleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListView::onTransactionBegin()
{
}

void KItemListView::onTransactionEnd()
{
}

bool KItemListView::event(QEvent* event)
{
    // Forward all events to the controller and handle them there
    if (m_controller && m_controller->processEvent(event, transform())) {
        event->accept();
        return true;
    }
    return QGraphicsWidget::event(event);
}

void KItemListView::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    m_mousePos = transform().map(event->pos());
    event->accept();
}

void KItemListView::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsWidget::mouseMoveEvent(event);

    m_mousePos = transform().map(event->pos());
    if (m_autoScrollTimer && !m_autoScrollTimer->isActive()) {
        m_autoScrollTimer->start(InitialAutoScrollDelay);
    }
}

void KItemListView::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
    event->setAccepted(true);
    setAutoScroll(true);
}

void KItemListView::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    QGraphicsWidget::dragMoveEvent(event);

    m_mousePos = transform().map(event->pos());
    if (m_autoScrollTimer && !m_autoScrollTimer->isActive()) {
        m_autoScrollTimer->start(InitialAutoScrollDelay);
    }
}

void KItemListView::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    QGraphicsWidget::dragLeaveEvent(event);
    setAutoScroll(false);
}

void KItemListView::dropEvent(QGraphicsSceneDragDropEvent* event)
{
    QGraphicsWidget::dropEvent(event);
    setAutoScroll(false);
}

QList<KItemListWidget*> KItemListView::visibleItemListWidgets() const
{
    return m_visibleItems.values();
}

void KItemListView::setSupportsItemExpanding(bool supportsExpanding)
{
    if (m_supportsItemExpanding != supportsExpanding) {
        m_supportsItemExpanding = supportsExpanding;
        updateSiblingsInformation();
    }
}

void KItemListView::slotItemsInserted(const KItemRangeList& itemRanges)
{
    updateVisibleRolesSizes(itemRanges);

    const bool hasMultipleRanges = (itemRanges.count() > 1);
    if (hasMultipleRanges) {
        beginTransaction();
    }

    m_layouter->markAsDirty();

    int previouslyInsertedCount = 0;
    foreach (const KItemRange& range, itemRanges) {
        // range.index is related to the model before anything has been inserted.
        // As in each loop the current item-range gets inserted the index must
        // be increased by the already previously inserted items.
        const int index = range.index + previouslyInsertedCount;
        const int count = range.count;
        if (index < 0 || count <= 0) {
            kWarning() << "Invalid item range (index:" << index << ", count:" << count << ")";
            continue;
        }
        previouslyInsertedCount += count;

        m_sizeHintResolver->itemsInserted(index, count);

        // Determine which visible items must be moved
        QList<int> itemsToMove;
        QHashIterator<int, KItemListWidget*> it(m_visibleItems);
        while (it.hasNext()) {
            it.next();
            const int visibleItemIndex = it.key();
            if (visibleItemIndex >= index) {
                itemsToMove.append(visibleItemIndex);
            }
        }

        // Update the indexes of all KItemListWidget instances that are located
        // after the inserted items. It is important to adjust the indexes in the order
        // from the highest index to the lowest index to prevent overlaps when setting the new index.
        qSort(itemsToMove);
        for (int i = itemsToMove.count() - 1; i >= 0; --i) {
            KItemListWidget* widget = m_visibleItems.value(itemsToMove[i]);
            Q_ASSERT(widget);
            const int newIndex = widget->index() + count;
            if (hasMultipleRanges) {
                setWidgetIndex(widget, newIndex);
            } else {
                // Try to animate the moving of the item
                moveWidgetToIndex(widget, newIndex);
            }
        }

        if (m_model->count() == count && m_activeTransactions == 0) {
            // Check whether a scrollbar is required to show the inserted items. In this case
            // the size of the layouter will be decreased before calling doLayout(): This prevents
            // an unnecessary temporary animation due to the geometry change of the inserted scrollbar.
            const bool verticalScrollOrientation = (scrollOrientation() == Qt::Vertical);
            const bool decreaseLayouterSize = ( verticalScrollOrientation && maximumScrollOffset() > size().height()) ||
                                              (!verticalScrollOrientation && maximumScrollOffset() > size().width());
            if (decreaseLayouterSize) {
                const int scrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
                QSizeF layouterSize = m_layouter->size();
                if (verticalScrollOrientation) {
                    layouterSize.rwidth() -= scrollBarExtent;
                } else {
                    layouterSize.rheight() -= scrollBarExtent;
                }
                m_layouter->setSize(layouterSize);
            }
        }

        if (!hasMultipleRanges) {
            doLayout(animateChangedItemCount(count) ? Animation : NoAnimation, index, count);
            updateSiblingsInformation();
        }
    }

    if (m_controller) {
        m_controller->selectionManager()->itemsInserted(itemRanges);
    }

    if (hasMultipleRanges) {
#ifndef QT_NO_DEBUG
        // Important: Don't read any m_layouter-property inside the for-loop in case if
        // multiple ranges are given! m_layouter accesses m_sizeHintResolver which is
        // updated in each loop-cycle and has only a consistent state after the loop.
        Q_ASSERT(m_layouter->isDirty());
#endif
        m_endTransactionAnimationHint = NoAnimation;
        endTransaction();
        updateSiblingsInformation();
    }
}

void KItemListView::slotItemsRemoved(const KItemRangeList& itemRanges)
{
    updateVisibleRolesSizes();

    const bool hasMultipleRanges = (itemRanges.count() > 1);
    if (hasMultipleRanges) {
        beginTransaction();
    }

    m_layouter->markAsDirty();

    for (int i = itemRanges.count() - 1; i >= 0; --i) {
        const KItemRange& range = itemRanges.at(i);
        const int index = range.index;
        const int count = range.count;
        if (index < 0 || count <= 0) {
            kWarning() << "Invalid item range (index:" << index << ", count:" << count << ")";
            continue;
        }

        m_sizeHintResolver->itemsRemoved(index, count);

        const int firstRemovedIndex = index;
        const int lastRemovedIndex = index + count - 1;
        const int lastIndex = m_model->count() + count - 1;

        // Remove all KItemListWidget instances that got deleted
        for (int i = firstRemovedIndex; i <= lastRemovedIndex; ++i) {
            KItemListWidget* widget = m_visibleItems.value(i);
            if (!widget) {
                continue;
            }

            m_animation->stop(widget);
            // Stopping the animation might lead to recycling the widget if
            // it is invisible (see slotAnimationFinished()).
            // Check again whether it is still visible:
            if (!m_visibleItems.contains(i)) {
                continue;
            }

            if (m_model->count() == 0 || hasMultipleRanges || !animateChangedItemCount(count)) {
                // Remove the widget without animation
                recycleWidget(widget);
            } else {
                // Animate the removing of the items. Special case: When removing an item there
                // is no valid model index available anymore. For the
                // remove-animation the item gets removed from m_visibleItems but the widget
                // will stay alive until the animation has been finished and will
                // be recycled (deleted) in KItemListView::slotAnimationFinished().
                m_visibleItems.remove(i);
                widget->setIndex(-1);
                m_animation->start(widget, KItemListViewAnimation::DeleteAnimation);
            }
        }

        // Update the indexes of all KItemListWidget instances that are located
        // after the deleted items
        for (int i = lastRemovedIndex + 1; i <= lastIndex; ++i) {
            KItemListWidget* widget = m_visibleItems.value(i);
            if (widget) {
                const int newIndex = i - count;
                if (hasMultipleRanges) {
                    setWidgetIndex(widget, newIndex);
                } else {
                    // Try to animate the moving of the item
                    moveWidgetToIndex(widget, newIndex);
                }
            }
        }

        if (!hasMultipleRanges) {
            // The decrease-layout-size optimization in KItemListView::slotItemsInserted()
            // assumes an updated geometry. If items are removed during an active transaction,
            // the transaction will be temporary deactivated so that doLayout() triggers a
            // geometry update if necessary.
            const int activeTransactions = m_activeTransactions;
            m_activeTransactions = 0;
            doLayout(animateChangedItemCount(count) ? Animation : NoAnimation, index, -count);
            m_activeTransactions = activeTransactions;
            updateSiblingsInformation();
        }
    }

    if (m_controller) {
        m_controller->selectionManager()->itemsRemoved(itemRanges);
    }

    if (hasMultipleRanges) {
#ifndef QT_NO_DEBUG
        // Important: Don't read any m_layouter-property inside the for-loop in case if
        // multiple ranges are given! m_layouter accesses m_sizeHintResolver which is
        // updated in each loop-cycle and has only a consistent state after the loop.
        Q_ASSERT(m_layouter->isDirty());
#endif
        m_endTransactionAnimationHint = NoAnimation;
        endTransaction();
        updateSiblingsInformation();
    }
}

void KItemListView::slotItemsMoved(const KItemRange& itemRange, const QList<int>& movedToIndexes)
{
    m_sizeHintResolver->itemsMoved(itemRange.index, itemRange.count);
    m_layouter->markAsDirty();

    if (m_controller) {
        m_controller->selectionManager()->itemsMoved(itemRange, movedToIndexes);
    }

    const int firstVisibleMovedIndex = qMax(firstVisibleIndex(), itemRange.index);
    const int lastVisibleMovedIndex = qMin(lastVisibleIndex(), itemRange.index + itemRange.count - 1);

    for (int index = firstVisibleMovedIndex; index <= lastVisibleMovedIndex; ++index) {
        KItemListWidget* widget = m_visibleItems.value(index);
        if (widget) {
            updateWidgetProperties(widget, index);
            initializeItemListWidget(widget);
        }
    }

    doLayout(NoAnimation);
    updateSiblingsInformation();
}

void KItemListView::slotItemsChanged(const KItemRangeList& itemRanges,
                                     const QSet<QByteArray>& roles)
{
    const bool updateSizeHints = itemSizeHintUpdateRequired(roles);
    if (updateSizeHints) {
        updateVisibleRolesSizes(itemRanges);
    }

    foreach (const KItemRange& itemRange, itemRanges) {
        const int index = itemRange.index;
        const int count = itemRange.count;

        if (updateSizeHints) {
            m_sizeHintResolver->itemsChanged(index, count, roles);
            m_layouter->markAsDirty();

            if (!m_layoutTimer->isActive()) {
                m_layoutTimer->start();
            }
        }

        // Apply the changed roles to the visible item-widgets
        const int lastIndex = index + count - 1;
        for (int i = index; i <= lastIndex; ++i) {
            KItemListWidget* widget = m_visibleItems.value(i);
            if (widget) {
                widget->setData(m_model->data(i), roles);
            }
        }

        if (m_grouped && roles.contains(m_model->sortRole())) {
            // The sort-role has been changed which might result
            // in modified group headers
            updateVisibleGroupHeaders();
            doLayout(NoAnimation);
        }
    }
}

void KItemListView::slotGroupedSortingChanged(bool current)
{
    m_grouped = current;
    m_layouter->markAsDirty();

    if (m_grouped) {
        updateGroupHeaderHeight();
    } else {
        // Clear all visible headers
        QMutableHashIterator<KItemListWidget*, KItemListGroupHeader*> it (m_visibleGroups);
        while (it.hasNext()) {
            it.next();
            recycleGroupHeaderForWidget(it.key());
        }
        Q_ASSERT(m_visibleGroups.isEmpty());
    }

    if (useAlternateBackgrounds()) {
        // Changing the group mode requires to update the alternate backgrounds
        // as with the enabled group mode the altering is done on base of the first
        // group item.
        QHashIterator<int, KItemListWidget*> it(m_visibleItems);
        while (it.hasNext()) {
            it.next();
            updateAlternateBackgroundForWidget(it.value());
        }
    }

    doLayout(NoAnimation);
}

void KItemListView::slotSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    if (m_grouped) {
        updateVisibleGroupHeaders();
        doLayout(NoAnimation);
    }
}

void KItemListView::slotSortRoleChanged(const QByteArray& current, const QByteArray& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    if (m_grouped) {
        updateVisibleGroupHeaders();
        doLayout(NoAnimation);
    }
}

void KItemListView::slotCurrentChanged(int current, int previous)
{
    Q_UNUSED(previous);

    KItemListWidget* previousWidget = m_visibleItems.value(previous, 0);
    if (previousWidget) {
        previousWidget->setCurrent(false);
    }

    KItemListWidget* currentWidget = m_visibleItems.value(current, 0);
    if (currentWidget) {
        currentWidget->setCurrent(true);
    }
}

void KItemListView::slotSelectionChanged(const QSet<int>& current, const QSet<int>& previous)
{
    Q_UNUSED(previous);

    QHashIterator<int, KItemListWidget*> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        const int index = it.key();
        KItemListWidget* widget = it.value();
        widget->setSelected(current.contains(index));
    }
}

void KItemListView::slotAnimationFinished(QGraphicsWidget* widget,
                                          KItemListViewAnimation::AnimationType type)
{
    KItemListWidget* itemListWidget = qobject_cast<KItemListWidget*>(widget);
    Q_ASSERT(itemListWidget);

    switch (type) {
    case KItemListViewAnimation::DeleteAnimation: {
        // As we recycle the widget in this case it is important to assure that no
        // other animation has been started. This is a convention in KItemListView and
        // not a requirement defined by KItemListViewAnimation.
        Q_ASSERT(!m_animation->isStarted(itemListWidget));

        // All KItemListWidgets that are animated by the DeleteAnimation are not maintained
        // by m_visibleWidgets and must be deleted manually after the animation has
        // been finished.
        recycleGroupHeaderForWidget(itemListWidget);
        m_widgetCreator->recycle(itemListWidget);
        break;
    }

    case KItemListViewAnimation::CreateAnimation:
    case KItemListViewAnimation::MovingAnimation:
    case KItemListViewAnimation::ResizeAnimation: {
        const int index = itemListWidget->index();
        const bool invisible = (index < m_layouter->firstVisibleIndex()) ||
                               (index > m_layouter->lastVisibleIndex());
        if (invisible && !m_animation->isStarted(itemListWidget)) {
            recycleWidget(itemListWidget);
        }
        break;
    }

    default: break;
    }
}

void KItemListView::slotLayoutTimerFinished()
{
    m_layouter->setSize(geometry().size());
    doLayout(Animation);
}

void KItemListView::slotRubberBandPosChanged()
{
    update();
}

void KItemListView::slotRubberBandActivationChanged(bool active)
{
    if (active) {
        connect(m_rubberBand, SIGNAL(startPositionChanged(QPointF,QPointF)), this, SLOT(slotRubberBandPosChanged()));
        connect(m_rubberBand, SIGNAL(endPositionChanged(QPointF,QPointF)), this, SLOT(slotRubberBandPosChanged()));
        m_skipAutoScrollForRubberBand = true;
    } else {
        disconnect(m_rubberBand, SIGNAL(startPositionChanged(QPointF,QPointF)), this, SLOT(slotRubberBandPosChanged()));
        disconnect(m_rubberBand, SIGNAL(endPositionChanged(QPointF,QPointF)), this, SLOT(slotRubberBandPosChanged()));
        m_skipAutoScrollForRubberBand = false;
    }

    update();
}

void KItemListView::slotVisibleRoleWidthChanged(const QByteArray& role,
                                                qreal currentWidth,
                                                qreal previousWidth)
{
    Q_UNUSED(previousWidth);

    m_useHeaderWidths = true;

    if (m_visibleRolesSizes.contains(role)) {
        QSizeF roleSize = m_visibleRolesSizes.value(role);
        roleSize.setWidth(currentWidth);
        m_visibleRolesSizes.insert(role, roleSize);
        m_stretchedVisibleRolesSizes.insert(role, roleSize);

        // Apply the new size to the layouter
        QSizeF dynamicItemSize = m_itemSize;
        if (dynamicItemSize.width() < 0) {
            const qreal requiredWidth = visibleRolesSizesWidthSum();
            dynamicItemSize.setWidth(qMax(size().width(), requiredWidth));
        }
        if (dynamicItemSize.height() < 0) {
            const qreal requiredHeight = visibleRolesSizesHeightSum();
            dynamicItemSize.setHeight(qMax(size().height(), requiredHeight));
        }

        m_layouter->setItemSize(dynamicItemSize);

        // Update the role sizes for all visible widgets
        QHashIterator<int, KItemListWidget*> it(m_visibleItems);
        while (it.hasNext()) {
            it.next();
            it.value()->setVisibleRolesSizes(m_stretchedVisibleRolesSizes);
        }
        doLayout(NoAnimation);
    }
}

void KItemListView::triggerAutoScrolling()
{
    if (!m_autoScrollTimer) {
        return;
    }

    int pos = 0;
    int visibleSize = 0;
    if (scrollOrientation() == Qt::Vertical) {
        pos = m_mousePos.y();
        visibleSize = size().height();
    } else {
        pos = m_mousePos.x();
        visibleSize = size().width();
    }

    if (m_autoScrollTimer->interval() == InitialAutoScrollDelay) {
        m_autoScrollIncrement = 0;
    }

    m_autoScrollIncrement = calculateAutoScrollingIncrement(pos, visibleSize, m_autoScrollIncrement);
    if (m_autoScrollIncrement == 0) {
        // The mouse position is not above an autoscroll margin (the autoscroll timer
        // will be restarted in mouseMoveEvent())
        m_autoScrollTimer->stop();
        return;
    }

    if (m_rubberBand->isActive() && m_skipAutoScrollForRubberBand) {
        // If a rubberband selection is ongoing the autoscrolling may only get triggered
        // if the direction of the rubberband is similar to the autoscroll direction. This
        // prevents that starting to create a rubberband within the autoscroll margins starts
        // an autoscrolling.

        const qreal minDiff = 4; // Ignore any autoscrolling if the rubberband is very small
        const qreal diff = (scrollOrientation() == Qt::Vertical)
                           ? m_rubberBand->endPosition().y() - m_rubberBand->startPosition().y()
                           : m_rubberBand->endPosition().x() - m_rubberBand->startPosition().x();
        if (qAbs(diff) < minDiff || (m_autoScrollIncrement < 0 && diff > 0) || (m_autoScrollIncrement > 0 && diff < 0)) {
            // The rubberband direction is different from the scroll direction (e.g. the rubberband has
            // been moved up although the autoscroll direction might be down)
            m_autoScrollTimer->stop();
            return;
        }
    }

    // As soon as the autoscrolling has been triggered at least once despite having an active rubberband,
    // the autoscrolling may not get skipped anymore until a new rubberband is created
    m_skipAutoScrollForRubberBand = false;

    const qreal maxVisibleOffset = qMax(qreal(0), maximumScrollOffset() - visibleSize);
    const qreal newScrollOffset = qMin(scrollOffset() + m_autoScrollIncrement, maxVisibleOffset);
    setScrollOffset(newScrollOffset);

   // Trigger the autoscroll timer which will periodically call
   // triggerAutoScrolling()
   m_autoScrollTimer->start(RepeatingAutoScrollDelay);
}

void KItemListView::slotGeometryOfGroupHeaderParentChanged()
{
    KItemListWidget* widget = qobject_cast<KItemListWidget*>(sender());
    Q_ASSERT(widget);
    KItemListGroupHeader* groupHeader = m_visibleGroups.value(widget);
    Q_ASSERT(groupHeader);
    updateGroupHeaderLayout(widget);
}

void KItemListView::setController(KItemListController* controller)
{
    if (m_controller != controller) {
        KItemListController* previous = m_controller;
        if (previous) {
            KItemListSelectionManager* selectionManager = previous->selectionManager();
            disconnect(selectionManager, SIGNAL(currentChanged(int,int)), this, SLOT(slotCurrentChanged(int,int)));
            disconnect(selectionManager, SIGNAL(selectionChanged(QSet<int>,QSet<int>)), this, SLOT(slotSelectionChanged(QSet<int>,QSet<int>)));
        }

        m_controller = controller;

        if (controller) {
            KItemListSelectionManager* selectionManager = controller->selectionManager();
            connect(selectionManager, SIGNAL(currentChanged(int,int)), this, SLOT(slotCurrentChanged(int,int)));
            connect(selectionManager, SIGNAL(selectionChanged(QSet<int>,QSet<int>)), this, SLOT(slotSelectionChanged(QSet<int>,QSet<int>)));
        }

        onControllerChanged(controller, previous);
    }
}

void KItemListView::setModel(KItemModelBase* model)
{
    if (m_model == model) {
        return;
    }

    KItemModelBase* previous = m_model;

    if (m_model) {
        disconnect(m_model, SIGNAL(itemsChanged(KItemRangeList,QSet<QByteArray>)),
                   this,    SLOT(slotItemsChanged(KItemRangeList,QSet<QByteArray>)));
        disconnect(m_model, SIGNAL(itemsInserted(KItemRangeList)),
                   this,    SLOT(slotItemsInserted(KItemRangeList)));
        disconnect(m_model, SIGNAL(itemsRemoved(KItemRangeList)),
                   this,    SLOT(slotItemsRemoved(KItemRangeList)));
        disconnect(m_model, SIGNAL(itemsMoved(KItemRange,QList<int>)),
                   this,    SLOT(slotItemsMoved(KItemRange,QList<int>)));
        disconnect(m_model, SIGNAL(groupedSortingChanged(bool)),
                   this,    SLOT(slotGroupedSortingChanged(bool)));
        disconnect(m_model, SIGNAL(sortOrderChanged(Qt::SortOrder,Qt::SortOrder)),
                   this,    SLOT(slotSortOrderChanged(Qt::SortOrder,Qt::SortOrder)));
        disconnect(m_model, SIGNAL(sortRoleChanged(QByteArray,QByteArray)),
                   this,    SLOT(slotSortRoleChanged(QByteArray,QByteArray)));
    }

    m_model = model;
    m_layouter->setModel(model);
    m_grouped = model->groupedSorting();

    if (m_model) {
        connect(m_model, SIGNAL(itemsChanged(KItemRangeList,QSet<QByteArray>)),
                this,    SLOT(slotItemsChanged(KItemRangeList,QSet<QByteArray>)));
        connect(m_model, SIGNAL(itemsInserted(KItemRangeList)),
                this,    SLOT(slotItemsInserted(KItemRangeList)));
        connect(m_model, SIGNAL(itemsRemoved(KItemRangeList)),
                this,    SLOT(slotItemsRemoved(KItemRangeList)));
        connect(m_model, SIGNAL(itemsMoved(KItemRange,QList<int>)),
                this,    SLOT(slotItemsMoved(KItemRange,QList<int>)));
        connect(m_model, SIGNAL(groupedSortingChanged(bool)),
                this,    SLOT(slotGroupedSortingChanged(bool)));
        connect(m_model, SIGNAL(sortOrderChanged(Qt::SortOrder,Qt::SortOrder)),
                this,    SLOT(slotSortOrderChanged(Qt::SortOrder,Qt::SortOrder)));
        connect(m_model, SIGNAL(sortRoleChanged(QByteArray,QByteArray)),
                this,    SLOT(slotSortRoleChanged(QByteArray,QByteArray)));
    }

    onModelChanged(model, previous);
}

KItemListRubberBand* KItemListView::rubberBand() const
{
    return m_rubberBand;
}

void KItemListView::doLayout(LayoutAnimationHint hint, int changedIndex, int changedCount)
{
    if (m_layoutTimer->isActive()) {
        m_layoutTimer->stop();
    }

    if (m_activeTransactions > 0) {
        if (hint == NoAnimation) {
            // As soon as at least one property change should be done without animation,
            // the whole transaction will be marked as not animated.
            m_endTransactionAnimationHint = NoAnimation;
        }
        return;
    }

    if (!m_model || m_model->count() < 0) {
        return;
    }

    int firstVisibleIndex = m_layouter->firstVisibleIndex();
    if (firstVisibleIndex < 0) {
        emitOffsetChanges();
        return;
    }

    // Do a sanity check of the scroll-offset property: When properties of the itemlist-view have been changed
    // it might be possible that the maximum offset got changed too. Assure that the full visible range
    // is still shown if the maximum offset got decreased.
    const qreal visibleOffsetRange = (scrollOrientation() == Qt::Horizontal) ? size().width() : size().height();
    const qreal maxOffsetToShowFullRange = maximumScrollOffset() - visibleOffsetRange;
    if (scrollOffset() > maxOffsetToShowFullRange) {
        m_layouter->setScrollOffset(qMax(qreal(0), maxOffsetToShowFullRange));
        firstVisibleIndex = m_layouter->firstVisibleIndex();
    }

    const int lastVisibleIndex = m_layouter->lastVisibleIndex();

    int firstSibblingIndex = -1;
    int lastSibblingIndex = -1;
    const bool supportsExpanding = supportsItemExpanding();

    QList<int> reusableItems = recycleInvisibleItems(firstVisibleIndex, lastVisibleIndex, hint);

    // Assure that for each visible item a KItemListWidget is available. KItemListWidget
    // instances from invisible items are reused. If no reusable items are
    // found then new KItemListWidget instances get created.
    const bool animate = (hint == Animation);
    for (int i = firstVisibleIndex; i <= lastVisibleIndex; ++i) {
        bool applyNewPos = true;
        bool wasHidden = false;

        const QRectF itemBounds = m_layouter->itemRect(i);
        const QPointF newPos = itemBounds.topLeft();
        KItemListWidget* widget = m_visibleItems.value(i);
        if (!widget) {
            wasHidden = true;
            if (!reusableItems.isEmpty()) {
                // Reuse a KItemListWidget instance from an invisible item
                const int oldIndex = reusableItems.takeLast();
                widget = m_visibleItems.value(oldIndex);
                setWidgetIndex(widget, i);
                updateWidgetProperties(widget, i);
                initializeItemListWidget(widget);
            } else {
                // No reusable KItemListWidget instance is available, create a new one
                widget = createWidget(i);
            }
            widget->resize(itemBounds.size());

            if (animate && changedCount < 0) {
                // Items have been deleted, move the created item to the
                // imaginary old position. They will get animated to the new position
                // later.
                const QRectF itemRect = m_layouter->itemRect(i - changedCount);
                if (itemRect.isEmpty()) {
                    const QPointF invisibleOldPos = (scrollOrientation() == Qt::Vertical)
                                                    ? QPointF(0, size().height()) : QPointF(size().width(), 0);
                    widget->setPos(invisibleOldPos);
                } else {
                    widget->setPos(itemRect.topLeft());
                }
                applyNewPos = false;
            }

            if (supportsExpanding && changedCount == 0) {
                if (firstSibblingIndex < 0) {
                    firstSibblingIndex = i;
                }
                lastSibblingIndex = i;
            }
        }

        if (animate) {
            const bool itemsRemoved = (changedCount < 0);
            const bool itemsInserted = (changedCount > 0);
            if (itemsRemoved && (i >= changedIndex + changedCount + 1)) {
                // The item is located after the removed items. Animate the moving of the position.
                applyNewPos = !moveWidget(widget, newPos);
            } else if (itemsInserted && i >= changedIndex) {
                // The item is located after the first inserted item
                if (i <= changedIndex + changedCount - 1) {
                    // The item is an inserted item. Animate the appearing of the item.
                    // For performance reasons no animation is done when changedCount is equal
                    // to all available items.
                    if (changedCount < m_model->count()) {
                        m_animation->start(widget, KItemListViewAnimation::CreateAnimation);
                    }
                } else if (!m_animation->isStarted(widget, KItemListViewAnimation::CreateAnimation)) {
                    // The item was already there before, so animate the moving of the position.
                    // No moving animation is done if the item is animated by a create animation: This
                    // prevents a "move animation mess" when inserting several ranges in parallel.
                    applyNewPos = !moveWidget(widget, newPos);
                }
            } else if (!itemsRemoved && !itemsInserted && !wasHidden) {
                // The size of the view might have been changed. Animate the moving of the position.
                applyNewPos = !moveWidget(widget, newPos);
            }
        } else {
            m_animation->stop(widget);
        }

        if (applyNewPos) {
            widget->setPos(newPos);
        }

        Q_ASSERT(widget->index() == i);
        widget->setVisible(true);

        if (widget->size() != itemBounds.size()) {
            // Resize the widget for the item to the changed size.
            if (animate) {
                // If a dynamic item size is used then no animation is done in the direction
                // of the dynamic size.
                if (m_itemSize.width() <= 0) {
                    // The width is dynamic, apply the new width without animation.
                    widget->resize(itemBounds.width(), widget->size().height());
                } else if (m_itemSize.height() <= 0) {
                    // The height is dynamic, apply the new height without animation.
                    widget->resize(widget->size().width(), itemBounds.height());
                }
                m_animation->start(widget, KItemListViewAnimation::ResizeAnimation, itemBounds.size());
            } else {
                widget->resize(itemBounds.size());
            }
        }

        // Updating the cell-information must be done as last step: The decision whether the
        // moving-animation should be started at all is based on the previous cell-information.
        const Cell cell(m_layouter->itemColumn(i), m_layouter->itemRow(i));
        m_visibleCells.insert(i, cell);
    }

    // Delete invisible KItemListWidget instances that have not been reused
    foreach (int index, reusableItems) {
        recycleWidget(m_visibleItems.value(index));
    }

    if (supportsExpanding && firstSibblingIndex >= 0) {
        Q_ASSERT(lastSibblingIndex >= 0);
        updateSiblingsInformation(firstSibblingIndex, lastSibblingIndex);
    }

    if (m_grouped) {
        // Update the layout of all visible group headers
        QHashIterator<KItemListWidget*, KItemListGroupHeader*> it(m_visibleGroups);
        while (it.hasNext()) {
            it.next();
            updateGroupHeaderLayout(it.key());
        }
    }

    emitOffsetChanges();
}

QList<int> KItemListView::recycleInvisibleItems(int firstVisibleIndex,
                                                int lastVisibleIndex,
                                                LayoutAnimationHint hint)
{
    // Determine all items that are completely invisible and might be
    // reused for items that just got (at least partly) visible. If the
    // animation hint is set to 'Animation' items that do e.g. an animated
    // moving of their position are not marked as invisible: This assures
    // that a scrolling inside the view can be done without breaking an animation.

    QList<int> items;

    QHashIterator<int, KItemListWidget*> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();

        KItemListWidget* widget = it.value();
        const int index = widget->index();
        const bool invisible = (index < firstVisibleIndex) || (index > lastVisibleIndex);

        if (invisible) {
            if (m_animation->isStarted(widget)) {
                if (hint == NoAnimation) {
                    // Stopping the animation will call KItemListView::slotAnimationFinished()
                    // and the widget will be recycled if necessary there.
                    m_animation->stop(widget);
                }
            } else {
                widget->setVisible(false);
                items.append(index);

                if (m_grouped) {
                    recycleGroupHeaderForWidget(widget);
                }
            }
        }
    }

    return items;
}

bool KItemListView::moveWidget(KItemListWidget* widget,const QPointF& newPos)
{
    if (widget->pos() == newPos) {
        return false;
    }

    bool startMovingAnim = false;

    // When having a grid the moving-animation should only be started, if it is done within
    // one row in the vertical scroll-orientation or one column in the horizontal scroll-orientation.
    // Otherwise instead of a moving-animation a create-animation on the new position will be used
    // instead. This is done to prevent overlapping (and confusing) moving-animations.
    const int index = widget->index();
    const Cell cell = m_visibleCells.value(index);
    if (cell.column >= 0 && cell.row >= 0) {
        if (scrollOrientation() == Qt::Vertical) {
            startMovingAnim = (cell.row == m_layouter->itemRow(index));
        } else {
            startMovingAnim = (cell.column == m_layouter->itemColumn(index));
        }
    }

    if (startMovingAnim) {
        m_animation->start(widget, KItemListViewAnimation::MovingAnimation, newPos);
        return true;
    }

    m_animation->stop(widget);
    m_animation->start(widget, KItemListViewAnimation::CreateAnimation);
    return false;
}

void KItemListView::emitOffsetChanges()
{
    const qreal newScrollOffset = m_layouter->scrollOffset();
    if (m_oldScrollOffset != newScrollOffset) {
        emit scrollOffsetChanged(newScrollOffset, m_oldScrollOffset);
        m_oldScrollOffset = newScrollOffset;
    }

    const qreal newMaximumScrollOffset = m_layouter->maximumScrollOffset();
    if (m_oldMaximumScrollOffset != newMaximumScrollOffset) {
        emit maximumScrollOffsetChanged(newMaximumScrollOffset, m_oldMaximumScrollOffset);
        m_oldMaximumScrollOffset = newMaximumScrollOffset;
    }

    const qreal newItemOffset = m_layouter->itemOffset();
    if (m_oldItemOffset != newItemOffset) {
        emit itemOffsetChanged(newItemOffset, m_oldItemOffset);
        m_oldItemOffset = newItemOffset;
    }

    const qreal newMaximumItemOffset = m_layouter->maximumItemOffset();
    if (m_oldMaximumItemOffset != newMaximumItemOffset) {
        emit maximumItemOffsetChanged(newMaximumItemOffset, m_oldMaximumItemOffset);
        m_oldMaximumItemOffset = newMaximumItemOffset;
    }
}

KItemListWidget* KItemListView::createWidget(int index)
{
    KItemListWidget* widget = m_widgetCreator->create(this);
    widget->setFlag(QGraphicsItem::ItemStacksBehindParent);

    m_visibleItems.insert(index, widget);
    m_visibleCells.insert(index, Cell());
    updateWidgetProperties(widget, index);
    initializeItemListWidget(widget);
    return widget;
}

void KItemListView::recycleWidget(KItemListWidget* widget)
{
    if (m_grouped) {
        recycleGroupHeaderForWidget(widget);
    }

    const int index = widget->index();
    m_visibleItems.remove(index);
    m_visibleCells.remove(index);

    m_widgetCreator->recycle(widget);
}

void KItemListView::setWidgetIndex(KItemListWidget* widget, int index)
{
    const int oldIndex = widget->index();
    m_visibleItems.remove(oldIndex);
    m_visibleCells.remove(oldIndex);

    m_visibleItems.insert(index, widget);
    m_visibleCells.insert(index, Cell());

    widget->setIndex(index);
}

void KItemListView::moveWidgetToIndex(KItemListWidget* widget, int index)
{
    const int oldIndex = widget->index();
    const Cell oldCell = m_visibleCells.value(oldIndex);

    setWidgetIndex(widget, index);

    const Cell newCell(m_layouter->itemColumn(index), m_layouter->itemRow(index));
    const bool vertical = (scrollOrientation() == Qt::Vertical);
    const bool updateCell = (vertical  && oldCell.row    == newCell.row) ||
                            (!vertical && oldCell.column == newCell.column);
    if (updateCell) {
        m_visibleCells.insert(index, newCell);
    }
}

void KItemListView::setLayouterSize(const QSizeF& size, SizeType sizeType)
{
    switch (sizeType) {
    case LayouterSize: m_layouter->setSize(size); break;
    case ItemSize: m_layouter->setItemSize(size); break;
    default: break;
    }
}

void KItemListView::updateWidgetProperties(KItemListWidget* widget, int index)
{
    widget->setVisibleRoles(m_visibleRoles);
    widget->setVisibleRolesSizes(m_stretchedVisibleRolesSizes);
    widget->setStyleOption(m_styleOption);

    const KItemListSelectionManager* selectionManager = m_controller->selectionManager();
    widget->setCurrent(index == selectionManager->currentItem());
    widget->setSelected(selectionManager->isSelected(index));
    widget->setHovered(false);
    widget->setEnabledSelectionToggle(enabledSelectionToggles());
    widget->setIndex(index);
    widget->setData(m_model->data(index));
    widget->setSiblingsInformation(QBitArray());
    updateAlternateBackgroundForWidget(widget);

    if (m_grouped) {
        updateGroupHeaderForWidget(widget);
    }
}

void KItemListView::updateGroupHeaderForWidget(KItemListWidget* widget)
{
    Q_ASSERT(m_grouped);

    const int index = widget->index();
    if (!m_layouter->isFirstGroupItem(index)) {
        // The widget does not represent the first item of a group
        // and hence requires no header
        recycleGroupHeaderForWidget(widget);
        return;
    }

    const QList<QPair<int, QVariant> > groups = model()->groups();
    if (groups.isEmpty()) {
        return;
    }

    KItemListGroupHeader* groupHeader = m_visibleGroups.value(widget);
    if (!groupHeader) {
        groupHeader = m_groupHeaderCreator->create(this);
        groupHeader->setParentItem(widget);
        m_visibleGroups.insert(widget, groupHeader);
        connect(widget, SIGNAL(geometryChanged()), this, SLOT(slotGeometryOfGroupHeaderParentChanged()));
    }
    Q_ASSERT(groupHeader->parentItem() == widget);

    const int groupIndex = groupIndexForItem(index);
    Q_ASSERT(groupIndex >= 0);
    groupHeader->setData(groups.at(groupIndex).second);
    groupHeader->setRole(model()->sortRole());
    groupHeader->setStyleOption(m_styleOption);
    groupHeader->setScrollOrientation(scrollOrientation());
    groupHeader->setItemIndex(index);

    groupHeader->show();
}

void KItemListView::updateGroupHeaderLayout(KItemListWidget* widget)
{
    KItemListGroupHeader* groupHeader = m_visibleGroups.value(widget);
    Q_ASSERT(groupHeader);

    const int index = widget->index();
    const QRectF groupHeaderRect = m_layouter->groupHeaderRect(index);
    const QRectF itemRect = m_layouter->itemRect(index);

    // The group-header is a child of the itemlist widget. Translate the
    // group header position to the relative position.
    if (scrollOrientation() == Qt::Vertical) {
        // In the vertical scroll orientation the group header should always span
        // the whole width no matter which temporary position the parent widget
        // has. In this case the x-position and width will be adjusted manually.
        groupHeader->setPos(-widget->x(), -groupHeaderRect.height());
        groupHeader->resize(size().width(), groupHeaderRect.size().height());
    } else {
        groupHeader->setPos(groupHeaderRect.x() - itemRect.x(), -widget->y());
        groupHeader->resize(groupHeaderRect.size());
    }
}

void KItemListView::recycleGroupHeaderForWidget(KItemListWidget* widget)
{
    KItemListGroupHeader* header = m_visibleGroups.value(widget);
    if (header) {
        header->setParentItem(0);
        m_groupHeaderCreator->recycle(header);
        m_visibleGroups.remove(widget);
        disconnect(widget, SIGNAL(geometryChanged()), this, SLOT(slotGeometryOfGroupHeaderParentChanged()));
    }
}

void KItemListView::updateVisibleGroupHeaders()
{
    Q_ASSERT(m_grouped);
    m_layouter->markAsDirty();

    QHashIterator<int, KItemListWidget*> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        updateGroupHeaderForWidget(it.value());
    }
}

int KItemListView::groupIndexForItem(int index) const
{
    Q_ASSERT(m_grouped);

    const QList<QPair<int, QVariant> > groups = model()->groups();
    if (groups.isEmpty()) {
        return -1;
    }

    int min = 0;
    int max = groups.count() - 1;
    int mid = 0;
    do {
        mid = (min + max) / 2;
        if (index > groups[mid].first) {
            min = mid + 1;
        } else {
            max = mid - 1;
        }
    } while (groups[mid].first != index && min <= max);

    if (min > max) {
        while (groups[mid].first > index && mid > 0) {
            --mid;
        }
    }

    return mid;
}

void KItemListView::updateAlternateBackgroundForWidget(KItemListWidget* widget)
{
    bool enabled = useAlternateBackgrounds();
    if (enabled) {
        const int index = widget->index();
        enabled = (index & 0x1) > 0;
        if (m_grouped) {
            const int groupIndex = groupIndexForItem(index);
            if (groupIndex >= 0) {
                const QList<QPair<int, QVariant> > groups = model()->groups();
                const int indexOfFirstGroupItem = groups[groupIndex].first;
                const int relativeIndex = index - indexOfFirstGroupItem;
                enabled = (relativeIndex & 0x1) > 0;
            }
        }
    }
    widget->setAlternateBackground(enabled);
}

bool KItemListView::useAlternateBackgrounds() const
{
    return m_itemSize.isEmpty() && m_visibleRoles.count() > 1;
}

QHash<QByteArray, qreal> KItemListView::headerRolesWidths() const
{
    QHash<QByteArray, qreal> rolesWidths;

    QHashIterator<QByteArray, QSizeF> it(m_stretchedVisibleRolesSizes);
    while (it.hasNext()) {
        it.next();
        rolesWidths.insert(it.key(), it.value().width());
    }

    return rolesWidths;
}

void KItemListView::updateVisibleRolesSizes(const KItemRangeList& itemRanges)
{
    if (!m_itemSize.isEmpty() || m_useHeaderWidths) {
        return;
    }

    const int itemCount = m_model->count();
    int rangesItemCount = 0;
    foreach (const KItemRange& range, itemRanges) {
        rangesItemCount += range.count;
    }

    if (itemCount == rangesItemCount) {
        m_visibleRolesSizes = visibleRolesSizes(itemRanges);
        if (m_header) {
            // Assure the the sizes are not smaller than the minimum defined by the header
            // TODO: Currently only implemented for a top-aligned header
            const qreal minHeaderRoleWidth = m_header->minimumRoleWidth();
            QMutableHashIterator<QByteArray, QSizeF> it (m_visibleRolesSizes);
            while (it.hasNext()) {
                it.next();
                const QSizeF& size = it.value();
                if (size.width() < minHeaderRoleWidth) {
                    const QSizeF newSize(minHeaderRoleWidth, size.height());
                    m_visibleRolesSizes.insert(it.key(), newSize);
                }
            }
        }
    } else {
        // Only a sub range of the roles need to be determined.
        // The chances are good that the sizes of the sub ranges
        // already fit into the available sizes and hence no
        // expensive update might be required.
        bool updateRequired = false;

        const QHash<QByteArray, QSizeF> updatedSizes = visibleRolesSizes(itemRanges);
        QHashIterator<QByteArray, QSizeF> it(updatedSizes);
        while (it.hasNext()) {
            it.next();
            const QByteArray& role = it.key();
            const QSizeF& updatedSize = it.value();
            const QSizeF currentSize = m_visibleRolesSizes.value(role);
            if (updatedSize.width() > currentSize.width() || updatedSize.height() > currentSize.height()) {
                m_visibleRolesSizes.insert(role, updatedSize);
                updateRequired = true;
            }
        }

        if (!updateRequired) {
            // All the updated sizes are smaller than the current sizes and no change
            // of the stretched roles-widths is required
            return;
        }
    }

    updateStretchedVisibleRolesSizes();
}

void KItemListView::updateVisibleRolesSizes()
{
    if (!m_model) {
        return;
    }

    const int itemCount = m_model->count();
    if (itemCount > 0) {
        updateVisibleRolesSizes(KItemRangeList() << KItemRange(0, itemCount));
    }
}

void KItemListView::updateStretchedVisibleRolesSizes()
{
    if (!m_itemSize.isEmpty() || m_useHeaderWidths || m_visibleRoles.isEmpty()) {
        return;
    }

    // Calculate the maximum size of an item by considering the
    // visible role sizes and apply them to the layouter. If the
    // size does not use the available view-size it the size of the
    // first role will get stretched.
    m_stretchedVisibleRolesSizes = m_visibleRolesSizes;
    const QByteArray role = m_visibleRoles.first();
    QSizeF firstRoleSize = m_stretchedVisibleRolesSizes.value(role);

    QSizeF dynamicItemSize = m_itemSize;

    if (dynamicItemSize.width() <= 0) {
        const qreal requiredWidth = visibleRolesSizesWidthSum();
        const qreal availableWidth = size().width();
        if (requiredWidth < availableWidth) {
            // Stretch the first role to use the whole width for the item
            firstRoleSize.rwidth() += availableWidth - requiredWidth;
            m_stretchedVisibleRolesSizes.insert(role, firstRoleSize);
        }
        dynamicItemSize.setWidth(qMax(requiredWidth, availableWidth));
    }

    if (dynamicItemSize.height() <= 0) {
        const qreal requiredHeight = visibleRolesSizesHeightSum();
        const qreal availableHeight = size().height();
        if (requiredHeight < availableHeight) {
            // Stretch the first role to use the whole height for the item
            firstRoleSize.rheight() += availableHeight - requiredHeight;
            m_stretchedVisibleRolesSizes.insert(role, firstRoleSize);
        }
        dynamicItemSize.setHeight(qMax(requiredHeight, availableHeight));
    }

    m_layouter->setItemSize(dynamicItemSize);

    if (m_header) {
        m_header->setVisibleRolesWidths(headerRolesWidths());
        m_header->resize(dynamicItemSize.width(), m_header->size().height());
    }

    // Update the role sizes for all visible widgets
    QHashIterator<int, KItemListWidget*> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        it.value()->setVisibleRolesSizes(m_stretchedVisibleRolesSizes);
    }
}

qreal KItemListView::visibleRolesSizesWidthSum() const
{
    qreal widthSum = 0;
    QHashIterator<QByteArray, QSizeF> it(m_visibleRolesSizes);
    while (it.hasNext()) {
        it.next();
        widthSum += it.value().width();
    }
    return widthSum;
}

qreal KItemListView::visibleRolesSizesHeightSum() const
{
    qreal heightSum = 0;
    QHashIterator<QByteArray, QSizeF> it(m_visibleRolesSizes);
    while (it.hasNext()) {
        it.next();
        heightSum += it.value().height();
    }
    return heightSum;
}

QRectF KItemListView::headerBoundaries() const
{
    return m_header ? m_header->geometry() : QRectF();
}

bool KItemListView::changesItemGridLayout(const QSizeF& newGridSize,
                                          const QSizeF& newItemSize,
                                          const QSizeF& newItemMargin) const
{
    if (newItemSize.isEmpty() || newGridSize.isEmpty()) {
        return false;
    }

    if (m_layouter->scrollOrientation() == Qt::Vertical) {
        const qreal itemWidth = m_layouter->itemSize().width();
        if (itemWidth > 0) {
            const int newColumnCount = itemsPerSize(newGridSize.width(),
                                                    newItemSize.width(),
                                                    newItemMargin.width());
            if (m_model->count() > newColumnCount) {
                const int oldColumnCount = itemsPerSize(m_layouter->size().width(),
                                                        itemWidth,
                                                        m_layouter->itemMargin().width());
                return oldColumnCount != newColumnCount;
            }
        }
    } else {
        const qreal itemHeight = m_layouter->itemSize().height();
        if (itemHeight > 0) {
            const int newRowCount = itemsPerSize(newGridSize.height(),
                                                 newItemSize.height(),
                                                 newItemMargin.height());
            if (m_model->count() > newRowCount) {
                const int oldRowCount = itemsPerSize(m_layouter->size().height(),
                                                     itemHeight,
                                                     m_layouter->itemMargin().height());
                return oldRowCount != newRowCount;
            }
        }
    }

    return false;
}

bool KItemListView::animateChangedItemCount(int changedItemCount) const
{
    if (m_layouter->size().isEmpty() || m_layouter->itemSize().isEmpty()) {
        return false;
    }

    const int maximum = (scrollOrientation() == Qt::Vertical)
                        ? m_layouter->size().width()  / m_layouter->itemSize().width()
                        : m_layouter->size().height() / m_layouter->itemSize().height();
    // Only animate if up to 2/3 of a row or column are inserted or removed
    return changedItemCount <= maximum * 2 / 3;
}


bool KItemListView::scrollBarRequired(const QSizeF& size) const
{
    const QSizeF oldSize = m_layouter->size();

    m_layouter->setSize(size);
    const qreal maxOffset = m_layouter->maximumScrollOffset();
    m_layouter->setSize(oldSize);

    return m_layouter->scrollOrientation() == Qt::Vertical ? maxOffset > size.height()
                                                           : maxOffset > size.width();
}

void KItemListView::updateGroupHeaderHeight()
{
    qreal groupHeaderHeight = m_styleOption.fontMetrics.height();
    qreal groupHeaderMargin = 0;

    if (scrollOrientation() == Qt::Horizontal) {
        // The vertical margin above and below the header should be
        // equal to the horizontal margin, not the vertical margin
        // from m_styleOption.
        groupHeaderHeight += 2 * m_styleOption.horizontalMargin;
        groupHeaderMargin = m_styleOption.horizontalMargin;
    } else if (m_itemSize.isEmpty()){
        groupHeaderHeight += 4 * m_styleOption.padding;
        groupHeaderMargin = m_styleOption.iconSize / 2;
    } else {
        groupHeaderHeight += 2 * m_styleOption.padding + m_styleOption.verticalMargin;
        groupHeaderMargin = m_styleOption.iconSize / 4;
    }
    m_layouter->setGroupHeaderHeight(groupHeaderHeight);
    m_layouter->setGroupHeaderMargin(groupHeaderMargin);

    updateVisibleGroupHeaders();
}

void KItemListView::updateSiblingsInformation(int firstIndex, int lastIndex)
{
    if (!supportsItemExpanding() || !m_model) {
        return;
    }

    if (firstIndex < 0 || lastIndex < 0) {
        firstIndex = m_layouter->firstVisibleIndex();
        lastIndex  = m_layouter->lastVisibleIndex();
    } else {
        const bool isRangeVisible = (firstIndex <= m_layouter->lastVisibleIndex() &&
                                     lastIndex  >= m_layouter->firstVisibleIndex());
        if (!isRangeVisible) {
            return;
        }
    }

    int previousParents = 0;
    QBitArray previousSiblings;

    // The rootIndex describes the first index where the siblings get
    // calculated from. For the calculation the upper most parent item
    // is required. For performance reasons it is checked first whether
    // the visible items before or after the current range already
    // contain a siblings information which can be used as base.
    int rootIndex = firstIndex;

    KItemListWidget* widget = m_visibleItems.value(firstIndex - 1);
    if (!widget) {
        // There is no visible widget before the range, check whether there
        // is one after the range:
        widget = m_visibleItems.value(lastIndex + 1);
        if (widget) {
            // The sibling information of the widget may only be used if
            // all items of the range have the same number of parents.
            const int parents = m_model->expandedParentsCount(lastIndex + 1);
            for (int i = lastIndex; i >= firstIndex; --i) {
                if (m_model->expandedParentsCount(i) != parents) {
                    widget = 0;
                    break;
                }
            }
        }
    }

    if (widget) {
        // Performance optimization: Use the sibling information of the visible
        // widget beside the given range.
        previousSiblings = widget->siblingsInformation();
        if (previousSiblings.isEmpty()) {
            return;
        }
        previousParents = previousSiblings.count() - 1;
        previousSiblings.truncate(previousParents);
    } else {
        // Potentially slow path: Go back to the upper most parent of firstIndex
        // to be able to calculate the initial value for the siblings.
        while (rootIndex > 0 && m_model->expandedParentsCount(rootIndex) > 0) {
            --rootIndex;
        }
    }

    Q_ASSERT(previousParents >= 0);
    for (int i = rootIndex; i <= lastIndex; ++i) {
        // Update the parent-siblings in case if the current item represents
        // a child or an upper parent.
        const int currentParents = m_model->expandedParentsCount(i);
        Q_ASSERT(currentParents >= 0);
        if (previousParents < currentParents) {
            previousParents = currentParents;
            previousSiblings.resize(currentParents);
            previousSiblings.setBit(currentParents - 1, hasSiblingSuccessor(i - 1));
        } else if (previousParents > currentParents) {
            previousParents = currentParents;
            previousSiblings.truncate(currentParents);
        }

        if (i >= firstIndex) {
            // The index represents a visible item. Apply the parent-siblings
            // and update the sibling of the current item.
            KItemListWidget* widget = m_visibleItems.value(i);
            if (!widget) {
                continue;
            }

            QBitArray siblings = previousSiblings;
            siblings.resize(siblings.count() + 1);
            siblings.setBit(siblings.count() - 1, hasSiblingSuccessor(i));

            widget->setSiblingsInformation(siblings);
        }
    }
}

bool KItemListView::hasSiblingSuccessor(int index) const
{
    bool hasSuccessor = false;
    const int parentsCount = m_model->expandedParentsCount(index);
    int successorIndex = index + 1;

    // Search the next sibling
    const int itemCount = m_model->count();
    while (successorIndex < itemCount) {
        const int currentParentsCount = m_model->expandedParentsCount(successorIndex);
        if (currentParentsCount == parentsCount) {
            hasSuccessor = true;
            break;
        } else if (currentParentsCount < parentsCount) {
            break;
        }
        ++successorIndex;
    }

    if (m_grouped && hasSuccessor) {
        // If the sibling is part of another group, don't mark it as
        // successor as the group header is between the sibling connections.
        for (int i = index + 1; i <= successorIndex; ++i) {
            if (m_layouter->isFirstGroupItem(i)) {
                hasSuccessor = false;
                break;
            }
        }
    }

    return hasSuccessor;
}

int KItemListView::calculateAutoScrollingIncrement(int pos, int range, int oldInc)
{
    int inc = 0;

    const int minSpeed = 4;
    const int maxSpeed = 128;
    const int speedLimiter = 96;
    const int autoScrollBorder = 64;

    // Limit the increment that is allowed to be added in comparison to 'oldInc'.
    // This assures that the autoscrolling speed grows gradually.
    const int incLimiter = 1;

    if (pos < autoScrollBorder) {
        inc = -minSpeed + qAbs(pos - autoScrollBorder) * (pos - autoScrollBorder) / speedLimiter;
        inc = qMax(inc, -maxSpeed);
        inc = qMax(inc, oldInc - incLimiter);
    } else if (pos > range - autoScrollBorder) {
        inc = minSpeed + qAbs(pos - range + autoScrollBorder) * (pos - range + autoScrollBorder) / speedLimiter;
        inc = qMin(inc, maxSpeed);
        inc = qMin(inc, oldInc + incLimiter);
    }

    return inc;
}

int KItemListView::itemsPerSize(qreal size, qreal itemSize, qreal itemMargin)
{
    const qreal availableSize = size - itemMargin;
    const int count = availableSize / (itemSize + itemMargin);
    return count;
}



KItemListCreatorBase::~KItemListCreatorBase()
{
    qDeleteAll(m_recycleableWidgets);
    qDeleteAll(m_createdWidgets);
}

void KItemListCreatorBase::addCreatedWidget(QGraphicsWidget* widget)
{
    m_createdWidgets.insert(widget);
}

void KItemListCreatorBase::pushRecycleableWidget(QGraphicsWidget* widget)
{
    Q_ASSERT(m_createdWidgets.contains(widget));
    m_createdWidgets.remove(widget);

    if (m_recycleableWidgets.count() < 100) {
        m_recycleableWidgets.append(widget);
        widget->setVisible(false);
    } else {
        delete widget;
    }
}

QGraphicsWidget* KItemListCreatorBase::popRecycleableWidget()
{
    if (m_recycleableWidgets.isEmpty()) {
        return 0;
    }

    QGraphicsWidget* widget = m_recycleableWidgets.takeLast();
    m_createdWidgets.insert(widget);
    return widget;
}

KItemListWidgetCreatorBase::~KItemListWidgetCreatorBase()
{
}

void KItemListWidgetCreatorBase::recycle(KItemListWidget* widget)
{
    widget->setParentItem(0);
    widget->setOpacity(1.0);
    pushRecycleableWidget(widget);
}

KItemListGroupHeaderCreatorBase::~KItemListGroupHeaderCreatorBase()
{
}

void KItemListGroupHeaderCreatorBase::recycle(KItemListGroupHeader* header)
{
    header->setOpacity(1.0);
    pushRecycleableWidget(header);
}

#include "kitemlistview.moc"
