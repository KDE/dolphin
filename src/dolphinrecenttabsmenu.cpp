/*
 * SPDX-FileCopyrightText: 2014 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinrecenttabsmenu.h"

#include <KAcceleratorManager>
#include <KLocalizedString>
#include <kio/global.h>

#include <QMenu>

DolphinRecentTabsMenu::DolphinRecentTabsMenu(QObject* parent) :
    KActionMenu(QIcon::fromTheme(QStringLiteral("edit-undo")), i18n("Recently Closed Tabs"), parent)
{
    setPopupMode(QToolButton::InstantPopup);
    setEnabled(false);

    m_clearListAction = new QAction(i18n("Empty Recently Closed Tabs"), this);
    m_clearListAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-list")));
    addAction(m_clearListAction);

    addSeparator();

    connect(menu(), &QMenu::triggered,
            this, &DolphinRecentTabsMenu::handleAction);
}

void DolphinRecentTabsMenu::rememberClosedTab(const QUrl& url, const QByteArray& state)
{
    QAction* action = new QAction(menu());
    action->setText(url.path());
    action->setData(state);
    const QString iconName = KIO::iconNameForUrl(url);
    action->setIcon(QIcon::fromTheme(iconName));

    // Add the closed tab menu entry after the separator and
    // "Empty Recently Closed Tabs" entry
    if (menu()->actions().size() == 2) {
        addAction(action);
    } else {
        insertAction(menu()->actions().at(2), action);
    }
    Q_EMIT closedTabsCountChanged(menu()->actions().size() - 2);
    // Assure that only up to 6 closed tabs are shown in the menu.
    // 8 because of clear action + separator + 6 closed tabs
    if (menu()->actions().size() > 8) {
        removeAction(menu()->actions().last());
    }
    setEnabled(true);
    KAcceleratorManager::manage(menu());
}

void DolphinRecentTabsMenu::undoCloseTab()
{
    Q_ASSERT(menu()->actions().size() > 2);
    handleAction(menu()->actions().at(2));
}

void DolphinRecentTabsMenu::handleAction(QAction* action)
{
    if (action == m_clearListAction) {
        // Clear all actions except the "Empty Recently Closed Tabs"
        // action and the separator
        QList<QAction*> actions = menu()->actions();
        const int count = actions.size();
        for (int i = 2; i < count; ++i) {
            removeAction(actions.at(i));
        }
        Q_EMIT closedTabsCountChanged(0);
    } else {
        const QByteArray state = action->data().toByteArray();
        removeAction(action);
        delete action;
        action = nullptr;
        Q_EMIT restoreClosedTab(state);
        Q_EMIT closedTabsCountChanged(menu()->actions().size() - 2);
    }

    if (menu()->actions().count() <= 2) {
        setEnabled(false);
    }
}
