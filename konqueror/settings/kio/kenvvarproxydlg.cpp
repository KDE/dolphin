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
    m_gbHostnames->setSizePolicy( QSizePolicy(QSizePolicy::Expanding,
                                           QSizePolicy::Fixed,
                                           m_gbHostnames->sizePolicy().hasHeightForWidth()) );
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

    m_cbEnvHttp = new QCheckBox( i18n("&HTTP:"), m_gbHostnames, "m_cbEnvHttp" );
    QWhatsThis::add( m_cbEnvHttp, i18n("Check this box to enable environment "
                                      "variable based proxy setup for HTTP "
                                      "connections.") );
    m_leEnvHttp = new KLineEdit( m_gbHostnames, "m_leEnvHttp" );
    m_leEnvHttp->setEnabled( false );
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
    glay->addWidget( m_cbEnvHttp, 0, 0 );
    glay->addWidget( m_leEnvHttp, 0, 1 );

    m_cbEnvHttps = new QCheckBox( i18n("HTTP&S:"), m_gbHostnames, "m_cbEnvHttps" );
    QWhatsThis::add( m_cbEnvHttps, i18n("Check this box to enable environment "
                                        "variable based proxy setup for Https "
                                        "connections such as HTTPS.") );
    m_leEnvHttps = new KLineEdit( m_gbHostnames, "m_leEnvHttps" );
    m_leEnvHttps->setEnabled( false );
    m_leEnvHttps->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                            QSizePolicy::Fixed,
                                            m_leEnvHttps->sizePolicy().hasHeightForWidth()) );
    QWhatsThis::add( m_leEnvHttps, i18n("<qt>Enter the name of the environment "
                                        "variable, eg. <tt>HTTPS_PROXY"
                                        "</tt>, used to store the address of "
                                        "the Https proxy server.<p>"
                                        "Alternatively, you can click on the "
                                        "<tt>\"Auto Detect\"</tt> button "
                                        "to attempt an automatic discovery of "
                                        "this variable(s).</qt>") );

    glay->addWidget( m_cbEnvHttps, 1, 0 );
    glay->addWidget( m_leEnvHttps, 1, 1 );

    m_cbEnvFtp = new QCheckBox( i18n("&FTP:"), m_gbHostnames, "m_cbEnvFtp" );
    QWhatsThis::add( m_cbEnvFtp, i18n("Check this box to enable environment "
                                     "variable based proxy setup for FTP "
                                     "connections.") );

    m_leEnvFtp = new KLineEdit( m_gbHostnames, "m_leEnvFtp" );
    m_leEnvFtp->setEnabled( false );
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
    glay->addWidget( m_cbEnvFtp, 2, 0 );
    glay->addWidget( m_leEnvFtp, 2, 1 );

    serverLayout->addLayout( glay, 0, 0 );

    QHBoxLayout* hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );
    QSpacerItem* spacer = new QSpacerItem( 75, 16,
                              QSizePolicy::Fixed, QSizePolicy::Minimum );
    hlay->addItem( spacer );

    m_cbShowValue = new QCheckBox( i18n("Show actual &values"), m_gbHostnames,
                                  "m_cbShowValue" );
    hlay->addWidget( m_cbShowValue );
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
                                     "<p><u>For Https: </u>%2</p>"
                                     "<p><u>For FTP: </u>%3</p></qt>").arg(ENV_HTTP_PROXY).arg(ENV_HTTPS_PROXY).arg(ENV_FTP_PROXY));
    vlay->addWidget( m_pbDetect );

    spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
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
    connect( m_cbEnvHttp, SIGNAL( toggled(bool) ), m_leEnvHttp,
             SLOT( setEnabled(bool) ) );
    connect( m_cbEnvHttps, SIGNAL( toggled(bool) ), m_leEnvHttps,
             SLOT( setEnabled(bool) ) );
    connect( m_cbEnvFtp, SIGNAL( toggled(bool) ), m_leEnvFtp,
             SLOT( setEnabled(bool) ) );

    connect( m_cbEnvHttp, SIGNAL( toggled(bool) ), SLOT( setChecked(bool) ) );
    connect( m_cbEnvHttps, SIGNAL( toggled(bool) ), SLOT( setChecked(bool) ) );
    connect( m_cbEnvFtp, SIGNAL( toggled(bool) ), SLOT( setChecked(bool) ) );
    connect( m_cbShowValue, SIGNAL( toggled(bool) ), SLOT( showValue(bool) ) );

    connect( m_pbVerify, SIGNAL( clicked() ), SLOT( verifyPressed() ) );
    connect( m_pbDetect, SIGNAL( clicked() ), SLOT( autoDetectPressed() ) );

    setChecked( true );
}

void KEnvVarProxyDlg::setProxyData( const KProxyData& data )
{
    if ( data.type == KProtocolManager::EnvVarProxy )
    {
        if ( !data.httpProxy.isEmpty() )
        {
            m_cbEnvHttp->setChecked( true );
            m_leEnvHttp->setText( data.httpProxy );
        }

        if ( !data.httpsProxy.isEmpty() )
        {
            m_cbEnvHttps->setChecked( true );
            m_leEnvHttps->setText( data.httpsProxy );
        }
        if ( !data.ftpProxy.isEmpty() )
        {
            m_cbEnvFtp->setChecked( true );
            m_leEnvFtp->setText( data.ftpProxy );
        }

        m_gbExceptions->fillExceptions( data.noProxyFor );
        m_gbExceptions->setCheckReverseProxy( data.useReverseProxy );
    }
    else if ( data.type == KProtocolManager::NoProxy )
    {
        KURL u;

        // If this is a non-URL check...
        u = data.httpProxy;
        if ( !u.isValid() )
        {
            QString envVar;
            envVar = QString::fromLocal8Bit( getenv(data.httpProxy.local8Bit()) );
            if ( envVar.isEmpty() )
            {
                m_cbEnvHttp->setChecked( true );
                m_leEnvHttp->setText( envVar );
            }
        }

        u = data.httpsProxy;
        if ( !u.isValid() )
        {
            QString envVar;
            envVar = QString::fromLocal8Bit( getenv(data.httpsProxy.local8Bit()) );
            if ( envVar.isEmpty() )
            {
                m_cbEnvHttps->setChecked( true );
                m_leEnvHttps->setText( envVar );
            }
        }

        u = data.ftpProxy;
        if ( !u.isValid() )
        {
            QString envVar;
            envVar = QString::fromLocal8Bit( getenv(data.ftpProxy.local8Bit()) );
            if ( envVar.isEmpty() )
            {
                m_cbEnvFtp->setChecked( true );
                m_leEnvFtp->setText( envVar );
            }
        }

        m_gbExceptions->fillExceptions( data.noProxyFor );
        m_gbExceptions->setCheckReverseProxy( data.useReverseProxy );
    }
}

const KProxyData KEnvVarProxyDlg::data() const
{
    KProxyData data;

    if (!m_bHasValidData)
      return data;

    if ( m_cbEnvHttp->isChecked() )
        data.httpProxy = m_leEnvHttp->text();

    if ( m_cbEnvHttps->isChecked() )
        data.httpsProxy = m_leEnvHttps->text();

    if ( m_cbEnvFtp->isChecked() )
        data.ftpProxy = m_leEnvFtp->text();

    QStringList list = m_gbExceptions->exceptions();
    if ( list.count() )
        data.noProxyFor = list;

    data.useReverseProxy = m_gbExceptions->isReverseProxyChecked();
    data.type = KProtocolManager::EnvVarProxy;

    return data;
}


void KEnvVarProxyDlg::verifyPressed()
{
    if ( !validate() )
    {
        QString msg = i18n("The highlighted input field(s) contain unknown or "
                           "invalid proxy environment variable.");

        QString details = i18n("<qt>Make sure you entered the actual environment "
                               "variable name rather than its value. For "
                               "example, if the environment variable is <br><b>"
                               "HTTP_PROXY=http://localhost:3128</b><br> you need "
                               "to enter <b>HTTP_PROXY</b> here instead of the "
                               "actual address specified by the variable.</qt>");

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

    if ( m_cbEnvHttp->isChecked() )
    {
        list = QStringList::split( ',', QString::fromLatin1(ENV_HTTP_PROXY) );
        it = list.begin();
        for( ; it != list.end(); ++it )
        {
            env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
            if ( !env.isEmpty() )
            {
                m_leEnvHttp->setText( (m_cbShowValue->isChecked()?env:*it) );
                found |= true;
                break;
            }
        }
    }

    if ( m_cbEnvHttps->isChecked() )
    {
        list = QStringList::split( ',', QString::fromLatin1(ENV_HTTPS_PROXY));
        it = list.begin();
        for( ; it != list.end(); ++it )
        {
            env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
            if ( !env.isEmpty() )
            {
                m_leEnvHttps->setText( (m_cbShowValue->isChecked()?env:*it) );
                found |= true;
                break;
            }
        }
    }

    if ( m_cbEnvFtp->isChecked() )
    {
        list = QStringList::split( ',', QString::fromLatin1(ENV_FTP_PROXY) );
        it = list.begin();
        for( ; it != list.end(); ++it )
        {
            env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
            if ( !env.isEmpty() )
            {
                m_leEnvFtp->setText( (m_cbShowValue->isChecked()?env:*it) );
                found |= true;
                break;
            }
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
        if ( m_cbEnvHttp->isChecked() )
        {
            txt = m_leEnvHttp->text();
            env = QString::fromLocal8Bit( getenv( txt.local8Bit() ) );
            m_leEnvHttp->setText( env );
            m_lstEnvVars << txt;
        }
        if ( m_cbEnvHttps->isChecked() )
        {
            txt = m_leEnvHttps->text();
            env = QString::fromLocal8Bit( getenv( txt.local8Bit() ) );
            m_leEnvHttps->setText( env );
            m_lstEnvVars << txt;
        }
        if ( m_cbEnvFtp->isChecked() )
        {
            txt = m_leEnvFtp->text();
            env = QString::fromLocal8Bit( getenv( txt.local8Bit() ) );
            m_leEnvFtp->setText( env );
            m_lstEnvVars << txt;
        }
    }
    else
    {
        int count = 0;
        if ( m_cbEnvHttp->isChecked() )
            m_leEnvHttp->setText( m_lstEnvVars[count++] );

        if ( m_cbEnvHttps->isChecked() )
            m_leEnvHttps->setText( m_lstEnvVars[count++] );

        if ( m_cbEnvFtp->isChecked() )
            m_leEnvFtp->setText( m_lstEnvVars[count++] );
    }
}

void KEnvVarProxyDlg::setChecked( bool )
{
    bool checked;

    checked = (m_cbEnvHttp->isChecked() || m_cbEnvHttps->isChecked() ||
               m_cbEnvFtp->isChecked());

    m_pbVerify->setEnabled( checked );
    m_pbDetect->setEnabled( checked );
}

bool KEnvVarProxyDlg::validate()
{
    QFont f;
    QString value;

    if ( m_cbEnvHttp->isChecked() )
    {
        value = m_cbShowValue ? m_lstEnvVars[0] : m_leEnvHttp->text();

        if ( getenv( value.local8Bit() ) == 0L )
        {
            f = m_cbEnvHttp->font();
            f.setBold( true );
            m_cbEnvHttp->setFont( f );
            m_bHasValidData |= true;
        }
    }

    if ( m_cbEnvHttps->isChecked() )
    {
        value = m_cbShowValue ? m_lstEnvVars[1] : m_leEnvHttp->text();

        if ( getenv( value.local8Bit() ) == 0L )
        {
            f = m_cbEnvHttps->font();
            f.setBold( true );
            m_cbEnvHttps->setFont( f );
            m_bHasValidData |= true;
        }

    }

    if ( m_cbEnvFtp->isChecked() )
    {
        value = m_cbShowValue ? m_lstEnvVars[2] : m_leEnvHttp->text();

        if ( getenv( value.local8Bit() ) == 0L )
        {
            f = m_cbEnvFtp->font();
            f.setBold( true );
            m_cbEnvFtp->setFont( f );
            m_bHasValidData |= true;
        }
    }

    return m_bHasValidData;
}

void KEnvVarProxyDlg::slotOk()
{
    if ( !validate() )
    {
        QString msg = i18n("The highlighted input field(s) contain unknown or "
                           "invalid environment variable!");

        QString details = i18n("<qt>Make sure you entered the actual "
                               "environment variable name rather than the "
                               "address of the proxy server. For example, "
                               "if the set variable name used to specify the "
                               "HTTP proxy server is <b>HTTP_PROXY</b>, then "
                               "you need to enter <b>HTTP_PROXY</b> instead "
                               "of the actual address specified by the "
                               "variable.</qt>");

        KMessageBox::detailedError( this, msg, details,
                                    i18n("Invalid Proxy Setup") );
        return;
    }

    KDialogBase::slotOk ();
}

#include "kenvvarproxydlg.moc"
