//
// Three tabs for HTML Options
//
// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998

// KControl port & modifications
// (c) Torben Weis 1998
// End of the KControl port, added 'kfmclient configure' call.
// (c) David Faure 1998

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcolor.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qwhatsthis.h>
#include <qlineedit.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kconfig.h>
#include <X11/Xlib.h>

#include "htmlopts.h"

#include <konqdefaults.h> // include default values directly from konqueror
#include <klocale.h>

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

    label = new QLabel( i18n("Background Color:"), this );
    lay->addWidget( label, ++r, E);

    m_pBg = new KColorButton( bgColor, this );
    lay->addWidget(m_pBg, r, M);

    wtstr = i18n("This is the default background color for web pages. Note that if 'Always use my colors' "
       "is not selected, this value can be overridden by a differing web page.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pBg, wtstr );

    connect( m_pBg, SIGNAL( changed( const QColor & ) ),
             SLOT( slotBgColorChanged( const QColor & ) ) );
    connect( m_pBg, SIGNAL( changed( const QColor & ) ),
             SLOT( changed() ) );

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

    bgColor = KGlobalSettings::baseColor();
    bgColor = m_pConfig->readColorEntry( "BgColor", &bgColor );
    textColor = KGlobalSettings::textColor();
    textColor = m_pConfig->readColorEntry( "TextColor", &textColor );
    linkColor = KGlobalSettings::linkColor();
    linkColor = m_pConfig->readColorEntry( "LinkColor", &linkColor );
    vLinkColor = KGlobalSettings::visitedLinkColor();
    vLinkColor = m_pConfig->readColorEntry( "VLinkColor", &vLinkColor);
    bool forceDefaults = m_pConfig->readBoolEntry("ForceDefaultColors", false);

    m_pBg->setColor( bgColor );
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

    m_pBg->setColor( bgColor );
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
    m_pConfig->writeEntry( "BgColor", bgColor );
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

void KAppearanceOptions::slotBgColorChanged( const QColor &col )
{
    if ( bgColor != col )
        bgColor = col;
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


KAdvancedOptions::KAdvancedOptions(KConfig *config, QString group, QWidget *parent, const char *name )
    : KCModule( parent, name ), m_pConfig(config), m_groupname(group)
{
    QVBoxLayout *lay = new QVBoxLayout(this, 10, 5);

    QGroupBox *javaScript = new QGroupBox(i18n("Java Script"), this);
    cb_enableJavaScript = new QCheckBox(i18n("Enable Java&Script"), javaScript);

    QVBoxLayout *vbox = new QVBoxLayout(javaScript, 5, 5);
    vbox->addSpacing(10);
    vbox->addWidget(cb_enableJavaScript);

    QWhatsThis::add( cb_enableJavaScript, i18n("Enables the execution of scripts written in ECMA-Script "
        "(as known as JavaScript) that can be contained in HTML pages. Be aware that JavaScript support "
        "is not yet finished. Note that, as with any browser, enabling scripting languages can be a security problem.") );

    lay->addWidget(javaScript);

    connect(cb_enableJavaScript, SIGNAL(clicked()), this, SLOT(changed()));

    lay->setSpacing(10);

    QGroupBox *java = new QGroupBox(i18n("Java"), this);
    cb_enableJava = new QCheckBox(i18n("Enable &Java"), java);
    cb_showJavaConsole = new QCheckBox(i18n("Show Java Console"), java);
    lb_JavaArgs = new QLabel(i18n("Additional Java arguments"), java);
    le_JavaArgs = new QLineEdit(java);
    QButtonGroup *bg = new QButtonGroup();
    rb_autoDetect = new QRadioButton( i18n("Automatically detect Java"), java );
    rb_userDetect = new QRadioButton( i18n("Use user-specified Java"), java );
    bg->insert(rb_autoDetect);
    bg->insert(rb_userDetect);

    bg->setExclusive( TRUE );

    lb_JavaPath = new QLabel(i18n("Path to JDK"),java);
    le_JavaPath = new QLineEdit(java);

    QVBoxLayout *vbox2 = new QVBoxLayout(java, 5, 5);
    vbox2->addSpacing(10);
    vbox2->addWidget(cb_enableJava);
    vbox2->addWidget(cb_showJavaConsole);

    QHBoxLayout *hlay3 = new QHBoxLayout(10);
    hlay3->addWidget(rb_autoDetect);
    hlay3->addWidget(rb_userDetect);
    vbox2->addLayout(hlay3);

    QHBoxLayout *hlay = new QHBoxLayout(10);
    hlay->addWidget(lb_JavaPath, 1);
    hlay->addWidget(le_JavaPath, 5);

    vbox2->addLayout(hlay);

    QHBoxLayout *hlay2 = new QHBoxLayout(10);
    hlay2->addWidget(lb_JavaArgs, 1);
    hlay2->addWidget(le_JavaArgs, 5);

    vbox2->addLayout(hlay2);

    QWhatsThis::add( cb_enableJava, i18n("This option enables konqueror's support for java applets.") );
    QWhatsThis::add( cb_showJavaConsole, i18n("FIXME: what is this exactly?") );
    QString wtstr = i18n("If 'Automatically detect Java' is selected, konqueror will try to find "
       "your java installation on its own (this should normally work, if java is somewhere in your path). "
       "Select 'Use user-specified Java' if konqueror can't find your Java installation or if you have "
       "several virtual machines installed and want to use a special one. In this case, enter the full path "
       "to your java installation in the edit field below.");
    QWhatsThis::add( rb_autoDetect, wtstr );
    QWhatsThis::add( rb_userDetect, wtstr );
    wtstr = i18n("If 'Use user-specified Java' is selected, you'll need to enter the path to "
       "your Java installation here (i.e. /usr/lib/jdk ).");
    QWhatsThis::add( lb_JavaPath, wtstr );
    QWhatsThis::add( le_JavaPath, wtstr );
    wtstr = i18n("If you want special arguments to be passed to the virtual machine, enter them here.");
    QWhatsThis::add( lb_JavaArgs, wtstr );
    QWhatsThis::add( le_JavaArgs, wtstr );

    lay->addWidget(java);

    // Change
    connect(bg, SIGNAL(clicked(int)), this, SLOT(changed()));
    connect(cb_enableJava, SIGNAL(clicked()), this, SLOT(changed()));
    connect(cb_showJavaConsole, SIGNAL(clicked()), this, SLOT(changed()));
    connect(le_JavaPath, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
    connect(le_JavaArgs, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

    connect(cb_enableJava, SIGNAL(clicked()), this, SLOT(toggleJavaControls()));
    connect(bg, SIGNAL(clicked(int)), this, SLOT(toggleJavaControls()));

    lay->addStretch(10);
    lay->activate();

    load();
}

void KAdvancedOptions::load()
{
    // *** load ***
    m_pConfig->setGroup(m_groupname);
    bool bJavaScript = m_pConfig->readBoolEntry( "EnableJavaScript", false);
    bool bJava = m_pConfig->readBoolEntry( "EnableJava", false);
    bool bJavaConsole = m_pConfig->readBoolEntry( "ShowJavaConsole", false);
    bool bJavaAutoDetect = m_pConfig->readBoolEntry( "JavaAutoDetect", true);
    QString sJDKArgs = m_pConfig->readEntry( "JavaArgs", "" );
    QString sJDK = m_pConfig->readEntry( "JavaPath", "/usr/lib/jdk" );

    // *** apply to GUI ***

    cb_enableJavaScript->setChecked(bJavaScript);
    cb_enableJava->setChecked(bJava);
    cb_showJavaConsole->setChecked(bJavaConsole);

    if(bJavaAutoDetect)
      rb_autoDetect->setChecked(true);
    else
      rb_userDetect->setChecked(true);

    le_JavaArgs->setText(sJDKArgs);
    le_JavaPath->setText(sJDK);

    toggleJavaControls();
}

void KAdvancedOptions::defaults()
{
    cb_enableJavaScript->setChecked(false);
    cb_enableJava->setChecked(false);
    cb_showJavaConsole->setChecked(false);
    rb_autoDetect->setChecked(true);
    le_JavaPath->setText("/usr/lib/jdk");
    le_JavaArgs->setText("");

    toggleJavaControls();
}

void KAdvancedOptions::save()
{
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry( "EnableJavaScript", cb_enableJavaScript->isChecked());
    m_pConfig->writeEntry( "EnableJava", cb_enableJava->isChecked());
    m_pConfig->writeEntry( "ShowJavaConsole", cb_showJavaConsole->isChecked());
    m_pConfig->writeEntry( "JavaAutoDetect", rb_autoDetect->isChecked());
    m_pConfig->writeEntry( "JavaArgs", le_JavaArgs->text());
    m_pConfig->writeEntry( "JavaPath", le_JavaPath->text());
    m_pConfig->sync();
}

void KAdvancedOptions::changed()
{
  emit KCModule::changed(true);
}

void KAdvancedOptions::toggleJavaControls()
{
  bool isEnabled = cb_enableJava->isChecked();
  cb_showJavaConsole->setEnabled(isEnabled);
  lb_JavaArgs->setEnabled(isEnabled);
  le_JavaArgs->setEnabled(isEnabled);
  rb_autoDetect->setEnabled(isEnabled);
  rb_userDetect->setEnabled(isEnabled);
  lb_JavaPath->setEnabled(isEnabled && rb_userDetect->isChecked());
  le_JavaPath->setEnabled(isEnabled && rb_userDetect->isChecked());
}

#include "htmlopts.moc"




