#ifndef __plugin_part_h__
#define __plugin_part_h__

#include <kparts/browserextension.h>
#include <kparts/factory.h>
#include <kparts/part.h>
#include <klibloader.h>
#include <qwidget.h>

class KAboutData;
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

  virtual void requestURL(QCString url, QCString target);

private:

  PluginPart *_part;

};


class PluginFactory : public KParts::Factory
{
  Q_OBJECT

public:

  PluginFactory();
  virtual ~PluginFactory();

  virtual KParts::Part * createPart(QWidget *parentWidget = 0, const char *widgetName = 0,
  		            	    QObject *parent = 0, const char *name = 0,
  			            const char *classname = "KParts::Part",
   			            const QStringList &args = QStringList());

  static KInstance *instance();
  static KAboutData *aboutData();

private:

  static KInstance *s_instance;
  class NSPluginLoader *_loader;
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
  PluginPart(QWidget *parentWidget, const char *widgetName, QObject *parent,
             const char *name, const QStringList &args = QStringList());
  virtual ~PluginPart();

  void requestURL(QCString url, QCString target);

protected:
  virtual bool openURL(const KURL &url);
  virtual bool closeURL();
  virtual bool openFile() { return false; };

protected slots:
  void pluginResized(int,int);
  void widgetDestroyed( NSPluginInstance *inst );

private:
  NSPluginInstance *_widget;
  QLabel *_errorLabel;
  PluginCanvasWidget *_canvas;
  PluginBrowserExtension *_extension;
  NSPluginCallback *_callback;
  QStringList _args;
  class NSPluginLoader *_loader;
  static int s_loaderRef;
};


#endif
