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
#ifdef HAVE_KACTIVITIES
#include <KActivities/Consumer>
#endif

#include <QApplication>

QList<QUrl> Dolphin::validateUris(const QStringList &uriList)
{
    const QString currentDir = QDir::currentPath();
    QList<QUrl> urls;
    for (const QString &str : uriList) {
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

bool Dolphin::attachToExistingInstance(const QList<QUrl> &inputUrls,
                                       bool openFiles,
                                       bool splitView,
                                       const QString &preferredService,
                                       const QString &activationToken)
{
    bool attached = false;

    if (inputUrls.isEmpty()) {
        return false;
    }

    auto dolphinInterfaces = dolphinGuiInstances(preferredService);
    if (dolphinInterfaces.isEmpty()) {
        return false;
    }

    int activeWindowIndex = -1;
    for (const auto &interface : qAsConst(dolphinInterfaces)) {
        ++activeWindowIndex;

        auto isActiveWindowReply = interface.first->isActiveWindow();
        isActiveWindowReply.waitForFinished();
        if (!isActiveWindowReply.isError() && isActiveWindowReply.value()) {
            break;
        }
    }

    // check to see if any instances already have any of the given URLs or their parents open
    QList<QUrl> newWindowURLs;
    for (const QUrl &url : inputUrls) {
        bool urlFound = false;
        const QString urlString = url.toString();

        // looping through the windows starting from the active one
        int i = activeWindowIndex;
        do {
            auto &interface = dolphinInterfaces[i];

            auto isUrlOpenReply = openFiles ? interface.first->isItemVisibleInAnyView(urlString) : interface.first->isUrlOpen(urlString);
            isUrlOpenReply.waitForFinished();
            if (!isUrlOpenReply.isError() && isUrlOpenReply.value()) {
                interface.second.append(urlString);
                urlFound = true;
                break;
            }

            i = (i + 1) % dolphinInterfaces.size();
        } while (i != activeWindowIndex);

        if (!urlFound) {
            if (GeneralSettings::openExternallyCalledFolderInNewTab()) {
                dolphinInterfaces[activeWindowIndex].second.append(urlString);
            } else {
                newWindowURLs.append(url);
            }
        }
    }

    for (const auto &interface : qAsConst(dolphinInterfaces)) {
        if (interface.second.isEmpty()) {
            continue;
        }
        auto reply = openFiles ? interface.first->openFiles(interface.second, splitView) : interface.first->openDirectories(interface.second, splitView);
        reply.waitForFinished();
        if (!reply.isError()) {
            interface.first->activateWindow(activationToken);
            attached = true;
        }
    }
    if (attached && !newWindowURLs.isEmpty()) {
        if (openFiles) {
            openNewWindow(newWindowURLs, nullptr, Dolphin::OpenNewWindowFlag::Select);
        } else {
            openNewWindow(newWindowURLs);
        }
    }

    return attached;
}

QVector<QPair<QSharedPointer<OrgKdeDolphinMainWindowInterface>, QStringList>> Dolphin::dolphinGuiInstances(const QString &preferredService)
{
#ifdef HAVE_KACTIVITIES
    static std::once_flag one_consumer;
    static KActivities::Consumer *consumer;
    std::call_once(one_consumer, []() {
        consumer = new KActivities::Consumer();
        // ensures the consumer is ready for query
        QEventLoop loop;
        QObject::connect(consumer, &KActivities::Consumer::serviceStatusChanged, &loop, &QEventLoop::quit);
        loop.exec();
    });
#endif

    QVector<QPair<QSharedPointer<OrgKdeDolphinMainWindowInterface>, QStringList>> dolphinInterfaces;
    const auto tryAppendInterface = [&dolphinInterfaces](const QString &service) {
        // Check if instance can handle our URLs
        QSharedPointer<OrgKdeDolphinMainWindowInterface> interface(
            new OrgKdeDolphinMainWindowInterface(service, QStringLiteral("/dolphin/Dolphin_1"), QDBusConnection::sessionBus()));
        if (interface->isValid() && !interface->lastError().isValid()) {
#ifdef HAVE_KACTIVITIES
            const auto currentActivity = consumer->currentActivity();
            if (currentActivity.isEmpty() || currentActivity == QStringLiteral("00000000-0000-0000-0000-000000000000")
                || interface->isOnActivity(consumer->currentActivity()))
#endif
                if (interface->isOnCurrentDesktop()) {
                    dolphinInterfaces.append(qMakePair(interface, QStringList()));
                }
        }
    };

    if (!preferredService.isEmpty()) {
        tryAppendInterface(preferredService);
    }

    // Look for dolphin instances among all available dbus services.
    QDBusConnectionInterface *sessionInterface = QDBusConnection::sessionBus().interface();
    const QStringList dbusServices = sessionInterface ? sessionInterface->registeredServiceNames().value() : QStringList();
    // Don't match the service without trailing "-" (unique instance)
    const QString pattern = QStringLiteral("org.kde.dolphin-");
    // Don't match the pid without leading "-"
    const QString myPid = QLatin1Char('-') + QString::number(QCoreApplication::applicationPid());
    for (const QString &service : dbusServices) {
        if (service.startsWith(pattern) && !service.endsWith(myPid)) {
            tryAppendInterface(service);
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
    connect(configWatcher.data(), &KConfigWatcher::configChanged, &GlobalConfig::updateAnimationDurationFactor);
    return s_animationDurationFactor;
}

void GlobalConfig::updateAnimationDurationFactor(const KConfigGroup &group, const QByteArrayList &names)
{
    if (group.name() == QLatin1String("KDE") && names.contains(QByteArrayLiteral("AnimationDurationFactor"))) {
        s_animationDurationFactor = std::max(0.0, group.readEntry("AnimationDurationFactor", 1.0));
    }
}

double GlobalConfig::s_animationDurationFactor = -1.0;
