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
#include <qlineedit.h>
#include <kglobal.h>
#include <kconfig.h>
#include <X11/Xlib.h>

#include "htmlopts.h"

#include <konqdefaults.h> // include default values directly from konqueror
#include <klocale.h>

extern KConfig *g_pConfig;
extern QString g_groupname;


//-----------------------------------------------------------------------------

KAppearanceOptions::KAppearanceOptions(KConfig *config, QString group, QWidget *parent, const char *name)
    : KCModule( parent, name ), g_pConfig(config), groupname(group)
{

    QGridLayout *lay = new QGridLayout(this, 1 ,1 , 10, 5);
    int r = 0;
    int E = 0, M = 1, W = 3; //CT 3 (instead 2) allows smaller color buttons

    QButtonGroup *bg = new QButtonGroup( 1, QGroupBox::Vertical,
					 i18n("Font Size"), this );
    lay->addMultiCellWidget(bg, r, r, E, W);

    bg->setExclusive( TRUE );
    connect(bg, SIGNAL(clicked(int)), this, SLOT(changed()));

    m_pSmall = new QRadioButton( i18n("Small"), bg );
    m_pMedium = new QRadioButton( i18n("Medium"), bg );
    m_pLarge = new QRadioButton( i18n("Large"), bg );

    lay->addWidget(new QLabel( i18n("Standard Font"), this ), ++r, E);

    m_pStandard = new QComboBox( false, this );
    lay->addMultiCellWidget(m_pStandard, r, r, M, W);

    getFontList( standardFonts, "-*-*-*-*-*-*-*-*-*-*-p-*-*-*" );
    m_pStandard->insertStrList( &standardFonts );
    connect( m_pStandard, SIGNAL( activated(const QString&) ),
             SLOT( slotStandardFont(const QString&) ) );
    connect( m_pStandard, SIGNAL( activated(const QString&) ),
             SLOT(changed() ) );

    lay->addWidget(new QLabel( i18n( "Fixed Font"), this ), ++r, E);

    m_pFixed = new QComboBox( false, this );
    lay->addMultiCellWidget(m_pFixed, r, r, M, W);
    getFontList( fixedFonts, "-*-*-*-*-*-*-*-*-*-*-m-*-*-*" );
    m_pFixed->insertStrList( &fixedFonts );

    connect( m_pFixed, SIGNAL( activated(const QString&) ),
             SLOT( slotFixedFont(const QString&) ) );
    connect( m_pFixed, SIGNAL( activated(const QString&) ),
             SLOT(changed() ) );

    // default charset Lars Knoll 17Nov98 (moved by David)
    lay->addWidget(new QLabel( i18n( "Default Charset"), this ), ++r, E);

    m_pCharset = new QComboBox( false, this );
#ifdef __GNUC__
#warning FIXME, this seems to be broken in kcharsets (Simon)
#endif
//    charsets = KGlobal::charsets()->availableCharsetNames();
    charsets.prepend(i18n("Use language charset"));
    m_pCharset->insertStringList( charsets );
    lay->addMultiCellWidget(m_pCharset,r, r, M, W);
    
    connect( m_pCharset, SIGNAL( activated(const QString& ) ),
             SLOT( slotCharset(const QString&) ) );
    connect( m_pCharset, SIGNAL( activated(const QString& ) ),
             SLOT(changed() ) );

    connect( bg, SIGNAL( clicked( int ) ), SLOT( slotFontSize( int ) ) );

    lay->addWidget(new QLabel( i18n("Background Color:"), this ), ++r, E);

    m_pBg = new KColorButton( bgColor, this );
    lay->addWidget(m_pBg, r, M);
    connect( m_pBg, SIGNAL( changed( const QColor & ) ),
             SLOT( slotBgColorChanged( const QColor & ) ) );
    connect( m_pBg, SIGNAL( changed( const QColor & ) ),
             SLOT( changed() ) );

    lay->addWidget(new QLabel( i18n("Normal Text Color:"), this ), ++r, E);

    m_pText = new KColorButton( textColor, this );
    lay->addWidget(m_pText, r, M);
    connect( m_pText, SIGNAL( changed( const QColor & ) ),
             SLOT( slotTextColorChanged( const QColor & ) ) );
    connect( m_pText, SIGNAL( changed( const QColor & ) ),
             SLOT( changed() ) );

    lay->addWidget(new QLabel( i18n("URL Link Color:"), this ), ++r, E);

    m_pLink = new KColorButton( linkColor, this );
    lay->addWidget(m_pLink, r, M);
    connect( m_pLink, SIGNAL( changed( const QColor & ) ),
             SLOT( slotLinkColorChanged( const QColor & ) ) );
    connect( m_pLink, SIGNAL( changed( const QColor & ) ),
             SLOT( changed() ) );

    lay->addWidget(new QLabel( i18n("Followed Link Color:"), this ), ++r, E);

    m_pVLink = new KColorButton( vLinkColor, this );
    lay->addWidget(m_pVLink, r, M);
    connect( m_pVLink, SIGNAL( changed( const QColor & ) ),
             SLOT( slotVLinkColorChanged( const QColor & ) ) );
    connect( m_pVLink, SIGNAL( changed( const QColor & ) ),
             SLOT( changed() ) );

    forceDefaultsbox = new QCheckBox(i18n("Always use my colors"), this);
    r++; lay->addMultiCellWidget(forceDefaultsbox, r, r, E, W);
    connect(forceDefaultsbox, SIGNAL(clicked()), this, SLOT(changed()));

    r++; lay->setRowStretch(r, 10);
    load();
}

void KAppearanceOptions::getFontList( QStrList &list, const char *pattern )
{
    int num;

    char **xFonts = XListFonts( qt_xdisplay(), pattern, 2000, &num );

    for ( int i = 0; i < num; i++ )
    {
        addFont( list, xFonts[i] );
    }

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
    g_pConfig->setGroup(groupname);
    QString fs = g_pConfig->readEntry( "BaseFontSize" );
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

    stdName = g_pConfig->readEntry( "StandardFont" );
    fixedName = g_pConfig->readEntry( "FixedFont" );
    charsetName = g_pConfig->readEntry( "DefaultCharset" );

    bgColor = g_pConfig->readColorEntry( "BgColor", &HTML_DEFAULT_BG_COLOR );
    textColor = g_pConfig->readColorEntry( "TextColor", &HTML_DEFAULT_TXT_COLOR );
    linkColor = g_pConfig->readColorEntry( "LinkColor", &HTML_DEFAULT_LNK_COLOR );
    vLinkColor = g_pConfig->readColorEntry( "VLinkColor", &HTML_DEFAULT_VLNK_COLOR);
    bool forceDefaults = g_pConfig->readBoolEntry("ForceDefaultColors", false);

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
    stdName = KGlobal::generalFont().family();
    fixedName = KGlobal::fixedFont().family();
    charsetName = "";

    bgColor = HTML_DEFAULT_BG_COLOR;
    textColor = HTML_DEFAULT_TXT_COLOR;
    linkColor = HTML_DEFAULT_LNK_COLOR;
    vLinkColor = HTML_DEFAULT_VLNK_COLOR;

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
        stdName = KGlobal::generalFont().family();
    if ( fixedName.isEmpty() )
        fixedName = KGlobal::fixedFont().family();

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
    g_pConfig->setGroup(groupname);			
    g_pConfig->writeEntry( "BaseFontSize", fSize );
    g_pConfig->writeEntry( "StandardFont", stdName );
    g_pConfig->writeEntry( "FixedFont", fixedName );
    // If the user chose "Use language charset", write an empty string
    if (charsetName == i18n("Use language charset"))
        charsetName = "";
    g_pConfig->writeEntry( "DefaultCharset", charsetName );
    g_pConfig->writeEntry( "BgColor", bgColor );
    g_pConfig->writeEntry( "TextColor", textColor );
    g_pConfig->writeEntry( "LinkColor", linkColor);
    g_pConfig->writeEntry( "VLinkColor", vLinkColor );
    g_pConfig->writeEntry("ForceDefaultColors", forceDefaultsbox->isChecked() );
    g_pConfig->sync();
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
    : KCModule( parent, name ), g_pConfig(config), groupname(group)
{
    QVBoxLayout *lay = new QVBoxLayout(this, 10, 5);
    
    QGroupBox *javaScript = new QGroupBox(i18n("Java Script"), this);
    cb_enableJavaScript = new QCheckBox(i18n("Enable Java&Script"), javaScript);
    
    QVBoxLayout *vbox = new QVBoxLayout(javaScript, 5, 5);
    vbox->addSpacing(10);    
    vbox->addWidget(cb_enableJavaScript);
    
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
    g_pConfig->setGroup(groupname);
    bool bJavaScript = g_pConfig->readBoolEntry( "EnableJavaScript", false);
    bool bJava = g_pConfig->readBoolEntry( "EnableJava", false);
    bool bJavaConsole = g_pConfig->readBoolEntry( "ShowJavaConsole", false);
    bool bJavaAutoDetect = g_pConfig->readBoolEntry( "JavaAutoDetect", true);
    QString sJDKArgs = g_pConfig->readEntry( "JavaArgs", "" );
    QString sJDK = g_pConfig->readEntry( "JavaPath", "/usr/lib/jdk" );

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
    g_pConfig->setGroup(groupname);
    g_pConfig->writeEntry( "EnableJavaScript", cb_enableJavaScript->isChecked());
    g_pConfig->writeEntry( "EnableJava", cb_enableJava->isChecked());
    g_pConfig->writeEntry( "ShowJavaConsole", cb_showJavaConsole->isChecked());
    g_pConfig->writeEntry( "JavaAutoDetect", rb_autoDetect->isChecked());
    g_pConfig->writeEntry( "JavaArgs", le_JavaArgs->text());
    g_pConfig->writeEntry( "JavaPath", le_JavaPath->text());
    g_pConfig->sync();
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




