/*
 * SPDX-FileCopyrightText: 2013 Dawit Alemayehu <adawit@kde.org>
 * SPDX-FileCopyrightText: 2017 Elvis Angelaccio <elvis.angelaccio@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
    enum class ShiftState { Unknown, Pressed, Released };

    DolphinRemoveAction(QObject *parent, KActionCollection *collection);

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
