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

#include "dbusinterface.h"
#include "global.h"
#include "dolphin_generalsettings.h"

#include <KPropertiesDialog>

#include <QApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusConnectionInterface>

DBusInterface::DBusInterface() :
    QObject()
{
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/freedesktop/FileManager1"), this,
            QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);
    QDBusConnection::sessionBus().interface()->registerService(QStringLiteral("org.freedesktop.FileManager1"),
                                                               QDBusConnectionInterface::QueueService);
}

void DBusInterface::ShowFolders(const QStringList& uriList, const QString& startUpId)
{
    Q_UNUSED(startUpId);
    const QList<QUrl> urls = Dolphin::validateUris(uriList);
    if (urls.isEmpty()) {
        return;
    }
    const auto serviceName = QStringLiteral("org.kde.dolphin-%1").arg(QCoreApplication::applicationPid());
    if(!Dolphin::attachToExistingInstance(urls, false, GeneralSettings::splitView(), serviceName)) {
        Dolphin::openNewWindow(urls);
    }
}

void DBusInterface::ShowItems(const QStringList& uriList, const QString& startUpId)
{
    Q_UNUSED(startUpId);
    const QList<QUrl> urls = Dolphin::validateUris(uriList);
    if (urls.isEmpty()) {
        return;
    }
    const auto serviceName = QStringLiteral("org.kde.dolphin-%1").arg(QCoreApplication::applicationPid());
    if(!Dolphin::attachToExistingInstance(urls, true, GeneralSettings::splitView(), serviceName)) {
        Dolphin::openNewWindow(urls);
    };
}

void DBusInterface::ShowItemProperties(const QStringList& uriList, const QString& startUpId)
{
    Q_UNUSED(startUpId);
    const QList<QUrl> urls = Dolphin::validateUris(uriList);
    if (!urls.isEmpty()) {
        KPropertiesDialog::showDialog(urls);
    }
}
