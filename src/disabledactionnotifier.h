/*
 * SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QAction>
#include <QHash>
#include <QShortcut>

/**
 * @brief A helper class to display a notification when the user presses the shortcut of a disabled action.
 */
class DisabledActionNotifier : public QObject
{
    Q_OBJECT

public:
    DisabledActionNotifier(QObject *parent = nullptr);

    /**
     * Set the reason why the action is disabled.
     *
     * If the action is enabled, this function does nothing.
     *
     * Otherwise, it registers a shortcut, so when the user presses the shortcut for the
     * disabled action, it emits the disabledActionTriggered() signal with the disabled
     * action and the reason.
     *
     * If a reason has already been set, it will be replaced.
     */
    void setDisabledReason(QAction *action, QStringView reason);

    /**
     * Clear the reason if it's set before by setDisabledReason().
     *
     * If the action is enabled, this function does nothing.
     *
     * Otherwise, it unregisters any shortcut set by setDisabledReason() on the same action.
     *
     * When an action is disabled in two cases, but only case A needs to show the reason,
     * then case B should call this function. Otherwise, the reason set by case A might be
     * shown for case B.
     */
    void clearDisabledReason(QAction *action);

Q_SIGNALS:
    /**
     * Emitted when the user presses the shortcut of a disabled action.
     *
     * @param action The disabled action.
     * @param reason The reason set in setDisabledReason().
     */
    void disabledActionTriggered(const QAction *action, QString reason);

private:
    QHash<QAction *, QShortcut *> m_shortcuts;
};
