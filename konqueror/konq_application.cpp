#include "konq_application.h"
#include <konq_settings.h>
#include <dbus/qdbus.h>
#include "konq_mainwindow.h"
#include "KonquerorAdaptor.h"

KonquerorApplication::KonquerorApplication()
    : KApplication(),
      closed_by_sm( false )
{
    new KonquerorAdaptor;
    const QString dbusPath = "/Konqueror";
    const QString dbusInterface = "org.kde.Konqueror";
    QDBusConnection& dbus = QDBus::sessionBus();
    dbus.connect(QString(), dbusPath, dbusInterface, "reparseConfiguration", this, SLOT(slotReparseConfiguration()));
}

void KonquerorApplication::slotReparseConfiguration()
{
    KGlobal::config()->reparseConfiguration();
    KonqFMSettings::reparseConfiguration();

    QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
    if ( mainWindows )
    {
        foreach ( KonqMainWindow* window, *mainWindows )
            window->reparseConfiguration();
    }
}

#include "konq_application.moc"
