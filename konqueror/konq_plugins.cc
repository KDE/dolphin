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

#include "konq_plugins.h"

#include <kded_instance.h>
#include <ktrader.h>
#include <knaming.h>
#include <kactivator.h>
#include <kdebug.h>

void KonqPlugins::installKOMPlugins( KOM::Component_ptr comp )
{
  kdebug(0, 1202, "void KonqPlugins::installKOMPlugins( KOM::Component_ptr comp )" );

  KTrader *trader = KdedInstance::self()->ktrader();
  KNaming *naming = KdedInstance::self()->knaming();
  KActivator *activator = KdedInstance::self()->kactivator();
  
  KTrader::OfferList offers = trader->query( "Konqueror/KOMPlugin" );

  KTrader::OfferList::ConstIterator it = offers.begin();
  for (; it != offers.end(); ++it )
  {
    KService::PropertyPtr requiredIfProp = (*it)->property( "X-KDE-KonqKOM-RequiredInterfaces" );
    KService::PropertyPtr providedIfProp = (*it)->property( "X-KDE-KonqKOM-ProvidedInterfaces" );
    KService::PropertyPtr useNamingProp = (*it)->property( "X-KDE-KonqKOM-UseNamingService" );
    KService::PropertyPtr namingProp = (*it)->property( "X-KDE-KonqKOM-KNamingServiceName" );
    
    if ( !requiredIfProp || !providedIfProp )
      continue;
    
    bool useNaming = true;
    if ( useNamingProp && !useNamingProp->boolValue() )
      useNaming = false;
    
    QStringList requiredInterfaces = requiredIfProp->stringListValue();
    QStringList providedInterfaces = providedIfProp->stringListValue();
    CORBA::Object_var obj;
    QStringList::ConstIterator it2;
    
    if ( useNaming )
    {
      if ( !namingProp )
        continue;
	
      QString KNamingName = namingProp->stringValue();
    
      it2 = requiredInterfaces.begin();
      for (; it2 != requiredInterfaces.end(); ++it2 )
        if ( !comp->supportsInterface( (*it2).ascii() ) )
        {
          kdebug(0, 1202, "component does not support %s", (*it2).ascii() );
	  return;
        }
      
      obj = naming->resolve( KNamingName );
      if ( CORBA::is_nil( obj ) )
      {
        kdebug(0, 1202, "component is not loaded!" );
        return;
      }
    }      
    else
      {
        QString repoId = *( (*it)->repoIds().begin() );
	QString tag = (*it)->name();
	int tagPos = repoId.findRev( '#' );
	if ( tagPos != -1 )
	{
	  tag = repoId.mid( tagPos+1 );
	  repoId.truncate( tagPos );
	}
	
	obj = activator->activateService( (*it)->name(), repoId, tag );
	
        if ( CORBA::is_nil( obj ) )
        {
          kdebug(0, 1202, "component is not loaded!" );
          return;
        }
      }

    KOM::PluginFactory_var factory = KOM::PluginFactory::_narrow( obj );    
    
    KOM::InterfaceSeq reqIf;
    KOM::InterfaceSeq reqPlugins;
    KOM::InterfaceSeq prov;
    int i;
      
    reqIf.length( requiredInterfaces.count() );
    i = 0;
  
    it2 = requiredInterfaces.begin();
    for (; it2 != requiredInterfaces.end(); ++it2 )
      reqIf[ i++ ] = CORBA::string_dup( (*it2).ascii() );
	
    reqPlugins.length( 0 ); //TODO: support plugin dependencies (shouldn't be hard, I'm just too lazy to do it now :-) (Simon)

    prov.length( providedInterfaces.count() );
    i = 0;
      
    it2 = providedInterfaces.begin();
    for (; it2 != providedInterfaces.end(); ++it2 )
      prov[ i++ ] = CORBA::string_dup( (*it2).ascii() );
	
    comp->addPlugin( factory, reqIf, reqPlugins, prov, true );
  }
}
