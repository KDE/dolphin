/*

  This is a standalone application that executes Netscape plugins.


  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <mhk@caldera.de>
 
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


#include <kdebug.h>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <qxt.h>

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

QCString dcopId;              // id for dcop
QCString plugin;              // name of the plugin

DCOPClient *dcopClient;       // the dcop client


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


class MyApplication : public KApplication
{
public:

  MyApplication(int argc, char **argv, const QCString &name)
    : KApplication(argc, argv, name) {};

protected:

  bool x11EventFilter(XEvent *xev) {
    printf("ping\n");
    XtDispatchEvent(xev);
    return KApplication::x11EventFilter(xev);
  }

};


int main(int argc, char** argv)
{
  // trap X errors
  XSetErrorHandler(x_errhandler);                                           

  // Create application
  parseCommandLine(argc, argv);
  KXtApplication app(argc, argv, "nspluginviewer");

  // register DCOP service
  dcopClient = new DCOPClient;
  dcopClient->attach();
  dcopClient->registerAs(dcopId, false);

  // create the DCOP object for the plugin class
  NSPluginClass *cls = new NSPluginClass(plugin, plugin);

  // Testing code
  QString src = "file:/home/sschimanski/autsch.swf";
  QString mime = "application/x-shockwave-flash";

  //QString src = "file:/home/sschimanski/hund.avi";
  //QString mime = "video/avi";

  QStringList _argn, _argv;
  _argn << "WIDTH" << "HEIGHT" << "SRC" << "MIME";
  _argv << "400" << "250" << src << mime;
  cls->NewInstance(mime, 1, _argn, _argv);

  app.exec();
}
