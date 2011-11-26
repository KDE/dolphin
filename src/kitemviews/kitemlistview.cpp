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
    m_activeTransactions(0),
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
    }

    doLayout(Animation, 0, 0);

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

    m_itemSize = itemSize;

    const bool emptySize = itemSize.isEmpty();
    if (emptySize) {
        updateVisibleRolesSizes();
    } else {
        if (itemSize.width() < previousSize.width() || itemSize.height() < previousSize.height()) {
            prepareLayoutForIncreasedItemCount(itemSize, ItemSize);
        } else {
            m_layouter->setItemSize(itemSize);
        }
    }

    m_sizeHintResolver->clearCache();
    doLayout(Animation, 0, 0);
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
    if (!m_layoutTimer->isActive()) {
        doLayout(NoAnimation, 0, 0);
    }
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
    m_layouter->setItemOffset(offset);
    if (m_header) {
        m_header->setPos(-offset, 0);
    }
    if (!m_layoutTimer->isActive()) {
        doLayout(NoAnimation, 0, 0);
    }
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

    QHashIterator<int, KItemListWidget*> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        KItemListWidget* widget = it.value();
        widget->setVisibleRoles(roles);
        widget->setVisibleRolesSizes(m_stretchedVisibleRolesSizes);
    }

    m_sizeHintResolver->clearCache();
    m_layouter->markAsDirty();

    if (m_header) {
        m_header->setVisibleRoles(roles);
        m_header->setVisibleRolesWidths(headerRolesWidths());
        m_useHeaderWidths = false;
    }

    updateVisibleRolesSizes();
    doLayout(Animation, 0, 0);

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
        m_autoScrollTimer->setSingleShot(false);
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

    QHashIterator<int, KItemListWidget*> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        it.value()->setStyleOption(option);
    }

    m_sizeHintResolver->clearCache();
    doLayout(Animation, 0, 0);
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

    if (m_model->count() > 0) {
        prepareLayoutForIncreasedItemCount(rect.size(), LayouterSize);
    } else {
        m_layouter->setSize(rect.size());
    }

    if (!m_layoutTimer->isActive()) {
        m_layoutTimer->start();
    }

    // Changing the geometry does not require to do an expensive
    // update of the visible-roles sizes, only the stretched sizes
    // need to be adjusted to the new size.
    updateStretchedVisibleRolesSizes();
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
    return false;
}

QRectF KItemListView::itemRect(int index) const
{
    return m_layouter->itemRect(index);
}

void KItemListView::scrollToItem(int index)
{
    const QRectF viewGeometry = geometry();
    const QRectF currentRect = itemRect(index);

    if (!viewGeometry.contains(currentRect)) {
        qreal newOffset = scrollOffset();
        if (currentRect.top() < viewGeometry.top()) {
            Q_ASSERT(scrollOrientation() == Qt::Vertical);
            newOffset += currentRect.top() - viewGeometry.top();
        } else if ((currentRect.bottom() > viewGeometry.bottom())) {
            Q_ASSERT(scrollOrientation() == Qt::Vertical);
            newOffset += currentRect.bottom() - viewGeometry.bottom();
        } else if (currentRect.left() < viewGeometry.left()) {
            if (scrollOrientation() == Qt::Horizontal) {
                newOffset += currentRect.left() - viewGeometry.left();
            }
        } else if ((currentRect.right() > viewGeometry.right())) {
            if (scrollOrientation() == Qt::Horizontal) {
                newOffset += currentRect.right() - viewGeometry.right();
            }
        }

        if (newOffset != scrollOffset()) {
            emit scrollTo(newOffset);
        }
    }
}

int KItemListView::itemsPerOffset() const
{
    return m_layouter->itemsPerOffset();
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
        doLayout(Animation, 0, 0);
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

        m_useHeaderWidths = false;

        connect(m_header, SIGNAL(visibleRoleWidthChanged(QByteArray,qreal,qreal)),
                this, SLOT(slotVisibleRoleWidthChanged(QByteArray,qreal,qreal)));

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

void KItemListView::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    QGraphicsWidget::resizeEvent(event);
    if (m_itemSize.isEmpty() && m_useHeaderWidths) {
        QSizeF dynamicItemSize = m_layouter->itemSize();
        const QSizeF newSize = event->newSize();

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
}

void KItemListView::slotItemsInserted(const KItemRangeList& itemRanges)
{
    updateVisibleRolesSizes(itemRanges);

    const bool hasMultipleRanges = (itemRanges.count() > 1);
    if (hasMultipleRanges) {
        beginTransaction();
    }

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
            setWidgetIndex(widget, widget->index() + count);
        }

        m_layouter->markAsDirty();
        if (m_model->count() == count && maximumScrollOffset() > size().height()) {
            kDebug() << "Scrollbar required, skipping layout";
            const int scrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
            QSizeF layouterSize = m_layouter->size();
            if (scrollOrientation() == Qt::Vertical) {
                layouterSize.rwidth() -= scrollBarExtent;
            } else {
                layouterSize.rheight() -= scrollBarExtent;
            }
            m_layouter->setSize(layouterSize);
        }

        if (!hasMultipleRanges) {
            doLayout(Animation, index, count);
        }
    }

    if (m_controller) {
        m_controller->selectionManager()->itemsInserted(itemRanges);
    }

    if (hasMultipleRanges) {
        endTransaction();
    }
}

void KItemListView::slotItemsRemoved(const KItemRangeList& itemRanges)
{
    updateVisibleRolesSizes();

    const bool hasMultipleRanges = (itemRanges.count() > 1);
    if (hasMultipleRanges) {
        beginTransaction();
    }

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

            if (m_model->count() == 0) {
                // For performance reasons no animation is done when all items have
                // been removed.
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
                setWidgetIndex(widget, newIndex);
            }
        }

        m_layouter->markAsDirty();
        if (!hasMultipleRanges) {
            doLayout(Animation, index, -count);
        }
    }

    if (m_controller) {
        m_controller->selectionManager()->itemsRemoved(itemRanges);
    }

    if (hasMultipleRanges) {
        endTransaction();
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
            if (m_grouped) {
                updateGroupHeaderForWidget(widget);
            }
            initializeItemListWidget(widget);
        }
    }

    doLayout(NoAnimation, 0, 0);
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
            doLayout(NoAnimation, 0, 0);
        }
    }
}

void KItemListView::slotGroupedSortingChanged(bool current)
{
    m_grouped = current;
    m_layouter->markAsDirty();

    if (m_grouped) {
        // Apply the height of the header to the layouter
        const qreal groupHeaderHeight = m_styleOption.fontMetrics.height() +
                                        m_styleOption.margin * 2;
        m_layouter->setGroupHeaderHeight(groupHeaderHeight);

        updateVisibleGroupHeaders();
    } else {
        // Clear all visible headers
        QMutableHashIterator<KItemListWidget*, KItemListGroupHeader*> it (m_visibleGroups);
        while (it.hasNext()) {
            it.next();
            recycleGroupHeaderForWidget(it.key());
        }
        Q_ASSERT(m_visibleGroups.isEmpty());
    }

    doLayout(Animation, 0, 0);
}

void KItemListView::slotSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    if (m_grouped) {
        updateVisibleGroupHeaders();
        doLayout(Animation, 0, 0);
    }
}

void KItemListView::slotSortRoleChanged(const QByteArray& current, const QByteArray& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    if (m_grouped) {
        updateVisibleGroupHeaders();
        doLayout(Animation, 0, 0);
    }
}

void KItemListView::slotCurrentChanged(int current, int previous)
{
    Q_UNUSED(previous);

    KItemListWidget* previousWidget = m_visibleItems.value(previous, 0);
    if (previousWidget) {
        Q_ASSERT(previousWidget->isCurrent());
        previousWidget->setCurrent(false);
    }

    KItemListWidget* currentWidget = m_visibleItems.value(current, 0);
    if (currentWidget) {
        Q_ASSERT(!currentWidget->isCurrent());
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
    doLayout(Animation, 0, 0);
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
        foreach (KItemListWidget* widget, visibleItemListWidgets()) {
            widget->setVisibleRolesSizes(m_stretchedVisibleRolesSizes);
        }

        doLayout(Animation, 0, 0);
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

    setScrollOffset(scrollOffset() + m_autoScrollIncrement);

   // Trigger the autoscroll timer which will periodically call
   // triggerAutoScrolling()
   m_autoScrollTimer->start(RepeatingAutoScrollDelay);
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
        kDebug() << "Stopping layout timer, synchronous layout requested";
        m_layoutTimer->stop();
    }

    if (!m_model || m_model->count() < 0 || m_activeTransactions > 0) {
        return;
    }

    const int firstVisibleIndex = m_layouter->firstVisibleIndex();
    const int lastVisibleIndex = m_layouter->lastVisibleIndex();
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
    }

    // Determine all items that are completely invisible and might be
    // reused for items that just got (at least partly) visible.
    // Items that do e.g. an animated moving of their position are not
    // marked as invisible: This assures that a scrolling inside the view
    // can be done without breaking an animation.
    QList<int> reusableItems;
    QHashIterator<int, KItemListWidget*> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        KItemListWidget* widget = it.value();
        const int index = widget->index();
        const bool invisible = (index < firstVisibleIndex) || (index > lastVisibleIndex);
        if (invisible && !m_animation->isStarted(widget)) {
            widget->setVisible(false);
            reusableItems.append(index);

            if (m_grouped) {
                recycleGroupHeaderForWidget(widget);
            }
        }
    }

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

                if (m_grouped) {
                    updateGroupHeaderForWidget(widget);
                }
            } else {
                // No reusable KItemListWidget instance is available, create a new one
                widget = createWidget(i);
            }
            widget->resize(itemBounds.size());

            if (animate && changedCount < 0) {
                // Items have been deleted, move the created item to the
                // imaginary old position.
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
        } else if (m_animation->isStarted(widget, KItemListViewAnimation::MovingAnimation)) {
            applyNewPos = false;
        }

        if (animate) {
            const bool itemsRemoved = (changedCount < 0);
            const bool itemsInserted = (changedCount > 0);

            if (itemsRemoved && (i >= changedIndex + changedCount + 1)) {
                // The item is located after the removed items. Animate the moving of the position.
                m_animation->start(widget, KItemListViewAnimation::MovingAnimation, newPos);
                applyNewPos = false;
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
                    m_animation->start(widget, KItemListViewAnimation::MovingAnimation, newPos);
                    applyNewPos = false;
                }
            } else if (!itemsRemoved && !itemsInserted && !wasHidden) {
                // The size of the view might have been changed. Animate the moving of the position.
                m_animation->start(widget, KItemListViewAnimation::MovingAnimation, newPos);
                applyNewPos = false;
            }
        }

        if (applyNewPos) {
            widget->setPos(newPos);
        }

        Q_ASSERT(widget->index() == i);
        widget->setVisible(true);

        if (widget->size() != itemBounds.size()) {
            if (animate) {
                m_animation->start(widget, KItemListViewAnimation::ResizeAnimation, itemBounds.size());
            } else {
                widget->resize(itemBounds.size());
            }
        }
    }

    // Delete invisible KItemListWidget instances that have not been reused
    foreach (int index, reusableItems) {
        recycleWidget(m_visibleItems.value(index));
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

    updateWidgetProperties(widget, index);
    m_visibleItems.insert(index, widget);

    if (m_grouped) {
        updateGroupHeaderForWidget(widget);
    }

    initializeItemListWidget(widget);
    return widget;
}

void KItemListView::recycleWidget(KItemListWidget* widget)
{
    if (m_grouped) {
        recycleGroupHeaderForWidget(widget);
    }

    m_visibleItems.remove(widget->index());
    m_widgetCreator->recycle(widget);
}

void KItemListView::setWidgetIndex(KItemListWidget* widget, int index)
{
    const int oldIndex = widget->index();
    m_visibleItems.remove(oldIndex);
    updateWidgetProperties(widget, index);
    m_visibleItems.insert(index, widget);

    initializeItemListWidget(widget);
}

void KItemListView::prepareLayoutForIncreasedItemCount(const QSizeF& size, SizeType sizeType)
{
    // Calculate the first visible index and last visible index for the current size
    const int currentFirst = m_layouter->firstVisibleIndex();
    const int currentLast = m_layouter->lastVisibleIndex();

    const QSizeF currentSize = (sizeType == LayouterSize) ? m_layouter->size() : m_layouter->itemSize();

    // Calculate the first visible index and last visible index for the new size
    setLayouterSize(size, sizeType);
    const int newFirst = m_layouter->firstVisibleIndex();
    const int newLast = m_layouter->lastVisibleIndex();

    if ((currentFirst != newFirst) || (currentLast != newLast)) {
        // At least one index has been changed. Assure that widgets for all possible
        // visible items get created so that a move-animation can be started later.
        const int maxVisibleItems = m_layouter->maximumVisibleItems();
        int minFirst = qMin(newFirst, currentFirst);
        const int maxLast = qMax(newLast, currentLast);

        if (maxLast - minFirst + 1 < maxVisibleItems) {
            // Increasing the size might result in a smaller KItemListView::offset().
            // Decrease the first visible index in a way that at least the maximum
            // visible items are shown.
            minFirst = qMax(0, maxLast - maxVisibleItems + 1);
        }

        if (maxLast - minFirst > maxVisibleItems  + maxVisibleItems / 2) {
            // The creating of widgets is quite expensive. Assure that never more
            // than 50 % of the maximum visible items get created for the animations.
            return;
        }

        setLayouterSize(currentSize, sizeType);
        for (int i = minFirst; i <= maxLast; ++i) {
            if (!m_visibleItems.contains(i)) {
                KItemListWidget* widget = createWidget(i);
                const QRectF itemRect = m_layouter->itemRect(i);
                widget->setPos(itemRect.topLeft());
                widget->resize(itemRect.size());
            }
        }
        setLayouterSize(size, sizeType);
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
    widget->setAlternatingBackgroundColors(false);
    widget->setEnabledSelectionToggle(enabledSelectionToggles());
    widget->setIndex(index);
    widget->setData(m_model->data(index));
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

    KItemListGroupHeader* header = m_visibleGroups.value(widget);
    if (!header) {
        header = m_groupHeaderCreator->create(this);
        header->setParentItem(widget);
        m_visibleGroups.insert(widget, header);
    }
    Q_ASSERT(header->parentItem() == widget);

    // Determine the shown data for the header by doing a binary
    // search in the groups-list
    int min = 0;
    int max = groups.count() - 1;
    int mid = 0;
    do {
        mid = (min + max) / 2;
        if (index > groups.at(mid).first) {
            min = mid + 1;
        } else {
            max = mid - 1;
        }
    } while (groups.at(mid).first != index && min <= max);

    header->setData(groups.at(mid).second);
    header->setRole(model()->sortRole());
    header->setStyleOption(m_styleOption);
    header->setScrollOrientation(scrollOrientation());

    header->show();
}

void KItemListView::updateGroupHeaderLayout(KItemListWidget* widget)
{
    KItemListGroupHeader* header = m_visibleGroups.value(widget);
    Q_ASSERT(header);

    const int index = widget->index();
    const QRectF groupHeaderRect = m_layouter->groupHeaderRect(index);
    const QRectF itemRect = m_layouter->itemRect(index);

    // The group-header is a child of the itemlist widget. Translate the
    // group header position to the relative position.
    const QPointF groupHeaderPos(groupHeaderRect.x() - itemRect.x(),
                                 - groupHeaderRect.height());
    header->setPos(groupHeaderPos);
    header->resize(groupHeaderRect.size());
}

void KItemListView::recycleGroupHeaderForWidget(KItemListWidget* widget)
{
    KItemListGroupHeader* header = m_visibleGroups.value(widget);
    if (header) {
        header->setParentItem(0);
        m_groupHeaderCreator->recycle(header);
        m_visibleGroups.remove(widget);
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
    foreach (KItemListWidget* widget, visibleItemListWidgets()) {
        widget->setVisibleRolesSizes(m_stretchedVisibleRolesSizes);
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
