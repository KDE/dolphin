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
#include <kconfig.h>
#include <dcopclient.h>
#include <dcopstub.h>
#include <qobject.h>
#include <qxembed.h>
#include <qtextstream.h>
#include <qregexp.h>

#include "nspluginloader.h"
#include "nspluginloader.moc"

#include "NSPluginClassIface_stub.h"


NSPluginLoader *NSPluginLoader::s_instance = 0;
int NSPluginLoader::s_refCount = 0;


NSPluginInstance::NSPluginInstance(QWidget *parent, const QCString& app, const QCString& id)
  : EMBEDCLASS(parent), DCOPStub(app, id), NSPluginInstanceIface_stub(app, id)
{
    _loader = NSPluginLoader::instance();
    setBackgroundMode(QWidget::NoBackground);
    embed( NSPluginInstanceIface_stub::winId() );
}


NSPluginInstance::~NSPluginInstance()
{
   kdDebug() << "-> NSPluginInstance::~NSPluginInstance" << endl;
   shutdown();
   kdDebug() << "release" << endl;
   _loader->release();
   kdDebug() << "<- NSPluginInstance::~NSPluginInstance" << endl;
}


void NSPluginInstance::resizeEvent(QResizeEvent *event)
{
  EMBEDCLASS::resizeEvent(event);
  resizePlugin(width(), height());
  kdDebug() << "NSPluginInstance(client)::resizeEvent" << endl;
}


/*******************************************************************************/


NSPluginLoader::NSPluginLoader()
   : QObject(), _mapping(7, false), _viewer(0)
{
  scanPlugins();
  _mapping.setAutoDelete( true );

  // trap dcop register events
  kapp->dcopClient()->setNotifications(true);
  QObject::connect(kapp->dcopClient(),
                   SIGNAL(applicationRegistered(const QCString&)),
                   this, SLOT(applicationRegistered(const QCString&)));

  // load configuration
  KConfig cfg("kcmnspluginrc", false);
  cfg.setGroup("Misc");
  _useArtsdsp = cfg.readBoolEntry( "useArtsdsp", true );
}


NSPluginLoader *NSPluginLoader::instance()
{
  if (!s_instance)
    s_instance = new NSPluginLoader;

  s_refCount++;
  kdDebug() << "NSPluginLoader::instance -> " <<  s_refCount << endl;

  return s_instance;
}


void NSPluginLoader::release()
{
   s_refCount--;
   kdDebug() << "NSPluginLoader::release -> " <<  s_refCount << endl;

   if (s_refCount==0)
   {
      delete s_instance;
      s_instance = 0;
   }
}


NSPluginLoader::~NSPluginLoader()
{
   kdDebug() << "-> NSPluginLoader::~NSPluginLoader" << endl;
   unloadViewer();
   kdDebug() << "<- NSPluginLoader::~NSPluginLoader" << endl;
}


void NSPluginLoader::scanPlugins()
{
  QRegExp version(";version=[^:]*:");

  // open the cache file
  QFile cachef(locate("data", "nsplugins/cache"));
  if (!cachef.open(IO_ReadOnly)) {
      kdDebug() << "Could not load plugin cache file!" << endl;
      return;
  }

  QTextStream cache(&cachef);

  // read in cache
  QString line, plugin;
  while (!cache.atEnd()) {
      line = cache.readLine();
      if (line.isEmpty() || (line.left(1) == "#"))
        continue;

      if (line.left(1) == "[")
        {
          plugin = line.mid(1,line.length()-2);
          continue;
        }

      QStringList desc = QStringList::split(':', line, TRUE);
      QString mime = desc[0].stripWhiteSpace();
      QStringList suffixes = QStringList::split(',', desc[1].stripWhiteSpace());
      if (!mime.isEmpty())
        {
          // insert the mimetype -> plugin mapping
          _mapping.insert(mime, new QString(plugin));

          // insert the suffix -> mimetype mapping
          QStringList::Iterator suffix;
          for (suffix = suffixes.begin(); suffix != suffixes.end(); ++suffix) {

              // strip whitspaces and any preceeding '.'
              QString stripped = (*suffix).stripWhiteSpace();

              unsigned p=0;
              for ( ; p<stripped.length() && stripped[p]=='.'; p++ );
              stripped = stripped.right( stripped.length()-p );

              // add filetype to list
              if ( !stripped.isEmpty() && !_filetype.find(stripped) )
                  _filetype.insert( stripped, new QString(mime));
          }
        }
    }
}


QString NSPluginLoader::lookupMimeType(const QString &url)
{
  QDictIterator<QString> dit2(_filetype);
  while (dit2.current())
    {
      QString ext = QString(".")+dit2.currentKey();
      if (url.right(ext.length()) == ext)
        return *dit2.current();
      ++dit2;
    }
  return QString::null;
}


QString NSPluginLoader::lookup(const QString &mimeType)
{
    QString plugin;
    if (  _mapping[mimeType] )
        plugin = *_mapping[mimeType];

  kdDebug() << "Looking up plugin for mimetype " << mimeType << ": " << plugin << endl;

  return plugin;
}


bool NSPluginLoader::loadViewer()
{
   kdDebug() << "NSPluginLoader::loadViewer" << endl;

   _running = false;
   _process = new KProcess;

   // get the dcop app id
   int pid = (int)getpid();
   _dcopid.sprintf("nspluginviewer-%d", pid);

   connect( _process, SIGNAL(processExited(KProcess*)),
            this, SLOT(processTerminated(KProcess*)) );

   // find the external viewer process
   QString viewer = KGlobal::dirs()->findExe("nspluginviewer");
   if (!viewer)
   {
      kdDebug() << "can't find nspluginviewer" << endl;
      delete _process;
      return false;
   }

   // find the external artsdsp process
   if( _useArtsdsp ) {
       kdDebug() << "trying to use artsdsp" << endl;
       QString artsdsp = KGlobal::dirs()->findExe("artsdsp");
       if (!artsdsp)
       {
           kdDebug() << "can't find artsdsp" << endl;
       } else
       {
           kdDebug() << artsdsp << endl;
           *_process << artsdsp;
       }
   } else
       kdDebug() << "don't using artsdsp" << endl;

   *_process << viewer;

   // tell the process it's parameters
   *_process << "-dcopid";
   *_process << _dcopid;

   // run the process
   kdDebug() << "Running nspluginviewer" << endl;
   _process->start();

   // wait for the process to run
   int cnt = 0;
   while (!kapp->dcopClient()->isApplicationRegistered(_dcopid))
   {
      //kapp->processEvents(); // would lead to recursive calls in khtml
      sleep(1); kdDebug() << "sleep" << endl;
      cnt++;
      if (cnt >= 10)
      {
         kdDebug() << "timeout" << endl;
         delete _process;
         return false;
      }

      if (!_process->isRunning())
      {
         kdDebug() << "nspluginviewer terminated" << endl;
         delete _process;
         return false;
      }
   }

   // get viewer dcop interface
   _viewer = new NSPluginViewerIface_stub( _dcopid, "viewer" );

   return _viewer!=0;
}


void NSPluginLoader::unloadViewer()
{
   kdDebug() << "-> NSPluginLoader::unloadViewer" << endl;

   if ( _viewer )
   {
      _viewer->shutdown();
      kdDebug() << "Shutdown viewer" << endl;
      delete _viewer;
      delete _process;
      _viewer = 0;
      _process = 0;
   }

   kdDebug() << "<- NSPluginLoader::unloadViewer" << endl;
}


void NSPluginLoader::applicationRegistered( const QCString& appId )
{
   kdDebug() << "DCOP application " << appId.data() << " just registered!" << endl;

   if ( _dcopid==appId )
   {
      _running = true;
      kdDebug() << "plugin now running" << endl;
   }
}


void NSPluginLoader::processTerminated(KProcess *proc)
{
   if ( _process == proc)
   {
      kdDebug() << "Viewer process  terminated" << endl;
      delete _viewer;
      delete _process;
      _viewer = 0;
      _process = 0;
   }
}


NSPluginInstance *NSPluginLoader::newInstance(QWidget *parent, QString url,
                                              QString mimeType, bool embed,
                                              QStringList argn, QStringList argv,
                                              QString appId, QString callbackId )
{
   kdDebug() << "-> NSPluginLoader::NewInstance( parent=" << (void*)parent << ", url=" << url << ", mime=" << mimeType << ", ...)" << endl;

   if ( !_viewer )
   {
      // load plugin viewer process
      loadViewer();

      if ( !_viewer )
      {
         kdDebug() << "No viewer dcop stub found" << endl;
         return 0;
      }
   }

   // check the mime type
   QString mime = mimeType;
   if (mime.isEmpty())
   {
      mime = lookupMimeType( url );
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
      if (argn[i].lower() == "width") width = argv[i].toUInt();
      if (argn[i].lower() == "height") height = argv[i].toUInt();
   }

   // lookup plugin for mime type
   QString plugin_name = lookup(mime);
   if (plugin_name.isEmpty())
   {
      kdDebug() << "No suitable plugin" << endl;
      return 0;
   }

   // get plugin class object
   DCOPRef cls_ref = _viewer->newClass( plugin_name );
   if ( cls_ref.isNull() )
   {
      kdDebug() << "Couldn't create plugin class" << endl;
      return 0;
   }
   NSPluginClassIface_stub *cls = new NSPluginClassIface_stub( cls_ref.app(), cls_ref.object() );

   // handle special plugin cases
   if ( mime=="application/x-shockwave-flash" )
       embed = true; // flash doesn't work in full mode :(


   // get plugin instance
   DCOPRef inst_ref = cls->newInstance( url, mime, embed, argn, argv, appId, callbackId );
   if ( inst_ref.isNull() )
   {
      kdDebug() << "Couldn't create plugin instance" << endl;
      return 0;
   }

   NSPluginInstance *plugin = new NSPluginInstance( parent, inst_ref.app(),
                                                    inst_ref.object() );

   kdDebug() << "<- NSPluginLoader::NewInstance = " << (void*)plugin << endl;
   return plugin;
}

