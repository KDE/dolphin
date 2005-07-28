////
// "Misc Options" Tab for KFM configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998
// (c) 2001 Waldo Bastian <bastian@kde.org>

#include <qlayout.h>//CT - 12Nov1998
#include <qwhatsthis.h>
#include <q3groupbox.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include "htmlopts.h"
#include "advancedTabDialog.h"

#include <konq_defaults.h> // include default values directly from konqueror
#include <kglobalsettings.h> // get default for DEFAULT_CHANGECURSOR
#include <klocale.h>
#include <kdialog.h>
#include <knuminput.h>
#include <kseparator.h>

#include <kapplication.h>
#include <dcopclient.h>


#include "htmlopts.moc"

enum UnderlineLinkType { UnderlineAlways=0, UnderlineNever=1, UnderlineHover=2 };
enum AnimationsType { AnimationsAlways=0, AnimationsNever=1, AnimationsLoopOnce=2 };
//-----------------------------------------------------------------------------

KMiscHTMLOptions::KMiscHTMLOptions(KConfig *config, QString group, QWidget *parent, const char *)
    : KCModule( parent, "kcmkonqhtml" ), m_pConfig(config), m_groupname(group)
{
    int row = 0;
    QGridLayout *lay = new QGridLayout(this, 10, 2, 0, KDialog::spacingHint());

    // Bookmarks
    setQuickHelp( i18n("<h1>Konqueror Browser</h1> Here you can configure Konqueror's browser "
              "functionality. Please note that the file manager "
              "functionality has to be configured using the \"File Manager\" "
              "configuration module. You can make some "
              "settings how Konqueror should handle the HTML code in "
              "the web pages it loads. It is usually not necessary to "
              "change anything here."));

    Q3GroupBox *bgBookmarks = new Q3GroupBox( i18n("Boo&kmarks"), this );
	bgBookmarks->setOrientation( Qt::Vertical );
	QVBoxLayout *laygroup1 = new QVBoxLayout(bgBookmarks->layout(), KDialog::spacingHint() );
    m_pAdvancedAddBookmarkCheckBox = new QCheckBox(i18n( "Ask for name and folder when adding bookmarks" ), bgBookmarks);
	laygroup1->addWidget(m_pAdvancedAddBookmarkCheckBox);
    QWhatsThis::add( m_pAdvancedAddBookmarkCheckBox, i18n( "If this box is checked, Konqueror will allow you to"
                                                        " change the title of the bookmark and choose a folder in which to store it when you add a new bookmark." ) );
    connect(m_pAdvancedAddBookmarkCheckBox, SIGNAL(clicked()), SLOT(slotChanged()));

    m_pOnlyMarkedBookmarksCheckBox = new QCheckBox(i18n( "Show only marked bookmarks in bookmark toolbar" ), bgBookmarks);
	laygroup1->addWidget(m_pOnlyMarkedBookmarksCheckBox);
    QWhatsThis::add( m_pOnlyMarkedBookmarksCheckBox, i18n( "If this box is checked, Konqueror will show only those"
                                                         " bookmarks in the bookmark toolbar which you have marked to do so in the bookmark editor." ) );
    connect(m_pOnlyMarkedBookmarksCheckBox, SIGNAL(clicked()), SLOT(slotChanged()));

    lay->addMultiCellWidget( bgBookmarks, row, row, 0, 1 );
    row++;

     // Form completion

    Q3GroupBox *bgForm = new Q3GroupBox( i18n("Form Com&pletion"), this );
    bgForm->setOrientation( Qt::Vertical );
    QVBoxLayout *laygroup2 = new QVBoxLayout(bgForm->layout(), KDialog::spacingHint() );
    m_pFormCompletionCheckBox = new QCheckBox(i18n( "Enable completion of &forms" ), bgForm);
    laygroup2->addWidget( m_pFormCompletionCheckBox );
    QWhatsThis::add( m_pFormCompletionCheckBox, i18n( "If this box is checked, Konqueror will remember"
                                                        " the data you enter in web forms and suggest it in similar fields for all forms." ) );
    connect(m_pFormCompletionCheckBox, SIGNAL(clicked()), SLOT(slotChanged()));

    m_pMaxFormCompletionItems = new KIntNumInput( bgForm );
    m_pMaxFormCompletionItems->setLabel( i18n( "&Maximum completions:" ) );
    m_pMaxFormCompletionItems->setRange( 0, 100 );
    laygroup2->addWidget( m_pMaxFormCompletionItems );
    QWhatsThis::add( m_pMaxFormCompletionItems,
        i18n( "Here you can select how many values Konqueror will remember for a form field." ) );
    connect(m_pMaxFormCompletionItems, SIGNAL(valueChanged(int)), SLOT(slotChanged()));

    lay->addMultiCellWidget( bgForm, row, row, 0, 1 );
    row++;

    // Tabbed Browsing

    Q3GroupBox *bgTabbedBrowsing = new Q3GroupBox( 0, Qt::Vertical, i18n("Tabbed Browsing"), this );
    QVBoxLayout *laygroup = new QVBoxLayout(bgTabbedBrowsing->layout(), KDialog::spacingHint() );

    m_pShowMMBInTabs = new QCheckBox( i18n( "Open &links in new tab instead of in new window" ), bgTabbedBrowsing );
    QWhatsThis::add( m_pShowMMBInTabs, i18n("This will open a new tab instead of a new window in various situations, "
                          "such as choosing a link or a folder with the middle mouse button.") );
    connect(m_pShowMMBInTabs, SIGNAL(clicked()), SLOT(slotChanged()));
    laygroup->addWidget(m_pShowMMBInTabs);

    m_pDynamicTabbarHide = new QCheckBox( i18n( "Hide the tab bar when only one tab is open" ), bgTabbedBrowsing );
    QWhatsThis::add( m_pDynamicTabbarHide, i18n("This will display the tab bar only if there are two or more tabs. Otherwise it will always be displayed.") );
    connect(m_pDynamicTabbarHide, SIGNAL(clicked()), SLOT(slotChanged()));
    laygroup->addWidget(m_pDynamicTabbarHide);

    QHBoxLayout *laytab = new QHBoxLayout(laygroup, KDialog::spacingHint());
    QPushButton *advancedTabButton = new QPushButton( i18n( "Advanced Options"), bgTabbedBrowsing );
    laytab->addWidget(advancedTabButton);
    laytab->addStretch();
    connect(advancedTabButton, SIGNAL(clicked()), this, SLOT(launchAdvancedTabDialog()));

    lay->addMultiCellWidget( bgTabbedBrowsing, row, row, 0, 1 );
    row++;

    // Mouse behavior

    Q3GroupBox *bgMouse = new Q3GroupBox( i18n("Mouse Beha&vior"), this );
    bgMouse->setOrientation( Qt::Vertical );
    QVBoxLayout *laygroup3 = new QVBoxLayout(bgMouse->layout(), KDialog::spacingHint() );

    m_cbCursor = new QCheckBox(i18n("Chan&ge cursor over links"), bgMouse );
    laygroup3->addWidget( m_cbCursor );
    QWhatsThis::add( m_cbCursor, i18n("If this option is set, the shape of the cursor will change "
       "(usually to a hand) if it is moved over a hyperlink.") );
    connect(m_cbCursor, SIGNAL(clicked()), SLOT(slotChanged()));

    m_pOpenMiddleClick = new QCheckBox( i18n ("M&iddle click opens URL in selection" ), bgMouse );
    laygroup3->addWidget( m_pOpenMiddleClick );
    QWhatsThis::add( m_pOpenMiddleClick, i18n (
      "If this box is checked, you can open the URL in the selection by middle clicking on a "
      "Konqueror view." ) );
    connect(m_pOpenMiddleClick, SIGNAL(clicked()), SLOT(slotChanged()));

    m_pBackRightClick = new QCheckBox( i18n( "Right click goes &back in history" ), bgMouse );
    laygroup3->addWidget( m_pBackRightClick );
    QWhatsThis::add( m_pBackRightClick, i18n(
      "If this box is checked, you can go back in history by right clicking on a Konqueror view. "
      "To access the context menu, press the right mouse button and move." ) );
    connect(m_pBackRightClick, SIGNAL(clicked()), SLOT(slotChanged()));

    lay->addMultiCellWidget( bgMouse, row, row, 0, 1 );
    row++;

    // Misc

    m_pAutoLoadImagesCheckBox = new QCheckBox( i18n( "A&utomatically load images"), this );
    QWhatsThis::add( m_pAutoLoadImagesCheckBox, i18n( "If this box is checked, Konqueror will automatically load any images that are embedded in a web page. Otherwise, it will display placeholders for the images, and you can then manually load the images by clicking on the image button.<br>Unless you have a very slow network connection, you will probably want to check this box to enhance your browsing experience." ) );
    connect(m_pAutoLoadImagesCheckBox, SIGNAL(clicked()), SLOT(slotChanged()));
    lay->addMultiCellWidget( m_pAutoLoadImagesCheckBox, row, row, 0, 1 );
    row++;

    m_pUnfinishedImageFrameCheckBox = new QCheckBox( i18n( "Dra&w frame around not completely loaded images"), this );
    QWhatsThis::add( m_pUnfinishedImageFrameCheckBox, i18n( "If this box is checked, Konqueror will draw a frame as placeholder around not yet fully loaded images that are embedded in a web page.<br>Especially if you have a slow network connection, you will probably want to check this box to enhance your browsing experience." ) );
    connect(m_pUnfinishedImageFrameCheckBox, SIGNAL(clicked()), SLOT(slotChanged()));
    lay->addMultiCellWidget( m_pUnfinishedImageFrameCheckBox, row, row, 0, 1 );
    row++;

    m_pAutoRedirectCheckBox = new QCheckBox( i18n( "Allow automatic delayed &reloading/redirecting"), this );
    QWhatsThis::add( m_pAutoRedirectCheckBox,
    i18n( "Some web pages request an automatic reload or redirection after a certain period of time. By unchecking this box Konqueror will ignore these requests." ) );
    connect(m_pAutoRedirectCheckBox, SIGNAL(clicked()), SLOT(slotChanged()));
    lay->addMultiCellWidget( m_pAutoRedirectCheckBox, row, row, 0, 1 );
    row++;


    // More misc

    KSeparator *sep = new KSeparator(this);
    lay->addMultiCellWidget(sep, row, row, 0, 1);
    row++;

    QLabel *label = new QLabel( i18n("Und&erline links:"), this);
    m_pUnderlineCombo = new QComboBox( false, this );
    label->setBuddy(m_pUnderlineCombo);
    m_pUnderlineCombo->insertItem(i18n("underline","Enabled"), UnderlineAlways);
    m_pUnderlineCombo->insertItem(i18n("underline","Disabled"), UnderlineNever);
    m_pUnderlineCombo->insertItem(i18n("Only on Hover"), UnderlineHover);
    lay->addWidget(label, row, 0);
    lay->addWidget(m_pUnderlineCombo, row, 1);
    row++;
    QString whatsThis = i18n("Controls how Konqueror handles underlining hyperlinks:<br>"
	    "<ul><li><b>Enabled</b>: Always underline links</li>"
	    "<li><b>Disabled</b>: Never underline links</li>"
	    "<li><b>Only on Hover</b>: Underline when the mouse is moved over the link</li>"
	    "</ul><br><i>Note: The site's CSS definitions can override this value</i>");
    QWhatsThis::add( label, whatsThis);
    QWhatsThis::add( m_pUnderlineCombo, whatsThis);
    connect(m_pUnderlineCombo, SIGNAL(activated(int)), SLOT(slotChanged()));



    label = new QLabel( i18n("A&nimations:"), this);
    m_pAnimationsCombo = new QComboBox( false, this );
    label->setBuddy(m_pAnimationsCombo);
    m_pAnimationsCombo->insertItem(i18n("animations","Enabled"), AnimationsAlways);
    m_pAnimationsCombo->insertItem(i18n("animations","Disabled"), AnimationsNever);
    m_pAnimationsCombo->insertItem(i18n("Show Only Once"), AnimationsLoopOnce);
    lay->addWidget(label, row, 0);
    lay->addWidget(m_pAnimationsCombo, row, 1);
    row++;
    whatsThis = i18n("Controls how Konqueror shows animated images:<br>"
	    "<ul><li><b>Enabled</b>: Show all animations completely.</li>"
	    "<li><b>Disabled</b>: Never show animations, show the start image only.</li>"
	    "<li><b>Show only once</b>: Show all animations completely but do not repeat them.</li>");
    QWhatsThis::add( label, whatsThis);
    QWhatsThis::add( m_pAnimationsCombo, whatsThis);
    connect(m_pAnimationsCombo, SIGNAL(activated(int)), SLOT(slotChanged()));

    lay->setRowStretch(row, 1);

    load();
    emit changed(false);
}

KMiscHTMLOptions::~KMiscHTMLOptions()
{
    delete m_pConfig;
}

void KMiscHTMLOptions::load()
{
    KConfig khtmlrc("khtmlrc", true, false);
#define SET_GROUP(x) m_pConfig->setGroup(x); khtmlrc.setGroup(x)
#define READ_BOOL(x,y) m_pConfig->readBoolEntry(x, khtmlrc.readBoolEntry(x, y))
#define READ_ENTRY(x) m_pConfig->readEntry(x, khtmlrc.readEntry(x))


    // *** load ***
    SET_GROUP( "MainView Settings" );
    bool bOpenMiddleClick = READ_BOOL( "OpenMiddleClick", true );
    bool bBackRightClick = READ_BOOL( "BackRightClick", false );
    SET_GROUP( "HTML Settings" );
    bool changeCursor = READ_BOOL("ChangeCursor", KDE_DEFAULT_CHANGECURSOR);
    bool underlineLinks = READ_BOOL("UnderlineLinks", DEFAULT_UNDERLINELINKS);
    bool hoverLinks = READ_BOOL("HoverLinks", true);
    bool bAutoLoadImages = READ_BOOL( "AutoLoadImages", true );
    bool bUnfinishedImageFrame = READ_BOOL( "UnfinishedImageFrame", true );
    QString strAnimations = READ_ENTRY( "ShowAnimations" ).lower();

    bool bAutoRedirect = m_pConfig->readBoolEntry( "AutoDelayedActions", true );

    // *** apply to GUI ***
    m_cbCursor->setChecked( changeCursor );
    m_pAutoLoadImagesCheckBox->setChecked( bAutoLoadImages );
    m_pUnfinishedImageFrameCheckBox->setChecked( bUnfinishedImageFrame );
    m_pAutoRedirectCheckBox->setChecked( bAutoRedirect );
    m_pOpenMiddleClick->setChecked( bOpenMiddleClick );
    m_pBackRightClick->setChecked( bBackRightClick );

    // we use two keys for link underlining so that this config file
    // is backwards compatible with KDE 2.0.  the HoverLink setting
    // has precedence over the UnderlineLinks setting
    if (hoverLinks)
    {
        m_pUnderlineCombo->setCurrentItem( UnderlineHover );
    }
    else
    {
        if (underlineLinks)
            m_pUnderlineCombo->setCurrentItem( UnderlineAlways );
        else
            m_pUnderlineCombo->setCurrentItem( UnderlineNever );
    }
    if (strAnimations == "disabled")
       m_pAnimationsCombo->setCurrentItem( AnimationsNever );
    else if (strAnimations == "looponce")
       m_pAnimationsCombo->setCurrentItem( AnimationsLoopOnce );
    else
       m_pAnimationsCombo->setCurrentItem( AnimationsAlways );

    m_pFormCompletionCheckBox->setChecked( m_pConfig->readBoolEntry( "FormCompletion", true ) );
    m_pMaxFormCompletionItems->setValue( m_pConfig->readNumEntry( "MaxFormCompletionItems", 10 ) );
    m_pMaxFormCompletionItems->setEnabled( m_pFormCompletionCheckBox->isChecked() );

    m_pConfig->setGroup("FMSettings");
    m_pShowMMBInTabs->setChecked( m_pConfig->readBoolEntry( "MMBOpensTab", false ) );
    m_pDynamicTabbarHide->setChecked( ! (m_pConfig->readBoolEntry( "AlwaysTabbedMode", false )) );

    KConfig config("kbookmarkrc", true, false);
    config.setGroup("Bookmarks");
    m_pAdvancedAddBookmarkCheckBox->setChecked( config.readBoolEntry("AdvancedAddBookmarkDialog", false) );
    m_pOnlyMarkedBookmarksCheckBox->setChecked( config.readBoolEntry("FilteredToolbar", false) );
}

void KMiscHTMLOptions::defaults()
{
    bool old = m_pConfig->readDefaults();
    m_pConfig->setReadDefaults(true);
    load();
    m_pConfig->setReadDefaults(old);
    m_pAdvancedAddBookmarkCheckBox->setChecked(false);
    m_pOnlyMarkedBookmarksCheckBox->setChecked(false);
}

void KMiscHTMLOptions::save()
{
    m_pConfig->setGroup( "MainView Settings" );
    m_pConfig->writeEntry( "OpenMiddleClick", m_pOpenMiddleClick->isChecked() );
    m_pConfig->writeEntry( "BackRightClick", m_pBackRightClick->isChecked() );
    m_pConfig->setGroup( "HTML Settings" );
    m_pConfig->writeEntry( "ChangeCursor", m_cbCursor->isChecked() );
    m_pConfig->writeEntry( "AutoLoadImages", m_pAutoLoadImagesCheckBox->isChecked() );
    m_pConfig->writeEntry( "UnfinishedImageFrame", m_pUnfinishedImageFrameCheckBox->isChecked() );
    m_pConfig->writeEntry( "AutoDelayedActions", m_pAutoRedirectCheckBox->isChecked() );
    switch(m_pUnderlineCombo->currentItem())
    {
      case UnderlineAlways:
        m_pConfig->writeEntry( "UnderlineLinks", true );
        m_pConfig->writeEntry( "HoverLinks", false );
        break;
      case UnderlineNever:
        m_pConfig->writeEntry( "UnderlineLinks", false );
        m_pConfig->writeEntry( "HoverLinks", false );
        break;
      case UnderlineHover:
        m_pConfig->writeEntry( "UnderlineLinks", false );
        m_pConfig->writeEntry( "HoverLinks", true );
        break;
    }
    switch(m_pAnimationsCombo->currentItem())
    {
      case AnimationsAlways:
        m_pConfig->writeEntry( "ShowAnimations", "Enabled" );
        break;
      case AnimationsNever:
        m_pConfig->writeEntry( "ShowAnimations", "Disabled" );
        break;
      case AnimationsLoopOnce:
        m_pConfig->writeEntry( "ShowAnimations", "LoopOnce" );
        break;
    }

    m_pConfig->writeEntry( "FormCompletion", m_pFormCompletionCheckBox->isChecked() );
    m_pConfig->writeEntry( "MaxFormCompletionItems", m_pMaxFormCompletionItems->value() );

    m_pConfig->setGroup("FMSettings");
    m_pConfig->writeEntry( "MMBOpensTab", m_pShowMMBInTabs->isChecked() );
    m_pConfig->writeEntry( "AlwaysTabbedMode", !(m_pDynamicTabbarHide->isChecked()) );
    m_pConfig->sync();

    KConfig config("kbookmarkrc", false, false);
    config.setGroup("Bookmarks");
    config.writeEntry("AdvancedAddBookmarkDialog", m_pAdvancedAddBookmarkCheckBox->isChecked());
    config.writeEntry("FilteredToolbar", m_pOnlyMarkedBookmarksCheckBox->isChecked());
    config.sync();

  QByteArray data;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );

    emit changed(false);
}


void KMiscHTMLOptions::slotChanged()
{
    m_pMaxFormCompletionItems->setEnabled( m_pFormCompletionCheckBox->isChecked() );
    emit changed(true);
}


void KMiscHTMLOptions::launchAdvancedTabDialog()
{
    advancedTabDialog* dialog = new advancedTabDialog(this, m_pConfig, "advancedTabDialog");
    dialog->exec();
}


