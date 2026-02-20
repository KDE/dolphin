/*
 * SPDX-FileCopyrightText: 2026 Albert Mkhitaryan <mkhalbert@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <servicemenushortcutmanager.h>

#include <KActionCategory>
#include <KActionCollection>
#include <KDesktopFileAction>
#include <KFileItemActions>
#include <KLocalizedString>
#include <KXMLGUIClient>
#include <KXMLGUIFactory>

#include <QAction>
#include <QDomDocument>
#include <QFileInfo>

static constexpr QLatin1String serviceMenuNamePrefix("servicemenu_");

ServiceMenuShortcutManager::ServiceMenuShortcutManager(KActionCollection *actionCollection, QObject *parent)
    : QObject(parent)
    , m_actionCollection(actionCollection)
    , m_category(new KActionCategory(i18nc("@title:group", "Context Menu Actions"), actionCollection))
{
}

void ServiceMenuShortcutManager::refresh(KFileItemActions *fileItemActions)
{
    // Uses takeAction() to remove without deleting. Actions are owned by KFileItemActions
    // and are deleted when called createServiceMenuActions() or destroyed
    for (const QString &name : std::as_const(m_actionNames)) {
        m_actionCollection->takeAction(m_actionCollection->action(name));
    }
    m_actionNames.clear();

    // Get action->execution mapping from KIO
    const QList<QAction *> actions = fileItemActions->createServiceMenuActions();

    for (QAction *action : actions) {
        const auto desktopAction = action->data().value<KDesktopFileAction>();
        const QString fileName = QFileInfo(desktopAction.desktopFilePath()).fileName();
        const QString name = serviceMenuNamePrefix + fileName + QLatin1String("::") + desktopAction.actionsKey();
        m_category->addAction(name, action);
        m_actionNames.append(name);
    }
}

void ServiceMenuShortcutManager::cleanupStaleShortcuts(KXMLGUIClient *client)
{
    const QString file = client->localXMLFile();
    if (file.isEmpty()) {
        return;
    }

    const QString xml = KXMLGUIFactory::readConfigFile(file, client->componentName());
    if (xml.isEmpty()) {
        return;
    }

    QDomDocument doc;
    doc.setContent(xml);
    QDomElement actionPropElement = KXMLGUIFactory::actionPropertiesElement(doc);

    bool modified = false;
    for (QDomElement e = actionPropElement.firstChildElement(); !e.isNull();) {
        QDomElement next = e.nextSiblingElement();
        const QString name = e.attribute(QStringLiteral("name"));
        if (name.startsWith(serviceMenuNamePrefix) && !m_actionNames.contains(name)) {
            actionPropElement.removeChild(e);
            modified = true;
        }
        e = next;
    }

    if (modified) {
        KXMLGUIFactory::saveConfigFile(doc, file, client->componentName());
    }
}