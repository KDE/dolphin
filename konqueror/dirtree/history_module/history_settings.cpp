#include <qcstring.h>
#include <qdatastream.h>

#include <kapp.h>
#include <kconfig.h>
#include <kglobal.h>
#include <dcopclient.h>

#include "history_settings.h"

KonqHistorySettings::KonqHistorySettings( QObject *parent, const char *name )
    : QObject( parent, name ),
      DCOPObject( "KonqHistorySettings" ),
      m_activeDialog( 0L )
{
    m_fontOlderThan.setItalic( true ); // default
}

KonqHistorySettings::KonqHistorySettings()
    : QObject(),
      DCOPObject( "KonqHistorySettings" ),
      m_activeDialog( 0L )
{
    m_fontOlderThan.setItalic( true ); // default
}

KonqHistorySettings::KonqHistorySettings( const KonqHistorySettings& s )
    : QObject(),
      DCOPObject( "KonqHistorySettings" ),
      m_activeDialog( 0L )
{
    m_valueYoungerThan = s.m_valueYoungerThan;
    m_valueOlderThan = s.m_valueOlderThan;

    m_metricYoungerThan = s.m_metricYoungerThan;
    m_metricOlderThan = s.m_metricOlderThan;

    m_detailedTips = s.m_detailedTips;

    m_fontYoungerThan = s.m_fontYoungerThan;
    m_fontOlderThan = s.m_fontOlderThan;
}

KonqHistorySettings::~KonqHistorySettings()
{
}

void KonqHistorySettings::readSettings()
{
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "HistorySettings" );

    m_valueYoungerThan = config->readNumEntry("Value youngerThan", 1 );
    m_valueOlderThan = config->readNumEntry("Value olderThan", 2 );

    QString minutes = QString::fromLatin1("minutes");
    QString days = QString::fromLatin1("days");
    QString metric = config->readEntry("Metric youngerThan", days );
    m_metricYoungerThan = (metric == days) ? DAYS : MINUTES;
    metric = config->readEntry("Metric olderThan", days );
    m_metricOlderThan = (metric == days) ? DAYS : MINUTES;

    m_detailedTips = config->readBoolEntry("Detailed Tooltips", true);

    m_fontYoungerThan = config->readFontEntry( "Font youngerThan",
					       &m_fontYoungerThan );
    m_fontOlderThan   = config->readFontEntry( "Font olderThan",
					       &m_fontOlderThan );
}

void KonqHistorySettings::applySettings()
{
    // notify other konqueror instances about the new configuration
    QByteArray data;
    QDataStream stream( data, IO_WriteOnly );
    stream << *this << objId();
    kapp->dcopClient()->send( "konqueror*", "KonqHistorySettings",
			      "notifySettingsChanged(KonqHistorySettings,QCString)",
			      data );
}

void KonqHistorySettings::notifySettingsChanged( KonqHistorySettings s,
						 QCString id )
{
    KonqHistorySettings oldSettings( s );

    m_valueYoungerThan = s.m_valueYoungerThan;
    m_valueOlderThan   = s.m_valueOlderThan;

    m_metricYoungerThan = s.m_metricYoungerThan;
    m_metricOlderThan   = s.m_metricOlderThan;

    m_detailedTips       = s.m_detailedTips;

    m_fontYoungerThan = s.m_fontYoungerThan;
    m_fontOlderThan   = s.m_fontOlderThan;

    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "HistorySettings" );

    config->writeEntry("Value youngerThan", m_valueYoungerThan );
    config->writeEntry("Value olderThan", m_valueOlderThan );

    QString minutes = QString::fromLatin1("minutes");
    QString days = QString::fromLatin1("days");
    config->writeEntry("Metric youngerThan", m_metricYoungerThan == DAYS ?
		       days : minutes );
    config->writeEntry("Metric olderThan", m_metricOlderThan == DAYS ?
		       days : minutes );

    config->writeEntry("Detailed Tooltips", m_detailedTips);

    config->writeEntry("Font youngerThan", m_fontYoungerThan );
    config->writeEntry("Font olderThan", m_fontOlderThan );

    if ( id == objId() )
	config->sync();

    emit settingsChanged( &oldSettings );
}


void KonqHistorySettings::setActiveDialog( QWidget *dialog )
{
    m_activeDialog = dialog;
}

QDataStream& operator<< (QDataStream& s, const KonqHistorySettings& e)
{
    s << e.m_valueYoungerThan;
    s << e.m_valueOlderThan;

    s << (int) e.m_metricYoungerThan;
    s << (int) e.m_metricOlderThan;

    s << (int) e.m_detailedTips;

    s << e.m_fontYoungerThan;
    s << e.m_fontOlderThan;

    return s;
}
QDataStream& operator>> (QDataStream& s, KonqHistorySettings& e)
{
    int i;
    s >> e.m_valueYoungerThan;
    s >> e.m_valueOlderThan;

    s >> i; e.m_metricYoungerThan = (bool) i;
    s >> i; e.m_metricOlderThan = (bool) i;

    s >> i; e.m_detailedTips = (bool) i;

    s >> e.m_fontYoungerThan;
    s >> e.m_fontOlderThan;

    return s;
}

#include "history_settings.moc"
