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
  kdDebug() << "main" << endl;
  setvbuf( stderr, NULL, _IONBF, 0 );
  KCmdLineArgs::init(argc, argv, "nsplugin", "A Netscape Plugin test program", "0.1");

  KApplication app(argc, argv, "nsplugin");

  app.dcopClient()->attach();
  app.dcopClient()->registerAs(app.name());
  app.dcopClient()->setNotifications(true);


  NSPluginLoader *loader = NSPluginLoader::instance();
  
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
  QWidget *win = loader->NewInstance(0, src, mime, 1, _argn, _argv);
/*  
  QStringList _argn, _argv;
  _argn << "TYPE" << "WIDTH" << "HEIGHT" << "java_docbase" << "CODE";
  _argv << "application/x-java-applet" << "450" << "350" << "file:///none" << "sun/plugin/panel/ControlPanelApplet.class";
  QWidget *win = loader->NewInstance(0, "", "application/x-java-applet", 1, _argn, _argv);
 */

  if (win)
    {
      app.setMainWidget(win);
      win->show();
  
      app.exec();

      delete win;
      delete loader;
    }
}
