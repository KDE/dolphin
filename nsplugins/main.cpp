#include <qstring.h>


#include <kapp.h>
#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <kdebug.h>

#include "NSPluginClassIface_stub.h"


#include "nspluginloader.h"


int main(int argc, char *argv[])
{
  KCmdLineArgs::init(argc, argv, "nsplugin", "A Netscape Plugin test program", "0.1");

  KApplication app(argc, argv, "nsplugin");

  app.dcopClient()->attach();
  app.dcopClient()->setNotifications(true);


  NSPluginLoader *loader = NSPluginLoader::instance();
  
  QWidget *win;

  QStringList _argn, _argv;
  _argn << "SRC" << "TYPE" << "WIDTH" << "HEIGHT";
  _argv << "file:/home/mhk/nsplugin/nsplugin/caldera.xpm" << "image/x-xpixmap" << "400" << "250";
  win = loader->NewInstance(0, "file:/home/mhk/nsplugin/nsplugin/caldera.xpm", "image/x-xpixmap", 1, _argn, _argv);
  
  if (win)
    {
      app.setMainWidget(win);
      win->show();
    }
  
  app.exec();

}
