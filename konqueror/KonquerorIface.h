
#ifndef __KonquerorIface_h__
#define __KonquerorIface_h__

#include <dcopobject.h>

class KonquerorIface : virtual public DCOPObject
{
  K_DCOP
public:

k_dcop:
  virtual void openBrowserWindow( const QString &url ) = 0;

};

#endif

