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

#include <QGraphicsScene>
#include <QGraphicsView>
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
    m_controller(controller)
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
    const qreal offsetDiff = (view->scrollOrientation() == Qt::Vertical) ? dy : dx;
    view->setOffset(currentOffset - offsetDiff);
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
    }
    if (current) {
        scene->addItem(current);
        connect(previous, SIGNAL(offsetChanged(int,int)), this, SLOT(updateScrollBars()));
        connect(current, SIGNAL(maximumOffsetChanged(int,int)), this, SLOT(updateScrollBars()));
    }
}

void KItemListContainer::updateScrollBars()
{
    const QSizeF size = m_controller->view()->size();

    if (m_controller->view()->scrollOrientation() == Qt::Vertical) {
        QScrollBar* scrollBar = verticalScrollBar();
        const int value = m_controller->view()->offset();
        const int maximum = qMax(0, int(m_controller->view()->maximumOffset() - size.height()));
        scrollBar->setPageStep(size.height());
        scrollBar->setMinimum(0);
        scrollBar->setMaximum(maximum);
        scrollBar->setValue(value);
        horizontalScrollBar()->setMaximum(0);
    } else {
        QScrollBar* scrollBar = horizontalScrollBar();
        const int value = m_controller->view()->offset();
        const int maximum = qMax(0, int(m_controller->view()->maximumOffset() - size.width()));
        scrollBar->setPageStep(size.width());
        scrollBar->setMinimum(0);
        scrollBar->setMaximum(maximum);
        scrollBar->setValue(value);
        verticalScrollBar()->setMaximum(0);
    }
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
}

#include "kitemlistcontainer.moc"
