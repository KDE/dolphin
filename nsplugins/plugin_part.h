#ifndef __plugin_part_h__
#define __plugin_part_h__

#include <kparts/browserextension.h>
#include <klibloader.h>
#include <qwidget.h>


class KInstance;
class PluginBrowserExtension;
class QLabel;
class NSPluginInstance;
class PluginPart;


#include "NSPluginCallbackIface.h"


class NSPluginCallback : public NSPluginCallbackIface
{
public:

  NSPluginCallback(PluginPart *part);

  void requestURL(QCString url);


private:

  PluginPart *_part;

};


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


class PluginCanvasWidget : public QWidget
{
  Q_OBJECT

public:

  PluginCanvasWidget(QWidget *parent=0, const char *name=0) 
    : QWidget(parent,name) {};
    

protected:

  void resizeEvent(QResizeEvent *e);

  
signals:

  void resized(int,int);

};


class PluginPart: public KParts::ReadOnlyPart
{
  Q_OBJECT
    
public:

  PluginPart(QWidget *parent, const char *name);
  virtual ~PluginPart();

  void requestURL(QCString url);

  
protected:

  virtual bool openURL(const KURL &url);
  virtual bool closeURL();
  virtual bool openFile() { return false; }; 
  

protected slots:
    
  void pluginResized(int,int);


private:

  NSPluginInstance *widget;
  PluginCanvasWidget *canvas;
  PluginBrowserExtension *m_extension;
  NSPluginCallback *callback;
  static class NSPluginLoader *s_loader;
  static int s_loaderRef;
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
