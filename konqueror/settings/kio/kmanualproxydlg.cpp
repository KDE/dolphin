/*
   kmanualproxydlg.cpp - Proxy configuration dialog

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
#include <qspinbox.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>
#include <klineedit.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kio/ioslave_defaults.h>

#include "kproxyexceptiondlg.h"
#include "kmanualproxydlg.h"


KManualProxyDlg::KManualProxyDlg( QWidget* parent, const char* name )
                :KProxyDialogBase( parent, name, true,
                                   i18n("Manual Proxy Configuration") )
{
    QWidget *page = new QWidget( this );
    setMainWidget (page);
    QVBoxLayout* mainLayout = new QVBoxLayout( page, KDialog::marginHint(),
                                               KDialog::spacingHint() );
    m_gbHostnames = new QGroupBox( i18n("Servers"), page, "m_gbHostnames" );
    m_gbHostnames->setSizePolicy( QSizePolicy(QSizePolicy::Expanding,
                                           QSizePolicy::Fixed,
                                           m_gbHostnames->sizePolicy().hasHeightForWidth()) );
    m_gbHostnames->setColumnLayout(0, Qt::Vertical );
    m_gbHostnames->layout()->setSpacing( 0 );
    m_gbHostnames->layout()->setMargin( 0 );

    QGridLayout* serversLayout = new QGridLayout( m_gbHostnames->layout() );
    serversLayout->setAlignment( Qt::AlignTop );
    serversLayout->setSpacing( KDialog::spacingHint() );
    serversLayout->setMargin( KDialog::marginHint() );

    QGridLayout* glay = new QGridLayout;
    glay->setSpacing( 6 );
    glay->setMargin( 0 );

    m_lbHttp = new QLabel( i18n("H&TTP:"), m_gbHostnames, "lbl_http" );
    glay->addWidget( m_lbHttp, 0, 0 );
    
    m_leHttp = new KLineEdit( m_gbHostnames, "m_leHttp" );
    m_leHttp->setMinimumWidth( m_leHttp->fontMetrics().width('W') * 20 );
    m_leHttp->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                         QSizePolicy::Fixed,
                                         m_leHttp->sizePolicy().hasHeightForWidth()) );
    QWhatsThis::add( m_leHttp, i18n("Enter the address of the HTTP proxy "
                                    "server.") );
    m_lbHttp->setBuddy( m_leHttp );
    glay->addWidget( m_leHttp, 0, 1 );                                    
    
    QLabel *label = new QLabel( i18n("Port"), m_gbHostnames, "lbl_httpport");
    label->setSizePolicy( QSizePolicy(QSizePolicy::Minimum,
                                      QSizePolicy::Fixed,
                                      label->sizePolicy().hasHeightForWidth()) );
    glay->addWidget( label, 0, 2 );                                      
    m_sbHttp = new QSpinBox( m_gbHostnames, "m_sbHttp" );
    QWhatsThis::add( m_sbHttp, i18n("Enter the port number of the HTTP "
                                    "proxy server. Default is 8080. "
                                    "Another common value is 3128.") );
    glay->addWidget( m_sbHttp, 0, 3 );

    m_lbHttps = new QLabel ( i18n("HTTP&S:"), m_gbHostnames, "lbl_https" );
    glay->addWidget( m_lbHttps, 1, 0 );
        
    m_leHttps = new KLineEdit( m_gbHostnames, "m_leHttps" );
    m_leHttps->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                            QSizePolicy::Fixed,
                                            m_leHttps->sizePolicy().hasHeightForWidth()) );
    
    QWhatsThis::add( m_leHttps, i18n("Enter the address of the HTTPS proxy "
                                     "server.") );
    m_lbHttps->setBuddy( m_leHttps );
    glay->addWidget( m_leHttps, 1, 1 );
        
    label = new QLabel( i18n("Port"), m_gbHostnames, "lbl_secureport" );
    label->setSizePolicy( QSizePolicy(QSizePolicy::Minimum,
                                      QSizePolicy::Fixed,
                                      label->sizePolicy().hasHeightForWidth()) );
    glay->addWidget( label, 1, 2 );
                                          
    m_sbHttps = new QSpinBox( m_gbHostnames, "sb_secport" );
    QWhatsThis::add( m_sbHttps, i18n("Enter the port number of the secure "
                                     "proxy server. Default is 8080. "
                                     "Another common value is 3128.") );
    glay->addWidget( m_sbHttps, 1, 3 );

    m_lbFtp = new QLabel( i18n("&FTP:"), m_gbHostnames, "lbl_ftp" );
    glay->addWidget( m_lbFtp, 2, 0 );
    
    m_leFtp = new KLineEdit( m_gbHostnames, "m_leFtp" );
    m_leFtp->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                            QSizePolicy::Fixed,
                                            m_leFtp->sizePolicy().hasHeightForWidth()) );                                            
    QWhatsThis::add( m_leFtp, i18n("Enter the address of the FTP proxy "
                                   "server") );
    m_lbFtp->setBuddy( m_leFtp );
    glay->addWidget( m_leFtp, 2, 1 );
        
    label = new QLabel( i18n("Port"), m_gbHostnames, "lbl_ftpport" );
    label->setSizePolicy( QSizePolicy(QSizePolicy::Minimum,
                                      QSizePolicy::Fixed,
                                      label->sizePolicy().hasHeightForWidth()) );
    glay->addWidget( label, 2, 2 );
                                          
    m_sbFtp = new QSpinBox( m_gbHostnames, "sb_ftpport" );
    QWhatsThis::add( m_sbFtp, i18n("Enter the port number of the FTP proxy "
                                   "server. Default 8080. Another common value "
                                   "is 3128.") );
    glay->addWidget( m_sbFtp, 2, 3 );

    serversLayout->addLayout( glay, 0, 0 );
    
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
    serversLayout->addLayout( hlay, 1, 0 );    

    QVBoxLayout* vlay = new QVBoxLayout;
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( 0 );

    m_pbCopyDown = new QPushButton( m_gbHostnames, "m_pbCopyDown" );
    m_pbCopyDown->setPixmap( BarIcon("down", KIcon::SizeSmall) );
    m_pbCopyDown->setSizePolicy( QSizePolicy( QSizePolicy::Fixed,
                                             QSizePolicy::Fixed,
                                             m_pbCopyDown->sizePolicy().hasHeightForWidth()) );

    QWhatsThis::add( m_pbCopyDown, i18n("<qt>This button allows you to copy "
                                        "the entry of one input field into all "
                                        "the others underneath it. For "
                                        "example, if you fill out the "
                                        "information for <tt>HTTP</tt> and "
                                        "press this button, whatever you "
                                        "entered will be copied to all the "
                                        "fields below that are enabled!") );
    vlay->addWidget( m_pbCopyDown );
    spacer = new QSpacerItem( 1, 1 );
    vlay->addItem( spacer );

    serversLayout->addLayout( vlay, 0, 1 );
    mainLayout->addWidget( m_gbHostnames );

    m_gbExceptions = new KExceptionBox( page, "m_gbExceptions" );
    m_gbExceptions->setSizePolicy( QSizePolicy(QSizePolicy::Expanding,
                                              QSizePolicy::Preferred,
                                              m_gbExceptions->sizePolicy().hasHeightForWidth()) );
    mainLayout->addWidget( m_gbExceptions );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );

    init();
}

KManualProxyDlg::~KManualProxyDlg()
{
}

void KManualProxyDlg::init()
{
    m_bHasValidData = false;
        
    m_sbHttp->setRange( 0, MAX_PORT_VALUE );
    m_sbHttps->setRange( 0, MAX_PORT_VALUE );
    m_sbFtp->setRange( 0, MAX_PORT_VALUE );   

    connect( m_cbSameProxy, SIGNAL( toggled(bool) ), SLOT( sameProxy(bool) ) );
    connect( m_pbCopyDown, SIGNAL( clicked() ), SLOT( copyDown() ) );
}

void KManualProxyDlg::setProxyData( const KProxyData &data )
{
    if ( data.type == KProtocolManager::NoProxy ||
         data.type == KProtocolManager::ManualProxy )
    {
        KURL u;

        int port;
        QString host;
        
        // Set the HTTP proxy
        u = data.httpProxy;
        if ( u.isValid() );
        {
            port = u.port();
            if ( port < 1 )
                port = DEFAULT_PROXY_PORT;
            u.setPort( 0 );
            host = u.url();
            m_leHttp->setText( host );
            m_sbHttp->setValue( port );
        }
        
        bool useSameProxy = (data.httpProxy == data.httpsProxy &&
                             data.httpProxy == data.ftpProxy);
    
        m_cbSameProxy->setChecked ( useSameProxy );
        
        
        if ( useSameProxy )
        {
          m_leHttps->setText ( host );
          m_leFtp->setText ( host );
          m_sbHttps->setValue( port );
          m_sbFtp->setValue( port );
          sameProxy ( true );
        }
        else
        {
            // Set the HTTPS proxy
            u = data.httpsProxy;
            if ( u.isValid() )
            {
                port = u.port();
                if ( port < 1 )
                    port = DEFAULT_PROXY_PORT;
            
                u.setPort( 0 );
                m_leHttps->setText( u.url() );
                m_sbHttps->setValue( port );
            }
          
            // Set the FTP proxy...
            u = data.ftpProxy;
            if ( u.isValid() )
            {
                port = u.port();
                if ( port < 1 )
                    port = DEFAULT_PROXY_PORT;
                u.setPort( 0 );
                m_leFtp->setText( u.url() );
                m_sbFtp->setValue( port );
            }
        }
        
        m_gbExceptions->fillExceptions( data.noProxyFor );
        m_gbExceptions->setCheckReverseProxy( data.useReverseProxy );
    }
    else
    {
       m_sbFtp->setValue( DEFAULT_PROXY_PORT );    
       m_sbHttp->setValue( DEFAULT_PROXY_PORT );
       m_sbHttps->setValue( DEFAULT_PROXY_PORT );
    }
}

const KProxyData KManualProxyDlg::data() const
{
    KURL u;
    KProxyData data;

    if (!m_bHasValidData)
      return data;

    u = m_leHttp->text();
    if ( u.isValid() )
    {
        u.setPort( m_sbHttp->value() );
        data.httpProxy = u.url();        
    }
    
    if ( m_cbSameProxy->isChecked () )
    {
        data.httpsProxy = data.httpProxy;
        data.ftpProxy = data.httpProxy;
    }
    else
    {
        u = m_leHttps->text();
        if ( u.isValid() )
        {
            u.setPort( m_sbHttps->value() );
            data.httpsProxy = u.url();          
        }
        
        u = m_leFtp->text();
        if ( u.isValid() )
        {
            u.setPort( m_sbFtp->value() );
            data.ftpProxy = u.url();
        }
    }

    QStringList list = m_gbExceptions->exceptions();
    if ( !list.isEmpty() )
        data.noProxyFor = list;

    data.type = KProtocolManager::ManualProxy;
    data.useReverseProxy = m_gbExceptions->isReverseProxyChecked();

    return data;
}

void KManualProxyDlg::sameProxy( bool enable )
{
    m_leHttps->setEnabled (!enable );
    m_leFtp->setEnabled (!enable );
    m_sbHttps->setEnabled (!enable );
    m_sbFtp->setEnabled (!enable );  
    m_pbCopyDown->setEnabled( !enable );      
}

bool KManualProxyDlg::validate()
{
    KURL u;
    QFont f;
    
    m_bHasValidData = false;

    u = m_leHttp->text();
    if ( u.isValid() )
        m_bHasValidData |= true;
    else
    {
        f = m_lbHttp->font();
        f.setBold( true );
        m_lbHttp->setFont( f );
    }
    
    if ( !m_cbSameProxy->isChecked () )
    {
        u = m_leHttps->text();
        if ( u.isValid() )
            m_bHasValidData |= true;
        else
        {
            f = m_lbHttps->font();
            f.setBold( true );
            m_lbHttps->setFont( f );
        }
    
        u = m_leFtp->text();
        if ( u.isValid() )
            m_bHasValidData |= true;
        else
        {
            f = m_lbFtp->font();
            f.setBold( true );
            m_lbFtp->setFont( f );
        }
    }

    if ( !m_bHasValidData )
    {
        QString msg = i18n("You must specify at least one proxy address.");

        QString details = i18n("<qt>Make sure that you have specified at least one "
                               "or more valid proxy addresses. Note that you <b>must"
                               "</b> supply a fully qualified address such as "
                               "<b>http://192.168.20.1</b> or <b>http://proxy.foo."
                               "com</b>. All addresses that do not start with a "
                               "protocol (eg: http://) will be rejected as invalid "
                               "proxy addresses.</qt>");

        KMessageBox::detailedError( this, msg, details,
                                    i18n("Invalid Proxy Setup") );
    }

    return m_bHasValidData;
}

void KManualProxyDlg::copyDown()
{
    int action = -1;

    if ( !m_leHttp->text().isEmpty() )
        action += 4;
    else if ( !m_leHttps->text().isEmpty() )
        action += 3;

    switch ( action )
    {
    case 3:
      m_leHttps->setText( m_leHttp->text() );
      m_sbHttps->setValue( m_sbHttp->value() );
      m_leFtp->setText( m_leHttp->text() );
      m_sbFtp->setValue( m_sbHttp->value() );
      break;
    case 2:
      m_leFtp->setText( m_leHttps->text() );
      m_sbFtp->setValue( m_sbHttps->value() );
      break;
    case 1:
    case 0:
    default:
        break;
    }
}

void KManualProxyDlg::slotOk()
{
    if ( validate() )
      KDialogBase::slotOk();
}

#include "kmanualproxydlg.moc"
