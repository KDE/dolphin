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
#include <qspinbox.h>
#include <kdebug.h>

#include <X11/Xlib.h>

#include "htmlopts.h"
#include "policydlg.h"

#include <konqdefaults.h> // include default values directly from konqueror
#include <klocale.h>
#include <khtml_settings.h>
#include <khtmldefaults.h>

//-----------------------------------------------------------------------------

KAppearanceOptions::KAppearanceOptions(KConfig *config, QString group, QWidget *parent, const char *name)
    : KCModule( parent, name ), m_pConfig(config), m_groupname(group)
{
    QString wtstr;

    QGridLayout *lay = new QGridLayout(this, 1 ,1 , 9, 5);
    int r = 0;
    int E = 0, M = 1, W = 3; //CT 3 (instead 2) allows smaller color buttons

    QButtonGroup *bg = new QButtonGroup( 1, QGroupBox::Vertical,
					 i18n("Font Size"), this );
    lay->addMultiCellWidget(bg, r, r, E, W);

    QWhatsThis::add( bg, i18n("This is the relative font size Konqueror uses to display web sites.") );

    bg->setExclusive( TRUE );
    connect(bg, SIGNAL(clicked(int)), this, SLOT(changed()));

    m_pSmall = new QRadioButton( i18n("Small"), bg );
    m_pMedium = new QRadioButton( i18n("Medium"), bg );
    m_pLarge = new QRadioButton( i18n("Large"), bg );

	QLabel* minSizeLA = new QLabel( i18n( "Minimum Font Size" ), this );
	lay->addWidget( minSizeLA, ++r, E );
	
	minSizeSB = new QSpinBox( this );
	connect( minSizeSB, SIGNAL( valueChanged( int ) ),
			 this, SLOT( slotMinimumFontSize( int ) ) );
	connect( minSizeSB, SIGNAL( valueChanged( int ) ),
			 this, SLOT( changed() ) );
	lay->addWidget( minSizeSB, r, r, M );
	QWhatsThis::add( minSizeSB, i18n( "Konqueror will never display text smaller than this size,<br> no matter the web site settings" ) );

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

	label = new QLabel( i18n( "SerifFont" ), this );
	lay->addWidget( label, ++r, E );

	m_pSerif = new QComboBox( false, this );
	lay->addMultiCellWidget( m_pSerif, r, r, M, W );
    getFontList( serifFonts, "-*-*-*-*-*-*-*-*-*-*-p-*-*-*" );
    m_pSerif->insertStrList( &serifFonts );

	wtstr= i18n( "This is the font used to display text that is marked up as serif." );
	QWhatsThis::add( label, wtstr );
	QWhatsThis::add( m_pSerif, wtstr );

	connect( m_pSerif, SIGNAL( activated( const QString& ) ),
			 SLOT( slotSerifFont( const QString& ) ) );
	connect( m_pFixed, SIGNAL( activated( const QString& ) ),
			 SLOT( changed() ) );

	label = new QLabel( i18n( "SansSerifFont" ), this );
	lay->addWidget( label, ++r, E );

	m_pSansSerif = new QComboBox( false, this );
	lay->addMultiCellWidget( m_pSansSerif, r, r, M, W );
    getFontList( sansSerifFonts, "-*-*-*-*-*-*-*-*-*-*-p-*-*-*" );
    m_pSansSerif->insertStrList( &sansSerifFonts );

	wtstr= i18n( "This is the font used to display text that is marked up as sans-serif." );
	QWhatsThis::add( label, wtstr );
	QWhatsThis::add( m_pSansSerif, wtstr );

	connect( m_pSansSerif, SIGNAL( activated( const QString& ) ),
			 SLOT( slotCursiveFont( const QString& ) ) );
	connect( m_pFixed, SIGNAL( activated( const QString& ) ),
			 SLOT( changed() ) );
	
	label = new QLabel( i18n( "CursiveFont" ), this );
	lay->addWidget( label, ++r, E );

	m_pCursive = new QComboBox( false, this );
	lay->addMultiCellWidget( m_pCursive, r, r, M, W );
    getFontList( cursiveFonts, "-*-*-*-*-*-*-*-*-*-*-p-*-*-*" );
    m_pCursive->insertStrList( &cursiveFonts );

	wtstr= i18n( "This is the font used to display text that is marked up as italic." );
	QWhatsThis::add( label, wtstr );
	QWhatsThis::add( m_pCursive, wtstr );

	connect( m_pCursive, SIGNAL( activated( const QString& ) ),
			 SLOT( slotCursiveFont( const QString& ) ) );
	connect( m_pCursive, SIGNAL( activated( const QString& ) ),
			 SLOT( changed() ) );

	label = new QLabel( i18n( "FantasyFont" ), this );
	lay->addWidget( label, ++r, E );

	m_pFantasy = new QComboBox( false, this );
	lay->addMultiCellWidget( m_pFantasy, r, r, M, W );
    getFontList( fantasyFonts, "-*-*-*-*-*-*-*-*-*-*-p-*-*-*" );
    m_pFantasy->insertStrList( &fantasyFonts );

	wtstr= i18n( "This is the font used to display text that is marked up as a fantasy font." );
	QWhatsThis::add( label, wtstr );
	QWhatsThis::add( m_pFantasy, wtstr );

	connect( m_pSerif, SIGNAL( activated( const QString& ) ),
			 SLOT( slotFantasyFont( const QString& ) ) );
	connect( m_pFixed, SIGNAL( activated( const QString& ) ),
			 SLOT( changed() ) );


    label = new QLabel( i18n( "Default Encoding"), this );
    lay->addWidget( label, ++r, E);

    m_pEncoding = new QComboBox( false, this );
    encodings = KGlobal::charsets()->availableEncodingNames();
    encodings.prepend(i18n("Use language encoding"));
    m_pEncoding->insertStringList( encodings );
    lay->addMultiCellWidget(m_pEncoding,r, r, M, W);

    wtstr = i18n("Select the default encoding to be used. Normally, you'll be fine with 'Use language encoding' "
       "and should not have to change this.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pEncoding, wtstr );

    connect( m_pEncoding, SIGNAL( activated(const QString& ) ),
             SLOT( slotEncoding(const QString&) ) );
    connect( m_pEncoding, SIGNAL( activated(const QString& ) ),
             SLOT(changed() ) );

    connect( bg, SIGNAL( clicked( int ) ), SLOT( slotFontSize( int ) ) );

    r++; lay->setRowStretch(r, 8);
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
    fSize = i;
}


void KAppearanceOptions::slotMinimumFontSize( int i )
{
  fMinSize = i;
}


void KAppearanceOptions::slotStandardFont(const QString& n )
{
    stdName = n;
}


void KAppearanceOptions::slotFixedFont(const QString& n )
{
    fixedName = n;
}


void KAppearanceOptions::slotSerifFont( const QString& n )
{
  serifName = n;
}


void KAppearanceOptions::slotSansSerifFont( const QString& n )
{
  sansSerifName = n;
}


void KAppearanceOptions::slotCursiveFont( const QString& n )
{
  cursiveName = n;
}


void KAppearanceOptions::slotFantasyFont( const QString& n )
{
  fantasyName = n;
}


void KAppearanceOptions::slotEncoding(const QString& n)
{
    encodingName = n;
}

void KAppearanceOptions::load()
{
    m_pConfig->setGroup(m_groupname);
    fSize = m_pConfig->readNumEntry( "FontSize", 1 ); // medium
    fMinSize = m_pConfig->readNumEntry( "MinimumFontSize", HTML_DEFAULT_MIN_FONT_SIZE );

    stdName = m_pConfig->readEntry( "StandardFont", KGlobalSettings::generalFont().family() );
    fixedName = m_pConfig->readEntry( "FixedFont", KGlobalSettings::fixedFont().family() );
    serifName = m_pConfig->readEntry( "SerifFont", HTML_DEFAULT_VIEW_SERIF_FONT );
    sansSerifName = m_pConfig->readEntry( "SansSerifFont", HTML_DEFAULT_VIEW_SANSSERIF_FONT );
    cursiveName = m_pConfig->readEntry( "CursiveFont", HTML_DEFAULT_VIEW_CURSIVE_FONT );
    fantasyName = m_pConfig->readEntry( "FantasyFont", HTML_DEFAULT_VIEW_FANTASY_FONT );
    encodingName = m_pConfig->readEntry( "DefaultEncoding", "" );
    kdDebug(0) << "encoding = " << encodingName << endl;

    updateGUI();
}

void KAppearanceOptions::defaults()
{
  fSize=1; // medium
  fMinSize = HTML_DEFAULT_MIN_FONT_SIZE;
  stdName = KGlobalSettings::generalFont().family();
  fixedName = KGlobalSettings::fixedFont().family();
  serifName = HTML_DEFAULT_VIEW_SERIF_FONT;
  sansSerifName = HTML_DEFAULT_VIEW_SANSSERIF_FONT;
  cursiveName = HTML_DEFAULT_VIEW_CURSIVE_FONT;
  fantasyName = HTML_DEFAULT_VIEW_FANTASY_FONT;

  encodingName = "";

  updateGUI();
}

void KAppearanceOptions::updateGUI()
{
    if ( stdName.isEmpty() )
        stdName = KGlobalSettings::generalFont().family();
    if ( fixedName.isEmpty() )
        fixedName = KGlobalSettings::fixedFont().family();
	if( serifName.isEmpty() )
	  serifName = HTML_DEFAULT_VIEW_SERIF_FONT;
	if( sansSerifName.isEmpty() )
	  sansSerifName = HTML_DEFAULT_VIEW_SANSSERIF_FONT;
	if( cursiveName.isEmpty() )
	  cursiveName = HTML_DEFAULT_VIEW_CURSIVE_FONT;
	if( fantasyName.isEmpty() )
	  fantasyName = HTML_DEFAULT_VIEW_FANTASY_FONT;

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

    QStrListIterator seit( serifFonts );
    for ( i = 0; seit.current(); ++seit, i++ )
    {
        if ( serifName == seit.current() )
            m_pSerif->setCurrentItem( i );
    }

    QStrListIterator ssit( sansSerifFonts );
    for ( i = 0; ssit.current(); ++ssit, i++ )
    {
        if ( sansSerifName == ssit.current() )
            m_pSansSerif->setCurrentItem( i );
    }

    QStrListIterator cit( cursiveFonts );
    for ( i = 0; cit.current(); ++cit, i++ )
    {
        if ( cursiveName == cit.current() )
            m_pCursive->setCurrentItem( i );
    }

    QStrListIterator fait( fantasyFonts );
    for ( i = 0; fait.current(); ++fait, i++ )
    {
        if ( fantasyName == fait.current() )
            m_pFantasy->setCurrentItem( i );
    }

    i = 0;
    for ( QStringList::Iterator it = encodings.begin(); it != encodings.end(); ++it, ++i )
    {
        if ( encodingName == *it )
            m_pEncoding->setCurrentItem( i );
    }

    m_pSmall->setChecked( fSize == 0 );
    m_pMedium->setChecked( fSize == 1 );
    m_pLarge->setChecked( fSize == 2 );

	minSizeSB->setValue( fMinSize );
}

void KAppearanceOptions::save()
{
    m_pConfig->setGroup(m_groupname);			
    m_pConfig->writeEntry( "FontSize", fSize );
	m_pConfig->writeEntry( "MinimumFontSize", fMinSize );
    m_pConfig->writeEntry( "StandardFont", stdName );
    m_pConfig->writeEntry( "FixedFont", fixedName );
	m_pConfig->writeEntry( "SerifFont", serifName );
	m_pConfig->writeEntry( "SansSerifFont", sansSerifName );
	m_pConfig->writeEntry( "CursiveFont", cursiveName );
	m_pConfig->writeEntry( "FantasyFont", fantasyName );
    // If the user chose "Use language encoding", write an empty string
    if (encodingName == i18n("Use language encoding"))
        encodingName = "";
    m_pConfig->writeEntry( "DefaultEncoding", encodingName );
    m_pConfig->sync();
}


void KAppearanceOptions::changed()
{
  emit KCModule::changed(true);
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
  addArgED = new QLineEdit( addArgHB );
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




