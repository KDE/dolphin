#include <qgrid.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qspinbox.h>
#include <qvgroupbox.h>
#include <qtooltip.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qtooltip.h>
#include <qgrid.h>
#include <qvgroupbox.h>

#include <kio/ioslave_defaults.h>
#include <ksaveioconfig.h>
#include <dcopclient.h>
#include <knuminput.h>
#include <klocale.h>
#include <kdialog.h>
#include <kconfig.h>

#include "netpref.h"

#define MAX_TIMEOUT_VALUE  3600

KIOPreferences::KIOPreferences( QWidget* parent,  const char* name )
               :KCModule( parent, name )
{
    QVBoxLayout* mainLayout = new QVBoxLayout( this, KDialog::marginHint(),
                                               KDialog::spacingHint() );
    gb_Timeout = new QVGroupBox( i18n("Timeout Values"), this, "gb_Timeout" );
    QWhatsThis::add( gb_Timeout, i18n( "Here you can set timeout values. "
				       "You might want to tweak them if "
				       "your connection is very slow." ) );


    QGrid *grid = new QGrid(4, Qt::Vertical, gb_Timeout);
    grid->setSpacing(KDialog::spacingHint());

    QLabel* lbl_socket = new QLabel( i18n( "Soc&ket read:" ), grid,
                                     "lbl_socket" );
    QLabel* lbl_proxy = new QLabel( i18n( "Pro&xy connect:" ), grid,
                            "lbl_proxy" );

    QLabel* lbl_serverConnect = new QLabel( i18n("Server co&nnect:"), grid,
                                            "lbl_serverConnect" );
    QLabel* lbl_serverResponse = new QLabel( i18n("Server &response:"), grid,
                                             "lbl_serverResponse" );

    sb_socketRead = new KIntNumInput( grid, "sb_socketRead" );
    sb_socketRead->setSuffix( i18n( " sec" ) );
    connect(sb_socketRead, SIGNAL(valueChanged ( int )),this, SLOT(timeoutChanged(int)));

    sb_proxyConnect = new KIntNumInput( grid, "sb_proxyConnect" );
    sb_proxyConnect->setSuffix( i18n( " sec" ) );
    connect(sb_proxyConnect, SIGNAL(valueChanged ( int )),this, SLOT(timeoutChanged(int)));

    sb_serverConnect = new KIntNumInput( grid, "sb_serverConnect" );
    sb_serverConnect->setSuffix( i18n( " sec" ) );
    connect(sb_serverConnect, SIGNAL(valueChanged ( int )),this, SLOT(timeoutChanged(int)));

    sb_serverResponse = new KIntNumInput( grid, "sb_serverResponse" );
    sb_serverResponse->setSuffix( i18n( " sec" ) );
    connect(sb_serverResponse, SIGNAL(valueChanged ( int )),this, SLOT(timeoutChanged(int)));

	 QWidget *spacer = new QWidget(grid);
    spacer->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));


    mainLayout->addWidget( gb_Timeout );



    gb_Ftp = new QVGroupBox( i18n( "FTP Options" ), this, "gb_Ftp" );
    cb_ftpEnablePasv = new QCheckBox( i18n( "Enable passive &mode (PASV)" ), gb_Ftp );
    QWhatsThis::add(cb_ftpEnablePasv, i18n( "Enables FTP's \"passive\" mode. This is required to allow FTP to work from behind firewalls." ));
    cb_ftpMarkPartial = new QCheckBox( i18n( "Mark &partially uploaded files" ), gb_Ftp );
    QWhatsThis::add(cb_ftpMarkPartial, i18n( "<p>Marks partially uploaded FTP files.</p>"
                                             "<p>When this option is enabled, partially uploaded files "
                                             "will have a \".part\" extention. This extension will be removed "
                                             "once the transfer is complete.</p>"));

    mainLayout->addWidget( gb_Ftp );

    connect(cb_ftpEnablePasv, SIGNAL(toggled ( bool  )),this,SLOT(configChanged()));
    connect(cb_ftpMarkPartial, SIGNAL(toggled ( bool  )),this,SLOT(configChanged()));

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

void KIOPreferences::load()
{
  KProtocolManager proto;

  sb_socketRead->setRange( MIN_TIMEOUT_VALUE, MAX_TIMEOUT_VALUE );
  sb_serverResponse->setRange( MIN_TIMEOUT_VALUE, MAX_TIMEOUT_VALUE );
  sb_serverConnect->setRange( MIN_TIMEOUT_VALUE, MAX_TIMEOUT_VALUE );
  sb_proxyConnect->setRange( MIN_TIMEOUT_VALUE, MAX_TIMEOUT_VALUE );

  sb_socketRead->setValue( proto.readTimeout() );
  sb_serverResponse->setValue( proto.responseTimeout() );
  sb_serverConnect->setValue( proto.connectTimeout() );
  sb_proxyConnect->setValue( proto.proxyConnectTimeout() );

  KConfig config( "kio_ftprc", true, false );
  cb_ftpEnablePasv->setChecked( !config.readBoolEntry( "DisablePassiveMode", false ) );
  cb_ftpMarkPartial->setChecked( config.readBoolEntry( "MarkPartial", true ) );
}

void KIOPreferences::save()
{
  KSaveIOConfig::setReadTimeout( sb_socketRead->value() );
  KSaveIOConfig::setResponseTimeout( sb_serverResponse->value() );
  KSaveIOConfig::setConnectTimeout( sb_serverConnect->value() );
  KSaveIOConfig::setProxyConnectTimeout( sb_proxyConnect->value() );

  KConfig config( "kio_ftprc", false, false );
  config.writeEntry( "DisablePassiveMode", !cb_ftpEnablePasv->isChecked() );
  config.writeEntry( "MarkPartial", cb_ftpMarkPartial->isChecked() );
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

  cb_ftpEnablePasv->setChecked( true );
  cb_ftpMarkPartial->setChecked( true );

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
