// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
// KControl port & modifications
// (c) Torben Weis 1998
// End of the KControl port, added 'kfmclient configure' call.
// (c) David Faure 1998
// New configuration scheme for Java/JavaScript
// (c) Kalle Dalheimer 2000
// Redesign and cleanup
// (c) Daniel Molkentin 2000

#include <kfiledialog.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kconfig.h>
#include <klistview.h>
#include <kmessagebox.h>
#include <kcolorbutton.h>
#include <kcharsets.h>
#include <kurlrequester.h>
#include <kdebug.h>
#include <klineedit.h>
#include <konq_defaults.h> // include default values directly from konqueror
#include <klocale.h>
#include <khtml_settings.h>
#include <khtmldefaults.h>
#include <knuminput.h>

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcolor.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qmessagebox.h>
#include <qwhatsthis.h>
#include <qlineedit.h>
#include <qvgroupbox.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>

#include "htmlopts.h"
#include "policydlg.h"
#include "javaopts.h"


KJavaOptions::KJavaOptions( KConfig* config, QString group,
                            QWidget *parent, const char *name )
    : KCModule( parent, name ),
      m_pConfig( config ),
      m_groupname( group )
{
    QVBoxLayout* toplevel = new QVBoxLayout( this, 10, 5 );

    /***************************************************************************
     ********************* Global Settings *************************************
     **************************************************************************/
    QVGroupBox* globalGB = new QVGroupBox( i18n( "Global Settings" ), this );
    toplevel->addWidget( globalGB );
    enableJavaGloballyCB = new QCheckBox( i18n( "Enable &Java globally" ), globalGB );
    connect( enableJavaGloballyCB, SIGNAL( clicked() ), this, SLOT( changed() ) );
    connect( enableJavaGloballyCB, SIGNAL( clicked() ), this, SLOT( toggleJavaControls() ) );


    /***************************************************************************
     ***************** Domain Specific Settings ********************************
     **************************************************************************/
    QGroupBox* domainSpecificGB = new QGroupBox( i18n( "Domain-specific" ), this );
    domainSpecificGB->setColumnLayout(0, Qt::Vertical );
    domainSpecificGB->layout()->setSpacing( 0 );
    domainSpecificGB->layout()->setMargin( 0 );
    QGridLayout* domainSpecificGBLayout = new QGridLayout( domainSpecificGB->layout() );
    domainSpecificGBLayout->setAlignment( Qt::AlignTop );
    domainSpecificGBLayout->setSpacing( 6 );
    domainSpecificGBLayout->setMargin( 11 );


    domainSpecificLV = new KListView( domainSpecificGB );
    domainSpecificLV->addColumn(i18n("Host/Domain"));
    domainSpecificLV->addColumn(i18n("Policy"), 100);
    connect(domainSpecificLV,SIGNAL(doubleClicked ( QListViewItem * )), this, SLOT( changePressed() ) );

    domainSpecificGBLayout->addMultiCellWidget( domainSpecificLV, 0, 5, 0, 0 );

    QPushButton* addDomainPB = new QPushButton( i18n("&Add..."), domainSpecificGB );
    domainSpecificGBLayout->addWidget( addDomainPB, 0, 1 );
    connect( addDomainPB, SIGNAL(clicked()), SLOT( addPressed() ) );

    QPushButton* changeDomainPB = new QPushButton( i18n("&Change..."), domainSpecificGB );
    domainSpecificGBLayout->addWidget( changeDomainPB, 1, 1 );
    connect( changeDomainPB, SIGNAL( clicked() ), this, SLOT( changePressed() ) );

    QPushButton* deleteDomainPB = new QPushButton( i18n("&Delete"), domainSpecificGB );
    domainSpecificGBLayout->addWidget( deleteDomainPB, 2, 1 );
    connect( deleteDomainPB, SIGNAL( clicked() ), this, SLOT( deletePressed() ) );

    QPushButton* importDomainPB = new QPushButton( i18n("&Import..."), domainSpecificGB );
    domainSpecificGBLayout->addWidget( importDomainPB, 3, 1 );
    connect( importDomainPB, SIGNAL( clicked() ), this, SLOT( importPressed() ) );
    importDomainPB->setEnabled( false );

    QPushButton* exportDomainPB = new QPushButton( i18n("&Export..."), domainSpecificGB );
    domainSpecificGBLayout->addWidget( exportDomainPB, 4, 1 );
    connect( exportDomainPB, SIGNAL( clicked() ), this, SLOT( exportPressed() ) );
    exportDomainPB->setEnabled( false );

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    domainSpecificGBLayout->addItem( spacer, 5, 1 );
    toplevel->addWidget( domainSpecificGB, 2 );

    /***************************************************************************
     ***************** Java Runtime Settings ***********************************
     **************************************************************************/
    QVGroupBox* javartGB = new QVGroupBox( i18n( "Java Runtime Settings" ), this );
    toplevel->addWidget( javartGB );

    QHBox* hbox = new QHBox( javartGB );
    javaConsoleCB = new QCheckBox( i18n( "&Show Java Console" ), hbox );
    connect( javaConsoleCB, SIGNAL(toggled( bool )), this, SLOT(changed()) );

    javaSecurityManagerCB = new QCheckBox( i18n("&Use Security Manager" ), hbox );
    connect( javaSecurityManagerCB, SIGNAL(toggled( bool )), this, SLOT(changed()) );

    enableShutdownCB = new QCheckBox( i18n("S&hutdown Applet Server when inactive"), javartGB );
    connect( enableShutdownCB, SIGNAL(toggled( bool )), this, SLOT(changed()) );
    connect( enableShutdownCB, SIGNAL(clicked()), this, SLOT(toggleJavaControls()) );

    QHBox* secondsHB = new QHBox( javartGB );
    serverTimeoutSB = new KIntNumInput( secondsHB );
    serverTimeoutSB->setRange( 0, 1000, 5 );
    serverTimeoutSB->setLabel( i18n("Applet Server Timeout (seconds)"), AlignLeft );

    QHBox* pathHB = new QHBox( javartGB );
    pathHB->setSpacing( 10 );
    QLabel* pathLA = new QLabel( i18n( "&Path to java executable, or 'java'" ),
                                 pathHB );
    pathED = new  KURLRequester( pathHB );
    connect( pathED, SIGNAL(textChanged( const QString& )), this, SLOT(changed()) );
    pathLA->setBuddy( pathED );

    QHBox* addArgHB = new QHBox( javartGB );
    addArgHB->setSpacing( 10 );
    QLabel* addArgLA = new QLabel( i18n( "Additional Java A&rguments" ), addArgHB  );
    addArgED = new QLineEdit( addArgHB );
    connect( addArgED, SIGNAL(textChanged( const QString& )), this, SLOT(changed()) );
    addArgLA->setBuddy( addArgED );

    /***************************************************************************
     ********************** WhatsThis? items ***********************************
     **************************************************************************/
    QWhatsThis::add( enableJavaGloballyCB, i18n("Enables the execution of scripts written in Java "
          "that can be contained in HTML pages. Be aware that Java support "
          "is not yet finished. Note that, as with any browser, enabling active contents can be a security problem.") );
    QString wtstr = i18n("This box contains the domains and hosts you have set "
                         "a specific Java policy for. This policy will be used "
                         "instead of the default policy for enabling or disabling Java applets on pages sent by these "
                         "domains or hosts. <p>Select a policy and use the controls on "
                         "the right to modify it.");
    QWhatsThis::add( domainSpecificLV, wtstr );
    QWhatsThis::add( domainSpecificGB, wtstr );
    QWhatsThis::add( addDomainPB, i18n("Click on this button to manually add a host or domain "
                                       "specific policy.") );
    QWhatsThis::add( changeDomainPB, i18n("Click on this button to change the policy for the "
                                          "host or domain selected in the list box.") );
    QWhatsThis::add( deleteDomainPB, i18n("Click on this button to change the policy for the "
                                          "host or domain selected in the list box.") );
    QWhatsThis::add( importDomainPB, i18n("Click this button to choose the file that contains "
                                          "the Java policies.  These policies will be merged "
                                          "with the exisiting ones.  Duplicate entries are ignored.") );
    QWhatsThis::add( exportDomainPB, i18n("Click this button to save the Java policy to a zipped "
                                          "file.  The file, named <b>java_policy.tgz</b>, will be "
                                          "saved to a location of your choice." ) );
    QWhatsThis::add( domainSpecificGB, i18n("Here you can set specific Java policies for any particular "
                                            "host or domain. To add a new policy, simply click the <i>Add...</i> "
                                            "button and supply the necessary information requested by the "
                                            "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                            "button and choose the new policy from the policy dialog box.  Clicking "
                                            "on the <i>Delete</i> button will remove the selected policy causing the default "
                                            "policy setting to be used for that domain. The <i>Import</i> and <i>Export</i> "
                                            "button allows you to easily share your policies with other people by allowing "
                                            "you to save and retrieve them from a zipped file.") );

    QWhatsThis::add( javaConsoleCB, i18n( "If this box is checked, Konqueror will open a console window that Java programs "
                                          "can use for character-based input/output. Well-written Java applets do not need "
                                          "this, but the console can help to find problems with Java applets.") );

    QWhatsThis::add( javaSecurityManagerCB, i18n( "Enabling the security manager will cause the jvm to run with a Security "
                                                  "Manager in place. This will keep applets from being able to read and "
                                                  "write to your file system, creating arbitrary sockets, and other actions "
                                                  "which could be used to compromise your system.  Disable this option at your "
                                                  "own risk.  You can modify your $HOME/.java.policy file with the Java "
                                                  "policytool utility to give code downloaded from certain sites more "
                                                  "permissions." ) );

    QWhatsThis::add( pathED, i18n("Enter the path to the java executable.  If you want to use the jre in "
                                  "your path, simply leave it as 'java'.  If you need to use a different jre, "
                                  "enter the path to the java executable (for example, /usr/lib/jdk/bin/java), "
                                  "or the path to the directory that contains 'bin/java' (for example, /opt/IBMJava2-13).") );

    QWhatsThis::add( addArgED, i18n("If you want special arguments to be passed to the virtual machine, enter them here.") );

    QWhatsThis::add( serverTimeoutSB, i18n("When all the applets have been destroyed, the applet server should shut down.  "
                                           "However, starting the jvm takes a lot of time.  If you would like to "
                                           "keep the java process running while you are "
                                           "browsing, you can set the timeout value to what you would like.  To keep "
                                           "the java process running for the whole time the konqueror process is, "
                                           "leave the Shutdown Applet Server checkbox unchecked.") );

    // Finally do the loading
    load();
}

void KJavaOptions::load()
{
    // *** load ***
    m_pConfig->setGroup(m_groupname);
    bool bJavaGlobal      = m_pConfig->readBoolEntry( "EnableJava", false);
    bool bJavaConsole     = m_pConfig->readBoolEntry( "ShowJavaConsole", false );
    bool bSecurityManager = m_pConfig->readBoolEntry( "UseSecurityManager", true );
    bool bServerShutdown  = m_pConfig->readBoolEntry( "ShutdownAppletServer", true );
    int  serverTimeout    = m_pConfig->readNumEntry( "AppletServerTimeout", 60 );
    QString sJavaPath     = m_pConfig->readEntry( "JavaPath", "java" );
    if( sJavaPath == "/usr/lib/jdk" )
        sJavaPath = "java";

    if( m_pConfig->hasKey( "JavaDomainSettings" ) )
        updateDomainList( m_pConfig->readListEntry("JavaDomainSettings") );
    else
        updateDomainList( m_pConfig->readListEntry("JavaScriptDomainAdvice") );

    // *** apply to GUI ***
    enableJavaGloballyCB->setChecked( bJavaGlobal );
    javaConsoleCB->setChecked( bJavaConsole );
    javaSecurityManagerCB->setChecked( bSecurityManager );

    addArgED->setText( m_pConfig->readEntry( "JavaArgs", "" ) );
    pathED->lineEdit()->setText( sJavaPath );

    enableShutdownCB->setChecked( bServerShutdown );
    serverTimeoutSB->setValue( serverTimeout );

    toggleJavaControls();
}

void KJavaOptions::defaults()
{
    enableJavaGloballyCB->setChecked( false );
    javaConsoleCB->setChecked( false );
    javaSecurityManagerCB->setChecked( true );
    pathED->lineEdit()->setText( "java" );
    addArgED->setText( "" );

    toggleJavaControls();
}

void KJavaOptions::save()
{
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry( "EnableJava", enableJavaGloballyCB->isChecked());
    m_pConfig->writeEntry( "ShowJavaConsole", javaConsoleCB->isChecked() );
    m_pConfig->writeEntry( "JavaArgs", addArgED->text() );
    m_pConfig->writeEntry( "JavaPath", pathED->lineEdit()->text() );
    m_pConfig->writeEntry( "UseSecurityManager", javaSecurityManagerCB->isChecked() );
    m_pConfig->writeEntry( "ShutdownAppletServer", enableShutdownCB->isChecked() );
    m_pConfig->writeEntry( "AppletServerTimeout", serverTimeoutSB->value() );

    QStringList domainConfig;
    QListViewItemIterator it( domainSpecificLV );
    QListViewItem* current;
    while( ( current = it.current() ) )
    {
        ++it;
        QCString javaPolicy = KHTMLSettings::adviceToStr(
                 (KHTMLSettings::KJavaScriptAdvice) javaDomainPolicy[current]);
        QCString javaScriptPolicy = KHTMLSettings::adviceToStr( KHTMLSettings::KJavaScriptDunno );
        domainConfig.append(QString::fromLatin1("%1:%2:%3").arg(current->text(0)).arg(javaPolicy).arg(javaScriptPolicy));
    }
    m_pConfig->writeEntry("JavaDomainSettings", domainConfig);

    m_pConfig->sync();
}

void KJavaOptions::changed()
{
    emit KCModule::changed(true);
}


void KJavaOptions::toggleJavaControls()
{
    bool isEnabled = enableJavaGloballyCB->isChecked();

    javaConsoleCB->setEnabled( isEnabled );
    javaSecurityManagerCB->setEnabled( isEnabled );
    addArgED->setEnabled( isEnabled );
    pathED->setEnabled( isEnabled );
    enableShutdownCB->setEnabled( isEnabled );

    serverTimeoutSB->setEnabled( enableShutdownCB->isChecked() && isEnabled );
}

void KJavaOptions::addPressed()
{
    PolicyDialog pDlg( false, true, this, 0L );
    int def_javapolicy = KHTMLSettings::KJavaScriptReject;
    int def_javascriptpolicy = KHTMLSettings::KJavaScriptReject;
    pDlg.setDefaultPolicy( def_javapolicy, def_javascriptpolicy );
    pDlg.setCaption( i18n( "New Java Policy" ) );
    if( pDlg.exec() )
    {
        KHTMLSettings::KJavaScriptAdvice int_advice = (KHTMLSettings::KJavaScriptAdvice)
                                                      pDlg.javaPolicyAdvice();
        QString advice = KHTMLSettings::adviceToStr( int_advice );
        QListViewItem* index = new QListViewItem( domainSpecificLV, pDlg.domain(),
                                                  advice );
        javaDomainPolicy.insert( index, int_advice );
        domainSpecificLV->setCurrentItem( index );
        changed();
    }
}

void KJavaOptions::changePressed()
{
    QListViewItem *index = domainSpecificLV->currentItem();
    if ( index == 0 )
    {
        KMessageBox::information( 0, i18n("You must first select a policy to be changed!" ) );
        return;
    }

    int javaAdvice = javaDomainPolicy[index];
    int javaScriptAdvice = KHTMLSettings::KJavaScriptDunno;
    PolicyDialog pDlg( false, true, this );
    pDlg.setDisableEdit( true, index->text(0) );
    pDlg.setCaption( i18n( "Change Java Policy" ) );
    pDlg.setDefaultPolicy( javaAdvice, javaScriptAdvice );
    if( pDlg.exec() )
    {
        javaDomainPolicy[index] = pDlg.javaPolicyAdvice();
        index->setText( 0, pDlg.domain() );
        index->setText( 1, i18n(KHTMLSettings::adviceToStr(
            (KHTMLSettings::KJavaScriptAdvice)javaDomainPolicy[index])) );
        changed();
    }
}

void KJavaOptions::deletePressed()
{
    QListViewItem *index = domainSpecificLV->currentItem();
    if ( index == 0 )
    {
        KMessageBox::information( 0, i18n("You must first select a policy to delete!" ) );
        return;
    }

    javaDomainPolicy.remove(index);
    delete index;
    changed();
}

void KJavaOptions::importPressed()
{
  // PENDING(kalle) Implement this.
}

void KJavaOptions::exportPressed()
{
  // PENDING(kalle) Implement this.
}

void KJavaOptions::updateDomainList(const QStringList &domainConfig)
{
    domainSpecificLV->clear();
    for ( QStringList::ConstIterator it = domainConfig.begin();
          it != domainConfig.end(); ++it)
    {
        QString domain;
        KHTMLSettings::KJavaScriptAdvice javaAdvice;
        KHTMLSettings::KJavaScriptAdvice javaScriptAdvice;
        KHTMLSettings::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
        QListViewItem* index = new QListViewItem( domainSpecificLV, domain,
                                                  i18n(KHTMLSettings::adviceToStr(javaAdvice))  );

        javaDomainPolicy[index] = javaAdvice;
    }
}

#include "javaopts.moc"
