/*
 * SPDX-FileCopyrightText: 2014 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHIN_RECENT_TABS_MENU_H
#define DOLPHIN_RECENT_TABS_MENU_H

#include <KActionMenu>

#include <QUrl>

class QAction;

class DolphinRecentTabsMenu : public KActionMenu
{
    Q_OBJECT

public:
    explicit DolphinRecentTabsMenu(QObject* parent);

public Q_SLOTS:
    void rememberClosedTab(const QUrl& url, const QByteArray& state);
    void undoCloseTab();

Q_SIGNALS:
    void restoreClosedTab(const QByteArray& state);
    void closedTabsCountChanged(unsigned int count);

private Q_SLOTS:
    void handleAction(QAction* action);

private:
    QAction* m_clearListAction;
};

#endif
