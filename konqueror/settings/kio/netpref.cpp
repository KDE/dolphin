#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qspinbox.h>
#include <qgroupbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

#include <kio/ioslave_defaults.h>
#include <kprotocolmanager.h>
#include <dcopclient.h>
#include <klocale.h>
#include <kdialog.h>
#include <kapp.h>

#include "netpref.h"

#define MAX_TIMEOUT_VALUE  3600

KIOPreferences::KIOPreferences( QWidget* parent,  const char* name )
               :KCModule( parent, name )
{
    setCaption( i18n( "Preferences" ) );
    QVBoxLayout* KIOPreferencesLayout = new QVBoxLayout( this );
    KIOPreferencesLayout->setSpacing( 2*KDialog::spacingHint() );
    KIOPreferencesLayout->setMargin( KDialog::marginHint() );

    gb_Timeout = new QGroupBox( this, "gb_Timeout" );
    gb_Timeout->setTitle( i18n( "Timeout Values" ) );
    gb_Timeout->setColumnLayout(0, Qt::Vertical );
    gb_Timeout->layout()->setSpacing( 0 );
    gb_Timeout->layout()->setMargin( 0 );
    QWhatsThis::add( gb_Timeout, i18n( "Here you can set timeout values. "
				       "You might want to tweak them if "
				       "your connection is very slow." ) );

    QGridLayout* gb_TimeoutLayout = new QGridLayout( gb_Timeout->layout() );
    gb_TimeoutLayout->setAlignment( Qt::AlignTop );
    gb_TimeoutLayout->setSpacing( KDialog::spacingHint() );
    gb_TimeoutLayout->setMargin( 11 );

    QVBoxLayout* vbox = new QVBoxLayout;
    vbox->setSpacing( KDialog::spacingHint() );
    vbox->setMargin( 0 );

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    vbox->addItem( spacer );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    vbox->addItem( spacer );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    vbox->addItem( spacer );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    vbox->addItem( spacer );

    gb_TimeoutLayout->addLayout( vbox, 0, 3 );


    vbox = new QVBoxLayout;
    vbox->setSpacing( KDialog::spacingHint() );
    vbox->setMargin( 0 );

    QLabel* lbl_socket = new QLabel( gb_Timeout, "lbl_socket" );
    lbl_socket->setText( i18n( "Soc&ket Read:" ) );
    vbox->addWidget( lbl_socket );

    QLabel* lbl_proxy = new QLabel( gb_Timeout, "lbl_proxy" );
    lbl_proxy->setText( i18n( "Pro&xy Connect:" ) );
    vbox->addWidget( lbl_proxy );

    QLabel* lbl_serverConnect = new QLabel( gb_Timeout, "lbl_serverConnect" );
    lbl_serverConnect->setText( i18n( "Server Co&nnect:" ) );
    vbox->addWidget( lbl_serverConnect );

    QLabel* lbl_serverResponse = new QLabel( gb_Timeout, "lbl_serverResponse" );
    lbl_serverResponse->setText( i18n( "Server &Response:" ) );
    vbox->addWidget( lbl_serverResponse );

    gb_TimeoutLayout->addLayout( vbox, 0, 0 );

    vbox = new QVBoxLayout;
    vbox->setSpacing( KDialog::spacingHint() );
    vbox->setMargin( 0 );
    spacer = new QSpacerItem( 28, 20, QSizePolicy::Fixed, QSizePolicy::Minimum );
    vbox->addItem( spacer );
    spacer = new QSpacerItem( 28, 20, QSizePolicy::Fixed, QSizePolicy::Minimum );
    vbox->addItem( spacer );
    spacer = new QSpacerItem( 28, 20, QSizePolicy::Fixed, QSizePolicy::Minimum );
    vbox->addItem( spacer );
    spacer = new QSpacerItem( 28, 20, QSizePolicy::Fixed, QSizePolicy::Minimum );
    vbox->addItem( spacer );

    gb_TimeoutLayout->addLayout( vbox, 0, 1 );

    vbox = new QVBoxLayout;
    vbox->setSpacing( KDialog::spacingHint() );
    vbox->setMargin( 0 );

    sb_socketRead = new QSpinBox( gb_Timeout, "sb_socketRead" );
    sb_socketRead->setSuffix( i18n( "    sec" ) );
    vbox->addWidget( sb_socketRead );

    sb_proxyConnect = new QSpinBox( gb_Timeout, "sb_proxyConnect" );
    sb_proxyConnect->setSuffix( i18n( "    sec" ) );
    vbox->addWidget( sb_proxyConnect );

    sb_serverConnect = new QSpinBox( gb_Timeout, "sb_serverConnect" );
    sb_serverConnect->setSuffix( i18n( "    sec" ) );
    vbox->addWidget( sb_serverConnect );

    sb_serverResponse = new QSpinBox( gb_Timeout, "sb_serverResponse" );
    sb_serverResponse->setSuffix( i18n( "    sec" ) );
    vbox->addWidget( sb_serverResponse );

    gb_TimeoutLayout->addLayout( vbox, 0, 2 );
    KIOPreferencesLayout->addWidget( gb_Timeout );

    spacer = new QSpacerItem( 1, 100, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ); 
    KIOPreferencesLayout->addItem( spacer );

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
  int min_val = KProtocolManager::minimumTimeoutThreshold();
  sb_socketRead->setMinValue( min_val );
  sb_serverResponse->setMinValue( min_val );
  sb_serverConnect->setMinValue( min_val );
  sb_proxyConnect->setMinValue( min_val );
  sb_socketRead->setMaxValue( MAX_TIMEOUT_VALUE );
  sb_serverResponse->setMaxValue( MAX_TIMEOUT_VALUE );
  sb_serverConnect->setMaxValue( MAX_TIMEOUT_VALUE );
  sb_proxyConnect->setMaxValue( MAX_TIMEOUT_VALUE );
}

void KIOPreferences::save()
{
  KProtocolManager::setReadTimeout( sb_socketRead->value() );
  KProtocolManager::setResponseTimeout( sb_serverResponse->value() );
  KProtocolManager::setConnectTimeout( sb_serverConnect->value() );
  KProtocolManager::setProxyConnectTimeout( sb_proxyConnect->value() );
  emit changed(true);

  // Inform running io-slaves about change...
  QByteArray data;
  QDataStream stream( data, IO_WriteOnly );
  stream << QString::null;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "*", "KIO::Scheduler", "reparseSlaveConfiguration(QString)", data );
}

void KIOPreferences::defaults()
{
  sb_socketRead->setValue( DEFAULT_READ_TIMEOUT );
  sb_serverResponse->setValue( DEFAULT_RESPONSE_TIMEOUT );
  sb_serverConnect->setValue( DEFAULT_CONNECT_TIMEOUT );
  sb_proxyConnect->setValue( DEFAULT_PROXY_CONNECT_TIMEOUT );
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
