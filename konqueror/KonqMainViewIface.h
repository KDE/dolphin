
#ifndef __KonqMainViewIface_h__
#define __KonqMainViewIface_h__

#include <dcopobject.h>

class KonqMainViewIface : virtual public DCOPObject
{
  K_DCOP
public:

k_dcop:
  virtual void configure ( ) = 0;
};

#endif
