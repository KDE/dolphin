//
//
// "Root Options" Tab for KFM configuration
//
// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
//
// Layouts
// (c) Christian Tibirna 1998
// Port to KControl, split from Misc Tab
// (c) David Faure 1998

#include <qlabel.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlayout.h>//CT - 12Nov1998
#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>

#include "rootopts.h"

#include <konqdefaults.h> // include default values directly from kfm

//-----------------------------------------------------------------------------

KRootOptions::KRootOptions( QWidget *parent, const char *name )
    : KConfigWidget( parent, name )
{
#define RO_ROWS 3
#define RO_COLS 1
  int row = 0;
  QGridLayout *lay = new QGridLayout(this, RO_ROWS, RO_COLS, 10);
  
  lay->setRowStretch(0,0);
  lay->setRowStretch(1,0);
  lay->setRowStretch(2,1);

  showHiddenBox = new QCheckBox(i18n("Show &Hidden Files on Desktop"), this);
  lay->addMultiCellWidget(showHiddenBox, row, row, 0, 0);
  
  row++;
  QFrame * hLine = new QFrame(this);
  hLine->setFrameStyle(QFrame::Sunken|QFrame::HLine);
  lay->addMultiCellWidget(hLine, row, row, 0, 0);

  lay->activate();

  loadSettings();
}

void KRootOptions::loadSettings()
{
    // Root Icons settings
    g_pConfig->setGroup( "Desktop Icons" );
    bool bShowHidden = g_pConfig->readBoolEntry("ShowHidden", DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    showHiddenBox->setChecked(bShowHidden);
}

void KRootOptions::defaultSettings()
{
    // Root Icons Settings
    showHiddenBox->setChecked(DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
}

void KRootOptions::saveSettings()
{
    // Root Icons Settings
    g_pConfig->setGroup( "KFM Root Icons" );
    g_pConfig->writeEntry("ShowHidden", showHiddenBox->isChecked());

    g_pConfig->sync();
}

void KRootOptions::applySettings()
{
    saveSettings();
}

#include "rootopts.moc"
