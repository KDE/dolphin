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
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


#include "nsplugin.h"

#include <qmessagebox.h>
#include <kdebug.h>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <klocale.h>
#include <qlist.h>
#include <qsocketnotifier.h>
#include <stdlib.h>
#include "../../config.h"

#include "kxt.h"

#include <X11/Intrinsic.h>
#include <X11/Shell.h>


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
  kdDebug(1430) << "Detected X Error: " << errstr << endl;
  return 1;
}

/*
 * As the plugin viewer needs to be a motif application, I give in to
 * the "old style" and keep lot's of global vars. :-)
 */

static QCString g_dcopId;
static XtAppContext g_appcon;
static bool g_quit = false;

void quitXt()
{
   g_quit = true;
}

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


/**
 * socket notifier handling
 *
 */

struct SocketNot
{
  int fd;
  QObject *obj;
  XtInputId id;
};

QList<SocketNot> _notifiers[3];

/**
 * socketCallback - send event to the socket notifier
 *
 */
void socketCallback(void *client_data, int */*source*/, XtInputId */*id*/)
{
  kdDebug(1430) << "-> socketCallback( client_data=" << client_data << " )" << endl;

  QEvent event( QEvent::SockAct );
  SocketNot *socknot = (SocketNot *)client_data;
  kdDebug(1430) << "obj=" << (void*)socknot->obj << endl;
  QApplication::sendEvent( socknot->obj, &event );

  kdDebug(1430) << "<- socketCallback" << endl;
}


/**
 * qt_set_socket_handler - redefined internal qt function to register sockets
 * The linker looks in the main binary first and finds this implementation before
 * the original one in Qt. I hope this works with every dynamic library loader on any OS.
 *
 */
extern bool qt_set_socket_handler( int, int, QObject *, bool );
bool qt_set_socket_handler( int sockfd, int type, QObject *obj, bool enable )
{
  if ( sockfd < 0 || type < 0 || type > 2 || obj == 0 ) {
#if defined(CHECK_RANGE)
      qWarning( "QSocketNotifier: Internal error" );
#endif
      return FALSE;
  }

  XtPointer inpMask = 0;

  switch (type) {
  case QSocketNotifier::Read:      inpMask = (XtPointer)XtInputReadMask; break;
  case QSocketNotifier::Write:     inpMask = (XtPointer)XtInputWriteMask; break;
  case QSocketNotifier::Exception: inpMask = (XtPointer)XtInputExceptMask; break;
  default: return FALSE;
  }

  if (enable) {
      SocketNot *sn = new SocketNot;
      sn->obj = obj;
      sn->fd = sockfd;

      if( _notifiers[type].isEmpty() ) {
          _notifiers[type].insert( 0, sn );
      } else {
          SocketNot *p = _notifiers[type].first();
          while ( p && p->fd > sockfd )
              p = _notifiers[type].next();

#if defined(CHECK_STATE)
          if ( p && p->fd==sockfd ) {
              static const char *t[] = { "read", "write", "exception" };
              qWarning( "QSocketNotifier: Multiple socket notifiers for "
                        "same socket %d and type %s", sockfd, t[type] );
          }
#endif
          if ( p )
              _notifiers[type].insert( _notifiers[type].at(), sn );
          else
              _notifiers[type].append( sn );
      }

      sn->id = XtAppAddInput( g_appcon, sockfd, inpMask, socketCallback, sn );

  } else {

      SocketNot *sn = _notifiers[type].first();
      while ( sn && !(sn->obj == obj && sn->fd == sockfd) )
          sn = _notifiers[type].next();
      if ( !sn )				// not found
          return FALSE;

      XtRemoveInput( sn->id );
      _notifiers[type].remove();
  }

  return TRUE;
}


int main(int argc, char** argv)
{
    // hack to avoid segfault in qapp's session manager routines
    setenv( "SESSION_MANAGER", "", 1 );

   // trap X errors
   kdDebug(1430) << "1 - XSetErrorHandler" << endl;
   XSetErrorHandler(x_errhandler);
   setvbuf( stderr, NULL, _IONBF, 0 );

   // Create application
   kdDebug(1430) << "2 - XtToolkitInitialize" << endl;
   XtToolkitInitialize();
   g_appcon = XtCreateApplicationContext();
   Display *dpy = XtOpenDisplay(g_appcon, NULL, "nspluginviewer", "nspluginviewer",
                                0, 0, &argc, argv);

   _notifiers[0].setAutoDelete( TRUE );
   _notifiers[1].setAutoDelete( TRUE );
   _notifiers[2].setAutoDelete( TRUE );

   kdDebug(1430) << "3 - parseCommandLine" << endl;
   parseCommandLine(argc, argv);
   kdDebug(1430) << "4 - KXtApplication app" << endl;
   KLocale::setMainCatalogue("nsplugin");
   KXtApplication app(dpy, argc, argv, "nspluginviewer");

   // initialize the dcop client
   kdDebug(1430) << "5 - app.dcopClient" << endl;
   DCOPClient *dcop = app.dcopClient();
   if (!dcop->attach())
   {
      QMessageBox::critical(NULL,
                            i18n("Error connecting to DCOP server"),
                            i18n("There was an error connecting to the Desktop\n"
                                 "communications server.  Please make sure that\n"
                                 "the 'dcopserver' process has been started, and\n"
                                 "then try again.\n"));
      exit(1);
   }

   kdDebug(1430) << "6 - dcop->registerAs" << endl;
   if (g_dcopId)
      g_dcopId = dcop->registerAs( g_dcopId, false );
   else
      g_dcopId = dcop->registerAs("nspluginviewer");

   // create dcop interface
   kdDebug(1430) << "7 - new NSPluginViewer" << endl;
   NSPluginViewer *viewer = new NSPluginViewer( "viewer", 0 );

   // start main loop
   kdDebug(1430) << "8 - XtAppProcessEvent" << endl;
   while (!g_quit)
     XtAppProcessEvent( g_appcon, XtIMAll);

   // delete viewer
   delete viewer;
}
