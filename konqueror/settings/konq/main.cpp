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


#include "rootopts.h"
#include "behaviour.h"
#include "fontopts.h"
#include "desktop.h"
#include "previews.h"

#include <kconfig.h>
#include <X11/Xlib.h>

/*
// for multihead
int konq_screen_number = 0;


KonqyModule::KonqyModule(QWidget *parent, const char *)
  : KCModule(parent, "kcmkonq")
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
  tab->addTab(font, i18n("A&ppearance"));
  connect(font, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  trash = new KTrashOptions(config, "Trash", this);
  tab->addTab(trash, i18n("&Trash"));
  connect(trash, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  previews = new KPreviewOptions( this );
  tab->addTab(previews, i18n("Pre&views"));
  connect(previews, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));
}


void KonqyModule::load()
{
  behaviour->load();
  font->load();
  trash->load();
  previews->load();
}


void KonqyModule::save()
{
  behaviour->save();
  font->save();
  trash->save();
  previews->save();

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
  previews->defaults();
}

QString KonqyModule::quickHelp() const
{
  return i18n("<h1>File Manager</h1> In this module, you can configure various"
    " aspects of Konqueror's file manager functionality. Please note that the"
    " Konqueror's web browser functionality has its own configuration module.<p>"
    " The configuration options for the file manager are organized under"
    " three tabs as described below:"
    " <h2>Behavior</h2>"
    " This tab contains global options for Konqueror, such as the directory"
    " tree's reaction to changes in adjoining views, whether directories are"
    " opened in new windows, etc."
    " <h2>Appearance</h2>"
    " This tab contains options customizing the appearance of Konqueror windows,"
    " such as the font and color of text, background color, etc."
    " <h2>Trash</h2>"
    " This tab contains options for customizing the behavior of"
    " Konqueror when you \"delete\" a file."
    " <h2>Previews</h2>"
    " This tab contains options for the file previews in Konqueror.");
}


void KonqyModule::moduleChanged(bool state)
{
  emit changed(state);
}

/////////////////////////////////////////////////////////////////////////////

KDesktopModule::KDesktopModule(QWidget *parent, const char *)
  : KCModule(parent, "kcmkonq")
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
  tab->addTab(root, i18n("D&esktop"));
  connect(root, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  font = new KonqFontOptions(config, "FMSettings", true, this);
  tab->addTab(font, i18n("A&ppearance"));
  connect(font, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  virtualDesks = new KDesktopConfig(this, "VirtualDesktops");
  tab->addTab(virtualDesks, i18n("&Number of Desktops"));
  connect(virtualDesks, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  paths = new DesktopPathConfig(this);
  tab->addTab(paths, i18n("&Paths"));
  connect(paths, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  //should we add Trash also here ?
}


void KDesktopModule::load()
{
  root->load();
  font->load();
  virtualDesks->load();
  paths->load();
}


void KDesktopModule::save()
{
  root->save();
  font->save();
  virtualDesks->save();
  paths->save();

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
  paths->defaults();
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

*/

static QCString configname()
{
	int desktop=0;
	if (qt_xdisplay())
		desktop = DefaultScreen(qt_xdisplay());
	QCString name;
	if (desktop == 0)
		name = "kdesktoprc";
    else
		name.sprintf("kdesktop-screen-%drc", desktop);

	return name;
}


extern "C"
{

  KCModule *create_behavior(QWidget *parent, const char *name)
  {
    KConfig *config = new KConfig("konquerorrc", false, true);
    return new KBehaviourOptions(config, "FMSettings", parent, name);
  }

  KCModule *create_appearance(QWidget *parent, const char *name)
  {
    KConfig *config = new KConfig("konquerorrc", false, true);
    return new KonqFontOptions(config, "FMSettings", false, parent, name);
  }

  KCModule *create_previews(QWidget *parent, const char *name)
  {
    return new KPreviewOptions(parent, name);
  }


  KCModule *create_dbehavior(QWidget *parent, const char *name)
  {
    KConfig *config = new KConfig(configname(), false, false);
    return new KRootOptions(config, parent);
  }

  KCModule *create_dappearance(QWidget *parent, const char *name)
  {
    KConfig *config = new KConfig(configname(), false, false);
    return new KonqFontOptions(config, "FMSettings", true, parent);
  }

  KCModule *create_dpath(QWidget *parent, const char *name)
  {
    KConfig *config = new KConfig(configname(), false, false);
    return new DesktopPathConfig(parent);
  }

  KCModule *create_ddesktop(QWidget *parent, const char *name)
  {
    return new KDesktopConfig(parent, "VirtualDesktops");
  }
}


