
#ifndef __KonquerorIface_h__
#define __KonquerorIface_h__

#include <dcopobject.h>
#include <dcopref.h>

class KonquerorIface : virtual public DCOPObject
{
  K_DCOP
public:

k_dcop:
  virtual void openBrowserWindow( const QString &url ) = 0;

  virtual ASYNC createBrowserWindowFromProfile( const QString &filename ) = 0;

  virtual ASYNC setMoveSelection( int move ) = 0;

};

#endif

