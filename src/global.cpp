/*
 * SPDX-FileCopyrightText: 2015 Ashish Bansal <bansal.ashish096@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "global.h"

#include "dolphin_generalsettings.h"
#include "dolphindebug.h"
#include "dolphinmainwindowinterface.h"
#include "views/viewproperties.h"

#include <KConfigWatcher>
#include <KDialogJobUiDelegate>
#include <KIO/ApplicationLauncherJob>
#include <KService>
#include <KWindowSystem>

#include <QApplication>
#include <QIcon>

QList<QUrl> Dolphin::validateUris(const QStringList& uriList)
{
    const QString currentDir = QDir::currentPath();
    QList<QUrl> urls;
    for (const QString& str : uriList) {
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
    KService::Ptr service(new KService(QApplication::applicationDisplayName(), command, QApplication::windowIcon().name()));
    auto *job = new KIO::ApplicationLauncherJob(service, window);
    job->setUrls(urls);
    job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, window));
    job->start();
}

bool Dolphin::attachToExistingInstance(const QList<QUrl>& inputUrls, bool openFiles, bool splitView, const QString& preferredService)
{
    bool attached = false;

    // TODO: once Wayland clients can raise or activate themselves remove check from conditional
    if (inputUrls.isEmpty() || !GeneralSettings::openExternallyCalledFolderInNewTab()) {
        return false;
    }

    auto dolphinInterfaces = dolphinGuiInstances(preferredService);
    if (dolphinInterfaces.isEmpty()) {
        return false;
    }

    QStringList newUrls;

    // check to see if any instances already have any of the given URLs open
    const auto urls = QUrl::toStringList(inputUrls);
    for (const QString& url : urls) {
        bool urlFound = false;
        for (auto& interface: dolphinInterfaces) {
            auto isUrlOpenReply = interface.first->isUrlOpen(url);
            isUrlOpenReply.waitForFinished();
            if (!isUrlOpenReply.isError() && isUrlOpenReply.value()) {
                interface.second.append(url);
                urlFound = true;
                break;
            }
        }
        if (!urlFound) {
            newUrls.append(url);
        }
    }

    for (const auto& interface: qAsConst(dolphinInterfaces)) {
        auto reply = openFiles ? interface.first->openFiles(newUrls, splitView) : interface.first->openDirectories(newUrls, splitView);
        reply.waitForFinished();
        if (!reply.isError()) {
            interface.first->activateWindow();
            attached = true;
            break;
        }
    }
    return attached;
}

QVector<QPair<QSharedPointer<OrgKdeDolphinMainWindowInterface>, QStringList>> Dolphin::dolphinGuiInstances(const QString& preferredService)
{
    QVector<QPair<QSharedPointer<OrgKdeDolphinMainWindowInterface>, QStringList>> dolphinInterfaces;
    if (!preferredService.isEmpty()) {
        QSharedPointer<OrgKdeDolphinMainWindowInterface> preferredInterface(
            new OrgKdeDolphinMainWindowInterface(preferredService,
                QStringLiteral("/dolphin/Dolphin_1"),
                QDBusConnection::sessionBus()));
        if (preferredInterface->isValid() && !preferredInterface->lastError().isValid()) {
            dolphinInterfaces.append(qMakePair(preferredInterface, QStringList()));
        }
    }

    // Look for dolphin instances among all available dbus services.
    QDBusConnectionInterface *sessionInterface = QDBusConnection::sessionBus().interface();
    const QStringList dbusServices = sessionInterface ? sessionInterface->registeredServiceNames().value() : QStringList();
    // Don't match the service without trailing "-" (unique instance)
    const QString pattern = QStringLiteral("org.kde.dolphin-");
    // Don't match the pid without leading "-"
    const QString myPid = QLatin1Char('-') + QString::number(QCoreApplication::applicationPid());
    for (const QString& service : dbusServices) {
        if (service.startsWith(pattern) && !service.endsWith(myPid)) {
            // Check if instance can handle our URLs
            QSharedPointer<OrgKdeDolphinMainWindowInterface> interface(
                        new OrgKdeDolphinMainWindowInterface(service,
                            QStringLiteral("/dolphin/Dolphin_1"),
                            QDBusConnection::sessionBus()));
            if (interface->isValid() && !interface->lastError().isValid()) {
                dolphinInterfaces.append(qMakePair(interface, QStringList()));
            }
        }
    }

    return dolphinInterfaces;
}

QPair<QString, Qt::SortOrder> Dolphin::sortOrderForUrl(QUrl &url)
{
    ViewProperties globalProps(url);
    return QPair<QString, Qt::SortOrder>(globalProps.sortRole(), globalProps.sortOrder());
}

double GlobalConfig::animationDurationFactor()
{
    if (s_animationDurationFactor >= 0.0) {
        return s_animationDurationFactor;
    }
    // This is the first time this method is called.
    auto kdeGlobalsConfig = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("KDE"));
    updateAnimationDurationFactor(kdeGlobalsConfig, {"AnimationDurationFactor"});

    KConfigWatcher::Ptr configWatcher = KConfigWatcher::create(KSharedConfig::openConfig());
    connect(configWatcher.data(), &KConfigWatcher::configChanged,
            &GlobalConfig::updateAnimationDurationFactor);
    return s_animationDurationFactor;
}

void GlobalConfig::updateAnimationDurationFactor(const KConfigGroup &group, const QByteArrayList &names)
{
    if (group.name() == QLatin1String("KDE") &&
        names.contains(QByteArrayLiteral("AnimationDurationFactor"))) {
        s_animationDurationFactor = std::max(0.0,
                group.readEntry("AnimationDurationFactor", 1.0));
    }
}

double GlobalConfig::s_animationDurationFactor = -1.0;
