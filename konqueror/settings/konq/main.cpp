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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
#include <QDesktopWidget>

static QString configname()
{
	int desktop = KApplication::desktop()->primaryScreen();
	QString name;
	if (desktop == 0)
		name = "kdesktoprc";
    else
		name.sprintf("kdesktop-screen-%drc", desktop);

	return name;
}

static KInstance *_kcmkonq = 0;

inline KInstance *kcmkonq() {
	if (_kcmkonq)
		return _kcmkonq;
	_kcmkonq = new KInstance("kcmkonq");
	return _kcmkonq;
}

extern "C"
{
  KDE_EXPORT KCModule *create_browser(QWidget *parent, const char *name)
  {
    KConfig *config = new KConfig("konquerorrc", false, true);
    return new KBrowserOptions(config, "FMSettings", kcmkonq(), parent);
  }

  KDE_EXPORT KCModule *create_behavior(QWidget *parent, const char *name)
  {
    KConfig *config = new KConfig("konquerorrc", false, true);
    return new KBehaviourOptions(config, "FMSettings", kcmkonq(), parent);
  }

  KDE_EXPORT KCModule *create_appearance(QWidget *parent, const char *name)
  {
    KConfig *config = new KConfig("konquerorrc", false, true);
    return new KonqFontOptions(config, "FMSettings", false, kcmkonq(), parent);
  }

  KDE_EXPORT KCModule *create_previews(QWidget *parent, const char *name)
  {
    return new KPreviewOptions(kcmkonq(), parent);
  }

  KDE_EXPORT KCModule *create_dbehavior(QWidget *parent, const char* /*name*/)
  {
    KConfig *config = new KConfig(configname(), false, false);
    return new DesktopBehaviorModule(config, kcmkonq(), parent);
  }

  KDE_EXPORT KCModule *create_dappearance(QWidget *parent, const char* /*name*/)
  {
    KConfig *config = new KConfig(configname(), false, false);
    return new KonqFontOptions(config, "FMSettings", true, kcmkonq(), parent);
  }

  KDE_EXPORT KCModule *create_dpath(QWidget *parent, const char* /*name*/)
  {
    //KConfig *config = new KConfig(configname(), false, false);
    return new DesktopPathConfig(kcmkonq(), parent);
  }

  KDE_EXPORT KCModule *create_ddesktop(QWidget *parent, const char* /*name*/)
  {
    return new KDesktopConfig(kcmkonq(), parent);
  }
}


