//
//
// "Desktop Options" Tab for KDesktop configuration
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

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdir.h>
#include <qgrid.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qvgroupbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kglobalsettings.h>
#include <klistview.h>
#include <klocale.h>
#include <kipc.h>
#include <kmessagebox.h>
#include <ktrader.h>
#include <kseparator.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>
#include <kmimetype.h>
#include <kcustommenueditor.h>

#include <X11/Xlib.h>
#include <dcopclient.h>
#include <kio/job.h>

#include "rootopts.h"

#include <konq_defaults.h> // include default values directly from libkonq

//-----------------------------------------------------------------------------

class KRootOptPreviewItem : public QCheckListItem
{
public:
    KRootOptPreviewItem(KRootOptions *rootOpts, QListView *parent,
                const KService::Ptr &plugin, bool on)
        : QCheckListItem(parent, plugin->name(), CheckBox),
          m_rootOpts(rootOpts)
    {
        m_pluginName = plugin->desktopEntryName();
        setOn(on);
    }
    KRootOptPreviewItem(KRootOptions *rootOpts, QListView *parent,
                bool on)
        : QCheckListItem(parent, i18n("Sound Files"), CheckBox),
          m_rootOpts(rootOpts)
    {
        m_pluginName = "audio/";
        setOn(on);
    }
    const QString &pluginName() const { return m_pluginName; }

protected:
    virtual void stateChange( bool ) { m_rootOpts->changed(); }

private:
    KRootOptions *m_rootOpts;
    QString m_pluginName;
};


class KRootOptDevicesItem : public QCheckListItem
{
public:
    KRootOptDevicesItem(KRootOptions *rootOpts, QListView *parent,
                const QString name, const QString mimetype, bool on)
        : QCheckListItem(parent, name, CheckBox),
          m_rootOpts(rootOpts),m_mimeType(mimetype){setOn(on);}

    const QString &mimeType() const { return m_mimeType; }

protected:
    virtual void stateChange( bool ) { m_rootOpts->changed(); }

private:
    KRootOptions *m_rootOpts;
    QString m_mimeType;
};



static const char * s_choices[6] = { "", "WindowListMenu", "DesktopMenu", "AppMenu", "CustomMenu1", "CustomMenu2" };

KRootOptions::KRootOptions(KConfig *config, QWidget *parent, const char * )
    : KCModule( parent, "kcmkonq" ), g_pConfig(config)
{
#define RO_LASTROW 3   // 2 GroupBoxes + last row = 3 rows. But it starts at 0 ;)
#define RO_LASTCOL 2
  int row = 0;
  QGridLayout *lay = new QGridLayout(this, RO_LASTROW+1, RO_LASTCOL+1, KDialog::spacingHint());
  QString strMouseButton1, strMouseButton3, strButtonTxt1, strButtonTxt3;
  bool leftHandedMouse;

  lay->setRowStretch(RO_LASTROW, 10); // last line grows

  /*
  lay->setColStretch(0,0);
  lay->setColStretch(1,0);
  lay->setColStretch(2,10);
  */

  QGroupBox *groupBox = new QVGroupBox(i18n("Misc Options"), this);
  lay->addWidget(groupBox, 0, 0);

  /*
   * The text on this form depends on the mouse setting, which can be right
   * or left handed.  The outer button functionality is actually swapped
   *
   */
  leftHandedMouse = ( KGlobalSettings::mouseSettings().handed == KGlobalSettings::KMouseSettings::LeftHanded);


  menuBarBox = new QCheckBox(i18n("Enable desktop &menu"), groupBox);
  connect(menuBarBox, SIGNAL(clicked()), this, SLOT(changed()));
  QWhatsThis::add( menuBarBox, i18n("Check this option if you want the"
                                    " desktop popup menus to appear at the top of the screen in the style"
                                    " of Macintosh. This setting is independent of the global top-level"
                                    " menu setting that applies to KDE applications.") );

  iconsEnabledBox = new QCheckBox(i18n("Enable &icons on desktop"), groupBox);
  connect(iconsEnabledBox, SIGNAL(clicked()), this, SLOT(enableChanged()));
  QWhatsThis::add( iconsEnabledBox, i18n("Uncheck this option if you do not want to have icons on the desktop."
                                      " Without icons the desktop will be somewhat faster but you will no"
                                      " longer be able to drag files to the desktop." ) );


  showHiddenBox = new QCheckBox(i18n("Show h&idden files on desktop"), groupBox);
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

  vrootBox = new QCheckBox(i18n("Pr&ograms in desktop window"), groupBox);
  connect(vrootBox, SIGNAL(clicked()), this, SLOT(changed()));
  QWhatsThis::add( vrootBox, i18n("Check this option if you want to"
                                    " run X11 programs that draw into the desktop such as xsnow, xpenguin or"
                                    " xmountain. If you have problems with applications like netscape that check"
                                    " the root window for running instances, disable this option.") );

  previewListView = new KListView( this );
  previewListView->setFullWidth(true);
  previewListView->addColumn( i18n("Show Previews for:") );
  QWhatsThis::add(previewListView, i18n("Select for which types of files you want to"
                                        " enable preview images"));

  lay->addWidget(previewListView, row, RO_LASTCOL);

  row++;
  groupBox = new QVGroupBox( i18n("Clicks on the Desktop"), this );
  lay->addMultiCellWidget( groupBox, row, row, 0, RO_LASTCOL );

  QWidget *grid = new QWidget(groupBox);

  strMouseButton1 = i18n("Left button:");
  strButtonTxt1 = i18n( "You can choose what happens when"
   " you click the left button of your pointing device on the desktop:");

  strMouseButton3 = i18n("Right button:");
  strButtonTxt3 = i18n( "You can choose what happens when"
   " you click the right button of your pointing device on the desktop:");

  if ( leftHandedMouse )
  {
     qSwap(strMouseButton1, strMouseButton3);
     qSwap(strButtonTxt1, strButtonTxt3);
  }

  QLabel *leftLabel = new QLabel( strMouseButton1, grid );
  leftComboBox = new QComboBox( grid );
  leftEditButton = new QPushButton( i18n("Edit..."), grid);
  leftLabel->setBuddy( leftComboBox );
  fillMenuCombo( leftComboBox );
  connect(leftEditButton, SIGNAL(clicked()), this, SLOT(editButtonPressed()));
  connect(leftComboBox, SIGNAL(activated(int)), this, SLOT(changed()));
  connect(leftComboBox, SIGNAL(activated(int)), this, SLOT(comboBoxChanged()));
  QString wtstr = strButtonTxt1 +
                  i18n(" <ul><li><em>No action:</em> as you might guess, nothing happens!</li>"
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
                       " panel (also known as \"Kicker\") hidden from view.</li></ul>");
  QWhatsThis::add( leftLabel, wtstr );
  QWhatsThis::add( leftComboBox, wtstr );

  QLabel *middleLabel = new QLabel( i18n("Middle button:"), grid );
  middleComboBox = new QComboBox( grid );
  middleEditButton = new QPushButton( i18n("Edit..."), grid);
  middleLabel->setBuddy( middleComboBox );
  fillMenuCombo( middleComboBox );
  connect(middleEditButton, SIGNAL(clicked()), this, SLOT(editButtonPressed()));
  connect(middleComboBox, SIGNAL(activated(int)), this, SLOT(changed()));
  connect(middleComboBox, SIGNAL(activated(int)), this, SLOT(comboBoxChanged()));
  wtstr = i18n("You can choose what happens when"
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
               " panel (also known as \"Kicker\") hidden from view.</li></ul>");
  QWhatsThis::add( middleLabel, wtstr );
  QWhatsThis::add( middleComboBox, wtstr );

  QLabel *rightLabel = new QLabel( strMouseButton3, grid );
  rightComboBox = new QComboBox( grid );
  rightEditButton = new QPushButton( i18n("Edit..."), grid);
  rightLabel->setBuddy( rightComboBox );
  fillMenuCombo( rightComboBox );
  connect(rightEditButton, SIGNAL(clicked()), this, SLOT(editButtonPressed()));
  connect(rightComboBox, SIGNAL(activated(int)), this, SLOT(changed()));
  connect(rightComboBox, SIGNAL(activated(int)), this, SLOT(comboBoxChanged()));
  wtstr = strButtonTxt3 +
          i18n(" <ul><li><em>No action:</em> as you might guess, nothing happens!</li>"
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
               " panel (also known as \"Kicker\") hidden from view.</li></ul>");
  QWhatsThis::add( rightLabel, wtstr );
  QWhatsThis::add( rightComboBox, wtstr );

  QGridLayout *gridLayout = new QGridLayout(grid, 3, 3);
  gridLayout->setColStretch(0, 0);
  gridLayout->setColStretch(1, 1);
  gridLayout->setColStretch(1, 0);

  gridLayout->addWidget(leftLabel, 0, 0, AlignVCenter | AlignLeft);
  gridLayout->addWidget(leftComboBox, 0, 1, AlignCenter);
  gridLayout->addWidget(leftEditButton, 0, 2, AlignCenter);
  gridLayout->addWidget(middleLabel, 1, 0, AlignVCenter | AlignLeft);
  gridLayout->addWidget(middleComboBox, 1, 1, AlignCenter);
  gridLayout->addWidget(middleEditButton, 1, 2, AlignCenter);
  gridLayout->addWidget(rightLabel, 2, 0, AlignVCenter | AlignLeft);
  gridLayout->addWidget(rightComboBox, 2, 1, AlignCenter);
  gridLayout->addWidget(rightEditButton, 2, 2, AlignCenter);
  
  //BEGIN devices configuration
  row++;
  groupBox = new QVGroupBox( i18n("Display Devices"), this );
  lay->addMultiCellWidget( groupBox, row, row, 0, RO_LASTCOL );
  
  enableDevicesBox = new QCheckBox(i18n("Enable"), groupBox);
  connect(enableDevicesBox, SIGNAL(clicked()), this, SLOT(changed()));
  connect(enableDevicesBox, SIGNAL(clicked()), this, SLOT(enableDevicesBoxChanged()));  
  devicesListView = new KListView( groupBox );
  devicesListView->setFullWidth(true);
  devicesListView->addColumn( i18n("Types to Display") );
  QWhatsThis::add(devicesListView, i18n("Deselect the device types which you do not want to see on the desktop"));
  //END devices configuration
  
  // -- Bottom --
  Q_ASSERT( row == RO_LASTROW-1 ); // if it fails here, check the row++ and RO_LASTROW above
  
  load();
}


void KRootOptions::fillDevicesListView()
{

    devicesListView->clear();
    devicesListView->setRootIsDecorated(true);
    KMimeType::List mimetypes = KMimeType::allMimeTypes();
    QValueListIterator<KMimeType::Ptr> it2(mimetypes.begin());
    g_pConfig->setGroup( "Devices" );
    enableDevicesBox->setChecked(g_pConfig->readBoolEntry("enabled",true));
    QString excludedDevices=g_pConfig->readEntry("exclude","kdedevice/hdd_mounted,kdedevice/hdd_unmounted,kdedevice/floppy_unmounted,kdedevice/cdrom_unmounted,kdedevice/floppy5_unmounted");
    for (; it2 != mimetypes.end(); ++it2) {
	if ((*it2)->name().startsWith("kdedevice/"))
	{
    	    bool ok=excludedDevices.contains((*it2)->name())==0;
		new KRootOptDevicesItem (this, devicesListView, (*it2)->comment(), (*it2)->name(),ok);
		
        }
    }
    devicesListView->setEnabled(enableDevicesBox->isChecked());
}

void KRootOptions::enableDevicesBoxChanged()
{
	devicesListView->setEnabled(enableDevicesBox->isChecked());
}

void KRootOptions::saveDevicesListView()
{
    g_pConfig->setGroup( "Devices" );
    g_pConfig->writeEntry("enabled",enableDevicesBox->isChecked());
    QStringList exclude;
    for (KRootOptDevicesItem *it=static_cast<KRootOptDevicesItem *>(devicesListView->firstChild());
     	it; it=static_cast<KRootOptDevicesItem *>(it->nextSibling()))
    	{
		if (!it->isOn()) exclude << it->mimeType();
	    }
     g_pConfig->writeEntry("exclude",exclude);   
}


void KRootOptions::fillMenuCombo( QComboBox * combo )
{
  combo->insertItem( i18n("No Action") );
  combo->insertItem( i18n("Window List Menu") );
  combo->insertItem( i18n("Desktop Menu") );
  combo->insertItem( i18n("Application Menu") );
  combo->insertItem( i18n("Custom Menu 1") );
  combo->insertItem( i18n("Custom Menu 2") );
}

void KRootOptions::load()
{
    g_pConfig->setGroup( "Desktop Icons" );
    bool bShowHidden = g_pConfig->readBoolEntry("ShowHidden", DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    showHiddenBox->setChecked(bShowHidden);
    bool bVertAlign = g_pConfig->readBoolEntry("VertAlign", DEFAULT_VERT_ALIGN);
    KTrader::OfferList plugins = KTrader::self()->query("ThumbCreator");
    previewListView->clear();
    QStringList previews = g_pConfig->readListEntry("Preview");
    for (KTrader::OfferList::ConstIterator it = plugins.begin(); it != plugins.end(); ++it)
        new KRootOptPreviewItem(this, previewListView, *it, previews.contains((*it)->desktopEntryName()));
    new KRootOptPreviewItem(this, previewListView, previews.contains("audio/"));
    //
    g_pConfig->setGroup( "Menubar" );
    bool bMenuBar = g_pConfig->readBoolEntry("ShowMenubar", false);
    menuBarBox->setChecked(bMenuBar);
    g_pConfig->setGroup( "General" );
    vrootBox->setChecked( g_pConfig->readBoolEntry( "SetVRoot", false ) );
    iconsEnabledBox->setChecked( g_pConfig->readBoolEntry( "Enabled", true ) );

    //
    g_pConfig->setGroup( "Mouse Buttons" );
    QString s;
    s = g_pConfig->readEntry( "Left", "" );
    for ( int c = 0 ; c < 6 ; c ++ )
    if (s == s_choices[c])
      { leftComboBox->setCurrentItem( c ); break; }
    s = g_pConfig->readEntry( "Middle", "WindowListMenu" );
    for ( int c = 0 ; c < 6 ; c ++ )
      if (s == s_choices[c])
      { middleComboBox->setCurrentItem( c ); break; }
    s = g_pConfig->readEntry( "Right", "DesktopMenu" );
    for ( int c = 0 ; c < 6 ; c ++ )
      if (s == s_choices[c])
      { rightComboBox->setCurrentItem( c ); break; }

    m_wheelSwitchesWorkspace = g_pConfig->readBoolEntry("WheelSwitchesWorkspace", false);

    comboBoxChanged();
    fillDevicesListView();
    enableChanged();
}

void KRootOptions::defaults()
{
    showHiddenBox->setChecked(DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    for (QListViewItem *item = previewListView->firstChild(); item; item = item->nextSibling())
        static_cast<KRootOptPreviewItem *>(item)->setOn(false);
    menuBarBox->setChecked(false);
    vrootBox->setChecked( false );
    leftComboBox->setCurrentItem( NOTHING );
    middleComboBox->setCurrentItem( WINDOWLISTMENU );
    rightComboBox->setCurrentItem( DESKTOPMENU );
    iconsEnabledBox->setChecked(true);
    enableChanged();
}


void KRootOptions::save()
{
    g_pConfig->setGroup( "Desktop Icons" );
    g_pConfig->writeEntry("ShowHidden", showHiddenBox->isChecked());
    QStringList previews;
    for ( KRootOptPreviewItem *item = static_cast<KRootOptPreviewItem *>( previewListView->firstChild() );
          item;
          item = static_cast<KRootOptPreviewItem *>( item->nextSibling() ) )
        if ( item->isOn() )
            previews.append( item->pluginName() );
    g_pConfig->writeEntry( "Preview", previews );
    g_pConfig->setGroup( "Menubar" );
    g_pConfig->writeEntry("ShowMenubar", menuBarBox->isChecked());
    g_pConfig->setGroup( "Mouse Buttons" );
    g_pConfig->writeEntry("Left", s_choices[ leftComboBox->currentItem() ] );
    g_pConfig->writeEntry("Middle", s_choices[ middleComboBox->currentItem() ]);
    g_pConfig->writeEntry("Right", s_choices[ rightComboBox->currentItem() ]);
    g_pConfig->writeEntry("WheelSwitchesWorkspace", m_wheelSwitchesWorkspace);

    g_pConfig->setGroup( "General" );
    g_pConfig->writeEntry( "SetVRoot", vrootBox->isChecked() );
    g_pConfig->writeEntry( "Enabled", iconsEnabledBox->isChecked() );
    saveDevicesListView();
    g_pConfig->sync();

    // Tell kdesktop about the new config file
    if ( !kapp->dcopClient()->isAttached() )
       kapp->dcopClient()->attach();
    QByteArray data;

    int konq_screen_number = 0;
    if (qt_xdisplay())
       konq_screen_number = DefaultScreen(qt_xdisplay());

    QCString appname;
    if (konq_screen_number == 0)
        appname = "kdesktop";
    else
        appname.sprintf("kdesktop-screen-%d", konq_screen_number);
    kapp->dcopClient()->send( appname, "KDesktopIface", "configure()", data );
}

void KRootOptions::enableChanged()
{
    bool enabled = iconsEnabledBox->isChecked();
    showHiddenBox->setEnabled(enabled);
    previewListView->setEnabled(enabled);
    vrootBox->setEnabled(enabled);

    changed();
}

void KRootOptions::changed()
{
  emit KCModule::changed(true);
}

void KRootOptions::comboBoxChanged()
{
  // 4 - CustomMenu1
  // 5 - CustomMenu2
  int i;
  i = leftComboBox->currentItem();
  leftEditButton->setEnabled((i == 4) || (i == 5));
  i = middleComboBox->currentItem();
  middleEditButton->setEnabled((i == 4) || (i == 5));
  i = rightComboBox->currentItem();
  rightEditButton->setEnabled((i == 4) || (i == 5));
}

void KRootOptions::editButtonPressed()
{
   int i = 0;
   if (sender() == leftEditButton)
      i = leftComboBox->currentItem();
   if (sender() == middleEditButton)
      i = middleComboBox->currentItem();
   if (sender() == rightEditButton)
      i = rightComboBox->currentItem();
   
   QString cfgFile;
   if (i == 4) // CustomMenu1
      cfgFile = "kdesktop_custom_menu1";
   if (i == 5) // CustomMenu2
      cfgFile = "kdesktop_custom_menu2";
   
   if (cfgFile.isEmpty())
      return;
      
   KCustomMenuEditor editor(this);
   KConfig cfg(cfgFile);
   
   editor.load(&cfg);
   if (editor.exec())
   {
      editor.save(&cfg);
      cfg.sync();
   }
}

QString KRootOptions::quickHelp() const
{
  return i18n("<h1>Desktop</h1>\n"
    "This module allows you to choose various options\n"
    "for your desktop, including the way in which icons are arranged and\n"
    "the pop-up menus associated with clicks of the middle and right mouse\n"
    "buttons on the desktop.\n"
    "Use the \"Whats This?\" (Shift+F1) to get help on specific options.");
}

DesktopPathConfig::DesktopPathConfig(QWidget *parent, const char * )
    : KCModule( parent, "kcmkonq" )
{
  QLabel * tmpLabel;

#undef RO_LASTROW
#undef RO_LASTCOL
#define RO_LASTROW 5   // 4 paths + last row
#define RO_LASTCOL 2

  int row = 0;
  QGridLayout *lay = new QGridLayout(this, RO_LASTROW+1, RO_LASTCOL+1, 10);

  lay->setRowStretch(RO_LASTROW,10); // last line grows

  lay->setColStretch(0,0);
  lay->setColStretch(1,0);
  lay->setColStretch(2,10);

  // Desktop Paths
  row++;
  tmpLabel = new QLabel(i18n("Des&ktop path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  urDesktop = new KURLRequester(this);
  urDesktop->setMode( KFile::Directory );
  tmpLabel->setBuddy( urDesktop );
  lay->addMultiCellWidget(urDesktop, row, row, 1, RO_LASTCOL);
  connect(urDesktop, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  QWhatsThis::add( urDesktop, i18n("This directory contains all the files"
                                   " which you see on your desktop. You can change the location of this"
                                   " directory if you want to, and the contents will move automatically"
                                   " to the new location as well.") );

  row++;
  tmpLabel = new QLabel(i18n("&Trash path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  urTrash = new KURLRequester(this);
  urTrash->setMode( KFile::Directory );
  tmpLabel->setBuddy( urTrash );
  lay->addMultiCellWidget(urTrash, row, row, 1, RO_LASTCOL);
  connect(urTrash, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  QString wtstr = i18n("This directory contains deleted files (until"
               " you empty the trashcan). You can change the location of this"
               " directory if you want to, and the contents will move automatically"
               " to the new location as well.");
  QWhatsThis::add( tmpLabel, wtstr );
  QWhatsThis::add( urTrash, wtstr );

  row++;
  tmpLabel = new QLabel(i18n("A&utostart path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  urAutostart = new KURLRequester(this);
  urAutostart->setMode( KFile::Directory );
  tmpLabel->setBuddy( urAutostart );
  lay->addMultiCellWidget(urAutostart, row, row, 1, RO_LASTCOL);
  connect(urAutostart, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  wtstr = i18n("This directory contains applications or"
               " links to applications (shortcuts) that you want to have started"
               " automatically whenever KDE starts. You can change the location of this"
               " directory if you want to, and the contents will move automatically"
               " to the new location as well.");
  QWhatsThis::add( tmpLabel, wtstr );
  QWhatsThis::add( urAutostart, wtstr );

  row++;
  tmpLabel = new QLabel(i18n("D&ocuments path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  urDocument = new KURLRequester(this);
  urDocument->setMode( KFile::Directory );
  tmpLabel->setBuddy( urDocument );
  lay->addMultiCellWidget(urDocument, row, row, 1, RO_LASTCOL);
  connect(urDocument, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  wtstr = i18n("This directory will be used by default to "
               "load or save documents from or to.");
  QWhatsThis::add( tmpLabel, wtstr );
  QWhatsThis::add( urDocument, wtstr );

  // -- Bottom --
  Q_ASSERT( row == RO_LASTROW-1 ); // if it fails here, check the row++ and RO_LASTROW above

  load();
}

void DesktopPathConfig::load()
{
    // Desktop Paths
    urDesktop->setURL( KGlobalSettings::desktopPath() );
    urTrash->setURL( KGlobalSettings::trashPath() );
    urAutostart->setURL( KGlobalSettings::autostartPath() );
    urDocument->setURL( KGlobalSettings::documentPath() );
    changed();
}

void DesktopPathConfig::defaults()
{
    // Desktop Paths - keep defaults in sync with kglobalsettings.cpp
    urDesktop->setURL( QDir::homeDirPath() + "/Desktop/" );
    urTrash->setURL( QDir::homeDirPath() + "/Desktop/" + i18n("Trash") + '/' );
    urAutostart->setURL( KGlobal::dirs()->localkdedir() + "Autostart/" );
    urDocument->setURL( QDir::homeDirPath() );
}

void DesktopPathConfig::save()
{
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cgs( config, "Paths" );

    bool pathChanged = false;
    bool trashMoved = false;
    bool autostartMoved = false;

    KURL desktopURL;
    desktopURL.setPath( KGlobalSettings::desktopPath() );
    KURL newDesktopURL;
    newDesktopURL.setPath(urDesktop->url());

    KURL trashURL;
    trashURL.setPath( KGlobalSettings::trashPath() );
    KURL newTrashURL;
    newTrashURL.setPath(urTrash->url());

    KURL autostartURL;
    autostartURL.setPath( KGlobalSettings::autostartPath() );
    KURL newAutostartURL;
    newAutostartURL.setPath(urAutostart->url());

    KURL documentURL;
    documentURL.setPath( KGlobalSettings::documentPath() );
    KURL newDocumentURL;
    newDocumentURL.setPath(urDocument->url());

    if ( !newDesktopURL.cmp( desktopURL, true ) )
    {
        // Test which other paths were inside this one (as it is by default)
        // and for each, test where it should go.
        // * Inside destination -> let them be moved with the desktop (but adjust name if necessary)
        // * Not inside destination -> move first
        // !!!
        kdDebug() << "desktopURL=" << desktopURL.url() << endl;
        kdDebug() << "trashURL=" << trashURL.url() << endl;
        if ( desktopURL.isParentOf( trashURL ) )
        {
            // The trash is on the desktop (no, I don't do this at home....)
            kdDebug() << "The trash is currently on the desktop" << endl;
            // Either the Trash field wasn't changed (-> need to update it)
            if ( newTrashURL.cmp( trashURL, true ) )
            {
                // Hack. It could be in a subdir inside desktop. Hmmm... Argl.
                urTrash->setURL( urDesktop->url() + trashURL.fileName() );
                kdDebug() << "The trash is moved with the desktop" << endl;
                trashMoved = true;
            }
            // or it has been changed (->need to move it from here, unless moving the desktop does it)
            else
            {
                KURL futureTrashURL;
                futureTrashURL.setPath( urDesktop->url() + trashURL.fileName() );
                if ( newTrashURL.cmp( futureTrashURL, true ) )
                    trashMoved = true; // The trash moves with the desktop
                else
                    trashMoved = moveDir( KGlobalSettings::trashPath(), urTrash->url(), i18n("Trash") );
            }
        }

        if ( desktopURL.isParentOf( autostartURL ) )
        {
            kdDebug() << "Autostart is on the desktop" << endl;

            // Either the Autostart field wasn't changed (-> need to update it)
            if ( newAutostartURL.cmp( autostartURL, true ) )
            {
                // Hack. It could be in a subdir inside desktop. Hmmm... Argl.
                urAutostart->setURL( urDesktop->url() + "Autostart/" );
                kdDebug() << "Autostart is moved with the desktop" << endl;
                autostartMoved = true;
            }
            // or it has been changed (->need to move it from here)
            else
            {
                KURL futureAutostartURL;
                futureAutostartURL.setPath( urDesktop->url() + "Autostart/" );
                if ( newAutostartURL.cmp( futureAutostartURL, true ) )
                    autostartMoved = true;
                else
                    autostartMoved = moveDir( KGlobalSettings::autostartPath(), urAutostart->url(), i18n("Autostart") );
            }
        }

        if ( moveDir( KGlobalSettings::desktopPath(), urDesktop->url(), i18n("Desktop") ) )
        {
//            config->writeEntry( "Desktop", urDesktop->url());
            config->writePathEntry( "Desktop", urDesktop->url(), true, true );
            pathChanged = true;
        }
    }

    if ( !newTrashURL.cmp( trashURL, true ) )
    {
        if (!trashMoved)
            trashMoved = moveDir( KGlobalSettings::trashPath(), urTrash->url(), i18n("Trash") );
        if (trashMoved)
        {
//            config->writeEntry( "Trash", urTrash->url());
            config->writePathEntry( "Trash", urTrash->url(), true, true );
            pathChanged = true;
        }
    }

    if ( !newAutostartURL.cmp( autostartURL, true ) )
    {
        if (!autostartMoved)
            autostartMoved = moveDir( KGlobalSettings::autostartPath(), urAutostart->url(), i18n("Autostart") );
        if (autostartMoved)
        {
//            config->writeEntry( "Autostart", Autostart->url());
            config->writePathEntry( "Autostart", urAutostart->url(), true, true );
            pathChanged = true;
        }
    }

    if ( !newDocumentURL.cmp( documentURL, true ) )
    {
//        config->writeEntry( "Documents", urDocument->url());
        config->writePathEntry( "Documents", urDocument->url(), true, true );
        pathChanged = true;
    }

    config->sync();

    if (pathChanged)
    {
        kdDebug() << "DesktopPathConfig::save sending message SettingsChanged" << endl;
        KIPC::sendMessageAll(KIPC::SettingsChanged, KApplication::SETTINGS_PATHS);
    }

    // Tell kdesktop about the new config file
    if ( !kapp->dcopClient()->isAttached() )
       kapp->dcopClient()->attach();
    QByteArray data;

    int konq_screen_number = 0;
    if (qt_xdisplay())
       konq_screen_number = DefaultScreen(qt_xdisplay());

    QCString appname;
    if (konq_screen_number == 0)
        appname = "kdesktop";
    else
        appname.sprintf("kdesktop-screen-%d", konq_screen_number);
    kapp->dcopClient()->send( appname, "KDesktopIface", "configure()", data );
}

bool DesktopPathConfig::moveDir( const KURL & src, const KURL & dest, const QString & type )
{
    if (!src.isLocalFile() || !dest.isLocalFile())
        return true;
    m_ok = true;
    // Ask for confirmation before moving the files
    if ( KMessageBox::questionYesNo( this, i18n("The path for '%1' has been changed.\nDo you want me to move the files from '%2' to '%3'?").
                              arg(type).arg(src.path()).arg(dest.path()), i18n("Confirmation required") )
            == KMessageBox::Yes )
    {
        KIO::Job * job = KIO::move( src, dest );
        connect( job, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotResult( KIO::Job * ) ) );
        // wait for job
        qApp->enter_loop();
    }
    kdDebug() << "DesktopPathConfig::slotResult returning " << m_ok << endl;
    return m_ok;
}

void DesktopPathConfig::slotResult( KIO::Job * job )
{
    if (job->error())
    {
        if ( job->error() != KIO::ERR_DOES_NOT_EXIST )
            m_ok = false;
        // If the source doesn't exist, no wonder we couldn't move the dir.
        // In that case, trust the user and set the new setting in any case.

        job->showErrorDialog(this);
    }
    qApp->exit_loop();
}

void DesktopPathConfig::changed()
{
  emit KCModule::changed(true);
}


QString DesktopPathConfig::quickHelp() const
{
  return i18n("<h1>Desktop</h1>\n"
    "This module allows you to choose where files on your desktop\n"
    "are being stored.\n"
    "Use the \"Whats This?\" (Shift+F1) to get help on specific options.");
}


#include "rootopts.moc"
