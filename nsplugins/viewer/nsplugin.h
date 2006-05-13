/*

  This is an encapsulation of the  Netscape plugin API.

  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
                     Stefan Schimanski <1Stein@gmx.de>
  Copyright (c) 2003-2005 George Staikos <staikos@kde.org>

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


#ifndef __NS_PLUGIN_H__
#define __NS_PLUGIN_H__


#include <dcopobject.h>
#include "NSPluginClassIface.h"
#include "NSPluginCallbackIface_stub.h"


#include <QObject>
#include <QString>
#include <QStringList>
#include <q3ptrqueue.h>
#include <q3dict.h>
#include <QMap>
#include <q3intdict.h>
#include <QPointer>
//Added by qt3to4:
#include <Q3PtrList>

#include <kparts/browserextension.h>  // for URLArgs
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


class NSPluginStreamBase : public QObject
{
Q_OBJECT
friend class NSPluginInstance;
public:
  NSPluginStreamBase( class NSPluginInstance *instance );
  ~NSPluginStreamBase();

  KUrl url() { return _url; }
  int pos() { return _pos; }
  void stop();

Q_SIGNALS:
  void finished( NSPluginStreamBase *strm );

protected:
  void finish( bool err );
  bool pump();
  bool error() { return _error; }
  void queue( const QByteArray &data );
  bool create( const QString& url, const QString& mimeType, void *notify, bool forceNotify = false );
  int tries() { return _tries; }
  void inform( );

  class NSPluginInstance *_instance;
  uint16 _streamType;
  NPStream *_stream;
  void *_notifyData;
  KUrl _url;
  QString _fileURL;
  QString _mimeType;
  QByteArray _data;
  class KTempFile *_tempFile;

private:
  int process( const QByteArray &data, int start );

  unsigned int _pos;
  QByteArray _queue;
  int _queuePos;
  int _tries;
  bool _onlyAsFile;
  bool _error;
  bool _informed;
  bool _forceNotify;
};


class NSPluginStream : public NSPluginStreamBase
{
  Q_OBJECT

public:
  NSPluginStream( class NSPluginInstance *instance );
  ~NSPluginStream();

  bool get(const QString& url, const QString& mimeType, void *notifyData, bool reload = false);
  bool post(const QString& url, const QByteArray& data, const QString& mimeType, void *notifyData, const KParts::URLArgs& args);

protected Q_SLOTS:
  void data(KIO::Job *job, const QByteArray &data);
  void totalSize(KJob *job, qulonglong size);
  void mimetype(KIO::Job * job, const QString &mimeType);
  void result(KJob *job);
  void resume();

protected:
  QPointer<KIO::TransferJob> _job;
  QTimer *_resumeTimer;
};


class NSPluginBufStream : public NSPluginStreamBase
{
  Q_OBJECT

public:
  NSPluginBufStream( class NSPluginInstance *instance );
  ~NSPluginBufStream();

  bool get( const QString& url, const QString& mimeType, const QByteArray &buf, void *notifyData, bool singleShot=false );

protected Q_SLOTS:
  void timer();

protected:
  QTimer *_timer;
  bool _singleShot;
};


class NSPluginInstance : public QObject, public virtual NSPluginInstanceIface
{
  Q_OBJECT

public:

  // constructor, destructor
  NSPluginInstance( NPP privateData, NPPluginFuncs *pluginFuncs, KLibrary *handle,
		    int width, int height, QString src, QString mime,
                    QString appId, QString callbackId, bool embed,
		    QObject *parent, const char* name=0 );
  ~NSPluginInstance();

  // DCOP functions
  void shutdown();
  int winId() { return XtWindow(_form); }
  int setWindow(int remove=0);
  void resizePlugin(int w, int h);
  void javascriptResult(int id, QString result);
  void displayPlugin();

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
  void NPURLNotify(QString url, NPReason reason, void *notifyData);

  // Event handling
  uint16 HandleEvent(void *event);

  // signal emitters
  void emitStatus( const QString &message);
  void requestURL( const QString &url, const QString &mime,
		   const QString &target, void *notify, bool forceNotify = false, bool reload = false );
  void postURL( const QString &url, const QByteArray& data, const QString &mime,
             const QString &target, void *notify, const KParts::URLArgs& args, bool forceNotify = false );

  QString normalizedURL(const QString& url) const;

public Q_SLOTS:
  void streamFinished( NSPluginStreamBase *strm );

private Q_SLOTS:
  void timer();

private:
  friend class NSPluginStreamBase;

  static void forwarder(Widget, XtPointer, XEvent *, Boolean*);

  void destroy();

  bool _destroyed;
  bool _visible;
  bool _firstResize;
  void addTempFile(KTempFile *tmpFile);
  Q3PtrList<KTempFile> _tempFiles;
  NSPluginCallbackIface_stub *_callback;
  Q3PtrList<NSPluginStreamBase> _streams;
  KLibrary *_handle;
  QTimer *_timer;

  NPP      _npp;
  NPPluginFuncs _pluginFuncs;

  Widget _area, _form, _toplevel;
  QString _baseURL;
  int _width, _height;

  struct Request
  {
      // A GET request
      Request( const QString &_url, const QString &_mime,
	       const QString &_target, void *_notify, bool _forceNotify = false,
               bool _reload = false)
	  { url=_url; mime=_mime; target=_target; notify=_notify; post=false; forceNotify = _forceNotify; reload = _reload; }

      // A POST request
      Request( const QString &_url, const QByteArray& _data,
               const QString &_mime, const QString &_target, void *_notify,
               const KParts::URLArgs& _args, bool _forceNotify = false)
	  { url=_url; mime=_mime; target=_target;
            notify=_notify; post=true; data=_data; args=_args;
            forceNotify = _forceNotify; }

      QString url;
      QString mime;
      QString target;
      QByteArray data;
      bool post;
      bool forceNotify;
      bool reload;
      void *notify;
      KParts::URLArgs args;
  };

  NPWindow _win;
  NPSetWindowCallbackStruct _win_info;
  Q3PtrQueue<Request> _waitingRequests;
  QMap<int, Request*> _jsrequests;
};


class NSPluginClass : public QObject, virtual public NSPluginClassIface
{
  Q_OBJECT
public:

  NSPluginClass( const QString &library, QObject *parent, const char *name=0 );
  ~NSPluginClass();

  QString getMIMEDescription();
  DCOPRef newInstance(QString url, QString mimeType, bool embed,
		      QStringList argn, QStringList argv,
                      QString appId, QString callbackId, bool reload );
  void destroyInstance( NSPluginInstance* inst );
  bool error() { return _error; }

  void setApp(const QByteArray& app) { _app = app; }
  const QByteArray& app() const { return _app; }

protected Q_SLOTS:
  void timer();

private:
  int initialize();
  void shutdown();

  KLibrary *_handle;
  QString  _libname;
  bool _constructed;
  bool _error;
  QTimer *_timer;

  NP_GetMIMEDescriptionUPP *_NP_GetMIMEDescription;
  NP_InitializeUPP *_NP_Initialize;
  NP_ShutdownUPP *_NP_Shutdown;

  NPPluginFuncs _pluginFuncs;
  NPNetscapeFuncs _nsFuncs;

  Q3PtrList<NSPluginInstance> _instances;
  Q3PtrList<NSPluginInstance> _trash;

  QByteArray _app;
};


class NSPluginViewer : public QObject, virtual public NSPluginViewerIface
{
    Q_OBJECT
public:
   NSPluginViewer( DCOPCString dcopId, QObject *parent, const char *name=0 );
   virtual ~NSPluginViewer();

   void shutdown();
   DCOPRef newClass( QString plugin );

private Q_SLOTS:
   void appUnregistered(const QByteArray& id);

private:
   Q3Dict<NSPluginClass> _classes;
};


#endif
