// Behaviour options for konqueror


#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qslider.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qradiobutton.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kdialog.h>
#include <konqdefaults.h>


#include "behaviour.h"

KBehaviourOptions::KBehaviourOptions(KConfig *config, QString group, bool showBuiltinGroup, QWidget *parent, const char *name )
    : KCModule(parent, name), g_pConfig(config), groupname(group), m_bShowBuiltinGroup(showBuiltinGroup)
{
    QLabel * label;
    int row = 0;

#define N_COLS 2
#define N_ROWS 8
    QGridLayout *lay = new QGridLayout(this,N_ROWS,N_COLS, // rows, cols
                                       KDialog::marginHint(),
				       KDialog::spacingHint());     // border, space
    lay->setRowStretch(N_ROWS-1,1);

    cbAutoSelect = new QCheckBox(i18n("&Automatically select icons"), this);
    lay->addMultiCellWidget(cbAutoSelect,row,row,0,N_COLS,Qt::AlignLeft);
    connect(cbAutoSelect, SIGNAL(clicked()), this, SLOT(changed()));

    //----------
    row++;
    slAutoSelect = new QSlider(0, 2000, 10, 0, QSlider::Horizontal, this);
    slAutoSelect->setSteps( 125, 125 );
    slAutoSelect->setTickmarks( QSlider::Below );
    slAutoSelect->setTickInterval( 250 );
    slAutoSelect->setTracking( true );
    lay->addMultiCellWidget(slAutoSelect,row,row,1,N_COLS);
    connect(slAutoSelect, SIGNAL(valueChanged(int)), this, SLOT(changed()));

    lDelay = new QLabel(slAutoSelect, i18n("De&lay:"), this);
    lDelay->adjustSize();
    lay->addWidget(lDelay,row,0);

    row++;
    label = new QLabel(i18n("Small"), this);
    lay->addWidget(label,row,1);

    label = new QLabel(i18n("Large"), this);
    lay->addWidget(label,row,2, Qt::AlignRight);
    //----------

    row++;
    cbCursor = new QCheckBox(i18n("&Change cursor shape when over an icon"), this);
    lay->addMultiCellWidget(cbCursor,row,row,0,N_COLS,Qt::AlignLeft);
    connect(cbCursor, SIGNAL(clicked()), this, SLOT(changed()));

    row++;
    cbUnderline = new QCheckBox(i18n("&Underline filenames"), this);
    lay->addMultiCellWidget(cbUnderline,row,row,0,N_COLS,Qt::AlignLeft);
    connect(cbUnderline, SIGNAL(clicked()), this, SLOT(changed()));

    connect( cbAutoSelect, SIGNAL( clicked() ), this, SLOT( slotClick() ) );

    row++;
    cbNewWin = new QCheckBox(i18n("&Open directories in separate windows"), this);
    lay->addMultiCellWidget(cbNewWin, row, row, 0, N_COLS-1, Qt::AlignLeft);
    connect(cbNewWin, SIGNAL(clicked()), this, SLOT(changed()));

    
    winPixmap = new QLabel(this);
    winPixmap->setPixmap(QPixmap(locate("data", 
					"kcontrol/pics/onlyone.png")));
    lay->addWidget(winPixmap, row, N_COLS);
    connect(cbNewWin, SIGNAL(toggled(bool)), SLOT(updateWinPixmap(bool)));

    // ----
    if (m_bShowBuiltinGroup)
    {
      row++;
      QGroupBox *gbox = new QGroupBox(i18n("Use builtin viewer for"), this);
      lay->addMultiCellWidget(gbox,row,row,0,N_COLS,Qt::AlignLeft);

      QGridLayout *grid = new QGridLayout(gbox, 4, 0,
                                          KDialog::marginHint(),
                                          KDialog::spacingHint());
      grid->addRowSpacing(0,10);
      grid->setRowStretch(0,0);

      cbEmbedText = new QCheckBox( i18n("Text files"), gbox );
      grid->addWidget( cbEmbedText, 1, 0 );
      connect(cbEmbedText, SIGNAL(clicked()), this, SLOT(changed()));

      cbEmbedImage = new QCheckBox( i18n("Image files"), gbox );
      grid->addWidget( cbEmbedImage, 2, 0 );
      connect(cbEmbedImage, SIGNAL(clicked()), this, SLOT(changed()));

      cbEmbedOther = new QCheckBox( i18n("Other (DVI,PS...)"), gbox );
      grid->addWidget( cbEmbedOther, 3, 0 );
      connect(cbEmbedOther, SIGNAL(clicked()), this, SLOT(changed()));

      gbox->setMinimumWidth( cbEmbedOther->width()+50 ); // workaround for groupbox title truncated
    }

    load();
}


void KBehaviourOptions::load()
{
    g_pConfig->setGroup( groupname );
    int  autoSelect = g_pConfig->readNumEntry("AutoSelect", DEFAULT_AUTOSELECT);
    if ( autoSelect < 0 ) autoSelect = 0;
    bool changeCursor = g_pConfig->readBoolEntry("ChangeCursor", DEFAULT_CHANGECURSOR);
    bool underlineLinks = g_pConfig->readBoolEntry("UnderlineLinks", DEFAULT_UNDERLINELINKS);

    cbAutoSelect->setChecked( autoSelect > 0 );
    slAutoSelect->setValue( autoSelect );
    cbCursor->setChecked( changeCursor );
    cbUnderline->setChecked( underlineLinks );
    cbNewWin->setChecked(g_pConfig->readBoolEntry("AlwaysNewWin", false));
    updateWinPixmap(cbNewWin->isChecked());

    if (m_bShowBuiltinGroup)
    {
        bool embedText = g_pConfig->readBoolEntry("EmbedText", true);
        bool embedImage = g_pConfig->readBoolEntry("EmbedImage", true);
        bool embedOther = g_pConfig->readBoolEntry("EmbedOther", true);

        cbEmbedText->setChecked( embedText );
        cbEmbedImage->setChecked( embedImage );
        cbEmbedOther->setChecked( embedOther );
    }

    slotClick();
}

void KBehaviourOptions::defaults()
{
    cbAutoSelect->setChecked( false );
    slAutoSelect->setValue( 50 );
    cbCursor->setChecked( false );
    cbUnderline->setChecked( true );
    cbNewWin->setChecked(false);

    if (m_bShowBuiltinGroup)
    {
        cbEmbedText->setChecked( true );
        cbEmbedImage->setChecked( true );
        cbEmbedOther->setChecked( true );
    }

    slotClick();
}

void KBehaviourOptions::save()
{
    g_pConfig->setGroup( groupname );
    g_pConfig->writeEntry( "AutoSelect", cbAutoSelect->isChecked()?slAutoSelect->value():-1 );
    g_pConfig->writeEntry( "ChangeCursor", cbCursor->isChecked() );
    g_pConfig->writeEntry( "UnderlineLinks", cbUnderline->isChecked() );

    if (m_bShowBuiltinGroup)
    {
        g_pConfig->writeEntry( "EmbedText", cbEmbedText->isChecked() );
        g_pConfig->writeEntry( "EmbedImage", cbEmbedImage->isChecked() );
        g_pConfig->writeEntry( "EmbedOther", cbEmbedOther->isChecked() );
    }

    g_pConfig->writeEntry( "AlwaysNewWin", cbNewWin->isChecked() );

    g_pConfig->sync();
}

void KBehaviourOptions::slotClick()
{
  g_pConfig->setGroup("KDE");
  bool singleClick =  g_pConfig->readBoolEntry("SingleClick", true);
  g_pConfig->setGroup( groupname );
  // Autoselect has a meaning only in single-click mode
  cbAutoSelect->setEnabled(singleClick);
  // Delay has a meaning only for autoselect
  bool bDelay = cbAutoSelect->isChecked() && singleClick;
  slAutoSelect->setEnabled( bDelay );
  lDelay->setEnabled( bDelay );
}

void KBehaviourOptions::updateWinPixmap(bool b)
{
  if (b)
    winPixmap->setPixmap(QPixmap(locate("data", 
					"kcontrol/pics/overlapping.png")));
  else
    winPixmap->setPixmap(QPixmap(locate("data", 
					"kcontrol/pics/onlyone.png")));
}

void KBehaviourOptions::changed()
{
  emit KCModule::changed(true);
}


#include "behaviour.moc"
