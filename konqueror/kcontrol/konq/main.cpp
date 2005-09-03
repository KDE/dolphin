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
 *  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include "rootopts.h"
#include "behaviour.h"
#include "fontopts.h"
#include "desktop.h"
#include "previews.h"
#include "browser.h"
#include "desktopbehavior_impl.h"

#include <kconfig.h>
#include <kapplication.h>
//Added by qt3to4:
#include <Q3CString>
#include <QDesktopWidget>

static Q3CString configname()
{
	int desktop = KApplication::desktop()->primaryScreen();
	Q3CString name;
	if (desktop == 0)
		name = "kdesktoprc";
    else
		name.sprintf("kdesktop-screen-%drc", desktop);

	return name;
}


extern "C"
{
  KDE_EXPORT KCModule *create_browser(QWidget *parent, const char *name)
  {
    KConfig *config = new KConfig("konquerorrc", false, true);
    return new KBrowserOptions(config, "FMSettings", parent, name);
  }

  KDE_EXPORT KCModule *create_behavior(QWidget *parent, const char *name)
  {
    KConfig *config = new KConfig("konquerorrc", false, true);
    return new KBehaviourOptions(config, "FMSettings", parent, name);
  }

  KDE_EXPORT KCModule *create_appearance(QWidget *parent, const char *name)
  {
    KConfig *config = new KConfig("konquerorrc", false, true);
    return new KonqFontOptions(config, "FMSettings", false, parent, name);
  }

  KDE_EXPORT KCModule *create_previews(QWidget *parent, const char *name)
  {
    return new KPreviewOptions(parent, name);
  }

  KDE_EXPORT KCModule *create_dbehavior(QWidget *parent, const char* /*name*/)
  {
    KConfig *config = new KConfig(configname(), false, false);
    return new DesktopBehaviorModule(config, parent);
  }

  KDE_EXPORT KCModule *create_dappearance(QWidget *parent, const char* /*name*/)
  {
    KConfig *config = new KConfig(configname(), false, false);
    return new KonqFontOptions(config, "FMSettings", true, parent);
  }

  KDE_EXPORT KCModule *create_dpath(QWidget *parent, const char* /*name*/)
  {
    //KConfig *config = new KConfig(configname(), false, false);
    return new DesktopPathConfig(parent);
  }

  KDE_EXPORT KCModule *create_ddesktop(QWidget *parent, const char* /*name*/)
  {
    return new KDesktopConfig(parent, "VirtualDesktops");
  }
}


