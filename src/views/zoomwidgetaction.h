/*
 * SPDX-FileCopyrightText: 2025 Gleb Kasachou <gkosachov99@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ZOOM_WIDGET_ACTION_H
#define ZOOM_WIDGET_ACTION_H

#include <KToolBarPopupAction>

/**
 * This WidgetAction combines the three zoom actions into one line.
 *
 * This requires less space, logically groups these actions, and keeps the menu open when the user uses the buttons.
 */

class ZoomWidgetAction : public KToolBarPopupAction
{
public:
    ZoomWidgetAction(QAction *zoomInAction, QAction *zoomResetAction, QAction *zoomOutAction, QObject *parent);

protected:
    QWidget *createWidget(QWidget *parent) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    QAction *m_zoomInAction;
    QAction *m_zoomResetAction;
    QAction *m_zoomOutAction;
};

#endif
