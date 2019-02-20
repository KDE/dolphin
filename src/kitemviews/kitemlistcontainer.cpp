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
#include "private/kitemlistsmoothscroller.h"

#include <QApplication>
#include <QFontMetrics>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QScrollBar>
#include <QStyleOption>

/**
 * Replaces the default viewport of KItemListContainer by a
 * non-scrollable viewport. The scrolling is done in an optimized
 * way by KItemListView internally.
 */
class KItemListContainerViewport : public QGraphicsView
{
    Q_OBJECT

public:
    KItemListContainerViewport(QGraphicsScene* scene, QWidget* parent);
protected:
    void wheelEvent(QWheelEvent* event) override;
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
    m_horizontalSmoothScroller(nullptr),
    m_verticalSmoothScroller(nullptr)
{
    Q_ASSERT(controller);
    controller->setParent(this);

    QGraphicsView* graphicsView = new KItemListContainerViewport(new QGraphicsScene(this), this);
    setViewport(graphicsView);

    m_horizontalSmoothScroller = new KItemListSmoothScroller(horizontalScrollBar(), this);
    m_verticalSmoothScroller = new KItemListSmoothScroller(verticalScrollBar(), this);

    if (controller->model()) {
        slotModelChanged(controller->model(), nullptr);
    }
    if (controller->view()) {
        slotViewChanged(controller->view(), nullptr);
    }

    connect(controller, &KItemListController::modelChanged,
            this, &KItemListContainer::slotModelChanged);
    connect(controller, &KItemListController::viewChanged,
            this, &KItemListContainer::slotViewChanged);
}

KItemListContainer::~KItemListContainer()
{
    // Don't rely on the QObject-order to delete the controller, otherwise
    // the QGraphicsScene might get deleted before the view.
    delete m_controller;
    m_controller = nullptr;
}

KItemListController* KItemListContainer::controller() const
{
    return m_controller;
}

void KItemListContainer::setEnabledFrame(bool enable)
{
    QGraphicsView* graphicsView = qobject_cast<QGraphicsView*>(viewport());
    if (enable) {
        setFrameShape(QFrame::StyledPanel);
        graphicsView->setPalette(palette());
        graphicsView->viewport()->setAutoFillBackground(true);
    } else {
        setFrameShape(QFrame::NoFrame);
        // Make the background of the container transparent and apply the window-text color
        // to the text color, so that enough contrast is given for all color
        // schemes
        QPalette p = graphicsView->palette();
        p.setColor(QPalette::Active,   QPalette::Text, p.color(QPalette::Active,   QPalette::WindowText));
        p.setColor(QPalette::Inactive, QPalette::Text, p.color(QPalette::Inactive, QPalette::WindowText));
        p.setColor(QPalette::Disabled, QPalette::Text, p.color(QPalette::Disabled, QPalette::WindowText));
        graphicsView->setPalette(p);
        graphicsView->viewport()->setAutoFillBackground(false);
    }
}

bool KItemListContainer::enabledFrame() const
{
    const QGraphicsView* graphicsView = qobject_cast<QGraphicsView*>(viewport());
    return graphicsView->autoFillBackground();
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

    smoothScroller->handleWheelEvent(event);
}

void KItemListContainer::slotScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous)
{
    Q_UNUSED(previous)
    updateSmoothScrollers(current);
}

void KItemListContainer::slotModelChanged(KItemModelBase* current, KItemModelBase* previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KItemListContainer::slotViewChanged(KItemListView* current, KItemListView* previous)
{
    QGraphicsScene* scene = static_cast<QGraphicsView*>(viewport())->scene();
    if (previous) {
        scene->removeItem(previous);
        disconnect(previous, &KItemListView::scrollOrientationChanged,
                   this, &KItemListContainer::slotScrollOrientationChanged);
        disconnect(previous, &KItemListView::scrollOffsetChanged,
                   this, &KItemListContainer::updateScrollOffsetScrollBar);
        disconnect(previous, &KItemListView::maximumScrollOffsetChanged,
                   this, &KItemListContainer::updateScrollOffsetScrollBar);
        disconnect(previous, &KItemListView::itemOffsetChanged,
                   this, &KItemListContainer::updateItemOffsetScrollBar);
        disconnect(previous, &KItemListView::maximumItemOffsetChanged,
                   this, &KItemListContainer::updateItemOffsetScrollBar);
        disconnect(previous, &KItemListView::scrollTo, this, &KItemListContainer::scrollTo);
        m_horizontalSmoothScroller->setTargetObject(nullptr);
        m_verticalSmoothScroller->setTargetObject(nullptr);
    }
    if (current) {
        scene->addItem(current);
        connect(current, &KItemListView::scrollOrientationChanged,
                this, &KItemListContainer::slotScrollOrientationChanged);
        connect(current, &KItemListView::scrollOffsetChanged,
                this, &KItemListContainer::updateScrollOffsetScrollBar);
        connect(current, &KItemListView::maximumScrollOffsetChanged,
                this, &KItemListContainer::updateScrollOffsetScrollBar);
        connect(current, &KItemListView::itemOffsetChanged,
                this, &KItemListContainer::updateItemOffsetScrollBar);
        connect(current, &KItemListView::maximumItemOffsetChanged,
                this, &KItemListContainer::updateItemOffsetScrollBar);
        connect(current, &KItemListView::scrollTo, this, &KItemListContainer::scrollTo);
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

    KItemListSmoothScroller* smoothScroller = nullptr;
    QScrollBar* scrollOffsetScrollBar = nullptr;
    int singleStep = 0;
    int pageStep = 0;
    int maximum = 0;
    if (view->scrollOrientation() == Qt::Vertical) {
        smoothScroller = m_verticalSmoothScroller;
        scrollOffsetScrollBar = verticalScrollBar();

        // Don't scroll super fast when using a wheel mouse:
        // We want to consider one "line" to be the text label which has a
        // roughly fixed height rather than using the height of the icon which
        // may be very tall
        const QFontMetrics metrics(font());
        singleStep = metrics.height() * QApplication::wheelScrollLines();

        // We cannot use view->size().height() because this height might
        // include the header widget, which is not part of the scrolled area.
        pageStep = view->verticalPageStep();

        // However, the total height of the view must be considered for the
        // maximum value of the scroll bar. Note that the view's scrollOffset()
        // refers to the offset of the top part of the view, which might be
        // hidden behind the header.
        maximum = qMax(0, int(view->maximumScrollOffset() - view->size().height()));
    } else {
        smoothScroller = m_horizontalSmoothScroller;
        scrollOffsetScrollBar = horizontalScrollBar();
        singleStep = view->itemSize().width();
        pageStep = view->size().width();
        maximum = qMax(0, int(view->maximumScrollOffset() - view->size().width()));
    }

    const int value = view->scrollOffset();
    if (smoothScroller->requestScrollBarUpdate(maximum)) {
        const bool updatePolicy = (scrollOffsetScrollBar->maximum() > 0 && maximum == 0)
                                  || horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOn;

        scrollOffsetScrollBar->setSingleStep(singleStep);
        scrollOffsetScrollBar->setPageStep(pageStep);
        scrollOffsetScrollBar->setMinimum(0);
        scrollOffsetScrollBar->setMaximum(maximum);
        scrollOffsetScrollBar->setValue(value);

        if (updatePolicy) {
            // Prevent a potential endless layout loop (see bug #293318).
            updateScrollOffsetScrollBarPolicy();
        }
    }
}

void KItemListContainer::updateItemOffsetScrollBar()
{
    const KItemListView* view = m_controller->view();
    if (!view) {
        return;
    }

    KItemListSmoothScroller* smoothScroller = nullptr;
    QScrollBar* itemOffsetScrollBar = nullptr;
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

    int extra = frameWidth() * 2;
    QStyleOption option;
    option.initFrom(this);
    int scrollbarSpacing = 0;
    if (style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, &option, this)) {
        scrollbarSpacing = style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarSpacing, &option, this);
    }

    const int widthDec = verticalScrollBar()->isVisible()
                         ? extra + scrollbarSpacing + style()->pixelMetric(QStyle::PM_ScrollBarExtent, &option, this)
                         : extra;

    const int heightDec = horizontalScrollBar()->isVisible()
                          ? extra + scrollbarSpacing + style()->pixelMetric(QStyle::PM_ScrollBarExtent, &option, this)
                          : extra;

    const QRectF newGeometry(0, 0, rect.width() - widthDec,
                             rect.height() - heightDec);
    if (m_controller->view()->geometry() != newGeometry) {
        m_controller->view()->setGeometry(newGeometry);

        // Get the real geometry of the view again since the scrollbars
        // visibilities and the view geometry may have changed in re-layout.
        static_cast<KItemListContainerViewport*>(viewport())->scene()->setSceneRect(m_controller->view()->geometry());
        static_cast<KItemListContainerViewport*>(viewport())->viewport()->setGeometry(m_controller->view()->geometry().toRect());

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

void KItemListContainer::updateScrollOffsetScrollBarPolicy()
{
    const KItemListView* view = m_controller->view();
    Q_ASSERT(view);
    const bool vertical = (view->scrollOrientation() == Qt::Vertical);

    QStyleOption option;
    option.initFrom(this);
    const int scrollBarInc = style()->pixelMetric(QStyle::PM_ScrollBarExtent, &option, this);

    QSizeF newViewSize = m_controller->view()->size();
    if (vertical) {
        newViewSize.rwidth() += scrollBarInc;
    } else {
        newViewSize.rheight() += scrollBarInc;
    }

    const Qt::ScrollBarPolicy policy = view->scrollBarRequired(newViewSize)
                                       ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAsNeeded;
    if (vertical) {
        setVerticalScrollBarPolicy(policy);
    } else {
        setHorizontalScrollBarPolicy(policy);
    }
}

#include "kitemlistcontainer.moc"
