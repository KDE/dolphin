
#include "konq_plugins.h"

#include <opApplication.h>
#include <opIMR.h>

#include <kservices.h>
#include <ksimpleconfig.h>

QDict< QList<KonqPlugins::imrActivationEntry> > KonqPlugins::s_dctViewServers;
QDict< QList<KonqPlugins::imrActivationEntry> > KonqPlugins::s_dctPartServers;
QDict< QList<KonqPlugins::imrActivationEntry> > KonqPlugins::s_dctEventFilterServers;
QDict< QString > KonqPlugins::s_dctServiceTypes;
      

void KonqPlugins::init()
{
  imr_init();
  
  s_dctViewServers.setAutoDelete( true );
  s_dctPartServers.setAutoDelete( true );
  s_dctEventFilterServers.setAutoDelete( true );
  s_dctServiceTypes.setAutoDelete( true );
}

bool KonqPlugins::isPluginServiceType( const QString serviceType, bool *isView, bool *isPart, bool *isEventFilter )
{
#warning "KonqPlugins::isPluginServiceType totally disabled because of KIO API"
#if 1
 return false;
#else
  list<KService::Offer> lstOffers;
  bool b1, b2, b3;

  b1 = ( s_dctViewServers[ serviceType ] != 0L );
  b2 = ( s_dctPartServers[ serviceType ] != 0L );
  b3 = ( s_dctEventFilterServers[ serviceType ] != 0L );
  
  if ( isView )
    *isView = b1;
    
  if ( isPart )
    *isPart = b2;
    
  if ( isEventFilter )
    *isEventFilter = b3;

  if ( b1 || b2 || b3 )
    return true;
  
  KService::findServiceByServiceType( serviceType, lstOffers );
  
  if ( lstOffers.size() == 0 )
    return false;

  bool res = false;

  list<KService::Offer>::iterator it = lstOffers.begin();
  for (; it != lstOffers.end(); ++it )
  {
    if ( it->m_pService->file() )
    {
      parseService( it->m_pService->file(), serviceType, &b1, &b2, &b3 );

      if ( b1 )
      {
        res = true;
	if ( isView )
	  *isView = true;
      }
      
      if ( b2 )
      {
        res = true;
	if ( isPart )
	  *isPart = true;
      }
      
      if ( b3 )
      {
        res = true;
	if ( isEventFilter )
	  *isEventFilter = true;
      }
      
    }
          
  }

  return res;    
#endif
}

CORBA::Object_ptr KonqPlugins::lookupServer( const QString serviceType, ServerType sType )
{
  QList<imrActivationEntry> *l = 0L;
  
  switch ( sType )
  {
    case View: l = s_dctViewServers[ serviceType ]; break;
    case Part: l = s_dctPartServers[ serviceType ]; break;
    case EventFilter: l = s_dctEventFilterServers[ serviceType ]; break;
  }
  
  if ( !l )
    return CORBA::Object::_nil();
    
  imrActivationEntry *e = l->getFirst(); //hm.....
  
  if ( !e )
    return CORBA::Object::_nil();
    
  return imr_activate( e->serverName, e->repoID );    
}
 
void KonqPlugins::parseService( const QString file, const QString serviceType, bool *isView, bool *isPart, bool *isEventFilter )
{
  KSimpleConfig config( file, true );
  QString serverExec;
  QString serverName;
  QString activationMode;
  QStrList repoIDs;

  *isView = false;
  *isPart = false;
  *isEventFilter = false;
  
  config.setGroup( "KDE Desktop Entry" );
  
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
    
  repoIDs.setAutoDelete( true );        
    
  QStrListIterator it( serviceTypes );
  for (; it.current(); ++it )
  {
    QList<imrActivationEntry> *l = 0L;
    
    config.setGroup( QString("ServiceType ") + it.current() );
    
    if ( strcmp( it.current(), "KonquerorView" ) == 0 )
    {
      *isView = true;
      if ( !( l = s_dctViewServers[ serviceType ] ) )
      {
        l = new QList<imrActivationEntry>;
	l->setAutoDelete( true );
	s_dctViewServers.insert( serviceType, l );
      }
    }
    else if ( strcmp( it.current(), "OpenPart" ) == 0 )
    {
      *isPart = true;
      if ( !( l = s_dctPartServers[ serviceType ] ) )
      {
        l = new QList<imrActivationEntry>;
	l->setAutoDelete( true );
	s_dctPartServers.insert( serviceType, l );
      }
    }
    else if ( strcmp( it.current(), "KonquerorEventFilter" ) == 0 )
    {
      *isEventFilter = true;
      if ( !( l = s_dctEventFilterServers[ serviceType ] ) )
      {
        l = new QList<imrActivationEntry>;
	l->setAutoDelete( true );
	s_dctEventFilterServers.insert( serviceType, l );
      }
    }
    
    if ( l )
    {
      imrActivationEntry *e = new imrActivationEntry;
      e->serverName = serverName;
      e->repoID = config.readEntry( "RepoID" );
      if ( !e->repoID.isNull() )
      {
        l->append( e );
	repoIDs.append( e->repoID );
      }
    }
    else
    {
     kdebug(0,1202,"unknown servicetype!!!!!!!!!!!!!!");
      return;
    }
    
  }

  CORBA::Object_var obj = opapp_orb->resolve_initial_references( "ImplementationRepository" );
  CORBA::ImplRepository_var imr = CORBA::ImplRepository::_narrow( obj );
  assert( !CORBA::is_nil( imr ) );
  imr_create( serverName, activationMode, serverExec, repoIDs, imr );
}

void KonqPlugins::reset()
{
  s_dctViewServers.clear();
  s_dctPartServers.clear();
  s_dctEventFilterServers.clear();
  s_dctServiceTypes.clear();
}

void KonqPlugins::associate( const QString viewName, const QString serviceType )
{
  if ( s_dctServiceTypes[ viewName ] )
    s_dctServiceTypes.remove( viewName );

  s_dctServiceTypes.insert( viewName, new QString( serviceType ) );
}
