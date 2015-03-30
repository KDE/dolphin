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

#include "dolphinmainwindow.h"
#include "dolphin_generalsettings.h"

#include <KDBusService>
#include <KAboutData>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QApplication>
#include <KLocalizedString>
#include "dolphindebug.h"
#include <kdelibs4configmigrator.h>

extern "C" Q_DECL_EXPORT int kdemain(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    Kdelibs4ConfigMigrator migrate(QStringLiteral("dolphin"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("dolphinrc"));
    migrate.setUiFiles(QStringList() << QStringLiteral("dolphinpart.rc") << QStringLiteral("dolphinui.rc"));
    migrate.migrate();

    app.setWindowIcon(QIcon::fromTheme("system-file-manager"));

    KAboutData aboutData("dolphin", i18n("Dolphin"), "14.12.95",
                         i18nc("@title", "File Manager"),
                         KAboutLicense::GPL,
                         i18nc("@info:credit", "(C) 2006-2014 Peter Penz, Frank Reininghaus, and Emmanuel Pescosta"));
    aboutData.setHomepage("http://dolphin.kde.org");
    aboutData.addAuthor(i18nc("@info:credit", "Emmanuel Pescosta"),
                        i18nc("@info:credit", "Maintainer (since 2014) and developer"),
                        "emmanuelpescosta099@gmail.com");
    aboutData.addAuthor(i18nc("@info:credit", "Frank Reininghaus"),
                        i18nc("@info:credit", "Maintainer (2012-2014) and developer"),
                        "frank78ac@googlemail.com");
    aboutData.addAuthor(i18nc("@info:credit", "Peter Penz"),
                        i18nc("@info:credit", "Maintainer and developer (2006-2012)"),
                        "peter.penz19@gmail.com");
    aboutData.addAuthor(i18nc("@info:credit", "Sebastian Trüg"),
                        i18nc("@info:credit", "Developer"),
                        "trueg@kde.org");
    aboutData.addAuthor(i18nc("@info:credit", "David Faure"),
                        i18nc("@info:credit", "Developer"),
                        "faure@kde.org");
    aboutData.addAuthor(i18nc("@info:credit", "Aaron J. Seigo"),
                        i18nc("@info:credit", "Developer"),
                        "aseigo@kde.org");
    aboutData.addAuthor(i18nc("@info:credit", "Rafael Fernández López"),
                        i18nc("@info:credit", "Developer"),
                        "ereslibre@kde.org");
    aboutData.addAuthor(i18nc("@info:credit", "Kevin Ottens"),
                        i18nc("@info:credit", "Developer"),
                        "ervin@kde.org");
    aboutData.addAuthor(i18nc("@info:credit", "Holger Freyther"),
                        i18nc("@info:credit", "Developer"),
                        "freyther@gmx.net");
    aboutData.addAuthor(i18nc("@info:credit", "Max Blazejak"),
                        i18nc("@info:credit", "Developer"),
                        "m43ksrocks@gmail.com");
    aboutData.addAuthor(i18nc("@info:credit", "Michael Austin"),
                        i18nc("@info:credit", "Documentation"),
                        "tuxedup@users.sourceforge.net");

    KAboutData::setApplicationData(aboutData);

    KDBusService dolphinDBusService;

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);

    // command line options
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("select"), i18nc("@info:shell", "The files and directories passed as arguments "
                                                                                        "will be selected.")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("split"), i18nc("@info:shell", "Dolphin will get started with a split view.")));
    parser.addPositionalArgument(QLatin1String("+[Url]"), i18nc("@info:shell", "Document to open"));

    parser.process(app);
    aboutData.processCommandLine(&parser);


    DolphinMainWindow* m_mainWindow = new DolphinMainWindow();
    m_mainWindow->setAttribute(Qt::WA_DeleteOnClose);

    QList<QUrl> urls;
    const QStringList args = parser.positionalArguments();
    foreach (const QString& str,  args) {
        const QUrl url = QUrl::fromUserInput(str, QString(), QUrl::AssumeLocalFile);
        if (url.isValid()) {
            urls.append(url);
        } else {
            qCWarning(DolphinDebug) << "Invalid URL: " << str;
        }
    }

    bool resetSplitSettings = false;
    if (parser.isSet("split") && !GeneralSettings::splitView()) {
        // Dolphin should be opened with a split view although this is not
        // set in the GeneralSettings. Temporary adjust the setting until
        // all passed URLs have been opened.
        GeneralSettings::setSplitView(true);
        resetSplitSettings = true;

        // We need 2 URLs to open Dolphin in split view mode
        if (urls.isEmpty()) { // No URL given - Open home URL in all two views
            urls.append(GeneralSettings::homeUrl());
            urls.append(GeneralSettings::homeUrl());
        } else if (urls.length() == 1) { // Only 1 URL given - Open given URL in all two views
            urls.append(urls.at(0));
        }
    }

    if (!urls.isEmpty()) {
        if (parser.isSet("select")) {
            m_mainWindow->openFiles(urls);
        } else {
            m_mainWindow->openDirectories(urls);
        }
    } else {
        const QUrl homeUrl(QUrl::fromLocalFile(GeneralSettings::homeUrl()));
        m_mainWindow->openNewActivatedTab(homeUrl);
    }

    if (resetSplitSettings) {
        GeneralSettings::setSplitView(false);
    }

    m_mainWindow->show();

    if (app.isSessionRestored()) {
        const QString className = KXmlGuiWindow::classNameOfToplevel(1);
        if (className == QLatin1String("DolphinMainWindow")) {
            m_mainWindow->restore(1);
        } else {
           qCWarning(DolphinDebug) << "Unknown class " << className << " in session saved data!";
        }
    }

    return app.exec(); // krazy:exclude=crash;
}
