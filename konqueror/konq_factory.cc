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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <kparts/browserextension.h>
#include <kparts/factory.h>
#include "konq_factory.h"
#include "konq_misc.h"
#include "konq_run.h"
#include "version.h"

#include <konq_settings.h>
#include <kdebug.h>
#include <kstddirs.h>
#include <kuserprofile.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <qwidget.h>
#include <qfile.h>

#include <assert.h>

KAboutData *KonqFactory::s_aboutData = 0;

KParts::ReadOnlyPart *KonqViewFactory::create( QWidget *parentWidget, const char *widgetName,
                                               QObject * parent, const char *name )
{
  if ( !m_factory )
    return 0L;

  QObject *obj = 0L;

  if ( m_factory->inherits( "KParts::Factory" ) )
  {
    if ( m_createBrowser )
      obj = static_cast<KParts::Factory *>(m_factory)->createPart( parentWidget, widgetName, parent, name, "Browser/View", m_args );

    if ( !obj )
      obj = static_cast<KParts::Factory *>(m_factory)->createPart( parentWidget, widgetName, parent, name, "KParts::ReadOnlyPart", m_args );
  }
  else
  {
    if ( m_createBrowser )
      obj = m_factory->create( parentWidget, name, "Browser/View", m_args );

    if ( !obj )
      obj = m_factory->create( parentWidget, name, "KParts::ReadOnlyPart", m_args );
  }

  if ( !obj->inherits( "KParts::ReadOnlyPart" ) )
      kdError(1202) << "Part " << obj << " (" << obj->className() << ") doesn't inherit KParts::ReadOnlyPart !" << endl;

  return static_cast<KParts::ReadOnlyPart *>(obj);
}

KonqViewFactory KonqFactory::createView( const QString &serviceType,
                                         const QString &serviceName,
                                         KService::Ptr *serviceImpl,
                                         KTrader::OfferList *partServiceOffers,
                                         KTrader::OfferList *appServiceOffers )
{
  kdDebug(1202) << "Trying to create view for \"" << serviceType << "\"" << endl;

  // We ask ourselves whether to do it or not only if no service was specified.
  // If it was (from the View menu or from RMB + Embedding service), just do it.
  bool forceAutoEmbed = !serviceName.isEmpty();
  if ( ! forceAutoEmbed )
  {
    if ( ! KonqFMSettings::settings()->shouldEmbed( serviceType ) )
    {
      kdDebug(1202) << "KonqFMSettings says: don't embed this servicetype" << endl;
      return KonqViewFactory();
    }
  }

  // We need to get those in any case
  KTrader::OfferList offers;

  // Query the trader
  getOffers( serviceType, &offers, appServiceOffers );

  if ( partServiceOffers )
     (*partServiceOffers) = offers;

  KService::Ptr service = 0L;

  // Look for this service
  if ( !serviceName.isEmpty() )
  {
      KTrader::OfferList::Iterator it = offers.begin();
      for ( ; it != offers.end() && !service ; ++it )
      {
          if ( (*it)->desktopEntryName() == serviceName )
          {
              kdDebug(1202) << "Found requested service " << serviceName << endl;
              service = *it;
          }
      }
  }

  KLibFactory *factory = 0L;

  if ( service )
  {
    kdDebug(1202) << "Trying to open lib for requested service " << service->desktopEntryName() << endl;
    factory = KLibLoader::self()->factory( QFile::encodeName(service->library()) );
    if ( !factory )
        KMessageBox::error(0,
                           i18n("There was an error loading the module %1.\nThe diagnostics is:\n%2")
                           .arg(service->name()).arg(KLibLoader::self()->lastErrorMessage()));
  }

  KTrader::OfferList::Iterator it = offers.begin();
  for ( ; !factory && it != offers.end() ; ++it )
  {
    service = (*it);
    // Allowed as default ?
    QVariant prop = service->property( "X-KDE-BrowserView-AllowAsDefault" );
    kdDebug(1202) << service->desktopEntryName() << " : X-KDE-BrowserView-AllowAsDefault is valid : " << prop.isValid() << endl;
    if ( !prop.isValid() || prop.toBool() ) // defaults to true
    {
      //kdDebug(1202) << "Trying to open lib for service " << service->name() << endl;
      // Try loading factory
      factory = KLibLoader::self()->factory( QFile::encodeName(service->library()) );
      if ( !factory )
        KMessageBox::error(0,
                           i18n("There was an error loading the module %1.\nThe diagnostics is:\n%2")
                           .arg(service->name()).arg(KLibLoader::self()->lastErrorMessage()));
      // If this works, we exit the loop.
    } else
      kdDebug(1202) << "Not allowed as default " << service->desktopEntryName() << endl;
  }

  if ( serviceImpl )
    (*serviceImpl) = service;

  if ( !factory )
  {
    kdWarning(1202) << "KonqFactory::createView : no factory" << endl;
    return KonqViewFactory();
  }

  QStringList args;

  QVariant prop = service->property( "X-KDE-BrowserView-Args" );

  if ( prop.isValid() )
  {
    QString argStr = prop.toString();
    args = QStringList::split( " ", argStr );
  }

  return KonqViewFactory( factory, args, service->serviceTypes().contains( "Browser/View" ) );
}

void KonqFactory::getOffers( const QString & serviceType,
                             KTrader::OfferList *partServiceOffers,
                             KTrader::OfferList *appServiceOffers )
{
    if ( appServiceOffers )
    {
        *appServiceOffers = KTrader::self()->query( serviceType, "Application",
"DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'",
                                                    QString::null );
    }

    if ( partServiceOffers )
    {
        *partServiceOffers = KTrader::self()->query( serviceType, "KParts/ReadOnlyPart",
                                                     QString::null, QString::null );
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
                        "(c) 1999-2001, The Konqueror developers",
                        0,
                        "http://www.konqueror.org" );
    s_aboutData->addAuthor( "David Faure", I18N_NOOP("developer (parts, I/O lib) and maintainer"), "faure@kde.org" );
    s_aboutData->addAuthor( "Simon Hausmann", I18N_NOOP("developer (framework, parts)"), "hausmann@kde.org" );
    s_aboutData->addAuthor( "Michael Reiher", I18N_NOOP("developer (framework)"), "michael.reiher@gmx.de" );
    s_aboutData->addAuthor( "Matthias Welk", I18N_NOOP("developer"), "welk@fokus.gmd.de" );
    s_aboutData->addAuthor( "Alexander Neundorf", I18N_NOOP("developer (List views)"), "neundorf@kde.org" );
    s_aboutData->addAuthor( "Michael Brade", I18N_NOOP("developer (List views)"), "brade@informatik.uni-muenchen.de" );
    s_aboutData->addAuthor( "Lars Knoll", I18N_NOOP("developer (HTML rendering engine)"), "knoll@kde.org" );
    s_aboutData->addAuthor( "Antti Koivisto", I18N_NOOP("developer (HTML rendering engine)"), "koivisto@kde.org" );
    s_aboutData->addAuthor( "Dirk Mueller", I18N_NOOP("developer (HTML rendering engine)"), "mueller@kde.org" );
    s_aboutData->addAuthor( "Peter Kelly", I18N_NOOP("developer (HTML rendering engine)"), "pmk@post.com" );
    s_aboutData->addAuthor( "Waldo Bastian", I18N_NOOP("developer (HTML rendering engine, I/O lib)"), "bastian@kde.org" );
    s_aboutData->addAuthor( "Matt Koss", I18N_NOOP("developer (I/O lib)"), "koss@miesto.sk" );
    s_aboutData->addAuthor( "Alex Zepeda", I18N_NOOP("developer (I/O lib)"), "jazepeda@pacbell.net" );
    s_aboutData->addAuthor( "Stephan Kulow", I18N_NOOP("developer (I/O lib)"), "coolo@kde.org" );
    s_aboutData->addAuthor( "Richard Moore", I18N_NOOP("developer (Java applet support)"), "rich@kde.org" );
    s_aboutData->addAuthor( "Dima Rogozin", I18N_NOOP("developer (Java applet support)"), "dima@mercury.co.il" );
    s_aboutData->addAuthor( "Wynn Wilkes", I18N_NOOP("developer (Java 2 security manager support,\n and other major improvements to applet support)"), "wynnw@calderasystems.com" );
    s_aboutData->addAuthor( "Harri Porten", I18N_NOOP("developer (JavaScript)"), "porten@kde.org" );
    s_aboutData->addAuthor( "Stefan Schimanski", I18N_NOOP("developer (Netscape plugin support)"), "schimmi@kde.org" );
    s_aboutData->addAuthor( "George Staikos", I18N_NOOP("developer (SSL support)"), "staikos@kde.org" );
    s_aboutData->addAuthor( "Dawit Alemayehu",I18N_NOOP("developer (I/O lib, Authentication support)"), "adawit@kde.org" );
    s_aboutData->addAuthor( "Carsten Pfeiffer",I18N_NOOP("developer (framework)"), "pfeiffer@kde.org" );
    s_aboutData->addAuthor( "Torsten Rahn", I18N_NOOP("graphics / icons"), "torsten@kde.org" );
    s_aboutData->addAuthor( "Torben Weis", I18N_NOOP("kfm author"), "weis@kde.org" );
  }
  return s_aboutData;
}

