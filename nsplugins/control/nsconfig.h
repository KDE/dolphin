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
#include "configwidget.h"


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

 protected slots:
  void change() { change( true ); };
  void change( bool c ) { emit changed(c); m_changed = c; };

  void scan();

 private:
  ConfigWidget *m_widget;
  bool m_changed;

/******************************************************************************/
 protected:
  void dirInit();
  void dirLoad( KConfig *config );
  void dirSave( KConfig *config );

 protected slots:
  void dirBrowse();
  void dirNew();
  void dirRemove();
  void dirUp();
  void dirDown();
  void dirEdited(const QString &);
  void dirSelect( QListBoxItem * );

/******************************************************************************/
 protected:
  void pluginInit();
  void pluginLoad( KConfig *config );
  void pluginSave( KConfig *config );

};

#endif
