#include "plugin_part.h"

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
  : KParts::ReadOnlyPart(parent, name)
{
  setInstance(PluginFactory::instance());
  
  // create a canvas to insert our widget
  QWidget *canvas = new QWidget(parent);
  canvas->setFocusPolicy(QWidget::ClickFocus);
  setWidget(canvas);
  
  m_extension = new PluginBrowserExtension(this);
  
  // as an example, display a blank white widget
  widget = new QLabel(canvas);
  widget->setText("plugin!");
  widget->setAutoResize(true);
  widget->show();
}


PluginPart::~PluginPart()
{
  closeURL();
}


bool PluginPart::openFile()
{
  widget->setText(m_file);
  
  return true;
}


bool PluginPart::closeURL()
{
  return true;
}


PluginBrowserExtension::PluginBrowserExtension(PluginPart *parent)
  : KParts::BrowserExtension(parent, "PluginBrowserExtension")
{
}


PluginBrowserExtension::~PluginBrowserExtension()
{
}
