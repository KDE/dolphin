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


#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kmessagebox.h>


#include "rootopts.h"
#include "behaviour.h"
#include "htmlopts.h"
#include "khttpoptdlg.h"
#include "miscopts.h"

#include "main.h"


KonqyModule::KonqyModule(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
  tab = new QTabWidget(this);

  behaviour = new KBehaviourOptions(this);
  tab->addTab(behaviour, i18n("&Behaviour"));
  connect(behaviour, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  font = new KFontOptions(this);
  tab->addTab(font, i18n("&Fonts"));
  connect(font, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  color = new KColorOptions(this);
  tab->addTab(color, i18n("&Colors"));
  connect(color, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  html = new KHtmlOptions(this);
  tab->addTab(html, i18n("&HTML"));
  connect(html, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  http = new KHTTPOptions(this);
  tab->addTab(http, i18n("H&TTP"));
  connect(http, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  misc = new KMiscOptions(this);
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


extern "C"
{

  KCModule *create_icons(QWidget *parent, const char *name) 
  { 
    KGlobal::locale()->insertCatalogue("kcmkonq");
    return new KRootOptions(parent, name);
  }

  KCModule *create_konqueror(QWidget *parent, const char *name) 
  { 
    KGlobal::locale()->insertCatalogue("kcmkonq");
    return new KonqyModule(parent, name);
  }

}


