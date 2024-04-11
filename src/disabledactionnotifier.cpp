/*
 * SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "disabledactionnotifier.h"

DisabledActionNotifier::DisabledActionNotifier(QObject *parent)
    : QObject(parent)
{
}

void DisabledActionNotifier::setDisabledReason(QAction *action, QStringView reason)
{
    if (action->isEnabled()) {
        return;
    }

    if (m_shortcuts.contains(action)) {
        clearDisabledReason(action);
    }

    QShortcut *shortcut = new QShortcut(action->shortcut(), parent());
    m_shortcuts.insert(action, shortcut);

    connect(
        action,
        &QAction::enabledChanged,
        this,
        [this, action](bool enabled) {
            if (enabled && m_shortcuts.contains(action)) {
                m_shortcuts.take(action)->deleteLater();
            }
        },
        Qt::SingleShotConnection);

    // Don't capture QStringView, as it may reference a temporary QString
    QString reasonString = reason.toString();
    connect(shortcut, &QShortcut::activated, this, [this, action, reasonString]() {
        Q_EMIT disabledActionTriggered(action, reasonString);
    });
}

void DisabledActionNotifier::clearDisabledReason(QAction *action)
{
    if (action->isEnabled()) {
        return;
    }

    action->disconnect(this);
    if (m_shortcuts.contains(action)) {
        m_shortcuts.take(action)->deleteLater();
    }
}

#include "moc_disabledactionnotifier.cpp"
