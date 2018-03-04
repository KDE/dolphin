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

#ifndef DOLPHINREMOVEACTION_H
#define DOLPHINREMOVEACTION_H

#include "dolphin_export.h"

#include <KActionCollection>

#include <QAction>
#include <QPointer>

/**
 * A QAction that manages the delete based on the current state of
 * the Shift key or the parameter passed to update.
 *
 * This class expects the presence of both the KStandardAction::MoveToTrash and
 * KStandardAction::DeleteFile actions in @ref collection.
 */
class DOLPHIN_EXPORT DolphinRemoveAction : public QAction
{
  Q_OBJECT
public:

    enum class ShiftState {
        Unknown,
        Pressed,
        Released
    };

    DolphinRemoveAction(QObject* parent, KActionCollection* collection);

    /**
     * Updates this action key based on @p shiftState.
     * Default value is QueryShiftState, meaning it will query QGuiApplication::modifiers().
     */
    void update(ShiftState shiftState = ShiftState::Unknown);

private Q_SLOTS:
    void slotRemoveActionTriggered();

private:
    QPointer<KActionCollection> m_collection;
    QPointer<QAction> m_action;
};

#endif
