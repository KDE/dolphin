//
// Three tabs for HTML Options
//
// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998

// KControl port & modifications
// (c) Torben Weis 1998
// End of the KControl port, added 'kfmclient configure' call.
// (c) David Faure 1998
// New configuration scheme for Java/JavaScript
// (C) Kalle Dalheimer 2000

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


#include <X11/Xlib.h>

#include "htmlopts.h"
#include "policydlg.h"

#include <konqdefaults.h> // include default values directly from konqueror
#include <klocale.h>
#include <khtml_settings.h>

//-----------------------------------------------------------------------------

KAppearanceOptions::KAppearanceOptions(KConfig *config, QString group, QWidget *parent, const char *name)
    : KCModule( parent, name ), m_pConfig(config), m_groupname(group)
{
    QString wtstr;

    QGridLayout *lay = new QGridLayout(this, 1 ,1 , 10, 5);
    int r = 0;
    int E = 0, M = 1, W = 3; //CT 3 (instead 2) allows smaller color buttons

    QButtonGroup *bg = new QButtonGroup( 1, QGroupBox::Vertical,
					 i18n("Font Size"), this );
    lay->addMultiCellWidget(bg, r, r, E, W);

    QWhatsThis::add( bg, i18n("This is the relative font size konqueror uses to display web sites.") );

    bg->setExclusive( TRUE );
    connect(bg, SIGNAL(clicked(int)), this, SLOT(changed()));

    m_pSmall = new QRadioButton( i18n("Small"), bg );
    m_pMedium = new QRadioButton( i18n("Medium"), bg );
    m_pLarge = new QRadioButton( i18n("Large"), bg );

    QLabel* label = new QLabel( i18n("Standard Font"), this );
    lay->addWidget( label , ++r, E);

    m_pStandard = new QComboBox( false, this );
    lay->addMultiCellWidget(m_pStandard, r, r, M, W);

    wtstr = i18n("This is the font used to display normal text in a web page.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pStandard, wtstr );

    getFontList( standardFonts, "-*-*-*-*-*-*-*-*-*-*-p-*-*-*" );
    m_pStandard->insertStrList( &standardFonts );
    connect( m_pStandard, SIGNAL( activated(const QString&) ),
             SLOT( slotStandardFont(const QString&) ) );
    connect( m_pStandard, SIGNAL( activated(const QString&) ),
             SLOT(changed() ) );

    label = new QLabel( i18n( "Fixed Font"), this );
    lay->addWidget( label, ++r, E);

    m_pFixed = new QComboBox( false, this );
    lay->addMultiCellWidget(m_pFixed, r, r, M, W);

    wtstr = i18n("This is the font used to display fixed-width (i.e. non-proportional) text.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pFixed, wtstr );

    getFontList( fixedFonts, "-*-*-*-*-*-*-*-*-*-*-m-*-*-*" );
    m_pFixed->insertStrList( &fixedFonts );

    connect( m_pFixed, SIGNAL( activated(const QString&) ),
             SLOT( slotFixedFont(const QString&) ) );
    connect( m_pFixed, SIGNAL( activated(const QString&) ),
             SLOT(changed() ) );

    // default charset Lars Knoll 17Nov98 (moved by David)
    label = new QLabel( i18n( "Default Charset"), this );
    lay->addWidget( label, ++r, E);

    m_pCharset = new QComboBox( false, this );
    charsets = KGlobal::charsets()->availableCharsetNames();
    charsets.prepend(i18n("Use language charset"));
    m_pCharset->insertStringList( charsets );
    lay->addMultiCellWidget(m_pCharset,r, r, M, W);

    wtstr = i18n("Select the default charset to be used. Normally, you'll be fine with 'Use language charset' "
       "and should not have to change this.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pCharset, wtstr );

    connect( m_pCharset, SIGNAL( activated(const QString& ) ),
             SLOT( slotCharset(const QString&) ) );
    connect( m_pCharset, SIGNAL( activated(const QString& ) ),
             SLOT(changed() ) );

    connect( bg, SIGNAL( clicked( int ) ), SLOT( slotFontSize( int ) ) );

    label = new QLabel( i18n("Normal Text Color:"), this );
    lay->addWidget( label, ++r, E);

    m_pText = new KColorButton( textColor, this );
    lay->addWidget(m_pText, r, M);

    wtstr = i18n("This is the default text color for web pages. Note that if 'Always use my colors' "
       "is not selected, this value can be overridden by a differing web page.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pText, wtstr );

    connect( m_pText, SIGNAL( changed( const QColor & ) ),
             SLOT( slotTextColorChanged( const QColor & ) ) );
    connect( m_pText, SIGNAL( changed( const QColor & ) ),
             SLOT( changed() ) );

    label = new QLabel( i18n("URL Link Color:"), this );
    lay->addWidget( label, ++r, E);

    m_pLink = new KColorButton( linkColor, this );
    lay->addWidget(m_pLink, r, M);

    wtstr = i18n("This is the default color used to display links. Note that if 'Always use my colors' "
       "is not selected, this value can be overridden by a differing web page.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pLink, wtstr );

    connect( m_pLink, SIGNAL( changed( const QColor & ) ),
             SLOT( slotLinkColorChanged( const QColor & ) ) );
    connect( m_pLink, SIGNAL( changed( const QColor & ) ),
             SLOT( changed() ) );

    label = new QLabel( i18n("Followed Link Color:"), this );
    lay->addWidget( label, ++r, E);

    m_pVLink = new KColorButton( vLinkColor, this );
    lay->addWidget(m_pVLink, r, M);

    wtstr = i18n("This is the color used to display links that you've already visited, i.e. are cached. "
       "Note that if 'Always use my colors' is not selected, this value can be overridden by a differing "
       "web page.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pVLink, wtstr );

    connect( m_pVLink, SIGNAL( changed( const QColor & ) ),
             SLOT( slotVLinkColorChanged( const QColor & ) ) );
    connect( m_pVLink, SIGNAL( changed( const QColor & ) ),
             SLOT( changed() ) );

    forceDefaultsbox = new QCheckBox(i18n("Always use my colors"), this);
    r++; lay->addMultiCellWidget(forceDefaultsbox, r, r, E, W);

    QWhatsThis::add( forceDefaultsbox, i18n("If this is selected, konqueror will always "
       "use your default colors to display a web page, even if different colors are specified "
       "in its HTML description.") );

    connect(forceDefaultsbox, SIGNAL(clicked()), this, SLOT(changed()));

    r++; lay->setRowStretch(r, 10);
    load();
}

void KAppearanceOptions::getFontList( QStrList &list, const char *pattern )
{
    int num;

    char **xFonts = XListFonts( qt_xdisplay(), pattern, 2000, &num );

    for ( int i = 0; i < num; i++ )
        addFont( list, xFonts[i] );

    list.sort();
    XFreeFontNames( xFonts );
}

void KAppearanceOptions::addFont( QStrList &list, const char *xfont )
{
    const char *ptr = strchr( xfont, '-' );
    if ( !ptr )
        return;
	
    ptr = strchr( ptr + 1, '-' );
    if ( !ptr )
        return;

    QString font = ptr + 1;

    int pos;
    if ( ( pos = font.find( '-' ) ) > 0 )
    {
        font.truncate( pos );

        if ( font.find( "open look", 0, false ) >= 0 )
            return;

        QStrListIterator it( list );

        for ( ; it.current(); ++it )
            if ( it.current() == font )
                return;

        list.append( font.ascii() );
    }
}

void KAppearanceOptions::slotFontSize( int i )
{
    fSize = i+3;
}

void KAppearanceOptions::slotStandardFont(const QString& n )
{
    stdName = n;
}

void KAppearanceOptions::slotFixedFont(const QString& n )
{
    fixedName = n;
}

void KAppearanceOptions::slotCharset(const QString& n)
{
    charsetName = n;
}

void KAppearanceOptions::load()
{
    m_pConfig->setGroup(m_groupname);
    QString fs = m_pConfig->readEntry( "BaseFontSize" );
    if ( !fs.isEmpty() )
    {
        fSize = fs.toInt();
        if ( fSize < 3 )
            fSize = 3;
        else if ( fSize > 5 )
            fSize = 5;
    }
    else
        fSize = 3;

    stdName = m_pConfig->readEntry( "StandardFont" );
    fixedName = m_pConfig->readEntry( "FixedFont" );
    charsetName = m_pConfig->readEntry( "DefaultCharset" );

    textColor = KGlobalSettings::textColor();
    textColor = m_pConfig->readColorEntry( "TextColor", &textColor );
    linkColor = KGlobalSettings::linkColor();
    linkColor = m_pConfig->readColorEntry( "LinkColor", &linkColor );
    vLinkColor = KGlobalSettings::visitedLinkColor();
    vLinkColor = m_pConfig->readColorEntry( "VLinkColor", &vLinkColor);
    bool forceDefaults = m_pConfig->readBoolEntry("ForceDefaultColors", false);

    m_pText->setColor( textColor );
    m_pLink->setColor( linkColor );
    m_pVLink->setColor( vLinkColor );
    forceDefaultsbox->setChecked( forceDefaults );

    updateGUI();
}

void KAppearanceOptions::defaults()
{
    fSize=4;
    stdName = KGlobalSettings::generalFont().family();
    fixedName = KGlobalSettings::fixedFont().family();
    charsetName = "";

    bgColor = KGlobalSettings::baseColor();
    textColor = KGlobalSettings::textColor();
    linkColor = KGlobalSettings::linkColor();
    vLinkColor = KGlobalSettings::visitedLinkColor();

    m_pText->setColor( textColor );
    m_pLink->setColor( linkColor );
    m_pVLink->setColor( vLinkColor );
    forceDefaultsbox->setChecked( false );

    updateGUI();
}

void KAppearanceOptions::updateGUI()
{
    if ( stdName.isEmpty() )
        stdName = KGlobalSettings::generalFont().family();
    if ( fixedName.isEmpty() )
        fixedName = KGlobalSettings::fixedFont().family();

    QStrListIterator sit( standardFonts );
    int i;
    for ( i = 0; sit.current(); ++sit, i++ )
    {
        if ( stdName == sit.current() )
            m_pStandard->setCurrentItem( i );
    }

    QStrListIterator fit( fixedFonts );
    for ( i = 0; fit.current(); ++fit, i++ )
    {
        if ( fixedName == fit.current() )
            m_pFixed->setCurrentItem( i );
    }

    for ( QStringList::Iterator cit = charsets.begin(); cit != charsets.end(); ++cit )
    {
        if ( charsetName == *cit )
            m_pCharset->setCurrentItem( i );
    }

    m_pSmall->setChecked( fSize == 3 );
    m_pMedium->setChecked( fSize == 4 );
    m_pLarge->setChecked( fSize == 5 );
}

void KAppearanceOptions::save()
{
    m_pConfig->setGroup(m_groupname);			
    m_pConfig->writeEntry( "BaseFontSize", fSize );
    m_pConfig->writeEntry( "StandardFont", stdName );
    m_pConfig->writeEntry( "FixedFont", fixedName );
    // If the user chose "Use language charset", write an empty string
    if (charsetName == i18n("Use language charset"))
        charsetName = "";
    m_pConfig->writeEntry( "DefaultCharset", charsetName );
    m_pConfig->writeEntry( "TextColor", textColor );
    m_pConfig->writeEntry( "LinkColor", linkColor);
    m_pConfig->writeEntry( "VLinkColor", vLinkColor );
    m_pConfig->writeEntry("ForceDefaultColors", forceDefaultsbox->isChecked() );
    m_pConfig->sync();
}


void KAppearanceOptions::changed()
{
  emit KCModule::changed(true);
}

void KAppearanceOptions::slotTextColorChanged( const QColor &col )
{
    if ( textColor != col )
        textColor = col;
}

void KAppearanceOptions::slotLinkColorChanged( const QColor &col )
{
    if ( linkColor != col )
        linkColor = col;
}

void KAppearanceOptions::slotVLinkColorChanged( const QColor &col )
{
    if ( vLinkColor != col )
        vLinkColor = col;
}


KJavaScriptOptions::KJavaScriptOptions( KConfig* config, QString group, QWidget *parent, 
										const char *name ) :
  KCModule( parent, name ), m_pConfig( config ), m_groupname( group )
{
  QVBoxLayout* toplevel = new QVBoxLayout( this, 10, 5 );

  // the global checkbox
  QVGroupBox* globalGB = new QVGroupBox( i18n( "Global Settings" ), this );
  toplevel->addWidget( globalGB );
  enableJavaGloballyCB = new QCheckBox( i18n( "Enable &Java globally" ), globalGB );
  QWhatsThis::add( enableJavaGloballyCB, i18n("Enables the execution of scripts written in Java "
        "that can be contained in HTML pages. Be aware that Java support "
        "is not yet finished. Note that, as with any browser, enabling active contents can be a security problem.") );
  connect( enableJavaGloballyCB, SIGNAL( clicked() ), this, SLOT( changed() ) );
  connect( enableJavaGloballyCB, SIGNAL( clicked() ), 
		   this, SLOT( toggleJavaControls() ) );
  enableJavaScriptGloballyCB = new QCheckBox( i18n( "Enable Java&Script globally" ), 
											  globalGB );
  QWhatsThis::add( enableJavaScriptGloballyCB, i18n("Enables the execution of scripts written in ECMA-Script "
        "(as known as JavaScript) that can be contained in HTML pages. Be aware that JavaScript support "
        "is not yet finished. Note that, as with any browser, enabling scripting languages can be a security problem.") );
  connect( enableJavaScriptGloballyCB, SIGNAL( clicked() ), this, SLOT( changed() ) );

  // the domain-specific listview (copied and modified from Cookies configuration)
  QVGroupBox* domainSpecificGB = new QVGroupBox( i18n( "Domain-specific" ), this );
  toplevel->addWidget( domainSpecificGB, 2 );
  QHBox* domainSpecificHB = new QHBox( domainSpecificGB );
  domainSpecificHB->setSpacing( 10 );
  domainSpecificLV = new KListView( domainSpecificHB );
  domainSpecificLV->setMinimumHeight( 0.10 * sizeHint().height() );
  domainSpecificLV->addColumn(i18n("Hostname"));
  domainSpecificLV->addColumn(i18n("Java Policy"), 100);
  domainSpecificLV->addColumn(i18n("JavaScript Policy"), 120);
  QString wtstr = i18n("This box contains the domains and hosts you have set "
					   "a specific JavaScript policy for. This policy will be used "
					   "instead of the default policy for enabling or disabling JavaScript on pages sent by these "
					   "domains or hosts. <p>Select a policy and use the controls on "
					   "the right to modify it.");
  QWhatsThis::add( domainSpecificLV, wtstr );
  QWhatsThis::add( domainSpecificGB, wtstr );
  
  QVBox* domainSpecificVB = new QVBox( domainSpecificHB );
  domainSpecificVB->setSpacing( 10 );
  QPushButton* addDomainPB = new QPushButton( i18n("Add..."), domainSpecificVB );
  QWhatsThis::add( addDomainPB, i18n("Click on this button to manually add a domain-"
									 "specific policy.") );
  connect( addDomainPB, SIGNAL(clicked()), SLOT( addPressed() ) );
  
  QPushButton* changeDomainPB = new QPushButton( i18n("Change..."), domainSpecificVB );
  QWhatsThis::add( changeDomainPB, i18n("Click on this button to change the policy for the "
										"domain selected in the list box.") );
  connect( changeDomainPB, SIGNAL( clicked() ), this, SLOT( changePressed() ) );
  
  QPushButton* deleteDomainPB = new QPushButton( i18n("Delete"), domainSpecificVB );
  QWhatsThis::add( deleteDomainPB, i18n("Click on this button to change the policy for the "
										"domain selected in the list box.") );
  connect( deleteDomainPB, SIGNAL( clicked() ), this, SLOT( deletePressed() ) );

  QPushButton* importDomainPB = new QPushButton( i18n("Import..."), domainSpecificVB );
  QWhatsThis::add( importDomainPB, i18n("Click this button to choose the file that contains "
										"the JavaScript policies.  These policies will be merged "
										"with the exisiting ones.  Duplicate enteries are ignored.") );
  connect( importDomainPB, SIGNAL( clicked() ), this, SLOT( importPressed() ) );
  importDomainPB->setEnabled( false );
  
  QPushButton* exportDomainPB = new QPushButton( i18n("Export..."), domainSpecificVB );
  QWhatsThis::add( exportDomainPB, i18n("Click this button to save the JavaScript policy to a zipped "
										"file.  The file, named <b>javascript_policy.tgz</b>, will be "
										"saved to a location of your choice." ) );
  
  connect( exportDomainPB, SIGNAL( clicked() ), this, SLOT( exportPressed() ) );
  exportDomainPB->setEnabled( false );
  
  QWhatsThis::add( domainSpecificGB, i18n("Here you can set specific JavaScript policies for any particular "
										  "domain. To add a new policy, simply click the <i>Add...</i> "
										  "button and supply the necessary information requested by the "
										  "dialog box. To change an exisiting policy, click on the <i>Change...</i> "
										  "button and choose the new policy from the policy dialog box.  Clicking "
										  "on the <i>Delete</i> will remove the selected policy causing the default "
										  "policy setting to be used for that domain. The <i>Import</i> and <i>Export</i> "
										  "button allows you to easily share your policies with other people by allowing "
										  "you to save and retrive them from a zipped file.") );

  // the Java runtime settings
  QVGroupBox* javartGB = new QVGroupBox( i18n( "Java Runtime Settings" ), this );
  toplevel->addWidget( javartGB );

  javaConsoleCB = new QCheckBox( i18n( "Show Java Console" ), javartGB );
  QWhatsThis::add( javaConsoleCB, i18n("FIXME: what is this exactly?") );
  connect( javaConsoleCB, SIGNAL( toggled( bool ) ),
		   this, SLOT( changed() ) );
  QHBox* findJavaHB = new QHBox( javartGB );
  QButtonGroup* dummy = new QButtonGroup( javartGB );
  dummy->hide();
  findJavaHB->setSpacing( 10 );
  autoDetectRB = new QRadioButton( i18n( "&Automatically detect Java" ),
								   findJavaHB );
  connect( autoDetectRB, SIGNAL( toggled( bool ) ),
		   this, SLOT( changed() ) );
  connect( autoDetectRB, SIGNAL( toggled( bool ) ),
		   this, SLOT( toggleJavaControls() ) );
  dummy->insert( autoDetectRB );
  userSpecifiedRB = new QRadioButton( i18n( "Use user-specified Java" ),
									  findJavaHB );
  connect( userSpecifiedRB, SIGNAL( toggled( bool ) ),
		   this, SLOT( changed() ) );
  connect( userSpecifiedRB, SIGNAL( toggled( bool ) ),
		   this, SLOT( toggleJavaControls() ) );
  dummy->insert( userSpecifiedRB );
  wtstr = i18n("If 'Automatically detect Java' is selected, konqueror will try to find "
       "your java installation on its own (this should normally work, if java is somewhere in your path). "
					   "Select 'Use user-specified Java' if konqueror can't find your Java installation or if you have "
					   "several virtual machines installed and want to use a special one. In this case, enter the full path "
					   "to your java installation in the edit field below.");
  QWhatsThis::add( autoDetectRB, wtstr );
  QWhatsThis::add( userSpecifiedRB, wtstr );

  QHBox* pathHB = new QHBox( javartGB );
  pathHB->setSpacing( 10 );
  QLabel* pathLA = new QLabel( i18n( "&Path to JDK" ), pathHB );
  pathED = new QLineEdit( pathHB );
  connect( pathED, SIGNAL( textChanged( const QString& ) ), 
		   this, SLOT( changed() ) );
  pathLA->setBuddy( pathED );
  QHBox* addArgHB = new QHBox( javartGB );
  addArgHB->setSpacing( 10 );
  QLabel* addArgLA = new QLabel( i18n( "Additional Java A&rguments" ), addArgHB  );
  addArgED = new QLineEdit( javartGB );
  connect( addArgED, SIGNAL( textChanged( const QString& ) ), 
		   this, SLOT( changed() ) );
  addArgLA->setBuddy( addArgED );
  wtstr = i18n("If 'Use user-specified Java' is selected, you'll need to enter the path to "
			   "your Java installation here (i.e. /usr/lib/jdk ).");
  QWhatsThis::add( pathED, wtstr );
  wtstr = i18n("If you want special arguments to be passed to the virtual machine, enter them here.");
  QWhatsThis::add( addArgED, wtstr );

  // Finally do the loading
  load();
}


void KJavaScriptOptions::load()
{
    // *** load ***
    m_pConfig->setGroup(m_groupname);
    bool bJavaGlobal = m_pConfig->readBoolEntry( "EnableJava", false);
	bool bJavaScriptGlobal = m_pConfig->readBoolEntry( "EnableJavaScript",
														false );
	bool bJavaConsole = m_pConfig->readBoolEntry( "ShowJavaConsole", false );
	bool bJavaAutoDetect = m_pConfig->readBoolEntry( "JavaAutoDetect", true );
	QString sJDKArgs = m_pConfig->readEntry( "JavaArgs", "" );
    QString sJDK = m_pConfig->readEntry( "JavaPath", "/usr/lib/jdk" );

	updateDomainList(m_pConfig->readListEntry("JavaScriptDomainAdvice"));
	changeJavaEnabled();
	changeJavaScriptEnabled();

    // *** apply to GUI ***
    enableJavaGloballyCB->setChecked( bJavaGlobal );
	enableJavaScriptGloballyCB->setChecked( bJavaScriptGlobal );
	javaConsoleCB->setChecked( bJavaConsole );
	if( bJavaAutoDetect )
	  autoDetectRB->setChecked( true );
	else
	  userSpecifiedRB->setChecked( true );

	addArgED->setText( sJDKArgs );
	pathED->setText( sJDK );

	toggleJavaControls();
}

void KJavaScriptOptions::defaults()
{
  enableJavaGloballyCB->setChecked( false );
  enableJavaScriptGloballyCB->setChecked( false );
  javaConsoleCB->setChecked( false );
  autoDetectRB->setChecked( true );
  pathED->setText( "/usr/lib/jdk" );
  addArgED->setText( "" );

  toggleJavaControls();
}

void KJavaScriptOptions::save()
{
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry( "EnableJava", enableJavaGloballyCB->isChecked());
    m_pConfig->writeEntry( "EnableJavaScript", enableJavaScriptGloballyCB->isChecked());
	m_pConfig->writeEntry( "ShowJavaConsole", javaConsoleCB->isChecked() );
	m_pConfig->writeEntry( "JavaAutoDetect", autoDetectRB->isChecked() );
	m_pConfig->writeEntry( "JavaArgs", addArgED->text() );
	m_pConfig->writeEntry( "JavaPath", pathED->text() );

	QStringList domainConfig;
	QListViewItemIterator it( domainSpecificLV );
	QListViewItem* current;
	while( ( current = it.current() ) ) {
	  ++it;
	  domainConfig.append(QString::fromLatin1("%1:%2:%3").arg(current->text(0)).arg(javaDomainPolicy[current]).arg(javaScriptDomainPolicy[current]));
	}
	m_pConfig->writeEntry("JavaScriptDomainAdvice", domainConfig);
	
    m_pConfig->sync();
}

void KJavaScriptOptions::changed()
{
  emit KCModule::changed(true);
}


void KJavaScriptOptions::toggleJavaControls()
{
  bool isEnabled = enableJavaGloballyCB->isChecked();
  javaConsoleCB->setEnabled(isEnabled);
  addArgED->setEnabled(isEnabled);
  autoDetectRB->setEnabled(isEnabled);
  userSpecifiedRB->setEnabled(isEnabled);
  pathED->setEnabled(isEnabled && userSpecifiedRB->isChecked());
}

void KJavaScriptOptions::addPressed()
{
    PolicyDialog pDlg( this, 0L );
    // We subtract one from the enum value because
    // KJavaScriptDunno is not part of the choice list.
    int def_javapolicy = KHTMLSettings::KJavaScriptReject - 1;
    int def_javascriptpolicy = KHTMLSettings::KJavaScriptReject - 1;
    pDlg.setDefaultPolicy( def_javapolicy, def_javascriptpolicy );
    pDlg.setCaption( i18n( "New Java/JavaScript Policy" ) );
    if( pDlg.exec() ) {
	  QListViewItem* index = new QListViewItem( domainSpecificLV, pDlg.domain(),
												KHTMLSettings::adviceToStr( (KHTMLSettings::KJavaScriptAdvice)
															 pDlg.javaPolicyAdvice() ),
												KHTMLSettings::adviceToStr( (KHTMLSettings::KJavaScriptAdvice)
															 pDlg.javaScriptPolicyAdvice() ) );
	  javaDomainPolicy.insert( index, KHTMLSettings::adviceToStr( (KHTMLSettings::KJavaScriptAdvice)pDlg.javaPolicyAdvice() ) );
	  javaScriptDomainPolicy.insert( index, KHTMLSettings::adviceToStr( (KHTMLSettings::KJavaScriptAdvice)pDlg.javaScriptPolicyAdvice() ) );
	  domainSpecificLV->setCurrentItem( index );
	  changed();
    }
}

void KJavaScriptOptions::changePressed()
{
    QListViewItem *index = domainSpecificLV->currentItem();
    if ( index == 0 )
    {
        KMessageBox::information( 0, i18n("You must first select a policy to be changed!" ) );
        return;
    }

    KHTMLSettings::KJavaScriptAdvice javaAdvice = KHTMLSettings::strToAdvice(javaDomainPolicy[index]);
    KHTMLSettings::KJavaScriptAdvice javaScriptAdvice = KHTMLSettings::strToAdvice(javaScriptDomainPolicy[index]);

    PolicyDialog pDlg( this );
    pDlg.setDisableEdit( false, index->text(0) );
    pDlg.setCaption( i18n( "Change Java/JavaScript Policy" ) );
    // We subtract one from the enum value because
    // KJavaScriptDunno is not part of the choice list.
    pDlg.setDefaultPolicy( javaAdvice - 1, javaScriptAdvice - 1  );
    if( pDlg.exec() )
    {
      javaDomainPolicy[index] =
		KHTMLSettings::adviceToStr((KHTMLSettings::KJavaScriptAdvice)pDlg.javaPolicyAdvice());
	  javaScriptDomainPolicy[index] =
		KHTMLSettings::adviceToStr((KHTMLSettings::KJavaScriptAdvice)pDlg.javaScriptPolicyAdvice() );
      index->setText(1, i18n(javaDomainPolicy[index]) );
	  index->setText(2, i18n(javaScriptDomainPolicy[index] ));
      changed();
    }
}

void KJavaScriptOptions::deletePressed()
{
    QListViewItem *index = domainSpecificLV->currentItem();
    if ( index == 0 )
    {
        KMessageBox::information( 0, i18n("You must first select a policy to delete!" ) );
        return;
    }
    javaDomainPolicy.remove(index);
    javaScriptDomainPolicy.remove(index);
    delete index;
    changed();
}

void KJavaScriptOptions::importPressed()
{
  // PENDING(kalle) Implement this.
}

void KJavaScriptOptions::exportPressed()
{
  // PENDING(kalle) Implement this.
}

void KJavaScriptOptions::changeJavaEnabled()
{
  bool enabled = enableJavaGloballyCB->isChecked();
  enableJavaGloballyCB->setChecked( enabled );
}

void KJavaScriptOptions::changeJavaScriptEnabled()
{
  bool enabled = enableJavaScriptGloballyCB->isChecked();
  enableJavaScriptGloballyCB->setChecked( enabled );
}

void KJavaScriptOptions::updateDomainList(const QStringList &domainConfig)
{
    for (QStringList::ConstIterator it = domainConfig.begin();
         it != domainConfig.end(); ++it) {
      QString domain;
      KHTMLSettings::KJavaScriptAdvice javaAdvice;
	  KHTMLSettings::KJavaScriptAdvice javaScriptAdvice;
      KHTMLSettings::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
      QCString javaAdvStr = KHTMLSettings::adviceToStr(javaAdvice);
	  QCString javaScriptAdvStr = KHTMLSettings::adviceToStr(javaScriptAdvice);
      QListViewItem *index =
        new QListViewItem( domainSpecificLV, domain, i18n(javaAdvStr), 
						   i18n( javaScriptAdvStr ) );
      javaDomainPolicy[index] = javaAdvStr;
      javaScriptDomainPolicy[index] = javaScriptAdvStr;
    }
}


#include "htmlopts.moc"




