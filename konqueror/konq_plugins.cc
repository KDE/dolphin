
#include "konq_plugins.h"

#include <kded_instance.h>
#include <ktrader.h>
#include <knaming.h>
#include <kdebug.h>

void KonqPlugins::installKOMPlugins( KOM::Component_ptr comp )
{
  kdebug(0, 1202, "void KonqPlugins::installKOMPlugins( KOM::Component_ptr comp )" );

  KTrader *trader = KdedInstance::self()->ktrader();
  KNaming *naming = KdedInstance::self()->knaming();
  
  KTrader::OfferList offers = trader->query( "Konqueror/KOMPlugin" );

  KTrader::OfferList::ConstIterator it = offers.begin();
  for (; it != offers.end(); ++it )
  {
    QStringList requiredInterfaces = (*it)->property( "X-KDE-KonqKOM-RequiredInterfaces" )->stringListValue();
    QStringList providedInterfaces = (*it)->property( "X-KDE-KonqKOM-ProvidedInterfaces" )->stringListValue();
    QString KNamingName = (*it)->property( "X-KDE-KonqKOM-KNamingServiceName" )->stringValue();
    
    QStringList::ConstIterator it2 = requiredInterfaces.begin();
    for (; it2 != requiredInterfaces.end(); ++it2 )
      if ( !comp->supportsInterface( (*it2).ascii() ) )
      {
        kdebug(0, 1202, "component does not support %s", (*it2).ascii() );
	return;
      }
      
    CORBA::Object_var obj = naming->resolve( KNamingName );
    if ( CORBA::is_nil( obj ) )
    {
      kdebug(0, 1202, "component is not loaded!" );
      return;
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
