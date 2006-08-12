/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 1999 David Faure <faure@kde.org>
    Copyright (C) 1999 Torben Weis <weis@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <kparts/factory.h>
#include "konq_factory.h"
#include "version.h"

#include <konq_view.h>
#include <konq_settings.h>
#include <konq_mainwindow.h>
#include <kdebug.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h> 
#include <kservicetypetrader.h>

#include <QWidget>
#include <QFile>

#include <assert.h>

KAboutData *KonqFactory::s_aboutData = 0;

KonqViewFactory::KonqViewFactory( KLibFactory *factory, const QStringList &args,
                                  bool createBrowser )
    : m_factory( factory ), m_args( args ), m_createBrowser( createBrowser )
{
    if ( m_createBrowser )
        m_args << QLatin1String( "Browser/View" );
}

KParts::ReadOnlyPart *KonqViewFactory::create( QWidget *parentWidget, QObject * parent )
{
  if ( !m_factory )
    return 0L;

  QObject *obj = 0L;

  if ( m_factory->inherits( "KParts::Factory" ) )
  {
    if ( m_createBrowser )
      obj = static_cast<KParts::Factory *>(m_factory)->createPart( parentWidget, parent, "Browser/View", m_args );

    if ( !obj )
      obj = static_cast<KParts::Factory *>(m_factory)->createPart( parentWidget, parent, "KParts::ReadOnlyPart", m_args );
  }
  else
  {
    if ( m_createBrowser )
      obj = m_factory->create( parentWidget, "Browser/View", m_args );

    if ( !obj )
      obj = m_factory->create( parentWidget, "KParts::ReadOnlyPart", m_args );
  }

  if ( !obj->inherits( "KParts::ReadOnlyPart" ) )
      kError(1202) << "Part " << obj << " (" << obj->metaObject()->className() << ") doesn't inherit KParts::ReadOnlyPart !" << endl;

  return static_cast<KParts::ReadOnlyPart *>(obj);
}

KonqViewFactory KonqFactory::createView( const QString &serviceType,
                                         const QString &serviceName,
                                         KService::Ptr *serviceImpl,
                                         KService::List *partServiceOffers,
                                         KService::List *appServiceOffers,
					 bool forceAutoEmbed )
{
  kDebug(1202) << "Trying to create view for \"" << serviceType << "\"" << endl;

  // We need to get those in any case
  KService::List offers, appOffers;

  // Query the trader
  getOffers( serviceType, &offers, &appOffers );

  if ( partServiceOffers )
     (*partServiceOffers) = offers;
  if ( appServiceOffers )
     (*appServiceOffers) = appOffers;

  // We ask ourselves whether to do it or not only if no service was specified.
  // If it was (from the View menu or from RMB + Embedding service), just do it.
  forceAutoEmbed = forceAutoEmbed || !serviceName.isEmpty();
  // Or if we have no associated app anyway, then embed.
  forceAutoEmbed = forceAutoEmbed || ( appOffers.isEmpty() && !offers.isEmpty() );
  // Or if the associated app is konqueror itself, then embed.
  if ( !appOffers.isEmpty() )
    forceAutoEmbed = forceAutoEmbed || KonqMainWindow::isMimeTypeAssociatedWithSelf( serviceType, appOffers.first() );

  if ( ! forceAutoEmbed )
  {
    if ( ! KonqFMSettings::settings()->shouldEmbed( serviceType ) )
    {
      kDebug(1202) << "KonqFMSettings says: don't embed this servicetype" << endl;
      return KonqViewFactory();
    }
  }

  KService::Ptr service;

  // Look for this service
  if ( !serviceName.isEmpty() )
  {
      KService::List::Iterator it = offers.begin();
      for ( ; it != offers.end() && !service ; ++it )
      {
          if ( (*it)->desktopEntryName() == serviceName )
          {
              kDebug(1202) << "Found requested service " << serviceName << endl;
              service = *it;
          }
      }
  }

  KLibFactory *factory = 0L;

  if ( service )
  {
    kDebug(1202) << "Trying to open lib for requested service " << service->desktopEntryName() << endl;
    factory = KLibLoader::self()->factory( QFile::encodeName(service->library()) );
    if ( !factory )
        KMessageBox::error(0,
                           i18n("There was an error loading the module %1.\nThe diagnostics is:\n%2",
                            service->name(), KLibLoader::self()->lastErrorMessage()));
  }

  KService::List::Iterator it = offers.begin();
  for ( ; !factory && it != offers.end() ; ++it )
  {
    service = (*it);
    // Allowed as default ?
    QVariant prop = service->property( "X-KDE-BrowserView-AllowAsDefault" );
    kDebug(1202) << service->desktopEntryName() << " : X-KDE-BrowserView-AllowAsDefault is valid : " << prop.isValid() << endl;
    if ( !prop.isValid() || prop.toBool() ) // defaults to true
    {
      //kDebug(1202) << "Trying to open lib for service " << service->name() << endl;
      // Try loading factory
      factory = KLibLoader::self()->factory( QFile::encodeName(service->library()) );
      if ( !factory )
        KMessageBox::error(0,
                           i18n("There was an error loading the module %1.\nThe diagnostics is:\n%2",
                            service->name(), KLibLoader::self()->lastErrorMessage()));
      // If this works, we exit the loop.
    } else
      kDebug(1202) << "Not allowed as default " << service->desktopEntryName() << endl;
  }

  if ( serviceImpl )
    (*serviceImpl) = service;

  if ( !factory )
  {
    kWarning(1202) << "KonqFactory::createView : no factory" << endl;
    return KonqViewFactory();
  }

  QStringList args;

  QVariant prop = service->property( "X-KDE-BrowserView-Args" );

  if ( prop.isValid() )
  {
    QString argStr = prop.toString();
    args = argStr.split( " ");
  }

  return KonqViewFactory( factory, args, service->serviceTypes().contains( "Browser/View" ) );
}

void KonqFactory::getOffers( const QString & serviceType,
                             KService::List *partServiceOffers,
                             KService::List *appServiceOffers )
{
#warning Temporary hack
    if ( partServiceOffers && serviceType[0].isUpper() ) {
        *partServiceOffers = KServiceTypeTrader::self()->query( serviceType, 
                    "DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'");
        return;

    }
    if ( appServiceOffers )
    {
        *appServiceOffers = KMimeTypeTrader::self()->query( serviceType, "Application",
"DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'");
    }

    if ( partServiceOffers )
    {
        *partServiceOffers = KMimeTypeTrader::self()->query( serviceType, "KParts/ReadOnlyPart" );
    }
}


const KAboutData *KonqFactory::aboutData()
{
  if (!s_aboutData)
  {
    s_aboutData = new KAboutData( "konqueror", I18N_NOOP("Konqueror"),
                        KONQUEROR_VERSION,
                        I18N_NOOP("Web browser, file manager, ..."),
                        KAboutData::License_GPL,
                        I18N_NOOP("(c) 1999-2005, The Konqueror developers"),
                        0,
                        I18N_NOOP("http://konqueror.kde.org") );
    s_aboutData->addAuthor( "David Faure", I18N_NOOP("developer (framework, parts, JavaScript, I/O lib) and maintainer"), "faure@kde.org" );
    s_aboutData->addAuthor( "Simon Hausmann", I18N_NOOP("developer (framework, parts)"), "hausmann@kde.org" );
    s_aboutData->addAuthor( "Michael Reiher", I18N_NOOP("developer (framework)"), "michael.reiher@gmx.de" );
    s_aboutData->addAuthor( "Matthias Welk", I18N_NOOP("developer"), "welk@fokus.gmd.de" );
    s_aboutData->addAuthor( "Alexander Neundorf", I18N_NOOP("developer (List views)"), "neundorf@kde.org" );
    s_aboutData->addAuthor( "Michael Brade", I18N_NOOP("developer (List views, I/O lib)"), "brade@kde.org" );
    s_aboutData->addAuthor( "Lars Knoll", I18N_NOOP("developer (HTML rendering engine)"), "knoll@kde.org" );
    s_aboutData->addAuthor( "Dirk Mueller", I18N_NOOP("developer (HTML rendering engine)"), "mueller@kde.org" );
    s_aboutData->addAuthor( "Peter Kelly", I18N_NOOP("developer (HTML rendering engine)"), "pmk@post.com" );
    s_aboutData->addAuthor( "Waldo Bastian", I18N_NOOP("developer (HTML rendering engine, I/O lib)"), "bastian@kde.org" );
    s_aboutData->addAuthor( "Germain Garand", I18N_NOOP("developer (HTML rendering engine)"), "germain@ebooksfrance.org" );
    s_aboutData->addAuthor( "Leo Savernik", I18N_NOOP("developer (HTML rendering engine)"), "l.savernik@aon.at" );
    s_aboutData->addAuthor( "Stephan Kulow", I18N_NOOP("developer (HTML rendering engine, I/O lib, regression test framework)"), "coolo@kde.org" );
    s_aboutData->addAuthor( "Antti Koivisto", I18N_NOOP("developer (HTML rendering engine)"), "koivisto@kde.org" );
    s_aboutData->addAuthor( "Zack Rusin",  I18N_NOOP("developer (HTML rendering engine)"), "zack@kde.org" );
    s_aboutData->addAuthor( "Tobias Anton", I18N_NOOP( "developer (HTML rendering engine)" ), "anton@stud.fbi.fh-darmstadt.de" );
    s_aboutData->addAuthor( "Lubos Lunak", I18N_NOOP( "developer (HTML rendering engine)" ), "l.lunak@kde.org" );
    s_aboutData->addAuthor( "Allan Sandfeld Jensen", I18N_NOOP( "developer (HTML rendering engine)" ), "kde@carewolf.com" );
    s_aboutData->addAuthor( "Apple Safari Developers", I18N_NOOP("developer (HTML rendering engine, JavaScript)"), "" );
    s_aboutData->addAuthor( "Harri Porten", I18N_NOOP("developer (JavaScript)"), "porten@kde.org" );
    s_aboutData->addAuthor( "Koos Vriezen", I18N_NOOP("developer (Java applets and other embedded objects)"), "koos.vriezen@xs4all.nl" );
    s_aboutData->addAuthor( "Matt Koss", I18N_NOOP("developer (I/O lib)"), "koss@miesto.sk" );
    s_aboutData->addAuthor( "Alex Zepeda", I18N_NOOP("developer (I/O lib)"), "zipzippy@sonic.net" );
    s_aboutData->addAuthor( "Richard Moore", I18N_NOOP("developer (Java applet support)"), "rich@kde.org" );
    s_aboutData->addAuthor( "Dima Rogozin", I18N_NOOP("developer (Java applet support)"), "dima@mercury.co.il" );
    s_aboutData->addAuthor( "Wynn Wilkes", I18N_NOOP("developer (Java 2 security manager support,\n and other major improvements to applet support)"), "wynnw@calderasystems.com" );
    s_aboutData->addAuthor( "Stefan Schimanski", I18N_NOOP("developer (Netscape plugin support)"), "schimmi@kde.org" );
    s_aboutData->addAuthor( "George Staikos", I18N_NOOP("developer (SSL, Netscape plugins)"), "staikos@kde.org" );
    s_aboutData->addAuthor( "Dawit Alemayehu",I18N_NOOP("developer (I/O lib, Authentication support)"), "adawit@kde.org" );
    s_aboutData->addAuthor( "Carsten Pfeiffer",I18N_NOOP("developer (framework)"), "pfeiffer@kde.org" );
    s_aboutData->addAuthor( "Torsten Rahn", I18N_NOOP("graphics/icons"), "torsten@kde.org" );
    s_aboutData->addAuthor( "Torben Weis", I18N_NOOP("kfm author"), "weis@kde.org" );
    s_aboutData->addAuthor( "Joseph Wenninger", I18N_NOOP("developer (navigation panel framework)"),"jowenn@kde.org");
    s_aboutData->addAuthor( "Stephan Binner", I18N_NOOP("developer (misc stuff)"),"binner@kde.org");
    s_aboutData->addAuthor( "Ivor Hewitt", I18N_NOOP("developer (AdBlock filter)"),"ivor@ivor.org");
  }
  return s_aboutData;
}

