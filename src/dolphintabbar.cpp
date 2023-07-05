/*
 * SPDX-FileCopyrightText: 2014 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphintabbar.h"

#include <KLocalizedString>

#include <QDragEnterEvent>
#include <QMenu>
#include <QMimeData>
#include <QTimer>

DolphinTabBar::DolphinTabBar(QWidget *parent)
    : QTabBar(parent)
    , m_autoActivationIndex(-1)
    , m_tabToBeClosedOnMiddleMouseButtonRelease(-1)
{
    setAcceptDrops(true);
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setMovable(true);
    setTabsClosable(true);

    m_autoActivationTimer = new QTimer(this);
    m_autoActivationTimer->setSingleShot(true);
    m_autoActivationTimer->setInterval(800);
    connect(m_autoActivationTimer, &QTimer::timeout, this, &DolphinTabBar::slotAutoActivationTimeout);
}

void DolphinTabBar::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    const int index = tabAt(event->position().toPoint());

    if (mimeData->hasUrls()) {
        event->acceptProposedAction();
        updateAutoActivationTimer(index);
    }

    QTabBar::dragEnterEvent(event);
}

void DolphinTabBar::dragLeaveEvent(QDragLeaveEvent *event)
{
    updateAutoActivationTimer(-1);

    QTabBar::dragLeaveEvent(event);
}

void DolphinTabBar::dragMoveEvent(QDragMoveEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    const int index = tabAt(event->position().toPoint());

    if (mimeData->hasUrls()) {
        updateAutoActivationTimer(index);
    }

    QTabBar::dragMoveEvent(event);
}

void DolphinTabBar::dropEvent(QDropEvent *event)
{
    // Disable the auto activation timer
    updateAutoActivationTimer(-1);

    const QMimeData *mimeData = event->mimeData();
    const int index = tabAt(event->position().toPoint());

    if (mimeData->hasUrls()) {
        Q_EMIT tabDropEvent(index, event);
    }

    QTabBar::dropEvent(event);
}

void DolphinTabBar::mousePressEvent(QMouseEvent *event)
{
    const int index = tabAt(event->pos());

    if (index >= 0 && event->button() == Qt::MiddleButton) {
        m_tabToBeClosedOnMiddleMouseButtonRelease = index;
        return;
    }

    QTabBar::mousePressEvent(event);
}

void DolphinTabBar::mouseReleaseEvent(QMouseEvent *event)
{
    const int index = tabAt(event->pos());

    if (index >= 0 && index == m_tabToBeClosedOnMiddleMouseButtonRelease && event->button() == Qt::MiddleButton) {
        // Mouse middle click on a tab closes this tab.
        Q_EMIT tabCloseRequested(index);
        return;
    }

    QTabBar::mouseReleaseEvent(event);
}

void DolphinTabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    int index = tabAt(event->pos());

    if (index < 0) {
        // empty tabbar area case
        index = currentIndex();
    }
    // Double click on the tabbar opens a new activated tab
    // with the url from the doubleclicked tab or currentTab otherwise.
    Q_EMIT openNewActivatedTab(index);

    QTabBar::mouseDoubleClickEvent(event);
}

void DolphinTabBar::contextMenuEvent(QContextMenuEvent *event)
{
    const int index = tabAt(event->pos());

    if (index >= 0) {
        // Tab context menu
        QMenu menu(this);

        QAction *newTabAction = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-new")), i18nc("@action:inmenu", "New Tab"));
        QAction *detachTabAction = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-detach")), i18nc("@action:inmenu", "Detach Tab"));
        QAction *closeOtherTabsAction = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-close-other")), i18nc("@action:inmenu", "Close Other Tabs"));
        QAction *closeTabAction = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-close")), i18nc("@action:inmenu", "Close Tab"));

        QAction *selectedAction = menu.exec(event->globalPos());
        if (selectedAction == newTabAction) {
            Q_EMIT openNewActivatedTab(index);
        } else if (selectedAction == detachTabAction) {
            Q_EMIT tabDetachRequested(index);
        } else if (selectedAction == closeOtherTabsAction) {
            const int tabCount = count();
            for (int i = 0; i < index; i++) {
                Q_EMIT tabCloseRequested(0);
            }
            for (int i = index + 1; i < tabCount; i++) {
                Q_EMIT tabCloseRequested(1);
            }
        } else if (selectedAction == closeTabAction) {
            Q_EMIT tabCloseRequested(index);
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

#include "moc_dolphintabbar.cpp"
