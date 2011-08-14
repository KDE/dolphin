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
    m_sliderMovedByUser(false),
    m_viewOffsetAnimation(0)
{
    Q_ASSERT(controller);
    controller->setParent(this);
    initialize();
}

KItemListContainer::KItemListContainer(QWidget* parent) :
    QAbstractScrollArea(parent),
    m_controller(0)
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

    const qreal currentOffset = view->offset();
    qreal offsetDiff = (view->scrollOrientation() == Qt::Vertical) ? dy : dx;

    const bool animRunning = (m_viewOffsetAnimation->state() == QAbstractAnimation::Running);
    if (animRunning) {
        // Stopping a running animation means skipping the range from the current offset
        // until the target offset. To prevent skipping of the range the difference
        // is added to the new target offset.
        m_viewOffsetAnimation->stop();
        const qreal targetOffset = m_viewOffsetAnimation->endValue().toReal();
        offsetDiff += (currentOffset - targetOffset);
    }

    const qreal newOffset = currentOffset - offsetDiff;

    if (m_sliderMovedByUser || animRunning) {
        m_viewOffsetAnimation->stop();
        m_viewOffsetAnimation->setStartValue(currentOffset);
        m_viewOffsetAnimation->setEndValue(newOffset);
        m_viewOffsetAnimation->setEasingCurve(animRunning ? QEasingCurve::OutQuad : QEasingCurve::InOutQuad);
        m_viewOffsetAnimation->start();
    } else {
        view->setOffset(newOffset);
    }
}

bool KItemListContainer::eventFilter(QObject* obj, QEvent* event)
{
    Q_ASSERT(obj == horizontalScrollBar() || obj == verticalScrollBar());

    // Check whether the scrollbar has been adjusted by a mouse-event
    // triggered by the user and remember this in m_sliderMovedByUser.
    // The smooth scrolling will only get active if m_sliderMovedByUser
    // is true (see scrollContentsBy()).
    const bool scrollVertical = (m_controller->view()->scrollOrientation() == Qt::Vertical);
    const bool checkEvent = ( scrollVertical && obj == verticalScrollBar()) ||
                            (!scrollVertical && obj == horizontalScrollBar());
    if (checkEvent) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            m_sliderMovedByUser = true;
           break;

        case QEvent::MouseButtonRelease:
            m_sliderMovedByUser = false;
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
    KItemListView* view = m_controller->view();
    if (!view || event->orientation() != view->scrollOrientation()) {
        return;
    }

    const int numDegrees = event->delta() / 8;
    const int numSteps = numDegrees / 15;

    const bool previous = m_sliderMovedByUser;
    m_sliderMovedByUser = true;
    if (view->scrollOrientation() == Qt::Vertical) {
        const int value = verticalScrollBar()->value();
        verticalScrollBar()->setValue(value - numSteps * view->size().height());
    } else {
        const int value = horizontalScrollBar()->value();
        horizontalScrollBar()->setValue(value - numSteps * view->size().width());
    }
    m_sliderMovedByUser = previous;

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
        disconnect(previous, SIGNAL(offsetChanged(int,int)), this, SLOT(updateScrollBars()));
        disconnect(previous, SIGNAL(maximumOffsetChanged(int,int)), this, SLOT(updateScrollBars()));
        m_viewOffsetAnimation->setTargetObject(0);
    }
    if (current) {
        scene->addItem(current);
        connect(current, SIGNAL(offsetChanged(int,int)), this, SLOT(updateScrollBars()));
        connect(current, SIGNAL(maximumOffsetChanged(int,int)), this, SLOT(updateScrollBars()));
        m_viewOffsetAnimation->setTargetObject(current);
    }
}

void KItemListContainer::updateScrollBars()
{
    const KItemListView* view = m_controller->view();
    if (!view) {
        return;
    }

    QScrollBar* scrollBar = 0;
    int singleStep = 0;
    int pageStep = 0;
    QScrollBar* otherScrollBar = 0;
    if (view->scrollOrientation() == Qt::Vertical) {
        scrollBar = verticalScrollBar();
        singleStep = view->itemSize().height();
        pageStep = view->size().height();
        otherScrollBar = horizontalScrollBar();
    } else {
        scrollBar = horizontalScrollBar();
        singleStep = view->itemSize().width();
        pageStep = view->size().width();
        otherScrollBar = verticalScrollBar();
    }

    const int value = view->offset();
    const int maximum = qMax(0, int(view->maximumOffset() - pageStep));
    if (m_viewOffsetAnimation->state() == QAbstractAnimation::Running) {
        if (maximum == scrollBar->maximum()) {
            // The value has been changed by the animation, no update
            // of the scrollbars is required as their target state will be
            // reached with the end of the animation.
            return;
        }

        // The maximum has been changed which indicates that the content
        // of the view has been changed. Stop the animation in any case and
        // update the scrollbars immediately.
        m_viewOffsetAnimation->stop();
    }

    scrollBar->setSingleStep(singleStep);
    scrollBar->setPageStep(pageStep);
    scrollBar->setMinimum(0);
    scrollBar->setMaximum(maximum);
    scrollBar->setValue(value);

    disconnect(view, SIGNAL(scrollTo(int)),
               otherScrollBar, SLOT(setValue(int)));
    connect(view, SIGNAL(scrollTo(int)),
            scrollBar, SLOT(setValue(int)));

    // Make sure that the other scroll bar is hidden
    otherScrollBar->setMaximum(0);
    otherScrollBar->setValue(0);
}

void KItemListContainer::updateGeometries()
{
    QRect rect = geometry();

    int widthDec = frameWidth() * 2;
    if (verticalScrollBar()->isVisible()) {
        widthDec += style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    }

    int heightDec = frameWidth() * 2;
    if (horizontalScrollBar()->isVisible()) {
        heightDec += style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    }

    rect.adjust(0, 0, -widthDec, -heightDec);

    m_controller->view()->setGeometry(QRect(0, 0, rect.width(), rect.height()));

    static_cast<KItemListContainerViewport*>(viewport())->scene()->setSceneRect(0, 0, rect.width(), rect.height());
    static_cast<KItemListContainerViewport*>(viewport())->viewport()->setGeometry(QRect(0, 0, rect.width(), rect.height()));

    updateScrollBars();
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

    m_viewOffsetAnimation = new QPropertyAnimation(this, "offset");
    m_viewOffsetAnimation->setDuration(500);

    horizontalScrollBar()->installEventFilter(this);
    verticalScrollBar()->installEventFilter(this);
}

#include "kitemlistcontainer.moc"
