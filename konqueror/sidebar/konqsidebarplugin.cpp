#include "konqsidebarplugin.h"
#include "konqsidebarplugin.moc"
#include <kdebug.h>

KonqSidebar_PluginInterface *KonqSidebarPlugin::getInterfaces(){
  KonqSidebar_PluginInterface *jw1;
  kdDebug()<<"getInterfaces()"<<endl;

  jw1=dynamic_cast<KonqSidebar_PluginInterface *> (parent());

  if (jw1==0) kdDebug()<<"Kann keine Typumwandlung durchführen"<<endl; else kdDebug()<<"Translation worked"<<endl;
  return jw1;
 
}
