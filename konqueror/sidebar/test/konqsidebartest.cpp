#include "konqsidebartest.h"
#include "konqsidebartest.moc"

extern "C"
{
    void* create_konqsidebartest(QObject *par,QWidget *widp,QString &desktopname,const char *name)
    {
        return new SidebarTest(par,widp,desktopname,name);
    }
};
