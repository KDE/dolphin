
#include "konq_plugins.h"

#include <opApplication.h>
#include <opIMR.h>

#include <kservices.h>
#include <ksimpleconfig.h>

QDict< QList<KonqPlugins::imrActivationEntry> > KonqPlugins::s_dctViewServers;
QDict< QString > KonqPlugins::s_dctServiceTypes;
QList< KonqPlugins::KOMPluginEntry > KonqPlugins::s_lstKOMPlugins;
QDict< KonqPlugins::imrCreationEntry > KonqPlugins::s_dctServers;
      
void KonqPlugins::init()
{
  imr_init();
  
  s_dctViewServers.setAutoDelete( true );
  s_dctServiceTypes.setAutoDelete( true );
  
  s_lstKOMPlugins.setAutoDelete( true );
  
  s_dctServers.setAutoDelete( true );
  
  parseAllServices();
}

CORBA::Object_ptr KonqPlugins::lookupViewServer( const QString serviceType )
{
  QList<imrActivationEntry> *l = s_dctViewServers[ serviceType ];
  
  if ( !l )
    return CORBA::Object::_nil();
    
  imrActivationEntry *e = l->getFirst(); //hm.....
  
  if ( !e )
    return CORBA::Object::_nil();

  checkServer( e->serverName );
      
  return imr_activate( e->serverName, e->repoID );    
}

void KonqPlugins::installKOMPlugins( KOM::Component_ptr comp )
{
  kdebug(0, 1202, "void KonqPlugins::installKOMPlugins( KOM::Component_ptr comp )" );

  QListIterator<KOMPluginEntry> it( s_lstKOMPlugins );
  for (; it.current(); ++it )
  {
    QStrListIterator it2( it.current()->requiredInterfaces );
    for (; it2.current(); ++it2 )
      {
        kdebug(0, 1202, "checking interface %s", it2.current() );
        if ( !comp->supportsInterface( it2.current() ) )
	{
	  kdebug(0, 1202, "no, bailing out" );
	  return;
	}
      }	  
	
    kdebug(0, 1202, "YES, activating plugin");
    checkServer( it.current()->iae.serverName );
      
    CORBA::Object_var obj = imr_activate( it.current()->iae.serverName, it.current()->iae.repoID );
    assert( !CORBA::is_nil( obj ) );
    KOM::PluginFactory_var factory = KOM::PluginFactory::_narrow( obj );
    assert( !CORBA::is_nil( factory ) );
      
    KOM::InterfaceSeq reqIf;
    KOM::InterfaceSeq reqPlugins;
    KOM::InterfaceSeq prov;
    int i;
      
    reqIf.length( it.current()->requiredInterfaces.count() );
    i = 0;
  
    it2 = QStrListIterator( it.current()->requiredInterfaces );
    for (; it2.current(); ++it2 )
      reqIf[ i++ ] = CORBA::string_dup( it2.current() );
	
    reqPlugins.length( 0 ); //TODO: support plugin dependencies (shouldn't be hard, I'm just too lazy to do it now :-) (Simon)

    prov.length( it.current()->providedInterfaces.count() );
    i = 0;
      
    it2 = QStrListIterator( it.current()->providedInterfaces );
    for (; it2.current(); ++it2 )
      prov[ i++ ] = CORBA::string_dup( it2.current() );
	
    comp->addPlugin( factory, reqIf, reqPlugins, prov, true );
  }
}
 
void KonqPlugins::reset()
{
  s_dctViewServers.clear();
  s_dctServiceTypes.clear();
  s_lstKOMPlugins.clear();
  s_dctServers.clear();
  parseAllServices();
}

void KonqPlugins::associate( const QString viewName, const QString serviceType )
{
  if ( s_dctServiceTypes[ viewName ] )
    s_dctServiceTypes.remove( viewName );

  s_dctServiceTypes.insert( viewName, new QString( serviceType ) );
}

void KonqPlugins::parseService( KService *service )
{
#warning "Disabled plugin parsing !!! FIXME ! (David)"
  return;
  KSimpleConfig config("",true); // whatever ;)

  //KSimpleConfig config( service->file(), true );

  QString serverExec;
  QString serverName;
  QString activationMode;
  imrCreationEntry *creationEntry;
  
  config.setDesktopGroup();
  
  QStrList serviceTypes;
  config.readListEntry( "ServiceTypes", serviceTypes );
  if ( serviceTypes.count() == 0 )
    return;

  serverExec = config.readEntry( "ServerExec" );
  if ( serverExec.isNull() )
    return;
    
  serverName = config.readEntry( "ServerName" );
  if ( serverName.isNull() )
    return;     

  activationMode = config.readEntry( "ActivationMode" );
  if ( activationMode.isNull() )
    return;

  creationEntry = new imrCreationEntry;    
  creationEntry->repoIDs.setAutoDelete( true );
  creationEntry->active = false;
  creationEntry->serverExec = serverExec;
  creationEntry->activationMode = activationMode;
    
  QStrListIterator it( serviceTypes );
  for (; it.current(); ++it )
  {
    config.setGroup( QString("ServiceType ") + it.current() );

    kdebug(0, 1202, "parsing servicetype %s", it.current() );
        
    if ( strcmp( it.current(), "KonquerorView" ) == 0 )
    {
      QList<imrActivationEntry> *l = 0L;
    
      QString repoID = config.readEntry( "RepoID" );
      creationEntry->repoIDs.append( repoID );

      kdebug(0, 1202, "found view plugin");
            
      QStringList serviceList = service->serviceTypes();
      QStringList::Iterator it2 = serviceList.begin();
      for (; it2 != serviceList.end(); ++it2 )
      {
        if ( !( l = s_dctViewServers[ *it2 ] ) )
        {
          l = new QList<imrActivationEntry>;
	  l->setAutoDelete( true );
	  s_dctViewServers.insert( *it2, l );
        }

	kdebug(0, 1202, "registering view plugin for %s", it2->ascii() );
        imrActivationEntry *e = new imrActivationEntry;
        e->serverName = serverName;
        e->repoID = repoID;
        l->append( e );
      }	
    }
    else if ( strcmp( it.current(), "KOMPlugin" ) == 0 )
    {
      kdebug(0, 1202, "found komplugin");
    
      KOMPluginEntry *e = new KOMPluginEntry;
      e->iae.serverName = serverName;
      e->iae.repoID = config.readEntry( "RepoID" );
      creationEntry->repoIDs.append( e->iae.repoID );
      
      e->requiredInterfaces.setAutoDelete( true );
      config.readListEntry( "RequiredInterfaces", e->requiredInterfaces );

      e->providedInterfaces.setAutoDelete( true );
      config.readListEntry( "ProvidedInterfaces", e->providedInterfaces );
      
      s_lstKOMPlugins.append( e );
    }
    else
    {
      kdebug(0,1202,"unknown servicetype!!!!!!!!!!!!!!");
      return;
    }
  }
  
  s_dctServers.insert( serverName, creationEntry );
}

void KonqPlugins::parseAllServices()
{
#warning "Disabled plugin parsing !!! FIXME ! (David)"
  return;
  QListIterator<KService> it( KService::services() );
  for (; it.current(); ++it )
  {
    kdebug(0, 1202, "parsing service %s for plugins", it.current()->name().ascii() );
    parseService( it.current() );
  }    
}

void KonqPlugins::checkServer( const QString &serverName )
{
  imrCreationEntry *e = s_dctServers[ serverName ];
  kdebug( 0, 1202, "void KonqPlugins::checkServer( const QString &serverName )");
  kdebug( 0, 1202, "checking server %s", serverName.ascii() );  
  if ( !e ) return;
  kdebug( 0, 1202, "found server entry" );
  if ( !e->active )
  {
    kdebug(0, 1202, "creating %s", serverName.ascii() );
    CORBA::Object_var obj = opapp_orb->resolve_initial_references( "ImplementationRepository" );
    CORBA::ImplRepository_var imr = CORBA::ImplRepository::_narrow( obj );
    assert( !CORBA::is_nil( imr ) );
    
    imr_create( serverName, e->activationMode, e->serverExec, e->repoIDs, imr );
    e->active = true;
  }
  else kdebug( 0, 1202, "server already created!" );
}
