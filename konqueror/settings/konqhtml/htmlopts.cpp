////
// "Misc Options" Tab for KFM configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998

#include <qlabel.h>
#include <qgroupbox.h>
#include <qlayout.h>//CT - 12Nov1998
#include <qwhatsthis.h>
#include <qvbuttongroup.h>
#include <qradiobutton.h>
#include <kapp.h>

#include "htmlopts.h"

#include <konq_defaults.h> // include default values directly from konqueror
#include <kglobalsettings.h> // get default for DEFAULT_CHANGECURSOR
#include <klocale.h>
#include <kconfig.h>
#include <kurlrequester.h>
#include <klineedit.h>
#include <kfiledialog.h>

#include "htmlopts.moc"

enum UnderlineLinkType { Always=0, Never=1, Hover=2 };
//-----------------------------------------------------------------------------

KMiscHTMLOptions::KMiscHTMLOptions(KConfig *config, QString group, QWidget *parent, const char *name )
    : KCModule( parent, name ), m_pConfig(config), m_groupname(group)
{
    QVBoxLayout *lay = new QVBoxLayout(this, 10, 5);

    cbCursor = new QCheckBox(i18n("&Change cursor over links"), this);
    lay->addWidget(cbCursor);

    QWhatsThis::add( cbCursor, i18n("If this option is set, the shape of the cursor will change "
       "(usually to a hand) if it is moved over a hyperlink.") );

    connect(cbCursor, SIGNAL(clicked()), this, SLOT(changed()));

    QVButtonGroup *bg = new QVButtonGroup( i18n("Un&derline Links"), this );
    bg->setExclusive( TRUE );
    connect(bg, SIGNAL(clicked(int)), this, SLOT(changed()));
    lay->addWidget(bg);
    QWhatsThis::add( bg, i18n("Controls how Konqueror handles underlining hyperlinks.") );

    m_pUnderlineRadio[Always] = new QRadioButton( i18n("&Always"), bg );
    m_pUnderlineRadio[Never] = new QRadioButton( i18n("&Never"), bg );
    m_pUnderlineRadio[Hover] = new QRadioButton( i18n("&Hover"), bg );

    m_pAutoLoadImagesCheckBox = new QCheckBox( i18n( ""
     "A&utomatically load images\n"
     "(Otherwise, click the Images button to load when needed)" ), this );
    QWhatsThis::add( m_pAutoLoadImagesCheckBox, i18n( "If this box is checked, Konqueror will automatically load any images that are embedded in a web page. Otherwise, it will display placeholders for the images, and you can then manually load the images by clicking on the image button.<br>Unless you have a very slow network connection, you will probably want to check this box to enhance your browsing experience." ) );
    connect(m_pAutoLoadImagesCheckBox, SIGNAL(clicked()), this, SLOT(changed()));
    lay->addWidget( m_pAutoLoadImagesCheckBox, 1 );

/*  Tackat doesn't want too much options :)

    m_pEnableFaviconCheckBox = new QCheckBox( i18n( "Enable \"&favorite icon\" support" ), this );
    QWhatsThis::add( m_pEnableFaviconCheckBox, i18n( "This will cause konqueror to look for <b>\"/favicon.ico\"</b> on the server "
    "to display this icon in the locationbar and the bookmarks.") );
    lay->addWidget( m_pEnableFaviconCheckBox, 1 );
    connect(m_pEnableFaviconCheckBox, SIGNAL(clicked()), this, SLOT(changed()));
*/

    userSheet = new QCheckBox(i18n("Enable user defined &style sheet"), this);
    lay->addWidget(userSheet);
    connect( userSheet, SIGNAL( clicked() ), this, SLOT( changed() ));

    QWhatsThis::add( userSheet, i18n("If this box is checked, konqueror will try to load a user defined style sheet as specified in the location below. The style sheet allows you to completely override the way web pages are rendered in your browser. The file specified should contain a valid style sheet (see http://www.w3.org/Style/CSS for further information on cascading style sheets).") );

    userSheetLocation = new KURLRequester( this, "sheet");
    connect( userSheetLocation, SIGNAL( openFileDialog( KURLRequester * )),
	     SLOT( slotOpenFileDialog( KURLRequester * )));
    lay->addWidget(userSheetLocation);
    connect( userSheetLocation->lineEdit(), SIGNAL( textChanged( const QString & ) ), this, SLOT( changed() ) );
    connect( userSheet, SIGNAL( toggled( bool )), userSheetLocation, SLOT( setEnabled( bool ) ) );

    lay->addStretch(10);
    lay->activate();

    load();
}

void KMiscHTMLOptions::load()
{
    // *** load ***
    m_pConfig->setGroup( "HTML Settings" );
    bool changeCursor = m_pConfig->readBoolEntry("ChangeCursor", KDE_DEFAULT_CHANGECURSOR);
    bool underlineLinks = m_pConfig->readBoolEntry("UnderlineLinks", DEFAULT_UNDERLINELINKS);
    bool hoverLinks = m_pConfig->readBoolEntry("HoverLinks", true);
    bool bAutoLoadImages = m_pConfig->readBoolEntry( "AutoLoadImages", true );

    bool sheetEnabled = m_pConfig->readBoolEntry("UserStyleSheetEnabled", false);
    QString sheet = m_pConfig->readEntry("UserStyleSheet", "");

    // *** apply to GUI ***
    userSheet->setChecked( sheetEnabled );
    userSheetLocation->lineEdit()->setText(sheet);
    userSheetLocation->setEnabled( sheetEnabled );


    cbCursor->setChecked( changeCursor );
    m_pAutoLoadImagesCheckBox->setChecked( bAutoLoadImages );

    // we use two keys for link underlining so that this config file
    // is backwards compatible with KDE 2.0.  the HoverLink setting
    // has precedence over the UnderlineLinks setting
    if (hoverLinks)
    {
        m_pUnderlineRadio[Hover]->setChecked( true );
    }
    else
    {
        if (underlineLinks)
            m_pUnderlineRadio[Always]->setChecked( true );
        else
            m_pUnderlineRadio[Never]->setChecked( true );
    }
}

void KMiscHTMLOptions::defaults()
{
    cbCursor->setChecked( false );
    m_pAutoLoadImagesCheckBox->setChecked( true );
    m_pUnderlineRadio[Hover]->setChecked( true );
}

void KMiscHTMLOptions::save()
{
    m_pConfig->setGroup( "HTML Settings" );
    m_pConfig->writeEntry( "ChangeCursor", cbCursor->isChecked() );
    m_pConfig->writeEntry( "AutoLoadImages", m_pAutoLoadImagesCheckBox->isChecked() );
//    m_pConfig->writeEntry( "EnableFavicon", m_pEnableFaviconCheckBox->isChecked() );
    m_pConfig->writeEntry( "UserStyleSheetEnabled", userSheet->isChecked() );
    m_pConfig->writeEntry( "UserStyleSheet", userSheetLocation->lineEdit()->text() );
    if (m_pUnderlineRadio[Hover]->isChecked())
    {
        m_pConfig->writeEntry( "UnderlineLinks", false );
        m_pConfig->writeEntry( "HoverLinks", true );
    }
    else
    {
        m_pConfig->writeEntry( "HoverLinks", false );
        m_pConfig->writeEntry( "UnderlineLinks", m_pUnderlineRadio[Always]->isChecked() );
    }

    m_pConfig->sync();
}

void KMiscHTMLOptions::slotOpenFileDialog( KURLRequester *req )
{
    if ( req == userSheetLocation )
	userSheetLocation->fileDialog()->setFilter("*.css");
}

void KMiscHTMLOptions::changed()
{
    emit KCModule::changed(true);
}
