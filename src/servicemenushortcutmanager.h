/*
 * SPDX-FileCopyrightText: 2026 Albert Mkhitaryan <mkhalbert@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SERVICEMENUSHORTCUTMANAGER_H
#define SERVICEMENUSHORTCUTMANAGER_H

#include <QObject>
#include <QStringList>

class KActionCategory;
class KActionCollection;
class KFileItemActions;
class KXMLGUIClient;

/**
 * @brief Registers service menu actions with KActionCollection for keyboard shortcuts.
 *
 * This lightweight manager bridges KIO's service menu actions with Dolphin's
 * shortcut system. Actions are obtained from KFileItemActions and registered
 * with the action collection. Execution is handled entirely by KFileItemActions.
 *
 */
class ServiceMenuShortcutManager : public QObject
{
    Q_OBJECT

public:
    explicit ServiceMenuShortcutManager(KActionCollection *actionCollection, QObject *parent = nullptr);

    void refresh(KFileItemActions *fileItemaActions);
    void cleanupStaleShortcuts(KXMLGUIClient *client);

private:
    KActionCollection *m_actionCollection;
    KActionCategory *m_category;
    QStringList m_actionNames;
};

#endif // SERVICEMENUSHORTCUTMANAGER_H
