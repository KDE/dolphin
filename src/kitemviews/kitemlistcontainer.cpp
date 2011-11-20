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
#include "kitemlistsmoothscroller_p.h"
#include "kitemlistview.h"
#include "kitemmodelbase.h"

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QStyle>

#include <KDebug>

/**
 * Replaces the default viewport of KItemListContainer by a
 * non-scrollable viewport. The scrolling is done in an optimized
 * way by KItemListView internally.
 */
class KItemListContainerViewport : public QGraphicsView
{
public:
    KItemListContainerViewport(QGraphicsScene* scene, QWidget* parent);
protected:
    virtual void wheelEvent(QWheelEvent* event);
};

KItemListContainerViewport::KItemListContainerViewport(QGraphicsScene* scene, QWidget* parent) :
    QGraphicsView(scene, parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewportMargins(0, 0, 0, 0);
    setFrameShape(QFrame::NoFrame);
}

void KItemListContainerViewport::wheelEvent(QWheelEvent* event)
{
    // Assure that the wheel-event gets forwarded to the parent
    // and not handled at all by QGraphicsView.
    event->ignore();
}



KItemListContainer::KItemListContainer(KItemListController* controller, QWidget* parent) :
    QAbstractScrollArea(parent),
    m_controller(controller),
    m_horizontalSmoothScroller(0),
    m_verticalSmoothScroller(0)
{
    Q_ASSERT(controller);
    controller->setParent(this);
    initialize();
}

KItemListContainer::KItemListContainer(QWidget* parent) :
    QAbstractScrollArea(parent),
    m_controller(0),
    m_horizontalSmoothScroller(0),
    m_verticalSmoothScroller(0)
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
    m_horizontalSmoothScroller->scrollContentsBy(dx);
    m_verticalSmoothScroller->scrollContentsBy(dy);
}

void KItemListContainer::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        event->ignore();
        return;
    }

    KItemListView* view = m_controller->view();
    if (!view) {
        event->ignore();
        return;
    }

    const bool scrollHorizontally = (event->orientation() == Qt::Horizontal) ||
                                    (event->orientation() == Qt::Vertical && !verticalScrollBar()->isVisible());
    KItemListSmoothScroller* smoothScroller = scrollHorizontally ?
                                              m_horizontalSmoothScroller : m_verticalSmoothScroller;

    const int numDegrees = event->delta() / 8;
    const int numSteps = numDegrees / 15;

    const QScrollBar* scrollBar = smoothScroller->scrollBar();
    smoothScroller->scrollTo(scrollBar->value() - numSteps * scrollBar->pageStep() / 4);

    event->accept();
}

void KItemListContainer::slotScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous)
{
    Q_UNUSED(previous);
    updateSmoothScrollers(current);
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
        disconnect(current, SIGNAL(scrollOrientationChanged(Qt::Orientation,Qt::Orientation)), this, SLOT(slotScrollOrientationChanged(Qt::Orientation,Qt::Orientation)));
        disconnect(previous, SIGNAL(scrollOffsetChanged(qreal,qreal)),        this, SLOT(updateScrollOffsetScrollBar()));
        disconnect(previous, SIGNAL(maximumScrollOffsetChanged(qreal,qreal)), this, SLOT(updateScrollOffsetScrollBar()));
        disconnect(previous, SIGNAL(itemOffsetChanged(qreal,qreal)),          this, SLOT(updateItemOffsetScrollBar()));
        disconnect(previous, SIGNAL(maximumItemOffsetChanged(qreal,qreal)),   this, SLOT(updateItemOffsetScrollBar()));
        disconnect(previous, SIGNAL(scrollTo(qreal)),                         this, SLOT(scrollTo(qreal)));
        m_horizontalSmoothScroller->setTargetObject(0);
        m_verticalSmoothScroller->setTargetObject(0);
    }
    if (current) {
        scene->addItem(current);
        connect(current, SIGNAL(scrollOrientationChanged(Qt::Orientation,Qt::Orientation)), this, SLOT(slotScrollOrientationChanged(Qt::Orientation,Qt::Orientation)));
        connect(current, SIGNAL(scrollOffsetChanged(qreal,qreal)),        this, SLOT(updateScrollOffsetScrollBar()));
        connect(current, SIGNAL(maximumScrollOffsetChanged(qreal,qreal)), this, SLOT(updateScrollOffsetScrollBar()));
        connect(current, SIGNAL(itemOffsetChanged(qreal,qreal)),          this, SLOT(updateItemOffsetScrollBar()));
        connect(current, SIGNAL(maximumItemOffsetChanged(qreal,qreal)),   this, SLOT(updateItemOffsetScrollBar()));
        connect(current, SIGNAL(scrollTo(qreal)),                         this, SLOT(scrollTo(qreal)));
        m_horizontalSmoothScroller->setTargetObject(current);
        m_verticalSmoothScroller->setTargetObject(current);
        updateSmoothScrollers(current->scrollOrientation());
    }
}

void KItemListContainer::scrollTo(qreal offset)
{
    const KItemListView* view = m_controller->view();
    if (view) {
        if (view->scrollOrientation() == Qt::Vertical) {
            m_verticalSmoothScroller->scrollTo(offset);
        } else {
            m_horizontalSmoothScroller->scrollTo(offset);
        }
    }
}

void KItemListContainer::updateScrollOffsetScrollBar()
{
    const KItemListView* view = m_controller->view();
    if (!view) {
        return;
    }

    KItemListSmoothScroller* smoothScroller = 0;
    QScrollBar* scrollOffsetScrollBar = 0;
    int singleStep = 0;
    int pageStep = 0;
    if (view->scrollOrientation() == Qt::Vertical) {
        smoothScroller = m_verticalSmoothScroller;
        scrollOffsetScrollBar = verticalScrollBar();
        singleStep = view->itemSize().height();
        pageStep = view->size().height();
    } else {
        smoothScroller = m_horizontalSmoothScroller;
        scrollOffsetScrollBar = horizontalScrollBar();
        singleStep = view->itemSize().width();
        pageStep = view->size().width();
    }

    const int value = view->scrollOffset();
    const int maximum = qMax(0, int(view->maximumScrollOffset() - pageStep));
    if (smoothScroller->requestScrollBarUpdate(maximum)) {
        scrollOffsetScrollBar->setSingleStep(singleStep);
        scrollOffsetScrollBar->setPageStep(pageStep);
        scrollOffsetScrollBar->setMinimum(0);
        scrollOffsetScrollBar->setMaximum(maximum);
        scrollOffsetScrollBar->setValue(value);
    }
}

void KItemListContainer::updateItemOffsetScrollBar()
{
    const KItemListView* view = m_controller->view();
    if (!view) {
        return;
    }

    KItemListSmoothScroller* smoothScroller = 0;
    QScrollBar* itemOffsetScrollBar = 0;
    int singleStep = 0;
    int pageStep = 0;
    if (view->scrollOrientation() == Qt::Vertical) {
        smoothScroller = m_horizontalSmoothScroller;
        itemOffsetScrollBar = horizontalScrollBar();
        singleStep = view->size().width() / 10;
        pageStep = view->size().width();
    } else {
        smoothScroller = m_verticalSmoothScroller;
        itemOffsetScrollBar = verticalScrollBar();
        singleStep = view->size().height() / 10;
        pageStep = view->size().height();
    }

    const int value = view->itemOffset();
    const int maximum = qMax(0, int(view->maximumItemOffset()) - pageStep);
    if (smoothScroller->requestScrollBarUpdate(maximum)) {
        itemOffsetScrollBar->setSingleStep(singleStep);
        itemOffsetScrollBar->setPageStep(pageStep);
        itemOffsetScrollBar->setMinimum(0);
        itemOffsetScrollBar->setMaximum(maximum);
        itemOffsetScrollBar->setValue(value);
    }
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

    const QRectF newGeometry(0, 0, rect.width(), rect.height());
    if (m_controller->view()->geometry() != newGeometry) {
        m_controller->view()->setGeometry(newGeometry);

        static_cast<KItemListContainerViewport*>(viewport())->scene()->setSceneRect(0, 0, rect.width(), rect.height());
        static_cast<KItemListContainerViewport*>(viewport())->viewport()->setGeometry(QRect(0, 0, rect.width(), rect.height()));

        updateScrollOffsetScrollBar();
        updateItemOffsetScrollBar();
    }
}

void KItemListContainer::updateSmoothScrollers(Qt::Orientation orientation)
{
    if (orientation == Qt::Vertical) {
        m_verticalSmoothScroller->setPropertyName("scrollOffset");
        m_horizontalSmoothScroller->setPropertyName("itemOffset");
    } else {
        m_horizontalSmoothScroller->setPropertyName("scrollOffset");
        m_verticalSmoothScroller->setPropertyName("itemOffset");
    }
}

void KItemListContainer::initialize()
{
    if (m_controller) {
        if (m_controller->model()) {
            slotModelChanged(m_controller->model(), 0);
        }
        if (m_controller->view()) {
            slotViewChanged(m_controller->view(), 0);
        }
    } else {
        m_controller = new KItemListController(this);
    }

    connect(m_controller, SIGNAL(modelChanged(KItemModelBase*,KItemModelBase*)),
            this, SLOT(slotModelChanged(KItemModelBase*,KItemModelBase*)));
    connect(m_controller, SIGNAL(viewChanged(KItemListView*,KItemListView*)),
            this, SLOT(slotViewChanged(KItemListView*,KItemListView*)));

    QGraphicsView* graphicsView = new KItemListContainerViewport(new QGraphicsScene(this), this);
    setViewport(graphicsView);

    m_horizontalSmoothScroller = new KItemListSmoothScroller(horizontalScrollBar(), this);
    m_verticalSmoothScroller = new KItemListSmoothScroller(verticalScrollBar(), this);
}

#include "kitemlistcontainer.moc"
