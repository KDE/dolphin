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


NSPluginLoader *NSPluginLoader::s_instance = 0;
int NSPluginLoader::s_refCount = 0;


NSPluginInstance::NSPluginInstance(QWidget *parent, PluginPrivateData *data, const QCString& app, const QCString& id)
  : QXEmbed(parent), DCOPStub(app, id), NSPluginInstanceIface_stub(app, id), _data(data)
{
  setBackgroundMode(QWidget::NoBackground);
  embed(NSPluginInstanceIface_stub::winId());
}


NSPluginInstance::~NSPluginInstance()
{
  destroyPlugin();
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


NSPluginLoader *NSPluginLoader::instance()
{
  if (!s_instance)
    s_instance = new NSPluginLoader;

  s_refCount++;

  return s_instance;
}


void NSPluginLoader::release()
{
  s_refCount--;

  if (s_refCount==0)
  {
    delete s_instance;
    s_instance = 0;
  }
}


NSPluginLoader::~NSPluginLoader()
{
  kdDebug() << "~NSPluginLoader" << endl;
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

  kdDebug() << "Looking up plugin for mimetype " << mimeType << ": " << plugin << endl;

  return plugin;
}


bool NSPluginLoader::loadPlugin(const QString &plugin)
{
  kdDebug() << "NSPluginLoader::loadPlugin" << endl;
  PluginPrivateData *data = _private[plugin];

  if (data)
  {
    kdDebug() << "old plugin found: data=" << data << endl;
  } else
    {
      data = new PluginPrivateData;
      data->running = false;
      data->process = new KProcess;

      // get the dcop app id
      int pid = (int)getpid();
      QString dcopPlugin;
      QTextOStream(&dcopPlugin) << plugin << "-" << pid;
      data->dcopid = dcopPlugin;

      // register process
      _private.insert(plugin, data);

      connect(data->process, SIGNAL(processExited(KProcess*)),
	      this, SLOT(processTerminated(KProcess*)));

      // find the external viewer process
      QString viewer = KGlobal::dirs()->findExe("nspluginviewer");
      if (!viewer)
      {
      	kdError() << "Can't find nspluginviewer" << endl;
      	
      	delete data->process;
      	_private.remove(plugin);
      	delete data;
      	
	return false;
      }
      *data->process << viewer;

      // tell the process it's parameters
      *data->process << "-dcopid";
      *data->process << data->dcopid;
      *data->process << "-plugin";
      *data->process << plugin;

      // run the process
      kdDebug() << "Running nspluginviewer" << endl;
      data->process->start();

      // wait for the process to run
      int cnt = 0;
      while (!kapp->dcopClient()->isApplicationRegistered(data->dcopid))
	{
	  //kapp->processEvents(); // would lead to recursive calls in khtml
	  sleep(1); kdDebug() << "sleep" << endl;
	  cnt++;
	  if (cnt >= 100)
	    {
	      delete data->process;
	      _private.remove(plugin);
      	      delete data;

	      return false;
	    } 
	}

      // create the proxy object
      kdDebug() << "Creating NSPluginClassIface_stub" << endl;
      cnt = 0;
      while (1)
      {
      	data->stub = new NSPluginClassIface_stub(data->dcopid, plugin.ascii());
      	if (data->stub) break;
      	
      	//kapp->processEvents(); // would lead to recursive calls in khtml
      	      	
      	sleep(1); kdDebug() << "sleep" << endl;      	
     	cnt++;
     	if (cnt >= 10)
	{
	  delete data->process;
      	  _private.remove(plugin);
      	  delete data;
      	        	 	
	  return false;
	}
      }

      kdDebug() << "stub = " << data->stub << endl;
    }

  return true;
}


void NSPluginLoader::unloadPlugin(const QString &plugin)
{
  PluginPrivateData *data = _private[plugin];
  if (data)
    {
      kdDebug() << "unloading plugin " << plugin << endl;
      delete data->process;
      _private.remove(plugin);
    }
}


void NSPluginLoader::applicationRegistered( const QCString& appId )
{
  kdDebug() << "DCOP application " << appId.data() << " just registered!" << endl;

  QDictIterator<PluginPrivateData> dit(_private);
  while (dit.current())
    {
      if (dit.current()->dcopid == appId)
	{
	  dit.current()->running = true;
	  kdDebug() << "plugin now running" << endl;
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
  kdDebug() << "-> NSPluginLoader::NewInstance( parent=" << parent << ", url=" << url << ", mime=" << mimeType << ", ...)" << endl;

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
  NSPluginInstance *inst = 0L;
  if (!ref.isNull())
    inst = new NSPluginInstance(parent, data, ref.app(), ref.object());

  kdDebug() << "<- NSPluginLoader::NewInstance = " << inst << endl;
  return inst;
}
