
#include "konq_plugins.h"

#include <kded_instance.h>
#include <ktrader.h>
#include <kactivator.h>
#include <kdebug.h>

QMap<QString,QString> KonqPlugins::s_mapServiceTypes;

CORBA::Object_ptr KonqPlugins::lookupViewServer( const QString serviceType )
{
  KTrader *trader = KdedInstance::self()->ktrader();
  KActivator *activator = KdedInstance::self()->kactivator();
  
  KTrader::OfferList offers = trader->query( serviceType, "'Konqueror/View' in ServiceTypes" );

  if ( offers.count() == 0 )
    return CORBA::Object::_nil();
    
  KTrader::ServicePtr service = offers.getFirst();
  
  QString repoId = service->repoIds().getFirst();
  
  if ( repoId.isNull() )
    return CORBA::Object::_nil();
  
  QString tag = service->name();
  int tagPos = repoId.findRev( "#" );
  if ( tagPos != -1 )
  {
    tag = repoId.mid( tagPos+1 );
    repoId.truncate( tagPos );
  }

  return activator->activateService( service->name(), repoId, tag );
}

void KonqPlugins::installKOMPlugins( KOM::Component_ptr comp )
{
  kdebug(0, 1202, "void KonqPlugins::installKOMPlugins( KOM::Component_ptr comp )" );

  KTrader *trader = KdedInstance::self()->ktrader();
  KActivator *activator = KdedInstance::self()->kactivator();
  
  KTrader::OfferList offers = trader->query( "KOMPlugin" );

  KTrader::OfferList::ConstIterator it = offers.begin();
  for (; it != offers.end(); ++it )
  {
    QStringList requiredInterfaces = (*it)->property( "RequiredInterfaces" )->stringListValue();
    QStringList providedInterfaces = (*it)->property( "ProvidedInterfaces" )->stringListValue();
    
    QStringList::ConstIterator it2 = requiredInterfaces.begin();
    for (; it2 != requiredInterfaces.end(); ++it2 )
      if ( !comp->supportsInterface( (*it2).ascii() ) )
      {
        kdebug(0, 1202, "component does not support %s", (*it2).ascii() );
	return;
      }
      
    QString repoId = (*it)->repoIds().getFirst();
    QString tag = (*it)->name();
    int tagPos = repoId.findRev( "#" );
    if ( tagPos != -1 )
    {
      tag = repoId.mid( tagPos+1 );
      repoId.truncate( tagPos );
    }
    
    CORBA::Object_var obj = activator->activateService( (*it)->name(), repoId, tag );

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
 
void KonqPlugins::associate( const QString &viewName, const QString &serviceType )
{
  QMap<QString,QString>::Iterator it = s_mapServiceTypes.find( viewName );
  
  if ( it != s_mapServiceTypes.end() )
    s_mapServiceTypes.remove( it );
  
  s_mapServiceTypes.insert( viewName, serviceType );
}

QString KonqPlugins::getServiceType( const QString &viewName )
{
  QMap<QString,QString>::Iterator it = s_mapServiceTypes.find( viewName );
  
  if ( it != s_mapServiceTypes.end() )
    return (*it).data();
  else
    return QString::null;
}