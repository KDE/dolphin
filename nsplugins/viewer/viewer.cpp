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
#include <qintdict.h>
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
  kdDebug() << "Detected X Error: " << errstr << endl;
  return 1;
}

/*
 * As the plugin viewer needs to be a motif application, I give in to
 * the "old style" and keep lot's of global vars. :-)
 */

static QCString dcopId;
static XtAppContext appcon;
static bool quit = false;

void quitXt()
{
   quit = true;
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
         dcopId = argv[i+1];
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
  int sock;
  int type;
  QObject *obj;
  XtInputId id;
};

QIntDict<SocketNot> _read_notifiers;
QIntDict<SocketNot> _write_notifiers;
QIntDict<SocketNot> _except_notifiers;


/**
 * socketCallback - send event to the socket notifier
 *
 */
void socketCallback(void *client_data, int */*source*/, XtInputId */*id*/)
{
  kdDebug() << "-> socketCallback( client_data=" << client_data << " )" << endl;

  QEvent event( QEvent::SockAct );
  SocketNot *socknot = (SocketNot *)client_data;
  kdDebug() << "obj=" << (void*)socknot->obj << endl;
  QApplication::sendEvent( socknot->obj, &event );

  kdDebug() << "<- socketCallback" << endl;
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
  kdDebug() << "-> qt_set_socket_handler( sockfd=" << sockfd << ", type=" << type << ", obj=" << obj << ", enable=" << enable << " )" << endl;

  if ( sockfd < 0 || type < 0 || type > 2 || obj == 0 )
  {
     return FALSE;
  }

  SocketNot *socknot = 0;
  QIntDict<SocketNot> *notifiers;
  XtPointer inpMask = 0;

  switch (type)
  {
  case QSocketNotifier::Read:
     inpMask = (XtPointer)XtInputReadMask;
     notifiers = &_read_notifiers;
     break;
  case QSocketNotifier::Write:
     inpMask = (XtPointer)XtInputWriteMask;
     notifiers = &_write_notifiers;
     break;
  case QSocketNotifier::Exception:
     inpMask = (XtPointer)XtInputExceptMask;
     notifiers = &_except_notifiers;
     break;
  default: return FALSE;
  }

  socknot = notifiers->find( sockfd );
  if (enable)
  {
    if (!socknot)
    {
      socknot = new SocketNot;
    } else
    {
        XtRemoveInput( socknot->id );
        notifiers->remove( socknot->sock );
    }

    socknot->sock = sockfd;
    socknot->type = type;
    socknot->obj = obj;
    socknot->id = XtAppAddInput( appcon, sockfd, inpMask, socketCallback, socknot );
    notifiers->insert( sockfd, socknot );
  } else
      if (socknot)
      {
        XtRemoveInput( socknot->id );
        notifiers->remove( socknot->sock );
        delete socknot;
      }

  kdDebug() << "<- qt_set_socket_handler" << endl;
  return TRUE;
}


int main(int argc, char** argv)
{
    // hack to avoid segfault in qapp's session manager routines
    setenv( "SESSION_MANAGER", "", 1 );

   // trap X errors
   kdDebug() << "1 - XSetErrorHandler" << endl;
   XSetErrorHandler(x_errhandler);
   setvbuf( stderr, NULL, _IONBF, 0 );

   // Create application
   kdDebug() << "2 - XtToolkitInitialize" << endl;
   XtToolkitInitialize();
   appcon = XtCreateApplicationContext();
   Display *dpy = XtOpenDisplay(appcon, NULL, "nspluginviewer", "nspluginviewer", 0, 0, &argc, argv);

   kdDebug() << "3 - parseCommandLine" << endl;
   parseCommandLine(argc, argv);
   kdDebug() << "4 - KXtApplication app" << endl;
   KXtApplication app(dpy, argc, argv, "nspluginviewer");

   // initialize the dcop client
   kdDebug() << "5 - app.dcopClient" << endl;
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

   kdDebug() << "6 - dcop->registerAs" << endl;
   if (dcopId)
      dcopId = dcop->registerAs(dcopId, false);
   else
      dcopId = dcop->registerAs("nspluginviewer");

   // create dcop interface
   kdDebug() << "7 - new NSPluginViewer" << endl;
   NSPluginViewer *viewer = new NSPluginViewer( "viewer", 0 );

   // start main loop
   kdDebug() << "8 - XtAppNextEvent" << endl;
   XEvent xe;
   while (!quit)
   {
      XtAppNextEvent( appcon, &xe );
      XtDispatchEvent( &xe );
   }

   // delete viewer
   delete viewer;
}
