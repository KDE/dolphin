#ifndef __konq_plugins_h__
#define __konq_plugins_h__

#include <CORBA.h>

#include <qstring.h>
#include <qdict.h>
#include <qlist.h>

class KonqPlugins
{
public:
  enum ServerType { View, Part, EventFilter };

  static void init();
   
  static bool isPluginServiceType( const QString serviceType, bool *isView = 0L, bool *isPart = 0L, bool *isEventFilter = 0L );
  static CORBA::Object_ptr lookupServer( const QString serviceType, ServerType sType );
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
  static void parseService( const QString file, const QString serviceType, bool *isView, bool *isPart, bool *isEventFilter );

  struct imrActivationEntry
  {
    QString serverName;
    QString repoID;
  };
  
  static QDict< QList<imrActivationEntry> > s_dctViewServers;
  static QDict< QList<imrActivationEntry> > s_dctPartServers;
  static QDict< QList<imrActivationEntry> > s_dctEventFilterServers;
  /* Maps view names to service types (only for plugin views!!) */
  static QDict<QString> s_dctServiceTypes;
};

#endif
