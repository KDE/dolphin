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

#include <kbrowser.h>
#include "konq_factory.h"
#include "konq_part.h"
#include "konq_misc.h"
#include "konq_run.h"

#include <konqsettings.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kstddirs.h>

unsigned long KonqFactory::m_instanceRefCnt = 0;
KInstance *KonqFactory::s_instance = 0L;

extern "C"
{
  void *init_libkonqueror()
  {
    return new KonqFactory;
  }
};

BrowserView *KonqViewFactory::create( QWidget *parent, const char *name )
{
  if ( !m_factory )
    return 0L;

  return (BrowserView *)m_factory->create( parent, name, "QObject", m_args );
}

KonqFactory::KonqFactory()
{
  s_instance = 0L;
  QString path = instance()->dirs()->saveLocation("data", "kfm/bookmarks", true);
  m_bookmarkManager = new KonqBookmarkManager( path );
  m_fileManager = new KonqFileManager;
}

KonqFactory::~KonqFactory()
{
  delete m_bookmarkManager;
  delete m_fileManager;

  if ( s_instance )
    delete s_instance;

  s_instance = 0L;
}

KonqViewFactory KonqFactory::createView( const QString &serviceType,
                                         const QString &serviceName,
				         KService::Ptr *serviceImpl,
				         KTrader::OfferList *serviceOffers )
{
  kdebug(0,1202,QString("trying to create view for \"%1\"").arg(serviceType));

  // First check user's settings
  QString mimeTypeGroup = serviceType.left(serviceType.find("/"));
  if (serviceType != "text/html") // HACK. Will be replaced by a X-KDE-Embed setting in the mimetypes
    if (! KonqFMSettings::defaultIconSettings()->shouldEmbed( mimeTypeGroup ) )
      return KonqViewFactory();

  // Then query the trader
  QString constraint = "('Browser/View' in ServiceTypes)";

  KTrader::OfferList offers = KTrader::self()->query( serviceType, constraint );

  if ( offers.count() == 0 ) //no results?
    return KonqViewFactory();

  KService::Ptr service = offers.first();

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
      KService::PropertyPtr prop = (*it)->property( "X-KDE-BrowserView-AllowAsDefault" );
      if ( !!prop && prop->boolValue() )
      {
        service = *it;
        break;
      }
    }
  }

  KLibFactory *factory = KLibLoader::self()->factory( service->library() );

  //  if ( !factory )
  //    return 0L;

  if ( serviceOffers )
    (*serviceOffers) = offers;

  if ( serviceImpl )
    (*serviceImpl) = service;

  QStringList args;

  KService::PropertyPtr prop = service->property( "X-KDE-BrowserView-Args" );

  if ( prop )
  {
    QString argStr = prop->stringValue();
    args = QStringList::split( " ", argStr );
  }

  return KonqViewFactory( factory, args );
}

QObject* KonqFactory::create( QObject* parent, const char* name, const char* /*classname*/, const QStringList & )
{
//  if ( !parent || !parent->inherits( "Part" ) )
//    return 0L;

  KonqPart *part = new KonqPart( parent, name );
  emit objectCreated( part );

  return part;
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
    s_instance = new KInstance( "konqueror" );

  return s_instance;
}

#include "konq_factory.moc"
