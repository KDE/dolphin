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


#include <stream.h>
#include <stdlib.h>


#include <qdir.h>
#include <qdict.h>


#include "nsplugin.h"
#include "nsplugin.moc"


#include <klibloader.h>
#include <kdebug.h>
#include <kurl.h>
#include <kio/netaccess.h>


#include <X11/Intrinsic.h>


#include "resolve.h"


#include <Xm/DrawingA.h>


// the topmost widget
extern Widget toplevel;


NSPluginInstance::NSPluginInstance(NPP _privateData, KLibrary *handle, int width, int height)
  : _handle(handle), _width(width), _height(height)
{
  func_GetValue = 0;
  func_SetValue = 0;
  func_SetWindow = 0;
  func_DestroyStream = 0;
  func_NewStream = 0;
  func_StreamAsFile = 0;
  func_Write = 0;
  func_WriteReady = 0; 
  func_URLNotify = 0;
  func_Destroy = 0;
  func_HandleEvent = 0;

  _npp = (NPP) malloc(sizeof(NPP));
  *_npp = *_privateData;
  _npp->ndata = this;

  kDebugInfo("NSPluginInstance::NSPluginInstance");
  kDebugInfo("pdata = %p", _npp->pdata);
  kDebugInfo("ndata = %p", _npp->ndata);

  // create drawing area
  Arg args[5];
  Cardinal nargs=0;
  XtSetArg(args[nargs], XmNwidth, width); nargs++;
  XtSetArg(args[nargs], XmNheight, height); nargs++;
  _area = XtCreateWidget("area", xmDrawingAreaWidgetClass, toplevel, args, nargs);

  XtRealizeWidget(_area);
  XtMapWidget(_area);

  //  setWindow();
} 


NSPluginInstance::~NSPluginInstance()
{
  RESOLVE_VOID(Destroy);

  NPError (*fp) (NPP, NPSavedData **); 
  fp = (NPError (*)(NPP, NPSavedData **)) func_Destroy;

  fp(_npp, 0);

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
  RESOLVE(GetValue);

  NPError (*fp) (void *, NPPVariable, void *); 
  fp = (NPError (*)(void *, NPPVariable, void *)) func_GetValue;
  NPError error = fp(_npp, variable, value);

  CHECK(GetValue,error);
}


NPError NSPluginInstance::SetValue(NPPVariable variable, void *value)
{
  RESOLVE(SetValue);

  NPError (*fp) (void *, NPPVariable, void *); 
  fp = (NPError (*)(void *, NPPVariable, void *)) func_SetValue;

  NPError error = fp(_npp, variable, value);

  CHECK(SetValue,error);
}


NPError NSPluginInstance::SetWindow(NPWindow *window)
{
  RESOLVE(SetWindow);
  
  NPError (*fp) (void *, NPWindow *); 
  fp = (NPError (*)(void *, NPWindow *)) func_SetWindow;

  NPError error = fp(_npp, window);

  CHECK(SetWindow,error);
}


NPError NSPluginInstance::DestroyStream(NPStream *stream, NPReason reason)
{
  RESOLVE(DestroyStream);

  NPError (*fp) (NPP, NPStream*, NPReason); 
  fp = (NPError (*)(NPP, NPStream*, NPReason)) func_DestroyStream;
  
  NPError error = fp(_npp, stream, reason);

  CHECK(DestroyStream,error);
}


NPError NSPluginInstance::NewStream(const NPMIMEType type, NPStream *stream, NPBool seekable, uint16 *stype)
{
  RESOLVE(NewStream);

  NPError (*fp) (NPP, NPMIMEType, NPStream*, NPBool, uint16*);
  fp = (NPError (*)(NPP, NPMIMEType, NPStream*, NPBool, uint16*)) func_NewStream;
  
  NPError error = fp(_npp, type, stream, seekable, stype);

  CHECK(NewStream,error);
}


void NSPluginInstance::StreamAsFile(NPStream *stream, const char *fname)
{
  RESOLVE_VOID(StreamAsFile);

  NPError (*fp) (NPP, NPStream*, const char*);
  fp = (NPError (*)(NPP, NPStream*, const char*)) func_StreamAsFile;

  fp(_npp, stream, fname);
}


int32 NSPluginInstance::Write(NPStream *stream, int32 offset, int32 len, void *buf)
{
  RESOLVE_RETVAL(Write,0);

  int32 (*fp) (NPP, NPStream*, int32, int32, void*);
  fp = (int32 (*)(NPP, NPStream*, int32, int32, void*)) func_Write;
  
  return fp(_npp, stream, offset, len, buf);
}


int32 NSPluginInstance::WriteReady(NPStream *stream)
{
  RESOLVE_RETVAL(WriteReady,0);

  int32 (*fp) (NPP, NPStream*);
  fp = (int32 (*)(NPP, NPStream*)) func_WriteReady;
  
  return fp(_npp, stream);
}


void NSPluginInstance::URLNotify(const char *url, NPReason reason, void *notifyData)
{
  RESOLVE_VOID(URLNotify);

  void (*fp) (NPP, const char*, NPReason, void*);
  fp = (void (*)(NPP, const char*, NPReason, void*)) func_URLNotify;

  fp(_npp, url, reason, notifyData);
}


NSPluginClass::NSPluginClass(const QString &library, const QCString dcopId)
  : DCOPObject(dcopId), _libname(library)
{
  func_Initialize = 0; 
  func_GetMIMEDescription = 0; 
  func_Shutdown = 0;
  func_New = 0;
  func_GetJavaClass = 0;

  _handle = KLibLoader::self()->library(library);

  kDebugInfo("Library handle=%p", _handle);

  if (!_handle)
  {
    kDebugInfo("Could not dlopen %s", library.ascii());
    return;
  }

  kDebugInfo("Plugin library %s loaded!", library.ascii());

  Initialize();
}


NSPluginClass::~NSPluginClass()
{
  Shutdown();
  KLibLoader::self()->unloadLibrary(_libname);
}



int NSPluginClass::Initialize()
{
  //  RESOLVE(Initialize);
  kDebugInfo("NSPluginInstance::Initialize()"); 
                                              
  if (!_handle)                               
    return NPERR_GENERIC_ERROR;                             
                                              
  if (!func_Initialize)                        
    func_Initialize = _handle->symbol("NPP_Initialize");
  else                                        
    kDebugError("FUNC ALREADY INITIALIZED!");
                                              
  if (!func_Initialize)                        
  {                                           
    kDebugInfo("Failed: NPP_Initialize could not be resolved"); 
    return NPERR_GENERIC_ERROR;                             
  }                                           
  kDebugInfo("Resolved NPP_Initialize to %p", func_Initialize);

  NPError (*fp)();
  fp = (NPError (*)()) func_Initialize;
  
  NPError error = fp();

  CHECK(Initialize,error);
}


QString NSPluginClass::GetMIMEDescription()
{
  RESOLVE_RETVAL(GetMIMEDescription,0);

  char *(*fp)();
  fp = (char *(*)()) func_GetMIMEDescription;

  return fp();
}



void NSPluginClass::Shutdown()
{
  RESOLVE_VOID(Shutdown);

  void (*fp)();
  fp = (void (*)()) func_Shutdown;

  fp();
}


int NSPluginClass::NewInstance(QString mimeType, int mode, QStringList argn, QStringList argv)
{
  // copy parameters over
  unsigned int argc = argn.count();
  char *_argn[argc], *_argv[argc];
  for (unsigned int i=0; i<argc; i++)
    {
      _argn[i] = strdup((const char*)argn[i].ascii());
      _argv[i] = strdup((const char*)argv[i].ascii());
      kdDebug() << "argn=" << _argn[i] << " argv=" << _argv[i] << endl;
    }

  // create the instance
  NSPluginInstance *inst = New(mimeType.ascii(), mode, argc, _argn, _argv, 0);
  kdDebug() << "Instance: " << inst << endl;
  if (!inst)
    return 0;


  NSPluginStream stream(inst);
  stream.get("file:/build/kdelibs/khtml/nsplugins/caldera.xpm", "image/x-xpixmap");


  return inst->winId();
}


NSPluginInstance *NSPluginClass::New(const char *mimeType, uint16 mode, int16 argc,
		           char *argn[], char *argv[], NPSavedData *saved)
{
  int width=300, height=300;

  RESOLVE_RETVAL(New,0);

  NPP_t _npp;
   
  NPError (*fp) (NPMIMEType, NPP, uint16, int16, char *[], char *[], NPSavedData *); 
  fp = (NPError (*)(NPMIMEType, NPP, uint16, int16, char *[], char *[], NPSavedData *)) func_New;

  kdDebug() << "New(" << mimeType << "," << mode << "," << argc << endl;

  for (unsigned int i=0; i<argc; i++)
    {      
      kdDebug() << argn[i] << "=" << argv[i] << endl;

      if (!strcasecmp(argn[i], "WIDTH"))
	width = atoi(argv[i]);
      if (!strcasecmp(argn[i], "HEIGHT"))
	height = atoi(argv[i]);
    }

  NPError error = fp(mimeType, &_npp, mode, argc, argn, argv, saved);
  
  kDebugInfo("Result of NPP_New: %d", error);

  if (error == NPERR_NO_ERROR)
    return new NSPluginInstance(&_npp, _handle, width, height);
  
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


// redraw
void NPN_ForceRedraw(NPP instance)
{
  kDebugInfo("NPN_ForceRedraw() [unimplemented]");
}


#ifdef NP4
// invalidate rect
void NPN_InvalidateRect(NPP instance, NP_Rect *invalidRect)
{
  kDebugInfo("NPN_InvalidateRect() [unimplemented]");
}


// invalidate region
void NPN_InvalidateRegion(NPP instance, NP_Region *invalidRegion)
{
  kDebugInfo("NPN_InvalidateRegion() [unimplemented]");
}
#endif


// get value
NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value)
{
  kDebugInfo("NPN_GetValue(), variable=%d", variable);

  switch (variable)
  {
  case NPNVxDisplay:
    *((char **)value) = ":0";
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


int32 NPN_Write(NPP /*instance*/, NPStream */*stream*/, int32 /*len*/, void */*buf*/)
{
  kDebugInfo("NPN_Write() [unimplemented]");

  return 0;
}


// URL functions
NPError NPN_GetURL(NPP /*instance*/, const char */*url*/, const char */*target*/)
{
  kDebugInfo("NPN_GetURL() [unimplemented]");

  return NPERR_GENERIC_ERROR;
}


NPError NPN_GetURLNotify(NPP instance, const char *url, const char *target)
{
  kDebugInfo("NPN_GetURLNotify() [unimplemented]");

  return NPERR_GENERIC_ERROR;
}


NPError NPN_PostURL(NPP instance, const char *url, const char *target, uint32 len, const char *buf, NPBool file)
{
  kDebugInfo("NPN_PostURL() [unimplemented]");

  return NPERR_GENERIC_ERROR;
}


NPError NPN_PostURLNotify(NPP instance, const char *url, const char *target, uint32 len, const char *buf, NPBool file, void *notifyData)
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
const char *NPN_UserAgent(NPP instance)
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
  *browser_major = 0;
  *browser_minor = 9;

  *plugin_major = NP_VERSION_MAJOR;
  *plugin_minor = NP_VERSION_MINOR;
}


// JAVA functions
JRIEnv *NPN_GetJavaEnv()
{
  // FIXME: Allow use of JAVA
  return 0;
}


jref NPN_GetJavaPeer(NPP instance)
{
  kDebugInfo("NPN_GetJavaPeer() [unimplemented]");

  return 0;
}


NSPluginStream::NSPluginStream(NSPluginInstance *instance)
  : QObject(), _instance(instance), _job(0), _stream(0)
{
}


NSPluginStream::~NSPluginStream()
{
  delete _job;

  if (_stream)
    {
      _instance->DestroyStream(_stream, NPRES_DONE);
      delete _stream;
    }
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

  // create new stream
  _stream = new NPStream;
  _stream->ndata = this;
  _stream->url = strdup(url.ascii());
  _stream->end = 0;

  // inform the plugin
  _instance->NewStream((char*)mimeType.ascii(), _stream, false, &_streamType);
  kDebugInfo("NewStream stype=%d", _streamType);

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
    }

  // TODO: Implement NP_ASFILE!

  // start the kio job
  _job = KIO::get(url);
  connect(_job, SIGNAL(data(KIO::Job *, const QByteArray &)),
	  this, SLOT(data(KIO::Job *, const QByteArray &)));
  connect(_job, SIGNAL(result(KIO::Job *)),
	  this, SLOT(result(KIO::Job *)));
}


void NSPluginStream::data(KIO::Job *job, const QByteArray &data)
{
  int32 avail, ready, sent, to_sent;

  avail = data.size();
  sent = 0;
  while (sent < avail)
    {
      ready = _instance->WriteReady(_stream);
      to_sent = QMIN(ready, (avail-sent));

      kDebugInfo("Feeding stream to plugin: offset=%d, len=%d", sent, to_sent);
      _instance->Write(_stream, sent, to_sent, data.data());
      
      sent += to_sent;
    }
}

void NSPluginStream::result(KIO::Job *job)
{
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
