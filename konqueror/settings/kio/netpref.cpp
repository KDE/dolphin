#include <qlabel.h>
#include <qlayout.h>
#include <qspinbox.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>

#include <kio/ioslave_defaults.h>
#include <ksaveioconfig.h>
#include <dcopclient.h>
#include <klocale.h>
#include <kdialog.h>
#include <kconfig.h>

#include "netpref.h"

#define MAX_TIMEOUT_VALUE  3600

KIOPreferences::KIOPreferences( QWidget* parent,  const char* name )
               :KCModule( parent, name )
{
    QVBoxLayout* mainLayout = new QVBoxLayout( this, 2*KDialog::marginHint(),
                                               KDialog::spacingHint() );
    gb_Timeout = new QGroupBox( i18n("Timeout Values"), this, "gb_Timeout" );
    gb_Timeout->setColumnLayout(0, Qt::Vertical );
    gb_Timeout->layout()->setSpacing( 0 );
    gb_Timeout->layout()->setMargin( 0 );
    QWhatsThis::add( gb_Timeout, i18n( "Here you can set timeout values. "
				       "You might want to tweak them if "
				       "your connection is very slow." ) );

    QVBoxLayout* gb_TimeoutLayout = new QVBoxLayout( gb_Timeout->layout() );
    gb_TimeoutLayout->setAlignment( Qt::AlignTop );
    gb_TimeoutLayout->setSpacing( 6 );
    gb_TimeoutLayout->setMargin( 11 );

    QGridLayout* grid_topLevel = new QGridLayout;
    grid_topLevel->setSpacing( KDialog::spacingHint() );
    grid_topLevel->setMargin( 2*KDialog::marginHint() );

    QGridLayout* grid_firstColumn = new QGridLayout;
    grid_firstColumn->setSpacing( KDialog::spacingHint() );
    grid_firstColumn->setMargin( 0 );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Fixed, QSizePolicy::Minimum );
    grid_firstColumn->addItem( spacer, 0, 1 );

    QVBoxLayout* vlay_firstColumnLabels = new QVBoxLayout;
    vlay_firstColumnLabels->setSpacing( KDialog::spacingHint() );
    vlay_firstColumnLabels->setMargin( 0 );

    QLabel* lbl_socket = new QLabel( i18n( "Soc&ket Read:" ), gb_Timeout,
                                     "lbl_socket" );
    vlay_firstColumnLabels->addWidget( lbl_socket );
    QLabel* lbl_proxy = new QLabel( i18n( "Pro&xy Connect:" ), gb_Timeout,
                            "lbl_proxy" );
    vlay_firstColumnLabels->addWidget( lbl_proxy );

    grid_firstColumn->addLayout( vlay_firstColumnLabels, 0, 0 );

    QVBoxLayout* vlay_firstColumnSpinBox = new QVBoxLayout;
    vlay_firstColumnSpinBox->setSpacing( KDialog::spacingHint() );
    vlay_firstColumnSpinBox->setMargin( 0 );

    sb_socketRead = new QSpinBox( gb_Timeout, "sb_socketRead" );
    sb_socketRead->setSuffix( i18n( "    sec" ) );
    vlay_firstColumnSpinBox->addWidget( sb_socketRead );

    sb_proxyConnect = new QSpinBox( gb_Timeout, "sb_proxyConnect" );
    sb_proxyConnect->setSuffix( i18n( "    sec" ) );
    vlay_firstColumnSpinBox->addWidget( sb_proxyConnect );

    grid_firstColumn->addLayout( vlay_firstColumnSpinBox, 0, 2 );

    grid_topLevel->addLayout( grid_firstColumn, 0, 0 );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Preferred, QSizePolicy::Minimum );
    grid_topLevel->addItem( spacer, 0, 3 );

    QGridLayout* grid_secondColumn = new QGridLayout;
    grid_secondColumn->setSpacing( KDialog::spacingHint() );
    grid_secondColumn->setMargin( 0 );

    QVBoxLayout* vlay_secondColumnLabel = new QVBoxLayout;
    vlay_secondColumnLabel->setSpacing( KDialog::spacingHint() );
    vlay_secondColumnLabel->setMargin( 0 );
    QLabel* lbl_serverConnect = new QLabel( i18n("Server Co&nnect:"), gb_Timeout,
                                            "lbl_serverConnect" );
    vlay_secondColumnLabel->addWidget( lbl_serverConnect );
    QLabel* lbl_serverResponse = new QLabel( i18n("Server &Response:"), gb_Timeout,
                                             "lbl_serverResponse" );
    vlay_secondColumnLabel->addWidget( lbl_serverResponse );
    grid_secondColumn->addLayout( vlay_secondColumnLabel, 0, 0 );

    QVBoxLayout* vlay_secondColumnSpinBox = new QVBoxLayout;
    vlay_secondColumnSpinBox->setSpacing( KDialog::spacingHint() );
    vlay_secondColumnSpinBox->setMargin( 0 );

    sb_serverConnect = new QSpinBox( gb_Timeout, "sb_serverConnect" );
    sb_serverConnect->setSuffix( i18n( "    secs" ) );
    vlay_secondColumnSpinBox->addWidget( sb_serverConnect );

    sb_serverResponse = new QSpinBox( gb_Timeout, "sb_serverResponse" );
    sb_serverResponse->setSuffix( i18n( "    secs" ) );
    vlay_secondColumnSpinBox->addWidget( sb_serverResponse );

    grid_secondColumn->addLayout( vlay_secondColumnSpinBox, 0, 2 );

    spacer = new QSpacerItem( 16, 20, QSizePolicy::Fixed, QSizePolicy::Minimum );
    grid_secondColumn->addItem( spacer, 0, 1 );

    grid_topLevel->addLayout( grid_secondColumn, 0, 2 );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Preferred, QSizePolicy::Minimum );
    grid_topLevel->addItem( spacer, 0, 1 );
    gb_TimeoutLayout->addLayout( grid_topLevel );
    mainLayout->addWidget( gb_Timeout );

    gb_Ftp = new QGroupBox( 1, Qt::Vertical, i18n( "FTP Options" ), this, "gb_Ftp" );
    cb_ftpDisablePasv = new QCheckBox( i18n( "Disable Passive &Mode (PASV)" ), gb_Ftp );
    mainLayout->addWidget( gb_Ftp );

    mainLayout->addStretch();

    lbl_socket->setBuddy( sb_socketRead );
    lbl_proxy->setBuddy( sb_proxyConnect );
    lbl_serverConnect->setBuddy( sb_serverConnect );
    lbl_serverResponse->setBuddy( sb_serverResponse );

    load();
}

KIOPreferences::~KIOPreferences()
{
}

void KIOPreferences::timeoutChanged(int val)
{
  emit changed(true);
}

void KIOPreferences::load()
{
  sb_socketRead->setValue( KProtocolManager::readTimeout() );
  sb_serverResponse->setValue( KProtocolManager::responseTimeout() );
  sb_serverConnect->setValue( KProtocolManager::connectTimeout() );
  sb_proxyConnect->setValue( KProtocolManager::proxyConnectTimeout() );

  sb_socketRead->setMinValue( MIN_TIMEOUT_VALUE );
  sb_serverResponse->setMinValue( MIN_TIMEOUT_VALUE );
  sb_serverConnect->setMinValue( MIN_TIMEOUT_VALUE );
  sb_proxyConnect->setMinValue( MIN_TIMEOUT_VALUE );

  sb_socketRead->setMaxValue( MAX_TIMEOUT_VALUE );
  sb_serverResponse->setMaxValue( MAX_TIMEOUT_VALUE );
  sb_serverConnect->setMaxValue( MAX_TIMEOUT_VALUE );
  sb_proxyConnect->setMaxValue( MAX_TIMEOUT_VALUE );

  KConfig config( "kio_ftprc", true, false );
  cb_ftpDisablePasv->setChecked( config.readBoolEntry( "DisablePassiveMode", false ) );
}

void KIOPreferences::save()
{
  KSaveIOConfig::setReadTimeout( sb_socketRead->value() );
  KSaveIOConfig::setResponseTimeout( sb_serverResponse->value() );
  KSaveIOConfig::setConnectTimeout( sb_serverConnect->value() );
  KSaveIOConfig::setProxyConnectTimeout( sb_proxyConnect->value() );

  KConfig config( "kio_ftprc", false, false );
  config.writeEntry( "DisablePassiveMode", cb_ftpDisablePasv->isChecked() );
  config.sync();

  emit changed(true);

  // Inform running io-slaves about change...
  QByteArray data;
  QDataStream stream( data, IO_WriteOnly );
  stream << QString::null;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "*", "KIO::Scheduler",
                            "reparseSlaveConfiguration(QString)", data );
}

void KIOPreferences::defaults()
{
  sb_socketRead->setValue( DEFAULT_READ_TIMEOUT );
  sb_serverResponse->setValue( DEFAULT_RESPONSE_TIMEOUT );
  sb_serverConnect->setValue( DEFAULT_CONNECT_TIMEOUT );
  sb_proxyConnect->setValue( DEFAULT_PROXY_CONNECT_TIMEOUT );

  cb_ftpDisablePasv->setChecked( false );

  emit changed(true);
}

QString KIOPreferences::quickHelp() const
{
  return i18n("<h1>Network Preferences</h1>Here you can define"
              " the behavior of KDE programs when using Internet"
              " and network connections. If you experience timeouts"
              " and problems or sit behind a modem, you might want"
              " to adjust these values." );
}

#include "netpref.moc"
