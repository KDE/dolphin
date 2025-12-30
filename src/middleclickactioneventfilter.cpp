/*
 * SPDX-FileCopyrightText: 2017 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "middleclickactioneventfilter.h"

#include <QAction>
#include <QActionEvent>
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

    } else if (event->type() == QEvent::ActionChanged) {
        // KXmlGui adds delayed popup menu's actions to its own context menu.
        // In order for middle click to work, we need to somehow detect when the action is added
        // on this menu and install the event filter on it.
        // Visibility of the action changes when the menu shows in the menu for the first time,
        // so we'll receive an ActionChanged event and use that.
        auto *actionEvent = static_cast<QActionEvent *>(event);

        const auto items = actionEvent->action()->associatedObjects();
        for (auto *item : items) {
            if (auto *menu = qobject_cast<QMenu *>(item)) {
                if (menu != actionEvent->action()->parent()) { // Parent is the regular menu.
                    menu->installEventFilter(this);
                }
            }
        }
    }

    return QObject::eventFilter(watched, event);
}

#include "moc_middleclickactioneventfilter.cpp"
