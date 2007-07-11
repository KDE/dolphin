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

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

int main(int argc, char **argv)
{
    KAboutData about("dolphin", 0,
                     ki18nc("@info", "Dolphin"),
                     "0.9.0",
                     ki18nc("@info", "File Manager"),
                     KAboutData::License_GPL,
                     ki18nc("@info:credit", "(C) 2006 Peter Penz"));
    about.setHomepage("http://enzosworld.gmxhome.de");
    about.setBugAddress("peter.penz@gmx.at");
    about.addAuthor(ki18nc("@info:credit", "Peter Penz"),
                    ki18nc("@info:credit", "Maintainer and developer"),
                    "peter.penz@gmx.at");
    about.addAuthor(ki18nc("@info:credit", "Cvetoslav Ludmiloff"),
                    ki18nc("@info:credit", "Developer"),
                    "ludmiloff@gmail.com");
    about.addAuthor(ki18nc("@info:credit", "Stefan Monov"),
                    ki18nc("@info:credit", "Developer"),
                    "logixoul@gmail.com");
    about.addAuthor(ki18nc("@info:credit", "Michael Austin"),
                    ki18nc("@info:credit", "Documentation"),
                    "tuxedup@users.sourceforge.net");
    about.addAuthor(ki18nc("@info:credit", "Orville Bennett"),
                    ki18nc("@info:credit", "Documentation"), "obennett@hartford.edu");
    about.addCredit(ki18nc("@info:credit", "Aaron J. Seigo"),
                    ki18nc("@info:credit", "... for the great support and the amazing patches"));
    about.addCredit(ki18nc("@info:credit", "Patrice Tremblay and Gregor Kalisnik"),
                    ki18nc("@info:credit", "... for their patches"));
    about.addCredit(ki18nc("@info:credit", "Ain, Itai, Ivan, Stephane, Patrice, Piotr and Stefano"),
                    ki18nc("@info:credit", "... for their translations"));

    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("+[Url]", ki18nc("@info:shell", "Document to open"));
    KCmdLineArgs::addCmdLineOptions(options);

    if (!DolphinApplication::start()) {
        return 0;
    }

    DolphinApplication app;
#ifdef __GNUC__
#warning TODO, SessionManagement
#endif
#if 0
    if (false /* KDE4-TODO: app.isSessionRestored() */) {
        int n = 1;
        while (KMainWindow::canBeRestored(n)) {
            Dolphin::mainWin().restore(n);
            ++n;
        }
    } else {
#endif
        return app.exec();
    }
