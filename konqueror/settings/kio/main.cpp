// (c) Torben Weis 1998
// (c) David Faure 1998


#include <klocale.h>
#include <kglobal.h>


#include "kcookiesdlg.h"
#include "ksmboptdlg.h"
#include "useragentdlg.h"
#include "kproxydlg.h"


extern "C" 
{

  KCModule *create_cookie(QWidget *parent, const char *name)
  {
    KGlobal::locale()->insertCatalogue("kcmkio");
    return new KCookiesOptions(parent, name);
  };

  KCModule *create_smb(QWidget *parent, const char *name)
  {
    KGlobal::locale()->insertCatalogue("kcmkio");
    return new KSMBOptions(parent, name);
  };

  KCModule *create_useragent(QWidget *parent, const char *name)
  {
    KGlobal::locale()->insertCatalogue("kcmkio");
    return new UserAgentOptions(parent, name);
  };

  KCModule *create_proxy(QWidget *parent, const char *name)
  {
    KGlobal::locale()->insertCatalogue("kcmkio");
    return new KProxyOptions(parent, name);
  };

}
