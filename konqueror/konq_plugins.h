#ifndef __konq_plugins_h__
#define __konq_plugins_h__

#include <CORBA.h>

#include <qstring.h>
#include <qstrlist.h>
#include <qdict.h>
#include <qlist.h>

#include <kom.h>
#include <kservices.h>

class KonqPlugins
{
public:
  static void init();
   
  static CORBA::Object_ptr lookupViewServer( const QString serviceType );
  
  static void installKOMPlugins( KOM::Component_ptr comp );
  
  static void reset();

  /**
   * Associate a view-name with a service type. When a plugin view is created, we need
   * to store its name and service here, in case we want to create another one from the
   * view name (e.g. "split")
   */
  static void associate( const QString viewName, const QString serviceType );

  /**
   * @return the service type that the view named 'viewName' handles
   */
  static QString getServiceType( QString viewName ) { return * s_dctServiceTypes[viewName]; }

private:
  static void parseService( KService *service );

  static void parseAllServices();

  static void checkServer( const QString &serverName );
  
  struct imrCreationEntry
  {
    bool active;
    QString serverExec;
    QString activationMode;
    QStrList repoIDs;
  };
    
  struct imrActivationEntry
  {
    QString serverName;
    QString repoID;
  };

  struct KOMPluginEntry
  {
    imrActivationEntry iae;
    QStrList requiredInterfaces;
    QStrList providedInterfaces;
  };

  static QDict< QList<imrActivationEntry> > s_dctViewServers;
  /* Maps view names to service types (only for plugin views!!) */
  static QDict<QString> s_dctServiceTypes;
  
  static QList< KOMPluginEntry > s_lstKOMPlugins;
  
  static QDict<imrCreationEntry> s_dctServers;
};

#endif
