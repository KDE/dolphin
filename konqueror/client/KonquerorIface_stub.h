#ifndef __KonquerorIface_STUB__
#define __KonquerorIface_STUB__

#include <dcopstub.h>

class KonquerorIface_stub : virtual public DCOPStub
{
public:
	KonquerorIface_stub( const QCString& app, const QCString& id );
	virtual void configure() ;
	virtual void openBrowserWindow(const QString& url) ;
};

#include <dcopobject.h>
#endif
