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


#include <qmessagebox.h>
#include <kdebug.h>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <qxt.h>
#include <klocale.h>
#include <qintdict.h>
#include <qsocketnotifier.h>

#include "kxt.h"
#include "nsplugin.h"

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
  kDebugInfo("Detected X Error: %s", errstr);
  return 1; 
}


/*
 * As the plugin viewer needs to be a motif application, I give in to 
 * the "old style" and keep lot's of global vars. :-)
 */

QCString plugin;              // name of the plugin
QCString dcopId;
XtAppContext appcon;

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
      if (!strcmp(argv[i], "-plugin") && (i+1 < argc))
	{
	  plugin = argv[i+1];
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
  kDebugInfo("-> socketCallback( client_data=%x )", client_data);

  QEvent event( QEvent::SockAct );
  SocketNot *socknot = (SocketNot *)client_data;
  QApplication::sendEvent( socknot->obj, &event );

  kDebugInfo("<- dcopReadCallback");
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
  kDebugInfo("qt_set_socket_handler( sockfd=%d, type=%d, obj=%x, enable=%d )");

  SocketNot *socknot = 0;
  QIntDict<SocketNot> *notifiers;

  switch (type)
  {
  case QSocketNotifier::Read: notifiers = &_read_notifiers; break;
  case QSocketNotifier::Write: notifiers = &_write_notifiers; break;
  case QSocketNotifier::Exception: notifiers = &_except_notifiers; break;
  default: return FALSE;
  }

  socknot = notifiers->find( sockfd );
  if (enable)
  {
    if (!socknot)
    {
      socknot = new SocketNot;
      notifiers->insert( sockfd, socknot );
    } else
        XtRemoveInput( socknot->id );
  	
    socknot->sock = sockfd;
    socknot->type = type;
    socknot->obj = obj;
  	
    switch (type)
    {
    case QSocketNotifier::Read:
    	socknot->id = XtAppAddInput(appcon, sockfd, (XtPointer)XtInputReadMask,
  	                            socketCallback, socknot);
        break;

    case QSocketNotifier::Write:
    	socknot->id = XtAppAddInput(appcon, sockfd, (XtPointer)XtInputWriteMask,
  	                            socketCallback, socknot);
        break;

    case QSocketNotifier::Exception:
    	socknot->id = XtAppAddInput(appcon, sockfd, (XtPointer)XtInputExceptMask,
  	                            socketCallback, socknot);
        break;
    }
  } else
      if (socknot)
      {
    	XtRemoveInput( socknot->id );
    	notifiers->remove( socknot->sock );
    	delete socknot;
      }

  return TRUE;
}


int main(int argc, char** argv)
{
  // trap X errors
  XSetErrorHandler(x_errhandler);
  setvbuf( stderr, NULL, _IONBF, 0 );

  // Create application
  XtToolkitInitialize();
  appcon = XtCreateApplicationContext();
  Display *dpy = XtOpenDisplay(appcon, NULL, "nspluginviewer", "nspluginviewer", 0, 0, &argc, argv);

  parseCommandLine(argc, argv);
  KXtApplication app(dpy, argc, argv, "nspluginviewer");

  // initialize the dcop client
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

  dcopId = dcop->registerAs(plugin);

  // create the DCOP object for the plugin class
  NSPluginClass *cls = new NSPluginClass(plugin, plugin);

  // Testing code
  //QString src = "file:/home/sschimanski/kimble_themovie.swf";
  //QString src = "file:/home/sschimanski/autsch.swf";
  //QString mime = "application/x-shockwave-flash";

  //QString src = "file:/opt/kde/share/Circuit.jpg";
  //QString mime = "image/jpg";

  //QString src = "file:/home/sschimanski/hund.avi";
  //QString mime = "video/avi";

  /*QStringList _argn, _argv;
  _argn << "WIDTH" << "HEIGHT" << "SRC" << "MIME";
  _argv << "400" << "250" << src << mime;
  cls->NewInstance(mime, 1, _argn, _argv);*/

  // start main loop
  XtAppMainLoop(appcon);

  delete cls;
}
