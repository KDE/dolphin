/*

  This is an encapsulation of the  Netscape plugin API.

  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
                     Stefan Schimanski <1Stein@gmx.de>
 
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
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/                                                                            


#ifndef __NS_PLUGIN_H__
#define __NS_PLUGIN_H__


#include <dcopobject.h>
#include "NSPluginClassIface.h"
#include "NSPluginCallbackIface_stub.h"


#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qdict.h>
#include <qintdict.h>


#include <kio/job.h>


#define XP_UNIX
#include "sdk/npupp.h"

typedef char* NP_GetMIMEDescriptionUPP(void);
typedef NPError NP_InitializeUPP(NPNetscapeFuncs*, NPPluginFuncs*);
typedef NPError NP_ShutdownUPP(void);


#include <X11/Intrinsic.h>


void quitXt();

class KLibrary;
class QTimer;


class NSPluginStream : public QObject
{
  Q_OBJECT

public:

  NSPluginStream( class NSPluginInstance *instance );
  ~NSPluginStream();

  void get(QString url, QString mimeType, void *notifyData);

private slots:

  void data(KIO::Job *job, const QByteArray &data);
  void result(KIO::Job *job);

  void resume();

private:

  unsigned int process( const QByteArray &data, int start );

  class NSPluginInstance *_instance;
  KIO::TransferJob *_job;
  NPStream         *_stream;
  uint16           _streamType;
  void             *_notifyData;
  QString          _url;

  class KTempFile  *_tempFile;

  unsigned int	   _pos;
  QTimer	   *_resumeTimer;
  const QByteArray *_queue;
  unsigned int	   _queuePos;
};


class NSPluginInstance : public QObject, public virtual NSPluginInstanceIface
{
  Q_OBJECT

public:

  // constructor, destructor
  NSPluginInstance( NPP privateData, NPPluginFuncs *pluginFuncs, KLibrary *handle, 
		    int width, int height, QString src, QString mime,
		    QObject *parent );
  ~NSPluginInstance();

  // value handling
  NPError NPGetValue(NPPVariable variable, void *value);
  NPError NPSetValue(NPNVariable variable, void *value);

  // window handling
  NPError NPSetWindow(NPWindow *window);

  // stream functions
  NPError NPDestroyStream(NPStream *stream, NPReason reason);
  NPError NPNewStream(NPMIMEType type, NPStream *stream, NPBool seekable, uint16 *stype);
  void NPStreamAsFile(NPStream *stream, const char *fname);
  int32 NPWrite(NPStream *stream, int32 offset, int32 len, void *buf);
  int32 NPWriteReady(NPStream *stream);  
  
  // URL functions
  void NPURLNotify(const char *url, NPReason reason, void *notifyData);

  // Event handling
  uint16 HandleEvent(void *event);

  // signal emitters
  void emitStatus(const char *message) { emit status(message); };

  // window handling
  int winId() { return XtWindow(_toplevel); };
  int setWindow(int remove=0);
  void resizePlugin(int w, int h);

  void setCallback( QCString app, QCString obj );
  void requestURL( QCString url, QCString target, void *notify );

  void destroyStream( NSPluginStream *strm );

  void destroyPlugin();

signals:

  void status(const char *message);

private slots:
  void shutdown();

private:
  void destroy();

  friend class NSPluginStream;

  bool _destroyed;
  void addTempFile(KTempFile *tmpFile);
  QList<KTempFile> _tempFiles;
  NSPluginCallbackIface_stub *_callback;
  QList<NSPluginStream> _streams;
  KLibrary *_handle;
  QTimer *_shutdownTimer;

  NPP      _npp;
  NPPluginFuncs _pluginFuncs;

  Widget _area, _form, _toplevel;
  QString _src;
  int _width, _height;
};


class NSPluginClass : public QObject, virtual public NSPluginClassIface
{
public:

  // constructor, destructor
  NSPluginClass( const QString &library, QObject *parent );
  ~NSPluginClass();

  // initialization method
  int Initialize();

  // inquire supported MIME types
  QString GetMIMEDescription();

  // shutdown method
  void Shutdown();

  DCOPRef NewInstance(QString url, QString mimeType, int mode, QStringList argn, QStringList argv);
  
private:
  KLibrary *_handle;
  QString  _libname;
  bool _constructed, _initialized;

  NP_GetMIMEDescriptionUPP *_NP_GetMIMEDescription;
  NP_InitializeUPP *_NP_Initialize;
  NP_ShutdownUPP *_NP_Shutdown;

  NPPluginFuncs _pluginFuncs;
  NPNetscapeFuncs _nsFuncs;
};


class NSPluginViewer : public QObject, virtual public NSPluginViewerIface
{
public:
   NSPluginViewer( QCString dcopId, QObject *parent );
   virtual ~NSPluginViewer();

   void Shutdown();
   DCOPRef NewClass( QString plugin );

private:
   QDict<NSPluginClass> _classes;
};


#endif
