#include "plugin_part.h"
#include "plugin_part.moc"


#include "nspluginloader.h"


#include <kinstance.h>
#include <klocale.h>
#include <kaboutdata.h>

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


/**
 * We need one static instance of the factory for our C 'main'
 * function
 */
KInstance *PluginFactory::s_instance = 0L;

PluginFactory::PluginFactory()
{
}


PluginFactory::~PluginFactory()
{
  if (s_instance)
    delete s_instance;
  
  s_instance = 0;
}


QObject *PluginFactory::create(QObject *parent, const char *name, const char*,
			       const QStringList& )
{
  QObject *obj = new PluginPart((QWidget*)parent, name);
  emit objectCreated(obj);
  return obj;
}


KInstance *PluginFactory::instance()
{
  if ( !s_instance )
    {
      KAboutData about("plugin", I18N_NOOP("plugin"), "1.99");
      s_instance = new KInstance(&about);
    }
  return s_instance;
}


PluginPart::PluginPart(QWidget *parent, const char *name)
  : KParts::ReadOnlyPart(parent, name), widget(0)
{
  setInstance(PluginFactory::instance());
  
  m_extension = new PluginBrowserExtension(this);
 
  // create a canvas to insert our widget
  canvas = new PluginCanvasWidget(parent);
  canvas->setFocusPolicy(QWidget::ClickFocus);
  setWidget(canvas);
  QObject::connect(canvas, SIGNAL(resized(int,int)),
		   this, SLOT(pluginResized(int,int)));
}


PluginPart::~PluginPart()
{
  closeURL();
}


bool PluginPart::openURL(const KURL &url)
{
  delete widget;

  NSPluginLoader *loader = NSPluginLoader::instance();

  QStringList _argn, _argv;
  _argn << "SRC";
  _argv << url.url();
  widget = loader->NewInstance(canvas, url.url(), m_extension->urlArgs().serviceType, 1, _argn, _argv);
  
  if (widget)
    {
      widget->resize(canvas->width(), canvas->height());
      widget->show();
    }

  return widget != 0;
}


bool PluginPart::closeURL()
{
  delete widget;
  widget = 0;

  return true;
}


void PluginPart::pluginResized(int w, int h)
{
  if (widget)
    {
      widget->resize(w,h);
    }
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
