#ifndef __plugin_part_h__
#define __plugin_part_h__

#include <kparts/browserextension.h>
#include <klibloader.h>

class KInstance;
class PluginBrowserExtension;
class QLabel;

class PluginFactory : public KLibFactory
{
  Q_OBJECT

public:

  PluginFactory();
  virtual ~PluginFactory();
  
  virtual QObject* create(QObject* parent = 0, const char* name = 0,
			  const char* classname = "QObject",
			  const QStringList &args = QStringList());
  
  static KInstance *instance();


private:

  static KInstance *s_instance;

};


class PluginPart: public KParts::ReadOnlyPart
{
  Q_OBJECT
    
public:

  PluginPart(QWidget *parent, const char *name);
  virtual ~PluginPart();

  
protected:

  virtual bool openURL(const KURL &url);
  virtual bool closeURL();
  virtual bool openFile() { return false; }; 


private:

  QWidget *widget, *canvas;
  PluginBrowserExtension *m_extension;

};


class PluginBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT
  
  friend class PluginPart;

public:
  
  PluginBrowserExtension(PluginPart *parent);
  virtual ~PluginBrowserExtension();
};


#endif
