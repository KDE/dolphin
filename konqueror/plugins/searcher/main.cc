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

#include <qregexp.h>
#include <qxembed.h>

#include <komShutdownManager.h>
#include <komAutoLoader.h>

#include <opUIUtils.h>

KonqSearcher::KonqSearcher( KOM::Component_ptr core )
: KOMPlugin( core )
{
}

void KonqSearcher::cleanUp()
{
  if ( m_bIsClean )
    return;

  //TODO
    
  KOMPlugin::cleanUp();
}

CORBA::Boolean KonqSearcher::eventFilter( KOM::Base_ptr obj, const char *name, const CORBA::Any &value )
{
  if ( strcmp( name, "Konqueror/GUI/URLEntered" ) == 0 )
  {
    cerr << "filtering " << name << endl;
    CORBA::WChar *url;
    value >>= CORBA::Any::to_wstring( url, 0 );
    QString qurl = C2Q( url );
    if ( qurl.left(3) == "av:" && qurl.length() > 3 )
    {
      cerr << "skipping!" << endl;
      
      QString qnewurl = "http://www.altavista.com/cgi-bin/query?q=" + qurl.mid(3).replace( QRegExp( " " ), "+" );
      
      CORBA::WString_var wnewurl = Q2C( qnewurl );
      EMIT_EVENT_WSTRING( obj, "Konqueror/GUI/URLEntered", wnewurl );
      
      return (CORBA::Boolean)true;
    }
  }
  return (CORBA::Boolean)false;
}

KonqSearcherFactory::KonqSearcherFactory( const CORBA::BOA::ReferenceData &refData )
: KOM::PluginFactory_skel( refData )
{
}

KonqSearcherFactory::KonqSearcherFactory( CORBA::Object_ptr obj )
: KOM::PluginFactory_skel( obj )
{
}

KOM::Plugin_ptr KonqSearcherFactory::create( KOM::Component_ptr core )
{
  KonqSearcher *plugin = new KonqSearcher( core );
  KOMShutdownManager::self()->watchObject( plugin );
  return KOM::Plugin::_duplicate( plugin );
}

int main( int argc, char **argv )
{
  KOMApplication app( argc, argv );

  KOMAutoLoader<KonqSearcherFactory> pluginLoader( "IDL:KOM/PluginFactory:1.0", "KonqSearcher" );

  ConfigWidget *w = new ConfigWidget;
  
  if ( !QXEmbed::processClientCmdline( w, argc, argv ) )
    delete w;

  app.exec();
  return 0;
}
