/***************************************************************************
 *   Copyright (C) 2013 by Dawit Alemayehu <adawit@kde.org>                *
 *   Copyright (C) 2017 by Elvis Angelaccio <elvis.angelaccio@kde.org>     *
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

#include "dolphinremoveaction.h"

#include <QApplication>

#include <KLocalizedString>


DolphinRemoveAction::DolphinRemoveAction(QObject* parent, KActionCollection* collection) :
    QAction(parent),
    m_collection(collection)
{
    update();
    connect(this, &DolphinRemoveAction::triggered, this, &DolphinRemoveAction::slotRemoveActionTriggered);
}

void DolphinRemoveAction::slotRemoveActionTriggered()
{
    if (m_action) {
        m_action->trigger();
    }
}

void DolphinRemoveAction::update(ShiftState shiftState)
{
    if (!m_collection) {
        m_action = nullptr;
        return;
    }

    if (shiftState == ShiftState::Unknown) {
        shiftState = QGuiApplication::keyboardModifiers() & Qt::ShiftModifier ? ShiftState::Pressed : ShiftState::Released;
    }

    switch (shiftState) {
    case ShiftState::Pressed: {
        m_action = m_collection->action(KStandardAction::name(KStandardAction::DeleteFile));
        // Make sure we show Shift+Del in the context menu.
        auto deleteShortcuts = m_action->shortcuts();
        deleteShortcuts.removeAll(Qt::SHIFT | Qt::Key_Delete);
        deleteShortcuts.prepend(Qt::SHIFT | Qt::Key_Delete);
        m_collection->setDefaultShortcuts(this, deleteShortcuts);
        break;
    }
    case ShiftState::Released:
        m_action = m_collection->action(KStandardAction::name(KStandardAction::MoveToTrash));
        m_collection->setDefaultShortcuts(this, m_action->shortcuts());
        break;
    case ShiftState::Unknown:
        Q_UNREACHABLE();
        break;
    }

    if (m_action) {
        setText(m_action->text());
        setIcon(m_action->icon());
        setEnabled(m_action->isEnabled());
    }
}
