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
// Big changes to accomodate per-domain settings
// (c) Leo Savernik 2002

#include <config.h>
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


JavaPolicies::JavaPolicies(KConfig* config, const QString &group, bool global,
  		const QString &domain) :
	Policies(config,group,global,domain,"java.","EnableJava") {
}

JavaPolicies::JavaPolicies() : Policies(0,QString::null,false,
	QString::null,QString::null,QString::null) {
}

JavaPolicies::~JavaPolicies() {
}

KJavaOptions::KJavaOptions( KConfig* config, QString group,
                            QWidget *parent, const char *name )
    : KCModule( parent, name ),
      _removeJavaScriptDomainAdvice(false),
      m_pConfig( config ),
      m_groupname( group ),
      java_global_policies(config,group,true),
      _removeJavaDomainSettings(false)
{
    QVBoxLayout* toplevel = new QVBoxLayout( this, 10, 5 );

    /***************************************************************************
     ********************* Global Settings *************************************
     **************************************************************************/
    QVGroupBox* globalGB = new QVGroupBox( i18n( "Global Settings" ), this );
    toplevel->addWidget( globalGB );
    enableJavaGloballyCB = new QCheckBox( i18n( "Enable Ja&va globally" ), globalGB );
    connect( enableJavaGloballyCB, SIGNAL( clicked() ), this, SLOT( slotChanged() ) );
    connect( enableJavaGloballyCB, SIGNAL( clicked() ), this, SLOT( toggleJavaControls() ) );


    /***************************************************************************
     ***************** Domain Specific Settings ********************************
     **************************************************************************/
    QGroupBox* domainSpecificGB = new QGroupBox( i18n( "Doma&in-Specific" ), this );
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
    connect(domainSpecificLV,SIGNAL(returnPressed ( QListViewItem * )), this, SLOT( changePressed() ) );

    domainSpecificGBLayout->addMultiCellWidget( domainSpecificLV, 0, 5, 0, 0 );

    QPushButton* addDomainPB = new QPushButton( i18n("&New..."), domainSpecificGB );
    domainSpecificGBLayout->addWidget( addDomainPB, 0, 1 );
    connect( addDomainPB, SIGNAL(clicked()), SLOT( addPressed() ) );

    QPushButton* changeDomainPB = new QPushButton( i18n("Chan&ge..."), domainSpecificGB );
    domainSpecificGBLayout->addWidget( changeDomainPB, 1, 1 );
    connect( changeDomainPB, SIGNAL( clicked() ), this, SLOT( changePressed() ) );

    QPushButton* deleteDomainPB = new QPushButton( i18n("D&elete"), domainSpecificGB );
    domainSpecificGBLayout->addWidget( deleteDomainPB, 2, 1 );
    connect( deleteDomainPB, SIGNAL( clicked() ), this, SLOT( deletePressed() ) );

#if 0
    QPushButton* importDomainPB = new QPushButton( i18n("&Import..."), domainSpecificGB );
    domainSpecificGBLayout->addWidget( importDomainPB, 3, 1 );
    connect( importDomainPB, SIGNAL( clicked() ), this, SLOT( importPressed() ) );
    importDomainPB->setEnabled( false );
    importDomainPB->hide();

    QPushButton* exportDomainPB = new QPushButton( i18n("&Export..."), domainSpecificGB );
    domainSpecificGBLayout->addWidget( exportDomainPB, 4, 1 );
    connect( exportDomainPB, SIGNAL( clicked() ), this, SLOT( exportPressed() ) );
    exportDomainPB->setEnabled( false );
    exportDomainPB->hide();
#endif

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    domainSpecificGBLayout->addItem( spacer, 5, 1 );
    toplevel->addWidget( domainSpecificGB, 2 );

    /***************************************************************************
     ***************** Java Runtime Settings ***********************************
     **************************************************************************/
    QVGroupBox* javartGB = new QVGroupBox( i18n( "Java Runtime Settings" ), this );
    toplevel->addWidget( javartGB );

    QHBox* hbox = new QHBox( javartGB );
    javaConsoleCB = new QCheckBox( i18n( "Sho&w Java console" ), hbox );
    connect( javaConsoleCB, SIGNAL(toggled( bool )), this, SLOT(slotChanged()) );

    javaSecurityManagerCB = new QCheckBox( i18n("&Use security manager" ), hbox );
    connect( javaSecurityManagerCB, SIGNAL(toggled( bool )), this, SLOT(slotChanged()) );

    enableShutdownCB = new QCheckBox( i18n("Shu&tdown applet server when inactive"), javartGB );
    connect( enableShutdownCB, SIGNAL(toggled( bool )), this, SLOT(slotChanged()) );
    connect( enableShutdownCB, SIGNAL(clicked()), this, SLOT(toggleJavaControls()) );

    QHBox* secondsHB = new QHBox( javartGB );
    serverTimeoutSB = new KIntNumInput( secondsHB );
    serverTimeoutSB->setRange( 0, 1000, 5 );
    serverTimeoutSB->setLabel( i18n("App&let server timeout:"), AlignLeft );
    serverTimeoutSB->setSuffix(i18n(" sec"));
    connect(serverTimeoutSB, SIGNAL(valueChanged(int)),this,SLOT(slotChanged()));

    QHBox* pathHB = new QHBox( javartGB );
    pathHB->setSpacing( 10 );
    QLabel* pathLA = new QLabel( i18n( "&Path to Java executable, or 'java':" ),
                                 pathHB );
    pathED = new  KURLRequester( pathHB );
    connect( pathED, SIGNAL(textChanged( const QString& )), this, SLOT(slotChanged()) );
    pathLA->setBuddy( pathED );

    QHBox* addArgHB = new QHBox( javartGB );
    addArgHB->setSpacing( 10 );
    QLabel* addArgLA = new QLabel( i18n( "Additional Java a&rguments:" ), addArgHB  );
    addArgED = new QLineEdit( addArgHB );
    connect( addArgED, SIGNAL(textChanged( const QString& )), this, SLOT(slotChanged()) );
    addArgLA->setBuddy( addArgED );

    /***************************************************************************
     ********************** WhatsThis? items ***********************************
     **************************************************************************/
    QWhatsThis::add( enableJavaGloballyCB, i18n("Enables the execution of scripts written in Java "
          "that can be contained in HTML pages. "
          "Note that, as with any browser, enabling active contents can be a security problem.") );
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
#if 0
    QWhatsThis::add( importDomainPB, i18n("Click this button to choose the file that contains "
                                          "the Java policies. These policies will be merged "
                                          "with the exisiting ones. Duplicate entries are ignored.") );
    QWhatsThis::add( exportDomainPB, i18n("Click this button to save the Java policy to a zipped "
                                          "file. The file, named <b>java_policy.tgz</b>, will be "
                                          "saved to a location of your choice." ) );
#endif
    QWhatsThis::add( domainSpecificGB, i18n("Here you can set specific Java policies for any particular "
                                            "host or domain. To add a new policy, simply click the <i>Add...</i> "
                                            "button and supply the necessary information requested by the "
                                            "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                            "button and choose the new policy from the policy dialog box. Clicking "
                                            "on the <i>Delete</i> button will remove the selected policy causing the default "
                                            "policy setting to be used for that domain.") );
#if 0
                                            "The <i>Import</i> and <i>Export</i> "
                                            "button allows you to easily share your policies with other people by allowing "
                                            "you to save and retrieve them from a zipped file.") );
#endif

    QWhatsThis::add( javaConsoleCB, i18n( "If this box is checked, Konqueror will open a console window that Java programs "
                                          "can use for character-based input/output. Well-written Java applets do not need "
                                          "this, but the console can help to find problems with Java applets.") );

    QWhatsThis::add( javaSecurityManagerCB, i18n( "Enabling the security manager will cause the jvm to run with a Security "
                                                  "Manager in place. This will keep applets from being able to read and "
                                                  "write to your file system, creating arbitrary sockets, and other actions "
                                                  "which could be used to compromise your system. Disable this option at your "
                                                  "own risk. You can modify your $HOME/.java.policy file with the Java "
                                                  "policytool utility to give code downloaded from certain sites more "
                                                  "permissions." ) );

    QWhatsThis::add( pathED, i18n("Enter the path to the java executable. If you want to use the jre in "
                                  "your path, simply leave it as 'java'. If you need to use a different jre, "
                                  "enter the path to the java executable (e.g. /usr/lib/jdk/bin/java), "
                                  "or the path to the directory that contains 'bin/java' (e.g. /opt/IBMJava2-13).") );

    QWhatsThis::add( addArgED, i18n("If you want special arguments to be passed to the virtual machine, enter them here.") );

    QString shutdown = i18n("When all the applets have been destroyed, the applet server should shut down. "
                                           "However, starting the jvm takes a lot of time. If you would like to "
                                           "keep the java process running while you are "
                                           "browsing, you can set the timeout value to whatever you like. To keep "
                                           "the java process running for the whole time that the konqueror process is, "
                                           "leave the Shutdown Applet Server checkbox unchecked.");
    QWhatsThis::add( serverTimeoutSB, shutdown);
    QWhatsThis::add( enableShutdownCB, shutdown);
    // Finally do the loading
    load();
}

void KJavaOptions::load()
{
    // *** load ***
    java_global_policies.load();
    bool bJavaGlobal      = java_global_policies.isFeatureEnabled();
    bool bJavaConsole     = m_pConfig->readBoolEntry( "ShowJavaConsole", false );
    bool bSecurityManager = m_pConfig->readBoolEntry( "UseSecurityManager", true );
    bool bServerShutdown  = m_pConfig->readBoolEntry( "ShutdownAppletServer", true );
    int  serverTimeout    = m_pConfig->readNumEntry( "AppletServerTimeout", 60 );
#if defined(PATH_JAVA)
    QString sJavaPath     = m_pConfig->readEntry( "JavaPath", PATH_JAVA );
#else
    QString sJavaPath     = m_pConfig->readEntry( "JavaPath", "java" );
#endif

    if( sJavaPath == "/usr/lib/jdk" )
        sJavaPath = "java";

    if( m_pConfig->hasKey( "JavaDomains" ) )
    	updateDomainList(m_pConfig->readListEntry("JavaDomains"));
    else if( m_pConfig->hasKey( "JavaDomainSettings" ) ) {
        updateDomainListLegacy( m_pConfig->readListEntry("JavaDomainSettings") );
	_removeJavaDomainSettings = true;
    } else {
        updateDomainListLegacy( m_pConfig->readListEntry("JavaScriptDomainAdvice") );
	_removeJavaScriptDomainAdvice = true;
    }

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
    java_global_policies.defaults();
    enableJavaGloballyCB->setChecked( false );
    javaConsoleCB->setChecked( false );
    javaSecurityManagerCB->setChecked( true );
    pathED->lineEdit()->setText( "java" );
    addArgED->setText( "" );
    enableShutdownCB->setChecked(true);
    serverTimeoutSB->setValue( 60 );
    toggleJavaControls();
}

void KJavaOptions::save()
{
    java_global_policies.save();
    m_pConfig->writeEntry( "ShowJavaConsole", javaConsoleCB->isChecked() );
    m_pConfig->writeEntry( "JavaArgs", addArgED->text() );
    m_pConfig->writeEntry( "JavaPath", pathED->lineEdit()->text() );
    m_pConfig->writeEntry( "UseSecurityManager", javaSecurityManagerCB->isChecked() );
    m_pConfig->writeEntry( "ShutdownAppletServer", enableShutdownCB->isChecked() );
    m_pConfig->writeEntry( "AppletServerTimeout", serverTimeoutSB->value() );

    QStringList domainList;
    DomainPolicyMap::Iterator it = javaDomainPolicy.begin();
    for (; it != javaDomainPolicy.end(); ++it) {
    	QListViewItem *current = it.key();
	JavaPolicies &pol = it.data();
	pol.save();
	domainList.append(current->text(0));
    }
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry("JavaDomains", domainList);
#if 0
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
#endif

    if (_removeJavaDomainSettings) {
      m_pConfig->deleteEntry("JavaDomainSettings");
      _removeJavaDomainSettings = false;
    }

    // sync moved to KJSParts::save
//    m_pConfig->sync();
}

void KJavaOptions::slotChanged()
{
    emit changed(true);
}


void KJavaOptions::toggleJavaControls()
{
    bool isEnabled = java_global_policies.isFeatureEnabled();

    javaConsoleCB->setEnabled( isEnabled );
    javaSecurityManagerCB->setEnabled( isEnabled );
    addArgED->setEnabled( isEnabled );
    pathED->setEnabled( isEnabled );
    enableShutdownCB->setEnabled( isEnabled );

    serverTimeoutSB->setEnabled( enableShutdownCB->isChecked() && isEnabled );
}

void KJavaOptions::addPressed()
{
    JavaPolicies pol_copy(m_pConfig,m_groupname,false);
    pol_copy.defaults();
    PolicyDialog pDlg(&pol_copy, this);
    pDlg.setCaption( i18n( "New Java Policy" ) );
    setupPolicyDlg(pDlg,pol_copy);
    if( pDlg.exec() ) {
        QListViewItem* index = new QListViewItem( domainSpecificLV, pDlg.domain(),
                                                  pDlg.featureEnabledPolicyText() );
	pol_copy.setDomain(pDlg.domain());
        javaDomainPolicy.insert(index, pol_copy);
        domainSpecificLV->setCurrentItem( index );
        slotChanged();
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

    JavaPolicies pol_copy = javaDomainPolicy[index];

    PolicyDialog pDlg( &pol_copy, this );
    pDlg.setDisableEdit( true, index->text(0) );
    pDlg.setCaption( i18n( "Change Java Policy" ) );
    setupPolicyDlg(pDlg,pol_copy);
    if( pDlg.exec() )
    {
        pol_copy.setDomain(pDlg.domain());
        javaDomainPolicy[index] = pol_copy;
        index->setText(0, pDlg.domain() );
        index->setText(1, pDlg.featureEnabledPolicyText());
        slotChanged();
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
    slotChanged();
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
    JavaPolicies pol(m_pConfig,m_groupname,false);
    for (QStringList::ConstIterator it = domainConfig.begin();
         it != domainConfig.end(); ++it) {
      QString domain = *it;
      pol.setDomain(domain);
      pol.load();

      QString policy;
      if (pol.isFeatureEnabledPolicyInherited())
        policy = i18n("Use global");
      else if (pol.isFeatureEnabled())
        policy = i18n("Accept");
      else
        policy = i18n("Reject");
      QListViewItem *index =
        new QListViewItem( domainSpecificLV, domain, policy );

      javaDomainPolicy[index] = pol;

    }
}

void KJavaOptions::updateDomainListLegacy(const QStringList &domainConfig)
{
    domainSpecificLV->clear();
    JavaPolicies pol(m_pConfig,m_groupname,false);
    pol.defaults();
    for ( QStringList::ConstIterator it = domainConfig.begin();
          it != domainConfig.end(); ++it)
    {
        QString domain;
        KHTMLSettings::KJavaScriptAdvice javaAdvice;
        KHTMLSettings::KJavaScriptAdvice javaScriptAdvice;
        KHTMLSettings::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
        QListViewItem* index = new QListViewItem( domainSpecificLV, domain,
                                                  i18n(KHTMLSettings::adviceToStr(javaAdvice))  );
        pol.setDomain(domain);
        pol.setFeatureEnabled(javaAdvice != KHTMLSettings::KJavaScriptReject);
        javaDomainPolicy[index] = pol;
    }
}

void KJavaOptions::setupPolicyDlg(PolicyDialog &pDlg,JavaPolicies &pol) {
  pDlg.setFeatureEnabledLabel(i18n("&Java policy:"));
  pDlg.setFeatureEnabledWhatsThis(i18n("Select a Java policy for "
                                    "the above host or domain."));
  pDlg.refresh();
}

#include "javaopts.moc"
