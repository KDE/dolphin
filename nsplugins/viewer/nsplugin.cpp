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


#include "NSPluginCallbackIface_stub.h"


#include <stdlib.h>
#include <unistd.h>

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
#include <dcopclient.h>


#include <X11/Intrinsic.h>
#include <X11/Composite.h>
#include <X11/Constraint.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <Xm/DrawingA.h>



NSPluginInstance::NSPluginInstance(NPP privateData, NPPluginFuncs *pluginFuncs, 
				   KLibrary *handle, int width, int height, 
				   QString src, QString mime, QObject *parent )
   : QObject( parent ), DCOPObject(), _destroyed(false),
     _callback(0), _handle(handle), _width(width), _height(height)
{
   _npp = privateData;
   _npp->ndata = this;
   _src = src;
   memcpy(&_pluginFuncs, pluginFuncs, sizeof(_pluginFuncs));
   _tempFiles.setAutoDelete( true );
   _streams.setAutoDelete( true );
   _shutdownTimer = new QTimer( this );
   connect( _shutdownTimer, SIGNAL(timeout()), this, SLOT(shutdown()) );

   kdDebug() << "NSPluginInstance::NSPluginInstance" << endl;
   kdDebug() << "pdata = " << _npp->pdata << endl;
   kdDebug() << "ndata = " << _npp->ndata << endl;

   // create drawing area
   Arg args[5];
   Cardinal nargs=0;
   XtSetArg(args[nargs], XtNwidth, width); nargs++;
   XtSetArg(args[nargs], XtNheight, height); nargs++;

   String n, c;
   XtGetApplicationNameAndClass(qt_xdisplay(), &n, &c);

   _toplevel = XtAppCreateShell("shell", c, topLevelShellWidgetClass,
				qt_xdisplay(), args, nargs);

   // What exactly does widget mapping mean? Without this call the widget isn't
   // embedded correctly. With it the viewer doesn't show anything in standalone mode.
   XtSetMappedWhenManaged(_toplevel, False);
   XtRealizeWidget(_toplevel);

   // Create form window that is searched for by flash plugin
   _form = XtCreateManagedWidget("form", compositeWidgetClass, _toplevel, args, nargs);
   XtRealizeWidget(_form);

   // Create widget that is passed to the plugin        	
   _area = XmCreateDrawingArea(_form, (char*)("drawingArea"), args, nargs);
   //_area = XtCreateManagedWidget( "drawingArea", constraintWidgetClass, _form, args, nargs );
   XtRealizeWidget(_area);
   XtMapWidget(_area);

   setWindow();

   // create source stream
   if (!src.isEmpty())
   {
      kdDebug() << "Starting src stream" << endl;
      NSPluginStream *s = new NSPluginStream( this );
      _streams.append( s );
      s->get( src, mime, 0 );
   } else
      kdDebug() << "No src stream" << endl;
} 


NSPluginInstance::~NSPluginInstance()
{
   kdDebug() << "-> ~NSPluginInstance" << endl;   

   destroy();   
   ::free(_npp);

   kdDebug() << "<- ~NSPluginInstance" << endl;
}

void NSPluginInstance::shutdown()
{
   kdDebug() << "-> NSPluginInstance::shutdown" << endl;
   delete this;
   kdDebug() << "<- NSPluginInstance::shutdown" << endl;
}

void NSPluginInstance::destroy()
{
   kdDebug() << "-> NSPluginInstance::destroy" << endl;
   if ( !_destroyed )
   {
      _destroyed = true;

      kdDebug() << "delete streams" << endl;
      _streams.clear();

      kdDebug() << "delete callbacks" << endl;
      delete _callback;
      _callback = 0;

      setWindow(true);

      kdDebug() << "destroy plugin" << endl;
      if ( _pluginFuncs.destroy )
	 _pluginFuncs.destroy( _npp, 0 );

      XtDestroyWidget(_area);
      XtDestroyWidget(_form);
      XtDestroyWidget(_toplevel);
   }

   kdDebug() << "<- NSPluginInstance::destroy" << endl;
}

void NSPluginInstance::destroyPlugin()
{
   kdDebug() << "-> NSPluginInstance::destroyPlugin" << endl;
   destroy();
   _shutdownTimer->start( 0, TRUE );
   kdDebug() << "<- NSPluginInstance::destroyPlugin" << endl;
}


void NSPluginInstance::requestURL( QCString url, QCString target, void *notify )
{
   if ( !target.isEmpty() )
   {
      if (_callback)
      { 
	 _callback->requestURL( url, target ); 
	 if ( notify ) NPURLNotify( url, NPRES_DONE, notify );
      }
   } else
   {
      if (!url.isEmpty())
      {
	 kdDebug() << "Starting new stream" << endl;

         NSPluginStream *s = new NSPluginStream( this );
         _streams.append( s );	 	 

	 // make absolute url
	 if ( KURL::isRelativeURL(url) )
	 {
	    KURL absUrl( _src, url );
	    s->get( absUrl.url(), QString::null, notify );
	 } else
	    s->get( url, QString::null, notify );
      } else
      {
	 kdDebug() << "No src stream" << endl;      
	 if ( notify ) NPURLNotify( url, NPRES_NETWORK_ERR, notify );
      }
   }
}


void NSPluginInstance::destroyStream( NSPluginStream *strm )
{   
   kdDebug() << "-> NSPluginInstance::destroyStream" << endl;
   _streams.remove( strm );
   delete strm;
   kdDebug() << "<- NSPluginInstance::destroyStream" << endl;
}


int NSPluginInstance::setWindow(int remove)
{  
   if (remove)
   {
      NPSetWindow(0);
      return NPERR_NO_ERROR;
   }
   
   kdDebug() << "-> NSPluginInstance::setWindow" << endl;

   NPWindow win;
   NPSetWindowCallbackStruct win_info;

   win.x = 0;
   win.y = 0;
   win.height = _height;
   win.width = _width;
   win.type = NPWindowTypeWindow;

   // Well, the docu says sometimes, this is only used on the
   // MAC, but sometimes it says it's always. Who knows...
   NPRect clip;
   clip.top = 0;
   clip.left = 0;
   clip.bottom = _height;
   clip.right = _width;  
   win.clipRect = clip;

   win.window = (void*) XtWindow(_area);
   kdDebug() << "Window ID = " << win.window << endl;

   win_info.type = NP_SETWINDOW;
   win_info.display = XtDisplay( _area );
   win_info.visual = DefaultVisualOfScreen( XtScreen(_area) );
   win_info.colormap = DefaultColormapOfScreen( XtScreen(_area) );
   win_info.depth = DefaultDepthOfScreen( XtScreen(_area) );
  
   win.ws_info = &win_info;

   NPError error = NPSetWindow( &win );

   kdDebug() << "<- NSPluginInstance::setWindow = " << error << endl;
   return error;
}


void NSPluginInstance::resizePlugin(int w, int h)
{
   kdDebug() << "-> NSPluginInstance::resizePlugin( w=" << w << ", h=" << h << " ) " << endl;

   _width = w;
   _height = h;

   Arg args[5];
   Cardinal nargs=0;
   XtSetArg(args[nargs], XtNwidth, _width); nargs++;
   XtSetArg(args[nargs], XtNheight, _height); nargs++;

   XtSetValues(_form, args, nargs);
   XtSetValues(_area, args, nargs);

   setWindow();

   kdDebug() << "<- NSPluginInstance::resizePlugin" << endl;
}


NPError NSPluginInstance::NPGetValue(NPPVariable variable, void *value)
{
   if (!_pluginFuncs.getvalue)
      return NPERR_GENERIC_ERROR;

   NPError error = _pluginFuncs.getvalue(_npp, variable, value);

   CHECK(GetValue,error);
}


NPError NSPluginInstance::NPSetValue(NPNVariable variable, void *value)
{
   if (!_pluginFuncs.setvalue)
      return NPERR_GENERIC_ERROR;

   NPError error = _pluginFuncs.setvalue(_npp, variable, value);

   CHECK(SetValue,error);
}


NPError NSPluginInstance::NPSetWindow(NPWindow *window)
{
   if (!_pluginFuncs.setwindow)
      return NPERR_GENERIC_ERROR;

   NPError error = _pluginFuncs.setwindow(_npp, window);

   CHECK(SetWindow,error);
}


NPError NSPluginInstance::NPDestroyStream(NPStream *stream, NPReason reason)
{
   if (!_pluginFuncs.destroystream)
      return NPERR_GENERIC_ERROR;

   NPError error = _pluginFuncs.destroystream(_npp, stream, reason);

   CHECK(DestroyStream,error);
}


NPError NSPluginInstance::NPNewStream(const NPMIMEType type, NPStream *stream, NPBool seekable, uint16 *stype)
{
   if (!_pluginFuncs.newstream)
      return NPERR_GENERIC_ERROR;

   NPError error = _pluginFuncs.newstream(_npp, type, stream, seekable, stype);

   CHECK(NewStream,error);
}


void NSPluginInstance::NPStreamAsFile(NPStream *stream, const char *fname)
{
   if (!_pluginFuncs.asfile)
      return;

   _pluginFuncs.asfile(_npp, stream, fname);
}


int32 NSPluginInstance::NPWrite(NPStream *stream, int32 offset, int32 len, void *buf)
{
   if (!_pluginFuncs.write)
      return 0;

   return _pluginFuncs.write(_npp, stream, offset, len, buf);
}


int32 NSPluginInstance::NPWriteReady(NPStream *stream)
{
   if (!_pluginFuncs.writeready)
      return 0;

   return _pluginFuncs.writeready(_npp, stream);
}


void NSPluginInstance::NPURLNotify(const char *url, NPReason reason, void *notifyData)
{
   if (!_pluginFuncs.urlnotify)
      return;

   _pluginFuncs.urlnotify(_npp, url, reason, notifyData);
}


void NSPluginInstance::addTempFile(KTempFile *tmpFile)
{
   _tempFiles.append(tmpFile);
}


void NSPluginInstance::setCallback(QCString app, QCString obj)
{
   delete _callback;
   _callback = new NSPluginCallbackIface_stub(app, obj);
}

/************************************************************************************/

NSPluginViewer::NSPluginViewer( QCString dcopId, QObject *parent )
   : QObject( parent ), DCOPObject(dcopId)
{
}


NSPluginViewer::~NSPluginViewer()
{
   kdDebug() << "NSPluginViewer::~NSPluginViewer" << endl;
   // TODO: shutdown
}


void NSPluginViewer::Shutdown()
{
   kdDebug() << "NSPluginViewer::Shutdown" << endl;
   // tell event loop to quit
   quitXt();
}


DCOPRef NSPluginViewer::NewClass( QString plugin )
{
   kdDebug() << "NSPluginViewer::NewClass( " << plugin << ")" << endl;

   NSPluginClass *cls = _classes[ plugin ];
   if ( !cls )
   {
      cls = new NSPluginClass( plugin, this );
      _classes.insert( plugin, cls );
   }

   kdDebug() << "NSPluginViewer::NewClass = " << (void*)cls << endl;

   return DCOPRef( kapp->dcopClient()->appId(), cls->objId() );
}


/****************************************************************************/


NSPluginClass::NSPluginClass( const QString &library, QObject *parent )
   : QObject( parent ), DCOPObject(), _libname(library), _constructed(false),  _initialized(false)
{
   _handle = KLibLoader::self()->library(library.latin1());
  
   kdDebug() << "Library handle=" << _handle << endl;

   if (!_handle)
   {
      kdDebug() << "Could not dlopen " << library << endl;
      return;
   }

   _NP_GetMIMEDescription = (NP_GetMIMEDescriptionUPP *)_handle->symbol("NP_GetMIMEDescription");
   _NP_Initialize = (NP_InitializeUPP *)_handle->symbol("NP_Initialize");
   _NP_Shutdown = (NP_ShutdownUPP *)_handle->symbol("NP_Shutdown");

   if (!_NP_GetMIMEDescription)
   {
      kdDebug() << "Could not get symbol NP_GetMIMEDescription" << endl;
      return;
   }

   if (!_NP_Initialize)
   {
      kdDebug() << "Could not get symbol NP_Initialize" << endl;
      return;
   }

   if (!_NP_Shutdown)
   {
      kdDebug() << "Could not get symbol NP_Shutdown" << endl;
      return;
   }

   kdDebug() << "Plugin library " << library << " loaded!" << endl;
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
   kdDebug() << "NSPluginClass::Initialize()" << endl;
                                              
   if (!_constructed)
      return NPERR_GENERIC_ERROR;                             
                                              
   if (_initialized)
      kdError() << "FUNC ALREADY INITIALIZED!" << endl;

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


DCOPRef NSPluginClass::NewInstance(QString url, QString mimeType, int mode, QStringList argn, QStringList argv)
{
   kdDebug() << "-> NSPluginClass::NewInstance" << endl;

   // copy parameters over
   unsigned int argc = argn.count();
   char *_argn[argc], *_argv[argc];
   QString src = url;
   int width = 300;
   int height = 300;

   for (unsigned int i=0; i<argc; i++)
   {
      const char *n = (const char*)argn[i].ascii();
      const char *v = (const char*)argv[i].ascii();
  	
      _argn[i] = strdup(n);      	
      _argv[i] = strdup(v);    	

      if (!strcasecmp(_argn[i], "WIDTH")) width = atoi(_argv[i]);
      if (!strcasecmp(_argn[i], "HEIGHT")) height = atoi(_argv[i]);      
      kdDebug() << "argn=" << _argn[i] << " argv=" << _argv[i] << endl;
   }

   // Create plugin instance
   char mime[256];
   strcpy(mime, mimeType.ascii());
   NPP npp = new NPP_t;
   npp->ndata = NULL;
   NPError error = _pluginFuncs.newp(mime, npp, mode, argc, _argn, _argv, 0); 
   kdDebug() << "Result of NPP_New = " << (int)error << endl;

   if (error != NPERR_NO_ERROR)
   {    
      delete npp;
      kdDebug() << "<- PlluginClass::NewInstance = 0" << endl; 
      return DCOPRef();
   }

   // Create DCOP instance for created plugin
   NSPluginInstance *inst = new NSPluginInstance( npp, &_pluginFuncs, _handle, 
						  width, height, src, mimeType, this );
   if (!inst)
   {
      kdDebug() << "<- PlluginClass::NewInstance = 0" << endl; 
      return DCOPRef();
   }

   kdDebug() << "<- NSPluginClass::NewInstance = %x" << (void*)inst << endl;
   return DCOPRef(kapp->dcopClient()->appId(), inst->objId());
}


// server side functions -----------------------------------------------------

// allocate memory
void *NPN_MemAlloc(uint32 size)
{
   void *mem = ::malloc(size);

   kdDebug() << "NPN_MemAlloc(), size=" << size << " allocated at " << mem << endl;

   return mem;
}


// free memory
void NPN_MemFree(void *ptr)
{
   kdDebug() << "NPN_MemFree() at " << ptr << endl;

   ::free(ptr);
}

uint32 NPN_MemFlush(uint32 /*size*/)
{
   kdDebug() << "NPN_MemFlush()" << endl;
   return 0;
}


// redraw
void NPN_ForceRedraw(NPP /*instance*/)
{
   kdDebug() << "NPN_ForceRedraw() [unimplemented]" << endl;
}


// invalidate rect
void NPN_InvalidateRect(NPP /*instance*/, NPRect */*invalidRect*/)
{
   kdDebug() << "NPN_InvalidateRect() [unimplemented]" << endl;
}


// invalidate region
void NPN_InvalidateRegion(NPP /*instance*/, NPRegion /*invalidRegion*/)
{
   kdDebug() << "NPN_InvalidateRegion() [unimplemented]" << endl;
}


// get value
NPError NPN_GetValue(NPP /*instance*/, NPNVariable variable, void *value)
{
   kdDebug() << "NPN_GetValue(), variable=" << static_cast<int>(variable) << endl;

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


NPError NPN_DestroyStream(NPP instance, NPStream* stream,
			  NPReason /*reason*/)
{
   kdDebug() << "NPN_DestroyStream()" << endl;

   NSPluginInstance *inst = (NSPluginInstance*) instance->ndata;
   inst->destroyStream( (NSPluginStream *)stream->ndata );

   return NPERR_GENERIC_ERROR;
}


NPError NPN_NewStream(NPP /*instance*/, NPMIMEType /*type*/, const char */*target*/, NPStream */*stream*/)
{
   kdDebug() << "NPN_NewStream() [unimplemented]" << endl;

   return NPERR_GENERIC_ERROR;
}


NPError NPN_RequestRead(NPStream */*stream*/, NPByteRange */*rangeList*/)
{
   kdDebug() << "NPN_RequestRead() [unimplemented]" << endl;

   return NPERR_GENERIC_ERROR;
}

NPError NPN_NewStream(NPP /*instance*/, NPMIMEType /*type*/,
                      const char* /*target*/, NPStream** /*stream*/)
{
   kdDebug() << "NPN_NewStream() [unimplemented]" << endl;

   return NPERR_GENERIC_ERROR;
}

int32 NPN_Write(NPP /*instance*/, NPStream */*stream*/, int32 /*len*/, void */*buf*/)
{
   kdDebug() << "NPN_Write() [unimplemented]" << endl;

   return 0;
}


// URL functions
NPError NPN_GetURL(NPP instance, const char *url, const char *target)
{
   kdDebug() << "NPN_GetURL: url=" << url << " target=" << target << endl;

   NSPluginInstance *inst = (NSPluginInstance*) instance->ndata;
   inst->requestURL( url, target, 0 );

   return NPERR_NO_ERROR;
}


NPError NPN_GetURLNotify(NPP instance, const char *url, const char *target,
			 void* notifyData)
{
   kdDebug() << "NPN_GetURLNotify: url=" << url << " target=" << target << endl;

   NSPluginInstance *inst = (NSPluginInstance*) instance->ndata;
   inst->requestURL( url, target, notifyData );    

   return NPERR_NO_ERROR;
}


NPError NPN_PostURL(NPP /*instance*/, const char */*url*/, const char */*target*/,
		    uint32 /*len*/, const char */*buf*/, NPBool /*file*/)
{
   kdDebug() << "NPN_PostURL() [unimplemented]" << endl;

   return NPERR_GENERIC_ERROR;
}


NPError NPN_PostURLNotify(NPP /*instance*/, const char */*url*/, const char */*target*/,
			  uint32 /*len*/, const char */*buf*/, NPBool /*file*/, void */*notifyData*/)
{
   kdDebug() << "NPN_PostURL() [unimplemented]" << endl;

   return NPERR_GENERIC_ERROR;
}


// display status message
void NPN_Status(NPP instance, const char *message)
{
   kdDebug() << "NPN_Status(): " << message << endl;

   if (!instance)
      return;

   // turn into an instance signal
   NSPluginInstance *inst = (NSPluginInstance*) instance->ndata;

   inst->emitStatus(message);
}


// inquire user agent
const char *NPN_UserAgent(NPP /*instance*/)
{
   kdDebug() << "NPN_UserAgent()" << endl;

   // FIXME: Use the same as konqy
   return "Mozilla";
}


// inquire version information
void NPN_Version(int *plugin_major, int *plugin_minor, int *browser_major, int *browser_minor)
{
   kdDebug() << "NPN_Version()" << endl;

   // FIXME: Use the sensible values
   *browser_major = NP_VERSION_MAJOR;
   *browser_minor = NP_VERSION_MINOR;

   *plugin_major = NP_VERSION_MAJOR;
   *plugin_minor = NP_VERSION_MINOR;
}


void NPN_ReloadPlugins(NPBool /*reloadPages*/)
{
   kdDebug() << "NPN_ReloadPlugins() [unimplemented]" << endl;
}


// JAVA functions
JRIEnv *NPN_GetJavaEnv()
{
   // FIXME: Allow use of JAVA
   return 0;
}


jref NPN_GetJavaPeer(NPP /*instance*/)
{
   kdDebug() << "NPN_GetJavaPeer() [unimplemented]" << endl;

   return 0;
}


NPError NPN_SetValue(NPP /*instance*/, NPPVariable /*variable*/, void */*value*/)
{
   kdDebug() << "NPN_SetValue() [unimplemented]" << endl;

   return NPERR_GENERIC_ERROR;
}

/*****************************************************************************************/

NSPluginStream::NSPluginStream( NSPluginInstance *instance )
   : QObject( instance ), _instance(instance), _job(0), _stream(0), _tempFile(0L), 
   _pos(0), _queue(0), _queuePos(0)
{
   _resumeTimer = new QTimer( this );
   connect(_resumeTimer, SIGNAL(timeout()), this, SLOT(resume()));
}


NSPluginStream::~NSPluginStream()
{
   kdDebug() << "-> ~NSPluginStream" << endl;
   
   //delete _job;

   if (_stream)
   {
      _instance->NPDestroyStream(_stream, NPRES_USER_BREAK);
      delete _stream;
   }

   delete _tempFile;
   delete _queue;

   kdDebug() << "<- ~NSPluginStream" << endl;
}


void NSPluginStream::get( QString url, QString mimeType, void *notify )
{
   KURL src(url);
   _url = url;
   _notifyData = notify;

   // terminate existing job
   if (_job)
   {
      delete _job;
      _job = 0;
   }

   // terminate existing stream
   if (_stream)
   {
      _instance->NPDestroyStream(_stream, NPRES_DONE);
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
   _stream->pdata = 0;
   _stream->lastmodified = 0;
   _stream->notifyData = 0;

   // inform the plugin
   _instance->NPNewStream((char*)mimeType.ascii(), _stream, false, &_streamType);
   kdDebug() << "NewStream stype=" << _streamType << endl;

   // prepare data transfer
   _tempFile = 0L;

   if (_streamType == NP_ASFILEONLY)
   {
      // do an synchronous download

      // TODO: The file downloaded should be placed in konqys cache
      // and retrieved from there in repeated downloads!

      QString tmpFile;
      if( KIO::NetAccess::download( src, tmpFile) )
      {
	 _instance->NPStreamAsFile( _stream, tmpFile.ascii() );
	 _instance->NPDestroyStream( _stream, NPRES_DONE );
	 if ( _notifyData ) _instance->NPURLNotify( url.ascii() /* ### FIXME!! */, NPRES_NETWORK_ERR, _notifyData );
	 delete _stream;
	 _stream = 0;

	 KIO::NetAccess::removeTempFile( tmpFile );
      }    
      return;
   } else if ( _streamType == NP_ASFILE )
   {      
      _tempFile = new KTempFile( QString::null, src.fileName() );
      _tempFile->setAutoDelete( TRUE );
   } else
   {
      kdDebug() << "Unknown stream type " << _streamType << " requested, trying NP_NORMAL" << endl;
      _streamType = NP_NORMAL;
   }

   // start the kio job
   kdDebug() << "-> KIO::get( " << url << " )" << endl;
   _job = KIO::get(url, false, false);
   connect(_job, SIGNAL(data(KIO::Job *, const QByteArray &)),
	   this, SLOT(data(KIO::Job *, const QByteArray &)));
   connect(_job, SIGNAL(result(KIO::Job *)),
	   this, SLOT(result(KIO::Job *)));
   kdDebug() << "<- KIO::get" << endl;
}



void NSPluginStream::data(KIO::Job * /*job*/, const QByteArray &data)
{
   //kdDebug() << "-> NSPluginStream::data" << endl;
   unsigned int pos = process( data, 0 );  
   if (pos<data.size())
   {
      kdDebug() << "pos<size" << endl;
      _job->suspend();

      _queue = new QByteArray( data );
      _queuePos = pos;
      _resumeTimer->start( 100, TRUE );
   }

   //kdDebug() << "<- NSPluginStream::data" << endl;
}


void NSPluginStream::resume()
{
   kdDebug() << "-> NSPluginStream::resume" << endl;
   if (_queue)
   {
      kdDebug() << "queue found at pos " << _queuePos << ", size " << _queue->size() << endl;
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
      kdDebug() << "restart timer" << endl;
      _resumeTimer->start( 100, TRUE );
   } else
   {
      kdDebug() << "resume job" << endl;
      _job->resume();
   }
   kdDebug() << "<- NSPluginStream::resume" << endl;
}


unsigned int NSPluginStream::process( const QByteArray &data, int start )
{
   int32 max, sent, to_sent, len;
   char *d = data.data()+start;

   to_sent = data.size()-start;
   while (to_sent>0)
   {
      max = _instance->NPWriteReady( _stream );
      len = QMIN(max, to_sent);

      //kdDebug() << "-> Feeding stream to plugin: offset=" << _pos << ", len=" << len << endl;
      sent = _instance->NPWrite( _stream, _pos, len, d );
      //kdDebug() << "<- Feeding stream: sent = " << sent << endl;

      if (sent==0) // interrupt the stream for a few ms
	 break;

      if (_tempFile)
      {      	 	
	 _tempFile->dataStream()->writeRawBytes( d, sent );      	
      }
      
      to_sent -= sent;
      _pos += sent;
      d += sent;
   }

   return data.size()-to_sent;
}

void NSPluginStream::result(KIO::Job *job)
{
   int err = job->error();
   kdDebug() << "NSPluginStream::result = " << err << endl;

   if ( !err )
   {
      if ( _tempFile )
      {
	 _tempFile->close();
	 _instance->addTempFile(_tempFile);	 
	 _instance->NPStreamAsFile(_stream, _tempFile->name().ascii());
	 _tempFile = 0;	 	 
      }
      
      _instance->NPDestroyStream( _stream, NPRES_DONE );
      if ( _notifyData ) _instance->NPURLNotify( _url.ascii() /* ### FIXME!!! */, NPRES_DONE, _notifyData );
   } else
   {
      // close temp file
      if ( _tempFile )
	 _tempFile->close();

      // destroy stream
      _instance->NPDestroyStream(_stream, NPRES_NETWORK_ERR);
      if ( _notifyData ) _instance->NPURLNotify( _url.ascii() /* ### FIXME!!! */, NPRES_NETWORK_ERR, _notifyData );
      delete _stream;
      _stream = 0;

      // destroy NSPluginStream object
      _instance->destroyStream( this );      
   }
}
