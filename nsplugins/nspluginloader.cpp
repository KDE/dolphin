/*

  This is an encapsulation of the  Netscape plugin API.


  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 
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
#include <klibloader.h>
#include <dcopclient.h>
#include <qobject.h>
#include <qxembed.h>


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


NSPluginLoader::NSPluginLoader()
  : QObject(), _mapping(7, false)
{
  // append default netscape paths
  // FIXME: do this more intelligent!
  // FIXME: Use MOZILLA_HOME
  _searchPaths.append("/usr/local/netscape/plugins");
  _searchPaths.append("/opt/netscape/plugins");
  _searchPaths.append("/opt/netscape/communicator/plugins");
  _searchPaths.append(QString("%1/.netscape/plugins").arg(getenv("HOME")));
  
  // append environment variable NPX_PLUGIN_PATH
  QStringList envs = QStringList::split(':', getenv("NPX_PLUGIN_PATH"));
  QStringList::Iterator it;
  for (it = envs.begin(); it != envs.end(); ++it)
    _searchPaths.append(*it);

  // do an initial scan
  rescanPlugins();

  // trap dcop register events
  QObject::connect(kapp->dcopClient(), SIGNAL(applicationRegistered(const QCString&)),
	  this, SLOT(applicationRegistered(const QCString&)));
}


NSPluginLoader::~NSPluginLoader()
{
  QDictIterator<PluginPrivateData> dit(_private);
  while (dit.current())
    {
      unloadPlugin(dit.currentKey());
      ++dit;
    }
}


void NSPluginLoader::rescanPlugins()
{
  // FIXME: This touches all plugins every time
  //        This should really use a cache!

  // iterate over all paths
  QStringList::Iterator it;
  for (it = _searchPaths.begin(); it != _searchPaths.end(); ++it)
    {
      kDebugInfo("Scanning %s", (*it).ascii());

      // iterate over all files 
      QDir files(*it, QString::null, QDir::Name|QDir::IgnoreCase, QDir::Files);
      if (!files.exists(*it))
	continue;

      for (unsigned int i=0; i<files.count(); i++)
	{
	  kDebugInfo("  testing %s/%s", (*it).ascii(), files[i].ascii());

	  // open the library and ask for the mimetype

	  void *func_GetMIMEDescription = 0; 
	  KLibrary *_handle = KLibLoader::self()->library(*it+"/"+files[i]);

	  if (!_handle)
	    continue;

	  func_GetMIMEDescription = _handle->symbol("NPP_GetMIMEDescription");
	  
	  if (!func_GetMIMEDescription)                        
	    continue;

	  char *(*fp)();
	  fp = (char *(*)()) func_GetMIMEDescription;
	  
	  char *mimeInfo = fp();

	  // check the mimeInformation
	  if (!mimeInfo)
	    {
	      kDebugInfo("  not a plugin");
	      KLibLoader::self()->unloadLibrary(*it+"/"+files[i]);
	      continue;
	    }

	  // FIXME: Some plugins will not work, e.g. because they
	  // use JAVA. These should be filtered out here!

	  //	  kDebugInfo("Mime info: %s", mimeInfo);

	  // parse mime info string
	  QStringList entries = QStringList::split(';', mimeInfo);
	  QStringList::Iterator entry;
	  for (entry = entries.begin(); entry != entries.end(); ++entry)
	    {
	      QStringList desc = QStringList::split(':', *entry);
	      QString mime = desc[0].stripWhiteSpace();
	      QStringList suffixes = QStringList::split(',', desc[1].stripWhiteSpace());
	      if (!mime.isEmpty())
		{
		  // insert the mimetype -> plugin mapping
		  _mapping.insert(mime, strdup(*it+"/"+files[i]));
		  
		  // insert the suffix -> mimetype mapping
		  QStringList::Iterator suffix;
		  for (suffix = suffixes.begin(); suffix != suffixes.end(); ++suffix)
		    if (!_filetype.find((*suffix).stripWhiteSpace()))
		      _filetype.insert((*suffix).stripWhiteSpace(), mime);
		}
	    }
	  
	  kDebugInfo("  is a plugin");

	  KLibLoader::self()->unloadLibrary(*it+"/"+files[i]);		  
	}
    }

  /*
#ifndef NDEBUG
  QDictIterator<char> dit(_mapping);
  while (dit.current())
    {
      kDebugInfo("Mapping mimetype %s to plugin %s", dit.currentKey().ascii(), dit.current());
      ++dit;
    }
  QDictIterator<char> dit2(_filetype);
  while (dit2.current())
    {
      kDebugInfo("Mapping file %s to mimetype %s", dit2.currentKey().ascii(), dit2.current());
      ++dit2;
    }
#endif
  */
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
	return false;
      *data->process << viewer;

      // tell the process it's parameters
      *data->process << "-dcopid";
      *data->process << plugin;
      *data->process << "-plugin";
      *data->process << plugin;

      // run the process
      kDebugInfo("Running nspluginviewer");
      data->process->start();

      // wait for the process to run
      int cnt = 0;
      while (!kapp->dcopClient()->isApplicationRegistered(plugin.ascii()))
	{
	  kapp->processEvents();
	  sleep(1); //kdDebug() << "sleep" << endl;
	  cnt++;
	  if (cnt >= 100)
	    {
	      data->process->kill();
	      return false;
	    } 
	}

      // create the proxy object
      data->stub = new NSPluginClassIface_stub(plugin.ascii(), plugin.ascii());
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



QWidget *NSPluginLoader::NewInstance(QWidget *parent, QString url, QString mimeType, int type,
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
  
  // load the plugin
  QString plugin = lookup(mime);
  if (plugin.isEmpty())
    {
      kdDebug() << "No suitable plugin" << endl;
      return 0;
    }

  // load the plugin
  if (!loadPlugin(plugin))
    {
      kdDebug() << "Could not load plugin" << endl;
      return 0;
    }

  // get a new plugin instance
  PluginPrivateData *data = _private[plugin];
  
  kdDebug() << data->stub->GetMIMEDescription() << endl;

  int winid = data->stub->NewInstance(mime, type, argn, argv);
  if (winid)
    {
      kdDebug() << "Window id = " << winid << endl;
      QXEmbed *win = new QXEmbed(parent);
      win->embed(winid);
      return win;
    }
  
  return 0;
}
