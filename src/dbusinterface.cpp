/*
 * SPDX-FileCopyrightText: 2015 Ashish Bansal <bansal.ashish096@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
    Q_UNUSED(startUpId)
    const QList<QUrl> urls = Dolphin::validateUris(uriList);
    if (urls.isEmpty()) {
        return;
    }
    const auto serviceName = isDaemon() ? QString() : QStringLiteral("org.kde.dolphin-%1").arg(QCoreApplication::applicationPid());
    if(!Dolphin::attachToExistingInstance(urls, false, GeneralSettings::splitView(), serviceName)) {
        Dolphin::openNewWindow(urls);
    }
}

void DBusInterface::ShowItems(const QStringList& uriList, const QString& startUpId)
{
    Q_UNUSED(startUpId)
    const QList<QUrl> urls = Dolphin::validateUris(uriList);
    if (urls.isEmpty()) {
        return;
    }
    const auto serviceName = isDaemon() ? QString() : QStringLiteral("org.kde.dolphin-%1").arg(QCoreApplication::applicationPid());
    if(!Dolphin::attachToExistingInstance(urls, true, GeneralSettings::splitView(), serviceName)) {
        Dolphin::openNewWindow(urls, nullptr, Dolphin::OpenNewWindowFlag::Select);
    };
}

void DBusInterface::ShowItemProperties(const QStringList& uriList, const QString& startUpId)
{
    Q_UNUSED(startUpId)
    const QList<QUrl> urls = Dolphin::validateUris(uriList);
    if (!urls.isEmpty()) {
        KPropertiesDialog::showDialog(urls);
    }
}

void DBusInterface::setAsDaemon()
{
    m_isDaemon = true;
}

bool DBusInterface::isDaemon() const
{
    return m_isDaemon;
}
