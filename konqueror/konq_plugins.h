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
};

#endif
