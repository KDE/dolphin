/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

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

#include "main.h"
#include "configwidget.h"
#include "enginecfg.h"

#include <konq_events.h>

#include <qregexp.h>
#include <qxembed.h>
#include <qapplication.h>

#include <kurl.h>
#include <kprotocolmanager.h>
#include <klibglobal.h>

#include <iostream.h>

KLibGlobal *KonqSearcherFactory::s_global = 0L;

KonqSearcher::KonqSearcher( QObject *parent )
: QObject( parent, "KonqSearcher" )
{
  parent->installEventFilter( this );
}

KonqSearcher::~KonqSearcher()
{
}

bool KonqSearcher::eventFilter( QObject *obj, QEvent *ev )
{

  if ( KonqURLEnterEvent::test( ev ) )
  {
    QString url = ((KonqURLEnterEvent *)ev)->url();

    cerr << "filtering " << url.ascii() << endl;

    KURL kurl( url );

    // candidate?
    if ( kurl.isMalformed() || !KProtocolManager::self().protocols().contains( kurl.protocol() ) )
    {
      int pos = url.find( ':' );
      QString key = url.left( pos );

      QString query = EngineCfg::self()->query( key );
      if ( query != QString::null )
      {
        QString querypart = url.mid( pos+1 ).replace( QRegExp( " " ), "+" );
	KURL::encode( querypart );
        QString newurl = query.replace( QRegExp( "|" ), querypart );
	
	KonqURLEnterEvent e( newurl );
	QApplication::sendEvent( parent(), &e );
	
	return true;
      }
    }
  }

  return false;
}

KonqSearcherFactory::KonqSearcherFactory( QObject *parent = 0, const char *name )
: KLibFactory( parent, name )
{
  s_global = new KLibGlobal( "konq_searcher" );
}

KonqSearcherFactory::~KonqSearcherFactory()
{
}

QObject *KonqSearcherFactory::create( QObject *parent, const char *name, const char* classname )
{
  return new KonqSearcher( parent );
}

KLibGlobal *KonqSearcherFactory::global()
{
  return s_global;
}

extern "C"
{
  void *init_libkonqsearcher()
  {
    return new KonqSearcherFactory;
  }
}

/*
int main( int argc, char **argv )
{
  KOMApplication app( argc, argv, "konq_searcher" );

  KOMAutoLoader<KonqSearcherFactory> pluginLoader( "IDL:KOM/PluginFactory:1.0", "KonqSearcher" );

  ConfigWidget *w = new ConfigWidget;

  if ( !QXEmbed::processClientCmdline( w, argc, argv ) )
    delete w;

  app.exec();
  return 0;
}
*/
#include "main.moc"
