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

#include <qdir.h>
#include <dcopclient.h>
#include <kapp.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kglobal.h>
#include <klocale.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kmessagebox.h>
#include <kstddirs.h>
#include <kipc.h>
#include <ktrader.h>
#include <kseparator.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qwhatsthis.h>
#include <qtl.h>
#include <qlistview.h>
#include <assert.h>

#include "rootopts.h"

#include <konq_defaults.h> // include default values directly from libkonq


extern int konq_screen_number;


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

static const char * s_choices[6] = { "", "WindowListMenu", "DesktopMenu", "AppMenu", "CustomMenu1", "CustomMenu2" };

KRootOptions::KRootOptions(KConfig *config, QWidget *parent, const char *name )
    : KCModule( parent, name ), g_pConfig(config)
{
  QLabel * tmpLabel;
#define RO_LASTROW 15   // 4 cb, 1 listview, 1 line, 3 combo, 1 line, 4 paths + last row
#define RO_LASTCOL 2
  int row = 0;
  QGridLayout *lay = new QGridLayout(this, RO_LASTROW+1, RO_LASTCOL+1, 10);
  QString strMouseButton1, strMouseButton3, strButtonTxt1, strButtonTxt3;
  bool leftHandedMouse;

  lay->setRowStretch(RO_LASTROW,10); // last line grows

  lay->setColStretch(0,0);
  lay->setColStretch(1,0);
  lay->setColStretch(2,10);

  /*
   * The text on this form depends on the mouse setting, which can be right
   * or left handed.  The outer button functionality is actually swapped
   *
   */
  leftHandedMouse = ( KGlobalSettings::mouseSettings().handed == KGlobalSettings::KMouseSettings::LeftHanded);

  VertAlignBox = new QCheckBox(i18n("Align Icons &Vertically on Desktop"), this);
  lay->addMultiCellWidget(VertAlignBox, row, row, 0, 0);
  connect(VertAlignBox, SIGNAL(clicked()), this, SLOT(changed()));
  QWhatsThis::add( VertAlignBox, i18n("Check this option if you want the icons"
                                      " on the desktop to be aligned vertically (in columns). If you leave this"
                                      " option unchecked, desktop icons are aligned horizontally.<p>"
                                      " Note that you can drag icons wherever you want to on the desktop. When"
                                      " you choose \"Arrange Icons\" from the Desktop menu, icons will be"
                                      " arranged horizontally or vertically.") );

  row++;
  showHiddenBox = new QCheckBox(i18n("Show &Hidden Files on Desktop"), this);
  lay->addMultiCellWidget(showHiddenBox, row, row, 0, 0);
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
  lay->addMultiCellWidget(menuBarBox, row, row, 0, 0);
  connect(menuBarBox, SIGNAL(clicked()), this, SLOT(changed()));
  QWhatsThis::add( menuBarBox, i18n("Check this option if you want the"
                                    " desktop popup menus to appear on the top of the screen in the style"
                                    " of Macintosh.  This setting is independent of the global top-level"
                                    " menu setting that applies to KDE applications.") );

  row++;
  vrootBox = new QCheckBox(i18n("Support Programs in Desktop Window"), this);
  lay->addMultiCellWidget(vrootBox, row, row, 0, 0);
  connect(vrootBox, SIGNAL(clicked()), this, SLOT(changed()));
  QWhatsThis::add( vrootBox, i18n("Check this option if you want to"
                                    " run X11 programs that draw into the desktop such as xsnow, xpenguin or"
                                    " xmountain. If you have problems with applications like netscape that check"
                                    " the root window for running instances, disable this option.") );

  row++;
  lay->setRowStretch( row, 10 );
  previewListView = new QListView( this );
  previewListView->addColumn( i18n("Show Previews for:") );
  lay->addMultiCellWidget( previewListView, row - 4, row, 1, RO_LASTCOL );
  QWhatsThis::add(previewListView, i18n("Select for which types of files you want to"
                                        " enable preview images"));

  row++;
  KSeparator * hLine2 = new KSeparator(KSeparator::HLine, this);
  lay->addMultiCellWidget(hLine2, row, row, 0, RO_LASTCOL);

  row++;
  tmpLabel = new QLabel( i18n("Clicks on the desktop"), this );
  lay->addMultiCellWidget( tmpLabel, row, row, 0, RO_LASTCOL );

  strMouseButton1 = i18n("Left Button");
  strButtonTxt1 = i18n( "You can choose what happens when"
   " you click the left button of your pointing device on the desktop:");

  strMouseButton3 = i18n("Right Button");
  strButtonTxt3 = i18n( "You can choose what happens when"
   " you click the right button of your pointing device on the desktop:");

  if ( leftHandedMouse )
  {
     qSwap(strMouseButton1, strMouseButton3);
     qSwap(strButtonTxt1, strButtonTxt3);
  }

  row++;
  tmpLabel = new QLabel( strMouseButton1, this );
  lay->addWidget( tmpLabel, row, 0 );
  leftComboBox = new QComboBox( this );
  tmpLabel->setBuddy( leftComboBox );
  lay->addWidget( leftComboBox, row, 1 );
  fillMenuCombo( leftComboBox );
  connect(leftComboBox, SIGNAL(activated(int)), this, SLOT(changed()));
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
  QWhatsThis::add( tmpLabel, wtstr );
  QWhatsThis::add( leftComboBox, wtstr );

  row++;
  tmpLabel = new QLabel( i18n("Middle button"), this );
  lay->addWidget( tmpLabel, row, 0 );
  middleComboBox = new QComboBox( this );
  tmpLabel->setBuddy( middleComboBox );
  lay->addWidget( middleComboBox, row, 1 );
  fillMenuCombo( middleComboBox );
  connect(middleComboBox, SIGNAL(activated(int)), this, SLOT(changed()));
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
  QWhatsThis::add( tmpLabel, wtstr );
  QWhatsThis::add( middleComboBox, wtstr );

  row++;
  tmpLabel = new QLabel( strMouseButton3, this );
  lay->addWidget( tmpLabel, row, 0 );
  rightComboBox = new QComboBox( this );
  tmpLabel->setBuddy( rightComboBox );
  lay->addWidget( rightComboBox, row, 1 );
  fillMenuCombo( rightComboBox );
  connect(rightComboBox, SIGNAL(activated(int)), this, SLOT(changed()));
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
  QWhatsThis::add( tmpLabel, wtstr );
  QWhatsThis::add( rightComboBox, wtstr );

  // Desktop Paths

  row++;
  KSeparator* hLine = new KSeparator(KSeparator::HLine, this);
  lay->addMultiCellWidget(hLine, row, row, 0, RO_LASTCOL);

  row++;
  tmpLabel = new QLabel(i18n("Desktop &path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  leDesktop = new QLineEdit(this);
  tmpLabel->setBuddy( leDesktop );
  lay->addMultiCellWidget(leDesktop, row, row, 1, RO_LASTCOL);
  connect(leDesktop, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  QWhatsThis::add( leDesktop, i18n("This directory contains all the files"
                                   " which you see on your desktop. You can change the location of this"
                                   " directory if you want to, and the contents will move automatically"
                                   " to the new location as well.") );

  row++;
  tmpLabel = new QLabel(i18n("&Trash path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  leTrash = new QLineEdit(this);
  tmpLabel->setBuddy( leTrash );
  lay->addMultiCellWidget(leTrash, row, row, 1, RO_LASTCOL);
  connect(leTrash, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  wtstr = i18n("This directory contains deleted files (until"
               " you empty the trashcan). You can change the location of this"
               " directory if you want to, and the contents will move automatically"
               " to the new location as well.");
  QWhatsThis::add( tmpLabel, wtstr );
  QWhatsThis::add( leTrash, wtstr );

  row++;
  tmpLabel = new QLabel(i18n("&Autostart path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  leAutostart = new QLineEdit(this);
  tmpLabel->setBuddy( leAutostart );
  lay->addMultiCellWidget(leAutostart, row, row, 1, RO_LASTCOL);
  connect(leAutostart, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  wtstr = i18n("This directory contains applications or"
               " links to applications (shortcuts) that you want to have started"
               " automatically whenever KDE starts. You can change the location of this"
               " directory if you want to, and the contents will move automatically"
               " to the new location as well.");
  QWhatsThis::add( tmpLabel, wtstr );
  QWhatsThis::add( leAutostart, wtstr );

  row++;
  tmpLabel = new QLabel(i18n("&Documents path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  leDocument = new QLineEdit(this);
  tmpLabel->setBuddy( leDocument );
  lay->addMultiCellWidget(leDocument, row, row, 1, RO_LASTCOL);
  connect(leDocument, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  wtstr = i18n("This directory will be used by default to "
               "load or save documents from or to.");
  QWhatsThis::add( tmpLabel, wtstr );
  QWhatsThis::add( leDocument, wtstr );

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
  combo->insertItem( i18n("Custom Menu 1") );
  combo->insertItem( i18n("Custom Menu 2") );
}

void KRootOptions::load()
{
    g_pConfig->setGroup( "Desktop Icons" );
    bool bShowHidden = g_pConfig->readBoolEntry("ShowHidden", DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    showHiddenBox->setChecked(bShowHidden);
    bool bVertAlign = g_pConfig->readNumEntry("VertAlign", DEFAULT_VERT_ALIGN);
    VertAlignBox->setChecked(bVertAlign);
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

    // Desktop Paths
    leDesktop->setText( KGlobalSettings::desktopPath() );
    leTrash->setText( KGlobalSettings::trashPath() );
    leAutostart->setText( KGlobalSettings::autostartPath() );
    leDocument->setText( KGlobalSettings::documentPath() );
}

void KRootOptions::defaults()
{
    showHiddenBox->setChecked(DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    VertAlignBox->setChecked(true);
    for (QListViewItem *item = previewListView->firstChild(); item; item = item->nextSibling())
        static_cast<KRootOptPreviewItem *>(item)->setOn(false);
    menuBarBox->setChecked(false);
    vrootBox->setChecked( false );
    leftComboBox->setCurrentItem( NOTHING );
    middleComboBox->setCurrentItem( WINDOWLISTMENU );
    rightComboBox->setCurrentItem( DESKTOPMENU );

    // Desktop Paths - keep defaults in sync with kglobalsettings.cpp
    leDesktop->setText( QDir::homeDirPath() + "/Desktop/" );
    leTrash->setText( QDir::homeDirPath() + "/Desktop/" + i18n("Trash") + '/' );
    leAutostart->setText( KGlobal::dirs()->localkdedir() + "Autostart/" );
    leDocument->setText( QDir::homeDirPath() );
}

void KRootOptions::save()
{
    g_pConfig->setGroup( "Desktop Icons" );
    g_pConfig->writeEntry("ShowHidden", showHiddenBox->isChecked());
    g_pConfig->writeEntry("VertAlign",VertAlignBox->isChecked());
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

    g_pConfig->setGroup( "General" );
    g_pConfig->writeEntry( "SetVRoot", vrootBox->isChecked() );
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cgs( config, "Paths" );

    bool pathChanged = false;
    bool trashMoved = false;
    bool autostartMoved = false;

    KURL desktopURL;
    desktopURL.setPath( KGlobalSettings::desktopPath() );
    KURL newDesktopURL;
    newDesktopURL.setPath(leDesktop->text());

    KURL trashURL;
    trashURL.setPath( KGlobalSettings::trashPath() );
    KURL newTrashURL;
    newTrashURL.setPath(leTrash->text());

    KURL autostartURL;
    autostartURL.setPath( KGlobalSettings::autostartPath() );
    KURL newAutostartURL;
    newAutostartURL.setPath(leAutostart->text());

    KURL documentURL;
    documentURL.setPath( KGlobalSettings::documentPath() );
    KURL newDocumentURL;
    newDocumentURL.setPath(leDocument->text());

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
                leTrash->setText( leDesktop->text() + trashURL.fileName() );
                kdDebug() << "The trash is moved with the desktop" << endl;
                trashMoved = true;
            }
            // or it has been changed (->need to move it from here, unless moving the desktop does it)
            else
            {
                KURL futureTrashURL;
                futureTrashURL.setPath( leDesktop->text() + trashURL.fileName() );
                if ( newTrashURL.cmp( futureTrashURL, true ) )
                    trashMoved = true; // The trash moves with the desktop
                else
                    trashMoved = moveDir( KGlobalSettings::trashPath(), leTrash->text(), i18n("Trash") );
            }
        }

        if ( desktopURL.isParentOf( autostartURL ) )
        {
            kdDebug() << "Autostart is on the desktop" << endl;

            // Either the Autostart field wasn't changed (-> need to update it)
            if ( newAutostartURL.cmp( autostartURL, true ) )
            {
                // Hack. It could be in a subdir inside desktop. Hmmm... Argl.
                leAutostart->setText( leDesktop->text() + "Autostart/" );
                kdDebug() << "Autostart is moved with the desktop" << endl;
                autostartMoved = true;
            }
            // or it has been changed (->need to move it from here)
            else
            {
                KURL futureAutostartURL;
                futureAutostartURL.setPath( leDesktop->text() + "Autostart/" );
                if ( newAutostartURL.cmp( futureAutostartURL, true ) )
                    autostartMoved = true;
                else
                    autostartMoved = moveDir( KGlobalSettings::autostartPath(), leAutostart->text(), i18n("Autostart") );
            }
        }

        if ( moveDir( KGlobalSettings::desktopPath(), leDesktop->text(), i18n("Desktop") ) )
        {
            config->writeEntry( "Desktop", leDesktop->text(), true, true );
            pathChanged = true;
        }
    }

    if ( !newTrashURL.cmp( trashURL, true ) )
    {
        if (!trashMoved)
            trashMoved = moveDir( KGlobalSettings::trashPath(), leTrash->text(), i18n("Trash") );
        if (trashMoved)
        {
            config->writeEntry( "Trash", leTrash->text(), true, true );
            pathChanged = true;
        }
    }

    if ( !newAutostartURL.cmp( autostartURL, true ) )
    {
        if (!autostartMoved)
            autostartMoved = moveDir( KGlobalSettings::autostartPath(), leAutostart->text(), i18n("Autostart") );
        if (autostartMoved)
        {
            config->writeEntry( "Autostart", leAutostart->text(), true, true );
            pathChanged = true;
        }
    }

    if ( !newDocumentURL.cmp( documentURL, true ) )
    {
        config->writeEntry( "Documents", leDocument->text(), true, true );
        pathChanged = true;
    }

    config->sync();
    g_pConfig->sync();

    if (pathChanged)
    {
        kdDebug() << "KRootOptions::save sending message SettingsChanged" << endl;
        KIPC::sendMessageAll(KIPC::SettingsChanged, KApplication::SETTINGS_PATHS);
    }
}

bool KRootOptions::moveDir( const KURL & src, const KURL & dest, const QString & type )
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
    kdDebug() << "KRootOptions::slotResult returning " << m_ok << endl;
    return m_ok;
}

void KRootOptions::slotResult( KIO::Job * job )
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


void KRootOptions::changed()
{
  emit KCModule::changed(true);
}


QString KRootOptions::quickHelp() const
{
  return i18n("<h1>Desktop</h1>\n"
    "This module allows you to choose various options\n"
    "for your desktop, including the way in which icons are arranged, the\n"
    "location of your desktop directory, and the pop-up menus associated\n"
    "with clicks of the middle and right mouse buttons on the desktop.\n"
    "Use the \"Whats This?\" (Shift+F1) to get help on specific options.");
}


#include "rootopts.moc"
