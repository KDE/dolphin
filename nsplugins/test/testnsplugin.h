/*
  This is an encapsulation of the  Netscape plugin API.

  Copyright (c) 2000 Stefan Schimanski <1Stein@gmx.de>
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 
*/                                                                            


#ifndef __TESTNSPLUGIN_H__
#define __TESTNSPLUGIN_H__

#include <QString>
#include <QWidget>
//Added by qt3to4:
#include <Q3PtrList>
#include <Q3BoxLayout>
#include <kmainwindow.h>
#include <QLayout>
#include <QList>

class NSPluginLoader;
class NSPluginInstance;

class TestNSPlugin : public KMainWindow
{
  Q_OBJECT

public:
  TestNSPlugin();
  virtual ~TestNSPlugin();

public Q_SLOTS:
  void newView();
  void closeView();
  void viewDestroyed( NSPluginInstance *inst );

protected:
  NSPluginLoader *m_loader;
  Q3PtrList<QWidget> m_plugins;
  QWidget *m_client;
  Q3BoxLayout *m_layout;
};


#endif
