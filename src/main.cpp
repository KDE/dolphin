/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2006 by Stefan Monov <logixoul@gmail.com>               *
 *   Copyright (C) 2015 by Mathieu Tarral <mathieu.tarral@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "dbusinterface.h"
#include "dolphin_generalsettings.h"
#include "dolphin_version.h"
#include "dolphindebug.h"
#include "dolphinmainwindow.h"
#include "global.h"

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
#include <Kdelibs4ConfigMigrator>

#include <QApplication>
#include <QCommandLineParser>

#ifndef Q_OS_WIN
#include <unistd.h>
#endif
#include <iostream>

extern "C" Q_DECL_EXPORT int kdemain(int argc, char **argv)
{
#ifndef Q_OS_WIN
    // Prohibit using sudo or kdesu (but allow using the root user directly)
    if (getuid() == 0) {
        if (!qEnvironmentVariableIsEmpty("SUDO_USER")) {
            std::cout << "Executing Dolphin with sudo is not possible due to unfixable security vulnerabilities." << std::endl;
            return EXIT_FAILURE;
        } else if (!qEnvironmentVariableIsEmpty("KDESU_USER")) {
            std::cout << "Executing Dolphin with kdesu is not possible due to unfixable security vulnerabilities." << std::endl;
            return EXIT_FAILURE;
        }
    }
#endif

    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("system-file-manager"), app.windowIcon()));

    KCrash::initialize();

    Kdelibs4ConfigMigrator migrate(QStringLiteral("dolphin"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("dolphinrc"));
    migrate.setUiFiles(QStringList() << QStringLiteral("dolphinpart.rc") << QStringLiteral("dolphinui.rc"));
    migrate.migrate();

    KLocalizedString::setApplicationDomain("dolphin");

    KAboutData aboutData(QStringLiteral("dolphin"), i18n("Dolphin"), QStringLiteral(DOLPHIN_VERSION_STRING),
                         i18nc("@title", "File Manager"),
                         KAboutLicense::GPL,
                         i18nc("@info:credit", "(C) 2006-2016 Peter Penz, Frank Reininghaus, and Emmanuel Pescosta"));
    aboutData.setHomepage(QStringLiteral("http://dolphin.kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Emmanuel Pescosta"),
                        i18nc("@info:credit", "Maintainer (since 2014) and developer"),
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

    KDBusService dolphinDBusService;
    DBusInterface interface;

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);

    // command line options
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("select"), i18nc("@info:shell", "The files and folders passed as arguments "
                                                                                        "will be selected.")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("split"), i18nc("@info:shell", "Dolphin will get started with a split view.")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("daemon"), i18nc("@info:shell", "Start Dolphin Daemon (only required for DBus Interface)")));
    parser.addPositionalArgument(QStringLiteral("+[Url]"), i18nc("@info:shell", "Document to open"));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (parser.isSet(QStringLiteral("daemon"))) {
        return app.exec();
    }

    const QStringList args = parser.positionalArguments();
    QList<QUrl> urls = Dolphin::validateUris(args);

    if (urls.isEmpty()) {
        // We need at least one URL to open Dolphin
        urls.append(Dolphin::homeUrl());
    }

    const bool splitView = parser.isSet(QStringLiteral("split")) || GeneralSettings::splitView();
    if (splitView && urls.size() < 2) {
        // Split view does only make sense if we have at least 2 URLs
        urls.append(urls.last());
    }

    DolphinMainWindow* mainWindow = new DolphinMainWindow();
    mainWindow->setAttribute(Qt::WA_DeleteOnClose);

    if (parser.isSet(QStringLiteral("select"))) {
        mainWindow->openFiles(urls, splitView);
    } else {
        mainWindow->openDirectories(urls, splitView);
    }

    mainWindow->show();

    if (app.isSessionRestored()) {
        const QString className = KXmlGuiWindow::classNameOfToplevel(1);
        if (className == QLatin1String("DolphinMainWindow")) {
            mainWindow->restore(1);
        } else {
           qCWarning(DolphinDebug) << "Unknown class " << className << " in session saved data!";
        }
    }

    return app.exec(); // krazy:exclude=crash;
}
