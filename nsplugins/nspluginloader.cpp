/*

  This is an encapsulation of the  Netscape plugin API.


  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
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


#include <qdir.h>


#include <kapp.h>
#include <kprocess.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <dcopclient.h>
#include <dcopstub.h>
#include <qobject.h>
#include <qxembed.h>
#include <qtextstream.h>
#include <qregexp.h>


#include "nspluginloader.h"
#include "nspluginloader.moc"

#include "NSPluginClassIface_stub.h"


class PluginPrivateData
{
public:

  KProcess                *process;
  QCString                dcopid;
  bool                    running;
  NSPluginClassIface_stub *stub;

};


NSPluginLoader *NSPluginLoader::_instance = 0;


NSPluginInstance::NSPluginInstance(QWidget *parent, PluginPrivateData *data, const QCString& app, const QCString& id)
  : QXEmbed(parent), DCOPStub(app, id), NSPluginInstanceIface_stub(app, id), _data(data)
{
  setBackgroundMode(QWidget::NoBackground);
  embed(NSPluginInstanceIface_stub::winId());
}


NSPluginInstance::~NSPluginInstance()
{
  _data->stub->DestroyInstance(NSPluginInstanceIface_stub::winId());
}


void NSPluginInstance::resizeEvent(QResizeEvent *event)
{
  QXEmbed::resizeEvent(event);
  resizePlugin(width(), height());
  kdDebug() << "NSPluginInstance(client)::resizeEvent" << endl;
}


NSPluginLoader::NSPluginLoader()
  : QObject(), _mapping(7, false)
{
  scanPlugins();

  // trap dcop register events
  kapp->dcopClient()->setNotifications(true);
  QObject::connect(kapp->dcopClient(), SIGNAL(applicationRegistered(const QCString&)),
	  this, SLOT(applicationRegistered(const QCString&)));
}


NSPluginLoader::~NSPluginLoader()
{
  kDebugInfo("~NSPluginLoader");
  QDictIterator<PluginPrivateData> dit(_private);
  while (dit.current())
    {
      unloadPlugin(dit.currentKey());
      ++dit;
    }
}


void NSPluginLoader::scanPlugins()
{
  QRegExp version(";version=[^:]*:");

  // open the cache file
  QFile cachef(locate("data", "nsplugins/cache"));
  if (!cachef.open(IO_ReadOnly))
    {
      kdDebug() << "Could not load plugin cache file!" << endl;
      return;
    }

  QTextStream cache(&cachef);

  // read in cache
  QString line, plugin;
  while (!cache.atEnd())
    {
      line = cache.readLine();
      if (line.isEmpty() || (line.left(1) == "#"))
	continue;

      if (line.left(1) == "[")
	{
	  plugin = line.mid(1,line.length()-2);
	  continue;
	}

      QStringList desc = QStringList::split(':', line);
      QString mime = desc[0].stripWhiteSpace();
      QStringList suffixes = QStringList::split(',', desc[1].stripWhiteSpace());
      if (!mime.isEmpty())
	{
	  // insert the mimetype -> plugin mapping
	  _mapping.insert(mime, strdup(plugin));
	  
	  // insert the suffix -> mimetype mapping
	  QStringList::Iterator suffix;
	  for (suffix = suffixes.begin(); suffix != suffixes.end(); ++suffix)
	    if (!_filetype.find((*suffix).stripWhiteSpace()))
	      _filetype.insert((*suffix).stripWhiteSpace(), mime);
	}
    }
}


QString NSPluginLoader::lookupMimeType(const QString &url)
{
  QDictIterator<char> dit2(_filetype);
  while (dit2.current())
    {
      QString ext = QString(".")+dit2.currentKey();
      if (url.right(ext.length()) == ext)
	return dit2.current();
      ++dit2;
    }
  return QString::null;
}


QString NSPluginLoader::lookup(const QString &mimeType)
{
  QString plugin = _mapping[mimeType];

  kDebugInfo("Looking up plugin for mimetype %s: %s", mimeType.ascii(), plugin.ascii());

  return plugin;
}


NSPluginLoader *NSPluginLoader::instance()
{
  if (!_instance)
    _instance = new NSPluginLoader;

  return _instance;
}


bool NSPluginLoader::loadPlugin(const QString &plugin)
{
  kDebugInfo("NSPluginLoader::loadPlugin");
  PluginPrivateData *data = _private[plugin];

  if (!data)
    {
      data = new PluginPrivateData;
      data->dcopid = plugin;
      data->running = false;

      data->process = new KProcess;

      connect(data->process, SIGNAL(processExited(KProcess*)),
	      this, SLOT(processTerminated(KProcess*)));

      // find the external viewer process
      QString viewer = KGlobal::dirs()->findExe("nspluginviewer");
      if (!viewer)
      {
      	kDebugError("Can't find nspluginviewer");
	return false;
      }
      *data->process << viewer;

      // tell the process it's parameters
      *data->process << "-dcopid";
      *data->process << plugin;
      *data->process << "-plugin";
      *data->process << plugin;

      // run the process
      kDebugInfo("Running nspluginviewer");
      data->process->start();

      // get the dcop app id
      int pid = (int)data->process->getPid();
      QString dcopPlugin;
      QTextOStream(&dcopPlugin) << plugin << "-" << pid;

      // wait for the process to run
      int cnt = 0;
      while (!kapp->dcopClient()->isApplicationRegistered(dcopPlugin.ascii()))
	{
	  kapp->processEvents();
	  sleep(1); kdDebug() << "sleep" << endl;
	  cnt++;
	  if (cnt >= 100)
	    {
	      data->process->kill();
	      return false;
	    } 
	}

      // create the proxy object
      kDebugInfo("Creating NSPluginClassIface_stub");
      cnt = 0;
      while (1)
      {
      	data->stub = new NSPluginClassIface_stub(dcopPlugin.ascii(), plugin.ascii());
      	if (data->stub) break;
      	
      	kapp->processEvents();
      	      	
      	sleep(1); kdDebug() << "sleep" << endl;      	
     	cnt++;
     	if (cnt >= 10)
	{
	  data->process->kill();
	  return false;
	}
      }

      kDebugInfo("stub = %x", data->stub);
    }

  _private.insert(plugin, data);

  return true;
}


void NSPluginLoader::unloadPlugin(const QString &plugin)
{
  PluginPrivateData *data = _private[plugin];
  if (data)
    {
      kdDebug() << "unloading plugin " << plugin << endl;

      if (data->running)
	{
	  data->running = false;		
	  delete data->process;
	}

      _private.remove(plugin);
    }
}


void NSPluginLoader::applicationRegistered( const QCString& appId )
{
  kDebugInfo("DCOP application %s just registered!", appId.data());

  QDictIterator<PluginPrivateData> dit(_private);
  while (dit.current())
    {
      if (dit.current()->dcopid == appId)
	{
	  dit.current()->running = true;
	  kDebugInfo("plugin now running");
	}
      ++dit;
    }
}


void NSPluginLoader::processTerminated(KProcess *proc)
{
  QDictIterator<PluginPrivateData> dit(_private);
  while (dit.current())
    {
      if (dit.current()->process == proc)
	{
	  kdDebug() << "Plugin process for " << dit.currentKey() << " terminated" << endl;
	  unloadPlugin(dit.currentKey());	  
	}
      ++dit;
    }
}



NSPluginInstance *NSPluginLoader::NewInstance(QWidget *parent, QString url, QString mimeType, int type,
					      QStringList argn, QStringList argv)
{
  // check the mime type
  QString mime = mimeType;
  if (mime.isEmpty())
    {
      mime = lookupMimeType(url);
      argn << "MIME";
      argv << mime;
    }
  if (mime.isEmpty())
    {
      kdDebug() << "Unknown MimeType" << endl;
      return 0;
    }

  // get requested size
  unsigned int width = 0;
  unsigned int height = 0;
  int argc = argn.count();
  for (int i=0; i<argc; i++)
    {
       if (!stricmp(argn[i], "width")) width = argv[i].toUInt();
       if (!stricmp(argn[i], "height")) height = argv[i].toUInt();
    }
  
  // load the plugin
  QString plugin = lookup(mime);
  if (plugin.isEmpty())
    {
      kdDebug() << "No suitable plugin" << endl;
      return 0;
    }

  if (!loadPlugin(plugin))
    {
      kdDebug() << "Could not load plugin" << endl;
      return 0;
    }

  // get a new plugin instance
  PluginPrivateData *data = _private[plugin];
  
  kdDebug() << data->stub->GetMIMEDescription() << endl;

  DCOPRef ref = data->stub->NewInstance(mime, type, argn, argv);
  if (!ref.isNull())
    return new NSPluginInstance(parent, data, ref.app(), ref.object());

  return 0;
}
