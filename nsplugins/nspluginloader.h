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
  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.

*/


#ifndef __NS_PLUGINLOADER_H__
#define __NS_PLUGINLOADER_H__


#include <qstring.h>
#include <qstringlist.h>
#include <qdict.h>
#include <qobject.h>
#include <qwidget.h>
#include <qxembed.h>

#include "NSPluginClassIface_stub.h"

#define EMBEDCLASS QXEmbed

class KProcess;
class QPushButton;
class QGridLayout;

class NSPluginInstance : public EMBEDCLASS, virtual public NSPluginInstanceIface_stub
{
  Q_OBJECT

public:
    NSPluginInstance(QWidget *parent, const QCString& app, const QCString& id);
    ~NSPluginInstance();

private slots:
    void doLoadPlugin();

protected:
    void resizeEvent(QResizeEvent *event);
    void windowChanged(WId w);
    class NSPluginLoader *_loader;
    bool shown;
    QPushButton *_button;
    QGridLayout *_layout;
};


class NSPluginLoader : public QObject
{
  Q_OBJECT

public:
  NSPluginLoader();
  ~NSPluginLoader();

  NSPluginInstance *newInstance(QWidget *parent,
				QString url, QString mimeType, bool embed,
				QStringList argn, QStringList argv,
                                QString appId, QString callbackId, bool reload );

  static NSPluginLoader *instance();
  void release();

protected:
  void scanPlugins();

  QString lookup(const QString &mimeType);
  QString lookupMimeType(const QString &url);

  bool loadViewer();
  void unloadViewer();

protected slots:
  void applicationRegistered( const QCString& appId );
  void processTerminated( KProcess *proc );

private:
  QStringList _searchPaths;
  QDict<QString> _mapping, _filetype;

  KProcess *_process;
  bool _running;
  QCString _dcopid;
  NSPluginViewerIface_stub *_viewer;
  bool _useArtsdsp;

  static NSPluginLoader *s_instance;
  static int s_refCount;
};


#endif
