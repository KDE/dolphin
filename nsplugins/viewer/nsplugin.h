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
#include <qqueue.h>
#include <qdict.h>
#include <qintdict.h>
#include <qguardedptr.h>

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

public:
  NSPluginStreamBase( class NSPluginInstance *instance );
  ~NSPluginStreamBase();

  KURL url() { return _url; };
  int pos() { return _pos; };
  void stop();

signals:
  void finished( NSPluginStreamBase *strm );

protected:
  void finish( bool err );
  bool pump();
  bool error() { return _error; };
  void queue( const QByteArray &data );
  bool create( QString url, QString mimeType, void *notify );
  int tries() { return _tries; };
  void inform( );

  class NSPluginInstance *_instance;
  uint16 _streamType;
  NPStream *_stream;
  void *_notifyData;
  QString _url;
  QString _fileURL;
  QString _mimeType;
  class KTempFile *_tempFile;

private:
  int process( const QByteArray &data, int start );

  unsigned int _pos;
  QByteArray _queue;
  unsigned int _queuePos;
  int _tries;
  bool _onlyAsFile;
  bool _error;
  bool _informed;
};


class NSPluginStream : public NSPluginStreamBase
{
  Q_OBJECT

public:
  NSPluginStream( class NSPluginInstance *instance );
  ~NSPluginStream();

  bool get(QString url, QString mimeType, void *notifyData);

protected slots:
  void data(KIO::Job *job, const QByteArray &data);
  void totalSize(KIO::Job *job, unsigned long size);
  void mimetype(KIO::Job * job, const QString &mimeType);
  void result(KIO::Job *job);
  void resume();

protected:
  QGuardedPtr<KIO::TransferJob> _job;
  QTimer *_resumeTimer;
};


class NSPluginBufStream : public NSPluginStreamBase
{
  Q_OBJECT

public:
  NSPluginBufStream( class NSPluginInstance *instance );
  ~NSPluginBufStream();

  bool get( QString url, QString mimeType, const QByteArray &buf, void *notifyData, bool singleShot=false );

protected slots:
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
                    QString appId, QString callbackId,
		    QObject *parent, const char* name=0 );
  ~NSPluginInstance();

  // DCOP functions
  void shutdown();
  int winId() { return XtWindow(_toplevel); };
  int setWindow(int remove=0);
  void resizePlugin(int w, int h);

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
		   const QString &target, void *notify );

public slots:
  void streamFinished( NSPluginStreamBase *strm );

private slots:
  void timer();

private:
  friend class NSPluginStreamBase;

  void destroy();

  bool _destroyed;
  void addTempFile(KTempFile *tmpFile);
  QList<KTempFile> _tempFiles;
  NSPluginCallbackIface_stub *_callback;
  QList<NSPluginStreamBase> _streams;
  KLibrary *_handle;
  QTimer *_timer;

  NPP      _npp;
  NPPluginFuncs _pluginFuncs;

  Widget _area, _form, _toplevel;
  QString _baseURL;
  int _width, _height;

  struct Request
  {
      Request( const QString &_url, const QString &_mime,
	       const QString &_target, void *_notify)
	  { url=_url; mime=_mime; target=_target; notify=_notify; };

      QString url;
      QString mime;
      QString target;
      void *notify;
  };

  QQueue<Request> _waitingRequests;
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
                      QString appId, QString callbackId );
  void destroyInstance( NSPluginInstance* inst );
  bool error() { return _error; };

protected slots:
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

  QList<NSPluginInstance> _instances;
  QList<NSPluginInstance> _trash;
};


class NSPluginViewer : public QObject, virtual public NSPluginViewerIface
{
    Q_OBJECT
public:
   NSPluginViewer( QCString dcopId, QObject *parent, const char *name=0 );
   virtual ~NSPluginViewer();

   void shutdown();
   DCOPRef newClass( QString plugin );

private:
   QDict<NSPluginClass> _classes;
};


#endif
