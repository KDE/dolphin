#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xlibint.h>

#ifdef __hpux
#include <dl.h>
#else
#include <dlfcn.h>
#endif

#define XP_UNIX 1
#include "sdk/npupp.h"


NPNetscapeFuncs gNetscapeFuncs;	/* Netscape Function table */
NPNetscapeFuncs gExtNetscapeFuncs;	/* table that is passed to the plugin*/
NPPluginFuncs gPluginFuncs;

typedef char* NP_GetMIMEDescription_t(void);
typedef NPError NP_Initialize_t(NPNetscapeFuncs*, NPPluginFuncs*);
typedef NPError NP_Shutdown_t(void);
typedef NPError NP_GetValue_t(void *future, NPPVariable variable, void *value);

NP_GetMIMEDescription_t *gNP_GetMIMEDescription = NULL;
NP_Initialize_t *gNP_Initialize = NULL;
NP_Shutdown_t *gNP_Shutdown = NULL;
NP_GetValue_t *gNP_GetValue = NULL;

#ifdef __hpux
shl_t gLib;
#else
void *gLib = 0L;
#endif
FILE *ef = 0L;

#define DEB fprintf

static
void UnloadPlugin() {
#ifdef __hpux
   if (gLib) {
		DEB( ef, "-> UnloadPlugin\n" );
      shl_unload(gLib);
		DEB( ef, "<- UnloadPlugin\n" );
               
      gLib=0L;
   }
#else
	if ( gLib )	{
		DEB( ef, "-> UnloadPlugin\n" );
		dlclose( gLib );
		gLib = 0L;
		
		DEB( ef, "<- UnloadPlugin\n" );
		
		if (ef) fclose( ef );
	}
#endif
}

static
void LoadPlugin() {
	if ( !gLib ) {
		ef = fopen( "/tmp/plugin.log", "a" );
		DEB( ef, "-------------------------------\n" );
		fclose( ef );
		ef = fopen( "/tmp/plugin.log", "a" );
		setvbuf( ef, NULL, _IONBF, 0 );
		DEB( ef, "-> LoadPlugin\n" );
		
#ifdef __hpux
                gLib = shl_load("/tmp/plugin.so", BIND_IMMEDIATE, 0L);
                if (shl_findsym(&gLib, "/tmp/plugin.so", (short) TYPE_PROCEDURE, (void *) &gNP_GetMIMEDescription))
                   gNP_GetMIMEDescription = NULL;
                if (shl_findsym(&gLib, "/tmp/plugin.so", (short) TYPE_PROCEDURE, (void *) &gNP_Initialize))
                   gNP_Initialize = NULL;
                if (shl_findsym(&gLib, "/tmp/plugin.so", (short) TYPE_PROCEDURE, (void *) &gNP_Shutdown))
                   gNP_Shutdown = NULL;
                if (shl_findsym(&gLib, "/tmp/plugin.so", (short) TYPE_PROCEDURE, (void *) &gNP_GetValue))
                   gNP_GetValue = NULL;
#else
		gLib = dlopen( "/tmp/plugin.so", RTLD_NOW );
		DEB( ef, "gLib = %x\n", gLib );

		gNP_GetMIMEDescription = dlsym( gLib, "NP_GetMIMEDescription" );
		gNP_Initialize = dlsym( gLib, "NP_Initialize" );
		gNP_Shutdown = dlsym( gLib, "NP_Shutdown" );
		gNP_GetValue = dlsym( gLib, "NP_GetValue" );
#endif
		DEB( ef, "gNP_GetMIMEDescription = %x\n", NP_GetMIMEDescription );
		DEB( ef, "gNP_Initialize = %x\n", gNP_Initialize );
		DEB( ef, "gNP_Shutdown = %x\n", gNP_Shutdown );
		DEB( ef, "gNP_GetValue = %x\n", gNP_GetValue );
		
		if ( !gNP_GetMIMEDescription || !gNP_Initialize || !gNP_Initialize || !gNP_GetValue ) {
			DEB( ef, "<- LoadPlugin - will unload before\n" );
		 	UnloadPlugin();
		} else
				DEB( ef, "<- LoadPlugin\n" );
	}
}

extern char *NP_GetMIMEDescription(void);
char *NP_GetMIMEDescription(void)
{
	char * ret;
	
	LoadPlugin();
	if ( !gLib ) return NULL;	
	DEB(ef, "-> NP_GetMIMEDescription()\n" );
	
	ret = gNP_GetMIMEDescription();
	DEB(ef, "<- NP_GetMIMEDescription = %s\n", ret );
	return ret;
}

/*static
NPError MyNPP_Initialize(void)
{
	NPError err;	
	DEB(ef, "-> NPP_Initialize( )\n");
	
	err = gPluginFuncs.initialize( );
	DEB(ef, "<- NPP_Initialize = %d\n", err);
	return err;
}*/

/*static
void MyNPP_Shutdown(void)
{
	DEB(ef, "-> NPP_Shutdown( )\n");
	gPluginFuncs.shutdown( );
	DEB(ef, "<- NPP_Shutdown\n");
} */

static
NPError MyNPP_New(NPMIMEType pluginType, NPP instance,
                uint16 mode, int16 argc, char* argn[],
                char* argv[], NPSavedData* saved)
{
	NPError err;	
	int n;
	DEB(ef, "-> NPP_New( %s, 0x%x, %d, %d, .., .., 0x%x )\n", pluginType, instance, mode, argc, saved);

	for ( n=0; n<argc; n++ ) {
	    DEB(ef, "%s=%s\n", argn[n], argv[n] );
	}

	err = gPluginFuncs.newp( pluginType, instance, mode, argc, argn, argv, saved );
	DEB(ef, "<- NPP_New = %d\n", err);
	return err;
}

static
NPError MyNPP_Destroy(NPP instance, NPSavedData** save)
{
	NPError err;	
	DEB(ef, "-> NPP_Destrpy( %x, 0x%x )\n", instance, save);
	
	err = gPluginFuncs.destroy( instance, save );
	DEB(ef, "<- NPP_Destroy = %d\n", err);
	return err;
}

static
NPError MyNPP_SetWindow(NPP instance, NPWindow* window)
{
	NPError err;	
	NPSetWindowCallbackStruct *win_info;
	DEB(ef, "-> NPP_SetWindow( %x, 0x%x )\n", instance, window);
	
	DEB(ef, "window->window = 0x%x\n", window->window);
	DEB(ef, "window->x = %d\n", window->x);
	DEB(ef, "window->y = %d\n", window->y);
	DEB(ef, "window->width = %d\n", window->width);
	DEB(ef, "window->height = %d\n", window->height);
	DEB(ef, "window->ws_info = 0x%x\n", window->ws_info);
	DEB(ef, "window->type = 0x%x\n", window->type);
		
	win_info = (NPSetWindowCallbackStruct*)window->ws_info;
	DEB(ef, "win_info->type = %d\n", win_info->type);
	DEB(ef, "win_info->display = 0x%x\n", win_info->display);
	DEB(ef, "win_info->visual = 0x%x\n", win_info->visual);
 	DEB(ef, "win_info->colormap = 0x%x\n", win_info->colormap);
  	DEB(ef, "win_info->depth = %d\n", win_info->depth);
	
	err = gPluginFuncs.setwindow( instance, window );	
	DEB(ef, "<- NPP_SetWindow = %d\n", err);
	return err;
}

static
NPError MyNPP_NewStream(NPP instance, NPMIMEType type,
        	            NPStream* stream, NPBool seekable,
          	          uint16* stype)
{
	NPError err;	
	DEB(ef, "-> NPP_NewStream( %x, %s, 0x%x, %d, 0x%x )\n", instance, type, stream, seekable, stype);

	DEB(ef, "stream->ndata = 0x%x\n", stream->ndata);
	DEB(ef, "stream->url = %s\n", stream->url );
        DEB(ef, "stream->end = %d\n", stream->end );
	DEB(ef, "stream->pdata = 0x%x\n", stream->pdata );
	DEB(ef, "stream->lastmodified = %d\n", stream->lastmodified );
	DEB(ef, "stream->notifyData = 0x%x\n", stream->notifyData );
	
	err = gPluginFuncs.newstream( instance, type, stream, seekable, stype );		
	DEB(ef, "<- NPP_NewStream = %d\n", err);
	DEB(ef, "stype = %d\n", *stype);	
	return err;
}
          	
static
NPError MyNPP_DestroyStream(NPP instance, NPStream* stream,
		                      NPReason reason)
{
	NPError err;	
	DEB(ef, "-> NPP_DestroyStream( %x, 0x%x, %d )\n", instance, stream, reason);	
	
	err = gPluginFuncs.destroystream( instance, stream, reason );		
	DEB(ef, "<- NPP_DestroyStream = %d\n", err);
	return err;
}		

static
int32 MyNPP_WriteReady(NPP instance, NPStream* stream)
{
	int32 ret;	
	DEB(ef, "-> NPP_WriteReady( %x, 0x%x )\n", instance, stream);	
	
	ret = gPluginFuncs.writeready( instance, stream );		
	DEB(ef, "<- NPP_WriteReady = %d\n", ret);
	return ret;
}

static
int32 MyNPP_Write(NPP instance, NPStream* stream, int32 offset,
                  int32 len, void* buffer)
{
	int32 ret;	
	DEB(ef, "-> NPP_Write( %x, 0x%x, %d, %d, 0x%x )\n", instance, stream, offset, len, buffer);	
	
	ret = gPluginFuncs.write( instance, stream, offset, len, buffer );		
	DEB(ef, "<- NPP_Write = %d\n", ret);
	return ret;
}

static
void MyNPP_StreamAsFile(NPP instance, NPStream* stream,
                     const char* fname)
{	
	DEB(ef, "-> NPP_StreamAsFile( %x, 0x%x, %s )\n", instance, stream, fname);	
	
	gPluginFuncs.asfile( instance, stream, fname );		
	DEB(ef, "<- NPP_StreamAsFile\n");
}

static
void MyNPP_Print(NPP instance, NPPrint* platformPrint)
{
	DEB(ef, "-> NPP_Print( %x, 0x%x )\n", instance, platformPrint );		
	gPluginFuncs.print( instance, platformPrint );		
	DEB(ef, "<- NPP_Print\n");	
}

static
int16 MyNPP_HandleEvent(NPP instance, void* event)
{
	int16 ret;
	DEB(ef, "-> NPP_HandleEvent( %x, 0x%x )\n", instance, event);	
	
	ret = gPluginFuncs.event( instance, event );		
	DEB(ef, "<- NPP_HandleEvent = %d\n", ret);
	return ret;
}

static
void MyNPP_URLNotify(NPP instance, const char* url,
                    NPReason reason, void* notifyData)
{	
	DEB(ef, "-> NPP_URLNotify( %x, %s, %d, 0x%x )\n", instance, url, reason, notifyData );	
	gPluginFuncs.urlnotify( instance, url, reason, notifyData );		
	DEB(ef, "<- NPP_URLNotify\n");	
}

#if 0
static
jref MyNPP_GetJavaClass(void)
{
  jref ret;
	DEB(ef, "-> NPP_GetJavaClass( )\n" );	
	
/*	ret = gPluginFuncs.javaClass( );*/
	DEB(ef, "<- NPP_GetJavaClass = %d\n", ret);
	return ret;
}
#endif

static
NPError MyNPP_GetValue(void *instance, NPPVariable variable, void *value)
{
	NPError err;	
	DEB(ef, "-> NPP_GetValue( %x, %d, 0x%x )\n", instance, variable, value);	
	
	err = gPluginFuncs.getvalue( instance, variable, value );			
	DEB(ef, "<- NPP_GetValue = %d\n", err);
	return err;
}

static
NPError MyNPP_SetValue(void *instance, NPNVariable variable, void *value)
{
	NPError err;	
	DEB(ef, "-> NPP_SetValue( %x, %d, 0x%x )\n", instance, variable, value);	
	
	err = gPluginFuncs.getvalue( instance, variable, value );		
	DEB(ef, "<- NPP_SetValue = %d\n", err);
	return err;
}

/*static
void MyNPN_Version(int* plugin_major, int* plugin_minor,
              int* netscape_major, int* netscape_minor)
{	
	DEB(ef, "-> NPN_Version( %d, %d, %d, %d )\n", *plugin_major, *plugin_minor, *netscape_major, *netscape_minor);	
	
	gNetscapeFuncs.version( plugin_major, plugin_minor, netscape_major, netscape_minor );		
	DEB(ef, "<- NPN_Version\n");	
	DEB(ef, "plugin_major = %d\n", *plugin_major);
	DEB(ef, "plugin_minor = %d\n", *plugin_minor);
	DEB(ef, "netscape_major = %d\n", *plugin_major);
	DEB(ef, "netscape_minor = %d\n", *plugin_minor);
}*/

static
NPError MyNPN_GetURLNotify(NPP instance, const char* url,
                 const char* target, void* notifyData)
{
	NPError err;	
	DEB(ef, "-> NPN_GetURLNotify( %x, %s, %s, 0x%x )\n", instance, url, target, notifyData);	
	
	err = gNetscapeFuncs.geturlnotify( instance, url, target, notifyData );		
	DEB(ef, "<- NPN_GetURLNotify = %d\n", err);
	return err;
}

static
NPError MyNPN_GetURL(NPP instance, const char* url,
               const char* target)
{
	NPError err;	
	DEB(ef, "-> NPN_GetURL( %x, %s, %s )\n", instance, url, target );	
	
	err = gNetscapeFuncs.geturl( instance, url, target );		
	DEB(ef, "<- NPN_GetURL = %d\n", err);
	return err;
}

static
NPError MyNPN_PostURLNotify(NPP instance, const char* url,
                  const char* target, uint32 len,
                  const char* buf, NPBool file,
                  void* notifyData)
{
	NPError err;	
	DEB(ef, "-> NPN_PostURLNotify( %x, %s, %s, %d, 0x%x, %d, 0x%x )\n", instance, url, target, len, buf, file, notifyData);	
	
	err = gNetscapeFuncs.posturlnotify( instance, url, target, len, buf, file, notifyData );		
	DEB(ef, "<- NPN_PostURLNotify = %d\n", err);
	return err;
}

static
NPError MyNPN_PostURL(NPP instance, const char* url,
              const char* target, uint32 len,
              const char* buf, NPBool file)
{
	NPError err;	
	DEB(ef, "-> NPN_PostURL( %x, %s, %s, %d, 0x%x, %d )\n", instance, url, target, len, buf, file );	
	
	err = gNetscapeFuncs.posturl( instance, url, target, len, buf, file  );		
	DEB(ef, "<- NPN_PostURL = %d\n", err);
	return err;
}

static
NPError MyNPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
	NPError err;	
	DEB(ef, "-> NPN_RequestRead( %x, 0x%x )\n", stream, rangeList );	
	DEB(ef, "rangeList->offset = %d\n", rangeList->offset);
	DEB(ef, "rangeList->length = %d\n", rangeList->length);
	DEB(ef, "rangeList->next = 0x%x\n", rangeList->next);
	
	err = gNetscapeFuncs.requestread( stream, rangeList );		
	DEB(ef, "<- NPN_RequestRead = %d\n", err);
	DEB(ef, "rangeList->offset = %d\n", rangeList->offset);
	DEB(ef, "rangeList->length = %d\n", rangeList->length);
	DEB(ef, "rangeList->next = 0x%x\n", rangeList->next);
	return err;
}

static
NPError MyNPN_NewStream(NPP instance, NPMIMEType type,
                const char* target, NPStream** stream)
{
	NPError err;	
	DEB(ef, "-> NPN_NewStream( %x, %s, %s, 0x%x )\n", instance, type, target, stream);
	
	err = gNetscapeFuncs.newstream( instance, type, target, stream );		
	DEB(ef, "<- NPN_NewStream = %d\n", err);
	return err;
}

static
int32 MyNPN_Write(NPP instance, NPStream* stream, int32 len,
              void* buffer)
{
	int32 ret;	
	DEB(ef, "-> NPN_Write( %x, 0x%x, %d, 0x%x )\n", instance, stream, len, buffer);	
	
	ret = gNetscapeFuncs.write( instance, stream, len, buffer );		
	DEB(ef, "<- NPN_Write = %d\n", ret);
	return ret;
}

static
NPError MyNPN_DestroyStream(NPP instance, NPStream* stream,
                  NPReason reason)
{
	NPError err;	
	DEB(ef, "-> NPN_DestroyStream( %x, 0x%x, %d )\n", instance, stream, reason);	
	
	err = gNetscapeFuncs.destroystream( instance, stream, reason );		
	DEB(ef, "<- NPN_DestroyStream = %d\n", err);
	return err;
}

static
void MyNPN_Status(NPP instance, const char* message)
{	
	DEB(ef, "-> NPN_Status( %x, %s )\n", instance, message);	
	gNetscapeFuncs.status( instance, message );		
	DEB(ef, "<- NPN_Status\n");	
}

static
const char* MyNPN_UserAgent(NPP instance)
{
	const char *ret;
	DEB(ef, "-> NPN_UserAgent( %x )\n", instance);	
	
	ret = gNetscapeFuncs.uagent( instance );		
	DEB(ef, "<- NPN_UserAgent = %s\n", ret);
	return ret;
}

static
void* MyNPN_MemAlloc(uint32 size)
{
	void *ret;
	DEB(ef, "-> NPN_MemAlloc( %d )\n", size);	

	ret = gNetscapeFuncs.memalloc( size );		
	DEB(ef, "<- NPN_MemAlloc = 0x%x\n", ret);
	return ret;
}

static
void MyNPN_MemFree(void* ptr)
{	
	DEB(ef, "-> NPN_MemFree( 0x%x )\n", ptr);	
	gNetscapeFuncs.memfree( ptr );		
	DEB(ef, "<- NPN_MemFree\n");
}

static
uint32 MyNPN_MemFlush(uint32 size)
{
	uint ret;
	DEB(ef, "-> NPN_MemFlush( %d )\n", size);	

	ret = gNetscapeFuncs.memflush( size );		
	DEB(ef, "<- NPN_MemFlush = %d\n", ret);
	return ret;
}

static
void MyNPN_ReloadPlugins(NPBool reloadPages)
{	
	DEB(ef, "-> NPN_ReloadPlugins( %d )\n", reloadPages);	
	gNetscapeFuncs.reloadplugins( reloadPages );		
	DEB(ef, "<- NPN_ReloadPlugins\n");
}

static
JRIEnv* MyNPN_GetJavaEnv(void)
{
	JRIEnv *ret;
	DEB(ef, "-> NPN_GetJavaEnv( )\n");	

	ret = gNetscapeFuncs.getJavaEnv( );		
	DEB(ef, "<- NPN_GetJavaEnv = 0x%x\n", ret);
	return ret;
}

static
jref MyNPN_GetJavaPeer(NPP instance)
{
	jref ret;
	DEB(ef, "-> NPN_GetJavaPeer( %x )\n", instance);	

	ret = gNetscapeFuncs.getJavaPeer( instance );		
	DEB(ef, "<- NPN_GetJavaPeer = 0x%x\n", ret);
	return ret;	
}

static
NPError MyNPN_GetValue(NPP instance, NPNVariable variable,
               void *value)
{
	NPError ret;
	DEB(ef, "-> NPN_GetValue( %x, %d, 0x%x)\n", instance, variable, value);		
	ret = gNetscapeFuncs.getvalue( instance, variable, value );
	DEB(ef, "<- NPN_GetValue = %d\n", ret);
	return ret;
}

static
NPError MyNPN_SetValue(NPP instance, NPPVariable variable,
               void *value)
{
	NPError ret;
	DEB(ef, "-> NPN_SetValue( %x, %d, 0x%x)\n", instance, variable, value);	

	ret = gNetscapeFuncs.setvalue( instance, variable, value );		
	DEB(ef, "<- NPN_SetValue = %d\n", ret);
	return ret;
}

static
void MyNPN_InvalidateRect(NPP instance, NPRect *invalidRect)
{
	DEB(ef, "-> NPN_InvalidateRect( %x, 0x%x )\n", instance, invalidRect);	
	gNetscapeFuncs.invalidaterect( instance, invalidRect );		
	DEB(ef, "<- NPN_InvalidateRect\n");	
}

static
void MyNPN_InvalidateRegion(NPP instance, NPRegion invalidRegion)
{
	DEB(ef, "-> NPN_InvalidateRegion( %x, 0x%x )\n", instance, invalidRegion);	
	gNetscapeFuncs.invalidateregion( instance, invalidRegion );		
	DEB(ef, "<- NPN_InvalidateRegion\n");	
}

static
void MyNPN_ForceRedraw(NPP instance)
{
	DEB(ef, "-> NPN_ForceRedraw( %x )\n", instance);	
	gNetscapeFuncs.forceredraw( instance );		
	DEB(ef, "<- NPN_ForceRedraw\n");	
}

extern NPError NP_GetValue(void *future, NPPVariable variable, void *value);
NPError NP_GetValue(void *future, NPPVariable variable, void *value)
{
	NPError err;
	LoadPlugin();	
	if ( !gLib ) return NPERR_GENERIC_ERROR;
	DEB(ef, "-> NP_GetValue( %x, %d, %x )\n", future, variable, value );

	err = gNP_GetValue( future, variable, value );
	DEB(ef, "<- NP_GetValue = %d\n", err );
	return err;
}

extern NPError NP_Initialize(NPNetscapeFuncs* nsTable, NPPluginFuncs* pluginFuncs);
NPError NP_Initialize(NPNetscapeFuncs* nsTable, NPPluginFuncs* pluginFuncs)
{	
	NPError err;
	LoadPlugin();
	if ( !gLib ) return NPERR_GENERIC_ERROR;	
	DEB(ef, "-> NP_Initialize( %x, %x )\n", nsTable, pluginFuncs );
	
	DEB(ef, "nsTable->size = %d\n", nsTable->size);
	DEB(ef, "nsTable->version = 0x%x\n", nsTable->version);
	DEB(ef, "nsTable->geturl = 0x%x\n", nsTable->geturl);
	DEB(ef, "nsTable->posturl = 0x%x\n", nsTable->posturl);
	DEB(ef, "nsTable->requestread = 0x%x\n", nsTable->requestread);
	DEB(ef, "nsTable->newstream = 0x%x\n", nsTable->newstream);
	DEB(ef, "nsTable->write = 0x%x\n", nsTable->write);
	DEB(ef, "nsTable->destroystream = 0x%x\n", nsTable->destroystream);
	DEB(ef, "nsTable->status = 0x%x\n", nsTable->status);
	DEB(ef, "nsTable->uagent = 0x%x\n", nsTable->uagent);
	DEB(ef, "nsTable->memalloc = 0x%x\n", nsTable->memalloc);
	DEB(ef, "nsTable->memfree = 0x%x\n", nsTable->memfree);
	DEB(ef, "nsTable->memflush = 0x%x\n", nsTable->memflush);
	DEB(ef, "nsTable->reloadplugins = 0x%x\n", nsTable->reloadplugins);
	DEB(ef, "nsTable->getJavaEnv = 0x%x\n", nsTable->getJavaEnv);
	DEB(ef, "nsTable->getJavaPeer = 0x%x\n", nsTable->getJavaPeer);
	DEB(ef, "nsTable->geturlnotify = 0x%x\n", nsTable->geturlnotify);
	DEB(ef, "nsTable->posturlnotify = 0x%x\n", nsTable->posturlnotify);
	DEB(ef, "nsTable->getvalue = 0x%x\n", nsTable->getvalue);
	DEB(ef, "nsTable->setvalue = 0x%x\n", nsTable->setvalue);
	DEB(ef, "nsTable->invalidaterect = 0x%x\n", nsTable->invalidaterect);
	DEB(ef, "nsTable->invalidateregion = 0x%x\n", nsTable->invalidateregion);
	DEB(ef, "nsTable->forceredraw = 0x%x\n", nsTable->forceredraw);

	DEB(ef, "pluginFuncs->size = %d\n", pluginFuncs->size);
	DEB(ef, "pluginFuncs->version = 0x%x\n", pluginFuncs->version);
	DEB(ef, "pluginFuncs->newp = 0x%x\n", pluginFuncs->newp);
	DEB(ef, "pluginFuncs->destroy = 0x%x\n", pluginFuncs->destroy);
	DEB(ef, "pluginFuncs->setwindow = 0x%x\n", pluginFuncs->setwindow);
	DEB(ef, "pluginFuncs->newstream = 0x%x\n", pluginFuncs->newstream);
	DEB(ef, "pluginFuncs->destroystream = 0x%x\n", pluginFuncs->destroystream);
	DEB(ef, "pluginFuncs->asfile = 0x%x\n", pluginFuncs->asfile);
	DEB(ef, "pluginFuncs->writeready = 0x%x\n", pluginFuncs->writeready);
	DEB(ef, "pluginFuncs->write = 0x%x\n", pluginFuncs->write);
	DEB(ef, "pluginFuncs->print = 0x%x\n", pluginFuncs->print);
	DEB(ef, "pluginFuncs->event = 0x%x\n", pluginFuncs->event);
	DEB(ef, "pluginFuncs->urlnotify = 0x%x\n", pluginFuncs->urlnotify);
	DEB(ef, "pluginFuncs->javaClass = 0x%x\n", pluginFuncs->javaClass);
	DEB(ef, "pluginFuncs->getvalue = 0x%x\n", pluginFuncs->getvalue);
	DEB(ef, "pluginFuncs->setvalue = 0x%x\n", pluginFuncs->setvalue);
	
	if ( pluginFuncs->size>sizeof(gPluginFuncs) )
	{
		DEB(ef, "Plugin function table too big\n");
		return NPERR_GENERIC_ERROR;
	}
	
	if ( nsTable->size>sizeof(gNetscapeFuncs) )
	{
		DEB(ef, "Netscape function table too big\n");
		return NPERR_GENERIC_ERROR;
	}
	
	memcpy(&gNetscapeFuncs, nsTable, sizeof(gNetscapeFuncs));
	memcpy(&gExtNetscapeFuncs, nsTable, sizeof(gExtNetscapeFuncs));
		
	gExtNetscapeFuncs.geturl = MyNPN_GetURL;
	gExtNetscapeFuncs.posturl = MyNPN_PostURL;
	gExtNetscapeFuncs.requestread = MyNPN_RequestRead;
	gExtNetscapeFuncs.newstream = MyNPN_NewStream;
	gExtNetscapeFuncs.write = MyNPN_Write;
	gExtNetscapeFuncs.destroystream = MyNPN_DestroyStream;
	gExtNetscapeFuncs.status = MyNPN_Status;
  	gExtNetscapeFuncs.uagent = MyNPN_UserAgent;
	/*gExtNetscapeFuncs.memalloc = MyNPN_MemAlloc;
	gExtNetscapeFuncs.memfree = MyNPN_MemFree;
	gExtNetscapeFuncs.memflush = MyNPN_MemFlush;*/
	gExtNetscapeFuncs.reloadplugins = MyNPN_ReloadPlugins;
	gExtNetscapeFuncs.getJavaEnv = MyNPN_GetJavaEnv;
	gExtNetscapeFuncs.getJavaPeer = MyNPN_GetJavaPeer;
	gExtNetscapeFuncs.geturlnotify = MyNPN_GetURLNotify;
	gExtNetscapeFuncs.posturlnotify = MyNPN_PostURLNotify;
	gExtNetscapeFuncs.getvalue = MyNPN_GetValue;
	gExtNetscapeFuncs.setvalue = MyNPN_SetValue;
	gExtNetscapeFuncs.invalidaterect = MyNPN_InvalidateRect;
	gExtNetscapeFuncs.invalidateregion = MyNPN_InvalidateRegion;
	gExtNetscapeFuncs.forceredraw = MyNPN_ForceRedraw;	
	
	gPluginFuncs.size = sizeof( gPluginFuncs );

	DEB(ef, "call\n");

	err = gNP_Initialize( &gExtNetscapeFuncs, &gPluginFuncs );
	
	if (!err) {	
		/*memcpy(&pluginFuncs, gPluginFuncs, sizeof(gPluginFuncs));*/
				
		/*pluginFuncs->initialize = MyNPP_Initialize;
		pluginFuncs->shutdown = MyNPP_Shutdown;*/
		pluginFuncs->newp = MyNPP_New;
		pluginFuncs->destroy = MyNPP_Destroy;
		pluginFuncs->setwindow = MyNPP_SetWindow;
		pluginFuncs->newstream = MyNPP_NewStream;
		pluginFuncs->destroystream = MyNPP_DestroyStream;
		pluginFuncs->asfile = MyNPP_StreamAsFile;
		pluginFuncs->writeready = MyNPP_WriteReady;
		pluginFuncs->write = MyNPP_Write;
		pluginFuncs->print = MyNPP_Print;
		pluginFuncs->event = MyNPP_HandleEvent;
		pluginFuncs->urlnotify = MyNPP_URLNotify;
		pluginFuncs->javaClass = 0; /* MyNPP_GetJavaClass; */
		pluginFuncs->getvalue = (NPP_GetValueUPP)MyNPP_GetValue;
		pluginFuncs->setvalue = (NPP_SetValueUPP)MyNPP_SetValue;
	
		DEB(ef, "nsTable->size = %d\n", gExtNetscapeFuncs.size);
		DEB(ef, "nsTable->version = 0x%x\n", gExtNetscapeFuncs.version);
		DEB(ef, "nsTable->geturl = 0x%x\n", gExtNetscapeFuncs.geturl);
		DEB(ef, "nsTable->posturl = 0x%x\n", gExtNetscapeFuncs.posturl);
		DEB(ef, "nsTable->requestread = 0x%x\n", gExtNetscapeFuncs.requestread);
		DEB(ef, "nsTable->newstream = 0x%x\n", gExtNetscapeFuncs.newstream);
		DEB(ef, "nsTable->write = 0x%x\n", gExtNetscapeFuncs.write);
		DEB(ef, "nsTable->destroystream = 0x%x\n", gExtNetscapeFuncs.destroystream);
		DEB(ef, "nsTable->status = 0x%x\n", gExtNetscapeFuncs.status);
		DEB(ef, "nsTable->uagent = 0x%x\n", gExtNetscapeFuncs.uagent);
		DEB(ef, "nsTable->memalloc = 0x%x\n", gExtNetscapeFuncs.memalloc);
		DEB(ef, "nsTable->memfree = 0x%x\n", gExtNetscapeFuncs.memfree);
		DEB(ef, "nsTable->memflush = 0x%x\n", gExtNetscapeFuncs.memflush);
		DEB(ef, "nsTable->reloadplugins = 0x%x\n", gExtNetscapeFuncs.reloadplugins);
		DEB(ef, "nsTable->getJavaEnv = 0x%x\n", gExtNetscapeFuncs.getJavaEnv);
		DEB(ef, "nsTable->getJavaPeer = 0x%x\n", gExtNetscapeFuncs.getJavaPeer);
		DEB(ef, "nsTable->geturlnotify = 0x%x\n", gExtNetscapeFuncs.geturlnotify);
		DEB(ef, "nsTable->posturlnotify = 0x%x\n", gExtNetscapeFuncs.posturlnotify);
		DEB(ef, "nsTable->getvalue = 0x%x\n", gExtNetscapeFuncs.getvalue);
		DEB(ef, "nsTable->setvalue = 0x%x\n", gExtNetscapeFuncs.setvalue);
		DEB(ef, "nsTable->invalidaterect = 0x%x\n", gExtNetscapeFuncs.invalidaterect);
		DEB(ef, "nsTable->invalidateregion = 0x%x\n", gExtNetscapeFuncs.invalidateregion);
		DEB(ef, "nsTable->forceredraw = 0x%x\n", gExtNetscapeFuncs.forceredraw);

		DEB(ef, "pluginFuncs->size = %d\n", pluginFuncs->size);
		DEB(ef, "pluginFuncs->version = 0x%x\n", pluginFuncs->version);
		DEB(ef, "pluginFuncs->newp = 0x%x\n", pluginFuncs->newp);
		DEB(ef, "pluginFuncs->destroy = 0x%x\n", pluginFuncs->destroy);
		DEB(ef, "pluginFuncs->setwindow = 0x%x\n", pluginFuncs->setwindow);
		DEB(ef, "pluginFuncs->newstream = 0x%x\n", pluginFuncs->newstream);
		DEB(ef, "pluginFuncs->destroystream = 0x%x\n", pluginFuncs->destroystream);
		DEB(ef, "pluginFuncs->asfile = 0x%x\n", pluginFuncs->asfile);
		DEB(ef, "pluginFuncs->writeready = 0x%x\n", pluginFuncs->writeready);
		DEB(ef, "pluginFuncs->write = 0x%x\n", pluginFuncs->write);
		DEB(ef, "pluginFuncs->print = 0x%x\n", pluginFuncs->print);
		DEB(ef, "pluginFuncs->event = 0x%x\n", pluginFuncs->event);
		DEB(ef, "pluginFuncs->urlnotify = 0x%x\n", pluginFuncs->urlnotify);
		DEB(ef, "pluginFuncs->javaClass = 0x%x\n", pluginFuncs->javaClass);
		DEB(ef, "pluginFuncs->getvalue = 0x%x\n", pluginFuncs->getvalue);
		DEB(ef, "pluginFuncs->setvalue = 0x%x\n", pluginFuncs->setvalue);
	}
		
	DEB(ef, "<- NP_Initialize = %d\n", err );
	return err;
}

extern NPError NP_Shutdown(void);
NPError NP_Shutdown(void)
{
	NPError err;
	LoadPlugin();	
	if ( !gLib ) return NPERR_GENERIC_ERROR;	
	DEB(ef, "-> NP_Shutdown()\n" );
	
	err = gNP_Shutdown( );
	DEB(ef, "<- NP_Shutdown = %d\n", err );
	return err;
}
