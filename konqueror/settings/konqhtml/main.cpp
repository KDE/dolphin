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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <unistd.h>

#include <kapplication.h>
#include <QTabWidget>
#include <QLayout>
#include <QtDBus/QtDBus>

#include "jsopts.h"
#include "javaopts.h"
#include "pluginopts.h"
#include "appearance.h"
#include "htmlopts.h"
#include "filteropts.h"

#include "main.h"
#include <kaboutdata.h>
#include "main.moc"

#include <kgenericfactory.h>
typedef KGenericFactory<KJSParts, QWidget> KJSPartsFactory;
K_EXPORT_COMPONENT_FACTORY( khtml_java_js, KJSPartsFactory("kcmkonqhtml") )

KJSParts::KJSParts(QWidget *parent, const QStringList&)
	: KCModule(KJSPartsFactory::instance(), parent)
{
  mConfig = KSharedConfig::openConfig( "konquerorrc", false, false );
  KAboutData *about =
  new KAboutData(I18N_NOOP("kcmkonqhtml"), I18N_NOOP("Konqueror Browsing Control Module"),
                0, 0, KAboutData::License_GPL,
                I18N_NOOP("(c) 1999 - 2001 The Konqueror Developers"));

  about->addAuthor("Waldo Bastian",0,"bastian@kde.org");
  about->addAuthor("David Faure",0,"faure@kde.org");
  about->addAuthor("Matthias Kalle Dalheimer",0,"kalle@kde.org");
  about->addAuthor("Lars Knoll",0,"knoll@kde.org");
  about->addAuthor("Dirk Mueller",0,"mueller@kde.org");
  about->addAuthor("Daniel Molkentin",0,"molkentin@kde.org");
  about->addAuthor("Wynn Wilkes",0,"wynnw@caldera.com");

  about->addCredit("Leo Savernik",I18N_NOOP("JavaScript access controls\n"
    			"Per-domain policies extensions"),
			"l.savernik@aon.at");

  setAboutData( about );

  QVBoxLayout *layout = new QVBoxLayout(this);
  tab = new QTabWidget(this);
  layout->addWidget(tab);

  // ### the groupname is duplicated in KJSParts::save
  java = new KJavaOptions( mConfig, "Java/JavaScript Settings", instance(), this );
  tab->addTab( java, i18n( "&Java" ) );
  connect( java, SIGNAL( changed( bool ) ), SIGNAL( changed( bool ) ) );

  javascript = new KJavaScriptOptions( mConfig, "Java/JavaScript Settings", instance(), this );
  tab->addTab( javascript, i18n( "Java&Script" ) );
  connect( javascript, SIGNAL( changed( bool ) ), SIGNAL( changed( bool ) ) );
}

void KJSParts::load()
{
  javascript->load();
  java->load();
}


void KJSParts::save()
{
  javascript->save();
  java->save();

  // delete old keys after they have been migrated
  if (javascript->_removeJavaScriptDomainAdvice
      || java->_removeJavaScriptDomainAdvice) {
    mConfig->setGroup("Java/JavaScript Settings");
    mConfig->deleteEntry("JavaScriptDomainAdvice");
    javascript->_removeJavaScriptDomainAdvice = false;
    java->_removeJavaScriptDomainAdvice = false;
  }

  mConfig->sync();

  // Send signal to konqueror
  // Warning. In case something is added/changed here, keep kfmclient in sync
  QDBusMessage message =
      QDBusMessage::signal("/KonqMain", "org.kde.Konqueror.Main", "reparseConfiguration", QDBus::sessionBus());
  QDBus::sessionBus().send(message);
}


void KJSParts::defaults()
{
  javascript->defaults();
  java->defaults();
}

QString KJSParts::quickHelp() const
{
  return i18n("<h2>JavaScript</h2>On this page, you can configure "
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


