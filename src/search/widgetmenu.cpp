/*
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "widgetmenu.h"

#include <QApplication>
#include <QShowEvent>
#include <QWidgetAction>

using namespace Search;

Search::WidgetMenu::WidgetMenu(QWidget *parent)
    : QMenu{parent}
{
    connect(
        this,
        &QMenu::aboutToShow,
        this,
        [this]() {
            auto widgetAction = new QWidgetAction{this};
            auto widget = init();
            Q_CHECK_PTR(widget);
            widgetAction->setDefaultWidget(widget); // Transfers ownership to the widgetAction.
            addAction(widgetAction);
        },
        Qt::SingleShotConnection);
}

bool WidgetMenu::focusNextPrevChild(bool next)
{
    return QWidget::focusNextPrevChild(next);
}

void WidgetMenu::mouseReleaseEvent(QMouseEvent *event)
{
    return QWidget::mouseReleaseEvent(event);
}

void WidgetMenu::resizeToFitContents()
{
    auto *widgetAction = static_cast<QWidgetAction *>(actions().first());
    auto focusedChildWidget = QApplication::focusWidget();
    if (!widgetAction->defaultWidget()->isAncestorOf(focusedChildWidget)) {
        focusedChildWidget = nullptr;
    }

    // Removing and readding the widget triggers the resize.
    removeAction(widgetAction);
    addAction(widgetAction);

    // The previous removing and readding removed the focus from any child widgets. We return the focus to where it was.
    if (focusedChildWidget) {
        focusedChildWidget->setFocus();
    }
}

void WidgetMenu::showEvent(QShowEvent *event)
{
    if (!event->spontaneous()) {
        auto widgetAction = static_cast<QWidgetAction *>(actions().first());
        widgetAction->defaultWidget()->setFocus();
    }
    QMenu::showEvent(event);
}
