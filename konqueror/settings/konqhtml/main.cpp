/*
 * main.cpp
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 * Copyright (c) 2000 Daniel Molkentin <molkentin@kde.org>
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
#include <qmessagebox.h>
#include <qtabwidget.h>
#include <qlayout.h>

#include "htmlopts.h"
#include "khttpoptdlg.h"
#include "jsopts.h"
#include "javaopts.h"
#include "pluginopts.h"
#include "appearance.h"
#include "htmlopts.h"

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

  misc = new KMiscHTMLOptions(m_localConfig, "HTML Settings", this);
  tab->addTab( misc, i18n("&HTML") );
  connect(misc, SIGNAL( changed( bool ) ), this, SLOT(moduleChanged(bool) ) );

  appearance = new KAppearanceOptions(m_localConfig, "HTML Settings", this);
  tab->addTab(appearance, i18n( "&Appearance" ) );
  connect(appearance, SIGNAL( changed(bool) ), this, SLOT(moduleChanged(bool) ) );

  java = new KJavaOptions( m_localConfig, "Java/JavaScript Settings", this );
  tab->addTab( java, i18n( "&Java" ) );
  connect( java, SIGNAL( changed( bool ) ), this, SLOT( moduleChanged( bool ) ) );

  javascript = new KJavaScriptOptions( m_localConfig, "Java/JavaScript Settings", this );
  tab->addTab( javascript, i18n( "Java&Script" ) );
  connect( javascript, SIGNAL( changed( bool ) ), this, SLOT( moduleChanged( bool ) ) );

  plugin = new KPluginOptions( m_localConfig, "Java/JavaScript Settings", this );
  tab->addTab( plugin, i18n( "&Plugins" ) );
  connect( plugin, SIGNAL( changed( bool ) ), this, SLOT( moduleChanged( bool ) ) );

}

KonqHTMLModule::~KonqHTMLModule()
{
  delete m_localConfig;
  delete m_globalConfig;
}

void KonqHTMLModule::load()
{
  appearance->load();
  javascript->load();
  java->load();
  plugin->load();
  misc->load();
}


void KonqHTMLModule::save()
{
  appearance->save();
  javascript->save();
  java->save();
  plugin->save();
  misc->save();

  // Send signal to konqueror
  // Warning. In case something is added/changed here, keep kfmclient in sync
  QByteArray data;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );
}


void KonqHTMLModule::defaults()
{
  appearance->defaults();
  javascript->defaults();
  java->defaults();
  plugin->defaults();
  misc->defaults();
}

QString KonqHTMLModule::quickHelp() const
{
  return i18n("<h1>Konqueror Browser</h1> Here you can configure Konqueror's browser "
              "functionality. Please note that the file manager "
              "functionality has to be configured using the \"File Manager\" "
              "configuration module."
              "<h2>HTML</h2>On the HTML page, you can make some "
              "settings how Konqueror should handle the HTML code in "
              "the web pages it loads. It is usually not necessary to "
              "change anything here."
              "<h2>Appearance</h2>On this page, you can configure "
              "which fonts Konqueror should use to display the web "
              "pages you view."
              "<h2>JavaScript</h2>On this page, you can configure "
              "whether JavaScript programs embedded in web pages should "
              "be allowed to be executed by Konqueror."
              "<h2>Java</h2>On this page, you can configure "
              "whether Java applets embedded in web pages should "
              "be allowed to be executed by Konqueror."
              "<br><br><b>Note:</b> Active content is always a "
              "security risk, which is why Konqueror allows you to specify very "
              "fine-grained from which hosts you want to execute Java and/or "
              "JavaScript programs." );
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


