/*
 * SPDX-FileCopyrightText: 2009 Rahman Duran <rahman.duran@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * This code is largely based on the kremoteencodingplugin
 * SPDX-FileCopyrightText: 2003 Thiago Macieira <thiago.macieira@kdemail.net>
 * Distributed under the same terms.
 */

#include "dolphinremoteencoding.h"

#include "dolphindebug.h"
#include "dolphinviewactionhandler.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KCharsets>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KProtocolInfo>
#include <KProtocolManager>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QMenu>

#define DATA_KEY QStringLiteral("Charset")

DolphinRemoteEncoding::DolphinRemoteEncoding(QObject *parent, DolphinViewActionHandler *actionHandler)
    : QObject(parent)
    , m_actionHandler(actionHandler)
    , m_loaded(false)
    , m_idDefault(0)
{
    m_menu = new KActionMenu(QIcon::fromTheme(QStringLiteral("character-set")), i18n("Select Remote Charset"), this);
    m_actionHandler->actionCollection()->addAction(QStringLiteral("change_remote_encoding"), m_menu);
    connect(m_menu->menu(), &QMenu::aboutToShow, this, &DolphinRemoteEncoding::slotAboutToShow);

    m_menu->setEnabled(false);
    m_menu->setPopupMode(QToolButton::InstantPopup);
}

DolphinRemoteEncoding::~DolphinRemoteEncoding()
{
}

void DolphinRemoteEncoding::slotReload()
{
    loadSettings();
}

void DolphinRemoteEncoding::loadSettings()
{
    m_loaded = true;
    m_encodingDescriptions = KCharsets::charsets()->descriptiveEncodingNames();

    fillMenu();
}

void DolphinRemoteEncoding::slotAboutToOpenUrl()
{
    QUrl oldURL = m_currentURL;
    m_currentURL = m_actionHandler->currentView()->url();

    if (m_currentURL.scheme() != oldURL.scheme()) {
        // This plugin works on ftp, fish, etc.
        // everything whose type is T_FILESYSTEM except for local files
        if (!m_currentURL.isLocalFile() && KProtocolManager::outputType(m_currentURL) == KProtocolInfo::T_FILESYSTEM) {
            m_menu->setEnabled(true);
            loadSettings();
        } else {
            m_menu->setEnabled(false);
        }
        return;
    }

    if (m_currentURL.host() != oldURL.host()) {
        updateMenu();
    }
}

void DolphinRemoteEncoding::fillMenu()
{
    QMenu *menu = m_menu->menu();
    menu->clear();

    menu->addAction(i18n("Default"), this, &DolphinRemoteEncoding::slotDefault)->setCheckable(true);
    for (int i = 0; i < m_encodingDescriptions.size(); i++) {
        QAction *action = new QAction(m_encodingDescriptions.at(i), this);
        action->setCheckable(true);
        action->setData(i);
        menu->addAction(action);
    }
    menu->addSeparator();

    menu->addAction(i18n("Reload"), this, &DolphinRemoteEncoding::slotReload);
    m_idDefault = m_encodingDescriptions.size() + 2;

    connect(menu, &QMenu::triggered, this, &DolphinRemoteEncoding::slotItemSelected);
}

void DolphinRemoteEncoding::updateMenu()
{
    if (!m_loaded) {
        loadSettings();
    }

    // uncheck everything
    for (int i = 0; i < m_menu->menu()->actions().count(); i++) {
        m_menu->menu()->actions().at(i)->setChecked(false);
    }

    const QString charset = KCharsets::charsets()->descriptionForEncoding(KProtocolManager::charsetFor(m_currentURL));
    if (!charset.isEmpty()) {
        int id = 0;
        bool isFound = false;
        for (int i = 0; i < m_encodingDescriptions.size(); i++) {
            if (m_encodingDescriptions.at(i) == charset) {
                isFound = true;
                id = i;
                break;
            }
        }

        qCDebug(DolphinDebug) << "URL=" << m_currentURL << " charset=" << charset;

        if (!isFound) {
            qCWarning(DolphinDebug) << "could not find entry for charset=" << charset;
        } else {
            m_menu->menu()->actions().at(id)->setChecked(true);
        }
    } else {
        m_menu->menu()->actions().at(m_idDefault)->setChecked(true);
    }
}

void DolphinRemoteEncoding::slotAboutToShow()
{
    if (!m_loaded) {
        loadSettings();
    }
    updateMenu();
}

void DolphinRemoteEncoding::slotItemSelected(QAction *action)
{
    if (action) {
        int id = action->data().toInt();

        KConfig config(("kio_" + m_currentURL.scheme() + "rc").toLatin1());
        QString host = m_currentURL.host();
        if (m_menu->menu()->actions().at(id)->isChecked()) {
            QString charset = KCharsets::charsets()->encodingForName(m_encodingDescriptions.at(id));
            KConfigGroup cg(&config, host);
            cg.writeEntry(DATA_KEY, charset);
            config.sync();

            // Update the io-slaves...
            updateView();
        }
    }
}

void DolphinRemoteEncoding::slotDefault()
{
    // We have no choice but delete all higher domain level
    // settings here since it affects what will be matched.
    KConfig config(("kio_" + m_currentURL.scheme() + "rc").toLatin1());

    QStringList partList = m_currentURL.host().split('.', Qt::SkipEmptyParts);
    if (!partList.isEmpty()) {
        partList.erase(partList.begin());

        QStringList domains;
        // Remove the exact name match...
        domains << m_currentURL.host();

        while (!partList.isEmpty()) {
            if (partList.count() == 2) {
                if (partList[0].length() <= 2 && partList[1].length() == 2) {
                    break;
                }
            }

            if (partList.count() == 1) {
                break;
            }

            domains << partList.join(QLatin1Char('.'));
            partList.erase(partList.begin());
        }

        for (QStringList::const_iterator it = domains.constBegin(); it != domains.constEnd(); ++it) {
            qCDebug(DolphinDebug) << "Domain to remove: " << *it;
            if (config.hasGroup(*it)) {
                config.deleteGroup(*it);
            } else if (config.group("").hasKey(*it)) {
                config.group("").deleteEntry(*it); //don't know what group name is supposed to be XXX
            }
        }
    }
    config.sync();

    // Update the io-slaves.
    updateView();
}

void DolphinRemoteEncoding::updateView()
{
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KIO/Scheduler"), QStringLiteral("org.kde.KIO.Scheduler"), QStringLiteral("reparseSlaveConfiguration"));
    message << QString();
    QDBusConnection::sessionBus().send(message);

    // Reload the page with the new charset
    m_actionHandler->currentView()->setUrl(m_currentURL);
    m_actionHandler->currentView()->reload();
}

#include "moc_dolphinremoteencoding.cpp"
