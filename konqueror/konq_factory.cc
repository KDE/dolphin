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

#include "konq_factory.h"
#include "konq_part.h"
#include "konq_iconview.h"
#include "konq_treeview.h"
#include "konq_txtview.h"
#include "konq_htmlview.h"
#include "browser.h"

#include <kded_instance.h>
#include <ktrader.h>
#include <kdebug.h>
#include <klibglobal.h>

KLibGlobal *KonqFactory::s_global = 0L;

extern "C"
{
  void *init_konqueror()
  {
    return new KonqFactory;
  }
};

KonqFactory::KonqFactory()
{
  s_global = new KLibGlobal( "konqueror" );
}

BrowserView *KonqFactory::createView( const QString &serviceType,
			              QStringList &serviceTypes,
                                      Konqueror::DirectoryDisplayMode dirMode )
{
  serviceTypes.clear();

  kdebug(0,1202,"trying to create view for \"%s\"", serviceType.ascii());

  //check for builtin views first
  if ( serviceType == "inode/directory" )
  {
    serviceTypes.append( serviceType );
    if ( dirMode != Konqueror::TreeView )
    {
      KonqKfmIconView *iconView = new KonqKfmIconView;
      iconView->setViewMode( dirMode );
      return iconView;
    }
    else
      return new KonqTreeView();
  }
  else if ( serviceType == "text/html" )
  {
    serviceTypes.append( serviceType );
    return new KonqHTMLView();
  }
  else if ( serviceType.left( 5 ) == "text/" &&
            ( serviceType.mid( 5, 2 ) == "x-" ||
	      serviceType.mid( 5 ) == "english" ||
	      serviceType.mid( 5 ) == "plain" ) )
  {
    serviceTypes.append( serviceType );
    return new KonqTxtView;
  }

  //now let's query the Trader for view plugins
  KTrader *trader = KdedInstance::self()->ktrader();
  KActivator *activator = KdedInstance::self()->kactivator();

  KTrader::OfferList offers = trader->query( serviceType, "'Browser/View' in ServiceTypes" );

  if ( offers.count() == 0 ) //no results?
    return 0L;

  //activate the view plugin
  KService::Ptr service = offers.first();

  Factory *factory = Loader::self()->factory( service->library(), service->libraryMajor(),
                                              service->libraryMinor(),
					      service->libraryDependencies() );

  if ( !factory )
    return 0L;

  serviceTypes = service->serviceTypes();

  return (BrowserView *)factory->create();
}

QObject* KonqFactory::create( QObject* parent = 0, const char* name = 0 )
{
  if ( !parent || !parent->inherits( "Part" ) )
    return 0L;
  
  return new KonqPart( (Part *)parent, name );
}

KLibGlobal *KonqFactory::global()
{
  if ( !s_global )
    (void)init_konqueror();
  
  return s_global;
}
