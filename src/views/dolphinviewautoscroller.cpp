/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "dolphinviewautoscroller.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTimer>
#include <math.h>

DolphinViewAutoScroller::DolphinViewAutoScroller(QAbstractItemView* parent) :
    QObject(parent),
    m_rubberBandSelection(false),
    m_keyPressed(false),
    m_initializedTimestamp(false),
    m_horizontalScrollInc(0),
    m_verticalScrollInc(0),
    m_itemView(parent),
    m_timer(0),
    m_timestamp()
{
    m_itemView->setAutoScroll(false);
    m_itemView->viewport()->installEventFilter(this);
    m_itemView->installEventFilter(this);

    m_timer = new QTimer(this);
    m_timer->setSingleShot(false);
    m_timer->setInterval(1000 / 25); // 25 frames per second
    connect(m_timer, SIGNAL(timeout()), this, SLOT(scrollViewport()));
}

DolphinViewAutoScroller::~DolphinViewAutoScroller()
{
}

bool DolphinViewAutoScroller::isActive() const
{
    return m_timer->isActive();
}

void DolphinViewAutoScroller::handleCurrentIndexChange(const QModelIndex& current,
                                                       const QModelIndex& previous)
{
    // When the autoscroller is inactive and a key has been pressed, it must be
    // assured that the current item stays visible. The check whether the previous
    // item is valid is important because of #197951. The keypress check is done
    // because of #199833.
    if (current.isValid() && (previous.isValid() || m_keyPressed) && !isActive()) {
        m_itemView->scrollTo(current);
    }
}

bool DolphinViewAutoScroller::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_itemView->viewport()) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                m_rubberBandSelection = true;
            }
            break;

        case QEvent::MouseMove:
            if (m_rubberBandSelection) {
                triggerAutoScroll();
            }
            break;

        case QEvent::MouseButtonRelease:
            m_rubberBandSelection = false;
            stopAutoScroll();
            break;

        case QEvent::DragEnter:
        case QEvent::DragMove:
            m_rubberBandSelection = false;
            triggerAutoScroll();
            break;

        case QEvent::Drop:
        case QEvent::DragLeave:
            m_rubberBandSelection = false;
            stopAutoScroll();
            break;

        default:
            break;
        }
    } else if (watched == m_itemView) {
        switch (event->type()) {
        case QEvent::KeyPress:
            m_keyPressed = true;
            break;

        case QEvent::KeyRelease:
            m_keyPressed = false;
            break;

        default:
            break;
        }
    }

    return QObject::eventFilter(watched, event);
}

void DolphinViewAutoScroller::scrollViewport()
{
    if (m_timestamp.elapsed() < QApplication::startDragTime()) {
        return;
    }

    QScrollBar* verticalScrollBar = m_itemView->verticalScrollBar();
    if (verticalScrollBar) {
        const int value = verticalScrollBar->value();
        verticalScrollBar->setValue(value + m_verticalScrollInc);

    }
    QScrollBar* horizontalScrollBar = m_itemView->horizontalScrollBar();
    if (horizontalScrollBar) {
        const int value = horizontalScrollBar->value();
        horizontalScrollBar->setValue(value + m_horizontalScrollInc);

    }

    if (m_rubberBandSelection) {
        // The scrolling does not lead to an update of the rubberband
        // selection. Fake a mouse move event to let the QAbstractItemView
        // update the rubberband.
        QWidget* viewport = m_itemView->viewport();
        const QPoint pos = viewport->mapFromGlobal(QCursor::pos());
        QMouseEvent event(QEvent::MouseMove, pos, Qt::LeftButton, Qt::LeftButton, QApplication::keyboardModifiers());
        QCoreApplication::sendEvent(viewport, &event);
    }
}

void DolphinViewAutoScroller::triggerAutoScroll()
{
    const bool verticalScrolling = m_itemView->verticalScrollBar() &&
                                   m_itemView->verticalScrollBar()->isVisible();
    const bool horizontalScrolling = m_itemView->horizontalScrollBar() &&
                                     m_itemView->horizontalScrollBar()->isVisible();
    if (!verticalScrolling && !horizontalScrolling) {
        // no scrollbars are shown at all, so no autoscrolling is necessary
        stopAutoScroll();
        return;
    }

    QWidget* viewport = m_itemView->viewport();
    const QPoint pos = viewport->mapFromGlobal(QCursor::pos());
    if (verticalScrolling) {
        m_verticalScrollInc = calculateScrollIncrement(pos.y(), viewport->height());
    }
    if (horizontalScrolling) {
        m_horizontalScrollInc = calculateScrollIncrement(pos.x(), viewport->width());
    }

    if (m_timer->isActive()) {
        if ((m_horizontalScrollInc == 0) && (m_verticalScrollInc == 0)) {
            stopAutoScroll();
        }
    } else if ((m_horizontalScrollInc != 0) || (m_verticalScrollInc != 0)) {
        if (!m_initializedTimestamp) {
            m_initializedTimestamp = true;
            m_timestamp.start();
        }
        m_timer->start();
    }
}

void DolphinViewAutoScroller::stopAutoScroll()
{
    m_timer->stop();
    m_horizontalScrollInc = 0;
    m_verticalScrollInc = 0;
    m_initializedTimestamp = false;
}

int DolphinViewAutoScroller::calculateScrollIncrement(int cursorPos, int rangeSize) const
{
    int inc = 0;

    const int minSpeed = 4;
    const int maxSpeed = 768;
    const int speedLimiter = 48;
    const int autoScrollBorder = 64;

    if (cursorPos < autoScrollBorder) {
        inc = -minSpeed + qAbs(cursorPos - autoScrollBorder) * (cursorPos - autoScrollBorder) / speedLimiter;
        if (inc < -maxSpeed) {
            inc = -maxSpeed;
        }
    } else if (cursorPos > rangeSize - autoScrollBorder) {
        inc = minSpeed + qAbs(cursorPos - rangeSize + autoScrollBorder) * (cursorPos - rangeSize + autoScrollBorder) / speedLimiter;
        if (inc > maxSpeed) {
            inc = maxSpeed;
        }
    }

    return inc;
}

#include "dolphinviewautoscroller.moc"
