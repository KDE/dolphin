//
//
// "Desktop Icons Options" Tab for KDesktop configuration
//
// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
//
// Layouts
// (c) Christian Tibirna 1998
// Port to KControl, split from Misc Tab, Port to KControl2
// (c) David Faure 1998
// Desktop menus, paths
// (c) David Faure 2000

#include <config.h>

#include <qdir.h>
#include <dcopclient.h>
#include <kapp.h>
#include <kconfig.h>
#include <kglobalsettings.h>
#include <kglobal.h>
#include <klocale.h>
#include <kio/job.h>
#include <kmessagebox.h>
#include <kstddirs.h>
#include <kipc.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qwhatsthis.h>
#include <assert.h>

#include "rootopts.h"

#include <konqdefaults.h> // include default values directly from libkonq


//-----------------------------------------------------------------------------

static const char * s_choices[4] = { "", "WindowListMenu", "DesktopMenu", "AppMenu" };

KRootOptions::KRootOptions(KConfig *config, QWidget *parent, const char *name )
    : KCModule( parent, name ), g_pConfig(config)
{
  QLabel * tmpLabel;
#define RO_LASTROW 13   // 3 cb, 1 line, 3 combo, 1 line, 3 paths and 1 label + last row
#define RO_LASTCOL 2
  int row = 0;
  QGridLayout *lay = new QGridLayout(this, RO_LASTROW+1, RO_LASTCOL+1, 10);

  lay->setRowStretch(RO_LASTROW,10); // last line grows

  lay->setColStretch(0,0);
  lay->setColStretch(1,0);
  lay->setColStretch(2,10);

  VertAlignBox = new QCheckBox(i18n("Align Icons &Vertically on Desktop"), this);
  lay->addMultiCellWidget(VertAlignBox, row, row, 0, 1);
  connect(VertAlignBox, SIGNAL(clicked()), this, SLOT(changed()));
  QWhatsThis::add( VertAlignBox, i18n("Check this option if you want the icons"
    " on the desktop to be aligned vertically (in columns). If you leave this"
    " option unchecked, desktop icons are aligned horizontally.<p>"
    " Note that you can drag icons wherever you want to on the desktop. When"
    " you choose \"Arrange Icons\" from the Desktop menu, icons will be"
    " arranged horizontally or vertically.") );

  row++;
  showHiddenBox = new QCheckBox(i18n("Show &Hidden Files on Desktop"), this);
  lay->addMultiCellWidget(showHiddenBox, row, row, 0, 1);
  connect(showHiddenBox, SIGNAL(clicked()), this, SLOT(changed()));
  QWhatsThis::add( showHiddenBox, i18n("If you check this option, any files"
    " in your desktop directory that begin with a period (.) will be shown."
    " Usually, such files contain configuration information, and remain"
    " hidden from view.<p>"
    " For example, files which are named \".directory\" are plain text files"
    " which contain information for Konqueror, such as the icon to use in"
    " displaying a directory, the order in which files should be sorted, etc."
    " You should not change or delete these files unless you know what you"
    " are doing!") );

  row++;
  menuBarBox = new QCheckBox(i18n("Enable Desktop &Menu"), this);
  lay->addMultiCellWidget(menuBarBox, row, row, 0, 1);
  connect(menuBarBox, SIGNAL(clicked()), this, SLOT(changed()));
  QWhatsThis::add( menuBarBox, i18n("Check this option if you want the"
    " desktop popup menus to appear on the top of the screen in the style"
    " of Macintosh.  This setting is independent of the global top-level"
    " menu setting that applys to KDE applications.") );

  row++;
  QFrame * hLine2 = new QFrame(this);
  hLine2->setFrameStyle(QFrame::Sunken|QFrame::HLine);
  lay->addMultiCellWidget(hLine2, row, row, 0, RO_LASTCOL);

  row++;
  tmpLabel = new QLabel( i18n("Clicks on the desktop"), this );
  lay->addMultiCellWidget( tmpLabel, row, row, 0, RO_LASTCOL );

  row++;
  tmpLabel = new QLabel( i18n("Left button"), this );
  lay->addWidget( tmpLabel, row, 0 );
  leftComboBox = new QComboBox( this );
  lay->addWidget( leftComboBox, row, 1 );
  fillMenuCombo( leftComboBox );
  connect(leftComboBox, SIGNAL(activated(int)), this, SLOT(changed()));

  row++;
  tmpLabel = new QLabel( i18n("Middle button"), this );
  lay->addWidget( tmpLabel, row, 0 );
  middleComboBox = new QComboBox( this );
  lay->addWidget( middleComboBox, row, 1 );
  fillMenuCombo( middleComboBox );
  connect(middleComboBox, SIGNAL(activated(int)), this, SLOT(changed()));
  QWhatsThis::add( middleComboBox, i18n("You can choose what happens when"
    " you click the middle button of your pointing device on the desktop:"
    " <ul><li><em>No action:</em> as you might guess, nothing happens!</li>"
    " <li><em>Window list menu:</em> a menu showing all windows on all"
    " virtual desktops pops up. You can click on the desktop name to switch"
    " to that desktop, or on a window name to shift focus to that window,"
    " switching desktops if necessary, and restoring the window if it is"
    " hidden. Hidden or minimized windows are represented with their names"
    " in parentheses.</li>"
    " <li><em>Desktop menu:</em> a context menu for the desktop pops up."
    " Among other things, this menu has options for configuring the display,"
    " locking the screen, and logging out of KDE.</li>"
    " <li><em>Application menu:</em> the \"K\" menu pops up. This might be"
    " useful for quickly accessing applications if you like to keep the"
    " panel (also known as \"Kicker\") hidden from view.</li></ul>") );

  row++;
  tmpLabel = new QLabel( i18n("Right button"), this );
  lay->addWidget( tmpLabel, row, 0 );
  rightComboBox = new QComboBox( this );
  lay->addWidget( rightComboBox, row, 1 );
  fillMenuCombo( rightComboBox );
  connect(rightComboBox, SIGNAL(activated(int)), this, SLOT(changed()));
  QWhatsThis::add( rightComboBox, i18n("You can choose what happens when"
    " you click the right button of your pointing device on the desktop:"
    " <ul><li><em>No action:</em> as you might guess, nothing happens!</li>"
    " <li><em>Window list menu:</em> a menu showing all windows on all"
    " virtual desktops pops up. You can click on the desktop name to switch"
    " to that desktop, or on a window name to shift focus to that window,"
    " switching desktops if necessary, and restoring the window if it is"
    " hidden. Hidden or minimized windows are represented with their names"
    " in parentheses.</li>"
    " <li><em>Desktop menu:</em> a context menu for the desktop pops up."
    " Among other things, this menu has options for configuring the display,"
    " locking the screen, and logging out of KDE.</li>"
    " <li><em>Application menu:</em> the \"K\" menu pops up. This might be"
    " useful for quickly accessing applications if you like to keep the"
    " panel (also known as \"Kicker\") hidden from view.</li></ul>") );

  // Desktop Paths

  row++;
  QFrame * hLine = new QFrame(this);
  hLine->setFrameStyle(QFrame::Sunken|QFrame::HLine);
  lay->addMultiCellWidget(hLine, row, row, 0, RO_LASTCOL);

  row++;
  tmpLabel = new QLabel(i18n("Desktop path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  leDesktop = new QLineEdit(this);
  lay->addMultiCellWidget(leDesktop, row, row, 1, RO_LASTCOL);
  connect(leDesktop, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  QWhatsThis::add( leDesktop, i18n("This directory contains all the files"
    " which you see on your desktop. You can change the location of this"
    " directory if you want to, and the contents will move automatically"
    " to the new location as well.") );

  row++;
  tmpLabel = new QLabel(i18n("Trash path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  leTrash = new QLineEdit(this);
  lay->addMultiCellWidget(leTrash, row, row, 1, RO_LASTCOL);
  connect(leTrash, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  QWhatsThis::add( leTrash, i18n("This directory contains deleted files (until"
    " you empty the trashcan). You can change the location of this"
    " directory if you want to, and the contents will move automatically"
    " to the new location as well.") );

  row++;
  tmpLabel = new QLabel(i18n("Autostart path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  leAutostart = new QLineEdit(this);
  lay->addMultiCellWidget(leAutostart, row, row, 1, RO_LASTCOL);
  connect(leAutostart, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  QWhatsThis::add( leAutostart, i18n("This directory contains applications or"
    " links to applications (shortcuts) that you want to have started"
    " automatically whenever KDE starts. You can change the location of this"
    " directory if you want to, and the contents will move automatically"
    " to the new location as well.") );

  row++;
  tmpLabel = new QLabel(i18n("Note that changing a path automatically moves"
     " the contents of the directory.\nMoving them manually is not necessary."), this);
  lay->addMultiCellWidget(tmpLabel, row, row, 0, RO_LASTCOL);

  // -- Bottom --
  assert( row == RO_LASTROW-1 ); // if it fails here, check the row++ and RO_LASTROW above
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
    g_pConfig->setGroup( "Desktop Icons" );
    bool bShowHidden = g_pConfig->readBoolEntry("ShowHidden", DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    showHiddenBox->setChecked(bShowHidden);
    bool bVertAlign = g_pConfig->readNumEntry("VertAlign", DEFAULT_VERT_ALIGN);
    VertAlignBox->setChecked(bVertAlign);
    //
    g_pConfig->setGroup( "Menubar" );
    bool bMenuBar = g_pConfig->readBoolEntry("ShowMenubar", true);
    menuBarBox->setChecked(bMenuBar);
    //
    g_pConfig->setGroup( "Mouse Buttons" );
    QString s;
    s = g_pConfig->readEntry( "Left", "" );
    for ( int c = 0 ; c < 4 ; c ++ )
	if (s == s_choices[c])
	  { leftComboBox->setCurrentItem( c ); break; }
    s = g_pConfig->readEntry( "Middle", "WindowListMenu" );
    for ( int c = 0 ; c < 4 ; c ++ )
      if (s == s_choices[c])
      { middleComboBox->setCurrentItem( c ); break; }
    s = g_pConfig->readEntry( "Right", "DesktopMenu" );
    for ( int c = 0 ; c < 4 ; c ++ )
      if (s == s_choices[c])
      { rightComboBox->setCurrentItem( c ); break; }

    // Desktop Paths
    leDesktop->setText( KGlobalSettings::desktopPath() );
    leTrash->setText( KGlobalSettings::trashPath() );
    leAutostart->setText( KGlobalSettings::autostartPath() );
}

void KRootOptions::defaults()
{
    showHiddenBox->setChecked(DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    VertAlignBox->setChecked(true);
    menuBarBox->setChecked(true);
    leftComboBox->setCurrentItem( NOTHING );
    middleComboBox->setCurrentItem( WINDOWLISTMENU );
    rightComboBox->setCurrentItem( DESKTOPMENU );

    // Desktop Paths - keep defaults in sync with kglobalsettings.cpp
    leDesktop->setText( QDir::homeDirPath() + "/Desktop/" );
    leTrash->setText( QDir::homeDirPath() + "/Desktop/Trash/" );
    leAutostart->setText( KGlobal::dirs()->localkdedir() + "Autostart/" );
}

void KRootOptions::save()
{
    g_pConfig->setGroup( "Desktop Icons" );
    g_pConfig->writeEntry("ShowHidden", showHiddenBox->isChecked());
    g_pConfig->writeEntry("VertAlign",VertAlignBox->isChecked());
    g_pConfig->setGroup( "Menubar" );
    g_pConfig->writeEntry("ShowMenubar", menuBarBox->isChecked());
    g_pConfig->setGroup( "Mouse Buttons" );
    g_pConfig->writeEntry("Left", s_choices[ leftComboBox->currentItem() ] );
    g_pConfig->writeEntry("Middle", s_choices[ middleComboBox->currentItem() ]);
    g_pConfig->writeEntry("Right", s_choices[ rightComboBox->currentItem() ]);

    KConfig *config = KGlobal::config();
    KConfigGroupSaver cgs( config, "Paths" );

    bool pathChanged = false;

    if ( leDesktop->text() != KGlobalSettings::desktopPath() )
    {
        //TODO
        // Test which other paths were inside this one (as it is by default)
        // and for each, test where it should go.
        // * Inside destination -> let them be moved with the desktop (but adjust name if necessary)
        // * Not inside destination -> move first
        // !!!

        moveDir( KGlobalSettings::desktopPath(), leDesktop->text() );
        config->writeEntry( "Desktop", leDesktop->text(), true, true );
        pathChanged = true;
    }

    if ( leTrash->text() != KGlobalSettings::trashPath() )
    {
        moveDir( KGlobalSettings::trashPath(), leTrash->text() );
        config->writeEntry( "Trash", leTrash->text(), true, true );
        pathChanged = true;
    }

    if ( leAutostart->text() != KGlobalSettings::autostartPath() )
    {
        moveDir( KGlobalSettings::autostartPath(), leAutostart->text() );
        config->writeEntry( "Autostart", leAutostart->text(), true, true );
        pathChanged = true;
    }

    if (pathChanged)
        KIPC::sendMessageAll(KIPC::SettingsChanged, KApplication::SETTINGS_PATHS);

    config->sync();
    g_pConfig->sync();

}

void KRootOptions::moveDir( QString src, QString dest )
{
    KURL src_url;
    src_url.setPath(src);
    KURL dest_url;
    dest_url.setPath(dest);
    KIO::Job * job = KIO::move( src_url, dest_url );
    connect( job, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotResult( KIO::Job * ) ) );
    // wait for job
    qApp->enter_loop();
}

void KRootOptions::slotResult( KIO::Job * job )
{
    qApp->exit_loop();
    if (job->error())
        job->showErrorDialog(this);
}


void KRootOptions::changed()
{
  emit KCModule::changed(true);
}


QString KRootOptions::quickHelp()
{
  return i18n("<h1>Desktop</h1>\n"
    "This module allows you to choose various options\n"
    "for your desktop, including the way in which icons are arranged, the\n"
    "location of your desktop directory, and the pop-up menus associated\n"
    "with clicks of the middle and right mouse buttons on the desktop.\n"
    "Use the \"Whats This?\" (Shift+F1) to get help on specific options.");
}


#include "rootopts.moc"
