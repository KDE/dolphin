//
// KFM  Options
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
#include <qlabel.h>
#include <qlayout.h>//CT - 12Nov1998
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <kglobal.h>
#include <kconfig.h>
#include <X11/Xlib.h>

#include "htmlopts.h"

#include "defaults.h"
#include <klocale.h> // include default values directly from kfm

//-----------------------------------------------------------------------------

KFontOptions::KFontOptions( QWidget *parent, const char *name )
    : KConfigWidget( parent, name )
{
    QLabel *label;

    QGridLayout *lay = new QGridLayout(this,8,5,10,5);
    lay->addRowSpacing(0,10);
    lay->addRowSpacing(4,10);
    lay->addRowSpacing(0,10);
    lay->addRowSpacing(3,10);

    lay->setRowStretch(0,0);
    lay->setRowStretch(1,1);
    lay->setRowStretch(2,1);
    lay->setRowStretch(3,0);
    lay->setRowStretch(4,0);
    lay->setRowStretch(5,0);
    lay->setRowStretch(6,0);
    lay->setRowStretch(7,10);

    lay->setColStretch(0,0);
    lay->setColStretch(1,1);
    lay->setColStretch(2,2);
    lay->setColStretch(3,0);

    QButtonGroup *bg = new QButtonGroup( i18n("Font Size"), this );
    QGridLayout *bgLay = new QGridLayout(bg,2,3,10,5);
    bgLay->addRowSpacing(0,10);
    bgLay->setRowStretch(0,0);
    bgLay->setRowStretch(1,1);
    bg->setExclusive( TRUE );
  
    m_pSmall = new QRadioButton( i18n("Small"), bg );
    bgLay->addWidget(m_pSmall,1,0);

    m_pMedium = new QRadioButton( i18n("Medium"), bg );
    bgLay->addWidget(m_pMedium,1,1);

    m_pLarge = new QRadioButton( i18n("Large"), bg );
    bgLay->addWidget(m_pLarge,1,2);

    bgLay->activate();
    lay->addMultiCellWidget(bg,1,1,1,2);


    label = new QLabel( i18n("Standard Font"), this );    label->adjustSize();
    lay->addWidget(label,3,1);

    m_pStandard = new QComboBox( false, this );
    lay->addWidget(m_pStandard,3,2);

    getFontList( standardFonts, "-*-*-*-*-*-*-*-*-*-*-p-*-*-*" );
    m_pStandard->insertStrList( &standardFonts );
    connect( m_pStandard, SIGNAL( activated(const QString&) ),
             SLOT( slotStandardFont(const QString&) ) );
  
    label = new QLabel( i18n( "Fixed Font"), this );
    lay->addWidget(label,4,1);

    m_pFixed = new QComboBox( false, this );
    lay->addWidget(m_pFixed,4,2);
    m_pFixed->setGeometry( 120, 130, 180, 25 );
    getFontList( fixedFonts, "-*-*-*-*-*-*-*-*-*-*-m-*-*-*" );
    m_pFixed->insertStrList( &fixedFonts );
  
    connect( m_pFixed, SIGNAL( activated(const QString&) ),
             SLOT( slotFixedFont(const QString&) ) );
  
    // default charset Lars Knoll 17Nov98 (moved by David)
    label = new QLabel( i18n( "Default Charset"), this );
    lay->addWidget(label,5,1);
    lay->activate();

    m_pCharset = new QComboBox( false, this );
#warning FIXME, this seems to be broken in kcharsets (Simon)    
//    charsets = KGlobal::charsets()->availableCharsetNames();
    charsets.prepend(i18n("Use language charset"));
    m_pCharset->insertStringList( charsets );
    lay->addWidget(m_pCharset,5,2);
    
    connect( m_pCharset, SIGNAL( activated(const QString& ) ),
             SLOT( slotCharset(const QString&) ) );

    connect( bg, SIGNAL( clicked( int ) ), SLOT( slotFontSize( int ) ) );

    loadSettings();
}

void KFontOptions::getFontList( QStrList &list, const char *pattern )
{
    int num;
  
    char **xFonts = XListFonts( qt_xdisplay(), pattern, 2000, &num );
  
    for ( int i = 0; i < num; i++ )
    {
        addFont( list, xFonts[i] );
    }
  
    XFreeFontNames( xFonts );
}

void KFontOptions::addFont( QStrList &list, const char *xfont )
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

void KFontOptions::slotFontSize( int i )
{
    fSize = i+3;
}

void KFontOptions::slotStandardFont(const QString& n )
{
    stdName = n;
}

void KFontOptions::slotFixedFont(const QString& n )
{
    fixedName = n;
}

void KFontOptions::slotCharset(const QString& n)
{
    charsetName = n;
}

void KFontOptions::loadSettings()
{
    g_pConfig->setGroup( "KFM HTML Defaults" );		
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

    updateGUI();
}

void KFontOptions::defaultSettings()
{
    g_pConfig->setGroup( "KFM HTML Defaults" );			
    fSize=4;
    stdName = KGlobal::generalFont().family();
    fixedName = KGlobal::fixedFont().family();
    charsetName = "";

    updateGUI();
}

void KFontOptions::updateGUI()
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

void KFontOptions::saveSettings()
{
    g_pConfig->setGroup( "KFM HTML Defaults" );			
    g_pConfig->writeEntry( "BaseFontSize", fSize );
    g_pConfig->writeEntry( "StandardFont", stdName );
    g_pConfig->writeEntry( "FixedFont", fixedName );
    // If the user chose "Use language charset", write an empty string
    if (charsetName == i18n("Use language charset"))
        charsetName = "";
    g_pConfig->writeEntry( "DefaultCharset", charsetName );
    g_pConfig->sync();
}

void KFontOptions::applySettings()
{
    saveSettings();
}

//-----------------------------------------------------------------------------

KColorOptions::KColorOptions( QWidget *parent, const char *name )
    : KConfigWidget( parent, name )
{
    QLabel *label;

    QGridLayout *lay = new QGridLayout(this,12,5,10,5);
    lay->addRowSpacing(0,10);
    lay->addRowSpacing(1,30);
    lay->addRowSpacing(2, 5);
    lay->addRowSpacing(3,30);
    lay->addRowSpacing(4, 5);
    lay->addRowSpacing(5,30);
    lay->addRowSpacing(6, 5);
    lay->addRowSpacing(7,30);
    lay->addRowSpacing(11,10);
    lay->addColSpacing(0,10);
    lay->addColSpacing(2,20);
    lay->addColSpacing(3,80);
    lay->addColSpacing(4,10);

    lay->setRowStretch(0,0);
    lay->setRowStretch(1,0);
    lay->setRowStretch(2,1);
    lay->setRowStretch(3,0);
    lay->setRowStretch(4,1);
    lay->setRowStretch(5,0);
    lay->setRowStretch(6,1);
    lay->setRowStretch(7,0);
    lay->setRowStretch(8,1);
    lay->setRowStretch(9,1);
    lay->setRowStretch(10,1);
    lay->setRowStretch(11,0);

    lay->setColStretch(0,0);
    lay->setColStretch(1,0);
    lay->setColStretch(2,1);
    lay->setColStretch(3,0);
    lay->setColStretch(4,1);

    label = new QLabel( i18n("Background Color:"), this );
    lay->addWidget(label,1,1);

    m_pBg = new KColorButton( bgColor, this );
    lay->addWidget(m_pBg,1,3);
    connect( m_pBg, SIGNAL( changed( const QColor & ) ),
             SLOT( slotBgColorChanged( const QColor & ) ) );

    label = new QLabel( i18n("Normal Text Color:"), this );
    lay->addWidget(label,3,1);
  
    m_pText = new KColorButton( textColor, this );
    lay->addWidget(m_pText,3,3);
    connect( m_pText, SIGNAL( changed( const QColor & ) ),
             SLOT( slotTextColorChanged( const QColor & ) ) );

    label = new QLabel( i18n("URL Link Color:"), this );
    lay->addWidget(label,5,1);

    m_pLink = new KColorButton( linkColor, this );
    lay->addWidget(m_pLink,5,3);
    connect( m_pLink, SIGNAL( changed( const QColor & ) ),
             SLOT( slotLinkColorChanged( const QColor & ) ) );

    label = new QLabel( i18n("Followed Link Color:"), this );
    lay->addWidget(label,7,1);

    m_pVLink = new KColorButton( vLinkColor, this );
    lay->addWidget(m_pVLink,7,3);
    connect( m_pVLink, SIGNAL( changed( const QColor & ) ),
             SLOT( slotVLinkColorChanged( const QColor & ) ) );

    cursorbox = new QCheckBox(i18n("Change cursor over link."),
                              this);
    lay->addMultiCellWidget(cursorbox,8,8,1,3);

    underlinebox = new QCheckBox(i18n("Underline links"),
                                 this);
    lay->addMultiCellWidget(underlinebox,9,9,1,3);

    forceDefaultsbox = new QCheckBox(i18n("Always use my colors"),
                                 this);
    lay->addMultiCellWidget(forceDefaultsbox,10,10,1,3);

    loadSettings();
}

void KColorOptions::slotBgColorChanged( const QColor &col )
{
    if ( bgColor != col )
        bgColor = col;
}

void KColorOptions::slotTextColorChanged( const QColor &col )
{
    if ( textColor != col )
        textColor = col;
}

void KColorOptions::slotLinkColorChanged( const QColor &col )
{
    if ( linkColor != col )
        linkColor = col;
}

void KColorOptions::slotVLinkColorChanged( const QColor &col )
{
    if ( vLinkColor != col )
        vLinkColor = col;
}

void KColorOptions::loadSettings()
{
    g_pConfig->setGroup( "KFM HTML Defaults" );	
    bgColor = g_pConfig->readColorEntry( "BgColor", &HTML_DEFAULT_BG_COLOR );
    textColor = g_pConfig->readColorEntry( "TextColor", &HTML_DEFAULT_TXT_COLOR );
    linkColor = g_pConfig->readColorEntry( "LinkColor", &HTML_DEFAULT_LNK_COLOR );
    vLinkColor = g_pConfig->readColorEntry( "VLinkColor", &HTML_DEFAULT_VLNK_COLOR);
    bool changeCursor = g_pConfig->readBoolEntry("ChangeCursor", false);
    bool underlineLinks = g_pConfig->readBoolEntry("UnderlineLinks", true);
    bool forceDefaults = g_pConfig->readBoolEntry("ForceDefaultColors", false);

    m_pBg->setColor( bgColor );
    m_pText->setColor( textColor );
    m_pLink->setColor( linkColor );
    m_pVLink->setColor( vLinkColor );
    cursorbox->setChecked( changeCursor );
    underlinebox->setChecked( underlineLinks );
    forceDefaultsbox->setChecked( forceDefaults );
}

void KColorOptions::defaultSettings()
{
    bgColor = HTML_DEFAULT_BG_COLOR;
    textColor = HTML_DEFAULT_TXT_COLOR;
    linkColor = HTML_DEFAULT_LNK_COLOR;
    vLinkColor = HTML_DEFAULT_VLNK_COLOR;

    m_pBg->setColor( bgColor );
    m_pText->setColor( textColor );
    m_pLink->setColor( linkColor );
    m_pVLink->setColor( vLinkColor );
    cursorbox->setChecked( false );
    underlinebox->setChecked( false );
    forceDefaultsbox->setChecked( false );
}

void KColorOptions::saveSettings()
{
    g_pConfig->setGroup( "KFM HTML Defaults" );			
    g_pConfig->writeEntry( "BgColor", bgColor );
    g_pConfig->writeEntry( "TextColor", textColor );
    g_pConfig->writeEntry( "LinkColor", linkColor);
    g_pConfig->writeEntry( "VLinkColor", vLinkColor );
    g_pConfig->writeEntry( "ChangeCursor", cursorbox->isChecked() );
    g_pConfig->writeEntry( "UnderlineLinks", underlinebox->isChecked() );
    g_pConfig->writeEntry("ForceDefaultColors", forceDefaultsbox->isChecked() );
    g_pConfig->sync();
}

void KColorOptions::applySettings()
{
    saveSettings();
}

#include "htmlopts.moc"
