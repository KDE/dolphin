/*
 * Copyright 2015 Ashish Bansal<bansal.ashish096@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "global.h"

#include "dolphin_generalsettings.h"
#include "dolphindebug.h"

#include <KRun>
#include <KWindowSystem>

#include <QApplication>
#include <QIcon>
#include <QDBusInterface>
#include <QDBusConnectionInterface>

QList<QUrl> Dolphin::validateUris(const QStringList& uriList)
{
    const QString currentDir = QDir::currentPath();
    QList<QUrl> urls;
    foreach (const QString& str, uriList) {
        const QUrl url = QUrl::fromUserInput(str, currentDir, QUrl::AssumeLocalFile);
        if (url.isValid()) {
            urls.append(url);
        } else {
            qCWarning(DolphinDebug) << "Invalid URL: " << str;
        }
    }
    return urls;
}

QUrl Dolphin::homeUrl()
{
    return QUrl::fromUserInput(GeneralSettings::homeUrl(), QString(), QUrl::AssumeLocalFile);
}

void Dolphin::openNewWindow(const QList<QUrl> &urls, QWidget *window, const OpenNewWindowFlags &flags)
{
    QString command = QStringLiteral("dolphin --new-window");

    if (flags.testFlag(OpenNewWindowFlag::Select)) {
        command.append(QLatin1String(" --select"));
    }

    if (!urls.isEmpty()) {
        command.append(QLatin1String(" %U"));
    }
    KRun::run(
        command,
        urls,
        window,
        QApplication::applicationDisplayName(),
        QApplication::windowIcon().name()
    );
}

bool Dolphin::attachToExistingInstance(const QList<QUrl>& inputUrls, bool openFiles, bool splitView, const QString& preferredService)
{
    // TODO: once Wayland clients can raise or activate themselves remove check from conditional
    if (KWindowSystem::isPlatformWayland() || inputUrls.isEmpty() || !GeneralSettings::openExternallyCalledFolderInNewTab()) {
        return false;
    }

    QVector<QPair<QSharedPointer<QDBusInterface>, QStringList>> dolphinInterfaces;
    if (!preferredService.isEmpty()) {
        QSharedPointer<QDBusInterface> preferredInterface(
            new QDBusInterface(preferredService,
            QStringLiteral("/dolphin/Dolphin_1"),
            QString()) // #414402: use empty interface name to prevent QtDBus from caching the interface.
        );
        if (preferredInterface->isValid() && !preferredInterface->lastError().isValid()) {
            dolphinInterfaces.append(qMakePair(preferredInterface, QStringList()));
        }
    }

    // Look for dolphin instances among all available dbus services.
    const QStringList dbusServices = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
    // Don't match the service without trailing "-" (unique instance)
    const QString pattern = QStringLiteral("org.kde.dolphin-");
    // Don't match the pid without leading "-"
    const QString myPid = QLatin1Char('-') + QString::number(QCoreApplication::applicationPid());
    for (const QString& service : dbusServices) {
        if (service.startsWith(pattern) && !service.endsWith(myPid)) {
            // Check if instance can handle our URLs
            QSharedPointer<QDBusInterface> interface(
                new QDBusInterface(service,
                QStringLiteral("/dolphin/Dolphin_1"),
                QStringLiteral("org.kde.dolphin.MainWindow"))
            );
            if (interface->isValid() && !interface->lastError().isValid()) {
                dolphinInterfaces.append(qMakePair(interface, QStringList()));
            }
        }
    }

    if (dolphinInterfaces.isEmpty()) {
        return false;
    }

    QStringList newUrls;

    // check to see if any instances already have any of the given URLs open
    const auto urls = QUrl::toStringList(inputUrls);
    for (const QString& url : urls) {
        bool urlFound = false;
        for (auto& interface: dolphinInterfaces) {
            QDBusReply<bool> isUrlOpenReply = interface.first->call(QStringLiteral("isUrlOpen"), url);
            if (isUrlOpenReply.isValid() && isUrlOpenReply.value()) {
                interface.second.append(url);
                urlFound = true;
                break;
            }
        }
        if (!urlFound) {
            newUrls.append(url);
        }
    }
    dolphinInterfaces.front().second << newUrls;

    for (const auto& interface: dolphinInterfaces) {
        if (!interface.second.isEmpty()) {
            interface.first->call(openFiles ? QStringLiteral("openFiles") : QStringLiteral("openDirectories"), interface.second, splitView);
            interface.first->call(QStringLiteral("activateWindow"));
        }
    }
    return true;
}
