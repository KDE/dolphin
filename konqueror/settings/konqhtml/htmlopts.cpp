////
// "Misc Options" Tab for KFM configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998
// (c) 2001 Waldo Bastian <bastian@kde.org>

#include <qlayout.h>//CT - 12Nov1998
#include <qwhatsthis.h>
#include <qvgroupbox.h>
#include <qlabel.h>

#include "htmlopts.h"

#include <konq_defaults.h> // include default values directly from konqueror
#include <kglobalsettings.h> // get default for DEFAULT_CHANGECURSOR
#include <klocale.h>
#include <kdialog.h>
#include <knuminput.h>
#include <kseparator.h>

#include "htmlopts.moc"

enum UnderlineLinkType { UnderlineAlways=0, UnderlineNever=1, UnderlineHover=2 };
enum AnimationsType { AnimationsAlways=0, AnimationsNever=1, AnimationsLoopOnce=2 };
//-----------------------------------------------------------------------------

KMiscHTMLOptions::KMiscHTMLOptions(KConfig *config, QString group, QWidget *parent, const char *name )
    : QWidget( parent, name ), m_pConfig(config), m_groupname(group)
{
    int row = 0;
    QGridLayout *lay = new QGridLayout(this, 10, 2, KDialog::marginHint(), KDialog::spacingHint());

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

    lay->addMultiCellWidget( bgForm, row, row, 0, 1 );
    row++;


    // Misc

    cbCursor = new QCheckBox(i18n("Change cursor over &links"), this);
    lay->addMultiCellWidget(cbCursor, row, row, 0, 1);
    row++;

    QWhatsThis::add( cbCursor, i18n("If this option is set, the shape of the cursor will change "
       "(usually to a hand) if it is moved over a hyperlink.") );

    connect(cbCursor, SIGNAL(clicked()), this, SLOT(changed()));

    m_pBackRightClick = new QCheckBox( i18n( "Right click goes &back in history" ), this );
    QWhatsThis::add( m_pBackRightClick, i18n(
      "If this box is checked, you can go back in history by right clicking on a Konqueror view. "
      "To access the context menu, press the right mouse button and move." ) );
    lay->addMultiCellWidget( m_pBackRightClick, row, row, 0, 1);
    row++;
    connect(m_pBackRightClick, SIGNAL(clicked()), this, SLOT(changed()));

    m_pAutoLoadImagesCheckBox = new QCheckBox( i18n( "A&utomatically load images"), this );
    QWhatsThis::add( m_pAutoLoadImagesCheckBox, i18n( "If this box is checked, Konqueror will automatically load any images that are embedded in a web page. Otherwise, it will display placeholders for the images, and you can then manually load the images by clicking on the image button.<br>Unless you have a very slow network connection, you will probably want to check this box to enhance your browsing experience." ) );
    connect(m_pAutoLoadImagesCheckBox, SIGNAL(clicked()), this, SLOT(changed()));
    lay->addMultiCellWidget( m_pAutoLoadImagesCheckBox, row, row, 0, 1 );
    row++;

    m_pAutoRedirectCheckBox = new QCheckBox( i18n( "Allow automatic delayed &reloading / redirecting"), this );
    QWhatsThis::add( m_pAutoRedirectCheckBox,
    i18n( "Some web pages request an automatic reload or redirection after a certain period of time. By unchecking this box Konqueror will ignore these requests." ) );
    connect(m_pAutoRedirectCheckBox, SIGNAL(clicked()), this, SLOT(changed()));
    lay->addMultiCellWidget( m_pAutoRedirectCheckBox, row, row, 0, 1 );
    row++;


    // More misc

    KSeparator *sep = new KSeparator(this);
    lay->addMultiCellWidget(sep, row, row, 0, 1);
    row++;

    QLabel *label = new QLabel( i18n("Un&derline links:"), this);
    m_pUnderlineCombo = new QComboBox( false, this );
    label->setBuddy(m_pUnderlineCombo);
    m_pUnderlineCombo->insertItem(i18n("Enabled"), UnderlineAlways);
    m_pUnderlineCombo->insertItem(i18n("Disabled"), UnderlineNever);
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
    connect(m_pUnderlineCombo, SIGNAL(activated(int)), this, SLOT(changed()));



    label = new QLabel( i18n("A&nimations:"), this);
    m_pAnimationsCombo = new QComboBox( false, this );
    label->setBuddy(m_pAnimationsCombo);
    m_pAnimationsCombo->insertItem(i18n("Enabled"), AnimationsAlways);
    m_pAnimationsCombo->insertItem(i18n("Disabled"), AnimationsNever);
    m_pAnimationsCombo->insertItem(i18n("Show only once"), AnimationsLoopOnce);
    lay->addWidget(label, row, 0);
    lay->addWidget(m_pAnimationsCombo, row, 1);
    row++;
    whatsThis = i18n("Controls how Konqueror shows animated images:<br>"
	    "<ul><li><b>Enabled</b>: Show all animations completely.</li>"
	    "<li><b>Disabled</b>: Never show animations, show the start image only.</li>"
	    "<li><b>Show only once</b>: Show all animations completely but do not repeat them.</li>");
    QWhatsThis::add( label, whatsThis);
    QWhatsThis::add( m_pAnimationsCombo, whatsThis);
    connect(m_pAnimationsCombo, SIGNAL(activated(int)), this, SLOT(changed()));

    lay->setRowStretch(row, 1);

    load();
}

void KMiscHTMLOptions::load()
{
    // *** load ***
    m_pConfig->setGroup( "MainView Settings" );
    bool bBackRightClick = m_pConfig->readBoolEntry( "BackRightClick", false );
    m_pConfig->setGroup( "HTML Settings" );
    bool changeCursor = m_pConfig->readBoolEntry("ChangeCursor", KDE_DEFAULT_CHANGECURSOR);
    bool underlineLinks = m_pConfig->readBoolEntry("UnderlineLinks", DEFAULT_UNDERLINELINKS);
    bool hoverLinks = m_pConfig->readBoolEntry("HoverLinks", true);
    bool bAutoLoadImages = m_pConfig->readBoolEntry( "AutoLoadImages", true );
    bool bAutoRedirect = m_pConfig->readBoolEntry( "AutoDelayedActions", true );
    QString strAnimations = m_pConfig->readEntry( "ShowAnimations" ).lower();

    // *** apply to GUI ***
    cbCursor->setChecked( changeCursor );
    m_pAutoLoadImagesCheckBox->setChecked( bAutoLoadImages );
    m_pAutoRedirectCheckBox->setChecked( bAutoRedirect );
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
}

void KMiscHTMLOptions::defaults()
{
    cbCursor->setChecked( false );
    m_pAutoLoadImagesCheckBox->setChecked( true );
    m_pAutoRedirectCheckBox->setChecked( true );
    m_pUnderlineCombo->setCurrentItem( UnderlineAlways );
    m_pAnimationsCombo->setCurrentItem( AnimationsAlways );
    m_pFormCompletionCheckBox->setChecked(true);
    m_pMaxFormCompletionItems->setEnabled( true );
    m_pBackRightClick->setChecked( false );
    m_pMaxFormCompletionItems->setValue( 10 );
}

void KMiscHTMLOptions::save()
{
    m_pConfig->setGroup( "MainView Settings" );
    m_pConfig->writeEntry( "BackRightClick", m_pBackRightClick->isChecked() );
    m_pConfig->setGroup( "HTML Settings" );
    m_pConfig->writeEntry( "ChangeCursor", cbCursor->isChecked() );
    m_pConfig->writeEntry( "AutoLoadImages", m_pAutoLoadImagesCheckBox->isChecked() );
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

    m_pConfig->sync();
}


void KMiscHTMLOptions::changed()
{
    m_pMaxFormCompletionItems->setEnabled( m_pFormCompletionCheckBox->isChecked() );
    emit changed(true);
}
