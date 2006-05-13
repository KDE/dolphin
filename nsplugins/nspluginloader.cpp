/*

  This is an encapsulation of the  Netscape plugin API.


  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
                     Stefan Schimanski <1Stein@gmx.de>
  Copyright (c) 2002-2005 George Staikos <staikos@kde.org>

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/


#include <QDir>
#include <QGridLayout>
#include <QResizeEvent>


#include <kapplication.h>
#include <kprocess.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <dcopclient.h>
#include <dcopstub.h>
#include <QLayout>
#include <QObject>
#include <QPushButton>
#include <qx11embed_x11.h>
#include <QTextStream>
#include <QRegExp>

#include "nspluginloader.h"
#include "nspluginloader.moc"

#include "NSPluginClassIface_stub.h"

#include <config.h>

NSPluginLoader *NSPluginLoader::s_instance = 0;
int NSPluginLoader::s_refCount = 0;


NSPluginInstance::NSPluginInstance(QWidget *parent, const DCOPCString& app, const DCOPCString& id)
  : DCOPStub(app, id), NSPluginInstanceIface_stub(app, id), EMBEDCLASS(parent)
{
    _loader = 0L;
    shown = false;
    QGridLayout *_layout = new QGridLayout(this);
    _layout->setMargin(1);
    _layout->setSpacing(1);
    KConfig cfg("kcmnspluginrc", false);
    cfg.setGroup("Misc");
    if (cfg.readEntry("demandLoad", QVariant(false)).toBool()) {
        _button = new QPushButton(i18n("Start Plugin"), dynamic_cast<EMBEDCLASS*>(this));
        _layout->addWidget(_button, 0, 0);
        connect(_button, SIGNAL(clicked()), this, SLOT(doLoadPlugin()));
        show();
    } else {
        _button = 0L;
        doLoadPlugin();
    }
}


void NSPluginInstance::doLoadPlugin() {
    if (!_loader) {
        delete _button;
        _button = 0L;
        _loader = NSPluginLoader::instance();

        embedInto( NSPluginInstanceIface_stub::winId() );

        displayPlugin();
        show();
        shown = true;
    }
}


NSPluginInstance::~NSPluginInstance()
{
   kDebug() << "-> NSPluginInstance::~NSPluginInstance" << endl;
   shutdown();
   kDebug() << "release" << endl;
   _loader->release();
   kDebug() << "<- NSPluginInstance::~NSPluginInstance" << endl;
}


void NSPluginInstance::windowChanged(WId w)
{
    if (w == 0) {
        // FIXME: Put a notice here to tell the user that it crashed.
        repaint();
    }
}


void NSPluginInstance::resizeEvent(QResizeEvent *event)
{
  if (shown == false)
     return;
  EMBEDCLASS::resizeEvent(event);
  if (isVisible()) {
    resizePlugin(width(), height());
  }
  kDebug() << "NSPluginInstance(client)::resizeEvent" << endl;
}


/*******************************************************************************/


NSPluginLoader::NSPluginLoader()
   : QObject(), _mapping(7, false), _viewer(0)
{
  scanPlugins();
  _mapping.setAutoDelete( true );
  _filetype.setAutoDelete(true);

  // trap dcop register events
  kapp->dcopClient()->setNotifications(true);
  QObject::connect(kapp->dcopClient(),
                   SIGNAL(applicationRegistered(const QByteArray&)),
                   this, SLOT(applicationRegistered(const QByteArray&)));

  // load configuration
  KConfig cfg("kcmnspluginrc", false);
  cfg.setGroup("Misc");
  _useArtsdsp = cfg.readEntry( "useArtsdsp", QVariant(false )).toBool();
}


NSPluginLoader *NSPluginLoader::instance()
{
  if (!s_instance)
    s_instance = new NSPluginLoader;

  s_refCount++;
  kDebug() << "NSPluginLoader::instance -> " <<  s_refCount << endl;

  return s_instance;
}


void NSPluginLoader::release()
{
   s_refCount--;
   kDebug() << "NSPluginLoader::release -> " <<  s_refCount << endl;

   if (s_refCount==0)
   {
      delete s_instance;
      s_instance = 0;
   }
}


NSPluginLoader::~NSPluginLoader()
{
   kDebug() << "-> NSPluginLoader::~NSPluginLoader" << endl;
   unloadViewer();
   kDebug() << "<- NSPluginLoader::~NSPluginLoader" << endl;
}


void NSPluginLoader::scanPlugins()
{
  QRegExp version(";version=[^:]*:");

  // open the cache file
  QFile cachef(locate("data", "nsplugins/cache"));
  if (!cachef.open(QIODevice::ReadOnly)) {
      kDebug() << "Could not load plugin cache file!" << endl;
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

      QStringList desc = line.split(':', QString::KeepEmptyParts);
      QString mime = desc[0].trimmed();
      QStringList suffixes = desc[1].trimmed().split(',');
      if (!mime.isEmpty())
        {
          // insert the mimetype -> plugin mapping
          _mapping.insert(mime, new QString(plugin));

          // insert the suffix -> mimetype mapping
          QStringList::Iterator suffix;
          for (suffix = suffixes.begin(); suffix != suffixes.end(); ++suffix) {

              // strip whitspaces and any preceding '.'
              QString stripped = (*suffix).trimmed();

              int p=0;
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
  Q3DictIterator<QString> dit2(_filetype);
  while (dit2.current())
    {
      QString ext = QString(".")+dit2.currentKey();
      if (url.right(ext.length()) == ext)
        return *dit2.current();
      ++dit2;
    }
  return QString();
}


QString NSPluginLoader::lookup(const QString &mimeType)
{
    QString plugin;
    if (  _mapping[mimeType] )
        plugin = *_mapping[mimeType];

  kDebug() << "Looking up plugin for mimetype " << mimeType << ": " << plugin << endl;

  return plugin;
}


bool NSPluginLoader::loadViewer()
{
   kDebug() << "NSPluginLoader::loadViewer" << endl;

   _running = false;
   _process = new KProcess;

   // get the dcop app id
   int pid = (int)getpid();
   QString tmp;
   tmp.sprintf("nspluginviewer-%d",pid);
    _dcopid =tmp.toLatin1();

   connect( _process, SIGNAL(processExited(KProcess*)),
            this, SLOT(processTerminated(KProcess*)) );

   // find the external viewer process
   QString viewer = KGlobal::dirs()->findExe("nspluginviewer");
   if (viewer.isEmpty())
   {
      kDebug() << "can't find nspluginviewer" << endl;
      delete _process;
      return false;
   }

   // find the external artsdsp process
   if( !_useArtsdsp ) {
       kDebug() << "trying to use artsdsp" << endl;
       QString artsdsp = KGlobal::dirs()->findExe("artsdsp");
       if (artsdsp.isEmpty())
       {
           kDebug() << "can't find artsdsp" << endl;
       } else
       {
           kDebug() << artsdsp << endl;
           *_process << artsdsp;
       }
   } else
       kDebug() << "don't using artsdsp" << endl;

   *_process << viewer;

   // tell the process it's parameters
   *_process << "-dcopid";
   *_process << _dcopid;

   // run the process
   kDebug() << "Running nspluginviewer" << endl;
   _process->start();

   // wait for the process to run
   int cnt = 0;
   while (!kapp->dcopClient()->isApplicationRegistered(_dcopid))
   {
       //kapp->processEvents(); // would lead to recursive calls in khtml
#ifdef HAVE_USLEEP
       usleep( 50*1000 );
#else
      sleep(1); kDebug() << "sleep" << endl;
#endif
      cnt++;
#ifdef HAVE_USLEEP
      if (cnt >= 100)
#else
      if (cnt >= 10)
#endif
      {
         kDebug() << "timeout" << endl;
         delete _process;
         return false;
      }

      if (!_process->isRunning())
      {
         kDebug() << "nspluginviewer terminated" << endl;
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
   kDebug() << "-> NSPluginLoader::unloadViewer" << endl;

   if ( _viewer )
   {
      _viewer->shutdown();
      kDebug() << "Shutdown viewer" << endl;
      delete _viewer;
      delete _process;
      _viewer = 0;
      _process = 0;
   }

   kDebug() << "<- NSPluginLoader::unloadViewer" << endl;
}


void NSPluginLoader::applicationRegistered( const QByteArray& appId )
{
   kDebug() << "DCOP application " << appId.data() << " just registered!" << endl;

   if ( _dcopid==appId )
   {
      _running = true;
      kDebug() << "plugin now running" << endl;
   }
}


void NSPluginLoader::processTerminated(KProcess *proc)
{
   if ( _process == proc)
   {
      kDebug() << "Viewer process  terminated" << endl;
      delete _viewer;
      delete _process;
      _viewer = 0;
      _process = 0;
   }
}


NSPluginInstance *NSPluginLoader::newInstance(QWidget *parent, QString url,
                                              QString mimeType, bool embed,
                                              QStringList argn, QStringList argv,
                                              QString appId, QString callbackId, bool reload )
{
   kDebug() << "-> NSPluginLoader::NewInstance( parent=" << (void*)parent << ", url=" << url << ", mime=" << mimeType << ", ...)" << endl;

   if ( !_viewer )
   {
      // load plugin viewer process
      loadViewer();

      if ( !_viewer )
      {
         kDebug() << "No viewer dcop stub found" << endl;
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
      kDebug() << "Unknown MimeType" << endl;
      return 0;
   }

   // lookup plugin for mime type
   QString plugin_name = lookup(mime);
   if (plugin_name.isEmpty())
   {
      kDebug() << "No suitable plugin" << endl;
      return 0;
   }

   // get plugin class object
   DCOPRef cls_ref = _viewer->newClass( plugin_name );
   if ( cls_ref.isNull() )
   {
      kDebug() << "Couldn't create plugin class" << endl;
      return 0;
   }
   NSPluginClassIface_stub *cls = new NSPluginClassIface_stub( cls_ref.app(), cls_ref.object() );

   // handle special plugin cases
   if ( mime=="application/x-shockwave-flash" )
       embed = true; // flash doesn't work in full mode :(


   // get plugin instance
   DCOPRef inst_ref = cls->newInstance( url, mime, embed, argn, argv, appId, callbackId, reload );
   if ( inst_ref.isNull() )
   {
      kDebug() << "Couldn't create plugin instance" << endl;
      return 0;
   }

   NSPluginInstance *plugin = new NSPluginInstance( parent, inst_ref.app(),
                                                    inst_ref.object() );

   kDebug() << "<- NSPluginLoader::NewInstance = " << (void*)plugin << endl;

   return plugin;
}

// vim: ts=4 sw=4 et
