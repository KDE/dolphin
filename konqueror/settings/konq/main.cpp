/*
 * main.cpp
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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


#include <unistd.h>


#include <kapp.h>
#include <dcopclient.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kmessagebox.h>
#include <kconfig.h>


#include "rootopts.h"
#include "behaviour.h"
#include "htmlopts.h"
#include "khttpoptdlg.h"
#include "miscopts.h"

#include "main.h"
#include "main.moc"


KonqyModule::KonqyModule(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
  KConfig *config = new KConfig("konquerorrc", false, false);
  
  tab = new QTabWidget(this);

  behaviour = new KBehaviourOptions(config, "HTML Settings", this);
  tab->addTab(behaviour, i18n("&Behaviour"));
  connect(behaviour, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  font = new KFontOptions(config, "HTML Settings", this);
  tab->addTab(font, i18n("&Fonts"));
  connect(font, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  color = new KColorOptions(config, "HTML Settings", this);
  tab->addTab(color, i18n("&Colors"));
  connect(color, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  html = new KHtmlOptions(config, "HTML Settings", this);
  tab->addTab(html, i18n("&HTML"));
  connect(html, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  http = new KHTTPOptions(config, "HTML Settings", this);
  tab->addTab(http, i18n("H&TTP"));
  connect(http, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  misc = new KMiscOptions(config, "HTML Settings", this);
  tab->addTab(misc, i18n("&Other"));
  connect(misc, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

}


void KonqyModule::load()
{
  behaviour->load();
  font->load();
  color->load();
  html->load();
  http->load();
  misc->load();
}


void KonqyModule::save()
{
  behaviour->save();
  font->save();
  color->save();
  html->save();
  http->save();
  misc->save();
  
  // Send signal to konqueror
  // Warning. In case something is added/changed here, keep kfmclient in sync
  QByteArray data;
  kapp->dcopClient()->send( "*", "KonqMainViewIface", "reparseConfiguration()", data ); 
}


void KonqyModule::defaults()
{
  behaviour->defaults();
  font->defaults();
  color->defaults();
  html->defaults();
  http->defaults();
  misc->defaults();
}


void KonqyModule::moduleChanged(bool state)
{
  emit changed(state);
}


void KonqyModule::resizeEvent(QResizeEvent *)
{
  tab->setGeometry(0,0,width(),height());
}


KDesktopModule::KDesktopModule(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
  KConfig *config = new KConfig("konquerorrc", false, false);

  tab = new QTabWidget(this);

  root = new KRootOptions(config, "Desktop Settings", this);
  tab->addTab(root, i18n("&Behaviour"));
  connect(root, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  font = new KFontOptions(config, "Desktop Settings", this);
  tab->addTab(font, i18n("&Fonts"));
  connect(font, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  color = new KColorOptions(config, "Desktop Settings", this);
  tab->addTab(color, i18n("&Colors"));
  connect(color, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  html = new KHtmlOptions(config, "Desktop Settings", this);
  tab->addTab(html, i18n("&HTML"));
  connect(html, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

}


void KDesktopModule::load()
{
  font->load();
  color->load();
  html->load();
  root->load();
}


void KDesktopModule::save()
{
  font->save();
  color->save();
  html->save();
  root->load();

#warning David: What to do here?

  /*
  QString exeloc = locate("exe","kfmclient");
  if ( exeloc.isEmpty() ) {
  	  KMessageBox::error( 0L,
	  i18n( "Can't find the kfmclient program - can't apply configuration dynamically" ), i18n( "Error" ) );
	return;
  }

  QApplication::flushX();

  if ( fork() == 0 )
  {
    // execute 'kfmclient configure'
    execl(exeloc, "kfmclient", "configure", 0L);
    warning("Error launching 'kfmclient configure' !");
    exit(1);
  }
  */
}


void KDesktopModule::defaults()
{
  font->defaults();
  color->defaults();
  html->defaults();
  root->load();
}


void KDesktopModule::moduleChanged(bool state)
{
  emit changed(state);
}


void KDesktopModule::resizeEvent(QResizeEvent *)
{
  tab->setGeometry(0,0,width(),height());
}


extern "C"
{

  KCModule *create_icons(QWidget *parent, const char *name) 
  { 
    KGlobal::locale()->insertCatalogue("kcmkonq");
    return new KRootOptions(new KConfig("kdesktoprc", false, false), "", parent, name);
  }

  KCModule *create_konqueror(QWidget *parent, const char *name) 
  { 
    KGlobal::locale()->insertCatalogue("kcmkonq");
    return new KonqyModule(parent, name);
  }

  KCModule *create_desktop(QWidget *parent, const char *name) 
  { 
    KGlobal::locale()->insertCatalogue("kcmkonq");
    return new KDesktopModule(parent, name);
  }

}


