/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2006 Stefan Monov <logixoul@gmail.com>
 * SPDX-FileCopyrightText: 2015 Mathieu Tarral <mathieu.tarral@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dbusinterface.h"
#include "dolphin_generalsettings.h"
#include "dolphin_version.h"
#include "dolphindebug.h"
#include "dolphinmainwindow.h"
#include "global.h"
#include "config-dolphin.h"
#if HAVE_KUSERFEEDBACK
#include "userfeedback/dolphinfeedbackprovider.h"
#endif

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
#include <KConfigGui>
#include <KIO/PreviewJob>
#include <KWindowSystem>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <Kdelibs4ConfigMigrator>
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusAbstractInterface>
#include <QDBusConnectionInterface>
#include <QSessionManager>

#ifndef Q_OS_WIN
#include <unistd.h>
#endif
#include <iostream>

int main(int argc, char **argv)
{
#ifndef Q_OS_WIN
    // Prohibit using sudo or kdesu (but allow using the root user directly)
    if (getuid() == 0) {
        if (!qEnvironmentVariableIsEmpty("SUDO_USER")) {
            std::cout << "Running Dolphin with sudo can cause bugs and expose you to security vulnerabilities. "
                         "Instead use Dolphin normally and you will be prompted for elevated privileges when "
                         "performing file operations that require them."
                      << std::endl;
            return EXIT_FAILURE;
        } else if (!qEnvironmentVariableIsEmpty("KDESU_USER")) {
            std::cout << "Running Dolphin with kdesu can cause bugs and expose you to security vulnerabilities. "
                         "Instead use Dolphin normally and you will be prompted for elevated privileges when "
                         "performing file operations that require them."
                      << std::endl;
            return EXIT_FAILURE;
        }
    }
#endif

    /**
     * enable high dpi support
     */
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
#endif
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("system-file-manager"), app.windowIcon()));

    KIO::PreviewJob::setDefaultDevicePixelRatio(app.devicePixelRatio());

    KCrash::initialize();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Kdelibs4ConfigMigrator migrate(QStringLiteral("dolphin"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("dolphinrc"));
    migrate.setUiFiles(QStringList() << QStringLiteral("dolphinpart.rc") << QStringLiteral("dolphinui.rc"));
    migrate.migrate();
#endif

    KLocalizedString::setApplicationDomain("dolphin");

    KAboutData aboutData(QStringLiteral("dolphin"), i18n("Dolphin"), QStringLiteral(DOLPHIN_VERSION_STRING),
                         i18nc("@title", "File Manager"),
                         KAboutLicense::GPL,
                         i18nc("@info:credit", "(C) 2006-2018 Peter Penz, Frank Reininghaus, Emmanuel Pescosta and Elvis Angelaccio"));
    aboutData.setHomepage(QStringLiteral("https://kde.org/applications/system/org.kde.dolphin"));
    aboutData.addAuthor(i18nc("@info:credit", "Elvis Angelaccio"),
                        i18nc("@info:credit", "Maintainer (since 2018) and developer"),
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
    aboutData.addAuthor(i18nc("@info:credit", "Sebastian Trüg"),
                        i18nc("@info:credit", "Developer"),
                        QStringLiteral("trueg@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "David Faure"),
                        i18nc("@info:credit", "Developer"),
                        QStringLiteral("faure@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Aaron J. Seigo"),
                        i18nc("@info:credit", "Developer"),
                        QStringLiteral("aseigo@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Rafael Fernández López"),
                        i18nc("@info:credit", "Developer"),
                        QStringLiteral("ereslibre@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Kevin Ottens"),
                        i18nc("@info:credit", "Developer"),
                        QStringLiteral("ervin@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Holger Freyther"),
                        i18nc("@info:credit", "Developer"),
                        QStringLiteral("freyther@gmx.net"));
    aboutData.addAuthor(i18nc("@info:credit", "Max Blazejak"),
                        i18nc("@info:credit", "Developer"),
                        QStringLiteral("m43ksrocks@gmail.com"));
    aboutData.addAuthor(i18nc("@info:credit", "Michael Austin"),
                        i18nc("@info:credit", "Documentation"),
                        QStringLiteral("tuxedup@users.sourceforge.net"));

    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    // command line options
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("select"), i18nc("@info:shell", "The files and folders passed as arguments "
                                                                                        "will be selected.")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("split"), i18nc("@info:shell", "Dolphin will get started with a split view.")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("new-window"), i18nc("@info:shell", "Dolphin will explicitly open in a new window.")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("daemon"), i18nc("@info:shell", "Start Dolphin Daemon (only required for DBus Interface)")));
    parser.addPositionalArgument(QStringLiteral("+[Url]"), i18nc("@info:shell", "Document to open"));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    const bool splitView = parser.isSet(QStringLiteral("split")) || GeneralSettings::splitView();
    const bool openFiles = parser.isSet(QStringLiteral("select"));
    const QStringList args = parser.positionalArguments();
    QList<QUrl> urls = Dolphin::validateUris(args);
    // We later mutate urls, so we need to store if it was empty originally
    const bool startedWithURLs = !urls.isEmpty();


    if (parser.isSet(QStringLiteral("daemon"))) {
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

    DolphinMainWindow* mainWindow = new DolphinMainWindow();

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
    if (!parser.isSet(QStringLiteral("new-window"))
        && (app.isSessionRestored() || GeneralSettings::rememberOpenedTabs())
    ) {
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

#if HAVE_KUSERFEEDBACK
    auto feedbackProvider = DolphinFeedbackProvider::instance();
    Q_UNUSED(feedbackProvider)
#endif

    return app.exec(); // krazy:exclude=crash;
}
