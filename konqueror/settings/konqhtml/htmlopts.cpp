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
#include <qvgroupbox.h>
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
#include <knuminput.h>

#include "htmlopts.moc"

enum UnderlineLinkType { Always=0, Never=1, Hover=2 };
//-----------------------------------------------------------------------------

KMiscHTMLOptions::KMiscHTMLOptions(KConfig *config, QString group, QWidget *parent, const char *name )
    : KCModule( parent, name ), m_pConfig(config), m_groupname(group)
{
    QVBoxLayout *lay = new QVBoxLayout(this, 10, 5);

     // Form completion

    QVGroupBox *bgForm = new QVGroupBox( i18n("Form C&ompletion"), this );
    m_pFormCompletionCheckBox = new QCheckBox(i18n( "Enable completion of &forms" ), bgForm);
    QWhatsThis::add( m_pFormCompletionCheckBox, i18n( "If this box is checked, Konqueror will remember"
                                                        " the data you enter in web forms and suggest it in similar fields for all forms." ) );
    connect(m_pFormCompletionCheckBox, SIGNAL(clicked()), this, SLOT(changed()));

    m_pMaxFormCompletionItems = new KIntNumInput( bgForm );
    m_pMaxFormCompletionItems->setLabel( i18n( "&Maximum completions:" ) );
    m_pMaxFormCompletionItems->setRange( 1, 100 );
    QWhatsThis::add( m_pMaxFormCompletionItems,
        i18n( "Here you can select how many values Konqueror will remember for a form field." ) );
    connect(m_pMaxFormCompletionItems, SIGNAL(valueChanged(int)), SLOT(changed()));

    lay->addWidget( bgForm );

    // Underline Link Settings

    QVButtonGroup *bgLinks = new QVButtonGroup( i18n("Un&derline Links"), this );
    bgLinks->setExclusive( TRUE );
    connect(bgLinks, SIGNAL(clicked(int)), this, SLOT(changed()));
    QWhatsThis::add( bgLinks, i18n("Controls how Konqueror handles underlining hyperlinks:<br>"
				    "<ul><li><b>Always</b>: Always underline links</li>"
				    "<li><b>Never</b>: Never underline links</li>"
				    "<li><b>Hover</b>: Underline when the mouse is moved over the link</li>"
				    "</ul><br><i>Note: The site's CSS definitions can override this value</i>") );

    m_pUnderlineRadio[Always] = new QRadioButton( i18n("&Always"), bgLinks );
    m_pUnderlineRadio[Never] = new QRadioButton( i18n("&Never"), bgLinks );
    m_pUnderlineRadio[Hover] = new QRadioButton( i18n("&Hover"), bgLinks );

    lay->addWidget(bgLinks);


    // Misc

    cbCursor = new QCheckBox(i18n("&Change cursor over links"), this);
    lay->addWidget(cbCursor);

    QWhatsThis::add( cbCursor, i18n("If this option is set, the shape of the cursor will change "
       "(usually to a hand) if it is moved over a hyperlink.") );

    connect(cbCursor, SIGNAL(clicked()), this, SLOT(changed()));

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

    // *** apply to GUI ***
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

    m_pFormCompletionCheckBox->setChecked( m_pConfig->readBoolEntry( "FormCompletion", true ) );
    m_pMaxFormCompletionItems->setValue( m_pConfig->readNumEntry( "MaxFormCompletionItems", 10 ) );
    m_pMaxFormCompletionItems->setEnabled( m_pFormCompletionCheckBox->isChecked() );
}

void KMiscHTMLOptions::defaults()
{
    cbCursor->setChecked( false );
    m_pAutoLoadImagesCheckBox->setChecked( true );
    m_pUnderlineRadio[Always]->setChecked( true );
    m_pFormCompletionCheckBox->setChecked(true);
    m_pMaxFormCompletionItems->setEnabled( true );
}

void KMiscHTMLOptions::save()
{
    m_pConfig->setGroup( "HTML Settings" );
    m_pConfig->writeEntry( "ChangeCursor", cbCursor->isChecked() );
    m_pConfig->writeEntry( "AutoLoadImages", m_pAutoLoadImagesCheckBox->isChecked() );
//    m_pConfig->writeEntry( "EnableFavicon", m_pEnableFaviconCheckBox->isChecked() );
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

    m_pConfig->writeEntry( "FormCompletion", m_pFormCompletionCheckBox->isChecked() );
    m_pConfig->writeEntry( "MaxFormCompletionItems", m_pMaxFormCompletionItems->value() );

    m_pConfig->sync();
}


void KMiscHTMLOptions::changed()
{
    m_pMaxFormCompletionItems->setEnabled( m_pFormCompletionCheckBox->isChecked() );
    emit KCModule::changed(true);
}
