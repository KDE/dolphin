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

#include "htmlopts.h"
#include "khttpoptdlg.h"
#include "miscopts.h"

#include "main.h"
#include "main.moc"


KonqHTMLModule::KonqHTMLModule(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
  KConfig *config = new KConfig("konquerorrc", false, false);
  
  tab = new QTabWidget(this);

  misc = new KMiscHTMLOptions(config, "HTML Settings", this);
  tab->addTab(misc, i18n("&HTML"));
  connect(misc, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  font = new KFontOptions(config, "HTML Settings", this);
  tab->addTab(font, i18n("&Fonts"));
  connect(font, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  color = new KColorOptions(config, "HTML Settings", this);
  tab->addTab(color, i18n("&Colors"));
  connect(color, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  advanced = new KAdvancedOptions(config, "HTML Settings", this);
  tab->addTab(advanced, i18n("&Advanced"));
  connect(advanced, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  http = new KHTTPOptions(config, "HTML Settings", this);
  tab->addTab(http, i18n("H&TTP"));
  connect(http, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

}


void KonqHTMLModule::load()
{
  font->load();
  color->load();
  advanced->load();
  http->load();
  misc->load();
}


void KonqHTMLModule::save()
{
  font->save();
  color->save();
  advanced->save();
  http->save();
  misc->save();
  
  // Send signal to konqueror
  // Warning. In case something is added/changed here, keep kfmclient in sync
  QByteArray data;
  kapp->dcopClient()->send( "*", "KonqMainViewIface", "reparseConfiguration()", data ); 
}


void KonqHTMLModule::defaults()
{
  font->defaults();
  color->defaults();
  advanced->defaults();
  http->defaults();
  misc->defaults();
}


void KonqHTMLModule::moduleChanged(bool state)
{
  emit changed(state);
}


void KonqHTMLModule::resizeEvent(QResizeEvent *)
{
  tab->setGeometry(0,0,width(),height());
}


extern "C"
{

  KCModule *create_html(QWidget *parent, const char *name) 
  { 
    KGlobal::locale()->insertCatalogue("kcmkonqhtml");
    return new KonqHTMLModule(parent, name);
  }

}


