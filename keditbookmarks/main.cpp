/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <dcopclient.h>
#include <dcopref.h>
#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kuniqueapplication.h>

#include <kmessagebox.h>
#include <kwin.h>

#include <kbookmarkmanager.h>
#include <kbookmarkexporter.h>

#include "toplevel.h"

static KCmdLineOptions options[] = {
   {"exportmoz <filename>", I18N_NOOP("Export bookmarks to file in Mozilla format."), 0},
   {"exportns <filename>", I18N_NOOP("Export bookmarks to file in Netscape (4.x and earlier) format."), 0},
   {"address <address>", I18N_NOOP("Open at the given position in the bookmarks file"), 0},
   {"+[file]", I18N_NOOP("File to edit"), 0},
   KCmdLineLastOption
};

static void continueInWindow(QString _wname) {
   QCString wname = _wname.latin1();
   int id = -1;

   QCStringList apps = kapp->dcopClient()->registeredApplications();
   for (QCStringList::Iterator it = apps.begin(); it != apps.end(); ++it) {
      QCString &clientId = *it;

      if (qstrncmp(clientId, wname, wname.length()) != 0) {
         continue;
      }

      DCOPRef client(clientId.data(), wname + "-mainwindow#1");
      DCOPReply result = client.call("getWinID()");

      if (result.isValid()) {
         id = (int)result;
         break;
      }
   }

   KWin::setActiveWindow(id);
}

// TODO - make this register() or something like that and move dialog into main
static int askUser(KApplication &app, QString filename, bool &readonly) {
   QCString requestedName("keditbookmarks");

   if (!filename.isEmpty()) { 
      requestedName += "-" + filename.utf8();
   }

   if (app.dcopClient()->registerAs(requestedName, false) == requestedName) {
      return true;
   }

   int ret = KMessageBox::warningYesNo(0, 
                              i18n("Another instance of KEditBookmarks is already running, do you really "
                                   "want to open another instance or continue work in the same instance?\n"
                                   "Please note that, unfortunately, duplicate views are read-only."), 
                              i18n("Warning"),
                              i18n("Run Another"),     /* yes */
                              i18n("Continue in Same") /*  no */);

   if (ret == KMessageBox::No) {
      continueInWindow(requestedName);
      return false;
   } else if (ret == KMessageBox::Yes) {
      readonly = true;
   }

   return true;
}

int main(int argc, char **argv) {
   KLocale::setMainCatalogue("konqueror");
   KAboutData aboutData("keditbookmarks", I18N_NOOP("KEditBookmarks"), "1.2",
                        I18N_NOOP("Konqueror Bookmarks Editor"),
                        KAboutData::License_GPL,
                        I18N_NOOP("(c) 2000 - 2003, KDE developers") );
   aboutData.addAuthor("David Faure", I18N_NOOP("Initial author"), "faure@kde.org");
   aboutData.addAuthor("Alexander Kellett", I18N_NOOP("Maintainer"), "lypanov@kde.org");

   KCmdLineArgs::init(argc, argv, &aboutData);
   KApplication::addCmdLineOptions();
   KCmdLineArgs::addCmdLineOptions(options);

   KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
   bool isGui = !(args->isSet("exportmoz") || args->isSet("exportns"));

   KApplication::disableAutoDcopRegistration(); 
   KApplication app(isGui, isGui);

   bool gotArg = (args->count() == 1);

   QString filename = gotArg
                    ? QString::fromLatin1(args->arg(0))
                    : locateLocal("data", QString::fromLatin1("konqueror/bookmarks.xml"));

   if (!isGui) {
      KBookmarkManager *mgr = KBookmarkManager::managerForFile(filename, false);
      bool mozFlag = args->isSet("exportmoz");
      if (mozFlag && args->isSet("exportns")) {
         KCmdLineArgs::usage("You may only choose one of the --export options.");
      }
      QString path = QString::fromLocal8Bit(args->getOption(mozFlag ? "exportmoz" : "exportns"));
      KNSBookmarkExporter exporter(mgr, path);
      exporter.write(mozFlag);
      return 0; // error flag on exit?, 1?
   }

   QString address = 
      args->isSet("address")
        ? QString::fromLocal8Bit(args->getOption("address"))
        : "/0";

   args->clear();

   bool readonly = false; // passed by ref

   if (askUser(app, (gotArg ? filename : ""), readonly)) {
      KEBApp *toplevel = new KEBApp(filename, readonly, address);
      toplevel->show();
      app.setMainWidget(toplevel);
      return app.exec();
   }

   return 0;
}
