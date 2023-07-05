/*
 * SPDX-FileCopyrightText: 2017 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "middleclickactioneventfilter.h"

#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QToolBar>

MiddleClickActionEventFilter::MiddleClickActionEventFilter(QObject *parent)
    : QObject(parent)
{
}

MiddleClickActionEventFilter::~MiddleClickActionEventFilter() = default;

bool MiddleClickActionEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);

        if (me->button() == Qt::MiddleButton) {
            QToolBar *toolBar = qobject_cast<QToolBar *>(watched);
            if (toolBar) {
                QAction *action = toolBar->actionAt(me->pos());
                if (action) {
                    if (event->type() == QEvent::MouseButtonPress) {
                        m_lastMiddlePressedAction = action;
                    } else if (event->type() == QEvent::MouseButtonRelease) {
                        if (m_lastMiddlePressedAction == action) {
                            Q_EMIT actionMiddleClicked(action);
                        }
                        m_lastMiddlePressedAction = nullptr;
                    }
                }
            }
            QMenu *menu = qobject_cast<QMenu *>(watched);
            if (menu) {
                QAction *action = menu->actionAt(me->pos());
                if (action) {
                    if (event->type() == QEvent::MouseButtonPress) {
                        m_lastMiddlePressedAction = action;
                    } else if (event->type() == QEvent::MouseButtonRelease) {
                        if (m_lastMiddlePressedAction == action) {
                            Q_EMIT actionMiddleClicked(action);
                            return true;
                        }
                        m_lastMiddlePressedAction = nullptr;
                    }
                }
            }
        }
    }

    return QObject::eventFilter(watched, event);
}

#include "moc_middleclickactioneventfilter.cpp"
