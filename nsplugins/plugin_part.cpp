#include "plugin_part.h"
#include "plugin_part.moc"

#include "nspluginloader.h"

#include <kapp.h>
#include <klocale.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <dcopclient.h>

#include <qlabel.h>


class PluginBrowserExtension : public KParts::BrowserExtension
{
  friend class PluginPart;
public:
  PluginBrowserExtension( KParts::ReadOnlyPart *parent,
                          const char *name = 0L )
     : KParts::BrowserExtension( parent, name ) {}
  ~PluginBrowserExtension() {}
};



extern "C"
{
  /**
   * This function is the 'main' function of this part.  It takes
   * the form 'void *init_lib<library name>()  It always returns a
   * new factory object
   */
  void *init_libnsplugin()
  {
    return new PluginFactory;
  }
};


NSPluginCallback::NSPluginCallback(PluginPart *part)
  : DCOPObject()
{
  _part = part;
}


void NSPluginCallback::requestURL(QCString url, QCString target)
{
  _part->requestURL(url, target);
}


/**
 * We need one static instance of the factory for our C 'main'
 * function
 */
KInstance *PluginFactory::s_instance = 0L;


PluginFactory::PluginFactory()
{
  kdDebug() << "PluginFactory::PluginFactory" << endl;
  s_instance = 0;

  // preload plugin loader
  _loader = NSPluginLoader::instance();
}


PluginFactory::~PluginFactory()
{
   kdDebug() << "PluginFactory::~PluginFactory" << endl;

  _loader->release();

  if ( s_instance )
  {
    delete s_instance->aboutData();
    delete s_instance;
  }
  s_instance = 0;
}

KParts::Part * PluginFactory::createPart(QWidget *parentWidget, const char *widgetName,
  	                  	         QObject *parent, const char *name,
  			                 const char */*classname*/, const QStringList &args)
{
  kdDebug() << "PluginFactory::create" << endl;
  KParts::Part *obj = new PluginPart(parentWidget, widgetName, parent, name, args);
  emit objectCreated(obj);
  return obj;
}


KInstance *PluginFactory::instance()
{
  kdDebug() << "PluginFactory::instance" << endl;

  if ( !s_instance )
      s_instance = new KInstance( aboutData() );
  return s_instance;
}

KAboutData *PluginFactory::aboutData()
{
  KAboutData *about = new KAboutData("plugin", I18N_NOOP("plugin"), "1.99");
  return about;
} 

PluginPart::PluginPart(QWidget *parentWidget, const char *widgetName, QObject *parent,
                       const char *name, const QStringList &args)
  : KParts::ReadOnlyPart(parent, name), _widget(0), _callback(0), _args(args)
{
  setInstance(PluginFactory::instance());
  kdDebug() << "PluginPart::PluginPart" << endl;

  // we have to keep the class name of KParts::PluginBrowserExtension to let khtml find it
  _extension = (PluginBrowserExtension *)new KParts::BrowserExtension(this);

  // create loader
  _loader = NSPluginLoader::instance();

  // create a canvas to insert our widget
  _canvas = new PluginCanvasWidget( parentWidget, widgetName );
  _canvas->setFocusPolicy( QWidget::ClickFocus );
  _canvas->setBackgroundMode( QWidget::NoBackground );
  setWidget(_canvas);
  QObject::connect( _canvas, SIGNAL(resized(int,int)), this, SLOT(pluginResized(int,int)) );
}


PluginPart::~PluginPart()
{
  kdDebug() << "PluginPart::~PluginPart" << endl;
  closeURL();

  _loader->release();
  kdDebug() << "----------------------------------------------------" << endl;
}


bool PluginPart::openURL(const KURL &url)
{
   kdDebug() << "-> PluginPart::openURL" << endl;

   m_url = url;
   emit setWindowCaption( url.prettyURL() );

   // delete _widget; _widget = 0;
   QString surl = url.url();
   QString smime = _extension->urlArgs().serviceType;

   // handle arguments
   QStringList argn, argv;

   QStringList::Iterator it = _args.begin();
   for ( ; it != _args.end(); )
   {
      kdDebug() << "arg=" << *it << endl;

      int equalPos = (*it).find("=");
      if (equalPos>0)
      {
	 QString name = (*it).left(equalPos).upper();
	 QString value = (*it).right((*it).length()-equalPos-1);
	 if (value.at(0)=='\"') value = value.right(value.length()-1);
	 if (value.at(value.length()-1)=='\"') value = value.left(value.length()-1);

	 kdDebug() << "name=" << name << " value=" << value << endl;

	 if (!name.isEmpty())
	 {
	    argn << name;
	    argv << value;
	 }

	 if (surl.isEmpty() && (name=="SRC" || name=="DATA" || name=="MOVIE"))
	 {
	    kdDebug() << "found src=" << value << endl;
	    surl = value;
	 }
      }

      it++;
   }

   if (surl.isEmpty())
   {
      kdDebug() << "<- PluginPart::openURL - false (no url passed to nsplugin)" << endl;
      return false;
   }

   if (argn.contains("SRC")==0)
   {
      kdDebug() << "adding SRC=" << surl << endl;
      argn << "SRC";
      argv << surl;
   }

   if (!smime.isEmpty() && argn.contains("TYPE")==0)
   {
      kdDebug() << "adding TYPE=" << smime << endl;
      argn << "TYPE";
      argv << smime;
   }

   // create plugin widget
   _widget = _loader->NewInstance( _canvas, surl, smime, 1, argn, argv );
   if ( _widget )
   {
      _widget->resize( _canvas->width(), _canvas->height() );
      _widget->show();
      connect( _widget, SIGNAL(destroyed(NSPluginInstance *)), 
	       this, SLOT(widgetDestroyed(NSPluginInstance *)) );

      // create plugin callback
      delete _callback;
      _callback = new NSPluginCallback(this);
      _widget->setCallback(kapp->dcopClient()->appId(), _callback->objId());
   }    
  
   kdDebug() << "<- PluginPart::openURL = " << (int)(_widget!=0) << endl;
   return _widget!=0;
}


bool PluginPart::closeURL()
{
  kdDebug() << "PluginPart::closeURL" << endl;
  delete _widget;
  delete _callback;
  _widget = 0;
  _callback = 0;

  return true;
}


void PluginPart::requestURL(QCString url, QCString target)
{
  kdDebug() << "PluginPart::requestURL( url=" << url << ", target=" << target << endl;
  KURL new_url(this->url(), url);
  KParts::URLArgs args;
  args.frameName = target;
  emit _extension->openURLRequest( new_url, args );
}


void PluginPart::widgetDestroyed( NSPluginInstance *inst )
{
   kdDebug() << "PluginPart::widgetDestroyed" << endl;
   _widget = 0;
   closeURL();
}


void PluginPart::pluginResized(int w, int h)
{
  if ( _widget )
    _widget->resize(w,h);

  kdDebug() << "PluginPart::pluginResized()" << endl;
}


void PluginCanvasWidget::resizeEvent(QResizeEvent *ev)
{
  QWidget::resizeEvent(ev);
  emit resized(width(), height());
}
