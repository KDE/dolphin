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
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kconfig.h>
#include <klistview.h>
#include <kmessagebox.h>
#include <qlabel.h>
#include <kcolorbutton.h>
#include <kcharsets.h>
#include <qspinbox.h>
#include <kdebug.h>
#include <kurlrequester.h>
#include <X11/Xlib.h>
#include <klineedit.h>

#include "htmlopts.h"
#include "policydlg.h"
#include "javaopts.h"

#include <konq_defaults.h> // include default values directly from konqueror
#include <klocale.h>
#include <khtml_settings.h>
#include <khtmldefaults.h>

#include "javaopts.moc"

KJavaOptions::KJavaOptions( KConfig* config, QString group, QWidget *parent,
                            const char *name )
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
    QVGroupBox* domainSpecificGB = new QVGroupBox( i18n( "Domain-specific" ), this );
    toplevel->addWidget( domainSpecificGB, 2 );
    QHBox* domainSpecificHB = new QHBox( domainSpecificGB );
    domainSpecificHB->setSpacing( 10 );
    domainSpecificLV = new KListView( domainSpecificHB );
    domainSpecificLV->addColumn(i18n("Host/Domain"));
    domainSpecificLV->addColumn(i18n("Policy"), 100);

    QVBox* domainSpecificVB = new QVBox( domainSpecificHB );
    domainSpecificVB->setSpacing( 10 );
    QPushButton* addDomainPB = new QPushButton( i18n("Add..."), domainSpecificVB );
    connect( addDomainPB, SIGNAL(clicked()), SLOT( addPressed() ) );

    QPushButton* changeDomainPB = new QPushButton( i18n("Change..."), domainSpecificVB );
    connect( changeDomainPB, SIGNAL( clicked() ), this, SLOT( changePressed() ) );

    QPushButton* deleteDomainPB = new QPushButton( i18n("Delete"), domainSpecificVB );
    connect( deleteDomainPB, SIGNAL( clicked() ), this, SLOT( deletePressed() ) );

    QPushButton* importDomainPB = new QPushButton( i18n("Import..."), domainSpecificVB );
    connect( importDomainPB, SIGNAL( clicked() ), this, SLOT( importPressed() ) );
    importDomainPB->setEnabled( false );

    QPushButton* exportDomainPB = new QPushButton( i18n("Export..."), domainSpecificVB );
    connect( exportDomainPB, SIGNAL( clicked() ), this, SLOT( exportPressed() ) );
    exportDomainPB->setEnabled( false );


    /***************************************************************************
     ***************** Java Runtime Settings ***********************************
     **************************************************************************/
    QVGroupBox* javartGB = new QVGroupBox( i18n( "Java Runtime Settings" ), this );
    toplevel->addWidget( javartGB );

    QHBox* hbox = new QHBox( javartGB );
    javaConsoleCB = new QCheckBox( i18n( "Show Java Console" ), hbox );
    connect( javaConsoleCB, SIGNAL(toggled( bool )), this, SLOT(changed()) );
  		
    javaSecurityManagerCB = new QCheckBox( i18n("Use Security Manager" ), hbox );
    connect( javaSecurityManagerCB, SIGNAL(toggled( bool )), this, SLOT(changed()) );
  		
    QHBox* findJavaHB = new QHBox( javartGB );
    QButtonGroup* dummy = new QButtonGroup( javartGB );
    dummy->hide();
    findJavaHB->setSpacing( 10 );
    autoDetectRB = new QRadioButton( i18n( "&Automatically detect Java" ),
                                     findJavaHB );
    connect( autoDetectRB, SIGNAL(toggled( bool )), this, SLOT(changed()) );
    connect( autoDetectRB, SIGNAL(toggled( bool )), this, SLOT(toggleJavaControls()) );
    dummy->insert( autoDetectRB );
    userSpecifiedRB = new QRadioButton( i18n( "Use user-specified Java" ),
                                        findJavaHB );
    connect( userSpecifiedRB, SIGNAL(toggled( bool )), this, SLOT(changed()) );
    connect( userSpecifiedRB, SIGNAL(toggled( bool )), this, SLOT(toggleJavaControls()) );
    dummy->insert( userSpecifiedRB );

    QHBox* pathHB = new QHBox( javartGB );
    pathHB->setSpacing( 10 );
    QLabel* pathLA = new QLabel( i18n( "&Java Home or path to java" ), pathHB );
    pathED=new  KURLRequester( pathHB);
    connect( pathED, SIGNAL(textChanged( const QString& )), this, SLOT(changed()) );
    pathED->fileDialog()->setMode(KFile::Directory);
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
    wtstr = i18n("If 'Automatically detect Java' is selected, konqueror will try to find "
                 "your java installation on its own (this should normally work, if java is somewhere in your path). "
                 "Select 'Use user-specified Java' if konqueror can't find your Java installation or if you have "
                 "several virtual machines installed and want to use a special one. In this case, enter the full path "
                 "to your java installation in the edit field below.");
    QWhatsThis::add( autoDetectRB, wtstr );
    QWhatsThis::add( userSpecifiedRB, wtstr );
    wtstr = i18n("If 'Use user-specified Java' is selected, you'll need to enter the path to "
                 "your Java installation here (i.e. /usr/lib/jdk ).");
    QWhatsThis::add( pathED, wtstr );
    wtstr = i18n("If you want special arguments to be passed to the virtual machine, enter them here.");
    QWhatsThis::add( addArgED, wtstr );


    // Finally do the loading
    load();
}


void KJavaOptions::load()
{
    // *** load ***
    m_pConfig->setGroup(m_groupname);
    bool bJavaGlobal = m_pConfig->readBoolEntry( "EnableJava", false);
    bool bJavaConsole = m_pConfig->readBoolEntry( "ShowJavaConsole", false );
    bool bJavaAutoDetect = m_pConfig->readBoolEntry( "JavaAutoDetect", true );
    bool bSecurityManager = m_pConfig->readBoolEntry( "UseSecurityManager", true );
    QString sJDKArgs = m_pConfig->readEntry( "JavaArgs", "" );
    QString sJDK = m_pConfig->readEntry( "JavaPath", "/usr/bin/java" );

    if( m_pConfig->hasKey( "JavaDomainSettings" ) )
        updateDomainList( m_pConfig->readListEntry("JavaDomainSettings") );
    else
        updateDomainList( m_pConfig->readListEntry("JavaScriptDomainAdvice") );

    // *** apply to GUI ***
    enableJavaGloballyCB->setChecked( bJavaGlobal );
    javaConsoleCB->setChecked( bJavaConsole );
    javaSecurityManagerCB->setChecked( bSecurityManager );

    if( bJavaAutoDetect )
        autoDetectRB->setChecked( true );
    else
        userSpecifiedRB->setChecked( true );

    addArgED->setText( sJDKArgs );
    pathED->lineEdit()->setText( sJDK );

    toggleJavaControls();
}

void KJavaOptions::defaults()
{
    enableJavaGloballyCB->setChecked( false );
    javaConsoleCB->setChecked( false );
    javaSecurityManagerCB->setChecked( true );
    autoDetectRB->setChecked( true );
    pathED->lineEdit()->setText( "/usr/bin/java" );
    addArgED->setText( "" );

    toggleJavaControls();
}

void KJavaOptions::save()
{
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry( "EnableJava", enableJavaGloballyCB->isChecked());
    m_pConfig->writeEntry( "ShowJavaConsole", javaConsoleCB->isChecked() );
    m_pConfig->writeEntry( "JavaAutoDetect", autoDetectRB->isChecked() );
    m_pConfig->writeEntry( "JavaArgs", addArgED->text() );
    m_pConfig->writeEntry( "JavaPath", pathED->lineEdit()->text() );
    m_pConfig->writeEntry( "UseSecurityManager", javaSecurityManagerCB->isChecked() );

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
    if( domainConfig.count() > 0 )
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

    javaConsoleCB->setEnabled(isEnabled);
    javaSecurityManagerCB->setEnabled(isEnabled);
    addArgED->setEnabled(isEnabled);
    autoDetectRB->setEnabled(isEnabled);
    userSpecifiedRB->setEnabled(isEnabled);
    pathED->setEnabled(isEnabled && userSpecifiedRB->isChecked());
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
        QListViewItem* index = new QListViewItem( domainSpecificLV, pDlg.domain(),
                                                  KHTMLSettings::adviceToStr( (KHTMLSettings::KJavaScriptAdvice)
                                                                                                      pDlg.javaPolicyAdvice() ) );
        javaDomainPolicy.insert( index, (KHTMLSettings::KJavaScriptAdvice)pDlg.javaPolicyAdvice());
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
    pDlg.setDisableEdit( false, index->text(0) );
    pDlg.setCaption( i18n( "Change Java/JavaScript Policy" ) );
    pDlg.setDefaultPolicy( javaAdvice, javaScriptAdvice );
    if( pDlg.exec() )
    {
        javaDomainPolicy[index] = pDlg.javaPolicyAdvice();
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
    for ( QStringList::ConstIterator it = domainConfig.begin();
             it != domainConfig.end(); ++it)
    {
        QString domain;
        KHTMLSettings::KJavaScriptAdvice javaAdvice;
        KHTMLSettings::KJavaScriptAdvice javaScriptAdvice;
        KHTMLSettings::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
        QListViewItem* index =	new QListViewItem( domainSpecificLV, domain,
                                                   i18n(KHTMLSettings::adviceToStr(javaAdvice))  );

        javaDomainPolicy[index] = javaAdvice;
    }
}
