/*

  This application scans for Netscape plugins and create a cache and
  the necessary mimelnk and service files.


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
#include <qfile.h>
#include <qtextstream.h>
#include <qregexp.h>


#include <kapp.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <klibloader.h>


bool mimeExists(QString mime)
{
  QString fname = KGlobal::dirs()->findResource("mime", mime+".desktop");
  
  return !fname.isEmpty();
}


void generateMimeType(QString mime, QString extensions, QString description)
{
  QString dir = mime;
  int pos = mime.findRev('/');
  if (pos < 0)
    return;
  dir = mime.left(pos);

  dir = KGlobal::dirs()->saveLocation("mime", dir);

  kdDebug() << "Saving to " << dir + mime.mid(pos) + ".desktop" << endl;
  
  QFile f(dir + mime.mid(pos) + ".desktop");
  if (f.open(IO_WriteOnly))
    {
      QTextStream ts(&f);

      ts << "[Desktop Entry]" << endl;
      ts << "Name=Netscape plugin mimeinfo" << endl;
      ts << "Type=MimeType" << endl;
      ts << "MimeType=" << mime << endl;
      
      if (!extensions.isEmpty())
	{
	  ts << "Patterns=";
	  QStringList exts = QStringList::split(",", extensions);
	  for (QStringList::Iterator it=exts.begin(); it != exts.end(); ++it)
	    ts << "*." << *it << ";";
	  ts << endl;
	}

      if (!description.isEmpty())
	ts << "Comment=" << description << endl;

      ts << "X-KDE-AutoEmbed=true" << endl;

      f.close();
    }
}


int main(int argc, char *argv[])
{
  KApplication app(argc, argv, "pluginscan");

  QStringList _searchPaths, _mimeInfo;
  QRegExp version(";version=[^:]*:");

  // set up the paths used to look for plugins -----------------------------

  // FIXME: do this more intelligent!
  _searchPaths.append("/usr/local/netscape/plugins");
  _searchPaths.append("/opt/netscape/plugins");
  _searchPaths.append("/opt/netscape/communicator/plugins");
  _searchPaths.append("/usr/lib/netscape/plugins");
  _searchPaths.append(QString("%1/.netscape/plugins").arg(getenv("HOME")));
  _searchPaths.append(QString("%1/plugins").arg(getenv("MOZILLA_HOME")));

  // append environment variable NPX_PLUGIN_PATH
  QStringList envs = QStringList::split(':', getenv("NPX_PLUGIN_PATH"));
  QStringList::Iterator it;
  for (it = envs.begin(); it != envs.end(); ++it)
    _searchPaths.append(*it);


  // open the cache file for the mime information
  QString cacheName = KGlobal::dirs()->saveLocation("data", "nsplugins")+"/cache";
  kdDebug() << "Saving cache to " << cacheName << endl;
  QFile cachef(cacheName);
  if (!cachef.open(IO_WriteOnly))
    return -1;
  QTextStream cache(&cachef);


  // read in the plugins mime information ----------------------------------
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

	  func_GetMIMEDescription = _handle->symbol("NP_GetMIMEDescription");
	  
	  if (!func_GetMIMEDescription)                        
	    continue;

	  char *(*fp)();
	  fp = (char *(*)()) func_GetMIMEDescription;
	  
	  QString mimeInfo = fp();

	  // check the mimeInformation
	  if (!mimeInfo)
	    {
	      kDebugInfo("  not a plugin");
	      delete _handle;
	      continue;
	    }

	  // FIXME: Some plugins will not work, e.g. because they
	  // use JAVA. These should be filtered out here!

	  // remove version info, as it is not used at the moment
	  mimeInfo.replace(version, ":");

	  kdDebug() << "Mime info: " <<  mimeInfo << endl;

	  // note the plugin name
	  cache << "[" << *it + "/" + files[i] << "]" << endl;

	  // parse mime info string
	  QStringList entries = QStringList::split(';', mimeInfo);
	  QStringList::Iterator entry;
	  for (entry = entries.begin(); entry != entries.end(); ++entry)
	    {
	      cache << *entry << endl;
	      if (!_mimeInfo.contains(*entry))
		_mimeInfo.append(*entry);
	    }
	  
	  kDebugInfo("  is a plugin");

	  delete _handle;
	}
    }
  
  QStringList mimeTypes;

  // write mimetype files
  for (QStringList::Iterator it=_mimeInfo.begin(); it != _mimeInfo.end(); ++it)
    {
      kdDebug() << "MIME=" << *it << endl;

      QStringList info = QStringList::split(":", *it, true);
      if (info.count() == 3)
	{
	  if (!mimeTypes.contains(info[0]))
	    mimeTypes.append(info[0]);

	  if (!mimeExists(info[0]))
	    generateMimeType(info[0],info[1],info[2]);
	}
    }
  
  // write plugin lib service file
  QString fname = KGlobal::dirs()->saveLocation("services", "") + "/nsplugin.desktop";

  kdDebug() << "Writing service file to " << fname << endl;

  QFile f(fname);   
  if (f.open(IO_WriteOnly))
    {
      QTextStream ts(&f);

      ts << "[Desktop Entry]" << endl;
      ts << "Name=Netscape plugin viewer" << endl;
      ts << "Type=Service" << endl;
      ts << "Icon=netscape" << endl;
      ts << "Comment=Netscape plugin viewer" << endl;
      ts << "X-KDE-Library=libplugin" << endl;
      ts << "ServiceTypes=KParts/ReadOnlyPart,Browser/View" << endl;

      if (mimeTypes.count() > 0)
	{
	  ts << "MimeType=";
	  for (QStringList::Iterator it=mimeTypes.begin(); it != mimeTypes.end(); ++it)
	    ts << *it << ";";
	  ts << endl;
	}
	
      f.close();
    }

  cachef.close();
}
