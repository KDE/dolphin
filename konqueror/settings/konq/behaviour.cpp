// Behaviour options for konqueror


#include <qcheckbox.h>
#include <qslider.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qradiobutton.h>
#include <kconfig.h>
#include <klocale.h>
#include <konqdefaults.h>


#include "behaviour.h"


KBehaviourOptions::KBehaviourOptions( QWidget *parent, const char *name )
    : KCModule( parent, name )
{
    QLabel * label;
    int row = 0;

#define N_COLS 2
#define N_ROWS 7
    QGridLayout *lay = new QGridLayout(this,N_ROWS,N_COLS, // rows, cols
                                       20,15);     // border, space
    lay->setRowStretch(0,1);
    lay->setRowStretch(1,1);
    lay->setRowStretch(2,1);
    lay->setRowStretch(3,0);
    lay->setRowStretch(4,1);
    lay->setRowStretch(5,1);
    lay->setRowStretch(6,1);

    lay->setColStretch(0,0);
    lay->setColStretch(1,1);

    cbSingleClick = new QCheckBox(i18n("&Single click to activate"), this);
    lay->addMultiCellWidget(cbSingleClick,row,row,0,N_COLS,Qt::AlignLeft);
    connect(cbSingleClick, SIGNAL(clicked()), this, SLOT(changed()));

    row++;
    cbAutoSelect = new QCheckBox(i18n("&Auto select"), this);
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
    cbCursor = new QCheckBox(i18n("&Change cursor over link"), this);
    lay->addMultiCellWidget(cbCursor,row,row,0,N_COLS,Qt::AlignLeft);
    connect(cbCursor, SIGNAL(clicked()), this, SLOT(changed()));

    row++;
    cbUnderline = new QCheckBox(i18n("&Underline links"), this);
    lay->addMultiCellWidget(cbUnderline,row,row,0,N_COLS,Qt::AlignLeft);
    connect(cbUnderline, SIGNAL(clicked()), this, SLOT(changed()));

    connect( cbSingleClick, SIGNAL( clicked() ), this, SLOT( slotClick() ) );
    connect( cbAutoSelect, SIGNAL( clicked() ), this, SLOT( slotClick() ) );

    load();
}


void KBehaviourOptions::load()
{
    KConfig *config = new KConfig("konquerorrc");

    config->setGroup( "Behaviour" );	
    bool singleClick = config->readBoolEntry("SingleClick", DEFAULT_SINGLECLICK);
    int  autoSelect = config->readNumEntry("AutoSelect", DEFAULT_AUTOSELECT);
    if ( autoSelect < 0 ) autoSelect = 0;
    bool changeCursor = config->readBoolEntry("ChangeCursor", DEFAULT_CHANGECURSOR);
    bool underlineLinks = config->readBoolEntry("UnderlineLinks", DEFAULT_UNDERLINELINKS);

    cbSingleClick->setChecked( singleClick );
    cbAutoSelect->setChecked( autoSelect > 0 );
    slAutoSelect->setValue( autoSelect );
    cbCursor->setChecked( changeCursor );
    cbUnderline->setChecked( underlineLinks );

    slotClick();

    delete config;
}

void KBehaviourOptions::defaults()
{
    cbSingleClick->setChecked( true );
    cbAutoSelect->setChecked( false );
    slAutoSelect->setValue( 50 );
    cbCursor->setChecked( false );
    cbUnderline->setChecked( true );

    slotClick();
}

void KBehaviourOptions::save()
{
    KConfig *config = new KConfig("konquerorrc");

    config->setGroup( "Behaviour" );			
    config->writeEntry( "SingleClick", cbSingleClick->isChecked() );
    config->writeEntry( "AutoSelect", cbAutoSelect->isChecked()?slAutoSelect->value():-1 );
    config->writeEntry( "ChangeCursor", cbCursor->isChecked() );
    config->writeEntry( "UnderlineLinks", cbUnderline->isChecked() );
    config->sync();

    delete config;
}

void KBehaviourOptions::slotClick()
{
    // Autoselect has a meaning only in single-click mode
    cbAutoSelect->setEnabled( cbSingleClick->isChecked() );
    // Delay has a meaning only for autoselect
    bool bDelay = cbAutoSelect->isChecked() && cbSingleClick->isChecked();
    slAutoSelect->setEnabled( bDelay );
    lDelay->setEnabled( bDelay );
}


void KBehaviourOptions::changed()
{
  emit KCModule::changed(true);
}


#include "behaviour.moc"
