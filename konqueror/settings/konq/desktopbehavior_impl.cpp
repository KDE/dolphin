/**
 * (c) Martin R. Jones 1996
 * (c) Bernd Wuebben 1998
 * (c) Christian Tibirna 1998
 * (c) David Faure 1998, 2000
 * (c) John Firebaugh 2003
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "desktopbehavior_impl.h"

#include <QLayout>
#include <QCheckBox>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <q3buttongroup.h>
#include <QTabWidget>

//Added by qt3to4:
#include <QVBoxLayout>
#include <Q3CString>
#include <QDesktopWidget>

#include <k3listview.h>
#include <kservice.h>
#include <klocale.h>
#include <kglobalsettings.h>
#include <kmimetype.h>
#include <kservicetypetrader.h>
#include <kapplication.h>
#include <kcustommenueditor.h>
#include <konq_defaults.h> // include default values directly from libkonq
#include <kipc.h>
#include <kprotocolinfo.h>

DesktopBehaviorModule::DesktopBehaviorModule(KConfig *config, KInstance *inst, QWidget *parent )
    : KCModule( inst, parent )
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    m_behavior = new DesktopBehavior(config, this);
    layout->addWidget(m_behavior);
    connect(m_behavior, SIGNAL(changed()), this, SLOT(changed()));
}

void DesktopBehaviorModule::changed()
{
    emit KCModule::changed( true );
}

class DesktopBehaviorPreviewItem : public Q3CheckListItem
{
public:
    DesktopBehaviorPreviewItem(DesktopBehavior *rootOpts, Q3ListView *parent,
                const KService::Ptr &plugin, bool on)
        : Q3CheckListItem(parent, plugin->name(), CheckBox),
          m_rootOpts(rootOpts)
    {
        m_pluginName = plugin->desktopEntryName();
        setOn(on);
    }
    DesktopBehaviorPreviewItem(DesktopBehavior *rootOpts, Q3ListView *parent,
                bool on)
        : Q3CheckListItem(parent, i18n("Sound Files"), CheckBox),
          m_rootOpts(rootOpts)
    {
        m_pluginName = "audio/";
        setOn(on);
    }
    const QString &pluginName() const { return m_pluginName; }

protected:
    virtual void stateChange( bool ) { m_rootOpts->changed(); }

private:
    DesktopBehavior *m_rootOpts;
    QString m_pluginName;
};


class DesktopBehaviorMediaItem : public Q3CheckListItem
{
public:
    DesktopBehaviorMediaItem(DesktopBehavior *rootOpts, Q3ListView *parent,
                const QString name, const QString mimetype, bool on)
        : Q3CheckListItem(parent, name, CheckBox),
          m_rootOpts(rootOpts),m_mimeType(mimetype){setOn(on);}

    const QString &mimeType() const { return m_mimeType; }

protected:
    virtual void stateChange( bool ) { m_rootOpts->changed(); }

private:
    DesktopBehavior *m_rootOpts;
    QString m_mimeType;
};


static const int choiceCount=7;
static const char * s_choices[7] = { "", "WindowListMenu", "DesktopMenu", "AppMenu", "BookmarksMenu", "CustomMenu1", "CustomMenu2" };

DesktopBehavior::DesktopBehavior(KConfig *config, QWidget *parent, const char * )
    : DesktopBehaviorBase( parent, "kcmkonq" ), g_pConfig(config)
{
  QString strMouseButton1, strMouseButton3, strButtonTxt1, strButtonTxt3;

  /*
   * The text on this form depends on the mouse setting, which can be right
   * or left handed.  The outer button functionality is actually swapped
   *
   */
  bool leftHandedMouse = ( KGlobalSettings::mouseSettings().handed == KGlobalSettings::KMouseSettings::LeftHanded);

  m_bHasMedia = KProtocolInfo::isKnownProtocol(QLatin1String("media"));

  connect(desktopMenuGroup, SIGNAL(clicked(int)), this, SIGNAL(changed()));
  connect(iconsEnabledBox, SIGNAL(clicked()), this, SLOT(enableChanged()));
  connect(showHiddenBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(vrootBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(autoLineupIconsBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(toolTipBox, SIGNAL(clicked()), this, SIGNAL(changed()));

  strMouseButton1 = i18n("&Left button:");
  strButtonTxt1 = i18n( "You can choose what happens when"
   " you click the left button of your pointing device on the desktop:");

  strMouseButton3 = i18n("Right b&utton:");
  strButtonTxt3 = i18n( "You can choose what happens when"
   " you click the right button of your pointing device on the desktop:");

  if ( leftHandedMouse )
  {
     qSwap(strMouseButton1, strMouseButton3);
     qSwap(strButtonTxt1, strButtonTxt3);
  }

  leftLabel->setText( strMouseButton1 );
  leftLabel->setBuddy( leftComboBox );
  fillMenuCombo( leftComboBox );
  connect(leftEditButton, SIGNAL(clicked()), this, SLOT(editButtonPressed()));
  connect(leftComboBox, SIGNAL(activated(int)), this, SIGNAL(changed()));
  connect(leftComboBox, SIGNAL(activated(int)), this, SLOT(comboBoxChanged()));
  QString wtstr = strButtonTxt1 +
                  i18n(" <ul><li><em>No action</em></li>"
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
  leftLabel->setWhatsThis( wtstr );
  leftComboBox->setWhatsThis( wtstr );

  middleLabel->setBuddy( middleComboBox );
  fillMenuCombo( middleComboBox );
  connect(middleEditButton, SIGNAL(clicked()), this, SLOT(editButtonPressed()));
  connect(middleComboBox, SIGNAL(activated(int)), this, SIGNAL(changed()));
  connect(middleComboBox, SIGNAL(activated(int)), this, SLOT(comboBoxChanged()));
  wtstr = i18n("You can choose what happens when"
               " you click the middle button of your pointing device on the desktop:"
               " <ul><li><em>No action</em></li>"
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
  middleLabel->setWhatsThis( wtstr );
  middleComboBox->setWhatsThis( wtstr );

  rightLabel->setText( strMouseButton3 );
  rightLabel->setBuddy( rightComboBox );
  fillMenuCombo( rightComboBox );
  connect(rightEditButton, SIGNAL(clicked()), this, SLOT(editButtonPressed()));
  connect(rightComboBox, SIGNAL(activated(int)), this, SIGNAL(changed()));
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
  rightLabel->setWhatsThis( wtstr );
  rightComboBox->setWhatsThis( wtstr );

  if (m_bHasMedia)
  {
     connect(enableMediaBox, SIGNAL(clicked()), this, SLOT(enableChanged()));
  }
  else
  {
     delete behaviorTab->page(2);
  }

  load();
}

void DesktopBehavior::fillMediaListView()
{
    mediaListView->clear();
    mediaListView->setRootIsDecorated(false);
    KMimeType::List mimetypes = KMimeType::allMimeTypes();
    KMimeType::List::const_iterator it2(mimetypes.begin());
    g_pConfig->setGroup( "Media" );
    enableMediaBox->setChecked(g_pConfig->readEntry("enabled", false));
    QString excludedMedia=g_pConfig->readEntry("exclude","media/hdd_mounted,media/hdd_unmounted,media/floppy_unmounted,media/cdrom_unmounted,media/floppy5_unmounted");
    for (; it2 != mimetypes.end(); ++it2) {
       if ( ((*it2)->name().startsWith("media/")) )
	{
    	    bool ok=excludedMedia.contains((*it2)->name())==0;
		new DesktopBehaviorMediaItem (this, mediaListView, (*it2)->comment(), (*it2)->name(),ok);

        }
    }
}

void DesktopBehavior::saveMediaListView()
{
    if (!m_bHasMedia)
        return;

    g_pConfig->setGroup( "Media" );
    g_pConfig->writeEntry("enabled",enableMediaBox->isChecked());
    QStringList exclude;
    for (DesktopBehaviorMediaItem *it=static_cast<DesktopBehaviorMediaItem *>(mediaListView->firstChild());
     	it; it=static_cast<DesktopBehaviorMediaItem *>(it->nextSibling()))
    	{
		if (!it->isOn()) exclude << it->mimeType();
	    }
     g_pConfig->writeEntry("exclude",exclude);
}


void DesktopBehavior::fillMenuCombo( QComboBox * combo )
{
  combo->addItem( i18n("No Action") );
  combo->addItem( i18n("Window List Menu") );
  combo->addItem( i18n("Desktop Menu") );
  combo->addItem( i18n("Application Menu") );
  combo->addItem( i18n("Bookmarks Menu") );
  combo->addItem( i18n("Custom Menu 1") );
  combo->addItem( i18n("Custom Menu 2") );
}

void DesktopBehavior::load()
{
    g_pConfig->setGroup( "Desktop Icons" );
    bool bShowHidden = g_pConfig->readEntry("ShowHidden", DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    showHiddenBox->setChecked(bShowHidden);
    //bool bVertAlign = g_pConfig->readEntry("VertAlign", DEFAULT_VERT_ALIGN);
    KService::List plugins = KServiceTypeTrader::self()->query("ThumbCreator");
    previewListView->clear();
    QStringList previews = g_pConfig->readEntry("Preview", QStringList() );
    for (KService::List::ConstIterator it = plugins.begin(); it != plugins.end(); ++it)
        new DesktopBehaviorPreviewItem(this, previewListView, *it, previews.contains((*it)->desktopEntryName()));
    new DesktopBehaviorPreviewItem(this, previewListView, previews.contains("audio/"));
    //
    g_pConfig->setGroup( "FMSettings" );
    toolTipBox->setChecked(g_pConfig->readEntry( "ShowFileTips", true) );
    g_pConfig->setGroup( "Menubar" );
    KConfig config( "kdeglobals" );
    config.setGroup("KDE");
    bool globalMenuBar = config.readEntry("macStyle", false);
    bool desktopMenuBar = g_pConfig->readEntry("ShowMenubar", false);
    if ( globalMenuBar )
        desktopMenuGroup->setButton( 2 );
    else if ( desktopMenuBar )
        desktopMenuGroup->setButton( 1 );
    else
        desktopMenuGroup->setButton( 0 );
    g_pConfig->setGroup( "General" );
    vrootBox->setChecked( g_pConfig->readEntry( "SetVRoot", false ) );
    iconsEnabledBox->setChecked( g_pConfig->readEntry( "Enabled", true ) );
    autoLineupIconsBox->setChecked( g_pConfig->readEntry( "AutoLineUpIcons", false ) );

    //
    g_pConfig->setGroup( "Mouse Buttons" );
    QString s;
    s = g_pConfig->readEntry( "Left", "" );
    for ( int c = 0 ; c < choiceCount ; c ++ )
    if (s == s_choices[c])
      { leftComboBox->setCurrentIndex( c ); break; }
    s = g_pConfig->readEntry( "Middle", "WindowListMenu" );
    for ( int c = 0 ; c < choiceCount ; c ++ )
      if (s == s_choices[c])
      { middleComboBox->setCurrentIndex( c ); break; }
    s = g_pConfig->readEntry( "Right", "DesktopMenu" );
    for ( int c = 0 ; c < choiceCount ; c ++ )
      if (s == s_choices[c])
      { rightComboBox->setCurrentIndex( c ); break; }

    comboBoxChanged();
    if (m_bHasMedia)
        fillMediaListView();
    enableChanged();
}

void DesktopBehavior::defaults()
{
    showHiddenBox->setChecked(DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    for (Q3ListViewItem *item = previewListView->firstChild(); item; item = item->nextSibling())
        static_cast<DesktopBehaviorPreviewItem *>(item)->setOn(false);
    desktopMenuGroup->setButton( 0 );
    vrootBox->setChecked( false );
    autoLineupIconsBox->setChecked( true );
    leftComboBox->setCurrentIndex( NOTHING );
    middleComboBox->setCurrentIndex( WINDOWLISTMENU );
    rightComboBox->setCurrentIndex( DESKTOPMENU );
    iconsEnabledBox->setChecked(true);
    autoLineupIconsBox->setChecked(false);
    toolTipBox->setChecked(true);
    if (m_bHasMedia)
        fillMediaListView();

    comboBoxChanged();
    enableChanged();
}


void DesktopBehavior::save()
{
    g_pConfig->setGroup( "Desktop Icons" );
    g_pConfig->writeEntry("ShowHidden", showHiddenBox->isChecked());
    QStringList previews;
    for ( DesktopBehaviorPreviewItem *item = static_cast<DesktopBehaviorPreviewItem *>( previewListView->firstChild() );
          item;
          item = static_cast<DesktopBehaviorPreviewItem *>( item->nextSibling() ) )
        if ( item->isOn() )
            previews.append( item->pluginName() );
    g_pConfig->writeEntry( "Preview", previews );
    g_pConfig->setGroup( "FMSettings" );
    g_pConfig->writeEntry( "ShowFileTips", toolTipBox->isChecked() );
    g_pConfig->setGroup( "Menubar" );
    g_pConfig->writeEntry("ShowMenubar", desktopMenuGroup->selectedId() == 1);
    KConfig config( "kdeglobals" );
    config.setGroup("KDE");
    bool globalMenuBar = desktopMenuGroup->selectedId() == 2;
    if ( globalMenuBar != config.readEntry("macStyle", false) )
    {
        config.writeEntry( "macStyle", globalMenuBar, KConfigBase::Normal | KConfigBase::Global );
        config.sync();
        KIPC::sendMessageAll(KIPC::ToolbarStyleChanged);
    }
    g_pConfig->setGroup( "Mouse Buttons" );
    g_pConfig->writeEntry("Left", s_choices[ leftComboBox->currentIndex() ] );
    g_pConfig->writeEntry("Middle", s_choices[ middleComboBox->currentIndex() ]);
    g_pConfig->writeEntry("Right", s_choices[ rightComboBox->currentIndex() ]);

    g_pConfig->setGroup( "General" );
    g_pConfig->writeEntry( "SetVRoot", vrootBox->isChecked() );
    g_pConfig->writeEntry( "Enabled", iconsEnabledBox->isChecked() );
    g_pConfig->writeEntry( "AutoLineUpIcons", autoLineupIconsBox->isChecked() );

    saveMediaListView();
    g_pConfig->sync();

    // Tell kdesktop about the new config file
    int konq_screen_number = KApplication::desktop()->primaryScreen();
    Q3CString appname;
    if (konq_screen_number == 0)
        appname = "kdesktop";
    else
        appname.sprintf("kdesktop-screen-%d", konq_screen_number);
#ifdef __GNUC__
#warning Emit DBus signal, and commit kicker/kwin/kdesktop/plasma/whatever to it
#endif
#if 0
    kapp->dcopClient()->send( appname, "KDesktopIface", "configure()", data );
    // for the standalone menubar setting
    kapp->dcopClient()->send( "menuapplet*", "menuapplet", "configure()", data );
    kapp->dcopClient()->send( "kicker", "kicker", "configureMenubar()", data );
    kapp->dcopClient()->send( "kwin*", "", "reconfigure()", data );
#endif
}

void DesktopBehavior::enableChanged()
{
    bool enabled = iconsEnabledBox->isChecked();
    behaviorTab->setTabEnabled(1, enabled);
    vrootBox->setEnabled(enabled);

    if (m_bHasMedia)
    {
        behaviorTab->setTabEnabled(2, enabled);
        enableMediaBox->setEnabled(enabled);
        mediaListView->setEnabled(enableMediaBox->isChecked());
    }

    changed();
}

void DesktopBehavior::comboBoxChanged()
{
  // 4 - CustomMenu1
  // 5 - CustomMenu2
  int i;
  i = leftComboBox->currentIndex();
  leftEditButton->setEnabled((i == 4) || (i == 5));
  i = middleComboBox->currentIndex();
  middleEditButton->setEnabled((i == 4) || (i == 5));
  i = rightComboBox->currentIndex();
  rightEditButton->setEnabled((i == 4) || (i == 5));
}

void DesktopBehavior::editButtonPressed()
{
   int i = 0;
   if (sender() == leftEditButton)
      i = leftComboBox->currentIndex();
   if (sender() == middleEditButton)
      i = middleComboBox->currentIndex();
   if (sender() == rightEditButton)
      i = rightComboBox->currentIndex();

   QString cfgFile;
   if (i == 4) // CustomMenu1
      cfgFile = "kdesktop_custom_menu1";
   if (i == 5) // CustomMenu2
      cfgFile = "kdesktop_custom_menu2";

   if (cfgFile.isEmpty())
      return;

   KCustomMenuEditor editor(this);
   KConfig cfg(cfgFile, false, false);

   editor.load(&cfg);
   if (editor.exec())
   {
      editor.save(&cfg);
      cfg.sync();
      emit changed();
   }
}

QString DesktopBehavior::quickHelp() const
{
  return i18n("<h1>Behavior</h1>\n"
    "This module allows you to choose various options\n"
    "for your desktop, including the way in which icons are arranged and\n"
    "the pop-up menus associated with clicks of the middle and right mouse\n"
    "buttons on the desktop.\n"
    "Use the \"Whats This?\" (Shift+F1) to get help on specific options.");
}

#include "desktopbehavior_impl.moc"
