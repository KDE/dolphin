#include "konqsidebar_classic_wrap.h"
#include <klocale.h>
#include "konqsidebar_classic_wrap.moc"

extern "C"
{
    void* create_konqsidebar_classic_wrap(QObject *par,QWidget *widp,QString &desktopname,const char *name)
    {
        return new SidebarClassic(par,widp,desktopname,name);
    }
};

extern "C"
{
   bool add_konqsidebar_classic_wrap(QString* fn, QString*, QMap<QString,QString> *map)
   {
        map->insert("Type","Link");
	map->insert("Icon","view_sidetree");
	map->insert("Name",i18n("Classic Sidebar"));
 	map->insert("Open","false");
	map->insert("X-KDE-KonqSidebarModule","konqsidebar_classic_wrap");
	fn->setLatin1("sidebar_classic%1.desktop");
        return true;
   }
};
