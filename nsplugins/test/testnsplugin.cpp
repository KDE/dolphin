/*
  This is an encapsulation of the  Netscape plugin API.

  Copyright (c) 2000 Stefan Schimanski <1Stein@gmx.de>

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
#include <stdio.h>

#include <qstring.h>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <kdebug.h>
#include <kstdaction.h>
#include <kaction.h>

#include "testnsplugin.h"
#include "../NSPluginClassIface_stub.h"
#include "../nspluginloader.h"


TestNSPlugin::TestNSPlugin()
{
   m_loader = NSPluginLoader::instance();

   // client area
   m_client = new QWidget( this, "m_client" );
   setView( m_client );
   m_client->show();
   m_layout = new QHBoxLayout( m_client );

   // file menu
   KStdAction::openNew( this, SLOT(newView()), actionCollection());
   KStdAction::close( this, SLOT(closeView()), actionCollection());
   KStdAction::quit( kapp, SLOT(quit()), actionCollection());

   createGUI( "testnspluginui.rc" );
}


TestNSPlugin::~TestNSPlugin()
{
   kdDebug() << "-> TestNSPlugin::~TestNSPlugin" << endl;
   m_loader->release();
   kdDebug() << "<- TestNSPlugin::~TestNSPlugin" << endl;
}


void TestNSPlugin::newView()
{
   QStringList _argn, _argv;

   //QString src = "file:/home/sschimanski/kimble_themovie.swf";
   //QString src = "file:/home/sschimanski/in_ani.swf";
   //QString src = "http://homepages.tig.com.au/~dkl/swf/promo.swf";
   //QString mime = "application/x-shockwave-flash";

   _argn << "name" << "controls" << "console";
   _argv << "audio" << "ControlPanel" << "Clip1";
   QString src = "http://welt.is-kunden.de:554/ramgen/welt/avmedia/realaudio/0701lw177135.rm";
//   QString src = "nothing";
   QString mime = "audio/x-pn-realaudio-plugin";

   _argn << "SRC" << "TYPE" << "WIDTH" << "HEIGHT";
   _argv << src << mime << "400" << "100";
   QWidget *win = m_loader->newInstance( m_client, src, mime, 1, _argn, _argv, "appid", "callbackid" );

/*
    _argn << "TYPE" << "WIDTH" << "HEIGHT" << "java_docbase" << "CODE";
    _argv << "application/x-java-applet" << "450" << "350" << "file:///none" << "sun/plugin/panel/ControlPanelApplet.class";
    QWidget *win = loader->NewInstance(0, "", "application/x-java-applet", 1, _argn, _argv);
*/

   if ( win )
   {
      m_plugins.append( win );
      connect( win, SIGNAL(destroyed(NSPluginInstance *)),
               this, SLOT(viewDestroyed(NSPluginInstance *)) );
      m_layout->addWidget( win );
      win->show();
   } else
   {
      kdDebug() << "No widget created" << endl;
   }
}

void TestNSPlugin::closeView()
{
   kdDebug() << "closeView" << endl;
   QWidget *win = m_plugins.last();
   if ( win )
   {
      m_plugins.remove( win );
      delete win;
   } else
   {
      kdDebug() << "No widget available" << endl;
   }
}


void TestNSPlugin::viewDestroyed( NSPluginInstance *inst )
{
   kdDebug() << "TestNSPlugin::viewDestroyed" << endl;
   m_plugins.remove( inst );
}


int main(int argc, char *argv[])
{
   kdDebug() << "main" << endl;
   setvbuf( stderr, NULL, _IONBF, 0 );
   KCmdLineArgs::init(argc, argv, "nsplugin", "A Netscape Plugin test program", "0.1");

   KApplication app("nsplugin");

   app.dcopClient()->attach();
   app.dcopClient()->registerAs(app.name());
   app.dcopClient()->setNotifications(true);

   TestNSPlugin *win = new TestNSPlugin;
   app.setMainWidget( win );
   win->show();
   app.exec();

   delete win;
}

#include "testnsplugin.moc"
