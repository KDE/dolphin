// (c) Torben Weis 1998
// (c) David Faure 1998


#include <klocale.h>
#include <kglobal.h>


#include "kcookiesmain.h"
#include "netpref.h"
#include "smbrodlg.h"
#include "useragentdlg.h"
#include "kproxydlg.h"
#include "cache.h"


extern "C"
{

  KCModule *create_cookie(QWidget *parent, const char *name)
  {
    return new KCookiesMain(parent, "kcmkio");
  };

  KCModule *create_smb(QWidget *parent, const char *name)
  {
    return new SMBRoOptions(parent, "kcmkio");
    //return new KSMBOptions(parent, name);
  };

  KCModule *create_useragent(QWidget *parent, const char *name)
  {
    return new UserAgentOptions(parent, "kcmkio");
  };

  KCModule *create_proxy(QWidget *parent, const char *name)
  {
    return new KProxyDialog(parent, "kcmkio");
  };

  KCModule *create_cache(QWidget *parent, const char *name)
  {
    return new KCacheConfigDialog( parent, "kcmkio" );
  };

  KCModule *create_netpref(QWidget *parent, const char *name)
  {
    return new KIOPreferences(parent, "kcmkio");
  };
}
