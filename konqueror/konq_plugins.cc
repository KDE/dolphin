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
#include <knaming.h>
#include <kactivator.h>
#include <qstringlist.h>
#include <kdebug.h>

KTrader::OfferList KonqPlugins::komPluginOffers;
bool KonqPlugins::bInitialized = false;

void KonqPlugins::installKOMPlugins( KOM::Component_ptr comp )
{
  kdebug(0, 1202, "void KonqPlugins::installKOMPlugins( KOM::Component_ptr comp )" );

  if ( !bInitialized )
    reload();

  KTrader::OfferList::ConstIterator it = komPluginOffers.begin();
  KTrader::OfferList::ConstIterator end = komPluginOffers.end();
  for (; it != end; ++it )
    installPlugin( comp, *it );
}

void KonqPlugins::reload()
{
  komPluginOffers = KdedInstance::self()->ktrader()->query( "Konqueror/KOMPlugin" );

  //remove all invalid offers
  KTrader::OfferList::Iterator it = komPluginOffers.begin();
  KTrader::OfferList::Iterator end = komPluginOffers.end();
  while ( it != end )
  {
    KService::PropertyPtr requiredIfProp = (*it)->property( "X-KDE-KonqKOM-RequiredInterfaces" );
    KService::PropertyPtr providedIfProp = (*it)->property( "X-KDE-KonqKOM-ProvidedInterfaces" );
    KService::PropertyPtr useNamingProp = (*it)->property( "X-KDE-KonqKOM-UseNamingService" );
    KService::PropertyPtr namingProp = (*it)->property( "X-KDE-KonqKOM-KNamingServiceName" );
    
    //missing interface properties?
    if ( !requiredIfProp || !providedIfProp )
    {
      komPluginOffers.remove( it );
      it = komPluginOffers.begin();
      continue;
    }
    
    //use naming service but missing naming service name?
    if ( useNamingProp && useNamingProp->boolValue() && !namingProp )
    {
      komPluginOffers.remove( it );
      it = komPluginOffers.begin();
      continue;
    }
    
    ++it;
  }

  bInitialized = true;
}

void KonqPlugins::installPlugin( KOM::Component_ptr comp, KTrader::ServicePtr pluginInfo )
{
  QStringList requiredInterfaces = pluginInfo->property( "X-KDE-KonqKOM-RequiredInterfaces" )->stringListValue();
  
  QStringList::ConstIterator it = requiredInterfaces.begin();
  QStringList::ConstIterator end = requiredInterfaces.end();
  for (; it != end; ++it )
    if ( !comp->supportsInterface( (*it).ascii() ) )
    {
      kdebug(0, 1202, "component does not support %s", (*it).ascii() );
      return;
    }

  QStringList providedInterfaces = pluginInfo->property( "X-KDE-KonqKOM-ProvidedInterfaces" )->stringListValue();
  it = providedInterfaces.begin();
  end = providedInterfaces.end();
  for (; it != end; ++it )
    if ( comp->supportsPluginInterface( (*it).ascii() ) )
    {
      kdebug(0, 1202, "component already supports pluginIf %s", (*it).ascii() );
      return;
    }
    
  KService::PropertyPtr useNamingProp = pluginInfo->property( "X-KDE-KonqKOM-UseNamingService" );
  KService::PropertyPtr namingProp = pluginInfo->property( "X-KDE-KonqKOM-KNamingServiceName" );
  
  CORBA::Object_var obj;
  
  bool useNaming = true;
  if ( useNamingProp && !useNamingProp->boolValue() )
    useNaming = false;
    
  if ( useNaming )
    {
      QString KNamingName = namingProp->stringValue();
    
      obj = KdedInstance::self()->knaming()->resolve( KNamingName );
      if ( CORBA::is_nil( obj ) )
      {
        kdebug(0, 1202, "component is not loaded!" );
        return;
      }
    }      
    else
      {
        QString repoId = *( pluginInfo->repoIds().begin() );
	QString tag = pluginInfo->name();
	int tagPos = repoId.findRev( '#' );
	if ( tagPos != -1 )
	{
	  tag = repoId.mid( tagPos+1 );
	  repoId.truncate( tagPos );
	}
	
	obj = KdedInstance::self()->kactivator()->activateService( pluginInfo->name(), repoId, tag );
	
        if ( CORBA::is_nil( obj ) )
        {
          kdebug(0, 1202, "component is not loaded!" );
          return;
        }
      }

  KOM::PluginFactory_var factory = KOM::PluginFactory::_narrow( obj );    

  if ( CORBA::is_nil( factory ) )
  {
    kdebug(0, 1202, "server does not support KOM::PluginFactory!");
    return;
  }

  KOM::InterfaceSeq reqIf;
  KOM::InterfaceSeq reqPlugins;
  KOM::InterfaceSeq prov;
  CORBA::ULong i;
      
  reqIf.length( requiredInterfaces.count() );
  i = 0;
  
  it = requiredInterfaces.begin();
  end = requiredInterfaces.end();
  for (; it != end; ++it )
    reqIf[ i++ ] = CORBA::string_dup( (*it).ascii() );
	
  reqPlugins.length( 0 ); //TODO: support plugin dependencies (shouldn't be hard, I'm just too lazy to do it now :-) (Simon)

  prov.length( providedInterfaces.count() );
  i = 0;
      
  it = providedInterfaces.begin();
  end = providedInterfaces.end();
  for (; it != end; ++it )
    prov[ i++ ] = CORBA::string_dup( (*it).ascii() );
	
  comp->addPlugin( factory, reqIf, reqPlugins, prov, true );
}
