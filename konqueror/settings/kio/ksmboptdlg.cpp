// ksmboptdlg.h - SMB configuration by Nicolas Brodu <brodu@kde.org>
// code adapted from other option dialogs

#include <qlayout.h>
#include <qwhatsthis.h>
#include <kbuttonbox.h>

#include <kapp.h>
#include <kdialog.h>
#include <klocale.h>
#include <kglobal.h>
#include <kconfig.h>

#include "ksmboptdlg.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


KSMBOptions::KSMBOptions(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
    QGridLayout *topLayout = new QGridLayout(this,1,1,10,0,"smbOptTopLayout");
    labelInfo1 = new QLabel(i18n("Note: kio_smb is a SMB client only. The server, if any, cannot be configured from here."), this);
    topLayout->addWidget(labelInfo1,0,0);
/*  labelInfo2 = new QLabel(i18n(""), this);
    topLayout->addWidget(labelInfo2,1,0);*/
    topLayout->addRowSpacing(1,10);
    topLayout->setRowStretch(1,0);

    QGridLayout *subLayout;

    // =======================
    //   net groupbox
    // =======================

    groupNet = new QGroupBox(i18n("Network settings"), this);
    topLayout->addWidget(groupNet, 2, 0);
    topLayout->addRowSpacing(3,10);
    topLayout->setRowStretch(3,0);

    subLayout = new QGridLayout(groupNet,1,1,10,0,"smbOptNetSubLayout");

    subLayout->addColSpacing(1,10);
    subLayout->addRowSpacing(0,10);
    subLayout->setRowStretch(0,0);

    labelBrowseServer = new QLabel(i18n("&Browse server:"), groupNet);
    subLayout->addWidget(labelBrowseServer,1,0);

    editBrowseServer = new QLineEdit(groupNet);
    labelBrowseServer->setBuddy( editBrowseServer );
    subLayout->addWidget(editBrowseServer,1,2);
    connect(editBrowseServer, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
    QString wtstr = i18n( "Here you can specify the server that provides browsing information, such as the list of servers." );
    QWhatsThis::add( labelBrowseServer, wtstr );
    QWhatsThis::add( editBrowseServer, wtstr );


    subLayout->addRowSpacing(2,5);
    subLayout->setRowStretch(2,0);
    subLayout->addRowSpacing(4,5);
    subLayout->setRowStretch(4,0);

#ifdef USE_SAMBA
    labelOtherOptions = new QLabel(i18n("Other options:"), groupNet);
    subLayout->addWidget(labelOtherOptions,3,0);
    labelConfFile = new QLabel(QString(""SMB_CONF_FILE), groupNet);
    subLayout->addWidget(labelConfFile,3,2);
#else
    labelBroadcast = new QLabel(i18n("B&roadcast address:"), groupNet);
    subLayout->addWidget(labelBroadcast,3,0);

    editBroadcast = new QLineEdit(groupNet);
    labelBroadcast->setBuddy( editBroadcast );
    subLayout->addWidget(editBroadcast,3,2);
    connect(editBroadcast, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
    wtstr = i18n( "Enter the broadcast address of your network here. Usually, this is an IP address with 255 as the last of the four numbers." );
    QWhatsThis::add( labelBroadcast, wtstr );
    QWhatsThis::add( editBroadcast, wtstr );

    labelWINS = new QLabel(i18n("&WINS address:"), groupNet);
    subLayout->addWidget(labelWINS,5,0);

    editWINS = new QLineEdit(groupNet);
    labelWINS->setBuddy( editWINS );
    subLayout->addWidget(editWINS,5,2);
    connect(editWINS, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
    wtstr = i18n( "Specify the address of the WINS server here (if any)." );
    QWhatsThis::add( labelWINS, wtstr );
    QWhatsThis::add( editWINS, wtstr );

    subLayout->addRowSpacing(6,5);
    subLayout->setRowStretch(6,0);
#endif
    subLayout->activate();

    // =======================
    //   user groupbox
    // =======================

    groupUser = new QGroupBox(i18n("User settings"), this);
    topLayout->addWidget(groupUser, 4, 0);
    QWhatsThis::add( groupUser, i18n( "In this area, you can configure which shares to access. You can also remove shares that you no longer want to access or that have been revoked." ) );

    subLayout = new QGridLayout(groupUser,14,5,10,0,"smbOptUserSubLayout");
    subLayout->addRowSpacing(0,10);
    subLayout->setRowStretch(0,0);
    subLayout->addRowSpacing(3,5);
    subLayout->setRowStretch(3,0);
    subLayout->addRowSpacing(6,5);
    subLayout->setRowStretch(6,0);
    subLayout->addRowSpacing(9,5);
    subLayout->setRowStretch(9,0);
    subLayout->addRowSpacing(12,5);
    subLayout->setRowStretch(12,0);
    subLayout->addColSpacing(1,10);
    subLayout->setColStretch(1,0);
    subLayout->addColSpacing(3,10);
    subLayout->setColStretch(3,0);

    onserverLA = new QLabel( i18n( "&Server:" ), groupUser );
    subLayout->addMultiCellWidget(onserverLA,1,1,0,2);

    onserverED = new QLineEdit( groupUser );
    onserverLA->setBuddy( onserverED );
    subLayout->addMultiCellWidget(onserverED,2,2,0,2);
    connect( onserverED, SIGNAL( textChanged(const QString&) ),
       SLOT( textChanged( const QString&) ) );
    wtstr = i18n( "Enter the name of the server here on which you want to access a share. This must be the SMB name, not the DNS name. But usually, these two are the same." );
    QWhatsThis::add( onserverLA, wtstr );
    QWhatsThis::add( onserverED, wtstr );

    shareLA = new QLabel( i18n( "S&hare:" ), groupUser );
    subLayout->addMultiCellWidget(shareLA,4,4,0,2);

    shareED = new QLineEdit( groupUser );
    shareLA->setBuddy( shareED );
    subLayout->addMultiCellWidget(shareED,5,5,0,2);
    connect( shareED, SIGNAL( textChanged(const QString&) ),
       SLOT( textChanged( const QString&) ) );
    wtstr = i18n( "Enter the name of the share here that you want to access." );
    QWhatsThis::add( shareLA, wtstr );
    QWhatsThis::add( shareED, wtstr );

    loginasLA = new QLabel( i18n( "&Login:" ), groupUser );
    subLayout->addMultiCellWidget(loginasLA,7,7,0,2);

    loginasED = new QLineEdit( groupUser );
    loginasLA->setBuddy( loginasED );
    subLayout->addMultiCellWidget(loginasED,8,8,0,2);
    connect( loginasED, SIGNAL( textChanged(const QString&) ),
       SLOT( textChanged(const QString&) ) );
    wtstr = i18n( "Enter your username on the SMB server here." );
    QWhatsThis::add( loginasLA, wtstr );
    QWhatsThis::add( loginasED, wtstr );

    passwordLA = new QLabel( i18n( "&Password:" ), groupUser );
    subLayout->addMultiCellWidget(passwordLA,10,10,0,2);

    passwordED = new QLineEdit( groupUser );
    passwordLA->setBuddy( passwordED );
    passwordED->setEchoMode(QLineEdit::Password);
    subLayout->addMultiCellWidget(passwordED,11,11,0,2);
    connect( passwordED, SIGNAL( textChanged(const QString&) ),
       SLOT( textChanged( const QString&) ) );
    wtstr = i18n( "Enter your password on the SMB server here." );
    QWhatsThis::add( passwordLA, wtstr );
    QWhatsThis::add( passwordED, wtstr );

    addPB = new QPushButton( i18n( "&Add" ), groupUser );
    subLayout->addWidget(addPB,13,0);
    addPB->setEnabled( false );
    connect( addPB, SIGNAL( clicked() ), SLOT( addClicked() ) );
    connect( addPB, SIGNAL( clicked() ), SLOT( changed() ) );
    QWhatsThis::add( addPB, i18n( "Click this button to add access to a share as specified in the fields above." ) );

    deletePB = new QPushButton( i18n( "&Delete" ), groupUser );
    subLayout->addWidget(deletePB,13,2);
    QWhatsThis::add( deletePB, i18n( "Click this button to delete the currently selected share in the list." ) );

    deletePB->setEnabled( false );
    connect( deletePB, SIGNAL( clicked() ), SLOT( deleteClicked() ) );
    connect( deletePB, SIGNAL( clicked() ), SLOT( changed() ) );

    bindingsLA = new QLabel( i18n( "Known bindings:" ), groupUser );
    subLayout->addWidget(bindingsLA,1,4);

    bindingsLB = new QListBox( groupUser );
    subLayout->addMultiCellWidget(bindingsLB,2,13,4,4);
    wtstr = i18n( "This box lists the currently configured shares. You can add and delete entries by using the buttons <em>Add...</em> and <em>Delete</em>" );
    QWhatsThis::add( bindingsLA, wtstr );
    QWhatsThis::add( bindingsLB, wtstr );

    subLayout->activate();

    bindingsLB->setMultiSelection( false );
    connect( bindingsLB, SIGNAL( highlighted( const QString&) ),
       SLOT( listboxHighlighted( const QString& ) ) );

    topLayout->addRowSpacing(5,10);

    groupPassword = new QHButtonGroup(i18n("Password policy (while browsing)"), this);
    groupPassword->setExclusive( true );
    topLayout->addWidget(groupPassword, 6, 0);
    radioPolicyAdd = new QRadioButton( i18n("Add &new bindings"), groupPassword );
    connect(radioPolicyAdd, SIGNAL(clicked()), this, SLOT(changed()));
    QWhatsThis::add( radioPolicyAdd, i18n( "Select this option if you want a new entry in the above list to be created when a password is requested while browsing." ) );

    radioPolicyNoStore = new QRadioButton( i18n("Don't s&tore new bindings"), groupPassword );
    connect(radioPolicyNoStore, SIGNAL(clicked()), this, SLOT(changed()));
    radioPolicyAdd->setChecked(true);
    QWhatsThis::add( radioPolicyNoStore, i18n( "If this option is selected, no new entries are created in the above list while browsing." ) );

    topLayout->setRowStretch(0,0);
    topLayout->setRowStretch(1,0);
    topLayout->setRowStretch(2,1);
    topLayout->setRowStretch(3,0);
    topLayout->setRowStretch(4,1);
    topLayout->setRowStretch(5,0);
    topLayout->setRowStretch(6,1);

    topLayout->activate();

    bindings.setAutoDelete(true);

    // finaly read the options
    load();
}

KSMBOptions::~KSMBOptions()
{
}

void KSMBOptions::load()
{
  KConfig *g_pConfig = new KConfig("kioslaverc");

    QString tmp;
    g_pConfig->setGroup( "Browser Settings/SMB" );
    tmp = g_pConfig->readEntry( "Browse server" );
    editBrowseServer->setText( tmp );
#ifndef USE_SAMBA
    tmp = g_pConfig->readEntry( "Broadcast address" );
    editBroadcast->setText( tmp );
    tmp = g_pConfig->readEntry( "WINS address" );
    editWINS->setText( tmp );
#endif
    bindings.clear();
    bindingsLB->clear();
    int count = g_pConfig->readNumEntry( "Bindings count");
    QString key, server, share, login, password;
    for (int index=0; index<count; index++) {
        key.sprintf("server%d",index);
        server = g_pConfig->readEntry( key );
        key.sprintf("share%d",index);
        share = g_pConfig->readEntry( key );
        key.sprintf("login%d",index);
        login = g_pConfig->readEntry( key );
        key.sprintf("password%d",index);
        // unscramble
        QString scrambled = g_pConfig->readEntry( key );
        password = "";
        for (uint i=0; i<scrambled.length()/3; i++) {
            QChar qc1 = scrambled[i*3];
            QChar qc2 = scrambled[i*3+1];
            QChar qc3 = scrambled[i*3+2];
            unsigned int a1 = qc1.latin1() - '0';
            unsigned int a2 = qc2.latin1() - 'A';
            unsigned int a3 = qc3.latin1() - '0';
            unsigned int num = ((a1 & 0x3F) << 10) | ((a2& 0x1F) << 5) | (a3 & 0x1F);
            password[i] = QChar((uchar)((num - 17) ^ 173)); // restore
        }
        Item* item = new Item(server,share,login,password);
        int pos = index;
        for (int i=0; i<index; i++) {
            if (bindingsLB->item(i)->text() > item->text) {
                pos = i;
                break;
            }
        }
        bindingsLB->insertItem( new QListBoxText( item->text ), pos);
        bindings.append(item);
    }
    tmp = g_pConfig->readEntry( "Password policy" );
    if ( tmp == "Don't store" ) {
        radioPolicyAdd->setChecked(false);
        radioPolicyNoStore->setChecked(true);
    } else {
        radioPolicyNoStore->setChecked(false);
        radioPolicyAdd->setChecked(true);
    }

    delete g_pConfig;
}

void KSMBOptions::save()
{
  KConfig *g_pConfig = new KConfig("kioslaverc");

    g_pConfig->setGroup( "Browser Settings/SMB" );
    g_pConfig->writeEntry( "Browse server", editBrowseServer->text());
#ifndef USE_SAMBA
    g_pConfig->writeEntry( "Broadcast address", editBroadcast->text());
    g_pConfig->writeEntry( "WINS address", editWINS->text());
#endif
    g_pConfig->writeEntry( "Bindings count", bindingsLB->count());
    QString key; int index=0;
    for (Item* it = bindings.first(); (it); it = bindings.next(), index++) {
        key.sprintf("server%d",index);
        g_pConfig->writeEntry( key, it->server);
        key.sprintf("share%d",index);
        g_pConfig->writeEntry( key, it->share);
        key.sprintf("login%d",index);
        g_pConfig->writeEntry( key, it->login);
        key.sprintf("password%d",index);
        // Weak code, but least it makes the string unreadable
        QString scrambled;
        for (uint i=0; i<it->password.length(); i++) {
            QChar c = it->password[i];
            unsigned int num = (c.unicode() ^ 173) + 17;
            unsigned int a1 = (num & 0xFC00) >> 10;
            unsigned int a2 = (num & 0x3E0) >> 5;
            unsigned int a3 = (num & 0x1F);
            scrambled += (char)(a1+'0');
            scrambled += (char)(a2+'A');
            scrambled += (char)(a3+'0');
        }
        g_pConfig->writeEntry( key, scrambled);
    }
    if (radioPolicyAdd->isChecked())
        g_pConfig->writeEntry( "Password policy", "Add");
    else
        g_pConfig->writeEntry( "Password policy", "Don't store");
    delete g_pConfig;
}

void KSMBOptions::defaults()
{
    editBrowseServer->setText( "" );
#ifndef USE_SAMBA
    editBroadcast->setText( "" );
    editWINS->setText( "" );
#endif
    bindings.clear();
    bindingsLB->clear();
    onserverED->setText( "" );
    loginasED->setText( "" );
    passwordED->setText( "" );
    shareED->setText( "" );
    radioPolicyNoStore->setChecked(false);
    radioPolicyAdd->setChecked(true);
}

void KSMBOptions::textChanged( const QString& )
{
    if (((!onserverED->text().isEmpty() && !loginasED->text().isEmpty())
    || (!onserverED->text().isEmpty() && !shareED->text().isEmpty() && !passwordED->text().isEmpty()))
    && (loginasED->text().isEmpty() || shareED->text().isEmpty()))
        addPB->setEnabled( true );
    else
        addPB->setEnabled( false );
    if (!loginasED->text().isEmpty()) shareED->setEnabled(false);
    else shareED->setEnabled(true);
    if (!shareED->text().isEmpty()) loginasED->setEnabled(false);
    else loginasED->setEnabled(true);
}

KSMBOptions::Item::Item(const QString &e, const QString& h, const QString& l, const QString& p)
    : server(e), share(h), login(l), password(p)
{
    text = KSMBOptions::build_string(e,h,l,p);
}

QString KSMBOptions::build_string(const QString& server, const QString& share, const QString& login, const QString& password)
{
    QString text;
    if (!server.isEmpty()) text += i18n("server: ") + server;
    if (!share.isEmpty()) text += ", " + i18n("share: ") + share;
    if (!login.isEmpty()) text += ", " + i18n("login: ") + login;
    if (!password.isEmpty()) text += " (" + i18n("with password") + ")";
    return text;
}

void KSMBOptions::addClicked()
{
    QString server = onserverED->text();
    QString share = shareED->text();
    QString login = loginasED->text();
    QString password = passwordED->text();

    Item * item = 0;
    QString oldText; // for the QListbox change, if needed
    // First, avoid repetition.
    for (Item* it = bindings.first(); (it); it = bindings.next()) {
        // either share or login is empty
        // new login for server, or new password for share
        // it->share==share must be true in both case (both empty, or equal)
        if (it->server==server && it->share==share) {
            it->login = login; it->password = password;
            oldText = it->text;
            it->text = build_string(server, share, login, password);
            item = it; // found and replaced in our list
            break;
        }
    }
    // if a replacement was done, modify the list box
    if (item) {
        // seek the entry in the list box using 'old' values
        // must do a case sensitive search => findItem does no good
        for (uint i=0; i<bindingsLB->count(); i++) {
            if (bindingsLB->item(i)->text() == oldText) {
                bindingsLB->removeItem(i);
                break;
            }
        }
        int pos = bindingsLB->count();
        for (uint i=0; i<bindingsLB->count(); i++) {
            if (bindingsLB->item(i)->text() > item->text) {
                pos = i;
                break;
            }
        }
        bindingsLB->insertItem( new QListBoxText( item->text ), pos);
        bindingsLB->setCurrentItem(pos);
    // otherwise add the new text (and keep it sorted in just O(n))
    } else {
        item = new Item(server,share,login,password);
        int pos = bindingsLB->count();
        for (uint i=0; i<bindingsLB->count(); i++) {
            if (bindingsLB->item(i)->text() > item->text) {
                pos = i;
                break;
            }
        }
        bindingsLB->insertItem( new QListBoxText( item->text ), pos);
        bindingsLB->setCurrentItem(pos);
        bindings.append(item);
    }
//  listboxHighlighted("");
    onserverED->setText( "" );
    loginasED->setText( "" );
    passwordED->setText( "" );
    shareED->setText( "" );
    onserverED->setFocus();
}


void KSMBOptions::deleteClicked()
{
    if( bindingsLB->count() ) {
        QListBoxItem *boxitem = bindingsLB->item(highlighted_item);
        if (boxitem) {
            QString text = boxitem->text();
            for (Item* it = bindings.first(); (it); it = bindings.next()) {
                if (text == it->text) {
                    bindings.removeRef(it);
                    break;
                }
            }
        }
        bindingsLB->removeItem( highlighted_item );
    }
    if( !bindingsLB->count() ) // no more items
        listboxHighlighted("");
}


void KSMBOptions::listboxHighlighted( const QString& itemtext )
{
    bool found = false;
    for (Item* it = bindings.first(); (it); it = bindings.next()) {
        if (itemtext == it->text) {
            onserverED->setText( it->server );
            shareED->setText( it->share );
            loginasED->setText( it->login );
            passwordED->setText( it->password );
            found = true;
            break;
        }
    }
    if (!found) {
        onserverED->setText( "" );
        loginasED->setText( "" );
        passwordED->setText( "" );
        shareED->setText( "" );
    }
    deletePB->setEnabled( true );
    addPB->setEnabled( false );

    highlighted_item = bindingsLB->currentItem();
}

void KSMBOptions::changed()
{
  emit KCModule::changed(true);
}

QString KSMBOptions::quickHelp() const
{
    return i18n("<h1>Windows Shares</h1>Konqueror is able to access shared "
        "windows filesystems if properly configured. If there is a "
        "specific computer from which you want to browse, fill in "
        "the <em>Browse server</em> field. This is mandatory if you "
        "do not run Samba locally. The <em>Broadcast address</em> "
        "and <em>WINS address</em> fields will also be available if you "
        "use the native code, or the location of the 'smb.conf' file "
        "the options are read from when using Samba. In any case, the "
        "broadcast address (interfaces in smb.conf) must be set up if it "
        "is guessed incorrectly or you have multiple cards. A WINS server "
        "usually improves performance, and greatly reduces the network load.<p>"
        "The bindings are used to assign a default user for a given server, "
        "possibly with the corresponding password, or for accessing specific "
        "shares. If you choose, new bindings will be created for logins and "
        "shares accessed during browsing. You can edit all of them from here. "
        "Passwords will be stored locally, and scrambled so as to render them "
        "unreadable to the human eye. For security reasons you may wish not "
        "to do that, as entries with passwords are clearly indicated as such.<p>");
}

#include "ksmboptdlg.moc"
