//
//
// "Root Options" Tab for KDesktop configuration
//
// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
//
// Layouts
// (c) Christian Tibirna 1998
// Port to KControl, split from Misc Tab, Port to KControl2
// (c) David Faure 1998

#include <config.h>

#include <qlabel.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstddirs.h>
#include <kglobal.h>
#include <kconfig.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <assert.h>

#include "rootopts.h"

#include <konqdefaults.h> // include default values directly from libkonq

//-----------------------------------------------------------------------------

static const char * s_choices[4] = { "", "WindowListMenu", "DesktopMenu", "AppMenu" };

KRootOptions::KRootOptions( QWidget *parent, const char *name )
    : KCModule( parent, name )
{
  QLabel * tmpLabel;
#define RO_ROWS 7
#define RO_COLS 3
  int row = 0;
  QGridLayout *lay = new QGridLayout(this, RO_ROWS, RO_COLS, 10);
  
  lay->setRowStretch(0,0);
  lay->setRowStretch(1,0);
  lay->setRowStretch(2,0);
  lay->setRowStretch(3,0);
  lay->setRowStretch(4,0);
  lay->setRowStretch(5,0);
  lay->setRowStretch(6,10); // last line grows

  lay->setColStretch(0,1);
  lay->setColStretch(1,1);
  lay->setColStretch(2,5);

  showHiddenBox = new QCheckBox(i18n("Show &Hidden Files on Desktop"), this);
  lay->addMultiCellWidget(showHiddenBox, row, row, 0, RO_COLS);
  
  row++;
  QFrame * hLine = new QFrame(this);
  hLine->setFrameStyle(QFrame::Sunken|QFrame::HLine);
  lay->addMultiCellWidget(hLine, row, row, 0, RO_COLS);

  row++;
  tmpLabel = new QLabel( i18n("Clicks on the desktop"), this );
  lay->addMultiCellWidget( tmpLabel, row, row, 0, RO_COLS );

  row++;
  tmpLabel = new QLabel( i18n("Left button"), this );
  lay->addWidget( tmpLabel, row, 0 );
  leftComboBox = new QComboBox( this );
  lay->addWidget( leftComboBox, row, 1 );
  fillMenuCombo( leftComboBox );

  row++;
  tmpLabel = new QLabel( i18n("Middle button"), this );
  lay->addWidget( tmpLabel, row, 0 );
  middleComboBox = new QComboBox( this );
  lay->addWidget( middleComboBox, row, 1 );
  fillMenuCombo( middleComboBox );

  row++;
  tmpLabel = new QLabel( i18n("Right button"), this );
  lay->addWidget( tmpLabel, row, 0 );
  rightComboBox = new QComboBox( this );
  lay->addWidget( rightComboBox, row, 1 );
  fillMenuCombo( rightComboBox );

  row++;
  lay->addMultiCellWidget( new QWidget(this), row, row, 0, RO_COLS );

  assert( row == RO_ROWS - 1 );
  lay->activate();

  load();
}

void KRootOptions::fillMenuCombo( QComboBox * combo )
{
  combo->insertItem( i18n("No action") );
  combo->insertItem( i18n("Window List Menu") );
  combo->insertItem( i18n("Desktop Menu") );
  combo->insertItem( i18n("Application Menu") );
}

void KRootOptions::load()
{
    KConfig *config = new KConfig( "kdesktoprc" );
    config->setGroup( "Desktop Icons" );
    bool bShowHidden = config->readBoolEntry("ShowHidden", DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    showHiddenBox->setChecked(bShowHidden);
    //
    config->setGroup( "Mouse Buttons" );
    QString s = config->readEntry( "Left", "None" );
    for ( int c = 0 ; c < 4 ; c ++ )
      if (s == s_choices[c])
      { leftComboBox->setCurrentItem( c ); break; } 
    s = config->readEntry( "Middle", "WindowListMenu" );
    for ( int c = 0 ; c < 4 ; c ++ )
      if (s == s_choices[c])
      { middleComboBox->setCurrentItem( c ); break; } 
    s = config->readEntry( "Right", "DesktopMenu" );
    for ( int c = 0 ; c < 4 ; c ++ )
      if (s == s_choices[c])
      { rightComboBox->setCurrentItem( c ); break; } 
      
    delete config;
}

void KRootOptions::defaults()
{
    showHiddenBox->setChecked(DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    leftComboBox->setCurrentItem( NOTHING );
    middleComboBox->setCurrentItem( WINDOWLISTMENU );
    rightComboBox->setCurrentItem( DESKTOPMENU );
}

void KRootOptions::save()
{
    KConfig *config = new KConfig("kdesktoprc");
    config->setGroup( "Desktop Icons" );
    config->writeEntry("ShowHidden", showHiddenBox->isChecked());
    config->setGroup( "Mouse Buttons" );
    config->writeEntry("Left", s_choices[ leftComboBox->currentItem() ]);
    config->writeEntry("Middle", s_choices[ middleComboBox->currentItem() ]);
    config->writeEntry("Right", s_choices[ rightComboBox->currentItem() ]);

    config->sync();
    delete config;

    // Tell kdesktop about the new config file
    QString exeloc = locate("exe","kfmclient");
    if ( exeloc.isEmpty() ) {
      KMessageBox::error( 0L,
                          i18n( "Can't find the kfmclient program - can't apply configuration dynamically" ), i18n( "Error" ) );
      return;
    }
    if ( fork() == 0 )
    {
      execl(exeloc, "kfmclient", "configureDesktop", 0L);
      warning("Error launching 'kfmclient configureDesktop' !");
    }
}

extern "C"
{
  KCModule *create_icons(QWidget *parent, const char *name)
  {
    KGlobal::locale()->insertCatalogue("kcmkonq");
    return new KRootOptions(parent, name);
  };
}

#include "rootopts.moc"
