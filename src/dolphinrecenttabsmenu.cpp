/***************************************************************************
 * Copyright (C) 2013 by Emmanuel Pescosta <emmanuelpescosta099@gmail.com> *
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
#include <KMenu>

DolphinRecentTabsMenu::DolphinRecentTabsMenu(QObject* parent) :
    KActionMenu(KIcon("edit-undo"), i18n("Recently Closed Tabs"), parent),
    m_clearListAction(0)
{
    setDelayed(false);
    setEnabled(false);

    m_clearListAction = new QAction(i18n("Empty Recently Closed Tabs"), this);
    m_clearListAction->setIcon(KIcon("edit-clear-list"));
    addAction(m_clearListAction);

    addSeparator();

    connect(menu(), SIGNAL(triggered(QAction*)),
            this, SLOT(handleAction(QAction*)));
}

void DolphinRecentTabsMenu::rememberClosedTab(const ClosedTab& tab)
{
    QAction* action = new QAction(menu());
    action->setText(tab.text);
    action->setIcon(tab.icon);
    action->setData(QVariant::fromValue(tab));

    // Add the closed tab menu entry after the separator and
    // "Empty Recently Closed Tabs" entry
    if (menu()->actions().size() == 2) {
        addAction(action);
    } else {
        insertAction(menu()->actions().at(2), action);
    }

    // assure that only up to 8 closed tabs are shown in the menu
    if (menu()->actions().size() > 8) {
        removeAction(menu()->actions().last());
    }
    setEnabled(true);
    KAcceleratorManager::manage(menu());
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
    } else {
        const ClosedTab closedTab = action->data().value<ClosedTab>();
        emit restoreClosedTab(closedTab);
        removeAction(action);
    }

    if (menu()->actions().count() <= 2) {
        setEnabled(false);
    }
}