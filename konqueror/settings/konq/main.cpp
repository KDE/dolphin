/*
 * main.cpp
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <unistd.h>

#include <qlayout.h>

#include <kapp.h>
#include <dcopclient.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kdialog.h>

#include "rootopts.h"
#include "behaviour.h"
#include "fontopts.h"
#include "trashopts.h"
#include "desktop.h"

#include "main.h"
#include "main.moc"

#include <X11/Xlib.h>
#undef CallSucceeded
#undef Status

// for multihead
int konq_screen_number = 0;


KonqyModule::KonqyModule(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
  if (qt_xdisplay())
      konq_screen_number = DefaultScreen(qt_xdisplay());

  KConfig *config = new KConfig("konquerorrc", false, true);

  QVBoxLayout *layout = new QVBoxLayout(this);
  tab = new QTabWidget(this);
  layout->addWidget(tab);

  QString groupName = "FMSettings";
  behaviour = new KBehaviourOptions(config, groupName, this);
  tab->addTab(behaviour, i18n("&Behavior"));
  connect(behaviour, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  font = new KonqFontOptions(config, groupName, false, this);
  tab->addTab(font, i18n("&Appearance"));
  connect(font, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  trash = new KTrashOptions(config, "Trash", this);
  tab->addTab(trash, i18n("&Trash"));
  connect(trash, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));
}


void KonqyModule::load()
{
  behaviour->load();
  font->load();
  trash->load();
}


void KonqyModule::save()
{
  behaviour->save();
  font->save();
  trash->save();

  // Send signal to konqueror
  // Warning. In case something is added/changed here, keep kfmclient in sync
  QByteArray data;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );
}


void KonqyModule::defaults()
{
  behaviour->defaults();
  font->defaults();
  trash->defaults();
}

QString KonqyModule::quickHelp() const
{
  return i18n("<h1>File Manager</h1> In this module, you can configure various"
    " aspects of Konqueror's file manager functionality. Please note that the"
    " Konqueror's web browser functionality has its own configuration module.<p>"
    " The configuration options for the file manager are organized under"
    " four tabs as described below:"
    " <h2>Behavior</h2>"
    " This tab contains global options for Konqueror, such as the directory"
    " tree's reaction to changes in adjoining views, whether directories are"
    " opened in new windows, etc."
    " <h2>Appearance</h2>"
    " This tab contains options customizing the appearance of Konqueror windows,"
    " such as the font and color of text, background color, etc."
    " <h2>Trash</h2>"
    " This tab contains options for customizing the behavior of"
    " Konqueror when you \"delete\" a file.");
}


void KonqyModule::moduleChanged(bool state)
{
  emit changed(state);
}


KDesktopModule::KDesktopModule(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
    QCString configname;
    if (konq_screen_number == 0)
	configname = "kdesktoprc";
    else
	configname.sprintf("kdesktop-screen-%drc", konq_screen_number);

  KConfig *config = new KConfig(configname, false, false);

  QVBoxLayout *layout = new QVBoxLayout(this);
  tab = new QTabWidget(this);
  layout->addWidget(tab);

  root = new KRootOptions(config, this);
  tab->addTab(root, i18n("&Desktop"));
  connect(root, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  font = new KonqFontOptions(config, "FMSettings", true, this);
  tab->addTab(font, i18n("&Appearance"));
  connect(font, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  virtualDesks = new KDesktopConfig(this, "VirtualDesktops");
  tab->addTab(virtualDesks, i18n("&Number of Desktops"));
  connect(virtualDesks, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  //should we add Trash also here ?
}


void KDesktopModule::load()
{
  root->load();
  font->load();
  virtualDesks->load();
}


void KDesktopModule::save()
{
  root->save();
  font->save();
  virtualDesks->save();

  // Tell kdesktop about the new config file
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  QByteArray data;

  QCString appname;
  if (konq_screen_number == 0)
      appname = "kdesktop";
  else
      appname.sprintf("kdesktop-screen-%d", konq_screen_number);
  kapp->dcopClient()->send( appname, "KDesktopIface", "configure()", data );
}


void KDesktopModule::defaults()
{
  root->defaults();
  font->defaults();
  virtualDesks->defaults();
}


void KDesktopModule::moduleChanged(bool state)
{
  emit changed(state);
}


void KDesktopModule::resizeEvent(QResizeEvent *)
{
  tab->setGeometry(0,0,width(),height());
}

QString KDesktopModule::quickHelp() const
{
  return i18n("<h1>Desktop</h1>\n"
    "This module allows you to choose various options\n"
    "for your desktop, including the way in which icons are arranged, the\n"
    "location of your desktop directory, and the pop-up menus associated\n"
    "with clicks of the middle and right mouse buttons on the desktop.\n"
    "Use the \"Whats This?\" (Shift+F1) to get help on specific options.");               }

extern "C"
{

  KCModule *create_konqueror(QWidget *parent, const char *name)
  {
    KGlobal::locale()->insertCatalogue("kcmkonq");
    return new KonqyModule(parent, name);
  }

  KCModule *create_desktop(QWidget *parent, const char *name)
  {
    KGlobal::locale()->insertCatalogue("kcmkonq");
    return new KDesktopModule(parent, name);
  }

}


