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
#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kuniqueapplication.h>

#include <kmessagebox.h>

#include <kbookmarkmanager.h>
#include <kbookmarkexporter.h>

#include "core.h"

#include "toplevel.h"

static KCmdLineOptions options[] = {
   {"exportmoz <filename>", I18N_NOOP("Export bookmarks to file in Mozilla format."), 0},
   {"exportns <filename>", I18N_NOOP("Export bookmarks to file in Netscape (<=4) format."), 0},
   {"+[file]", I18N_NOOP("File to edit"), 0},
   {0, 0, 0}
};

static int askUser(KApplication &app, QString filename, bool &readonly) {
   QCString requestedName("keditbookmarks");

   if (filename != "") { 
      requestedName += "-" + filename.utf8();
   }

   if (app.dcopClient()->registerAs(requestedName,false) == requestedName) {
      return true;
   }

   int ret = KMessageBox::warningYesNo(0, 
                              i18n("Another instance of KEditBookmarks is already running, do you really "
                                   "want to open another instance or continue work in the same instance?\n"
                                   "Please note that, unfortunately, duplicate views are read-only."), 
                              i18n("Warning"),
                              i18n("Run Another"),     /* yes */
                              i18n("Continue in Same") /*  no */);

   if (ret != KMessageBox::No) {
      continueInWindow(requestedName);
      return false;
   } else {
      readonly = true;
      return true;
   }
}

int main(int argc, char **argv) {
   KLocale::setMainCatalogue("konqueror");
   KAboutData aboutData("keditbookmarks", I18N_NOOP("KEditBookmarks"), "1.2",
                        I18N_NOOP("Konqueror Bookmarks Editor"),
                        KAboutData::License_GPL,
                        I18N_NOOP("(c) 2000 - 2003, KDE developers") );
   aboutData.addAuthor("David Faure",0, "faure@kde.org");

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
      return 0;
   }

   args->clear();

   bool readonly = false;
   if (askUser(app, (gotArg ? filename : ""), readonly)) {
      KEBTopLevel *toplevel = new KEBTopLevel(filename, readonly);
      toplevel->show();
      app.setMainWidget(toplevel);
      return app.exec();
   } else {
      return 0;
   }
}
