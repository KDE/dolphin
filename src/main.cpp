/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2006 Stefan Monov <logixoul@gmail.com>
 * SPDX-FileCopyrightText: 2015 Mathieu Tarral <mathieu.tarral@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "admin/workerintegration.h"
#include "config-dolphin.h"
#include "dbusinterface.h"
#include "dolphin_generalsettings.h"
#include "dolphin_version.h"
#include "dolphindebug.h"
#include "dolphinmainwindow.h"
#include "global.h"
#if HAVE_KUSERFEEDBACK
#include "userfeedback/dolphinfeedbackprovider.h"
#endif

#include <KAboutData>
#include <KConfigGui>
#include <KCrash>
#include <KDBusService>
#include <KIO/PreviewJob>
#include <KIconTheme>
#include <KLocalizedString>
#include <KWindowSystem>

#define HAVE_STYLE_MANAGER __has_include(<KStyleManager>)
#if HAVE_STYLE_MANAGER
#include <KStyleManager>
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QSessionManager>

#if HAVE_X11
#include <private/qtx11extras_p.h>
#endif

#ifndef Q_OS_WIN
#include <unistd.h>
#endif
#include <iostream>

constexpr auto dolphinTranslationDomain{"dolphin"};

int main(int argc, char **argv)
{
#ifndef Q_OS_WIN
    // Prohibit using sudo or kdesu (but allow using the root user directly)
    if (getuid() == 0 && (!qEnvironmentVariableIsEmpty("SUDO_USER") || !qEnvironmentVariableIsEmpty("KDESU_USER"))) {
        QCoreApplication app(argc, argv); // Needed for the xi18ndc() call below.
        std::cout << qPrintable(
            xi18ndc(dolphinTranslationDomain,
                    "@info:shell %1 is a terminal command",
                    "Running <application>Dolphin</application> with <command>sudo</command> is discouraged. Please run <icode>%1</icode> instead.",
                    QStringLiteral("dolphin --sudo")))
                  << std::endl;
        // We could perform a privilege de-escalation here and continue as normal. It is a bit safer though to simply let the user restart without sudo.
        return EXIT_FAILURE;
    }
#endif

    /**
     * trigger initialisation of proper icon theme
     */
#if KICONTHEMES_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    KIconTheme::initTheme();
#endif

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("system-file-manager"), app.windowIcon()));

#if HAVE_STYLE_MANAGER
    /**
     * trigger initialisation of proper application style
     */
    KStyleManager::initStyle();
#else
    /**
     * For Windows and macOS: use Breeze if available
     * Of all tested styles that works the best for us
     */
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    QApplication::setStyle(QStringLiteral("breeze"));
#endif
#endif

    KLocalizedString::setApplicationDomain(dolphinTranslationDomain);

    KAboutData aboutData(QStringLiteral("dolphin"),
                         i18n("Dolphin"),
                         QStringLiteral(DOLPHIN_VERSION_STRING),
                         i18nc("@title", "File Manager"),
                         KAboutLicense::GPL,
                         i18nc("@info:credit", "(C) 2006-2022 The Dolphin Developers"));
    aboutData.setHomepage(QStringLiteral("https://apps.kde.org/dolphin"));
    aboutData.addAuthor(i18nc("@info:credit", "Felix Ernst"),
                        i18nc("@info:credit", "Maintainer (since 2021) and developer"),
                        QStringLiteral("felixernst@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Méven Car"),
                        i18nc("@info:credit", "Maintainer (since 2021) and developer (since 2019)"),
                        QStringLiteral("meven@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Elvis Angelaccio"),
                        i18nc("@info:credit", "Maintainer (2018-2021) and developer"),
                        QStringLiteral("elvis.angelaccio@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Emmanuel Pescosta"),
                        i18nc("@info:credit", "Maintainer (2014-2018) and developer"),
                        QStringLiteral("emmanuelpescosta099@gmail.com"));
    aboutData.addAuthor(i18nc("@info:credit", "Frank Reininghaus"),
                        i18nc("@info:credit", "Maintainer (2012-2014) and developer"),
                        QStringLiteral("frank78ac@googlemail.com"));
    aboutData.addAuthor(i18nc("@info:credit", "Peter Penz"),
                        i18nc("@info:credit", "Maintainer and developer (2006-2012)"),
                        QStringLiteral("peter.penz19@gmail.com"));
    aboutData.addAuthor(i18nc("@info:credit", "Sebastian Trüg"), i18nc("@info:credit", "Developer"), QStringLiteral("trueg@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "David Faure"), i18nc("@info:credit", "Developer"), QStringLiteral("faure@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Aaron J. Seigo"), i18nc("@info:credit", "Developer"), QStringLiteral("aseigo@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Rafael Fernández López"), i18nc("@info:credit", "Developer"), QStringLiteral("ereslibre@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Kevin Ottens"), i18nc("@info:credit", "Developer"), QStringLiteral("ervin@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Holger Freyther"), i18nc("@info:credit", "Developer"), QStringLiteral("freyther@gmx.net"));
    aboutData.addAuthor(i18nc("@info:credit", "Max Blazejak"), i18nc("@info:credit", "Developer"), QStringLiteral("m43ksrocks@gmail.com"));
    aboutData.addAuthor(i18nc("@info:credit", "Michael Austin"), i18nc("@info:credit", "Documentation"), QStringLiteral("tuxedup@users.sourceforge.net"));

    KAboutData::setApplicationData(aboutData);
    KCrash::initialize();

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    // command line options
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("select"),
                                        i18nc("@info:shell",
                                              "The files and folders passed as arguments "
                                              "will be selected.")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("split"), i18nc("@info:shell", "Dolphin will get started with a split view.")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("new-window"), i18nc("@info:shell", "Dolphin will explicitly open in a new window.")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("sudo") << QStringLiteral("admin"),
                                        i18nc("@info:shell", "Set up Dolphin for administrative tasks.")));
    parser.addOption(
        QCommandLineOption(QStringList() << QStringLiteral("daemon"), i18nc("@info:shell", "Start Dolphin Daemon (only required for DBus Interface).")));
    parser.addPositionalArgument(QStringLiteral("+[Url]"), i18nc("@info:shell", "Document to open"));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    const bool splitView = parser.isSet(QStringLiteral("split")) || GeneralSettings::splitView();
    const bool openFiles = parser.isSet(QStringLiteral("select"));
    const bool adminWorkerInfoWanted = parser.isSet(QStringLiteral("sudo")) || parser.isSet(QStringLiteral("admin"));
    const QStringList args = parser.positionalArguments();
    QList<QUrl> urls = Dolphin::validateUris(args);
    // We later mutate urls, so we need to store if it was empty originally
    const bool startedWithURLs = !urls.isEmpty();

    if (adminWorkerInfoWanted || std::any_of(urls.cbegin(), urls.cend(), [](const QUrl &url) {
            return url.scheme() == QStringLiteral("admin");
        })) {
        Admin::guideUserTowardsInstallingAdminWorker();
    }

    if (parser.isSet(QStringLiteral("daemon"))) {
        // Prevent KApplicationLauncherJob from cause the application to quit on job finish.
        QCoreApplication::setQuitLockEnabled(false);

        // Disable session management for the daemonized version
        // See https://bugs.kde.org/show_bug.cgi?id=417219
        auto disableSessionManagement = [](QSessionManager &sm) {
            sm.setRestartHint(QSessionManager::RestartNever);
        };
        QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
        QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

#ifdef FLATPAK
        KDBusService dolphinDBusService(KDBusService::NoExitOnFailure);
#else
        KDBusService dolphinDBusService;
#endif
        DBusInterface interface;
        interface.setAsDaemon();
        return app.exec();
    }

    if (!parser.isSet(QStringLiteral("new-window"))) {
        QString token;
        if (KWindowSystem::isPlatformWayland()) {
            token = qEnvironmentVariable("XDG_ACTIVATION_TOKEN");
            qunsetenv("XDG_ACTIVATION_TOKEN");
        } else if (KWindowSystem::isPlatformX11()) {
#if HAVE_X11
            token = QX11Info::nextStartupId();
#endif
        }

        if (Dolphin::attachToExistingInstance(urls, openFiles, splitView, QString(), token)) {
            // Successfully attached to existing instance of Dolphin
            return 0;
        }
    }

    if (!startedWithURLs) {
        // We need at least one URL to open Dolphin
        urls.append(Dolphin::homeUrl());
    }

    if (splitView && urls.size() < 2) {
        // Split view does only make sense if we have at least 2 URLs
        urls.append(urls.last());
    }

    DolphinMainWindow *mainWindow = new DolphinMainWindow();

    if (openFiles) {
        mainWindow->openFiles(urls, splitView);
    } else {
        mainWindow->openDirectories(urls, splitView);
    }

    mainWindow->show();

    // Allow starting Dolphin on a system that is not running DBus:
    KDBusService::StartupOptions serviceOptions = KDBusService::Multiple;
    if (!QDBusConnection::sessionBus().isConnected()) {
        serviceOptions |= KDBusService::NoExitOnFailure;
    }
    KDBusService dolphinDBusService(serviceOptions);
    DBusInterface interface;

    if (!app.isSessionRestored()) {
        KConfigGui::setSessionConfig(QStringLiteral("dolphin"), QStringLiteral("dolphin"));
    }

    // Only restore session if:
    // 1. Not explicitly opening a new instance
    // 2. The "remember state" setting is enabled or session restoration after
    //    reboot is in use
    // 3. There is a session available to restore
    if (!parser.isSet(QStringLiteral("new-window")) && (app.isSessionRestored() || GeneralSettings::rememberOpenedTabs())) {
        // Get saved state data for the last-closed Dolphin instance
        const QString serviceName = QStringLiteral("org.kde.dolphin-%1").arg(QCoreApplication::applicationPid());
        if (Dolphin::dolphinGuiInstances(serviceName).size() > 0) {
            const QString className = KXmlGuiWindow::classNameOfToplevel(1);
            if (className == QLatin1String("DolphinMainWindow")) {
                mainWindow->restore(1);
                // If the user passed any URLs to Dolphin, open those in the
                // window after session-restoring it
                if (startedWithURLs) {
                    if (openFiles) {
                        mainWindow->openFiles(urls, splitView);
                    } else {
                        mainWindow->openDirectories(urls, splitView);
                    }
                }
            } else {
                qCWarning(DolphinDebug) << "Unknown class " << className << " in session saved data!";
            }
        }
    }

    mainWindow->setSessionAutoSaveEnabled(GeneralSettings::rememberOpenedTabs());

    if (adminWorkerInfoWanted) {
        Admin::guideUserTowardsUsingAdminWorker();
    }

#if HAVE_KUSERFEEDBACK
    auto feedbackProvider = DolphinFeedbackProvider::instance();
    Q_UNUSED(feedbackProvider)
#endif

    return app.exec(); // krazy:exclude=crash;
}
