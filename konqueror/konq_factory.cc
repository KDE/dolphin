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
#include "konq_misc.h"
#include "konq_run.h"
#include "browser.h"

#include <ktrader.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kstddirs.h>

KInstance *KonqFactory::s_instance = 0L;

extern "C"
{
  void *init_libkonqueror()
  {
    return new KonqFactory;
  }
};

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

BrowserView *KonqFactory::createView( const QString &serviceType,
			              QStringList &serviceTypes,
                                      Konqueror::DirectoryDisplayMode dirMode )
{
  serviceTypes.clear();

  kdebug(0,1202,"trying to create view for \"%s\"", serviceType.ascii());
/*
  //check for builtin views first
  if ( serviceType == "inode/directory" )
  {
    serviceTypes.append( serviceType );
    if ( dirMode != Konqueror::TreeView )
    {
      KonqKfmIconView *iconView = (KonqKfmIconView *)KLibLoader::self()->factory( "libkonqiconview" )->create();
      iconView->setViewMode( dirMode );
      return iconView;
    }
    else
      return (BrowserView *)KLibLoader::self()->factory( "libkonqtreeview" )->create();
  }
  else if ( serviceType == "text/html" )
  {
    serviceTypes.append( serviceType );
    return (BrowserView *)KLibLoader::self()->factory( "libkonqhtmlview" )->create();
  }
  else if ( serviceType.left( 5 ) == "text/" &&
            ( serviceType.mid( 5, 2 ) == "x-" ||
	      serviceType.mid( 5 ) == "english" ||
	      serviceType.mid( 5 ) == "plain" ) )
  {
    serviceTypes.append( serviceType );
    return (BrowserView *)KLibLoader::self()->factory( "libkonqtxtview" )->create();
  }
*/
  //now let's query the Trader for view plugins

  QString constraint = "('Browser/View' in ServiceTypes)";
  
  if ( serviceType == "inode/directory" )
  {
    if ( dirMode != Konqueror::TreeView )
      constraint += "and (Name == 'KonqKfmIconView')";
    else
      constraint += "and (Name == 'KonqTreeView')";
  }

  KTrader::OfferList offers = KTrader::self()->query( serviceType, constraint );

  if ( offers.count() == 0 ) //no results?
    return 0L;

  //activate the view plugin
  KService::Ptr service = offers.first();

  KLibFactory *factory = KLibLoader::self()->factory( service->library() );

  if ( !factory )
    return 0L;

  serviceTypes = service->serviceTypes();

  BrowserView *view = (BrowserView *)factory->create();
  
  if ( serviceType == "inode/directory" && dirMode != Konqueror::TreeView )
  {
    KonqKfmIconView *iconView = (KonqKfmIconView *)view;
    iconView->setViewMode( dirMode );
  }
  
  return view;
}

QObject* KonqFactory::create( QObject* parent, const char* name, const char* /*classname*/ )
{
//  if ( !parent || !parent->inherits( "Part" ) )
//    return 0L;

  return new KonqPart( parent, name );
}

KInstance *KonqFactory::instance()
{
  if ( !s_instance )
    s_instance = new KInstance( "konqueror" );

  return s_instance;
}

#include "konq_factory.moc"
