/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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
#include <QCoreApplication>
#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTimer>

DolphinViewAutoScroller::DolphinViewAutoScroller(QAbstractItemView* parent) :
    QObject(parent),
    m_rubberBandSelection(false),
    m_scrollInc(0),
    m_itemView(parent),
    m_timer()
{
    m_itemView->setAutoScroll(false);
    m_itemView->viewport()->installEventFilter(this);
    
    m_timer = new QTimer(this);
    m_timer->setSingleShot(false);
    m_timer->setInterval(1000 / 25); // 25 frames per second
    connect(m_timer, SIGNAL(timeout()), this, SLOT(scrollViewport()));
}

DolphinViewAutoScroller::~DolphinViewAutoScroller()
{
}

#include <kdebug.h>
bool DolphinViewAutoScroller::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_itemView->viewport()) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            m_rubberBandSelection = true;
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
            
        case QEvent::DragLeave:
            m_rubberBandSelection = false;
            stopAutoScroll();
            break;
            
        default:
            break;
        }
    }

    return QObject::eventFilter(watched, event);
}

void DolphinViewAutoScroller::scrollViewport()
{
    // TODO: implement horizontal scrolling
    QScrollBar* verticalScrollBar = m_itemView->verticalScrollBar();
    if (verticalScrollBar != 0) {
        const int value = verticalScrollBar->value();
        verticalScrollBar->setValue(value + m_scrollInc);
        
        if (m_rubberBandSelection) {
            // The scrolling does not lead to an update of the rubberband
            // selection. Fake a mouse move event to let the QAbstractItemView
            // update the rubberband.
            QWidget* viewport = m_itemView->viewport();
            const QPoint pos = viewport->mapFromGlobal(QCursor::pos());
            QMouseEvent event(QEvent::MouseMove, pos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(viewport, &event);
        }
    }
}

void DolphinViewAutoScroller::triggerAutoScroll()
{
    // TODO: implement horizontal scrolling
    
    const int startSpeed = 2;
    const int speedLimiter = 8;
    const int scrollIncMax = 32;
    
    const int autoScrollBorder = 32;
    
    QWidget* viewport = m_itemView->viewport();
    const QPoint pos = viewport->mapFromGlobal(QCursor::pos());
    if (pos.y() < autoScrollBorder) {
        // scroll up
        m_scrollInc = -startSpeed + (pos.y() - autoScrollBorder) / speedLimiter;
        if (m_scrollInc < -scrollIncMax) {
            m_scrollInc = -scrollIncMax;
        }
    } else if (pos.y() > viewport->height() - autoScrollBorder) {
        // scroll down
        m_scrollInc = startSpeed + (pos.y() - viewport->height() + autoScrollBorder) / speedLimiter;
        if (m_scrollInc > scrollIncMax) {
            m_scrollInc = scrollIncMax;
        }
    } else {
        // no scrolling
        m_scrollInc = 0;
    }
    
    if (m_timer->isActive()) {
        if (m_scrollInc == 0) {
            m_timer->stop();
        }
    } else if (m_scrollInc != 0) {
        m_timer->start();
    }
}

void DolphinViewAutoScroller::stopAutoScroll()
{
    m_timer->stop();
    m_scrollInc = 0;
}

#include "dolphinviewautoscroller.moc"
