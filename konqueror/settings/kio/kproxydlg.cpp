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
#include <qradiobutton.h>

#include <kdebug.h>
#include <klocale.h>
#include <klistview.h>
#include <dcopclient.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <ksaveioconfig.h>

#include "kproxydlg.h"
#include "kproxydlg.moc"

KExceptionBox::KExceptionBox( QWidget* parent, const char* name )
              :QGroupBox( parent, name )
{
    setTitle( i18n("Exceptions") );
    QVBoxLayout* mainLayout = new QVBoxLayout( this,
                                               KDialog::marginHint(),
                                               KDialog::spacingHint() );
    mainLayout->setAlignment( Qt::AlignTop );

    QHBoxLayout* hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( KDialog::marginHint() );

    cb_reverseproxy = new QCheckBox( i18n("Only use proxy for entries "
                                          "in this list"), this,
                                          "cb_reverseproxy" );
    QWhatsThis::add( cb_reverseproxy, i18n("<qt>Check this box to reverse the "
                                           "use of the exception list. That "
                                           "is checking this box will result "
                                           "in the proxy servers being used "
                                           "only when the requested URL matches "
                                           "one of the addresses listed here."
                                           "<p>This feature is useful if all "
                                           "you want or need is to use a proxy "
                                           "server  for a few specific sites.<p>"
                                           "If you have more complex "
                                           "requirements you might want to use "
                                           "a configuration script</qt>") );
    hlay->addWidget( cb_reverseproxy );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    mainLayout->addLayout( hlay );

    QGridLayout* glay = new QGridLayout;
    glay->setSpacing( KDialog::spacingHint() );
    glay->setMargin( 0 );

    QVBoxLayout* vlay = new QVBoxLayout;
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( 0 );

    pb_new = new QPushButton( i18n("&New..."), this, "pb_new" );
    connect( pb_new, SIGNAL( clicked() ), SLOT( newPressed() ) );
    QWhatsThis::add( pb_new, i18n("Click this to add an address that should "
                                  "be exempt from using or forced to use, "
                                  "depending on the check box above, a proxy "
                                  "server.") );
    pb_change = new QPushButton( i18n("C&hange..."), this,
                                  "pb_change" );
    connect( pb_change, SIGNAL( clicked() ), SLOT( changePressed() ) );
    pb_change->setEnabled( false );
    QWhatsThis::add( pb_change, i18n("Click this button to change the "
                                     "selected exception address.") );
    pb_delete = new QPushButton( i18n("De&lete"), this,
                                  "pb_delete" );
    connect( pb_delete, SIGNAL( clicked() ), SLOT( deletePressed() ) );
    pb_delete->setEnabled( false );
    QWhatsThis::add( pb_delete, i18n("Click this button to delete the "
                                     "selected address.") );
    pb_deleteAll = new QPushButton( i18n("D&elete All"), this,
                                     "pb_deleteAll" );
    connect( pb_deleteAll, SIGNAL( clicked() ), SLOT( deleteAllPressed() ) );
    pb_deleteAll->setEnabled( false );
    QWhatsThis::add( pb_deleteAll, i18n("Click this button to delete all "
                                     "the address in the exception list.") );
    vlay->addWidget( pb_new );
    vlay->addWidget( pb_change );
    vlay->addWidget( pb_delete );
    vlay->addWidget( pb_deleteAll );

    glay->addLayout( vlay, 0, 1 );

    lv_exceptions = new KListView( this, "lv_exceptions" );
    connect( lv_exceptions, SIGNAL(selectionChanged()),
             SLOT(updateButtons()) );
    lv_exceptions->addColumn( i18n("Address") );
    QWhatsThis::add( lv_exceptions, i18n("<qt>Contains a list of addresses "
                                          "that should either by-pass the use "
                                          "of the proxy server(s) specified "
                                          "above or use these servers based "
                                          "on the state of the <tt>\"Only "
                                          "use proxy for entries in the "
                                          "list\"</tt> checkbox above.<p>"
                                          "If the box is checked, then only "
                                          "URLs that match the addresses "
                                          "listed here will be sent through "
                                          "the proxy server(s) shown above. "
                                          "Otherwise, the proxy servers are "
                                          "bypassed for this list.</qt>") );

    glay->addMultiCellWidget( lv_exceptions, 0, 1, 0, 0 );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                              QSizePolicy::MinimumExpanding );
    glay->addItem( spacer, 1, 1 );
    mainLayout->addLayout( glay );
}

bool KExceptionBox::handleDuplicate( const QString& site )
{
    QListViewItem* item = lv_exceptions->firstChild();
    while ( item != 0 )
    {
        if ( item->text(0).findRev( site ) != -1 &&
             item != lv_exceptions->currentItem() )
        {
            QString msg = i18n("<qt><center><b>%1</b><br/>"
                               "already exists!").arg(site);
            KMessageBox::error( this, msg, i18n("Duplicate Exception") );
            return true;
        }
        item = item->nextSibling();
    }
    return false;
}

void KExceptionBox::newPressed()
{
    KProxyExceptionDlg* dlg = new KProxyExceptionDlg( this );
    dlg->setCaption( i18n("New Exception") );
    if ( dlg->exec() == QDialog::Accepted )
    {
        QString exception = dlg->exception();
        if ( !handleDuplicate( exception ) )
        {
            QListViewItem* index = new QListViewItem( lv_exceptions,
                                                      exception );
            lv_exceptions->setCurrentItem( index );
        }
    }
    delete dlg;
}

void KExceptionBox::changePressed()
{
    KProxyExceptionDlg* dlg = new KProxyExceptionDlg( this );
    dlg->setCaption( i18n("Change Exception") );
    QString currentItem = lv_exceptions->currentItem()->text(0);
    dlg->setException( currentItem );
    if ( dlg->exec() == QDialog::Accepted )
    {
        QString exception = dlg->exception();
        if ( !handleDuplicate( exception ) )
        {
            QListViewItem* index = lv_exceptions->currentItem();
            index->setText( 0, exception );
            lv_exceptions->setCurrentItem( index );
        }
    }
    delete dlg;
}

void KExceptionBox::deletePressed()
{
    QListViewItem* item = lv_exceptions->selectedItem()->itemBelow();
    if ( !item )
        item = lv_exceptions->selectedItem()->itemAbove();
    delete lv_exceptions->selectedItem();
    if ( item )
        lv_exceptions->setSelected( item, true );
    updateButtons();
}

void KExceptionBox::deleteAllPressed()
{
    lv_exceptions->clear();
    updateButtons();
}

QStringList KExceptionBox::exceptions() const
{
    QStringList list;
    if ( lv_exceptions->childCount() )
    {
        QListViewItem* item = lv_exceptions->firstChild();
        for( ; item != 0L; item = item->nextSibling() )
            list << item->text(0);
    }
    return list;
}

void KExceptionBox::fillExceptions( const ProxyData* data )
{
    if ( data )
    {
        cb_reverseproxy->setChecked( data->useReverseProxy );
        if ( !data->noProxyFor.isEmpty() )
        {
            QStringList::ConstIterator it = data->noProxyFor.begin();
            for( ; it != data->noProxyFor.end(); ++it )
                (void) new QListViewItem( lv_exceptions, (*it) );
        }
    }
}

bool KExceptionBox::isReverseProxyChecked() const
{
    return cb_reverseproxy->isChecked();
}

void KExceptionBox::updateButtons()
{
    bool hasItems = lv_exceptions->childCount() > 0;
    bool itemSelected = ( hasItems && lv_exceptions->selectedItem()!=0 );
    pb_delete->setEnabled( itemSelected );
    pb_deleteAll->setEnabled( hasItems );
    pb_change->setEnabled( itemSelected );
}


KProxyDialog::KProxyDialog( QWidget* parent,  const char* name )
             :KCModule( parent, name )
{
    QVBoxLayout* mainLayout = new QVBoxLayout( this,
                                               KDialog::marginHint(),
                                               KDialog::spacingHint() );
    QHBoxLayout* hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    cb_useProxy = new QCheckBox( i18n("Use &Proxy"), this, "cb_useProxy" );
    cb_useProxy->setSizePolicy( QSizePolicy( QSizePolicy::Fixed,
                                QSizePolicy::Fixed,
                                cb_useProxy->sizePolicy().hasHeightForWidth()));
    QWhatsThis::add( cb_useProxy, i18n("<qt>Check this box to enable the use "
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
    hlay->addWidget( cb_useProxy );
    QSpacerItem* spacer = new QSpacerItem( 20, 20,
                                           QSizePolicy::MinimumExpanding,
                                           QSizePolicy::Minimum );
    hlay->addItem( spacer );
    mainLayout->addLayout( hlay );

    gb_configure = new QButtonGroup( i18n("Configuration"), this,
                                     "gb_configure" );
    gb_configure->setEnabled( false );
    QWhatsThis::add( gb_configure, i18n("Options for setting up connections "
                                        "to your proxy server(s)") );
    gb_configure->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                             QSizePolicy::Fixed,
                                             gb_configure->sizePolicy().hasHeightForWidth()) );

    gb_configure->setColumnLayout(0, Qt::Vertical );
    gb_configure->layout()->setSpacing( 0 );
    gb_configure->layout()->setMargin( 0 );
    QVBoxLayout* vlay = new QVBoxLayout( gb_configure->layout() );
    vlay->setMargin( 2*KDialog::marginHint() );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setAlignment( Qt::AlignTop );

    hlay = new QHBoxLayout;
    hlay->setSpacing(KDialog::spacingHint());
    hlay->setMargin(0);

    rb_autoDiscover = new QRadioButton( i18n("A&utomatically detected script "
                                        "file"), gb_configure,
                                        "rb_autoDiscover" );
    rb_autoDiscover->setChecked( true );
    QWhatsThis::add( rb_autoDiscover, i18n("<qt> Select this option if you want "
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
    hlay->addWidget( rb_autoDiscover );
    spacer = new QSpacerItem( 210, 16, QSizePolicy::Fixed,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    vlay->addLayout( hlay );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    rb_autoScript = new QRadioButton( i18n("Specified &script file"),
                                      gb_configure, "rb_autoScript" );
    QWhatsThis::add( rb_autoScript, i18n("Select this option if your proxy "
                                         "support is provided through a script "
                                         "file located at a specific address. "
                                         "You can then enter the address of "
                                         "the location below.") );
    hlay->addWidget( rb_autoScript );
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

    QLabel* label = new QLabel( i18n("&Location:") , gb_configure,
                                "lbl_location" );
    label->setEnabled( false );
    connect( rb_autoScript, SIGNAL( toggled(bool) ), label,
             SLOT( setEnabled(bool) ) );
    label->setEnabled( false );
    hlay->addWidget( label );

    ur_location = new KURLRequester( gb_configure, "ur_location" );
    ur_location->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                            QSizePolicy::Fixed,
                                            ur_location->sizePolicy().hasHeightForWidth()) );
    ur_location->setEnabled( false );
    ur_location->setFocusPolicy( KURLRequester::StrongFocus );
    label->setBuddy( ur_location );
    hlay->addWidget( ur_location );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed );
    hlay->addItem( spacer );
    vlay->addLayout( hlay );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    rb_envVar = new QRadioButton( i18n("Preset environment &variables"),
                                  gb_configure, "rb_envVar" );
    QWhatsThis::add( rb_envVar, i18n("<qt>Some systems are setup with "
                                     "environment variables such as "
                                     "<b>$HTTP_PROXY</b> to allow graphical "
                                     "as well as non-graphical application "
                                     "to share the same proxy configuration "
                                     "information.<p>Select this option and "
                                     "click on the <i>Setup...</i> button on "
                                     "the right side to provide the environment "
                                     "variable names used to set the address "
                                     "of the proxy server(s).</qt>") );
    hlay->addWidget( rb_envVar );
    spacer = new QSpacerItem( 45, 20, QSizePolicy::Fixed,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );

    pb_envSetup = new QPushButton( i18n("Setup..."), gb_configure,
                                   "pb_envSetup" );
    pb_envSetup->setSizePolicy( QSizePolicy(QSizePolicy::Fixed,
                                             QSizePolicy::Fixed,
                                             pb_envSetup->sizePolicy().hasHeightForWidth()) );
    pb_envSetup->setEnabled( false );
    hlay->addWidget( pb_envSetup );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    vlay->addLayout( hlay );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    rb_manual = new QRadioButton( i18n("&Manually specified settings"),
                                  gb_configure, "rb_manual" );
    QWhatsThis::add( rb_manual, i18n("<qt>Select this option and click on "
                                     "the <i>Setup...</i> button on the "
                                     "right side to manually setup the "
                                     "location of the proxy servers to be "
                                     "used.</qt>") );
    hlay->addWidget( rb_manual );
    spacer = new QSpacerItem( 40, 20, QSizePolicy::Fixed,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );

    pb_manSetup = new QPushButton( i18n("Setup..."), gb_configure,
                                   "pb_manSetup" );
    pb_manSetup->setSizePolicy( QSizePolicy(QSizePolicy::Fixed,
                                             QSizePolicy::Fixed,
                                             pb_manSetup->sizePolicy().hasHeightForWidth()) );
    pb_manSetup->setEnabled( false );
    hlay->addWidget( pb_manSetup );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    vlay->addLayout( hlay );
    mainLayout->addWidget( gb_configure );

    gb_auth = new QButtonGroup( i18n("Authorization"), this, "gb_auth" );
    gb_auth->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding,
                                        QSizePolicy::Fixed,
                                        gb_auth->sizePolicy().hasHeightForWidth()) );

    gb_auth->setEnabled( false );
    QWhatsThis::add( gb_auth, i18n("Setup the way authorization information "
                                   "should be handled when making proxy "
                                   "connections. The default option is to "
                                   "prompt you for password as needed.") );
    gb_auth->setColumnLayout(0, Qt::Vertical );
    gb_auth->layout()->setSpacing( 0 );
    gb_auth->layout()->setMargin( 0 );
    QVBoxLayout* gb_authLayout = new QVBoxLayout( gb_auth->layout() );
    gb_authLayout->setMargin( 2*KDialog::marginHint() );
    gb_authLayout->setSpacing( KDialog::spacingHint() );
    gb_authLayout->setAlignment( Qt::AlignTop );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    rb_prompt = new QRadioButton( i18n("Prompt as &needed"), gb_auth,
                                  "rb_prompt" );
    rb_prompt->setChecked( true );
    QWhatsThis::add( rb_prompt, i18n("Select this option if you want to "
                                     "be prompted for the login information "
                                     "as needed. This is default behavior.") );
    hlay->addWidget( rb_prompt );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                              QSizePolicy::Fixed );
    hlay->addItem( spacer );
    gb_authLayout->addLayout( hlay );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    rb_autoLogin = new QRadioButton( i18n("Use automatic lo&gin"), gb_auth,
                                     "rb_autoLogin" );
    QWhatsThis::add( rb_autoLogin, i18n("Select this option if you have "
                                        "already setup a login entry for "
                                        "your proxy server(s) in <tt>"
                                        "$KDEHOME/share/config/kionetrc"
                                        "</tt> file.") );
    hlay->addWidget( rb_autoLogin );
    gb_authLayout->addLayout( hlay );
    mainLayout->addWidget( gb_auth );

    spacer = new QSpacerItem( 1, 1, QSizePolicy::Expanding,
                              QSizePolicy::Minimum );
    mainLayout->addItem( spacer );

    // signals and slots connections
    connect( cb_useProxy, SIGNAL( toggled(bool) ),
             SLOT( useProxyChecked(bool) ) );

    connect( rb_autoDiscover, SIGNAL( toggled(bool) ),
             SLOT( autoDiscoverChecked( bool ) ) );
    connect( rb_autoScript, SIGNAL( toggled(bool) ),
             SLOT( autoScriptChecked( bool ) ) );
    connect( rb_manual, SIGNAL( toggled(bool) ),
             SLOT( manualChecked( bool ) ) );
    connect( rb_envVar, SIGNAL( toggled(bool) ),
             SLOT( envVarChecked( bool ) ) );

    connect( rb_prompt, SIGNAL( toggled(bool) ),
             SLOT( promptChecked( bool ) ) );
    connect( rb_autoLogin, SIGNAL( toggled(bool) ),
             SLOT( autoChecked( bool ) ) );

    connect( ur_location, SIGNAL( textChanged(const QString&) ),
             SLOT( autoScriptChanged(const QString&) ) );

    connect( pb_envSetup, SIGNAL( clicked() ), SLOT( setupManProxy() ) );
    connect( pb_manSetup, SIGNAL( clicked() ), SLOT( setupManProxy() ) );

    d = new ProxyData;
    m_btnId = -1;
    m_bSettingChanged = false;

    load();
}

KProxyDialog::~KProxyDialog()
{
    delete d;
}

void KProxyDialog::load()
{
    // Read stored settings
    useProxy = d->useProxy = KProtocolManager::useProxy();
    d->useReverseProxy = KProtocolManager::useReverseProxy();
    d->httpProxy = KProtocolManager::proxyFor( "http" );
    d->secureProxy = KProtocolManager::proxyFor( "https" );
    d->ftpProxy = KProtocolManager::proxyFor( "ftp" );
    d->gopherProxy = KProtocolManager::proxyFor( "gopher" );
    d->scriptProxy = KProtocolManager::proxyConfigScript();
    d->noProxyFor = QStringList::split( QRegExp("[',''\t'' ']"),
                                        KProtocolManager::noProxyFor() );

    cb_useProxy->setChecked( useProxy );
    gb_configure->setEnabled( useProxy );
    gb_auth->setEnabled( useProxy );

    if ( !d->scriptProxy.isEmpty() )
        ur_location->lineEdit()->setText( d->scriptProxy );
    switch ( KProtocolManager::proxyType() )
    {
        case KProtocolManager::WPADProxy:
          rb_autoDiscover->setChecked( true );
          m_btnId = 0;
          break;
        case KProtocolManager::PACProxy:
          rb_autoScript->setChecked( true );
          m_btnId = 1;
          break;
        case KProtocolManager::ManualProxy:
          rb_manual->setChecked( true );
          m_btnId = 2;
          break;
        case KProtocolManager::EnvVarProxy:
          rb_envVar->setChecked( true );
          d->envBased = true;
          m_btnId = 3;
          break;
        default:
          break;
    }

    switch( KProtocolManager::proxyAuthMode() )
    {
        case KProtocolManager::Prompt:
            rb_prompt->setChecked( true );
            m_btnId = 4;
            break;
        case KProtocolManager::Automatic:
            rb_autoLogin->setChecked( true );
            m_btnId = 5;
        default:
            break;
    }
}

void KProxyDialog::save()
{
    if ( !m_bSettingChanged )
    {
        m_bSettingChanged = false;
        return;   // Do not waste my time!
    }

    if ( cb_useProxy->isChecked() )
    {
        if ( rb_autoDiscover->isChecked() )
        {
            d->reset();
            KSaveIOConfig::setProxyType( KProtocolManager::WPADProxy );
        }
        else if ( rb_autoScript->isChecked() )
        {
            d->reset();
            KURL u = ur_location->lineEdit()->text();
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
                d->scriptProxy = u.url();
            }
        }
        else if ( rb_manual->isChecked() )
        {
            if ( !d->changed )
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
        else if ( rb_envVar->isChecked() )
        {
            if ( !d->changed )
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

        if ( rb_prompt->isChecked() )
            KSaveIOConfig::setProxyAuthMode( KProtocolManager::Prompt );
        else if ( rb_autoLogin->isChecked() )
            KSaveIOConfig::setProxyAuthMode( KProtocolManager::Automatic );
    }
    else
    {
        d->reset();
        KSaveIOConfig::setProxyType( KProtocolManager::NoProxy );
        // KSaveIOConfig::setProxyAuthMode( KProtocolManager::Prompt );
    }

    // Save the common proxy setting...
    KSaveIOConfig::setProxyFor( "http", d->httpProxy );
    KSaveIOConfig::setProxyFor( "https", d->secureProxy );
    KSaveIOConfig::setProxyFor( "ftp", d->ftpProxy );
    KSaveIOConfig::setProxyFor( "gopher", d->gopherProxy );
    KSaveIOConfig::setNoProxyFor( d->noProxyFor.join(",") );
    KSaveIOConfig::setProxyConfigScript( d->scriptProxy );
    KSaveIOConfig::setUseReverseProxy( d->useReverseProxy );

    // Update all running and applicable io-slaves
    QByteArray data;
    QDataStream stream( data, IO_WriteOnly );
    stream << "http" << "https" << "ftp" << "gopher";
    if ( !kapp->dcopClient()->isAttached() )
      kapp->dcopClient()->attach();
    kapp->dcopClient()->send( "*", "KIO::Scheduler",
                              "reparseSlaveConfiguration(QString)", data );
}

void KProxyDialog::defaults()
{
    d->reset();
    cb_useProxy->setChecked( false );
    changed( true );
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
    KCommonProxyDlg* dlg;
    if ( rb_envVar->isChecked() )
        dlg = new KEnvVarProxyDlg( this );
    else
        dlg = new KManualProxyDlg( this );

    dlg->setProxyData( d );
    if ( dlg->exec() == QDialog::Accepted )
    {
        ProxyData data = dlg->data();
        if ( data.changed )
        {
            d->reset();
            d->useReverseProxy = data.useReverseProxy;
            d->httpProxy = data.httpProxy;
            d->secureProxy = data.secureProxy;
            d->ftpProxy = data.ftpProxy;
            d->gopherProxy = data.gopherProxy;
            d->noProxyFor = data.noProxyFor;
            d->changed = data.changed;
            d->envBased = data.envBased;
            changed( true );
        }
    }
    delete dlg;
}

void KProxyDialog::autoDiscoverChecked( bool on )
{
    if ( m_btnId != 0 )
    {
        m_btnId = 0;
        if ( on )
        {
            changed( on );
            m_bSettingChanged = on;
        }
    }
}

void KProxyDialog::autoScriptChecked( bool on )
{
    if ( m_btnId != 1 )
    {
        m_btnId = 1;
        if ( on )
        {
            changed( on );
            m_bSettingChanged = on;
        }
    }
    ur_location->setEnabled( on );
}

void KProxyDialog::manualChecked( bool on )
{
    if ( m_btnId != 2 )
    {
        m_btnId = 2;
        m_bSettingChanged = on;
        if ( d )
          d->changed = false;
    }
    pb_manSetup->setEnabled( on );
}

void KProxyDialog::envVarChecked( bool on )
{
    if ( m_btnId != 3 )
    {
        m_btnId = 3;
        m_bSettingChanged = on;
        if ( d )
          d->changed = false;
    }
    pb_envSetup->setEnabled( on );
}

void KProxyDialog::promptChecked( bool on )
{
    if ( m_btnId != 4 )
    {
        m_btnId = 4;
        if ( on )
            changed( on );
    }
}

void KProxyDialog::autoChecked( bool on )
{
    if ( m_btnId != 5 )
    {
        m_btnId = 5;
        if ( on )
            changed( on );
    }
}

void KProxyDialog::useProxyChecked( bool on )
{
    if ( useProxy != on )
    {
        gb_configure->setEnabled( on );
        gb_auth->setEnabled( on );
        useProxy = on;
        changed( true );
    }
}

void KProxyDialog::autoScriptChanged( const QString& )
{
    changed( true );
}

void KProxyDialog::changed( bool flag )
{
    emit KCModule::changed( flag );
}
