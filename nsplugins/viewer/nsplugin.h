/*

  This is an encapsulation of the  Netscape plugin API.


  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 
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


#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qdict.h>

#include <dcopobject.h>
#include "NSPluginClassIface.h"


#include <kio/job.h>


#define XP_UNIX

#include <npapi.h>


#include <X11/Intrinsic.h>


class KLibrary;


class NSPluginInstance : QObject
{
  Q_OBJECT

public:

  // constructor, destructor
  NSPluginInstance(NPP _privateData, KLibrary *handle, int width, int height);
  ~NSPluginInstance();

  // value handling
  NPError GetValue(NPPVariable variable, void *value);
  NPError SetValue(NPPVariable variable, void *value);

  // window handling
  NPError SetWindow(NPWindow *window);

  // stream functions
  NPError DestroyStream(NPStream *stream, NPReason reason);
  NPError NewStream(NPMIMEType type, NPStream *stream, NPBool seekable, uint16 *stype);
  void StreamAsFile(NPStream *stream, const char *fname);
  int32 Write(NPStream *stream, int32 offset, int32 len, void *buf);
  int32 WriteReady(NPStream *stream);

  // Event handling
  uint16 HandleEvent(void *event);
  
  // URL functions
  void URLNotify(const char *url, NPReason reason, void *notifyData);


  // signal emitters
  void emitStatus(const char *message) { emit status(message); };

  // window id
  int winId() { return XtWindow(_area); };

  NPError setWindow(bool remove=false);


signals:

  void status(const char *message);


private:
  
  void *func_GetValue, *func_SetValue, *func_SetWindow, 
    *func_DestroyStream, *func_NewStream, *func_StreamAsFile,
    *func_Write, *func_WriteReady, *func_URLNotify, *func_Destroy,
    *func_HandleEvent;

  NPP      _npp;
  KLibrary *_handle;

  Widget   _area;

  int _width, _height;

};


class NSPluginStream : public QObject
{
  Q_OBJECT

public:

  NSPluginStream(NSPluginInstance *instance);
  ~NSPluginStream();

  void get(QString url, QString mimeType);

  
private slots:

  void data(KIO::Job *job, const QByteArray &data);
  void result(KIO::Job *job);

private:

  NSPluginInstance *_instance; 
  KIO::TransferJob *_job;
  NPStream         *_stream;
  uint16           _streamType;

};


class NSPluginClass : virtual public NSPluginClassIface
{
public:

  // constructor, destructor
  NSPluginClass(const QString &library, QCString dcopId);
  ~NSPluginClass();

  // initialization method
  int Initialize();

  // inquire supported MIME types
  QString GetMIMEDescription();

  // shutdown method
  void Shutdown();

  int NewInstance(QString mimeType, int mode, QStringList argn, QStringList argv);

  NSPluginInstance *New(const char *mimeType, uint16 mode=NP_EMBED, int16 argc=0,
  		        char *argn[]=0, char *argv[]=0, NPSavedData *saved=0);
  

private:

  KLibrary *_handle;
  QString  _libname;

  void *func_Initialize, *func_GetMIMEDescription, *func_Shutdown,
    *func_New, *func_GetJavaClass;
    
};


#endif
