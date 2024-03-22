/*
 * SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "disabledactionnotifier.h"

#include <KLocalizedString>

#include <QRegularExpression>

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
        delete m_shortcuts.take(action);
    }

    QShortcut *shortcut = new QShortcut(action->shortcut(), parent());
    m_shortcuts.insert(action, shortcut);

    connect(action, &QAction::enabledChanged, this, [this, action](bool enabled) {
        if (enabled) {
            delete m_shortcuts.take(action);
        }
    });

    // Don't capture QStringView, as it may reference a temporary QString
    QString reasonString = reason.toString();
    connect(shortcut, &QShortcut::activated, this, [this, action, reasonString]() {
        onShortcutActivated(action, reasonString);
    });
}

void DisabledActionNotifier::clearDisabledReason(QAction *action)
{
    if (action->isEnabled()) {
        return;
    }

    if (m_shortcuts.contains(action)) {
        delete m_shortcuts.take(action);
    }
}

void DisabledActionNotifier::onShortcutActivated(const QAction *action, QStringView reason)
{
    QString actionDesc = action->toolTip().toLower();
    // Remove the trailing "(X)" and ellipsis
    // TODO: The "(X)" match is a workaround for Chinese tooltips.
    //       Remove when the following is fixed:
    //       https://bugreports.qt.io/browse/QTBUG-123473
    QRegularExpression re("(\\([A-Za-z0-9]\\))?…?$");
    actionDesc.remove(re);

    QString text =
        i18nc("@info: message when the user presses the shortcut of a disabled action. %1 is description of the action. %2 is the reason it's disabled.",
              "Could not %1: %2",
              actionDesc,
              reason.toString());

    Q_EMIT disabledActionTriggered(action, text);
}

#include "moc_disabledactionnotifier.cpp"
