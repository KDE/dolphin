// (c) Torben Weis 1998
// (c) David Faure 1998
/*
 * main.cpp for lisa,reslisa,kio_lan and kio_rlan kcm module
 *
 *  Copyright (C) 2000,2001 Alexander Neundorf <neundorf@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qfile.h>

#include <kparts/componentfactory.h>
#include <klocale.h>

#include "kcookiesmain.h"
#include "netpref.h"
#include "smbrodlg.h"
#include "useragentdlg.h"
#include "kproxydlg.h"
#include "cache.h"

#include "main.h"

extern "C"
{

  KCModule *create_cookie(QWidget *parent, const char /**name*/)
  {
    return new KCookiesMain(parent);
  };

  KCModule *create_smb(QWidget *parent, const char /**name*/)
  {
    return new SMBRoOptions(parent);
  };

  KCModule *create_useragent(QWidget *parent, const char /**name*/)
  {
    return new UserAgentOptions(parent);
  };

  KCModule *create_proxy(QWidget *parent, const char /**name*/)
  {
    return new KProxyOptions(parent);
  };

  KCModule *create_cache(QWidget *parent, const char /**name*/)
  {
    return new KCacheConfigDialog( parent );
  };

  KCModule *create_netpref(QWidget *parent, const char /**name*/)
  {
    return new KIOPreferences(parent);
  };

  KCModule *create_lanbrowser(QWidget *parent, const char *)
  {
    return new LanBrowser(parent);
  }

}

static KCModule *load(QWidget *parent, const QString &libname, const QString &library, const QString &handle)
{
    KLibLoader *loader = KLibLoader::self();
    // attempt to load modules with ComponentFactory, only if the symbol init_<lib> exists
    // (this is because some modules, e.g. kcmkio with multiple modules in the library,
    // cannot be ported to KGenericFactory)
    KLibrary *lib = loader->library(QFile::encodeName(libname.arg(library)));
    if (lib) {
        QString initSym("init_");
        initSym += libname.arg(library);

        if ( lib->hasSymbol(QFile::encodeName(initSym)) )
        {
            // Reuse "lib" instead of letting createInstanceFromLibrary recreate it
            //KCModule *module = KParts::ComponentFactory::createInstanceFromLibrary<KCModule>(QFile::encodeName(libname.arg(library)));
            KLibFactory *factory = lib->factory();
            if ( factory )
            {
                KCModule *module = KParts::ComponentFactory::createInstanceFromFactory<KCModule>( factory );
                if (module)
                    return module;
            }
        }

	// get the create_ function
	QString factory("create_%1");
	void *create = lib->symbol(QFile::encodeName(factory.arg(handle)));

	if (create)
	    {
		// create the module
		KCModule* (*func)(QWidget *, const char *);
		func = (KCModule* (*)(QWidget *, const char *)) create;
		return  func(parent, 0);
	    }

        lib->unload();
    }
    return 0;
}

static KCModule *loadModule(QWidget *parent, const QString &module)
{
    KService::Ptr service = KService::serviceByDesktopName(module);
    if (!service)
       return 0;
    QString library = service->library();

    if (library.isEmpty())
       return 0;

    QString handle =  service->property("X-KDE-FactoryName").toString();
    if (handle.isEmpty())
       handle = library;

    KCModule *kcm = load(parent, "kcm_%1", library, handle);
    if (!kcm)
       kcm = load(parent, "libkcm_%1", library, handle);
    return kcm;
}



LanBrowser::LanBrowser(QWidget *parent)
:KCModule(parent,"kcmlanbrowser")
,layout(this)
,tabs(this)
{
   layout.addWidget(&tabs);

   smbPage = create_smb(&tabs, 0);
   tabs.addTab(smbPage, i18n("&Windows Shares"));
   connect(smbPage,SIGNAL(changed(bool)),this,SLOT(slotEmitChanged()));

   lisaPage = loadModule(&tabs, "kcmlisa");
   if (lisaPage)
   {
     tabs.addTab(lisaPage,i18n("&LISa Daemon"));
     connect(lisaPage,SIGNAL(changed()),this,SLOT(slotEmitChanged()));
   }

   resLisaPage = loadModule(&tabs, "kcmreslisa");
   if (resLisaPage)
   {
     tabs.addTab(resLisaPage,i18n("R&esLISa Daemon"));
     connect(resLisaPage,SIGNAL(changed()),this,SLOT(slotEmitChanged()));
   }

   kioLanPage = loadModule(&tabs, "kcmkiolan");
   if (kioLanPage)
   {
     tabs.addTab(kioLanPage,i18n("lan:/ && &rlan:/"));
     connect(kioLanPage,SIGNAL(changed()),this,SLOT(slotEmitChanged()));
   }

   setButtons(Apply|Help);
   load();
}

void LanBrowser::slotEmitChanged()
{
   emit changed(true);
}

void LanBrowser::load()
{
   smbPage->load();
   if (lisaPage)
     lisaPage->load();
   if (resLisaPage)
     resLisaPage->load();
   if (kioLanPage)
     kioLanPage->load();
}

void LanBrowser::save()
{
   smbPage->save();
   if (resLisaPage)
     resLisaPage->save();
   if (kioLanPage)
     kioLanPage->save();
   if (lisaPage)
     lisaPage->save();
}


QString LanBrowser::quickHelp() const
{
   return i18n("<h1>Local Network Browsing</h1>Here you setup your "
		"<b>\"Network Neighborhood\"</b>. You "
		"can use either the LISa daemon and the lan:/ ioslave, or the "
		"ResLISa daemon and the rlan:/ ioslave.<br><br>"
		"About the <b>LAN ioslave</b> configuration:<br> If you select it, the "
		"ioslave, <i>if available</i>, will check whether the host "
		"supports this service when you open this host. Please note "
		"that paranoid people might consider even this to be an attack.<br>"
		"<i>Always</i> means that you will always see the links for the "
		"services, regardless of whether they are actually offered by the host. "
		"<i>Never</i> means that you will never have the links to the services. "
		"In both cases you won't contact the host, so nobody will ever regard "
		"you as an attacker.<br><br>More information about <b>LISa</b> "
		"can be found at <a href=\"http://lisa-home.sourceforge.net\">"
		"the LISa Homepage</a> or contact Alexander Neundorf "
		"&lt;<a href=\"mailto:neundorf@kde.org\">neundorf@kde.org</a>&gt;.");
}

#include "main.moc"

