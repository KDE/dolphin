/*
 * main.h
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 * Copyright (c) 2000 Daniel Molkentin <molkentin@kde.org>
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


#ifndef __MAIN_H__
#define __MAIN_H__

#include <kcmodule.h>

class KAppearanceOptions;
class KJavaOptions;
class KJavaScriptOptions;
class KPluginOptions;
class KHTTPOptions;
class KMiscHTMLOptions;
class KRootOptions;

class QTabWidget;

class KonqHTMLModule : public KCModule
{
  Q_OBJECT

public:

  KonqHTMLModule(QWidget *parent, const char *name);
  virtual ~KonqHTMLModule();

  void load();
  void save();
  void defaults();
  QString quickHelp() const;


protected:

protected slots:

  void moduleChanged(bool state);


private:

  QTabWidget   *tab;

  KMiscHTMLOptions   *misc;
  KAppearanceOptions *appearance;
  KJavaScriptOptions *javascript;
  KJavaOptions       *java;
  KPluginOptions     *plugin;

  KConfig *m_globalConfig;
  KConfig *m_localConfig;
};

#endif
