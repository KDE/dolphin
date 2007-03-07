/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Stefan Monov <logixoul@gmail.com>               *
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

#include "dolphinapplication.h"
#include "dolphinmainwindow.h"
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <krun.h>
#include <QDBusInterface>
#include <QDBusReply>

static KCmdLineOptions options[] =
{
    { "+[Url]", I18N_NOOP( "Document to open" ), 0 },
    KCmdLineLastOption
};

void openWindow(DolphinApplication* app, const QString& url = QString())
{
    if (app != 0) {
        app->openWindow(url);
        return;
    }

    static QDBusInterface dbusIface("org.kde.dolphin", "/dolphin/Application", "",
                                    QDBusConnection::connectToBus(QDBusConnection::SessionBus, "session_bus"));
    QDBusReply<int> reply = dbusIface.call("openWindow", url);
}

int main(int argc, char **argv)
{
    KAboutData about("dolphin",
                     I18N_NOOP("Dolphin"),
                     "0.9.0",
                     I18N_NOOP("File Manager"),
                     KAboutData::License_GPL,
                     "(C) 2006 Peter Penz");
    about.setHomepage("http://enzosworld.gmxhome.de");
    about.setBugAddress("peter.penz@gmx.at");
    about.addAuthor("Peter Penz", I18N_NOOP("Maintainer and developer"), "peter.penz@gmx.at");
    about.addAuthor("Cvetoslav Ludmiloff", I18N_NOOP("Developer"), "ludmiloff@gmail.com");
    about.addAuthor("Stefan Monov", I18N_NOOP("Developer"), "logixoul@gmail.com");
    about.addAuthor("Michael Austin", I18N_NOOP("Documentation"), "tuxedup@users.sourceforge.net");
    about.addAuthor("Orville Bennett", I18N_NOOP("Documentation"), "obennett@hartford.edu");
    about.addCredit("Aaron J. Seigo", I18N_NOOP("... for the great support and the amazing patches"));
    about.addCredit("Patrice Tremblay and Gregor Kalisnik", I18N_NOOP("... for their patches"));
    about.addCredit("Ain, Itai, Ivan, Stephane, Patrice, Piotr and Stefano",
                    I18N_NOOP("... for their translations"));

    KCmdLineArgs::init(argc, argv, &about);
    KCmdLineArgs::addCmdLineOptions(options);
    DolphinApplication *app = 0;
    if (DolphinApplication::start()) {
        app = new DolphinApplication();
    }


#ifdef __GNUC__
#warning TODO, SessionManagement
#endif
#if 0
    if (false /* KDE4-TODO: app.isSessionRestored() */) {
        int n = 1;
        while (KMainWindow::canBeRestored(n)){
            Dolphin::mainWin().restore(n);
            ++n;
        }
    } else {
#endif

    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    if (args->count() > 0) {
        for (int i = 0; i < args->count(); ++i) {
            openWindow(app, args->arg(i));
        }
    }
    else {
        openWindow(app);
    }
    args->clear();
    if (app != 0) {
        return app->exec();
    }
}
