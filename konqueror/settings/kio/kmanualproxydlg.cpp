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
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

#include <kurl.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kio/ioslave_defaults.h>

#include "kproxydlg.h"

class KManLineEdit : public KLineEdit
{
public:
  KManLineEdit( QWidget *parent, const char *name=0L );

protected:
  virtual void keyPressEvent( QKeyEvent * );
};

KManLineEdit::KManLineEdit( QWidget *parent, const char *name )
            :KLineEdit( parent, name )
{
    // For now do not accept any drops since they
    // might contain characters we do not accept.
    // TODO: Re-implement ::dropEvent to allow
    // acceptable formats...
    setAcceptDrops( false );
}

void KManLineEdit::keyPressEvent( QKeyEvent * e )
{
    int key = e->key();
    QString keycode = e->text();
    if ( (key >= Qt::Key_Escape && key <= Qt::Key_Help) ||
          key == Qt::Key_Period ||
          (cursorPosition() > 0 && key == Qt::Key_Minus) ||
          (!keycode.isEmpty() && keycode.unicode()->isPrint()) )
    {
        KLineEdit::keyPressEvent(e);
        return;
    }
    e->accept();
}

KManualProxyDlg::KManualProxyDlg( QWidget* parent, const char* name )
                :KCommonProxyDlg( parent, name, true )
{
    setCaption( i18n("Manual Proxy Configuration") );
    QVBoxLayout* mainLayout = new QVBoxLayout( this,
                                               2*KDialog::marginHint(),
                                               KDialog::spacingHint() );
    gb_servers = new QGroupBox( i18n("Servers"), this, "gb_servers" );
    gb_servers->setSizePolicy( QSizePolicy(QSizePolicy::Expanding,
                                           QSizePolicy::Fixed,
                                           gb_servers->sizePolicy().hasHeightForWidth()) );
    gb_servers->setColumnLayout(0, Qt::Vertical );
    gb_servers->layout()->setSpacing( 0 );
    gb_servers->layout()->setMargin( 0 );
    QGridLayout* gb_serversLayout = new QGridLayout( gb_servers->layout() );
    gb_serversLayout->setAlignment( Qt::AlignTop );
    gb_serversLayout->setSpacing( KDialog::spacingHint() );
    gb_serversLayout->setMargin( 2*KDialog::marginHint() );

    QGridLayout* glay = new QGridLayout;
    glay->setSpacing( 6 );
    glay->setMargin( 0 );

    cb_httpproxy = new QCheckBox( i18n("&HTTP:"), gb_servers, "cb_httpproxy" );
    QWhatsThis::add( cb_httpproxy, i18n("Check this box to enable manual proxy "
                                        "setup for HTTP connections.") );
    le_httpproxy = new KManLineEdit( gb_servers, "le_httpproxy" );
    le_httpproxy->setMinimumWidth( le_httpproxy->fontMetrics().width('W') * 20 );
    le_httpproxy->setEnabled( false );
    le_httpproxy->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding,
                                              QSizePolicy::Fixed,
                                              le_httpproxy->sizePolicy().hasHeightForWidth()) );
    QWhatsThis::add( le_httpproxy, i18n("Enter the address of the HTTP proxy "
                                        "server.") );
    QLabel* label = new QLabel( i18n("Port"), gb_servers, "lbl_httpport");
    connect( cb_httpproxy, SIGNAL( toggled(bool) ), label,
             SLOT( setEnabled(bool) ) );
    label->setEnabled( false );
    label->setSizePolicy( QSizePolicy(QSizePolicy::Minimum,
                                      QSizePolicy::Fixed,
                                      label->sizePolicy().hasHeightForWidth()) );
    sb_httpproxy = new QSpinBox( gb_servers, "sb_httpproxy" );
    sb_httpproxy->setEnabled( false );
    QWhatsThis::add( sb_httpproxy, i18n("Enter the port number of the HTTP "
                                       "proxy server.  Default is 8080. "
                                       "Another common value is 3128.") );

    glay->addWidget( cb_httpproxy, 0, 0 );
    glay->addWidget( le_httpproxy, 0, 1 );
    glay->addWidget( label, 0, 2 );
    glay->addWidget( sb_httpproxy, 0, 3 );

    cb_secproxy = new QCheckBox( i18n("HTTP&S:"), gb_servers, "cb_secproxy" );
    QWhatsThis::add( cb_secproxy, i18n("Check this box to enable manual "
                                       "proxy setup for secure web "
                                       "connections (HTTPS).") );

    le_secproxy = new KManLineEdit( gb_servers, "le_secproxy" );
    le_secproxy->setEnabled( FALSE );
    le_secproxy->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                            QSizePolicy::Fixed,
                                            le_secproxy->sizePolicy().hasHeightForWidth()) );
    QWhatsThis::add( le_secproxy, i18n("Enter the address of the secure "
                                       "proxy server.") );
    label = new QLabel( i18n("Port"), gb_servers, "lbl_secureport" );
    connect( cb_secproxy, SIGNAL( toggled(bool) ), label,
             SLOT( setEnabled(bool) ) );
    label->setEnabled( false );
    label->setSizePolicy( QSizePolicy(QSizePolicy::Minimum,
                                      QSizePolicy::Fixed,
                                      label->sizePolicy().hasHeightForWidth()) );
    sb_secproxy = new QSpinBox( gb_servers, "sb_secport" );
    sb_secproxy->setEnabled( false );
    QWhatsThis::add( sb_secproxy, i18n("Enter the port number of the secure "
                                      "proxy server.  Default is 8080. "
                                      "Another common value is 3128.") );

    glay->addWidget( cb_secproxy, 1, 0 );
    glay->addWidget( le_secproxy, 1, 1 );
    glay->addWidget( label, 1, 2 );
    glay->addWidget( sb_secproxy, 1, 3 );

    cb_ftpproxy = new QCheckBox( i18n("&FTP:"), gb_servers, "cb_ftpproxy" );
    QWhatsThis::add( cb_ftpproxy, i18n("Check this box to enable manual proxy "
                                       "setup for FTP connections.") );

    le_ftpproxy = new KManLineEdit( gb_servers, "le_ftpproxy" );
    le_ftpproxy->setEnabled( false );
    le_ftpproxy->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                            QSizePolicy::Fixed,
                                            le_ftpproxy->sizePolicy().hasHeightForWidth()) );
    QWhatsThis::add( le_ftpproxy, i18n("Enter the address of the FTP proxy "
                                       "server") );
    label = new QLabel( i18n("Port"), gb_servers, "lbl_ftpport" );
    connect( cb_ftpproxy, SIGNAL( toggled(bool) ), label,
             SLOT( setEnabled(bool) ) );
    label->setEnabled( false );
    label->setSizePolicy( QSizePolicy(QSizePolicy::Minimum,
                                      QSizePolicy::Fixed,
                                      label->sizePolicy().hasHeightForWidth()) );
    sb_ftpproxy = new QSpinBox( gb_servers, "sb_ftpport" );
    sb_ftpproxy->setEnabled( false );
    QWhatsThis::add( sb_ftpproxy, i18n("Enter the port number of the FTP "
                                      "proxy server.  Default 8080. "
                                      "Another common value is 3128.") );

    glay->addWidget( cb_ftpproxy, 2, 0 );
    glay->addWidget( le_ftpproxy, 2, 1 );
    glay->addWidget( label, 2, 2 );
    glay->addWidget( sb_ftpproxy, 2, 3 );

    cb_gopherproxy = new QCheckBox( i18n("&Gopher:"), gb_servers,
                                    "cb_gopherproxy" );
    QWhatsThis::add( cb_gopherproxy, i18n("Check this box to enable manual "
                                          "proxy setup for gopher "
                                          "connections."));
    le_gopherproxy = new KManLineEdit( gb_servers, "le_gopherproxy" );
    le_gopherproxy->setEnabled( false );
    le_gopherproxy->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding,
                                                QSizePolicy::Fixed,
                                                le_gopherproxy->sizePolicy().hasHeightForWidth()) );
    QWhatsThis::add( le_gopherproxy, i18n("Enter the address of the gopher "
                                          "proxy server.") );
    label = new QLabel( i18n("Port"), gb_servers, "lbl_gopherproxy" );
    connect( cb_gopherproxy, SIGNAL( toggled(bool) ), label,
             SLOT( setEnabled(bool) ) );
    label->setEnabled( false );
    label->setSizePolicy( QSizePolicy(QSizePolicy::Minimum,
                                      QSizePolicy::Fixed,
                                      label->sizePolicy().hasHeightForWidth()) );
    sb_gopherproxy = new QSpinBox( gb_servers, "sb_gopherport" );
    sb_gopherproxy->setEnabled( false );
    QWhatsThis::add( sb_gopherproxy, i18n("Enter the port number for the gopher "
                                         "proxy server.  Default is 8080. "
                                         "Another common value is 3128.") );

    glay->addWidget( cb_gopherproxy, 3, 0 );
    glay->addWidget( le_gopherproxy, 3, 1 );
    glay->addWidget( label, 3, 2 );
    glay->addWidget( sb_gopherproxy, 3, 3 );

    gb_serversLayout->addLayout( glay, 0, 0 );

    QVBoxLayout* vlay = new QVBoxLayout;
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( 0 );

    pb_copyDown = new QPushButton( gb_servers, "pb_copyDown" );
    pb_copyDown->setPixmap( BarIcon("down", KIcon::SizeSmall) );
    pb_copyDown->setSizePolicy( QSizePolicy( QSizePolicy::Fixed,
                                             QSizePolicy::Fixed,
                                             pb_copyDown->sizePolicy().hasHeightForWidth()) );

    QWhatsThis::add( pb_copyDown, i18n("<qt>This button allows you to copy "
                                       "the entry of one input field into all "
                                       "the others underneath it.  For "
                                       "example, if you fill out the "
                                       "information for <tt>HTTP</tt> and "
                                       "press this button, whatever you "
                                       "entered will be copied to all the "
                                       "fields below that are enabled!") );
    vlay->addWidget( pb_copyDown );
    QSpacerItem* spacer = new QSpacerItem( 1, 1 );
    vlay->addItem( spacer );

    gb_serversLayout->addLayout( vlay, 0, 1 );
    mainLayout->addWidget( gb_servers );

    gb_exceptions = new KExceptionBox( this, "gb_exceptions" );
    gb_exceptions->setSizePolicy( QSizePolicy(QSizePolicy::Expanding,
                                              QSizePolicy::Preferred,
                                              gb_exceptions->sizePolicy().hasHeightForWidth()) );
    mainLayout->addWidget( gb_exceptions );

    QHBoxLayout* hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );

    pb_ok = new QPushButton( i18n("&OK"), this, "pb_ok" );
    pb_ok->setAutoDefault( true );
    hlay->addWidget( pb_ok );

    pb_cancel = new QPushButton( i18n("&Cancel"), this, "pb_cancel" );
    hlay->addWidget( pb_cancel );
    mainLayout->addLayout( hlay );

    init();
}

void KManualProxyDlg::init()
{
    d = 0L;
    // Enable entries
    connect( cb_httpproxy, SIGNAL( toggled(bool) ), le_httpproxy,
             SLOT( setEnabled(bool) ) );
    connect( cb_secproxy, SIGNAL( toggled(bool) ), le_secproxy,
             SLOT( setEnabled(bool) ) );
    connect( cb_ftpproxy, SIGNAL( toggled(bool) ), le_ftpproxy,
             SLOT( setEnabled(bool) ) );
    connect( cb_gopherproxy, SIGNAL( toggled(bool) ), le_gopherproxy,
             SLOT( setEnabled(bool) ) );

    // Enable port settings
    connect( cb_httpproxy, SIGNAL( toggled(bool) ), sb_httpproxy,
             SLOT( setEnabled(bool) ) );
    connect( cb_secproxy, SIGNAL( toggled(bool) ), sb_secproxy,
             SLOT( setEnabled(bool) ) );
    connect( cb_ftpproxy, SIGNAL( toggled(bool) ), sb_ftpproxy,
             SLOT( setEnabled(bool) ) );
    connect( cb_gopherproxy, SIGNAL( toggled(bool) ), sb_gopherproxy,
             SLOT( setEnabled(bool) ) );

    connect( cb_httpproxy, SIGNAL( toggled(bool) ), SLOT( setChecked(bool) ) );
    connect( cb_secproxy, SIGNAL( toggled(bool) ), SLOT( setChecked(bool) ) );
    connect( cb_ftpproxy, SIGNAL( toggled(bool) ), SLOT( setChecked(bool) ) );
    connect( cb_gopherproxy, SIGNAL( toggled(bool) ),
             SLOT( setChecked(bool) ) );

    connect( pb_ok, SIGNAL( clicked() ), SLOT( accept() ) );
    connect( pb_cancel, SIGNAL( clicked() ), SLOT( reject() ) );
    connect( pb_copyDown, SIGNAL( clicked() ), SLOT( copyDown() ) );

    sb_httpproxy->setMaxValue( MAX_PORT_VALUE );
    sb_secproxy->setMaxValue( MAX_PORT_VALUE );
    sb_ftpproxy->setMaxValue( MAX_PORT_VALUE );
    sb_gopherproxy->setMaxValue( MAX_PORT_VALUE );

    setChecked( true );
}

void KManualProxyDlg::setProxyData( const ProxyData* data )
{
    if ( data && !data->envBased )
    {
        int port;
        KURL u = data->httpProxy;
        cb_httpproxy->setChecked( !data->httpProxy.isEmpty() &&
                                  u.isValid() );
        port = u.port();
        if ( port < 1 )
            port = DEFAULT_PROXY_PORT;
        u.setPort( 0 );
        le_httpproxy->setText( u.url() );
        sb_httpproxy->setValue( port );

        u = data->secureProxy;
        cb_secproxy->setChecked( !data->secureProxy.isEmpty() &&
                                 u.isValid() );
        port = u.port();
        if ( port < 1 )
            port = DEFAULT_PROXY_PORT;
        u.setPort( 0 );
        le_secproxy->setText( u.url() );
        sb_secproxy->setValue( port );

        u = data->ftpProxy;
        cb_ftpproxy->setChecked( !data->ftpProxy.isEmpty() &&
                                 u.isValid() );
        port = u.port();
        if ( port < 1 )
            port = DEFAULT_PROXY_PORT;
        u.setPort( 0 );
        le_ftpproxy->setText( u.url() );
        sb_ftpproxy->setValue( port );

        u = data->gopherProxy;
        cb_gopherproxy->setChecked( !data->gopherProxy.isEmpty() &&
                                    u.isValid() );
        port = u.port();
        if ( port < 1 )
            port = DEFAULT_PROXY_PORT;
        u.setPort( 0 );
        le_gopherproxy->setText( u.url() );
        sb_gopherproxy->setValue( port );

        gb_exceptions->fillExceptions( data );
        d = data;
    }
    else
    {
       sb_httpproxy->setValue( DEFAULT_PROXY_PORT );
       sb_secproxy->setValue( DEFAULT_PROXY_PORT );
       sb_ftpproxy->setValue( DEFAULT_PROXY_PORT );
       sb_gopherproxy->setValue( DEFAULT_PROXY_PORT );
    }
}

ProxyData KManualProxyDlg::data() const
{
    KURL u;
    ProxyData data;
    if ( cb_httpproxy->isChecked() )
    {
        u = le_httpproxy->text();
        if ( u.isValid() )
        {
            u.setPort( sb_httpproxy->value() );
            data.httpProxy = u.url();
        }
    }
    if ( cb_secproxy->isChecked() )
    {
        u = le_secproxy->text();
        if ( u.isValid() )
        {
            u.setPort( sb_secproxy->value() );
            data.secureProxy = u.url();
        }
    }
    if ( cb_ftpproxy->isChecked() )
    {
        u = le_ftpproxy->text();
        if ( u.isValid() )
        {
            u.setPort( sb_ftpproxy->value() );
            data.ftpProxy = u.url();
        }
    }
    if ( cb_gopherproxy->isChecked() )
    {
        u = le_gopherproxy->text();
        if ( u.isValid() )
        {
            u.setPort( sb_gopherproxy->value() );
            data.gopherProxy = u.url();
        }
    }
    QStringList list = gb_exceptions->exceptions();
    if ( list.count() )
        data.noProxyFor = list;

    data.useReverseProxy = gb_exceptions->isReverseProxyChecked();
    data.changed = ( !d || (data.httpProxy != d->httpProxy ||
                    data.secureProxy != d->secureProxy ||
                    data.ftpProxy != d->ftpProxy ||
                    data.gopherProxy != d->gopherProxy ||
                    data.noProxyFor != d->noProxyFor ||
                    data.useReverseProxy != d->useReverseProxy) );
    data.envBased = false;
    return data;
}

void KManualProxyDlg::setChecked( bool )
{
   bool checked = (cb_httpproxy->isChecked() ||
                   cb_secproxy->isChecked() ||
                   cb_ftpproxy->isChecked() ||
                   cb_gopherproxy->isChecked());
    pb_copyDown->setEnabled( checked );
    pb_ok->setEnabled( checked );
}


bool KManualProxyDlg::validate()
{
    KURL u;
    QFont f;
    bool notValid = false;
    unsigned short int i = 0;
    if ( cb_httpproxy->isChecked() )
    {
        u = le_httpproxy->text();
        if ( !u.isValid() )
        {
            f = cb_httpproxy->font();
            f.setBold( true );
            cb_httpproxy->setFont( f );
            notValid |= true;
            i++;
        }
    }
    if ( cb_secproxy->isChecked() )
    {
        u = le_secproxy->text();
        if ( !u.isValid() )
        {
            f = cb_secproxy->font();
            f.setBold( true );
            cb_secproxy->setFont( f );
            notValid |= true;
            i++;
        }

    }
    if ( cb_ftpproxy->isChecked() )
    {
        u = le_ftpproxy->text();
        if ( !u.isValid() )
        {
            f = cb_ftpproxy->font();
            f.setBold( true );
            cb_ftpproxy->setFont( f );
            notValid |= true;
            i++;
        }
    }
    if ( cb_gopherproxy->isChecked() )
    {
        u = le_gopherproxy->text();
        if ( !u.isValid() )
        {
            f = cb_httpproxy->font();
            f.setBold( true );
            cb_httpproxy->setFont( f );
            notValid |= true;
            i++;
        }
    }

    if ( notValid )
    {
        QString msg;
        if ( i > 0 )
          msg = i18n("The highlighted input fields contain "
                     "invalid proxy addresses!");
        else
          msg = i18n("The highlighted input field contains "
                     "an invalid proxy address!");
        QString details = i18n("<qt>Make sure the proxy address(es) you "
                               "provided are valid. Note that you <b>must</b> "
                               "supply a fully fully qualified address such "
                               "as <b>http://192.168.20.1</b>. All addresses "
                               "specified without their protocols (eg: "
                               "\"http\" in above example) will be rejected "
                               "as invalid.</qt>");
        KMessageBox::detailedError( this, msg, details,
                                    i18n("Invalid Proxy Setup") );
    }
    return !notValid;
}

void KManualProxyDlg::copyDown()
{
    int action = -1;
    bool isHttpProxyChecked = cb_httpproxy->isChecked();
    bool isSecProxyChecked = cb_secproxy->isChecked();
    bool isFtpProxyChecked = cb_ftpproxy->isChecked();
    bool isGopherProxyChecked = cb_gopherproxy->isChecked();

    if ( isHttpProxyChecked )
        action += 4;
    else if ( isSecProxyChecked )
        action += 3;
    else if ( isFtpProxyChecked )
        action += 2;
    else if ( isGopherProxyChecked )
        action += 1;

    switch ( action )
    {
    case 3:
        if ( isSecProxyChecked )
        {
            le_secproxy->setText( le_httpproxy->text() );
            sb_secproxy->setValue( sb_httpproxy->value() );
        }
        if ( isFtpProxyChecked )
        {
            le_ftpproxy->setText( le_httpproxy->text() );
            sb_ftpproxy->setValue( sb_httpproxy->value() );
        }

        if ( isGopherProxyChecked )
        {
            le_gopherproxy->setText( le_httpproxy->text() );
            sb_gopherproxy->setValue( sb_httpproxy->value() );
        }
        break;
    case 2:
        if ( isFtpProxyChecked )
        {
            le_ftpproxy->setText( le_secproxy->text() );
            sb_ftpproxy->setValue( sb_secproxy->value() );
        }
        if ( isGopherProxyChecked )
        {
            le_gopherproxy->setText( le_secproxy->text() );
            sb_gopherproxy->setValue( sb_secproxy->value() );
        }
        break;
    case 1:
        if ( isGopherProxyChecked )
        {
            le_gopherproxy->setText( le_ftpproxy->text() );
            sb_gopherproxy->setValue( sb_ftpproxy->value() );
        }
    case 0:
    default:
        break;
    }
}

void KManualProxyDlg::accept()
{
    if ( validate() )
      QDialog::accept();
}

void KManualProxyDlg::reject()
{
    d = 0L;
    QDialog::reject();
}
