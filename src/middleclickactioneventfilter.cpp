/***************************************************************************
 *   Copyright (C) 2017 Kai Uwe Broulik <kde@privat.broulik.de>            *
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

#include "middleclickactioneventfilter.h"

#include <QAction>
#include <QEvent>
#include <QMouseEvent>
#include <QToolBar>

MiddleClickActionEventFilter::MiddleClickActionEventFilter(QObject *parent) : QObject(parent)
{

}

MiddleClickActionEventFilter::~MiddleClickActionEventFilter() = default;

bool MiddleClickActionEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress
            || event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);

        if (me->button() == Qt::MiddleButton) {
            QToolBar *toolBar = qobject_cast<QToolBar *>(watched);

            QAction *action = toolBar->actionAt(me->pos());
            if (action) {
                if (event->type() == QEvent::MouseButtonPress) {
                    m_lastMiddlePressedAction = action;
                } else if (event->type() == QEvent::MouseButtonRelease) {
                    if (m_lastMiddlePressedAction == action) {
                        emit actionMiddleClicked(action);
                    }
                    m_lastMiddlePressedAction = nullptr;
                }
            }
        }
    }

    return QObject::eventFilter(watched, event);
}
