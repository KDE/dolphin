#include "konqsidebar_classic_wrap.h"
#include "konqsidebar_classic_wrap.moc"

extern "C"
{
    void* create_konqsidebar_classic_wrap(QObject *par,QWidget *widp,QString &desktopname,const char *name)
    {
        return new SidebarClassic(par,widp,desktopname,name);
    }
};
