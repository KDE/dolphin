#ifndef __KDesktopIface_STUB__
#define __KDesktopIface_STUB__

#include <dcopstub.h>
#include <dcopobject.h>
#include <qstringlist.h>

class KDesktopIface_stub : virtual public DCOPStub
{
public:
	KDesktopIface_stub( const QCString& app, const QCString& id );
	virtual void rearrangeIcons(int bAsk) ;
	virtual void selectIconsInRect(int x, int y, int dx, int dy) ;
	virtual void selectAll() ;
	virtual void unselectAll() ;
	virtual QStringList selectedURLs() ;
	virtual void configure() ;
};

#endif
