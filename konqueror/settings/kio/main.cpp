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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QFile>
#include <QLabel>
#include <QLayout>
#include <QTabWidget>

#include <kcmoduleloader.h>
#include <klocale.h>
#include <kinstance.h>

#include "kcookiesmain.h"
#include "netpref.h"
#include "smbrodlg.h"
#include "useragentdlg.h"
#include "kproxydlg.h"
#include "cache.h"

#include "main.h"

static KInstance *_kcmkio = 0;

inline KInstance *inst() {
        if (_kcmkio)
                return _kcmkio;
        _kcmkio = new KInstance("kcmkio");
        return _kcmkio;
}


extern "C"
{

  KDE_EXPORT KCModule *create_cookie(QWidget *parent, const char /**name*/)
  {
    return new KCookiesMain(inst(), parent);
  }

  KDE_EXPORT KCModule *create_smb(QWidget *parent, const char /**name*/)
  {
    return new SMBRoOptions(inst(), parent);
  }

  KDE_EXPORT KCModule *create_useragent(QWidget *parent, const char /**name*/)
  {
    return new UserAgentDlg(inst(), parent);
  }

  KDE_EXPORT KCModule *create_proxy(QWidget *parent, const char /**name*/)
  {
    return new KProxyOptions(inst(), parent);
  }

  KDE_EXPORT KCModule *create_cache(QWidget *parent, const char /**name*/)
  {
    return new KCacheConfigDialog( inst(), parent );
  }

  KDE_EXPORT KCModule *create_netpref(QWidget *parent, const char /**name*/)
  {
    return new KIOPreferences( inst(), parent );
  }

  KDE_EXPORT KCModule *create_lanbrowser(QWidget *parent, const char *)
  {
    return new LanBrowser( parent );
  }

}

LanBrowser::LanBrowser(QWidget *parent)
:KCModule(inst(), parent)
,layout(this)
,tabs(this)
{
   setQuickHelp( i18n("<h1>Local Network Browsing</h1>Here you setup your "
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
		"In both cases you will not contact the host, so nobody will ever regard "
		"you as an attacker.<br><br>More information about <b>LISa</b> "
		"can be found at <a href=\"http://lisa-home.sourceforge.net\">"
		"the LISa Homepage</a> or contact Alexander Neundorf "
		"&lt;<a href=\"mailto:neundorf@kde.org\">neundorf@kde.org</a>&gt;."));
   
   layout.addWidget(&tabs);

   smbPage = create_smb(&tabs, 0);
   tabs.addTab(smbPage, i18n("&Windows Shares"));
   connect(smbPage,SIGNAL(changed(bool)), SLOT( changed() ));

   lisaPage = KCModuleLoader::loadModule("kcmlisa", KCModuleLoader::None,&tabs);
   if (lisaPage)
   {
     tabs.addTab(lisaPage,i18n("&LISa Daemon"));
     connect(lisaPage,SIGNAL(changed()), SLOT( changed() ));
   }

//   resLisaPage = KCModuleLoader::loadModule("kcmreslisa", KCModuleLoader::None,&tabs);
//   if (resLisaPage)
//   {
//     tabs.addTab(resLisaPage,i18n("R&esLISa Daemon"));
//     connect(resLisaPage,SIGNAL(changed()), SLOT( changed() ));
//   }

   kioLanPage = KCModuleLoader::loadModule("kcmkiolan", KCModuleLoader::None, &tabs);
   if (kioLanPage)
   {
     tabs.addTab(kioLanPage,i18n("lan:/ Iosla&ve"));
     connect(kioLanPage,SIGNAL(changed()), SLOT( changed() ));
   }

   setButtons(Apply|Help);
   load();
}

void LanBrowser::load()
{
   smbPage->load();
   if (lisaPage)
     lisaPage->load();
//   if (resLisaPage)
//     resLisaPage->load();
   if (kioLanPage)
     kioLanPage->load();
   emit changed(false);
}

void LanBrowser::save()
{
   smbPage->save();
//   if (resLisaPage)
//     resLisaPage->save();
   if (kioLanPage)
     kioLanPage->save();
   if (lisaPage)
     lisaPage->save();
   emit changed(false);
}

#include "main.moc"

