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

#include "kitemlistcontainer.h"

#include "kitemlistcontroller.h"
#include "kitemlistview.h"
#include "kitemmodelbase.h"

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QStyle>

#include <KDebug>

class KItemListContainerViewport : public QGraphicsView
{
public:
    KItemListContainerViewport(QGraphicsScene* scene, QWidget* parent)
        : QGraphicsView(scene, parent)
    {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setViewportMargins(0, 0, 0, 0);
        setFrameShape(QFrame::NoFrame);
    }

    void scrollContentsBy(int dx, int dy)
    {
        Q_UNUSED(dx);
        Q_UNUSED(dy);
        // Do nothing. This prevents that e.g. the wheel-event
        // results in a moving of the scene items.
    }
};

KItemListContainer::KItemListContainer(KItemListController* controller, QWidget* parent) :
    QAbstractScrollArea(parent),
    m_controller(controller),
    m_scrollBarPressed(false),
    m_smoothScrolling(false),
    m_smoothScrollingAnimation(0)
{
    Q_ASSERT(controller);
    controller->setParent(this);
    initialize();
}

KItemListContainer::KItemListContainer(QWidget* parent) :
    QAbstractScrollArea(parent),
    m_controller(0),
    m_scrollBarPressed(false),
    m_smoothScrolling(false),
    m_smoothScrollingAnimation(0)
{
    initialize();
}

KItemListContainer::~KItemListContainer()
{
}

KItemListController* KItemListContainer::controller() const
{
    return m_controller;
}

void KItemListContainer::keyPressEvent(QKeyEvent* event)
{
    // TODO: We should find a better way to handle the key press events in the view.
    // The reasons why we need this hack are:
    // 1. Without reimplementing keyPressEvent() here, the event would not reach the QGraphicsView.
    // 2. By default, the KItemListView does not have the keyboard focus in the QGraphicsScene, so
    //    simply sending the event to the QGraphicsView which is the KItemListContainer's viewport
    //    does not work.
    KItemListView* view = m_controller->view();
    if (view) {
        QApplication::sendEvent(view, event);
    }
}

void KItemListContainer::showEvent(QShowEvent* event)
{
    QAbstractScrollArea::showEvent(event);
    updateGeometries();
}

void KItemListContainer::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateGeometries();
}

void KItemListContainer::scrollContentsBy(int dx, int dy)
{
    KItemListView* view = m_controller->view();
    if (!view) {
        return;
    }

    const QScrollBar* scrollBar = (view->scrollOrientation() == Qt::Vertical)
                                  ? verticalScrollBar() : horizontalScrollBar();
    const qreal currentOffset = view->scrollOffset();
    if (static_cast<int>(currentOffset) == scrollBar->value()) {
        // The current offset is already synchronous to the scrollbar
        return;
    }

    qreal offsetDiff = (view->scrollOrientation() == Qt::Vertical) ? dy : dx;

    const bool animRunning = (m_smoothScrollingAnimation->state() == QAbstractAnimation::Running);
    if (animRunning) {
        // Stopping a running animation means skipping the range from the current offset
        // until the target offset. To prevent skipping of the range the difference
        // is added to the new target offset.
        const qreal oldEndOffset = m_smoothScrollingAnimation->endValue().toReal();
        offsetDiff += (currentOffset - oldEndOffset);
    }

    const qreal endOffset = currentOffset - offsetDiff;

    if (m_smoothScrolling || animRunning) {
        qreal startOffset = currentOffset;
        if (animRunning) {
            // If the animation was running and has been interrupted by assigning a new end-offset
            // one frame must be added to the start-offset to keep the animation smooth. This also
            // assures that animation proceeds even in cases where new end-offset are triggered
            // within a very short timeslots.
            startOffset += (endOffset - currentOffset) * 1000 / (m_smoothScrollingAnimation->duration() * 60);
        }

        m_smoothScrollingAnimation->stop();
        m_smoothScrollingAnimation->setStartValue(startOffset);
        m_smoothScrollingAnimation->setEndValue(endOffset);
        m_smoothScrollingAnimation->setEasingCurve(animRunning ? QEasingCurve::OutQuad : QEasingCurve::InOutQuad);
        m_smoothScrollingAnimation->start();
        view->setScrollOffset(startOffset);
    } else {
        view->setScrollOffset(endOffset);
    }
}

bool KItemListContainer::eventFilter(QObject* obj, QEvent* event)
{
    Q_ASSERT(obj == horizontalScrollBar() || obj == verticalScrollBar());

    // Check whether the scrollbar has been adjusted by a mouse-event
    // triggered by the user and remember this in m_scrollBarPressed.
    // The smooth scrolling will only get active if m_scrollBarPressed
    // is true (see scrollContentsBy()).
    const bool scrollVertical = (m_controller->view()->scrollOrientation() == Qt::Vertical);
    const bool checkEvent = ( scrollVertical && obj == verticalScrollBar()) ||
                            (!scrollVertical && obj == horizontalScrollBar());
    if (checkEvent) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            m_scrollBarPressed = true;
            m_smoothScrolling = true;
            break;

        case QEvent::MouseButtonRelease:
            m_scrollBarPressed = false;
            m_smoothScrolling = false;
            break;

        case QEvent::Wheel:
            wheelEvent(static_cast<QWheelEvent*>(event));
            break;

        default:
            break;
        }
    }

    return QAbstractScrollArea::eventFilter(obj, event);
}

void KItemListContainer::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        event->ignore();
        return;
    }

    KItemListView* view = m_controller->view();
    
    if (!view || event->orientation() != view->scrollOrientation()) {
        return;
    }

    const int numDegrees = event->delta() / 8;
    const int numSteps = numDegrees / 15;

    const bool previous = m_smoothScrolling;
    m_smoothScrolling = true;
    if (view->scrollOrientation() == Qt::Vertical) {
        const int value = verticalScrollBar()->value();
        verticalScrollBar()->setValue(value - numSteps * view->size().height());
    } else {
        const int value = horizontalScrollBar()->value();
        horizontalScrollBar()->setValue(value - numSteps * view->size().width());
    }
    m_smoothScrolling = previous;

    event->accept();
}

void KItemListContainer::slotModelChanged(KItemModelBase* current, KItemModelBase* previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListContainer::slotViewChanged(KItemListView* current, KItemListView* previous)
{
    QGraphicsScene* scene = static_cast<QGraphicsView*>(viewport())->scene();
    if (previous) {
        scene->removeItem(previous);
        disconnect(previous, SIGNAL(scrollOffsetChanged(qreal,qreal)),        this, SLOT(updateScrollOffsetScrollBar()));
        disconnect(previous, SIGNAL(maximumScrollOffsetChanged(qreal,qreal)), this, SLOT(updateScrollOffsetScrollBar()));
        disconnect(previous, SIGNAL(itemOffsetChanged(qreal,qreal)),          this, SLOT(updateItemOffsetScrollBar()));
        disconnect(previous, SIGNAL(maximumItemOffsetChanged(qreal,qreal)),   this, SLOT(updateItemOffsetScrollBar()));
        disconnect(previous, SIGNAL(scrollTo(qreal)),                         this, SLOT(scrollTo(qreal)));
        m_smoothScrollingAnimation->setTargetObject(0);
    }
    if (current) {
        scene->addItem(current);
        connect(current, SIGNAL(scrollOffsetChanged(qreal,qreal)),        this, SLOT(updateScrollOffsetScrollBar()));
        connect(current, SIGNAL(maximumScrollOffsetChanged(qreal,qreal)), this, SLOT(updateScrollOffsetScrollBar()));
        connect(current, SIGNAL(itemOffsetChanged(qreal,qreal)),          this, SLOT(updateItemOffsetScrollBar()));
        connect(current, SIGNAL(maximumItemOffsetChanged(qreal,qreal)),   this, SLOT(updateItemOffsetScrollBar()));
        connect(current, SIGNAL(scrollTo(qreal)),                         this, SLOT(scrollTo(qreal)));
        m_smoothScrollingAnimation->setTargetObject(current);
    }
}

void KItemListContainer::slotAnimationStateChanged(QAbstractAnimation::State newState,
                                                   QAbstractAnimation::State oldState)
{
    Q_UNUSED(oldState);
    if (newState == QAbstractAnimation::Stopped && m_smoothScrolling && !m_scrollBarPressed) {
        m_smoothScrolling = false;
    }
}


void KItemListContainer::scrollTo(qreal offset)
{
    const KItemListView* view = m_controller->view();
    if (!view) {
        return;
    }

    m_smoothScrolling = true;
    QScrollBar* scrollBar = (view->scrollOrientation() == Qt::Vertical)
                            ? verticalScrollBar() : horizontalScrollBar();
    scrollBar->setValue(offset);
}

void KItemListContainer::updateScrollOffsetScrollBar()
{
    const KItemListView* view = m_controller->view();
    if (!view) {
        return;
    }

    QScrollBar* scrollOffsetScrollBar = 0;
    int singleStep = 0;
    int pageStep = 0;
    if (view->scrollOrientation() == Qt::Vertical) {
        scrollOffsetScrollBar = verticalScrollBar();
        singleStep = view->itemSize().height();
        pageStep = view->size().height();
    } else {
        scrollOffsetScrollBar = horizontalScrollBar();
        singleStep = view->itemSize().width();
        pageStep = view->size().width();
    }

    const int value = view->scrollOffset();
    const int maximum = qMax(0, int(view->maximumScrollOffset() - pageStep));
    if (m_smoothScrollingAnimation->state() == QAbstractAnimation::Running) {
        if (maximum == scrollOffsetScrollBar->maximum()) {
            // The value has been changed by the animation, no update
            // of the scrollbars is required as their target state will be
            // reached with the end of the animation.
            return;
        }

        // The maximum has been changed which indicates that the content
        // of the view has been changed. Stop the animation in any case and
        // update the scrollbars immediately.
        m_smoothScrollingAnimation->stop();
    }

    scrollOffsetScrollBar->setSingleStep(singleStep);
    scrollOffsetScrollBar->setPageStep(pageStep);
    scrollOffsetScrollBar->setMinimum(0);
    scrollOffsetScrollBar->setMaximum(maximum);
    scrollOffsetScrollBar->setValue(value);
}

void KItemListContainer::updateItemOffsetScrollBar()
{
    const KItemListView* view = m_controller->view();
    if (!view) {
        return;
    }

    QScrollBar* itemOffsetScrollBar = 0;
    int singleStep = 0;
    int pageStep = 0;
    if (view->scrollOrientation() == Qt::Vertical) {
        itemOffsetScrollBar = horizontalScrollBar();
        singleStep = view->itemSize().width() / 10;
        pageStep = view->size().width();
    } else {
        itemOffsetScrollBar = verticalScrollBar();
        singleStep = view->itemSize().height() / 10;
        pageStep = view->size().height();
    }

    const int value = view->itemOffset();
    const int maximum = qMax(0, int(view->maximumItemOffset() - pageStep));

    itemOffsetScrollBar->setSingleStep(singleStep);
    itemOffsetScrollBar->setPageStep(pageStep);
    itemOffsetScrollBar->setMinimum(0);
    itemOffsetScrollBar->setMaximum(maximum);
    itemOffsetScrollBar->setValue(value);
}

void KItemListContainer::updateGeometries()
{
    QRect rect = geometry();

    const int widthDec = verticalScrollBar()->isVisible()
                         ? frameWidth() + style()->pixelMetric(QStyle::PM_ScrollBarExtent)
                         : frameWidth() * 2;

    const int heightDec = horizontalScrollBar()->isVisible()
                          ? frameWidth() + style()->pixelMetric(QStyle::PM_ScrollBarExtent)
                          : frameWidth() * 2;

    rect.adjust(0, 0, -widthDec, -heightDec);

    m_controller->view()->setGeometry(QRect(0, 0, rect.width(), rect.height()));

    static_cast<KItemListContainerViewport*>(viewport())->scene()->setSceneRect(0, 0, rect.width(), rect.height());
    static_cast<KItemListContainerViewport*>(viewport())->viewport()->setGeometry(QRect(0, 0, rect.width(), rect.height()));

    updateScrollOffsetScrollBar();
    updateItemOffsetScrollBar();
}

void KItemListContainer::initialize()
{
    if (!m_controller) {
        m_controller = new KItemListController(this);
    }

    connect(m_controller, SIGNAL(modelChanged(KItemModelBase*,KItemModelBase*)),
            this, SLOT(slotModelChanged(KItemModelBase*,KItemModelBase*)));
    connect(m_controller, SIGNAL(viewChanged(KItemListView*,KItemListView*)),
            this, SLOT(slotViewChanged(KItemListView*,KItemListView*)));

    QGraphicsView* graphicsView = new KItemListContainerViewport(new QGraphicsScene(this), this);
    setViewport(graphicsView);

    m_smoothScrollingAnimation = new QPropertyAnimation(this, "scrollOffset");
    m_smoothScrollingAnimation->setDuration(300);
    connect(m_smoothScrollingAnimation, SIGNAL(stateChanged(QAbstractAnimation::State,QAbstractAnimation::State)),
            this, SLOT(slotAnimationStateChanged(QAbstractAnimation::State,QAbstractAnimation::State)));

    horizontalScrollBar()->installEventFilter(this);
    verticalScrollBar()->installEventFilter(this);
}

#include "kitemlistcontainer.moc"
