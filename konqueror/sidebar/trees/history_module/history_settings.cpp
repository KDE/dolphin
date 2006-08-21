
#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>

#include "history_settings.h"

KonqSidebarHistorySettings::KonqSidebarHistorySettings( QObject *parent )
    : QObject( parent )
{
    m_fontOlderThan.setItalic( true ); // default

    new KonqSidebarHistorySettingsAdaptor( this );
    const QString dbusPath = "/KonqSidebarHistorySettings";
    const QString dbusInterface = "org.kde.Konqueror.SidebarHistorySettings";
    QDBusConnection *dbus = &QDBusConnection::sessionBus();
    dbus->registerObject( dbusPath, this );
    dbus->connect(QString(), dbusPath, dbusInterface, "notifySettingsChanged", this, SLOT(slotSettingsChanged()));
}

#if 0 // huh? copying a QObject?
KonqSidebarHistorySettings::KonqSidebarHistorySettings( const KonqSidebarHistorySettings& s )
    : QObject()
{
    m_valueYoungerThan = s.m_valueYoungerThan;
    m_valueOlderThan = s.m_valueOlderThan;

    m_metricYoungerThan = s.m_metricYoungerThan;
    m_metricOlderThan = s.m_metricOlderThan;

    m_detailedTips = s.m_detailedTips;

    m_fontYoungerThan = s.m_fontYoungerThan;
    m_fontOlderThan = s.m_fontOlderThan;
}
#endif

KonqSidebarHistorySettings::~KonqSidebarHistorySettings()
{
}

void KonqSidebarHistorySettings::readSettings(bool global)
{
    KConfig *config;
    QString oldgroup;

    if (global) {
      config = KGlobal::config();
      oldgroup= config->group();
    }
    else
      config = new KConfig("konquerorrc");

    config->setGroup("HistorySettings");
    m_valueYoungerThan = config->readEntry("Value youngerThan", 1 );
    m_valueOlderThan = config->readEntry("Value olderThan", 2 );

    QString minutes = QLatin1String("minutes");
    QString days = QLatin1String("days");
    QString metric = config->readEntry("Metric youngerThan", days );
    m_metricYoungerThan = (metric == days) ? DAYS : MINUTES;
    metric = config->readEntry("Metric olderThan", days );
    m_metricOlderThan = (metric == days) ? DAYS : MINUTES;

    m_detailedTips = config->readEntry("Detailed Tooltips", QVariant(true)).toBool();

    m_fontYoungerThan = config->readEntry( "Font youngerThan",
					       m_fontYoungerThan );
    m_fontOlderThan   = config->readEntry( "Font olderThan",
					       m_fontOlderThan );
    if (global)
      config->setGroup( oldgroup );
    else
      delete config;
}

void KonqSidebarHistorySettings::applySettings()
{
    KConfig *config = new KConfig("konquerorrc");
    config->setGroup("HistorySettings");

    config->writeEntry("Value youngerThan", m_valueYoungerThan );
    config->writeEntry("Value olderThan", m_valueOlderThan );

    QString minutes = QLatin1String("minutes");
    QString days = QLatin1String("days");
    config->writeEntry("Metric youngerThan", m_metricYoungerThan == DAYS ?
			  days : minutes );
    config->writeEntry("Metric olderThan", m_metricOlderThan == DAYS ?
 	 	 	   days : minutes );

    config->writeEntry("Detailed Tooltips", m_detailedTips);

    config->writeEntry("Font youngerThan", m_fontYoungerThan );
    config->writeEntry("Font olderThan", m_fontOlderThan );

    delete config;

    // notify konqueror instances about the new configuration
    emit notifySettingsChanged();
}

void KonqSidebarHistorySettings::slotSettingsChanged()
{
    readSettings(false);
    emit settingsChanged();
}

#include "history_settings.moc"
