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
NSPluginLoader *PluginFactory::s_loader = 0L;

PluginFactory::PluginFactory()
{
  kDebugInfo("PluginFactory");
}


PluginFactory::~PluginFactory()
{
  kDebugInfo("~PluginFactory");

  if (s_loader)
    delete s_loader;

  if (s_instance)
    delete s_instance;

  s_instance = 0;
}


QObject *PluginFactory::create(QObject *parent, const char *name, const char*,
			       const QStringList& )
{
  kDebugInfo("PluginFactory::create");
  QObject *obj = new PluginPart((QWidget*)parent, name);
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


NSPluginLoader *PluginFactory::loader()
{
  if (!s_loader)
    s_loader = NSPluginLoader::instance();

  return s_loader;
}


PluginPart::PluginPart(QWidget *parent, const char *name)
  : KParts::ReadOnlyPart(parent, name), widget(0), callback(0)
{
  setInstance(PluginFactory::instance());
  kDebugInfo("PluginPart");

  m_extension = new PluginBrowserExtension(this);

  // create a canvas to insert our widget
  canvas = new PluginCanvasWidget(parent);
  canvas->setFocusPolicy(QWidget::ClickFocus);
  canvas->setBackgroundMode(QWidget::NoBackground);
  setWidget(canvas);
  QObject::connect(canvas, SIGNAL(resized(int,int)), this, SLOT(pluginResized(int,int)));
}


PluginPart::~PluginPart()
{
  kDebugInfo("~PluginPart");
  closeURL();
}


bool PluginPart::openURL(const KURL &url)
{
  kDebugInfo("PluginPart::openURL");

  if (widget) delete widget;

  QStringList _argn, _argv;
  _argn << "SRC" << "TYPE";
  _argv << url.url() << m_extension->urlArgs().serviceType;
  widget =  PluginFactory::loader()->NewInstance(canvas, url.url(), m_extension->urlArgs().serviceType, 1, _argn, _argv);

  if (widget)
    {
      widget->resize(canvas->width(), canvas->height());
      widget->show();
    }

  if (callback) delete callback;
  callback = new NSPluginCallback(this);
  widget->setCallback(kapp->dcopClient()->appId(), callback->objId());

  return widget != 0;
}


bool PluginPart::closeURL()
{
  kDebugInfo("PluginPart::closeURL");
  if (widget) delete widget;
  if (callback) delete callback;
  widget = 0;
  callback = 0;

  return true;
}


void PluginPart::requestURL(QCString url)
{
  emit m_extension->openURLRequest( KURL( QString::fromLatin1( url ) ) );
}


void PluginPart::pluginResized(int w, int h)
{
  if (widget)
    widget->resize(w,h);

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
