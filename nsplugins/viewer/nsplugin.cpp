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


#include <stream.h>
#include <stdlib.h>


#include <qdir.h>
#include <qdict.h>
#include <qtimer.h>

#include "kxt.h"
#include "nsplugin.h"
#include "nsplugin.moc"
#include "resolve.h"

#include <klibloader.h>
#include <kdebug.h>
#include <kurl.h>
#include <kio/netaccess.h>
#include <ktempfile.h>

#include <X11/Intrinsic.h>
#include <Xm/DrawingA.h>


NSPluginInstance::NSPluginInstance(NPP _privateData, NPPluginFuncs *pluginFuncs, KLibrary *handle, int width, int height)
  : _handle(handle), _width(width), _height(height)
{
  _npp = (NPP) malloc(sizeof(NPP));
  *_npp = *_privateData;
  _npp->ndata = this;
  memcpy(&_pluginFuncs, pluginFuncs, sizeof(_pluginFuncs));

  kDebugInfo("NSPluginInstance::NSPluginInstance");
  kDebugInfo("pdata = %p", _npp->pdata);
  kDebugInfo("ndata = %p", _npp->ndata);

  // create drawing area
  Arg args[5];
  Cardinal nargs=0;
  XtSetArg(args[nargs], XmNwidth, width); nargs++;
  XtSetArg(args[nargs], XmNheight, height); nargs++;
  _area = XtCreateWidget("area", xmDrawingAreaWidgetClass, KXtApplication::toplevel, args, nargs);

  XtRealizeWidget(_area);
  XtMapWidget(_area);

  //  setWindow();
  setWindow();
} 


NSPluginInstance::~NSPluginInstance()
{
  _pluginFuncs.destroy(_npp, 0);

  ::free(_npp);

  setWindow(true);
  XtDestroyWidget(_area);

  return;
}


NPError NSPluginInstance::setWindow(bool remove)
{
  if (remove)
    {
      SetWindow(0);
      return NPERR_NO_ERROR;
    }

  NPWindow *win = new NPWindow;
  NPSetWindowCallbackStruct *win_info = new NPSetWindowCallbackStruct;

  win->x = 0;
  win->y = 0;
  win->height = _height;
  win->width = _width;
  win->type = NPWindowTypeWindow;

  // Well, the docu says sometimes, this is only used on the
  // MAC, but sometimes it says it's always. Who knows...
  NPRect clip;
  clip.top = 0;
  clip.left = 0;
  clip.bottom = _height;
  clip.right = _width;  
  win->clipRect = clip;

  win->window = (void*) XtWindow(_area);
  kdDebug() << "Window ID = " << win->window << endl;

  win_info->type = NP_SETWINDOW;
  win_info->display = qt_xdisplay();
  win_info->visual = (Visual*) DefaultVisual(qt_xdisplay(), DefaultScreen(qt_xdisplay()));
  win_info->colormap = DefaultColormap(qt_xdisplay(), DefaultScreen(qt_xdisplay()));
  win_info->depth = DefaultDepth(qt_xdisplay(), DefaultScreen(qt_xdisplay()));
  
  win->ws_info = win_info;

  NPError error = SetWindow(win);

  return error;
}


NPError NSPluginInstance::GetValue(NPPVariable variable, void *value)
{
  NPError error = _pluginFuncs.getvalue(_npp, variable, value);

  CHECK(GetValue,error);
}


NPError NSPluginInstance::SetValue(NPNVariable variable, void *value)
{
  NPError error = _pluginFuncs.setvalue(_npp, variable, value);

  CHECK(SetValue,error);
}


NPError NSPluginInstance::SetWindow(NPWindow *window)
{
  NPError error = _pluginFuncs.setwindow(_npp, window);

  CHECK(SetWindow,error);
}


NPError NSPluginInstance::DestroyStream(NPStream *stream, NPReason reason)
{
  NPError error = _pluginFuncs.destroystream(_npp, stream, reason);

  CHECK(DestroyStream,error);
}


NPError NSPluginInstance::NewStream(const NPMIMEType type, NPStream *stream, NPBool seekable, uint16 *stype)
{
  NPError error = _pluginFuncs.newstream(_npp, type, stream, seekable, stype);

  CHECK(NewStream,error);
}


void NSPluginInstance::StreamAsFile(NPStream *stream, const char *fname)
{
  _pluginFuncs.asfile(_npp, stream, fname);
}


int32 NSPluginInstance::Write(NPStream *stream, int32 offset, int32 len, void *buf)
{
  return _pluginFuncs.write(_npp, stream, offset, len, buf);
}


int32 NSPluginInstance::WriteReady(NPStream *stream)
{
  return _pluginFuncs.writeready(_npp, stream);
}


void NSPluginInstance::URLNotify(const char *url, NPReason reason, void *notifyData)
{
  _pluginFuncs.urlnotify(_npp, url, reason, notifyData);
}


NSPluginClass::NSPluginClass(const QString &library, const QCString dcopId)
  : DCOPObject(dcopId), _libname(library), _constructed(false),  _initialized(false)
{
  _handle = KLibLoader::self()->library(library);

  kDebugInfo("Library handle=%p", _handle);

  if (!_handle)
  {
    kDebugInfo("Could not dlopen %s", library.ascii());
    return;
  }

  _NP_GetMIMEDescription = (NP_GetMIMEDescriptionUPP *)_handle->symbol("NP_GetMIMEDescription");
  _NP_Initialize = (NP_InitializeUPP *)_handle->symbol("NP_Initialize");
  _NP_Shutdown = (NP_ShutdownUPP *)_handle->symbol("NP_Shutdown");

  if (!_NP_GetMIMEDescription)
  {
  	kDebugInfo("Could not get symbol NP_GetMIMEDescription");
    return;
  } else
	  	kDebugInfo("Resolved NP_GetMIMEDescription to %p", _NP_GetMIMEDescription);

  if (!_NP_Initialize)
  {
  	kDebugInfo("Could not get symbol NP_Initialize");
  	return;
  } else
  		kDebugInfo("Resolved NP_Initialize to %p", _NP_Initialize);

  if (!_NP_Shutdown)
  {
  	kDebugInfo("Could not get symbol NP_Shutdown");
  	return;
  } else
  		kDebugInfo("Resolved NP_Shutdown to %p", _NP_Shutdown);

  kDebugInfo("Plugin library %s loaded!", library.ascii());
  _constructed = true;

  Initialize();
}


NSPluginClass::~NSPluginClass()
{
  Shutdown();
  //KLibLoader::self()->unloadLibrary(_libname);
  delete _handle;
}



int NSPluginClass::Initialize()
{
  kDebugInfo("NSPluginInstance::Initialize()"); 
                                              
  if (!_constructed)
    return NPERR_GENERIC_ERROR;                             
                                              
  if (_initialized)
    kDebugError("FUNC ALREADY INITIALIZED!");

  memset(&_pluginFuncs, 0, sizeof(_pluginFuncs));
  memset(&_nsFuncs, 0, sizeof(_nsFuncs));

  _pluginFuncs.size = sizeof(_pluginFuncs);
  _nsFuncs.size = sizeof(_nsFuncs);
  _nsFuncs.version = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
  _nsFuncs.geturl = NPN_GetURL;
	_nsFuncs.posturl = NPN_PostURL;
	_nsFuncs.requestread = NPN_RequestRead;
  _nsFuncs.newstream = NPN_NewStream;
  _nsFuncs.write = NPN_Write;
  _nsFuncs.destroystream = NPN_DestroyStream;
  _nsFuncs.status = NPN_Status;
  _nsFuncs.uagent = NPN_UserAgent;
  _nsFuncs.memalloc = NPN_MemAlloc;
  _nsFuncs.memfree = NPN_MemFree;
  _nsFuncs.memflush = NPN_MemFlush;
  _nsFuncs.reloadplugins = NPN_ReloadPlugins;
  _nsFuncs.getJavaEnv = NPN_GetJavaEnv;
  _nsFuncs.getJavaPeer = NPN_GetJavaPeer;
  _nsFuncs.geturlnotify = NPN_GetURLNotify;
  _nsFuncs.posturlnotify = NPN_PostURLNotify;
  _nsFuncs.getvalue = NPN_GetValue;
  _nsFuncs.setvalue = NPN_SetValue;
  _nsFuncs.invalidaterect = NPN_InvalidateRect;
  _nsFuncs.invalidateregion = NPN_InvalidateRegion;
  _nsFuncs.forceredraw = NPN_ForceRedraw;
  
  NPError error = _NP_Initialize(&_nsFuncs, &_pluginFuncs);

  if (error==NPERR_NO_ERROR)
    _initialized = true;

  CHECK(Initialize,error);
}


QString NSPluginClass::GetMIMEDescription()
{
	return _NP_GetMIMEDescription();
}



void NSPluginClass::Shutdown()
{
	_NP_Shutdown();
}


int NSPluginClass::NewInstance(QString mimeType, int mode, QStringList argn, QStringList argv)
{
  QString src;

  // copy parameters over
  unsigned int argc = argn.count();
  char *_argn[argc], *_argv[argc];
  for (unsigned int i=0; i<argc; i++)
    {
      _argn[i] = strdup((const char*)argn[i].ascii());
      _argv[i] = strdup((const char*)argv[i].ascii());
      kdDebug() << "argn=" << _argn[i] << " argv=" << _argv[i] << endl;
      if (!stricmp(_argn[i], "src")) src = argv[i];
    }

  // create the instance
  NSPluginInstance *inst = New(mimeType.ascii(), mode, argc, _argn, _argv, 0);
  kdDebug() << "Instance: " << inst << endl;
  if (!inst)
    return 0;

  if (!src.isEmpty())
  {
    kdDebug() << "Starting src stream" << endl;
    NSPluginStream *s = new NSPluginStream( inst );
    _streams.append( s );
    s->get( src, mimeType );
  } else
      kdDebug() << "No src stream" << endl;

  return inst->winId();
}


NSPluginInstance *NSPluginClass::New(const char *mimeType, uint16 mode, int16 argc,
		           char *argn[], char *argv[], NPSavedData *saved)
{
  int width=300, height=300;
  NPP_t _npp;

  kdDebug() << "New(" << mimeType << "," << mode << "," << argc << endl;

  for (int i=0; i<argc; i++)
    {      
      kdDebug() << argn[i] << "=" << argv[i] << endl;

      if (!strcasecmp(argn[i], "WIDTH"))
	width = atoi(argv[i]);
      if (!strcasecmp(argn[i], "HEIGHT"))
	height = atoi(argv[i]);
    }

  char mime[256];
  strcpy(mime, mimeType);

  NPError error = _pluginFuncs.newp(mime, &_npp, mode, argc, argn, argv, saved);
  
  kDebugInfo("Result of NPP_New: %d", error);

  if (error == NPERR_NO_ERROR)
    return new NSPluginInstance(&_npp, &_pluginFuncs, _handle, width, height);

  return 0;
}



// server side functions -----------------------------------------------------

// allocate memory
void *NPN_MemAlloc(uint32 size)
{
  void *mem = ::malloc(size);

  kDebugInfo("NPN_MemAlloc(), size=%d allocated at %p", size, mem);

  return mem;
}


// free memory
void NPN_MemFree(void *ptr)
{
  kDebugInfo("NPN_MemFree() at %p", ptr);

  ::free(ptr);
}

uint32 NPN_MemFlush(uint32 /*size*/)
{
	kDebugInfo("NPN_MemFlush()");
	return 0;
}


// redraw
void NPN_ForceRedraw(NPP /*instance*/)
{
  kDebugInfo("NPN_ForceRedraw() [unimplemented]");
}


// invalidate rect
void NPN_InvalidateRect(NPP /*instance*/, NPRect */*invalidRect*/)
{
  kDebugInfo("NPN_InvalidateRect() [unimplemented]");
}


// invalidate region
void NPN_InvalidateRegion(NPP /*instance*/, NPRegion /*invalidRegion*/)
{
  kDebugInfo("NPN_InvalidateRegion() [unimplemented]");
}


// get value
NPError NPN_GetValue(NPP /*instance*/, NPNVariable variable, void *value)
{
  kDebugInfo("NPN_GetValue(), variable=%d", variable);

  switch (variable)
  {
  case NPNVxDisplay:
    *((struct _XDisplay**)value) = qt_xdisplay();
    return NPERR_NO_ERROR;
  case NPNVxtAppContext:
    value = XtDisplayToApplicationContext(qt_xdisplay());
    return NPERR_NO_ERROR;
#ifdef NP4
  case NPNVjavascriptEnabledBool:
    (bool)(*value) = false; // FIXME: Let's have a talk with Harri :-)
    return NPERR_NO_ERROR;
  case NPNVasdEnabledBool:
    (bool)(*value) = false; // FIXME: No clue what this means...
    return NPERR_NO_ERROR;
  case NPNVOfflineBool:
    (bool)(*value) = false;
    return NPERR_NO_ERROR;
#endif
  default:
    return NPERR_INVALID_PARAM;
  } 
}


// stream functions
NPError NPN_DestroyStream(NPP /*instance*/, NPStream /**stream*/, NPError /*reason*/)
{
  kDebugInfo("NPN_DestroyStream() [unimplemented]");

  return NPERR_GENERIC_ERROR;
}


NPError NPN_NewStream(NPP /*instance*/, NPMIMEType /*type*/, const char */*target*/, NPStream */*stream*/)
{
  kDebugInfo("NPN_NewStream() [unimplemented]");

  return NPERR_GENERIC_ERROR;
}


NPError NPN_RequestRead(NPStream */*stream*/, NPByteRange */*rangeList*/)
{
  kDebugInfo("NPN_RequestRead() [unimplemented]");

  return NPERR_GENERIC_ERROR;
}

NPError NPN_NewStream(NPP /*instance*/, NPMIMEType /*type*/,
                      const char* /*target*/, NPStream** /*stream*/)
{
  kDebugInfo("NPN_NewStream() [unimplemented]");

  return NPERR_GENERIC_ERROR;
}

int32 NPN_Write(NPP /*instance*/, NPStream */*stream*/, int32 /*len*/, void */*buf*/)
{
  kDebugInfo("NPN_Write() [unimplemented]");

  return 0;
}

NPError     NPN_DestroyStream(NPP /*instance*/, NPStream* /*stream*/,
						                  NPReason /*reason*/)
{
  kDebugInfo("NPN_DestroyStream() [unimplemented]");

  return NPERR_GENERIC_ERROR;
}

// URL functions
NPError NPN_GetURL(NPP /*instance*/, const char */*url*/, const char */*target*/)
{
  kDebugInfo("NPN_GetURL() [unimplemented]");

  return NPERR_GENERIC_ERROR;
}


NPError NPN_GetURLNotify(NPP /*instance*/, const char */*url*/, const char */*target*/,
												 void* /*notifyData*/)
{
  kDebugInfo("NPN_GetURLNotify() [unimplemented]");

  return NPERR_GENERIC_ERROR;
}


NPError NPN_PostURL(NPP /*instance*/, const char */*url*/, const char */*target*/,
										uint32 /*len*/, const char */*buf*/, NPBool /*file*/)
{
  kDebugInfo("NPN_PostURL() [unimplemented]");

  return NPERR_GENERIC_ERROR;
}


NPError NPN_PostURLNotify(NPP /*instance*/, const char */*url*/, const char */*target*/,
													uint32 /*len*/, const char */*buf*/, NPBool /*file*/, void */*notifyData*/)
{
  kDebugInfo("NPN_PostURL() [unimplemented]");

  return NPERR_GENERIC_ERROR;
}


// display status message
void NPN_Status(NPP instance, const char *message)
{
  kDebugInfo("NPN_Status(): %s", message);

  if (!instance)
    return;

  // turn into an instance signal
  NSPluginInstance *inst = (NSPluginInstance*) instance->ndata;

  inst->emitStatus(message);
}


// inquire user agent
const char *NPN_UserAgent(NPP /*instance*/)
{
  kDebugInfo("NPN_UserAgent()");

  // FIXME: Use the same as konqy
  return "Mozilla";
}


// inquire version information
void NPN_Version(int *plugin_major, int *plugin_minor, int *browser_major, int *browser_minor)
{
  kDebugInfo("NPN_Version()");

  // FIXME: Use the sensible values
  *browser_major = NP_VERSION_MAJOR;
  *browser_minor = NP_VERSION_MINOR;

  *plugin_major = NP_VERSION_MAJOR;
  *plugin_minor = NP_VERSION_MINOR;
}


void NPN_ReloadPlugins(NPBool /*reloadPages*/)
{
	kDebugInfo("NPN_ReloadPlugins() [unimplemented]");
}

// JAVA functions
JRIEnv *NPN_GetJavaEnv()
{
  // FIXME: Allow use of JAVA
  return 0;
}


jref NPN_GetJavaPeer(NPP /*instance*/)
{
  kDebugInfo("NPN_GetJavaPeer() [unimplemented]");

  return 0;
}


NPError NPN_SetValue(NPP /*instance*/, NPPVariable /*variable*/, void */*value*/)
{
  kDebugInfo("NPN_SetValue() [unimplemented]");

  return NPERR_GENERIC_ERROR;
}


NSPluginStream::NSPluginStream(NSPluginInstance *instance)
  : QObject(), _instance(instance), _job(0), _stream(0), _tempFile(0L), _pos(0), _queue(0), _queuePos(0)
{
  _resumeTimer = new QTimer( this );
  connect(_resumeTimer, SIGNAL(timeout()), this, SLOT(resume()));
}


NSPluginStream::~NSPluginStream()
{
  delete _job;

  if (_stream)
    {
      _instance->DestroyStream(_stream, NPRES_DONE);
      delete _stream;
    }

  if (_tempFile) delete _tempFile;
  if (_resumeTimer) delete _resumeTimer;
  if (_queue) delete _queue;
}


void NSPluginStream::get(QString url, QString mimeType)
{
  KURL src(url);
  // TODO: check url!

  // terminate existing job
  if (_job)
    {
      delete _job;
      _job = 0;
    }

  // terminate existing stream
  if (_stream)
    {
      _instance->DestroyStream(_stream, NPRES_DONE);
      delete _stream;
      _stream = 0;
    }

  // reset current position
  _pos = 0;

  // create new stream
  _stream = new NPStream;
  _stream->ndata = this;
  _stream->url = strdup(url.ascii());
  _stream->end = 0;

  // inform the plugin
  _instance->NewStream((char*)mimeType.ascii(), _stream, false, &_streamType);
  kDebugInfo("NewStream stype=%d", _streamType);

  // prepare data transfer
  _tempFile = 0L;

  if (_streamType == NP_ASFILEONLY)
    {
      // do an synchronous download

      // TODO: The file downloaded should be placed in konqys cache
      // and retrieved from there in repeated downloads!

      QString tmpFile;
      if(KIO::NetAccess::download(src, tmpFile))
	{
	  _instance->StreamAsFile(_stream, tmpFile.ascii());
	  _instance->DestroyStream(_stream, NPRES_DONE);
	  delete _stream;
	  _stream = 0;

	  KIO::NetAccess::removeTempFile( tmpFile );
	}    
      return;
    } else if (_streamType == NP_ASFILE)
    {
       _tempFile = new KTempFile( ); // TODO: keep file extension of original file
       _tempFile->setAutoDelete( TRUE );
    } else
    {
       kDebugWarning("Unknown stream type %d requested, trying NP_NORMAL", _streamType);
       _streamType = NP_NORMAL;
    }

  // start the kio job
  kDebugInfo("-> KIO::get( %s )", url.ascii());
  _job = KIO::get(url);
  connect(_job, SIGNAL(data(KIO::Job *, const QByteArray &)),
	  this, SLOT(data(KIO::Job *, const QByteArray &)));
  connect(_job, SIGNAL(result(KIO::Job *)),
	  this, SLOT(result(KIO::Job *)));
  kDebugInfo("<- KIO::get");
}



void NSPluginStream::data(KIO::Job */*job*/, const QByteArray &data)
{
  kDebugInfo("NSPluginStream::data");
  unsigned int pos = process( data, 0 );
  if (pos<data.size())
  {
    _job->suspend();

    _queue = new QByteArray( data );
    _queuePos = pos;
    _resumeTimer->start( 100, TRUE );
  }
}


void NSPluginStream::resume()
{
  kDebugInfo("NSPluginStream::resume");
  if (_queue)
  {
    kDebugInfo("queue found at pos %d, size %d", _queuePos, _queue->size());
    int pos = process( *_queue, _queuePos );
    _queuePos = pos;
    if (_queuePos>=_queue->size())
    {
      delete _queue;
      _queue = 0;
      _queuePos = 0;
    }
  }

  if (_queue)
  {
    _resumeTimer->start( 100, TRUE );
  } else
  {
    kDebugInfo("resume job");
    _job->resume();
  }
}


unsigned int NSPluginStream::process( const QByteArray &data, int start )
{
  int32 max, sent, to_sent, len;
  char *d = data.data()+start;

  to_sent = data.size()-start;
  while (to_sent>0)
    {
      max = _instance->WriteReady(_stream);
      len = QMIN(max, to_sent);

      kDebugInfo("-> Feeding stream to plugin: offset=%d, len=%d", _pos, len);
      sent = _instance->Write(_stream, _pos, len, d);
      kDebugInfo("<- Feeding stream: sent = %d", sent);

      if (_tempFile)
      {
      	kDebugInfo("Write to temp file");
      	//fwrite(fstream, d, sent);
      	_tempFile->dataStream()->writeBytes( d, sent );
      }
      
      to_sent -= sent;
      _pos += sent;
      d += sent;


      if (sent==0) // interrupt the stream for a few ms
        break;
    }

  return data.size()-to_sent;
}

void NSPluginStream::result(KIO::Job */*job*/)
{
  kDebugInfo("NSPluginStream::result");

  if (_tempFile)
  {
    _tempFile->close();
    _instance->StreamAsFile(_stream, _tempFile->name().ascii());
    _instance->DestroyStream(_stream, NPRES_DONE);
    delete _tempFile;
    _tempFile = 0;
  } else
      _instance->DestroyStream(_stream, NPRES_DONE);
  delete _stream;
  _stream = 0;
}



/**
 * setWindow - tells the plugin about its drawing window
 *
 * This function is used to initally tell the plugin about its window,
 * to report any changes in window size or position, and finally
 * to tell that the plugin has no more window to draw on.
Ü*
 */
/*
NPError setWindow(bool remove)
{
  if (remove)
    {
      _instance->SetWindow(0);
      return NPERR_NO_ERROR;
    }

  NPWindow *win = new NPWindow;
  NPSetWindowCallbackStruct *win_info = new NPSetWindowCallbackStruct;

  win->x = 0;
  win->y = 0;
  win->height = height;
  win->width = width;

  // Well, the docu says sometimes, this is only used on the
  // MAC, but sometimes it says it always. Who knows...
  NPRect clip;
  clip.top = 0;
  clip.left = 0;
  clip.bottom = height;
  clip.right = width;  
  win->clipRect = clip;

  win->window = (void*) XtWindow(area);

  win_info->type = NP_SETWINDOW;
  win_info->display = dpy;
  win_info->visual = (Visual*) DefaultVisual(dpy, DefaultScreen(dpy));
  win_info->colormap = DefaultColormap(dpy, DefaultScreen(dpy));
  win_info->depth = DefaultDepth(dpy, DefaultScreen(dpy));
  
  win->ws_info = win_info;

  NPError error = _instance->SetWindow(win);

  // embed into the containing widget
  if (embedId)
    {
      kDebugInfo("Embedding into window %d", embedId);
      XReparentWindow(dpy, XtWindow(toplevel), embedId, 0, 0);
    }

  return error;
}
*/
