#include <stdio.h>

#include <qstring.h>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <kdebug.h>

#include "NSPluginClassIface_stub.h"


#include "nspluginloader.h"


int main(int argc, char *argv[])
{
  setvbuf( stderr, NULL, _IONBF, 0 );
  KCmdLineArgs::init(argc, argv, "nsplugin", "A Netscape Plugin test program", "0.1");

  KApplication app(argc, argv, "nsplugin");

  app.dcopClient()->attach();
  app.dcopClient()->registerAs(app.name());
  app.dcopClient()->setNotifications(true);


  NSPluginLoader *loader = NSPluginLoader::instance();
  
  QWidget *win;

  //QString src = "file:/home/sschimanski/kimble_themovie.swf";
  QString src = "file:/home/sschimanski/autsch.swf";
  QString mime = "application/x-shockwave-flash";

  //QString src = "file:/opt/kde/share/Circuit.jpg";
  //QString mime = "image/jpg";

  //QString src = "file:/home/sschimanski/hund.avi";
  //QString mime = "video/avi";

  QStringList _argn, _argv;
  _argn << "SRC" << "TYPE" << "WIDTH" << "HEIGHT";
  _argv << src << mime << "400" << "250";
  win = loader->NewInstance(0, src, mime, 1, _argn, _argv);
  
  if (win)
    {
      app.setMainWidget(win);
      win->show();
    }
  
  app.exec();
}
