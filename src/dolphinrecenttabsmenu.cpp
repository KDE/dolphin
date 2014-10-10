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

#include "dolphinrecenttabsmenu.h"

#include <KLocalizedString>
#include <KAcceleratorManager>
#include <kio/global.h>
#include <QMenu>

DolphinRecentTabsMenu::DolphinRecentTabsMenu(QObject* parent) :
    KActionMenu(QIcon::fromTheme("edit-undo"), i18n("Recently Closed Tabs"), parent)
{
    setDelayed(false);
    setEnabled(false);

    m_clearListAction = new QAction(i18n("Empty Recently Closed Tabs"), this);
    m_clearListAction->setIcon(QIcon::fromTheme("edit-clear-list"));
    addAction(m_clearListAction);

    addSeparator();

    connect(menu(), SIGNAL(triggered(QAction*)),
            this, SLOT(handleAction(QAction*)));
}

void DolphinRecentTabsMenu::rememberClosedTab(const KUrl& url, const QByteArray& state)
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
    emit closedTabsCountChanged(menu()->actions().size() - 2);
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
        emit closedTabsCountChanged(0);
    } else {
        const QByteArray state = action->data().value<QByteArray>();
        removeAction(action);
        delete action;
        action = 0;
        emit restoreClosedTab(state);
        emit closedTabsCountChanged(menu()->actions().size() - 2);
    }

    if (menu()->actions().count() <= 2) {
        setEnabled(false);
    }
}