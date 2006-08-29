/*
  Netscape Plugin Loader KPart

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


#ifndef __plugin_part_h__
#define __plugin_part_h__

#include <kparts/browserextension.h>
#include <kparts/factory.h>
#include <kparts/part.h>
#include <klibloader.h>
#include <QWidget>
#include <QPointer>

class KAboutData;
class KInstance;
class PluginBrowserExtension;
class PluginLiveConnectExtension;
class QLabel;
class NSPluginInstance;
class PluginPart;

class PluginFactory : public KParts::Factory
{
  Q_OBJECT

public:
  PluginFactory();
  virtual ~PluginFactory();

  virtual KParts::Part * createPartObject(QWidget *parentWidget = 0, QObject *parent = 0,
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

  PluginCanvasWidget(QWidget *parent=0)
    : QWidget(parent) {}

protected:
  void resizeEvent(QResizeEvent *e);

Q_SIGNALS:
  void resized(int,int);
};


class PluginPart: public KParts::ReadOnlyPart
{
  Q_OBJECT
public:
  PluginPart(QWidget *parentWidget, QObject *parent,
             const QStringList &args = QStringList());
  virtual ~PluginPart();

  // CallBack interface (DBus-exported)
  void postURL(const QString& url, const QString& target, const QByteArray& data, const QString& mime);
  void requestURL(const QString& url, const QString& target);
  void statusMessage( QString msg );
  void evalJavaScript( int id, const QString& script );
  void reloadPage();

  void changeSrc(const QString& url);

protected:
  virtual bool openUrl(const KUrl &url);
  virtual bool closeUrl();
  virtual bool openFile() { return false; };

protected Q_SLOTS:
  void pluginResized(int,int);
  void saveAs();

private:
  QPointer<QWidget> _widget;
  PluginCanvasWidget *_canvas;
  PluginBrowserExtension *_extension;
  PluginLiveConnectExtension *_liveconnect;
  QStringList _args;
  class NSPluginLoader *_loader;
  bool *_destructed;
};


class PluginLiveConnectExtension : public KParts::LiveConnectExtension
{
Q_OBJECT
public:
    PluginLiveConnectExtension(PluginPart* part);
    virtual ~PluginLiveConnectExtension();
    virtual bool put(const unsigned long, const QString &field, const QString &value);
    virtual bool get(const unsigned long, const QString&, Type&, unsigned long&, QString&);
    virtual bool call(const unsigned long, const QString&, const QStringList&, Type&, unsigned long&, QString&);

    QString evalJavaScript( const QString & script );

Q_SIGNALS:
    void partEvent( const unsigned long objid, const QString & event, const KParts::LiveConnectExtension::ArgList & args );

private:
    PluginPart *_part;
    QString *_retval;
};


#endif
