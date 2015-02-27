/***************************************************************************
 * Copyright (C) 2014 by Emmanuel Pescosta <emmanuelpescosta099@gmail.com> *
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

#include "dolphintabbar.h"

#include <QTimer>
#include <QDragEnterEvent>
#include <KLocalizedString>
#include <QMenu>
#include <QIcon>
#include <QUrl>
#include <QMimeData>

DolphinTabBar::DolphinTabBar(QWidget* parent) :
    QTabBar(parent),
    m_autoActivationIndex(-1)
{
    setAcceptDrops(true);
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setMovable(true);
    setTabsClosable(true);

    m_autoActivationTimer = new QTimer(this);
    m_autoActivationTimer->setSingleShot(true);
    m_autoActivationTimer->setInterval(800);
    connect(m_autoActivationTimer, SIGNAL(timeout()),
            this, SLOT(slotAutoActivationTimeout()));
}

void DolphinTabBar::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    const int index = tabAt(event->pos());

    if (mimeData->hasUrls()) {
        event->acceptProposedAction();
        updateAutoActivationTimer(index);
    }

    QTabBar::dragEnterEvent(event);
}

void DolphinTabBar::dragLeaveEvent(QDragLeaveEvent* event)
{
    updateAutoActivationTimer(-1);

    QTabBar::dragLeaveEvent(event);
}

void DolphinTabBar::dragMoveEvent(QDragMoveEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    const int index = tabAt(event->pos());

    if (mimeData->hasUrls()) {
        updateAutoActivationTimer(index);
    }

    QTabBar::dragMoveEvent(event);
}

void DolphinTabBar::dropEvent(QDropEvent* event)
{
    // Disable the auto activation timer
    updateAutoActivationTimer(-1);

    const QMimeData* mimeData = event->mimeData();
    const int index = tabAt(event->pos());

    if (index >= 0 && mimeData->hasUrls()) {
        emit tabDropEvent(index, event);
    }

    QTabBar::dropEvent(event);
}

void DolphinTabBar::mousePressEvent(QMouseEvent* event)
{
    const int index = tabAt(event->pos());

    if (index >= 0 && event->button() == Qt::MiddleButton) {
        // Mouse middle click on a tab closes this tab.
        emit tabCloseRequested(index);
        return;
    }

    QTabBar::mousePressEvent(event);
}

void DolphinTabBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    const int index = tabAt(event->pos());

    if (index < 0) {
        // Double click on the empty tabbar area opens a new activated tab
        // with the url from the current tab.
        emit openNewActivatedTab(currentIndex());
        return;
    }

    QTabBar::mouseDoubleClickEvent(event);
}

void DolphinTabBar::contextMenuEvent(QContextMenuEvent* event)
{
    const int index = tabAt(event->pos());

    if (index >= 0) {
        // Tab context menu
        QMenu menu(this);

        QAction* newTabAction = menu.addAction(QIcon::fromTheme("tab-new"), i18nc("@action:inmenu", "New Tab"));
        QAction* detachTabAction = menu.addAction(QIcon::fromTheme("tab-detach"), i18nc("@action:inmenu", "Detach Tab"));
        QAction* closeOtherTabsAction = menu.addAction(QIcon::fromTheme("tab-close-other"), i18nc("@action:inmenu", "Close Other Tabs"));
        QAction* closeTabAction = menu.addAction(QIcon::fromTheme("tab-close"), i18nc("@action:inmenu", "Close Tab"));

        QAction* selectedAction = menu.exec(event->globalPos());
        if (selectedAction == newTabAction) {
            emit openNewActivatedTab(index);
        } else if (selectedAction == detachTabAction) {
            emit tabDetachRequested(index);
        } else if (selectedAction == closeOtherTabsAction) {
            const int tabCount = count();
            for (int i = 0; i < index; i++) {
                emit tabCloseRequested(0);
            }
            for (int i = index + 1; i < tabCount; i++) {
                emit tabCloseRequested(1);
            }
        } else if (selectedAction == closeTabAction) {
            emit tabCloseRequested(index);
        }

        return;
    }

    QTabBar::contextMenuEvent(event);
}

void DolphinTabBar::slotAutoActivationTimeout()
{
    if (m_autoActivationIndex >= 0) {
        setCurrentIndex(m_autoActivationIndex);
        updateAutoActivationTimer(-1);
    }
}

void DolphinTabBar::updateAutoActivationTimer(const int index)
{
    if (m_autoActivationIndex != index) {
        m_autoActivationIndex = index;

        if (m_autoActivationIndex < 0) {
            m_autoActivationTimer->stop();
        } else {
            m_autoActivationTimer->start();
        }
    }
}
