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

#include <QDBusConnection>
#include <QList>
#include <QUrl>
#include <KPropertiesDialog>
#include <KRun>

DBusInterface::DBusInterface() :
    QObject()
{
    QDBusConnection::sessionBus().registerService("org.freedesktop.FileManager1");
    QDBusConnection::sessionBus().registerObject("/org/freedesktop/FileManager1", this,
            QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);
}

void DBusInterface::ShowFolders(const QStringList& uriList, const QString& startUpId)
{
    Q_UNUSED(startUpId);
    const QList<QUrl> urls = Dolphin::validateUris(uriList);
    if (urls.isEmpty()) {
        return;
    }
    KRun::run("dolphin %u", urls, nullptr);
}

void DBusInterface::ShowItems(const QStringList& uriList, const QString& startUpId)
{
    Q_UNUSED(startUpId);
    const QList<QUrl> urls = Dolphin::validateUris(uriList);
    if (urls.isEmpty()) {
        return;
    }
    KRun::run("dolphin --select %u", urls, nullptr);
}

void DBusInterface::ShowItemProperties(const QStringList& uriList, const QString& startUpId)
{
    Q_UNUSED(startUpId);
    const QList<QUrl> urls = Dolphin::validateUris(uriList);
    if (!urls.isEmpty()) {
        KPropertiesDialog::showDialog(urls);
    }
}
