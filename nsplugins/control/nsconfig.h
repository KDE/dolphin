/**
 * nsconfig.h
 *
 * Copyright (c) 2000 Stefan Schimanski <1Stein@gmx.de>
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

#ifndef __nsconfig_h__
#define __nsconfig_h__

#include <kcmodule.h>

class QCheckBox;
class QListView;
class KProcess;

class NSPluginConfig : public KCModule
{
  Q_OBJECT

public:
  NSPluginConfig(QWidget *parent = 0L, const char *name = 0L);
  virtual ~NSPluginConfig();

  void load();
  void save();
  void defaults();
  QString quickHelp() const;

  int buttons();

protected slots:
  void configChanged();

  void changeDirs();
  void scan();
  void fillPluginList();

private:
  QCheckBox *m_startkdeScan;
  QListView *m_pluginList;
};

#endif
