/*
 * SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KLocalizedString>

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
     * @param text   The name of the action and the reason why the action is disabled.
     */
    void disabledActionTriggered(const QAction *action, QString text);

private:
    void onShortcutActivated(const QAction *action, QStringView reason);

    QHash<QAction *, QShortcut *> m_shortcuts;
};

namespace DisabledReason
{

// Note: the reason should start with a lowercase letter, because it will be shown in
// a sentence like "Could not <action>: <reason>".

const QString destinationReadOnly =
    i18nc("@info reason for disabled action: copy/move files to destination", "you do not have permission to write into the destination folder.");
const QString emptyClipboard = i18nc("@info reason for disabled action: paste", "the clipboard is empty.");
const QString noPermission = i18nc("@info reason for disabled action: cut/paste/rename/duplicate files", "you do not have permission in this folder.");
const QString noSelection = i18nc("@info reason for disabled action: file operations", "no files selected.");
const QString targetInOriginWhenMove = i18nc("@info reason for disabled action: move files to the other view", "moving a folder onto itself is not possible.");
const QString targetInOriginWhenCopy = i18nc("@info reason for disabled action: copy files to the other view", "copying a folder onto itself is not possible.");

}
