/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

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
#include <kuniqueapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include "toplevel.h"

static KCmdLineOptions options[] =
{
  { "+[file]", I18N_NOOP("File to edit"), 0 },
  { 0, 0, 0}
};

int main(int argc, char ** argv)
{
  KLocale::setMainCatalogue("konqueror");
  KAboutData aboutData( "keditbookmarks", I18N_NOOP("KEditBookmarks"), "1.0",
                        I18N_NOOP("Konqueror Bookmarks Editor"),
                        KAboutData::License_GPL,
                        I18N_NOOP("(c) 2000, KDE developers") );
  aboutData.addAuthor("David Faure",0, "faure@kde.org");

  KCmdLineArgs::init( argc, argv, &aboutData );
  KApplication::addCmdLineOptions();
  // KUniqueApplication::addCmdLineOptions();
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication::disableAutoDcopRegistration(); 
  KApplication app;

  bool unique;

  QCString appName = "keditbookmarks";
  QCString givenName = app.dcopClient()->registerAs(appName,false);

  if (givenName != appName) {
     int answer = KMessageBox::warningYesNo( 0, i18n("Another instance of KEditBookmarks is already running, do you really want to open another instance or continue work in the same instance?\nPlease note that, unfortunately, duplicate views are read-only."), i18n("Warning"), i18n("Run another"), i18n("Quit") );
     if (0) {
        i18n("Continue in same");
     }
     unique = false;
     bool quit = (answer==KMessageBox::No);
     if (quit) {
        // app.dcopClient()->send( "keditbookmarks", "KEditBookmarks", "activateWindow()", data );
        return 0;
     }
  } else {
     unique = true;
  } 

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  QString bookmarksFile = (args->count() == 0)
                          ? locateLocal("data", QString::fromLatin1("konqueror/bookmarks.xml") )
                          : QString::fromLatin1(args->arg(0));
  args->clear();

  KEBTopLevel * toplevel = new KEBTopLevel( bookmarksFile, !unique );
  toplevel->show();
  app.setMainWidget(toplevel);

  return app.exec();
}
