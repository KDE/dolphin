/*

  This is a standalone application that executes Netscape plugins.


  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <mhk@caldera.de>
                     Stefan Schimanski <1Stein@gmx.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/


#include <config.h>

#include <kapplication.h>
#include "nsplugin.h"

#include <dcopclient.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <q3ptrlist.h>
#include <QSocketNotifier>
//Added by qt3to4:
#include <QEvent>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef Bool
#undef Bool
#endif
#include <kconfig.h>

#include "qxteventloop.h"

/**
 *  Use RLIMIT_DATA on systems that don't define RLIMIT_AS,
 *  such as FreeBSD 4, NetBSD and OpenBSD.
 */

#ifndef RLIMIT_AS
#define RLIMIT_AS RLIMIT_DATA
#endif

/**
 * The error handler catches all X errors, writes the error
 * message to the debug log and continues.
 *
 * This is done to prevent abortion of the plugin viewer
 * in case the plugin does some invalid X operation.
 *
 */
static int x_errhandler(Display *dpy, XErrorEvent *error)
{
  char errstr[256];
  XGetErrorText(dpy, error->error_code, errstr, 256);
  kDebug(1430) << "Detected X Error: " << errstr << endl;
  return 1;
}

/*
 * As the plugin viewer needs to be a motif application, I give in to
 * the "old style" and keep lot's of global vars. :-)
 */

static DCOPCString g_dcopId;

/**
 * parseCommandLine - get command line parameters
 *
 */
void parseCommandLine(int argc, char *argv[])
{
   for (int i=0; i<argc; i++)
   {
      if (!strcmp(argv[i], "-dcopid") && (i+1 < argc))
      {
         g_dcopId = argv[i+1];
         i++;
      }
   }
}

int main(int argc, char** argv)
{
    // nspluginviewer is a helper app, it shouldn't do session management at all
   setenv( "SESSION_MANAGER", "", 1 );

   // trap X errors
   kDebug(1430) << "1 - XSetErrorHandler" << endl;
   XSetErrorHandler(x_errhandler);
   setvbuf( stderr, NULL, _IONBF, 0 );

   kDebug(1430) << "2 - parseCommandLine" << endl;
   parseCommandLine(argc, argv);

   kDebug(1430) << "3 - create QXtEventLoop" << endl;
#warning QXtEventLoop is missing
   // QXtEventLoop integrator( "nspluginviewer" );
   parseCommandLine(argc, argv);
   KLocale::setMainCatalog("nsplugin");

   kDebug(1430) << "4 - create KApplication" << endl;

   KCmdLineArgs::init(argc, argv, "nspluginviewer", "nspluginviewer", "", "");
   KApplication app;

   {
      KConfig cfg("kcmnspluginrc", true);
      cfg.setGroup("Misc");
      int v = qBound(0, cfg.readEntry("Nice Level", 0), 19);
      if (v > 0) {
         nice(v);
      }
      v = cfg.readEntry("Max Memory", 0);
      if (v > 0) {
         rlimit rl;
         memset(&rl, 0, sizeof(rl));
         if (0 == getrlimit(RLIMIT_AS, &rl)) {
            rl.rlim_cur = qMin(v, int(rl.rlim_max));
            setrlimit(RLIMIT_AS, &rl);
         }
      }
   }

   // initialize the dcop client
   kDebug(1430) << "5 - app.dcopClient" << endl;
   DCOPClient *dcop = app.dcopClient();
   if (!dcop->attach())
   {
      KMessageBox::error(NULL,
                            i18n("There was an error connecting to the Desktop "
                                 "communications server. Please make sure that "
                                 "the 'dcopserver' process has been started, and "
                                 "then try again."),
                            i18n("Error Connecting to DCOP Server"));
      exit(1);
   }

   kDebug(1430) << "6 - dcop->registerAs" << endl;
   if (!g_dcopId.isEmpty())
      g_dcopId = dcop->registerAs( g_dcopId, false );
   else
      g_dcopId = dcop->registerAs("nspluginviewer");

   dcop->setNotifications(true);

   // create dcop interface
   kDebug(1430) << "7 - new NSPluginViewer" << endl;
   NSPluginViewer *viewer = new NSPluginViewer( "viewer", 0 );

   // start main loop
   kDebug(1430) << "8 - app.exec()" << endl;
   app.exec();

   // delete viewer
   delete viewer;
}
