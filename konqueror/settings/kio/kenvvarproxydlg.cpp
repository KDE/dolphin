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

#include <klocale.h>
#include <kmessagebox.h>

#include "kproxydlg.h"

#define ENV_HTTP_PROXY    "HTTP_PROXY,http_proxy,HTTPPROXY,httpproxy,PROXY,\
                           proxy"
#define ENV_SECURE_PROXY  "HTTPS_PROXY,https_proxy,HTTPSPROXY,httpsproxy,\
                           PROXY,proxy"
#define ENV_FTP_PROXY     "FTP_PROXY,ftp_proxy,FTPPROXY,ftpproxy,PROXY,proxy"
#define ENV_GOPHER_PROXY  "GOPHER_PROXY,gopher_proxy,GOPHERPROXY,gopherproxy,\
                           PROXY,proxy"

class KEnvLineEdit : public KLineEdit
{
public:
  KEnvLineEdit( QWidget *parent, const char *name=0L );

protected:
  virtual void keyPressEvent( QKeyEvent * );
};

KEnvLineEdit::KEnvLineEdit( QWidget *parent, const char *name )
            :KLineEdit( parent, name )
{
    // For now do not accept any drops since they
    // might contain characters we do not accept.
    // TODO: Re-implement ::dropEvent to allow
    // acceptable formats...
    setAcceptDrops( false );
}

void KEnvLineEdit::keyPressEvent( QKeyEvent * e )
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

KEnvVarProxyDlg::KEnvVarProxyDlg( QWidget* parent, const char* name )
                :KCommonProxyDlg( parent, name, true )
{
    setCaption( i18n( "Variable Proxy Configuration" ) );
    QVBoxLayout* mainLayout = new QVBoxLayout( this );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( 2*KDialog::marginHint() );

    gb_servers = new QGroupBox( this, "gb_servers" );
    gb_servers->setSizePolicy( QSizePolicy(QSizePolicy::Expanding,
                                           QSizePolicy::Fixed,
                                           gb_servers->sizePolicy().hasHeightForWidth()) );
    gb_servers->setTitle( i18n( "Servers" ) );
    gb_servers->setColumnLayout(0, Qt::Vertical );
    gb_servers->layout()->setSpacing( 0 );
    gb_servers->layout()->setMargin( 0 );
    QGridLayout* gb_serversLayout = new QGridLayout( gb_servers->layout() );
    gb_serversLayout->setAlignment( Qt::AlignTop );
    gb_serversLayout->setSpacing( KDialog::spacingHint() );
    gb_serversLayout->setMargin( 2*KDialog::marginHint() );

    QGridLayout* glay = new QGridLayout;
    glay->setSpacing( KDialog::spacingHint() );
    glay->setMargin( 0 );

    cb_envHttp = new QCheckBox( i18n("&HTTP:"), gb_servers, "cb_envHttp" );
    QWhatsThis::add( cb_envHttp, i18n("Check this box to enable environment "
                                      "variable based proxy setup for HTTP "
                                      "connections.") );
    le_envHttp = new KEnvLineEdit( gb_servers, "le_envHttp" );
    le_envHttp->setEnabled( false );
    le_envHttp->setMinimumWidth( le_envHttp->fontMetrics().width('W') * 20 );
    le_envHttp->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                           QSizePolicy::Fixed,
                                           le_envHttp->sizePolicy().hasHeightForWidth()) );
    QWhatsThis::add( le_envHttp, i18n("<qt>Enter the name of the environment "
                                      "variable, eg. <tt>HTTP_PROXY</tt>, "
                                      "used to store the address of the HTTP "
                                      "proxy server.<p>Alternatively, you can "
                                      "click on the <tt>\"Auto Detect\"</tt> "
                                      "button to attempt automatic discovery "
                                      "of this variable.</qt>") );
    glay->addWidget( cb_envHttp, 0, 0 );
    glay->addWidget( le_envHttp, 0, 1 );

    cb_envSecure = new QCheckBox( i18n("HTTP&S:"), gb_servers, "cb_envSecure" );
    QWhatsThis::add( cb_envSecure, i18n("Check this box to enable environment "
                                        "variable based proxy setup for secure "
                                        "connections such as HTTPS.") );
    le_envSecure = new KEnvLineEdit( gb_servers, "le_envSecure" );
    le_envSecure->setEnabled( false );
    le_envSecure->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                            QSizePolicy::Fixed,
                                            le_envSecure->sizePolicy().hasHeightForWidth()) );
    QWhatsThis::add( le_envSecure, i18n("<qt>Enter the name of the environment "
                                        "variable, eg. <tt>HTTPS_PROXY"
                                        "</tt>, used to store the address of "
                                        "the secure proxy server.<p>"
                                        "Alternatively, you can click on the "
                                        "<tt>\"Auto Detect\"</tt> button "
                                        "to attempt an automatic discovery of "
                                        "this variable(s).</qt>") );

    glay->addWidget( cb_envSecure, 1, 0 );
    glay->addWidget( le_envSecure, 1, 1 );

    cb_envFtp = new QCheckBox( i18n("&FTP:"), gb_servers, "cb_envFtp" );
    QWhatsThis::add( cb_envFtp, i18n("Check this box to enable environment "
                                     "variable based proxy setup for FTP "
                                     "connections.") );

    le_envFtp = new KEnvLineEdit( gb_servers, "le_envFtp" );
    le_envFtp->setEnabled( false );
    le_envFtp->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                          QSizePolicy::Fixed,
                                          le_envFtp->sizePolicy().hasHeightForWidth()) );
    QWhatsThis::add( le_envFtp, i18n("<qt>Enter the name of the environment "
                                     "variable, eg. <tt>FTP_PROXY</tt>, "
                                     "used to store the address of the FTP "
                                     "proxy server.<p>Alternatively, you can "
                                     "click on the <tt>\"Auto Detect\"</tt> "
                                     "button to attempt an automatic discovery "
                                     "of this variable.</qt>") );
    glay->addWidget( cb_envFtp, 2, 0 );
    glay->addWidget( le_envFtp, 2, 1 );


    cb_envGopher = new QCheckBox( gb_servers, "cb_envGopher" );
    cb_envGopher->setText( i18n( "&Gopher:" ) );
    QWhatsThis::add( cb_envGopher, i18n("Check this box to enable environment "
                                        "variable based proxy setup for "
                                        "Gopher connections.") );

    le_envGopher = new KEnvLineEdit( gb_servers, "le_envGopher" );
    le_envGopher->setEnabled( false );
    le_envGopher->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                             QSizePolicy::Fixed,
                                             le_envGopher->sizePolicy().hasHeightForWidth()) );
    QWhatsThis::add( le_envGopher, i18n("<qt>Enter the name of the environment "
                                        "variable, eg. <tt>GOPHER_PROXY"
                                        "</tt>, used to store the address "
                                        "of the gopher proxy server.<p>"
                                        "Alternatively, you can click on the "
                                        "<tt>\"Auto Detect\"</tt> "
                                        "button on the right to attempt an "
                                        "automatic discovery of these "
                                        "varibles.</qt>") );

    glay->addWidget( cb_envGopher, 3, 0 );
    glay->addWidget( le_envGopher, 3, 1 );
    gb_serversLayout->addLayout( glay, 0, 0 );

    QHBoxLayout* hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );
    QSpacerItem* spacer = new QSpacerItem( 75, 16,
                              QSizePolicy::Fixed, QSizePolicy::Minimum );
    hlay->addItem( spacer );

    cb_showValue = new QCheckBox( i18n("Show Actual &Values"), gb_servers,
                                  "cb_showValue" );
    hlay->addWidget( cb_showValue );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::MinimumExpanding,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    gb_serversLayout->addLayout( hlay, 1, 0 );

    QVBoxLayout* vlay = new QVBoxLayout;
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( 0 );

    pb_verify = new QPushButton( i18n("&Verify"), gb_servers, "pb_verify" );
    QWhatsThis::add( pb_verify, i18n("<qt>Click this button to quickly "
                                     "determine whether or not the environment "
                                     "variable names you supplied are valid.  "
                                     "If an environment variable is not found "
                                     "the associted labels will be "
                                     "<b>highlighted</b> to indicate the "
                                     "invalid settings.</qt>") );
    vlay->addWidget( pb_verify );

    pb_detect = new QPushButton( i18n("Auto &Detect"), gb_servers,
                                 "pb_detect" );
    QWhatsThis::add( pb_detect, i18n("<qt>Click on this button to attempt "
                                     "automatic discovery of the environment "
                                     "variables used for setting system wide "
                                     "proxy information.<p> This automatic "
                                     "lookup works by searching for the "
                                     "following common variable names:</p>"
                                     "<p><u>For Http: </u>%1</p>"
                                     "<p><u>For Secure: </u>%2</p>"
                                     "<p><u>For FTP: </u>%3</p>"
                                     "<p><u>For Gopher: </u>%4</p></qt>").arg(ENV_HTTP_PROXY).arg(ENV_SECURE_PROXY).arg(ENV_FTP_PROXY).arg(ENV_GOPHER_PROXY) );
    vlay->addWidget( pb_detect );

    spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                              QSizePolicy::MinimumExpanding );
    vlay->addItem( spacer );

    gb_serversLayout->addLayout( vlay, 0, 1 );
    mainLayout->addWidget( gb_servers );

    gb_exceptions = new KExceptionBox( this, "gb_exceptions" );
    gb_exceptions->setSizePolicy( QSizePolicy(QSizePolicy::Expanding,
                                              QSizePolicy::Preferred,
                                              gb_exceptions->sizePolicy().hasHeightForWidth()) );
    mainLayout->addWidget( gb_exceptions );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    spacer = new QSpacerItem( 20, 20, QSizePolicy::MinimumExpanding,
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

void KEnvVarProxyDlg::init()
{
    d = 0L;
    connect( cb_envHttp, SIGNAL( toggled(bool) ), le_envHttp,
             SLOT( setEnabled(bool) ) );
    connect( cb_envSecure, SIGNAL( toggled(bool) ), le_envSecure,
             SLOT( setEnabled(bool) ) );
    connect( cb_envFtp, SIGNAL( toggled(bool) ), le_envFtp,
             SLOT( setEnabled(bool) ) );
    connect( cb_envGopher, SIGNAL( toggled(bool) ), le_envGopher,
             SLOT( setEnabled(bool) ) );

    connect( cb_envHttp, SIGNAL( toggled(bool) ), SLOT( setChecked(bool) ) );
    connect( cb_envSecure, SIGNAL( toggled(bool) ), SLOT( setChecked(bool) ) );
    connect( cb_envFtp, SIGNAL( toggled(bool) ), SLOT( setChecked(bool) ) );
    connect( cb_envGopher, SIGNAL( toggled(bool) ), SLOT( setChecked(bool) ) );
    connect( cb_showValue, SIGNAL( toggled(bool) ), SLOT( showValue(bool) ) );    


    connect( pb_ok, SIGNAL( clicked() ), SLOT( accept() ) );
    connect( pb_cancel, SIGNAL( clicked() ), SLOT( reject() ) );
    connect( pb_verify, SIGNAL( clicked() ), SLOT( verifyPressed() ) );
    connect( pb_detect, SIGNAL( clicked() ), SLOT( autoDetectPressed() ) );

    setChecked( true );
}

void KEnvVarProxyDlg::setProxyData( const ProxyData* data )
{
    if ( data && data->envBased )
    {
        if ( !data->httpProxy.isEmpty() )
        {
            cb_envHttp->setChecked( true );
            le_envHttp->setText( data->httpProxy );
        }

        if ( !data->secureProxy.isEmpty() )
        {
            cb_envSecure->setChecked( true );
            le_envSecure->setText( data->secureProxy );
        }

        if ( !data->ftpProxy.isEmpty() )
        {
            cb_envFtp->setChecked( true );
            le_envFtp->setText( data->ftpProxy );
        }

        if ( !data->gopherProxy.isEmpty() )
        {
            cb_envGopher->setChecked( true );
            le_envGopher->setText( data->gopherProxy );
        }

        gb_exceptions->fillExceptions( data );
        d = data;
    }
}

ProxyData KEnvVarProxyDlg::data() const
{
    ProxyData data;
    if ( cb_envHttp->isChecked() )
        data.httpProxy = le_envHttp->text();
    if ( cb_envSecure->isChecked() )
        data.secureProxy = le_envSecure->text();
    if ( cb_envFtp->isChecked() )
        data.ftpProxy = le_envFtp->text();
    if ( cb_envGopher->isChecked() )
        data.gopherProxy = le_envGopher->text();

    QStringList list = gb_exceptions->exceptions();
    if ( list.count() )
        data.noProxyFor = list;

    data.useReverseProxy = gb_exceptions->isReverseProxyChecked();
    data.changed = ( !d || (data.httpProxy != d->httpProxy ||
                    data.secureProxy != d->secureProxy ||
                    data.ftpProxy != d->ftpProxy ||
                    data.gopherProxy != d->gopherProxy ||
                    data.noProxyFor != d->noProxyFor ||
                    data.useReverseProxy != d->useReverseProxy ) );
    data.envBased = true;
    return data;
}


void KEnvVarProxyDlg::verifyPressed()
{
    unsigned short i;
    if ( !validate(i) )
    {
        QString msg;
        if ( i > 0 )
            msg = i18n("The highlighted input fields contain unknown or "
                       "invalid \nenvironment variable names.");
        else
            msg = i18n("The highlighted input field contains unknown or "
                       "invalid \nenvironment variable name.");
        QString details = i18n("<qt>Make sure you entered the actual "
                               "environment variable name rather than the "
                               "address of the proxy server.  For example, "
                               "if the set variable name used to specify the "
                               "http proxy server is <b>HTTP_PROXY</b>, then "
                               "you need to enter <b>HTTP_PROXY</b> instead "
                               "of the actual address specified by the "
                               "variable.</qt>");
        KMessageBox::detailedSorry( this, msg, details,
                                    i18n("Invalid Proxy Setup") );
    }
    else
    {
        KMessageBox::information( this, i18n("Environment variable(s) "
                                             "successfully verified!"),
                                        i18n("Proxy Setup") );
    }
}

void KEnvVarProxyDlg::autoDetectPressed()
{
    bool found = false;
    QString env;
    QStringList list;
    QStringList::ConstIterator it;
    if ( cb_envHttp->isChecked() )
    {
        list = QStringList::split( ',', QString::fromLatin1(ENV_HTTP_PROXY) );
        it = list.begin();
        for( ; it != list.end(); ++it )
        {
            env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
            if (  !env.isEmpty() )
            {
                le_envHttp->setText( (cb_showValue->isChecked()?env:*it) );
                found |= true;
                break;
            }
        }
    }
    if ( cb_envSecure->isChecked() )
    {
        list = QStringList::split( ',',
                                   QString::fromLatin1(ENV_SECURE_PROXY) );
        it = list.begin();
        for( ; it != list.end(); ++it )
        {
            env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
            if ( !env.isEmpty() )
            {
                le_envSecure->setText( (cb_showValue->isChecked()?env:*it) );
                found |= true;
                break;
            }
        }
    }
    if ( cb_envFtp->isChecked() )
    {
        list = QStringList::split( ',', QString::fromLatin1(ENV_FTP_PROXY) );
        it = list.begin();
        for( ; it != list.end(); ++it )
        {
            env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
            if ( !env.isEmpty() )
            {
                le_envFtp->setText( (cb_showValue->isChecked()?env:*it) );
                found |= true;
                break;
            }
        }
    }
    if ( cb_envGopher->isChecked() )
    {
        list = QStringList::split( ',',
                                   QString::fromLatin1(ENV_GOPHER_PROXY) );
        it = list.begin();
        for( ; it != list.end(); ++it )
        {
            env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
            if ( !env.isEmpty() )
            {
                le_envGopher->setText( (cb_showValue->isChecked()?env:*it) );
                found |= true;
                break;
            }
        }
    }

    if ( !found )
    {
        QString msg = i18n("Could not find any of the commonly used\n"
                           "environment vairbale names for setting\n"
                           "system wide proxy information!");
        QString details = i18n("<qt>To see the variable names used in the "
                               "automatic detection process, close this "
                               "dialog box, click on the quick help button "
                               "(<b>?</b>) on the top right corner of the "
                               "\"Variable Proxy Configuration\" dialog and "
                               "then click on the Auto Detect button.</qt> ");
        KMessageBox::detailedSorry( this, msg, details,
                                    i18n("Result of Auto Proxy Detect") );
    }
}

void KEnvVarProxyDlg::showValue( bool enable )
{
    if ( enable )
    {
        QString txt, env;
        lst_envVars.clear();
        if ( cb_envHttp->isChecked() )
        {
            txt = le_envHttp->text();
            env = QString::fromLocal8Bit( getenv( txt.local8Bit() ) );
            le_envHttp->setText( env );
            lst_envVars << txt;
        }
        if ( cb_envSecure->isChecked() )
        {
            txt = le_envSecure->text();
            env = QString::fromLocal8Bit( getenv( txt.local8Bit() ) );
            le_envSecure->setText( env );
            lst_envVars << txt;
        }
        if ( cb_envFtp->isChecked() )
        {
            txt = le_envFtp->text();
            env = QString::fromLocal8Bit( getenv( txt.local8Bit() ) );
            le_envFtp->setText( env );
            lst_envVars << txt;
        }
        if ( cb_envGopher->isChecked() )
        {
            txt = le_envGopher->text();
            env = QString::fromLocal8Bit( getenv( txt.local8Bit() ) );
            le_envGopher->setText( env );
            lst_envVars << txt;
        }
    }
    else
    {
        int count = 0;
        if ( cb_envHttp->isChecked() )
            le_envHttp->setText( lst_envVars[count++] );
        if ( cb_envSecure->isChecked() )
            le_envSecure->setText( lst_envVars[count++] );
        if ( cb_envFtp->isChecked() )
            le_envFtp->setText( lst_envVars[count++] );
        if ( cb_envGopher->isChecked() )
            le_envGopher->setText( lst_envVars[count++] );
    }
}

void KEnvVarProxyDlg::setChecked( bool )
{
    bool checked = (cb_envHttp->isChecked() ||
                    cb_envSecure->isChecked() ||
                    cb_envFtp->isChecked() ||
                    cb_envGopher->isChecked());
    pb_verify->setEnabled( checked );
    pb_detect->setEnabled( checked );
    pb_ok->setEnabled( checked );
}

bool KEnvVarProxyDlg::validate( unsigned short& i )
{
    QFont f;
    bool notValid = false;
    i = 0;
    if ( cb_envHttp->isChecked() )
    {
        if ( getenv( le_envHttp->text().local8Bit() ) == 0L )
        {
            f = cb_envHttp->font();
            f.setBold( true );
            cb_envHttp->setFont( f );
            notValid |= true;
            i++;
        }

    }
    if ( cb_envSecure->isChecked() )
    {
        if ( getenv( le_envSecure->text().local8Bit() ) == 0L )
        {
            f = cb_envSecure->font();
            f.setBold( true );
            cb_envSecure->setFont( f );
            notValid |= true;
            i++;
        }

    }
    if ( cb_envFtp->isChecked() )
    {
        if ( getenv( le_envFtp->text().local8Bit() ) == 0L )
        {
            f = cb_envFtp->font();
            f.setBold( true );
            cb_envFtp->setFont( f );
            notValid |= true;
            i++;
        }
    }
    if ( cb_envGopher->isChecked() )
    {
        if ( getenv( le_envGopher->text().local8Bit() ) == 0L )
        {
            f = cb_envGopher->font();
            f.setBold( true );
            cb_envGopher->setFont( f );
            notValid |= true;
            i++;
        }
    }
    return !notValid;
}

void KEnvVarProxyDlg::reject()
{
    d = 0L;
    QDialog::reject();
}

void KEnvVarProxyDlg::accept()
{
    unsigned short i;
    if ( !validate( i ) )
    {
        QString msg;
        if ( i > 0 )
            msg = i18n("The highlighted input fields contain unknown or "
                       "invalid \nenvironment variables!");
        else
            msg = i18n("The highlighted input field contains an unknown or "
                       "invalid \nenvironment variable!");
        QString details = i18n("<qt>Make sure you entered the actual "
                               "environment variable name rather than the "
                               "adrdress of the proxy server.  For example, "
                               "if the set variable name used to specify the "
                               "http proxy server is <b>HTTP_PROXY</b>, then "
                               "you need to enter <b>HTTP_PROXY</b> instead "
                               "of the actual address specified by the "
                               "variable.</qt>");
        KMessageBox::detailedError( this, msg, details,
                                    i18n("Invalid Proxy Setup") );
        return;
    }
    QDialog::accept();
}
