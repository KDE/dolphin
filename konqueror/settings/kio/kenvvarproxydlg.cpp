/*
   kenvvarproxydlg.cpp - Proxy configuration dialog

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

#include <stdlib.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

#include <kdebug.h>
#include <klocale.h>
#include <klineedit.h>
#include <kmessagebox.h>

#include "kproxyexceptiondlg.h"
#include "kenvvarproxydlg.h"


#define ENV_FTP_PROXY     "FTP_PROXY,ftp_proxy,FTPPROXY,ftpproxy,PROXY,proxy"
#define ENV_HTTP_PROXY    "HTTP_PROXY,http_proxy,HTTPPROXY,httpproxy,PROXY,proxy"
#define ENV_HTTPS_PROXY   "HTTPS_PROXY,https_proxy,HTTPSPROXY,httpsproxy,PROXY,proxy"
#define NO_PROXY          "NO_PROXY, no_proxy"


KEnvVarProxyDlg::KEnvVarProxyDlg( QWidget* parent, const char* name )
                :KProxyDialogBase( parent, name, true,
                                   i18n( "Variable Proxy Configuration" ) )
{
  QWidget *page = new QWidget( this );
  setMainWidget (page);

  QVBoxLayout* mainLayout = new QVBoxLayout( page );
  mainLayout->setSpacing( KDialog::spacingHint() );

  m_gbHostnames = new QGroupBox( page, "m_gbHostnames" );

  QSizePolicy policy (QSizePolicy::Expanding, QSizePolicy::Fixed,
                      m_gbHostnames->sizePolicy().hasHeightForWidth());
  m_gbHostnames->setSizePolicy( policy );
  m_gbHostnames->setTitle( i18n( "Variables" ) );
  m_gbHostnames->setColumnLayout(0, Qt::Vertical );
  m_gbHostnames->layout()->setSpacing( 0 );
  m_gbHostnames->layout()->setMargin( 0 );

  QGridLayout* gridLayout = new QGridLayout( m_gbHostnames->layout() );
  gridLayout->setAlignment( Qt::AlignTop );
  gridLayout->setSpacing( KDialog::spacingHint() );
  gridLayout->setMargin( KDialog::marginHint() );

  QGridLayout* glay = new QGridLayout;
  glay->setSpacing( KDialog::spacingHint() );
  glay->setMargin( 0 );

  m_lbEnvHttp = new QLabel( i18n("H&TTP:"), m_gbHostnames, "lbl_envHttp" );
  glay->addWidget( m_lbEnvHttp, 0, 0 );

  m_leEnvHttp = new KLineEdit( m_gbHostnames, "m_leEnvHttp" );
  m_leEnvHttp->setMinimumWidth( m_leEnvHttp->fontMetrics().width('W') * 20 );
  m_leEnvHttp->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                         QSizePolicy::Fixed,
                                         m_leEnvHttp->sizePolicy().hasHeightForWidth()) );
  QWhatsThis::add( m_leEnvHttp, i18n("<qt>Enter the name of the environment "
                                     "variable, e.g. <tt>HTTP_PROXY</tt>, "
                                     "used to store the address of the HTTP "
                                     "proxy server.<p>Alternatively, you can "
                                     "click on the <tt>\"Auto Detect\"</tt> "
                                     "button to attempt automatic discovery "
                                     "of this variable.</qt>") );
  m_lbEnvHttp->setBuddy ( m_leEnvHttp );
  glay->addWidget( m_leEnvHttp, 0, 1 );

  m_lbEnvHttps = new QLabel( i18n("HTTP&S:"), m_gbHostnames, "lbl_envHttps" );
  glay->addWidget( m_lbEnvHttps, 1, 0 );

  m_leEnvHttps = new KLineEdit( m_gbHostnames, "m_leEnvHttps" );
  m_leEnvHttps->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                          QSizePolicy::Fixed,
                                          m_leEnvHttps->sizePolicy().hasHeightForWidth()) );
  QWhatsThis::add( m_leEnvHttps, i18n("<qt>Enter the name of the environment "
                                      "variable, e.g. <tt>HTTPS_PROXY"
                                      "</tt>, used to store the address of "
                                      "the HTTPS proxy server.<p>"
                                      "Alternatively, you can click on the "
                                      "<tt>\"Auto Detect\"</tt> button "
                                      "to attempt an automatic discovery of "
                                      "this variable.</qt>") );
  m_lbEnvHttps->setBuddy ( m_leEnvHttps );
  glay->addWidget( m_leEnvHttps, 1, 1 );

  m_lbEnvFtp = new QLabel( i18n("&FTP:"), m_gbHostnames, "lbl_envFtp" );
  glay->addWidget( m_lbEnvFtp, 2, 0 );

  m_leEnvFtp = new KLineEdit( m_gbHostnames, "m_leEnvFtp" );
  m_leEnvFtp->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                        QSizePolicy::Fixed,
                                        m_leEnvFtp->sizePolicy().hasHeightForWidth()) );
  QWhatsThis::add( m_leEnvFtp, i18n("<qt>Enter the name of the environment "
                                   "variable, e.g. <tt>FTP_PROXY</tt>, "
                                   "used to store the address of the FTP "
                                   "proxy server.<p>Alternatively, you can "
                                   "click on the <tt>\"Auto Detect\"</tt> "
                                   "button to attempt an automatic discovery "
                                   "of this variable.</qt>") );
  m_lbEnvFtp->setBuddy ( m_leEnvFtp );
  glay->addWidget( m_leEnvFtp, 2, 1 );

  m_lbEnvNoProxy = new QLabel( i18n("&No proxy:"), m_gbHostnames, "lbl_envNoProxy" );
  glay->addWidget( m_lbEnvNoProxy, 3, 0 );

  m_leEnvNoProxy = new KLineEdit( m_gbHostnames, "m_leEnvNoProxy" );
  m_leEnvNoProxy->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                        QSizePolicy::Fixed,
                                        m_leEnvNoProxy->sizePolicy().hasHeightForWidth()) );
  QWhatsThis::add( m_leEnvNoProxy, i18n("<qt>No Proxy For environment variable "
                                   ", e.g. <tt>NO_PROXY</tt>, "
                                   "used to store the addresses of sites for which "
                                   "the proxy server should not be used. You can "
                                   "click on the <tt>\"Auto Detect\"</tt> "
                                   "button to attempt an automatic discovery "
                                   "of this variable.</qt>") );
  m_lbEnvNoProxy->setBuddy ( m_leEnvNoProxy );
  glay->addWidget( m_leEnvNoProxy, 3, 1 );

  gridLayout->addLayout( glay, 0, 0 );

  QVBoxLayout* vlay = new QVBoxLayout;
  vlay->setSpacing( KDialog::spacingHint() );
  vlay->setMargin( 0 );

  m_pbVerify = new QPushButton( i18n("&Verify"), m_gbHostnames, "m_pbVerify" );
  QWhatsThis::add( m_pbVerify, i18n("<qt>Click this button to quickly "
                                    "determine whether or not the environment "
                                    "variable names you supplied are valid. "
                                    "If an environment variable is not found, "
                                    "the associated labels will be "
                                    "<b>highlighted</b> to indicate the "
                                    "invalid settings.</qt>") );
  vlay->addWidget( m_pbVerify );

  m_pbDetect = new QPushButton( i18n("Auto &Detect"), m_gbHostnames,
                                "m_pbDetect" );
  QWhatsThis::add( m_pbDetect, i18n("<qt>Click on this button to attempt "
                                    "automatic discovery of the environment "
                                    "variables used for setting system wide "
                                    "proxy information.<p> This automatic "
                                    "lookup works by searching for the "
                                    "following common variable names:</p>"
                                    "<p><u>For HTTP: </u>%1</p>"
                                    "<p><u>For HTTPS: </u>%2</p>"
                                    "<p><u>For FTP: </u>%3</p></qt>")
                                    .arg(ENV_HTTP_PROXY)
                                    .arg(ENV_HTTPS_PROXY)
                                    .arg(ENV_FTP_PROXY));

  vlay->addWidget( m_pbDetect );

  m_pbShowValue = new QPushButton( i18n("Show &Values"), m_gbHostnames,
                                   "m_pbDetect" );
  m_pbShowValue->setToggleButton ( true );

  m_pbShowValue->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );

  QWhatsThis::add( m_pbShowValue, i18n("<qt>Click on this button to see "
                                   "the actual values associated with the "
                                   "environment variables.</qt>"));

  vlay->addWidget( m_pbShowValue );


  QSpacerItem* spacer = new QSpacerItem( 10, 10, QSizePolicy::Minimum,
                                        QSizePolicy::MinimumExpanding );
  vlay->addItem( spacer );

  gridLayout->addLayout( vlay, 0, 1 );

  mainLayout->addWidget( m_gbHostnames );

#if 0
  m_gbExceptions = new KExceptionBox( page, "m_gbExceptions" );
  m_gbExceptions->setSizePolicy( QSizePolicy(QSizePolicy::Expanding,
                                            QSizePolicy::Preferred,
                                            m_gbExceptions->sizePolicy().hasHeightForWidth()) );
  mainLayout->addWidget( m_gbExceptions );


  QHBoxLayout* hlay = new QHBoxLayout;
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setMargin( 0 );

  spacer = new QSpacerItem( 20, 20, QSizePolicy::MinimumExpanding,
                            QSizePolicy::Minimum );
  hlay->addItem( spacer );
#endif

  init();
}

KEnvVarProxyDlg::~KEnvVarProxyDlg ()
{
}

void KEnvVarProxyDlg::init()
{
  m_bHasValidData = false;

  connect( m_pbShowValue, SIGNAL( toggled(bool) ), SLOT( showValue(bool) ) );
  connect( m_pbVerify, SIGNAL( clicked() ), SLOT( verifyPressed() ) );
  connect( m_pbDetect, SIGNAL( clicked() ), SLOT( autoDetectPressed() ) );
}

void KEnvVarProxyDlg::setProxyData( const KProxyData& data )
{
  // Setup HTTP Proxy...
  KURL u = data.httpProxy;
  if (!u.isEmpty() && !u.isValid())
  {
    m_mapEnvVars["http"].name = data.httpProxy;
    m_mapEnvVars["http"].value = QString::fromLocal8Bit( getenv(data.httpProxy.local8Bit()) );
    if ( !m_mapEnvVars["http"].value.isEmpty() )
      m_leEnvHttp->setText( data.httpProxy );
  }

  // Setup HTTPS Proxy...
  u = data.httpsProxy;
  if (!u.isEmpty() && !u.isValid())
  {
    m_mapEnvVars["https"].name = data.httpsProxy;
    m_mapEnvVars["https"].value = QString::fromLocal8Bit( getenv(data.httpsProxy.local8Bit()) );
    if ( !m_mapEnvVars["https"].value.isEmpty() )
      m_leEnvHttps->setText( data.httpsProxy );
  }

  // Setup FTP Proxy...
  u = data.ftpProxy;
  if (!u.isEmpty() && !u.isValid())
  {
    m_mapEnvVars["ftp"].name = data.ftpProxy;
    m_mapEnvVars["ftp"].value = QString::fromLocal8Bit( getenv(data.ftpProxy.local8Bit()) );
    if ( !m_mapEnvVars["ftp"].value.isEmpty() )
      m_leEnvFtp->setText( data.ftpProxy );
  }

  u = data.noProxyFor.join(",");
  if (!u.isEmpty() && !u.isValid())
  {
    QString noProxy = u.url();
    m_mapEnvVars["noProxy"].name = noProxy;
    m_mapEnvVars["noProxy"].value = QString::fromLocal8Bit( getenv(noProxy.local8Bit()) );
    if ( !m_mapEnvVars["noProxy"].value.isEmpty() )
      m_leEnvNoProxy->setText( noProxy );
  }

#if 0
  //m_gbExceptions->fillExceptions( data.noProxyFor );
  //m_gbExceptions->setCheckReverseProxy( data.useReverseProxy );
#endif
}

const KProxyData KEnvVarProxyDlg::data() const
{
  KProxyData data;

  if (m_bHasValidData)
  {
    data.httpProxy = m_mapEnvVars["http"].name;
    data.httpsProxy = m_mapEnvVars["https"].name;
    data.ftpProxy = m_mapEnvVars["ftp"].name;
    data.noProxyFor = m_mapEnvVars["noProxy"].name;
    data.type = KProtocolManager::EnvVarProxy;
  }

  return data;
}


void KEnvVarProxyDlg::verifyPressed()
{
  if ( !validate() )
  {
    QString msg = i18n("You must specify at least one valid proxy "
                       "environment variable.");

    QString details = i18n("<qt>Make sure you entered the actual environment "
                           "variable name rather than its value. For "
                           "example, if the environment variable is <br><b>"
                           "HTTP_PROXY=http://localhost:3128</b><br> you need "
                           "to enter <b>HTTP_PROXY</b> here instead of the "
                           "actual value http://localhost:3128.</qt>");

    KMessageBox::detailedSorry( this, msg, details,
                                i18n("Invalid Proxy Setup") );
  }
  else
  {
    KMessageBox::information( this, i18n("Successfully verified."),
                                    i18n("Proxy Setup") );
  }
}

void KEnvVarProxyDlg::autoDetectPressed()
{
  QString env;
  QStringList::ConstIterator it;

  bool found = false;

  setHighLight (false, m_lbEnvHttp);
  setHighLight (false, m_lbEnvHttps);
  setHighLight (false, m_lbEnvFtp);

  // Detect HTTP proxy settings...
  QStringList list = QStringList::split( ',', QString::fromLatin1(ENV_HTTP_PROXY) );
  for( it = list.begin(); it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
    if ( !env.isEmpty() )
    {
      m_mapEnvVars ["http"].name = *it;
      m_mapEnvVars ["http"].value = env;
      m_leEnvHttp->setText( (m_pbShowValue->isOn() ? env : *it) );
      found |= true;
      break;
    }
  }

  // Detect HTTPS proxy settings...
  list = QStringList::split( ',', QString::fromLatin1(ENV_HTTPS_PROXY));
  for( it = list.begin(); it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
    if ( !env.isEmpty() )
    {
      m_mapEnvVars ["https"].name = *it;
      m_mapEnvVars ["https"].value = env;
      m_leEnvHttps->setText( (m_pbShowValue->isOn() ? env : *it) );
      found |= true;
      break;
    }
  }

  // Detect FTP proxy settings...
  list = QStringList::split( ',', QString::fromLatin1(ENV_FTP_PROXY) );
  for(it = list.begin(); it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
    if ( !env.isEmpty() )
    {
      m_mapEnvVars ["ftp"].name = *it;
      m_mapEnvVars ["ftp"].value = env;
      m_leEnvFtp->setText( (m_pbShowValue->isOn() ? env : *it) );
      found |= true;
      break;
    }
  }

  // Detect the NO_PROXY settings...
  list = QStringList::split( ',', QString::fromLatin1(NO_PROXY) );
  for(it = list.begin(); it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
    if ( !env.isEmpty() )
    {
      m_mapEnvVars ["noProxy"].name = *it;
      m_mapEnvVars ["noProxy"].value = env;
      m_leEnvNoProxy->setText( (m_pbShowValue->isOn() ? env : *it) );
      break;
    }
  }

  if ( !found )
  {
    QString msg = i18n("Did not detect any environment variables "
                       "commonly used to set system wide proxy "
                       "information.");

    QString details = i18n("<qt>To learn about the variable names the "
                           "automatic detection process searches for, "
                           "press OK, click on the quick help button "
                           "on the window title bar of the "
                           "previous dialog and then click on the "
                           "\"<b>Auto Detect</b>\" button.</qt>");

    KMessageBox::detailedSorry( this, msg, details,
                                i18n("Automatic Proxy Variable Detection") );
  }
}

void KEnvVarProxyDlg::showValue( bool enable )
{
  m_pbShowValue->setText (enable ? i18n ("Hide &Values"):i18n ("Show &Values"));

  m_leEnvHttp->setReadOnly (enable);
  m_leEnvHttps->setReadOnly (enable);
  m_leEnvFtp->setReadOnly (enable);
  m_leEnvNoProxy->setReadOnly (enable);

  if (!m_leEnvHttp->text().isEmpty())
  {
    QString value = enable ? m_mapEnvVars["http"].value : m_mapEnvVars["http"].name;
    m_leEnvHttp->setText( value );
  }

  if (!m_leEnvHttps->text().isEmpty())
  {
    QString value = enable ? m_mapEnvVars["https"].value : m_mapEnvVars["https"].name;
    m_leEnvHttps->setText( value );
  }

  if (!m_leEnvFtp->text().isEmpty())
  {
    QString value = enable ? m_mapEnvVars["ftp"].value : m_mapEnvVars["ftp"].name;
    m_leEnvFtp->setText( value );
  }

  if (!m_leEnvNoProxy->text().isEmpty())
  {
    QString value = enable ? m_mapEnvVars["noProxy"].value : m_mapEnvVars["noProxy"].name;
    m_leEnvNoProxy->setText( value );
  }
}
void KEnvVarProxyDlg::setHighLight (bool highlight, QWidget* widget)
{
  if (!widget)
    return;

  QFont f = widget->font();
  f.setBold( highlight );
  widget->setFont( f );
}

bool KEnvVarProxyDlg::validate()
{
  int count = 0;

  QString value = m_mapEnvVars["http"].value;
  if ( !value.isEmpty() )
    count++;

  value = m_mapEnvVars["https"].value;
  if ( !value.isEmpty() )
    count++;

  value = m_mapEnvVars["ftp"].value;
  if ( !value.isEmpty() )
    count++;

  m_bHasValidData = (count > 0);

  return m_bHasValidData;
}

void KEnvVarProxyDlg::slotOk()
{
  if ( !validate() )
  {
    setHighLight (true, m_lbEnvHttp);
    setHighLight (true, m_lbEnvHttps);
    setHighLight (true, m_lbEnvFtp);

    QString msg = i18n("You must specify at least one valid proxy "
                       "environment variable.");

    QString details = i18n("<qt>Make sure you entered the actual environment "
                           "variable name rather than its value. For "
                           "example, if the environment variable is <br><b>"
                           "HTTP_PROXY=http://localhost:3128</b><br> you need "
                           "to enter <b>HTTP_PROXY</b> here instead of the "
                           "actual value http://localhost:3128.</qt>");

    KMessageBox::detailedError( this, msg, details,
                                i18n("Invalid Proxy Setup") );
    return;
  }

  KDialogBase::slotOk ();
}

#include "kenvvarproxydlg.moc"
