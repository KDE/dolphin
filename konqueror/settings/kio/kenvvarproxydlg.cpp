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

#include <kurl.h>
#include <klocale.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kio/ioslave_defaults.h>

#include "kproxyexceptiondlg.h"
#include "kenvvarproxydlg.h"


#define ENV_FTP_PROXY   "FTP_PROXY,ftp_proxy,FTPPROXY,ftpproxy,PROXY,proxy"
#define ENV_HTTP_PROXY  "HTTP_PROXY,http_proxy,HTTPPROXY,httpproxy,PROXY,proxy"
#define ENV_HTTPS_PROXY "HTTPS_PROXY,https_proxy,HTTPSPROXY,httpsproxy,PROXY,proxy"


KEnvVarProxyDlg::KEnvVarProxyDlg( QWidget* parent, const char* name )
                :KProxyDialogBase( parent, name, true,
                                   i18n( "Variable Proxy Configuration" ) )
{
  QWidget *page = new QWidget( this );
  setMainWidget (page);

  QVBoxLayout* mainLayout = new QVBoxLayout( page );
  mainLayout->setSpacing( KDialog::spacingHint() );
  mainLayout->setMargin( KDialog::marginHint() );

  m_gbHostnames = new QGroupBox( page, "m_gbHostnames" );
  
  QSizePolicy policy (QSizePolicy::Expanding, QSizePolicy::Fixed,
                      m_gbHostnames->sizePolicy().hasHeightForWidth());
  m_gbHostnames->setSizePolicy( policy );
  m_gbHostnames->setTitle( i18n( "Servers" ) );
  m_gbHostnames->setColumnLayout(0, Qt::Vertical );
  m_gbHostnames->layout()->setSpacing( 0 );
  m_gbHostnames->layout()->setMargin( 0 );

  QGridLayout* serverLayout = new QGridLayout( m_gbHostnames->layout() );
  serverLayout->setAlignment( Qt::AlignTop );
  serverLayout->setSpacing( KDialog::spacingHint() );
  serverLayout->setMargin( KDialog::marginHint() );

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
                                     "variable, eg. <tt>HTTP_PROXY</tt>, "
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
                                      "variable, eg. <tt>HTTPS_PROXY"
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
                                   "variable, eg. <tt>FTP_PROXY</tt>, "
                                   "used to store the address of the FTP "
                                   "proxy server.<p>Alternatively, you can "
                                   "click on the <tt>\"Auto Detect\"</tt> "
                                   "button to attempt an automatic discovery "
                                   "of this variable.</qt>") );
  m_lbEnvFtp->setBuddy ( m_leEnvFtp );  
  glay->addWidget( m_leEnvFtp, 2, 1 );

  serverLayout->addLayout( glay, 0, 0 );

  QHBoxLayout *hlay = new QHBoxLayout;
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setMargin( 0 );
  
  QSpacerItem *spacer = new QSpacerItem( 75, 16, QSizePolicy::Fixed,
                                         QSizePolicy::Minimum );
  hlay->addItem( spacer );

  m_cbSameProxy = new QCheckBox( i18n("Use same proxy server for all protocols"), 
                                 m_gbHostnames, "m_cbSameProxy" );
  hlay->addWidget( m_cbSameProxy );
  spacer = new QSpacerItem( 20, 20, QSizePolicy::MinimumExpanding,
                            QSizePolicy::Minimum );
  hlay->addItem( spacer );
  serverLayout->addLayout( hlay, 1, 0 );

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
  
  QWhatsThis::add( m_pbShowValue, i18n("<qt>Click on this button to see "
                                   "the actual values associated with the "
                                   "environment variables.</qt>"));
  vlay->addWidget( m_pbShowValue );

  spacer = new QSpacerItem( 10, 10, QSizePolicy::Minimum,
                            QSizePolicy::MinimumExpanding );
  vlay->addItem( spacer );

  serverLayout->addLayout( vlay, 0, 1 );
  mainLayout->addWidget( m_gbHostnames );

  m_gbExceptions = new KExceptionBox( page, "m_gbExceptions" );
  m_gbExceptions->setSizePolicy( QSizePolicy(QSizePolicy::Expanding,
                                            QSizePolicy::Preferred,
                                            m_gbExceptions->sizePolicy().hasHeightForWidth()) );
  mainLayout->addWidget( m_gbExceptions );

  hlay = new QHBoxLayout;
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setMargin( 0 );

  spacer = new QSpacerItem( 20, 20, QSizePolicy::MinimumExpanding,
                            QSizePolicy::Minimum );
  hlay->addItem( spacer );

  init();
}

KEnvVarProxyDlg::~KEnvVarProxyDlg ()
{
}

void KEnvVarProxyDlg::init()
{
  m_bHasValidData = false;
  
  connect( m_pbShowValue, SIGNAL( toggled(bool) ), SLOT( showValue(bool) ) );                                                                             
  connect( m_cbSameProxy, SIGNAL( toggled(bool) ), SLOT( sameProxy(bool) ) );                                                                             
  
  connect( m_pbVerify, SIGNAL( clicked() ), SLOT( verifyPressed() ) );
  connect( m_pbDetect, SIGNAL( clicked() ), SLOT( autoDetectPressed() ) );
  connect( m_cbSameProxy, SIGNAL( toggled(bool) ), SLOT( sameProxy(bool) ) );  
}

void KEnvVarProxyDlg::setProxyData( const KProxyData& data )
{
  if ( data.type == KProtocolManager::EnvVarProxy )
  {
    m_leEnvHttp->setText( data.httpProxy );
    m_leEnvHttps->setText( data.httpsProxy );
    m_leEnvFtp->setText( data.ftpProxy );    
   
    bool useSameProxy = (data.httpProxy == data.httpsProxy &&
                         data.httpProxy == data.ftpProxy);
    
    m_cbSameProxy->setChecked ( useSameProxy );
        
    if ( useSameProxy )
      sameProxy ( true );
  }
  else if ( data.type == KProtocolManager::NoProxy )
  {
    KURL u;
    QString envVar;
       
    // If this is a non-URL check...
    u = data.httpProxy;
    if ( !u.isValid() )
    {
      envVar = QString::fromLocal8Bit( getenv(data.httpProxy.local8Bit()) );
      if ( !envVar.isEmpty() )
        m_leEnvHttp->setText( data.httpProxy );
    }
    
    bool useSameProxy = (data.httpProxy == data.httpsProxy &&
                         data.httpProxy == data.ftpProxy);
    
    m_cbSameProxy->setChecked ( useSameProxy );
        
    if (useSameProxy)
    {
      m_leEnvHttps->setText ( m_leEnvHttp->text () );
      m_leEnvFtp->setText ( m_leEnvHttp->text () );
      sameProxy ( true );
    }
    else
    {
      u = data.httpsProxy;      
      if ( !u.isValid() )
      {
        envVar = QString::fromLocal8Bit( getenv(data.httpsProxy.local8Bit()) );
        if ( !envVar.isEmpty() )
          m_leEnvHttps->setText( data.httpsProxy );
      }
      
      u = data.ftpProxy;
      if ( !u.isValid() )
      {
        envVar = QString::fromLocal8Bit( getenv(data.ftpProxy.local8Bit()) );
        if ( !envVar.isEmpty() )
          m_leEnvFtp->setText( data.ftpProxy );
      }
    }
  }
  
  m_gbExceptions->fillExceptions( data.noProxyFor );
  m_gbExceptions->setCheckReverseProxy( data.useReverseProxy );  
}

const KProxyData KEnvVarProxyDlg::data() const
{
  KProxyData data;

  if (!m_bHasValidData)
    return data;

  if ( !m_pbShowValue->isOn() )
    data.httpProxy = m_leEnvHttp->text();
  else
    data.httpProxy = m_lstEnvVars.count() > 0 ? m_lstEnvVars[0]: "";
  
  if ( m_cbSameProxy->isChecked () )
  {
    data.httpsProxy = data.httpProxy;
    data.ftpProxy = data.httpProxy;
  }
  else
  {
    if ( !m_pbShowValue->isOn() )
      data.httpsProxy = m_leEnvHttps->text();
    else
      data.httpsProxy = m_lstEnvVars.count() > 1 ? m_lstEnvVars[1]: "";
    
    if ( !m_pbShowValue->isOn() )
      data.ftpProxy = m_leEnvFtp->text();
    else
      data.ftpProxy = m_lstEnvVars.count() > 2 ? m_lstEnvVars[2]: "";
  }

  QStringList list = m_gbExceptions->exceptions();
  if ( list.count() )
    data.noProxyFor = list;
  
  data.type = KProtocolManager::EnvVarProxy;
  data.useReverseProxy = m_gbExceptions->isReverseProxyChecked();

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
  QStringList list;
  QStringList::ConstIterator it;

  bool found = false;

  list = QStringList::split( ',', QString::fromLatin1(ENV_HTTP_PROXY) );
  it = list.begin();
  
  for( ; it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
    if ( !env.isEmpty() )
    {
      m_leEnvHttp->setText( (m_pbShowValue->isOn()?env:*it) );
      found |= true;
      break;
    }
  }

  list = QStringList::split( ',', QString::fromLatin1(ENV_HTTPS_PROXY));
  it = list.begin();
  for( ; it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
    if ( !env.isEmpty() )
    {
      m_leEnvHttps->setText( (m_pbShowValue->isOn()?env:*it) );
      found |= true;
      break;
    }
  }
  
  list = QStringList::split( ',', QString::fromLatin1(ENV_FTP_PROXY) );
  it = list.begin();
  for( ; it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
    if ( !env.isEmpty() )
    {
      m_leEnvFtp->setText( (m_pbShowValue->isOn()?env:*it) );
      found |= true;
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
                           "(<b>?</b>) on the top right corner of the "
                           "previous dialog and then click on the "
                           "\"Auto Detect\" button.</qt> ");

    KMessageBox::detailedSorry( this, msg, details,
                                i18n("Automatic Proxy Variable Detection") );
  }
}

void KEnvVarProxyDlg::showValue( bool enable )
{
  if ( enable )
  {
    QString txt, env;
    
    m_lstEnvVars.clear();
    txt = m_leEnvHttp->text();
    m_pbShowValue->setText ( i18n ("Hide &Values  ") );
        
    if (!txt.isEmpty())
    {
      env = QString::fromLocal8Bit( getenv( txt.local8Bit() ) );
      m_leEnvHttp->setText( env );
      m_lstEnvVars << txt;
    }
    
    txt = m_leEnvHttps->text();
    if (!txt.isEmpty())
    {
      env = QString::fromLocal8Bit( getenv( txt.local8Bit() ) );
      m_leEnvHttps->setText( env );
      m_lstEnvVars << txt;
    }
    
    txt = m_leEnvFtp->text();
    if (!txt.isEmpty());
    {
      env = QString::fromLocal8Bit( getenv( txt.local8Bit() ) );
      m_leEnvFtp->setText( env );
      m_lstEnvVars << txt;
    }
  }
  else
  {
    int count = m_lstEnvVars.count ();
    m_pbShowValue->setText ( i18n ("Show &values") );
    
    if ( count > 0 )
      m_leEnvHttp->setText( m_lstEnvVars[0] );
    if ( count > 1 )
      m_leEnvHttps->setText( m_lstEnvVars[1] );
    if ( count > 2 )    
      m_leEnvFtp->setText( m_lstEnvVars[2] );
  }
}

void KEnvVarProxyDlg::sameProxy( bool enable )
{
  m_leEnvHttps->setEnabled (!enable);
  m_leEnvFtp->setEnabled (!enable);
}

bool KEnvVarProxyDlg::validate()
{
  QFont f;
  QString value;
  
  m_bHasValidData = true;
  
  if ( !m_pbShowValue->isOn() )
    value = m_leEnvHttp->text();
  else
    value = m_lstEnvVars.count() > 0 ? m_lstEnvVars[0]: "";     

  if ( value.isEmpty() || getenv( value.local8Bit() ) == 0L )
  {
    f = m_lbEnvHttp->font();
    f.setBold( true );
    m_lbEnvHttp->setFont( f );
    m_bHasValidData = false;
  }
  
  if (!m_cbSameProxy->isChecked())
  {
    if ( !m_pbShowValue->isOn() )
      value = m_leEnvHttps->text();
    else
      value = m_lstEnvVars.count() > 1 ? m_lstEnvVars[1]: "";
  
    if ( value.isEmpty() || getenv( value.local8Bit() ) == 0L )
    {
      f = m_lbEnvHttps->font();
      f.setBold( true );
      m_lbEnvHttps->setFont( f );
      m_bHasValidData = false;
    }
  
    if ( !m_pbShowValue->isOn() )
      value = m_leEnvFtp->text();
    else
      value = m_lstEnvVars.count() > 2 ? m_lstEnvVars[2]: "";
  
    if ( value.isEmpty() || getenv( value.local8Bit() ) == 0L )
    {
      f = m_lbEnvFtp->font();
      f.setBold( true );
      m_lbEnvFtp->setFont( f );
      m_bHasValidData = false;
    }
  }

  return m_bHasValidData;
}

void KEnvVarProxyDlg::slotOk()
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
    
    KMessageBox::detailedError( this, msg, details,
                                i18n("Invalid Proxy Setup") );
    return;
  }

  KDialogBase::slotOk ();
}

#include "kenvvarproxydlg.moc"
