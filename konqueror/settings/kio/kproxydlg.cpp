/*
   kproxydlg.cpp - Proxy configuration dialog

   Copyright (C) 2001- Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License (GPL) version 2 as published by the Free Software
   Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qregexp.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>

#include <klocale.h>
#include <klistview.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kprotocolmanager.h>

#include "ksaveioconfig.h"
#include "kenvvarproxydlg.h"
#include "kmanualproxydlg.h"

#include "kproxydlg.h"


KProxyDialog::KProxyDialog( QWidget* parent,  const char* name )
             :KCModule( parent, name )
{
    QVBoxLayout* mainLayout = new QVBoxLayout( this,
                                               KDialog::marginHint(),
                                               KDialog::spacingHint() );
    QHBoxLayout* hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    m_cbUseProxy = new QCheckBox( i18n("Use &proxy"), this, "m_cbUseProxy" );
    m_cbUseProxy->setSizePolicy( QSizePolicy( QSizePolicy::Fixed,
                                QSizePolicy::Fixed,
                                m_cbUseProxy->sizePolicy().hasHeightForWidth()));
    QWhatsThis::add( m_cbUseProxy, i18n("<qt>Check this box to enable the use "
                                       "of proxy servers for your internet "
                                       "connection.<p>Please note that using "
                                       "proxy servers is optional, but has the "
                                       "benefit or advantage of giving you "
                                       "faster access to data on the internet."
                                       "<p>If you are uncertain whether or not "
                                       "you need to use a proxy server to "
                                       "connect to the internet, please consult "
                                       "with your internet service provider's "
                                       "setup guide or your system administrator."
                                       "</qt>") );
    hlay->addWidget( m_cbUseProxy );
    QSpacerItem* spacer = new QSpacerItem( 20, 20,
                                           QSizePolicy::MinimumExpanding,
                                           QSizePolicy::Minimum );
    hlay->addItem( spacer );
    mainLayout->addLayout( hlay );

    m_gbConfigure = new QButtonGroup( i18n("Configuration"), this,
                                     "m_gbConfigure" );
    m_gbConfigure->setEnabled( false );
    QWhatsThis::add( m_gbConfigure, i18n("Options for setting up connections "
                                        "to your proxy server(s)") );
    m_gbConfigure->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                             QSizePolicy::Fixed,
                                             m_gbConfigure->sizePolicy().hasHeightForWidth()) );

    m_gbConfigure->setColumnLayout(0, Qt::Vertical );
    m_gbConfigure->layout()->setSpacing( 0 );
    m_gbConfigure->layout()->setMargin( 0 );
    QVBoxLayout* vlay = new QVBoxLayout( m_gbConfigure->layout() );
    vlay->setMargin( 2*KDialog::marginHint() );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setAlignment( Qt::AlignTop );

    hlay = new QHBoxLayout;
    hlay->setSpacing(KDialog::spacingHint());
    hlay->setMargin(0);

    m_rbAutoDiscover = new QRadioButton( i18n("A&utomatically detected script "
                                        "file"), m_gbConfigure,
                                        "m_rbAutoDiscover" );
    m_rbAutoDiscover->setChecked( true );
    QWhatsThis::add( m_rbAutoDiscover, i18n("<qt> Select this option if you want "
                                           "the proxy setup configuration script "
                                           "file to be automatically detected and "
                                           "downloaded.<p>This option only differs "
                                           "from the next choice in that it "
                                           "<i>does not</i> require you to suplly "
                                           "the location of the configuration script "
                                           "file. Instead it will be automatically "
                                           "downloaded using the <b>Web Access "
                                           "Protocol Discovery (WAPD)</b>.<p>"
                                           "<u>NOTE:</u>  If you have problem on "
                                           "using this setup, please consult the FAQ "
                                           "section at http://www.konqueror.org for "
                                           "more information.</qt>") );
    hlay->addWidget( m_rbAutoDiscover );
    spacer = new QSpacerItem( 210, 16, QSizePolicy::Fixed,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    vlay->addLayout( hlay );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    m_rbAutoScript = new QRadioButton( i18n("Specified &script file"),
                                      m_gbConfigure, "m_rbAutoScript" );
    QWhatsThis::add( m_rbAutoScript, i18n("Select this option if your proxy "
                                         "support is provided through a script "
                                         "file located at a specific address. "
                                         "You can then enter the address of "
                                         "the location below.") );
    hlay->addWidget( m_rbAutoScript );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    vlay->addLayout( hlay );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Fixed,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );

    QLabel* label = new QLabel( i18n("&Location:") , m_gbConfigure,
                                "lbl_location" );
    label->setEnabled( false );
    connect( m_rbAutoScript, SIGNAL( toggled(bool) ), label,
             SLOT( setEnabled(bool) ) );
    label->setEnabled( false );
    hlay->addWidget( label );

    m_location = new KURLRequester( m_gbConfigure, "m_location" );
    m_location->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                            QSizePolicy::Fixed,
                                            m_location->sizePolicy().hasHeightForWidth()) );
    m_location->setEnabled( false );
    m_location->setFocusPolicy( KURLRequester::StrongFocus );
    label->setBuddy( m_location );
    hlay->addWidget( m_location );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed );
    hlay->addItem( spacer );
    vlay->addLayout( hlay );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    m_rbEnvVar = new QRadioButton( i18n("Preset environment &variables"),
                                  m_gbConfigure, "m_rbEnvVar" );
    QWhatsThis::add( m_rbEnvVar, i18n("<qt>Some systems are setup with "
                                     "environment variables such as "
                                     "<b>$HTTP_PROXY</b> to allow graphical "
                                     "as well as non-graphical application "
                                     "to share the same proxy configuration "
                                     "information.<p>Select this option and "
                                     "click on the <i>Setup...</i> button on "
                                     "the right side to provide the environment "
                                     "variable names used to set the address "
                                     "of the proxy server(s).</qt>") );
    hlay->addWidget( m_rbEnvVar );
    spacer = new QSpacerItem( 45, 20, QSizePolicy::Fixed,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );

    m_pbEnvSetup = new QPushButton( i18n("Setup..."), m_gbConfigure,
                                   "m_pbEnvSetup" );
    m_pbEnvSetup->setSizePolicy( QSizePolicy(QSizePolicy::Fixed,
                                             QSizePolicy::Fixed,
                                             m_pbEnvSetup->sizePolicy().hasHeightForWidth()) );
    hlay->addWidget( m_pbEnvSetup );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    vlay->addLayout( hlay );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    m_rbManual = new QRadioButton( i18n("&Manually specified settings"),
                                  m_gbConfigure, "m_rbManual" );
    QWhatsThis::add( m_rbManual, i18n("<qt>Select this option and click on "
                                     "the <i>Setup...</i> button on the "
                                     "right side to manually setup the "
                                     "location of the proxy servers to be "
                                     "used.</qt>") );
    hlay->addWidget( m_rbManual );
    spacer = new QSpacerItem( 40, 20, QSizePolicy::Fixed,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );

    m_pbManSetup = new QPushButton( i18n("Setup..."), m_gbConfigure,
                                   "m_pbManSetup" );
    m_pbManSetup->setSizePolicy( QSizePolicy(QSizePolicy::Fixed,
                                             QSizePolicy::Fixed,
                                             m_pbManSetup->sizePolicy().hasHeightForWidth()) );
    hlay->addWidget( m_pbManSetup );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    vlay->addLayout( hlay );
    mainLayout->addWidget( m_gbConfigure );

    m_gbAuth = new QButtonGroup( i18n("Authorization"), this, "m_gbAuth" );
    m_gbAuth->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                        QSizePolicy::Fixed,
                                        m_gbAuth->sizePolicy().hasHeightForWidth()) );

    m_gbAuth->setEnabled( false );
    QWhatsThis::add( m_gbAuth, i18n("Setup the way that authorization information "
                                   "should be handled when making proxy "
                                   "connections. The default option is to "
                                   "prompt you for password as needed.") );
    m_gbAuth->setColumnLayout(0, Qt::Vertical );
    m_gbAuth->layout()->setSpacing( 0 );
    m_gbAuth->layout()->setMargin( 0 );

    QVBoxLayout* m_gbAuthLayout = new QVBoxLayout( m_gbAuth->layout() );
    m_gbAuthLayout->setMargin( KDialog::marginHint() );
    m_gbAuthLayout->setSpacing( KDialog::spacingHint() );
    m_gbAuthLayout->setAlignment( Qt::AlignTop );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    m_rbPrompt = new QRadioButton( i18n("Prompt as &needed"), m_gbAuth,
                                   "m_rbPrompt" );
    m_rbPrompt->setChecked( true );
    QWhatsThis::add( m_rbPrompt, i18n("Select this option if you want to be "
                                      "prompted for the login information "
                                      "as needed. This is default behavior.") );
    hlay->addWidget( m_rbPrompt );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                              QSizePolicy::Fixed );
    hlay->addItem( spacer );
    m_gbAuthLayout->addLayout( hlay );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    m_rbAutoLogin = new QRadioButton( i18n("Use automatic lo&gin"), m_gbAuth,
                                      "m_rbAutoLogin" );
    QWhatsThis::add( m_rbAutoLogin, i18n("Select this option if you have "
                                        "already setup a login entry for "
                                        "your proxy server(s) in <tt>"
                                        "$KDEHOME/share/config/kionetrc"
                                        "</tt> file.") );
    hlay->addWidget( m_rbAutoLogin );
    m_gbAuthLayout->addLayout( hlay );
    mainLayout->addWidget( m_gbAuth );

    spacer = new QSpacerItem( 1, 1, QSizePolicy::Expanding,
                              QSizePolicy::Minimum );
    mainLayout->addItem( spacer );

    // signals and slots connections
    connect( m_cbUseProxy, SIGNAL( toggled(bool) ),
             SLOT( useProxyChecked(bool) ) );

    connect( m_rbAutoDiscover, SIGNAL( toggled(bool) ),
             SLOT( autoDiscoverChecked() ) );
    connect( m_rbAutoScript, SIGNAL( toggled(bool) ),
             SLOT( autoScriptChecked( bool ) ) );

    connect( m_rbPrompt, SIGNAL( toggled(bool) ),
             SLOT( promptChecked() ) );
    connect( m_rbAutoLogin, SIGNAL( toggled(bool) ),
             SLOT( autoChecked() ) );

    connect( m_location, SIGNAL( textChanged(const QString&) ),
             SLOT( autoScriptChanged(const QString&) ) );

    connect( m_pbEnvSetup, SIGNAL( clicked() ), SLOT( setupEnvProxy() ) );
    connect( m_pbManSetup, SIGNAL( clicked() ), SLOT( setupManProxy() ) );

    load();
}

KProxyDialog::~KProxyDialog()
{
  delete m_data;
  m_data = 0;
}

void KProxyDialog::load()
{
  bool useProxy;
  KProtocolManager proto;

  m_data = new KProxyData;

  useProxy = proto.useProxy();
  m_data->type = proto.proxyType();
  m_data->httpProxy = proto.proxyFor( "http" );
  m_data->httpsProxy = proto.proxyFor( "https" );
  m_data->ftpProxy = proto.proxyFor( "ftp" );
  m_data->scriptProxy = proto.proxyConfigScript();
  m_data->useReverseProxy = proto.useReverseProxy();
  m_data->noProxyFor = QStringList::split( QRegExp("[',''\t'' ']"),
                                      proto.noProxyFor() );

  m_cbUseProxy->setChecked( useProxy );
  m_gbConfigure->setEnabled( useProxy );
  m_gbAuth->setEnabled( useProxy );

  if ( !m_data->scriptProxy.isEmpty() )
    m_location->lineEdit()->setText( m_data->scriptProxy );

  switch ( m_data->type )
  {
    case KProtocolManager::WPADProxy:
      m_rbAutoDiscover->setChecked( true );
      break;
    case KProtocolManager::PACProxy:
      m_rbAutoScript->setChecked( true );
      break;
    case KProtocolManager::ManualProxy:
      m_rbManual->setChecked( true );
      break;
    case KProtocolManager::EnvVarProxy:
      m_rbEnvVar->setChecked( true );
      break;
    default:
      break;
  }

  switch( proto.proxyAuthMode() )
  {
    case KProtocolManager::Prompt:
      m_rbPrompt->setChecked( true );
      break;
    case KProtocolManager::Automatic:
      m_rbAutoLogin->setChecked( true );
    default:
      break;
  }
}

void KProxyDialog::save()
{
  if ( m_cbUseProxy->isChecked() )
  {
    if ( m_rbAutoDiscover->isChecked() )
    {
      KSaveIOConfig::setProxyType( KProtocolManager::WPADProxy );
    }
    else if ( m_rbAutoScript->isChecked() )
    {
      KURL u;

      u = m_location->lineEdit()->text();

      if ( !u.isValid() )
      {
        QString msg = i18n("<qt>The address of the automatic proxy "
                           "configuration script is invalid! Please "
                           "correct this problem before proceeding. "
                           "Otherwise the changes you made will be "
                           "ignored!</qt>");
        KMessageBox::error( this, msg, i18n("Invalid Proxy Setup") );
        return;
      }
      else
      {
        KSaveIOConfig::setProxyType( KProtocolManager::PACProxy );
        m_data->scriptProxy = u.url();
      }
    }
    else if ( m_rbManual->isChecked() )
    {
      if ( m_data->type != KProtocolManager::ManualProxy )
      {
        QString msg = i18n("<qt>Proxy information was not setup "
                           "properly! Please click on the <em>"
                           "Setup...</em> button to correct this "
                           "problem before proceeding! Otherwise "
                           "the changes you made will be ignored!"
                           "</qt>");
        KMessageBox::error( this, msg, i18n("Invalid Proxy Setup") );
        return;
      }

      KSaveIOConfig::setProxyType( KProtocolManager::ManualProxy );
    }
    else if ( m_rbEnvVar->isChecked() )
    {
      if ( m_data->type != KProtocolManager::EnvVarProxy )
      {
        QString msg = i18n("<qt>Proxy information was not setup "
                           "properly! Please click on the <em>"
                           "Setup...</em> button to correct this "
                           "problem before proceeding! Otherwise "
                           "the changes you made will be ignored!"
                           "</qt>");
        KMessageBox::error( this, msg, i18n("Invalid Proxy Setup") );
        return;
      }

      KSaveIOConfig::setProxyType( KProtocolManager::EnvVarProxy );
    }

    if ( m_rbPrompt->isChecked() )
      KSaveIOConfig::setProxyAuthMode( KProtocolManager::Prompt );
    else if ( m_rbAutoLogin->isChecked() )
      KSaveIOConfig::setProxyAuthMode( KProtocolManager::Automatic );
  }
  else
  {
    KSaveIOConfig::setProxyType( KProtocolManager::NoProxy );
  }

  // Save the common proxy setting...
  KSaveIOConfig::setProxyFor( "ftp", m_data->ftpProxy );
  KSaveIOConfig::setProxyFor( "http", m_data->httpProxy );
  KSaveIOConfig::setProxyFor( "https", m_data->httpsProxy );

  KSaveIOConfig::setProxyConfigScript( m_data->scriptProxy );
  KSaveIOConfig::setUseReverseProxy( m_data->useReverseProxy );
  KSaveIOConfig::setNoProxyFor( m_data->noProxyFor.join(",") );


  KSaveIOConfig::updateRunningIOSlaves (this);
     
  emit changed( false );
}

void KProxyDialog::defaults()
{
  m_data->reset();
  m_cbUseProxy->setChecked( false );
  emit changed( true );
}

QString KProxyDialog::quickHelp() const
{
  return i18n( "This module lets you configure your proxy and cache "
               "settings. A proxy is a program on another computer "
               "that receives requests from your machine to access "
               "a certain web page (or other Internet resources), "
               "retrieves the page and sends it back to you." );
}

void KProxyDialog::setupManProxy()
{
  m_rbManual->setChecked(true);

  KManualProxyDlg* dlg = new KManualProxyDlg( this );

  dlg->setProxyData( *m_data );

  if ( dlg->exec() == QDialog::Accepted )
  {
    *m_data = dlg->data();
    emit changed( true );
  }

  delete dlg;
}

void KProxyDialog::setupEnvProxy()
{
  m_rbEnvVar->setChecked(true);

  KEnvVarProxyDlg* dlg = new KEnvVarProxyDlg( this );

  dlg->setProxyData( *m_data );

  if ( dlg->exec() == QDialog::Accepted )
  {    
    *m_data = dlg->data();
    emit changed( true );
  }

  delete dlg;
}

void KProxyDialog::autoDiscoverChecked()
{
  emit changed( true );
}

void KProxyDialog::autoScriptChecked( bool on )
{
  emit changed( true );
  m_location->setEnabled( on );
}

void KProxyDialog::manualChecked()
{
  emit changed( true );
}

void KProxyDialog::envVarChecked()
{
  emit changed( true );
}

void KProxyDialog::promptChecked()
{
  emit changed( true );
}

void KProxyDialog::autoChecked()
{
  emit changed( true );
}

void KProxyDialog::useProxyChecked( bool on )
{
  m_gbConfigure->setEnabled( on );
  m_gbAuth->setEnabled( on );

  emit changed( true );
}

void KProxyDialog::autoScriptChanged( const QString& )
{
  emit changed( true );
}

#include "kproxydlg.moc"
