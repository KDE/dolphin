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
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstddirs.h>
#include <kglobal.h>
#include <kconfig.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "rootopts.h"

#include <konqdefaults.h> // include default values directly from libkonq

//-----------------------------------------------------------------------------

KRootOptions::KRootOptions( QWidget *parent, const char *name )
    : KCModule( parent, name )
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

  load();
}

void KRootOptions::load()
{
    KConfig *config = KGlobal::config();
    config->setGroup( "Desktop Icons" );
    bool bShowHidden = config->readBoolEntry("ShowHidden", DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    showHiddenBox->setChecked(bShowHidden);
}

void KRootOptions::defaults()
{
    showHiddenBox->setChecked(DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
}

void KRootOptions::save()
{
    KConfig *config = KGlobal::config();
    config->setGroup( "Desktop Icons" );
    config->writeEntry("ShowHidden", showHiddenBox->isChecked());

    config->sync();

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
