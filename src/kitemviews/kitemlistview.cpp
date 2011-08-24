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
#include "kitemlistgroupheader.h"
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

KItemListView::KItemListView(QGraphicsWidget* parent) :
    QGraphicsWidget(parent),
    m_autoScrollMarginEnabled(false),
    m_grouped(false),
    m_activeTransactions(0),
    m_itemSize(),
    m_controller(0),
    m_model(0),
    m_visibleRoles(),
    m_visibleRolesSizes(),
    m_widgetCreator(0),
    m_groupHeaderCreator(0),
    m_styleOption(),
    m_visibleItems(),
    m_visibleGroups(),
    m_sizeHintResolver(0),
    m_layouter(0),
    m_animation(0),
    m_layoutTimer(0),
    m_oldOffset(0),
    m_oldMaximumOffset(0),
    m_rubberBand(0),
    m_mousePos()
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
    updateLayout();
    onScrollOrientationChanged(orientation, previousOrientation);
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

    if (!markVisibleRolesSizesAsDirty()) {
        if (itemSize.width() < previousSize.width() || itemSize.height() < previousSize.height()) {
            prepareLayoutForIncreasedItemCount(itemSize, ItemSize);
        } else {
            m_layouter->setItemSize(itemSize);
        }
    }

    m_sizeHintResolver->clearCache();
    updateLayout();
    onItemSizeChanged(itemSize, previousSize);
}

QSizeF KItemListView::itemSize() const
{
    return m_itemSize;
}

void KItemListView::setOffset(qreal offset)
{
    if (offset < 0) {
        offset = 0;
    }

    const qreal previousOffset = m_layouter->offset();
    if (offset == previousOffset) {
        return;
    }

    m_layouter->setOffset(offset);
    m_animation->setOffset(offset);
    if (!m_layoutTimer->isActive()) {
        doLayout(NoAnimation, 0, 0);
        update();
    }
    onOffsetChanged(offset, previousOffset);
}

qreal KItemListView::offset() const
{
    return m_layouter->offset();
}

qreal KItemListView::maximumOffset() const
{
    return m_layouter->maximumOffset();
}

void KItemListView::setVisibleRoles(const QHash<QByteArray, int>& roles)
{
    const QHash<QByteArray, int> previousRoles = m_visibleRoles;
    m_visibleRoles = roles;

    QHashIterator<int, KItemListWidget*> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        KItemListWidget* widget = it.value();
        widget->setVisibleRoles(roles);
        widget->setVisibleRolesSizes(m_visibleRolesSizes);
    }

    m_sizeHintResolver->clearCache();
    m_layouter->markAsDirty();
    onVisibleRolesChanged(roles, previousRoles);

    markVisibleRolesSizesAsDirty();
    updateLayout();
}

QHash<QByteArray, int> KItemListView::visibleRoles() const
{
    return m_visibleRoles;
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
    updateLayout();
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

    if (m_itemSize.isEmpty()) {
        m_layouter->setItemSize(QSizeF());
    }

    if (m_model->count() > 0) {
        prepareLayoutForIncreasedItemCount(rect.size(), LayouterSize);
    } else {
        m_layouter->setSize(rect.size());
    }

    m_layoutTimer->start();
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
    Q_UNUSED(index);
    Q_UNUSED(pos);
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

QHash<QByteArray, QSizeF> KItemListView::visibleRoleSizes() const
{
    return QHash<QByteArray, QSizeF>();
}

QRectF KItemListView::itemBoundingRect(int index) const
{
    return m_layouter->itemBoundingRect(index);
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
        updateLayout();
    }
}

bool KItemListView::isTransactionActive() const
{
    return m_activeTransactions > 0;
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
            rubberBandRect.moveTo(topLeft.x(), topLeft.y() - offset());
        } else {
            rubberBandRect.moveTo(topLeft.x() - offset(), topLeft.y());
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

void KItemListView::onOffsetChanged(qreal current, qreal previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListView::onVisibleRolesChanged(const QHash<QByteArray, int>& current, const QHash<QByteArray, int>& previous)
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
    m_mousePos = transform().map(event->pos());
    QGraphicsWidget::mouseMoveEvent(event);
}

void KItemListView::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
    event->setAccepted(true);
}

QList<KItemListWidget*> KItemListView::visibleItemListWidgets() const
{
    return m_visibleItems.values();
}

void KItemListView::slotItemsInserted(const KItemRangeList& itemRanges)
{
    markVisibleRolesSizesAsDirty();

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
        if (m_model->count() == count && maximumOffset() > size().height()) {
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
            update();
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
    markVisibleRolesSizesAsDirty();

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
            update();
        }
    }

    if (m_controller) {
        m_controller->selectionManager()->itemsRemoved(itemRanges);
    }

    if (hasMultipleRanges) {
        endTransaction();
    }
}

void KItemListView::slotItemsChanged(const KItemRangeList& itemRanges,
                                     const QSet<QByteArray>& roles)
{
    foreach (const KItemRange& itemRange, itemRanges) {
        const int index = itemRange.index;
        const int count = itemRange.count;

        m_sizeHintResolver->itemsChanged(index, count, roles);

        const int lastIndex = index + count - 1;
        for (int i = index; i <= lastIndex; ++i) {
            KItemListWidget* widget = m_visibleItems.value(i);
            if (widget) {
                widget->setData(m_model->data(i), roles);
            }
        }
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

    const QRectF viewGeometry = geometry();
    const QRectF currentBoundingRect = itemBoundingRect(current);

    if (!viewGeometry.contains(currentBoundingRect)) {
        // Make sure that the new current item is fully visible in the view.
        qreal newOffset = offset();
        if (currentBoundingRect.top() < viewGeometry.top()) {
            Q_ASSERT(scrollOrientation() == Qt::Vertical);
            newOffset += currentBoundingRect.top() - viewGeometry.top();
        }
        else if ((currentBoundingRect.bottom() > viewGeometry.bottom())) {
            Q_ASSERT(scrollOrientation() == Qt::Vertical);
            newOffset += currentBoundingRect.bottom() - viewGeometry.bottom();
        }
        else if (currentBoundingRect.left() < viewGeometry.left()) {
            if (scrollOrientation() == Qt::Horizontal) {
                newOffset += currentBoundingRect.left() - viewGeometry.left();
            }
        }
        else if ((currentBoundingRect.right() > viewGeometry.right())) {
            if (scrollOrientation() == Qt::Horizontal) {
                newOffset += currentBoundingRect.right() - viewGeometry.right();
            }
        }

        if (newOffset != offset()) {
            emit scrollTo(newOffset);
        }
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
        KItemListGroupHeader* header = m_visibleGroups.value(itemListWidget);
        if (header) {
            m_groupHeaderCreator->recycle(header);
            m_visibleGroups.remove(itemListWidget);
        }
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

void KItemListView::slotRubberBandStartPosChanged()
{
    update();
}

void KItemListView::slotRubberBandEndPosChanged()
{
    // The autoscrolling is triggered asynchronously otherwise it
    // might be possible to have an endless recursion: The autoscrolling
    // might adjust the position which might result in updating the
    // rubberband end-position.
    QTimer::singleShot(0, this, SLOT(triggerAutoScrolling()));
    update();
}

void KItemListView::slotRubberBandActivationChanged(bool active)
{
    if (active) {
        connect(m_rubberBand, SIGNAL(startPositionChanged(QPointF,QPointF)), this, SLOT(slotRubberBandStartPosChanged()));
        connect(m_rubberBand, SIGNAL(endPositionChanged(QPointF,QPointF)), this, SLOT(slotRubberBandEndPosChanged()));
    } else {
        disconnect(m_rubberBand, SIGNAL(startPositionChanged(QPointF,QPointF)), this, SLOT(slotRubberBandStartPosChanged()));
        disconnect(m_rubberBand, SIGNAL(endPositionChanged(QPointF,QPointF)), this, SLOT(slotRubberBandEndPosChanged()));
    }

    update();
}

void KItemListView::triggerAutoScrolling()
{
    int pos = 0;
    int visibleSize = 0;
    if (scrollOrientation() == Qt::Vertical) {
        pos = m_mousePos.y();
        visibleSize = size().height();
    } else {
        pos = m_mousePos.x();
        visibleSize = size().width();
    }

    const int inc = calculateAutoScrollingIncrement(pos, visibleSize);
    if (inc == 0) {
        // The mouse position is not above an autoscroll margin
        return;
    }

    if (m_rubberBand->isActive()) {
        // If a rubberband selection is ongoing the autoscrolling may only get triggered
        // if the direction of the rubberband is similar to the autoscroll direction.

        const qreal minDiff = 4; // Ignore any autoscrolling if the rubberband is very small
        const qreal diff = (scrollOrientation() == Qt::Vertical)
                           ? m_rubberBand->endPosition().y() - m_rubberBand->startPosition().y()
                           : m_rubberBand->endPosition().x() - m_rubberBand->startPosition().x();
        if (qAbs(diff) < minDiff || (inc < 0 && diff > 0) || (inc > 0 && diff < 0)) {
            // The rubberband direction is different from the scroll direction (e.g. the rubberband has
            // been moved up although the autoscroll direction might be down)
            return;
        }
    }

    emit scrollTo(offset() + inc);
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
    }

    m_model = model;
    m_layouter->setModel(model);
    m_grouped = !model->groupRole().isEmpty();

    if (m_model) {
        connect(m_model, SIGNAL(itemsChanged(KItemRangeList,QSet<QByteArray>)),
                this,    SLOT(slotItemsChanged(KItemRangeList,QSet<QByteArray>)));
        connect(m_model, SIGNAL(itemsInserted(KItemRangeList)),
                this,    SLOT(slotItemsInserted(KItemRangeList)));
        connect(m_model, SIGNAL(itemsRemoved(KItemRangeList)),
                this,    SLOT(slotItemsRemoved(KItemRangeList)));
    }

    onModelChanged(model, previous);
}

KItemListRubberBand* KItemListView::rubberBand() const
{
    return m_rubberBand;
}

void KItemListView::updateLayout()
{
    doLayout(Animation, 0, 0);
    update();
}

void KItemListView::doLayout(LayoutAnimationHint hint, int changedIndex, int changedCount)
{
    if (m_layoutTimer->isActive()) {
        kDebug() << "Stopping layout timer, synchronous layout requested";
        m_layoutTimer->stop();
    }

    if (m_model->count() < 0 || m_activeTransactions > 0) {
        return;
    }

    applyDynamicItemSize();

    const int firstVisibleIndex = m_layouter->firstVisibleIndex();
    const int lastVisibleIndex = m_layouter->lastVisibleIndex();
    if (firstVisibleIndex < 0) {
        emitOffsetChanges();
        return;
    }

    // Do a sanity check of the offset-property: When properties of the itemlist-view have been changed
    // it might be possible that the maximum offset got changed too. Assure that the full visible range
    // is still shown if the maximum offset got decreased.
    const qreal visibleOffsetRange = (scrollOrientation() == Qt::Horizontal) ? size().width() : size().height();
    const qreal maxOffsetToShowFullRange = maximumOffset() - visibleOffsetRange;
    if (offset() > maxOffsetToShowFullRange) {
        m_layouter->setOffset(qMax(qreal(0), maxOffsetToShowFullRange));
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
        }
    }

    // Assure that for each visible item a KItemListWidget is available. KItemListWidget
    // instances from invisible items are reused. If no reusable items are
    // found then new KItemListWidget instances get created.
    const bool animate = (hint == Animation);
    for (int i = firstVisibleIndex; i <= lastVisibleIndex; ++i) {
        bool applyNewPos = true;
        bool wasHidden = false;

        const QRectF itemBounds = m_layouter->itemBoundingRect(i);
        const QPointF newPos = itemBounds.topLeft();
        KItemListWidget* widget = m_visibleItems.value(i);
        if (!widget) {
            wasHidden = true;
            if (!reusableItems.isEmpty()) {
                // Reuse a KItemListWidget instance from an invisible item
                const int oldIndex = reusableItems.takeLast();
                widget = m_visibleItems.value(oldIndex);
                setWidgetIndex(widget, i);
            } else {
                // No reusable KItemListWidget instance is available, create a new one
                widget = createWidget(i);
            }
            widget->resize(itemBounds.size());

            if (animate && changedCount < 0) {
                // Items have been deleted, move the created item to the
                // imaginary old position.
                const QRectF itemBoundingRect = m_layouter->itemBoundingRect(i - changedCount);
                if (itemBoundingRect.isEmpty()) {
                    const QPointF invisibleOldPos = (scrollOrientation() == Qt::Vertical)
                                                    ? QPointF(0, size().height()) : QPointF(size().width(), 0);
                    widget->setPos(invisibleOldPos);
                } else {
                    widget->setPos(itemBoundingRect.topLeft());
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
            m_animation->start(widget, KItemListViewAnimation::ResizeAnimation, itemBounds.size());
        }
    }

    // Delete invisible KItemListWidget instances that have not been reused
    foreach (int index, reusableItems) {
        recycleWidget(m_visibleItems.value(index));
    }

    emitOffsetChanges();
}

void KItemListView::emitOffsetChanges()
{
    const qreal newOffset = m_layouter->offset();
    if (m_oldOffset != newOffset) {
        emit offsetChanged(newOffset, m_oldOffset);
        m_oldOffset = newOffset;
    }

    const qreal newMaximumOffset = m_layouter->maximumOffset();
    if (m_oldMaximumOffset != newMaximumOffset) {
        emit maximumOffsetChanged(newMaximumOffset, m_oldMaximumOffset);
        m_oldMaximumOffset = newMaximumOffset;
    }
}

KItemListWidget* KItemListView::createWidget(int index)
{
    KItemListWidget* widget = m_widgetCreator->create(this);
    updateWidgetProperties(widget, index);
    m_visibleItems.insert(index, widget);

    if (m_grouped) {
        if (m_layouter->isFirstGroupItem(index)) {
            KItemListGroupHeader* header = m_groupHeaderCreator->create(widget);
            header->setPos(0, -50);
            header->resize(50, 50);
            m_visibleGroups.insert(widget, header);
        }
    }

    initializeItemListWidget(widget);
    return widget;
}

void KItemListView::recycleWidget(KItemListWidget* widget)
{
    if (m_grouped) {
        KItemListGroupHeader* header = m_visibleGroups.value(widget);
        if (header) {
            m_groupHeaderCreator->recycle(header);
            m_visibleGroups.remove(widget);
        }
    }

    m_visibleItems.remove(widget->index());
    m_widgetCreator->recycle(widget);
}

void KItemListView::setWidgetIndex(KItemListWidget* widget, int index)
{
    if (m_grouped) {
        bool createHeader = m_layouter->isFirstGroupItem(index);
        KItemListGroupHeader* header = m_visibleGroups.value(widget);
        if (header) {
            if (createHeader) {
                createHeader = false;
            } else {
                m_groupHeaderCreator->recycle(header);
                m_visibleGroups.remove(widget);
            }
        }

        if (createHeader) {
            KItemListGroupHeader* header = m_groupHeaderCreator->create(widget);
            header->setPos(0, -50);
            header->resize(50, 50);
            m_visibleGroups.insert(widget, header);
        }
    }

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
                const QPointF pos = m_layouter->itemBoundingRect(i).topLeft();
                widget->setPos(pos);
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

bool KItemListView::markVisibleRolesSizesAsDirty()
{
    const bool dirty = m_itemSize.isEmpty();
    if (dirty) {
        m_visibleRolesSizes.clear();
        m_layouter->setItemSize(QSizeF());
    }
    return dirty;
}

void KItemListView::applyDynamicItemSize()
{
    if (!m_itemSize.isEmpty()) {
        return;
    }

    if (m_visibleRolesSizes.isEmpty()) {
        m_visibleRolesSizes = visibleRoleSizes();
        foreach (KItemListWidget* widget, visibleItemListWidgets()) {
            widget->setVisibleRolesSizes(m_visibleRolesSizes);
        }
    }

    if (m_layouter->itemSize().isEmpty()) {
        qreal requiredWidth = 0;
        qreal requiredHeight = 0;

        QHashIterator<QByteArray, QSizeF> it(m_visibleRolesSizes);
        while (it.hasNext()) {
            it.next();
            const QSizeF& visibleRoleSize = it.value();
            requiredWidth  += visibleRoleSize.width();
            requiredHeight += visibleRoleSize.height();
        }

        QSizeF dynamicItemSize = m_itemSize;
        if (dynamicItemSize.width() <= 0) {
            dynamicItemSize.setWidth(qMax(requiredWidth, size().width()));
        }
        if (dynamicItemSize.height() <= 0) {
            dynamicItemSize.setHeight(qMax(requiredHeight, size().height()));
        }

        m_layouter->setItemSize(dynamicItemSize);
    }
}

void KItemListView::updateWidgetProperties(KItemListWidget* widget, int index)
{
    widget->setVisibleRoles(m_visibleRoles);
    widget->setVisibleRolesSizes(m_visibleRolesSizes);
    widget->setStyleOption(m_styleOption);

    const KItemListSelectionManager* selectionManager = m_controller->selectionManager();
    widget->setCurrent(index == selectionManager->currentItem());

    if (selectionManager->hasSelection()) {
        const QSet<int> selectedItems = selectionManager->selectedItems();
        widget->setSelected(selectedItems.contains(index));
    } else {
        widget->setSelected(false);
    }

    widget->setHovered(false);

    widget->setIndex(index);
    widget->setData(m_model->data(index));
}

int KItemListView::calculateAutoScrollingIncrement(int pos, int size)
{
    int inc = 0;

    const int minSpeed = 4;
    const int maxSpeed = 768;
    const int speedLimiter = 48;
    const int autoScrollBorder = 64;

    if (pos < autoScrollBorder) {
        inc = -minSpeed + qAbs(pos - autoScrollBorder) * (pos - autoScrollBorder) / speedLimiter;
        if (inc < -maxSpeed) {
            inc = -maxSpeed;
        }
    } else if (pos > size - autoScrollBorder) {
        inc = minSpeed + qAbs(pos - size + autoScrollBorder) * (pos - size + autoScrollBorder) / speedLimiter;
        if (inc > maxSpeed) {
            inc = maxSpeed;
        }
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
