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
  m_globalConfig = new KConfig("khtmlrc", false, false);
  m_localConfig = new KConfig( "konquerorrc", false, false );

  QVBoxLayout *layout = new QVBoxLayout(this);
  tab = new QTabWidget(this);
  layout->addWidget(tab);

  misc = new KMiscHTMLOptions(m_globalConfig, "HTML Settings", this);
  tab->addTab(misc, i18n("&HTML"));
  connect(misc, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  appearance = new KAppearanceOptions(m_globalConfig, "HTML Settings", this);
  tab->addTab(appearance, i18n("&Appearance"));
  connect(appearance, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  advanced = new KAdvancedOptions(m_localConfig, "HTML Settings", this);
  tab->addTab(advanced, i18n("&Advanced"));
  connect(advanced, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  http = new KHTTPOptions(m_globalConfig, "HTML Settings", this);
  tab->addTab(http, i18n("H&TTP"));
  connect(http, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

}

KonqHTMLModule::~KonqHTMLModule()
{
  delete m_localConfig;
  delete m_globalConfig;
}

void KonqHTMLModule::load()
{
  appearance->load();
  advanced->load();
  http->load();
  misc->load();
}


void KonqHTMLModule::save()
{
  appearance->save();
  advanced->save();
  http->save();
  misc->save();

  // Send signal to konqueror
  // Warning. In case something is added/changed here, keep kfmclient in sync
  QByteArray data;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "konqueror*", "KonqMainViewIface", "reparseConfiguration()", data );
}


void KonqHTMLModule::defaults()
{
  appearance->defaults();
  advanced->defaults();
  http->defaults();
  misc->defaults();
}

QString KonqHTMLModule::quickHelp()
{
  return i18n("<h1>Konqueror Browser</h1> Here you can configure konqueror's browser functionality. "
    "Please note that the file manager functionality has to be configured using the \"File Manager\" "
    "configuration module. <h2>HTML</h2> ");
}


void KonqHTMLModule::moduleChanged(bool state)
{
  emit changed(state);
}

extern "C"
{

  KCModule *create_html(QWidget *parent, const char *name)
  {
    KGlobal::locale()->insertCatalogue("kcmkonqhtml");
    return new KonqHTMLModule(parent, name);
  }

}


