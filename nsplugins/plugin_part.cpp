#include "plugin_part.h"
#include "plugin_part.moc"


#include "nspluginloader.h"


#include <kapp.h>
#include <klocale.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <dcopclient.h>


#include <qlabel.h>


extern "C"
{
  /**
   * This function is the 'main' function of this part.  It takes
   * the form 'void *init_lib<library name>()  It always returns a
   * new factory object
   */
  void *init_libplugin()
  {
    return new PluginFactory;
  }
};



NSPluginCallback::NSPluginCallback(PluginPart *part)
  : DCOPObject()
{
  _part = part;
}


void NSPluginCallback::requestURL(QCString url)
{
  _part->requestURL(url);
}


/**
 * We need one static instance of the factory for our C 'main'
 * function
 */
KInstance *PluginFactory::s_instance = 0L;


PluginFactory::PluginFactory()
{
  kDebugInfo("PluginFactory");
}


PluginFactory::~PluginFactory()
{
  kDebugInfo("~PluginFactory");

  delete s_instance;
  s_instance = 0;
}


QObject *PluginFactory::create(QObject *parent, const char *name, const char*,
			       const QStringList& args)
{
  kDebugInfo("PluginFactory::create");
  QObject *obj = new PluginPart((QWidget*)parent, name, args);
  emit objectCreated(obj);
  return obj;
}


KInstance *PluginFactory::instance()
{
  kDebugInfo("PluginFactory::instance");
  if ( !s_instance )
    {
      KAboutData about("plugin", I18N_NOOP("plugin"), "1.99");
      s_instance = new KInstance(&about);
    }
  return s_instance;
}


NSPluginLoader *PluginPart::s_loader = 0L;
int PluginPart::s_loaderRef = 0;


PluginPart::PluginPart(QWidget *parent, const char *name,  const QStringList &args)
  : KParts::ReadOnlyPart(parent, name), _widget(0), _callback(0), _args(args)
{
  setInstance(PluginFactory::instance());
  kDebugInfo("PluginPart");

  _extension = new PluginBrowserExtension(this);

  // create loader
  if (!s_loader)
    s_loader = NSPluginLoader::instance();
  s_loaderRef++;

  // create a canvas to insert our widget
  _canvas = new PluginCanvasWidget(parent);
  _canvas->setFocusPolicy(QWidget::ClickFocus);
  _canvas->setBackgroundMode(QWidget::NoBackground);
  setWidget(_canvas);
  QObject::connect(_canvas, SIGNAL(resized(int,int)), this, SLOT(pluginResized(int,int)));
}


PluginPart::~PluginPart()
{
  kDebugInfo("~PluginPart");
  closeURL();

  s_loaderRef--;
  if (s_loaderRef==0)
  {
    delete s_loader;
    s_loader = 0L;
  }
}


bool PluginPart::openURL(const KURL &url)
{
  kDebugInfo("PluginPart::openURL");

  delete _widget;

  // handle arguments
  QStringList argn, argv;

  for ( QStringList::Iterator it = _args.begin(); it != _args.end(); )
  {
    int equalPos = (*it).find("=");
    if (equalPos>0)
    {
      QString name = (*it).left(equalPos-1);
      QString value = (*it).right((*it).length()-equalPos-1);
      if (value.at(0)=='\"') value = value.right(value.length()-1);
      if (value.at(value.length()-1)=='\"') value = value.left(value.length()-1);

      if (!name.isEmpty())
      {
      	argn << name;
      	argv << value;
      }
    }
  }

  if (!url.url().isEmpty())
  {
    argn << "SRC";
    argv << url.url();
  }

  if (!_extension->urlArgs().serviceType.isEmpty())
  {
    argn << "TYPE";
    argv << _extension->urlArgs().serviceType;
  }

  // create plugin widget
  _widget =  s_loader->NewInstance(_canvas, url.url(), _extension->urlArgs().serviceType, 1, argn, argv);
  if (_widget)
    {
      _widget->resize(_canvas->width(), _canvas->height());
      _widget->show();
    }

  // create plugin callback
  delete _callback;
  _callback = new NSPluginCallback(this);
  _widget->setCallback(kapp->dcopClient()->appId(), _callback->objId());

  return _widget != 0;
}


bool PluginPart::closeURL()
{
  kDebugInfo("PluginPart::closeURL");
  delete _widget;
  delete _callback;
  _widget = 0;
  _callback = 0;

  return true;
}


void PluginPart::requestURL(QCString url)
{
  emit _extension->openURLRequest( KURL( QString::fromLatin1( url ) ) );
}


void PluginPart::pluginResized(int w, int h)
{
  if (_widget)
    _widget->resize(w,h);

  kdDebug() << "PluginPart::pluginResized()" << endl;
}


PluginBrowserExtension::PluginBrowserExtension(PluginPart *parent)
  : KParts::BrowserExtension(parent, "PluginBrowserExtension")
{
}


PluginBrowserExtension::~PluginBrowserExtension()
{
}


void PluginCanvasWidget::resizeEvent(QResizeEvent *ev)
{
  QWidget::resizeEvent(ev);
  emit resized(width(), height());
}
