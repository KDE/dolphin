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
#include "konq_factory.h"
#include "konq_misc.h"
#include "konq_run.h"
#include "version.h"
#include "konq_propsview.h"

#include <konqsettings.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kstddirs.h>
#include <kuserprofile.h>
#include <kaboutdata.h>
#include <klocale.h>

#include <assert.h>

unsigned long KonqFactory::m_instanceRefCnt = 0;
KInstance *KonqFactory::s_instance = 0;
KAboutData *KonqFactory::s_aboutData = 0;
KonqPropsView *KonqFactory::s_defaultViewProps = 0;

KParts::ReadOnlyPart *KonqViewFactory::create( QWidget *parent, const char *name )
{
  if ( !m_factory )
    return 0L;

  KParts::ReadOnlyPart *obj = 0L;

  if ( m_createBrowser )
    obj = (KParts::ReadOnlyPart *)m_factory->create( parent, name, "Browser/View", m_args );

  if ( !obj )
    obj = (KParts::ReadOnlyPart *)m_factory->create( parent, name, "KParts::ReadOnlyPart", m_args );
  return obj;
}

KonqFactory::KonqFactory()
{
  s_instance = 0;
  /*QString path = */instance()->dirs()->saveLocation("data", "kfm/bookmarks", true);
  s_defaultViewProps = 0;
}

KonqFactory::~KonqFactory()
{
  if ( s_instance )
    delete s_instance;

  s_instance = 0L;
  
  if ( s_defaultViewProps )
    delete s_defaultViewProps;
  
  s_defaultViewProps = 0;
}

KonqViewFactory KonqFactory::createView( const QString &serviceType,
                                         const QString &serviceName,
				         KService::Ptr *serviceImpl,
				         KTrader::OfferList *partServiceOffers,
					 KTrader::OfferList *appServiceOffers )
{
  kdDebug(1202) << QString("trying to create view for \"%1\"").arg(serviceType) << endl;

  // We ask ourselves whether to do it or not only if no service was specified.
  // If it was (from the View menu or from RMB + Embedding service), just do it.
  bool forceAutoEmbed = !serviceName.isEmpty();
  if ( ! forceAutoEmbed )
  {
    if ( ! KonqFMSettings::settings()->shouldEmbed( serviceType ) )
      return KonqViewFactory();
  }

  KTrader::OfferList offers = KTrader::self()->query( serviceType );

  if ( appServiceOffers )
  {
    *appServiceOffers = offers;
    KTrader::OfferList::Iterator it = appServiceOffers->begin();
    while ( it != appServiceOffers->end() )
    {
      if ( (*it)->type() != "Application" )
      {
        appServiceOffers->remove( it );
	it = appServiceOffers->begin();
	continue;
      }
      ++it;
    }
  }

  KTrader::OfferList::Iterator it = offers.begin();
  while ( it != offers.end() )
  {
    QStringList serviceTypes = (*it)->serviceTypes();
    if ( !serviceTypes.contains( "KParts/ReadOnlyPart" ) && !serviceTypes.contains( "Browser/View" ) )
    {
      offers.remove( it );
      it = offers.begin();
      continue;
    }
    ++it;
  }

  KService::Ptr service;
  KLibFactory *factory = 0L;

  while ( 42 )
  {

    if ( offers.count() == 0 )
      return KonqViewFactory();

    service = offers.first();

    if ( !serviceName.isEmpty() )
    {
      KTrader::OfferList::ConstIterator it = offers.begin();
      KTrader::OfferList::ConstIterator end = offers.end();
      for (; it != end; ++it )
        if ( (*it)->name() == serviceName )
        {
          service = *it;
          break;
        }
    }
    else
    {
      KTrader::OfferList::ConstIterator it = offers.begin();
      KTrader::OfferList::ConstIterator end = offers.end();
      for (; it != end; ++it )
      {
        QVariant prop = (*it)->property( "X-KDE-BrowserView-AllowAsDefault" );
        if ( prop.isValid() && prop.toBool() )
        {
          service = *it;
          break;
        }
      }
    }

    factory = KLibLoader::self()->factory( service->library() );

    if ( factory )
      break;

    offers.remove( offers.begin() );
  }

  if ( partServiceOffers )
    (*partServiceOffers) = offers;

  if ( serviceImpl )
    (*serviceImpl) = service;

  QStringList args;

  QVariant prop = service->property( "X-KDE-BrowserView-Args" );

  if ( prop.isValid() )
  {
    QString argStr = prop.toString();
    args = QStringList::split( " ", argStr );
  }

  return KonqViewFactory( factory, args, service->serviceTypes().contains( "Browser/View" ) );
}

void KonqFactory::instanceRef()
{
  m_instanceRefCnt++;
}

void KonqFactory::instanceUnref()
{
  m_instanceRefCnt--;
  if ( m_instanceRefCnt == 0 && s_instance )
  {
    delete s_instance;
    s_instance = 0;

  }
}

KInstance *KonqFactory::instance()
{
  if ( !s_instance )
    s_instance = new KInstance( aboutData() );

  return s_instance;
}

const KAboutData *KonqFactory::aboutData()
{
  if (!s_aboutData)
  {
    s_aboutData = new KAboutData( "konqueror", I18N_NOOP("Konqueror"),
                        KONQUEROR_VERSION,
                        I18N_NOOP("Web browser, file manager, ..."),
                        KAboutData::License_GPL,
                        "(c) 1999-2000, The Konqueror developers" );
    s_aboutData->addAuthor( "Torben Weis", I18N_NOOP("kfm author"), "weis@kde.org" );
    s_aboutData->addAuthor( "David Faure", I18N_NOOP("developer (parts, I/O lib) and maintainer"), "faure@kde.org" );
    s_aboutData->addAuthor( "Simon Hausmann", I18N_NOOP("developer (framework, parts)"), "hausmann@kde.org" );
    s_aboutData->addAuthor( "Michael Reiher", I18N_NOOP("developer (framework)"), "michael.reiher@gmx.de" );
    s_aboutData->addAuthor( "Matthias Welk", I18N_NOOP("developer"), "welk@fokus.gmd.de" );
    s_aboutData->addAuthor( "Lars Knoll", I18N_NOOP("developer (HTML rendering engine)"), "knoll@kde.org" );
    s_aboutData->addAuthor( "Antti Koivisto", I18N_NOOP("developer (HTML rendering engine)"), "koivisto@kde.org" );
    s_aboutData->addAuthor( "Waldo Bastian", I18N_NOOP("developer (HTML rendering engine, I/O lib)"), "bastian@kde.org" );
    s_aboutData->addAuthor( "Matt Koss", I18N_NOOP("developer (I/O lib)"), "koss@napri.sk" );
    s_aboutData->addAuthor( "Alex Zepeda", I18N_NOOP("developer (I/O lib)"), "jazepeda@pacbell.net" );
    s_aboutData->addAuthor( "Stephan Kulow", I18N_NOOP("developer (I/O lib)"), "coolo@kde.org" );
  }
  return s_aboutData;
}

KonqPropsView *KonqFactory::defaultViewProps()
{
  if ( !s_defaultViewProps )
    s_defaultViewProps = KonqPropsView::defaultProps( instance() );
  
  return s_defaultViewProps;
}
