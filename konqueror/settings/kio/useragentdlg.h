/*
   Original Authors:
   Copyright (c) Kalle Dalheimer 1997
   Copyright (c) David Faure <faure@kde.org> 1998
   Copyright (c) Dirk Mueller <mueller@kde.org> 2000

   Completely re-written by:
   Copyright (C) 2000- Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License (GPL)
   version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _USERAGENTDLG_H
#define _USERAGENTDLG_H "$Id$"

#include <kcmodule.h>
#include <kconfig.h>

class QLabel;
class QCheckBox;
class QGroupBox;
class QPushButton;
class QStringList;
class QButtonGroup;

class KListView;
class FakeUASProvider;

class UserAgentOptions : public KCModule
{
  Q_OBJECT

public:
  UserAgentOptions ( QWidget * parent = 0L, const char * name = 0L) ;
  ~UserAgentOptions();

  virtual void load();
  virtual void save();
  virtual void defaults();
  QString quickHelp() const;

private slots:
  void updateButtons();
  void selectionChanged();

  void deleteAllPressed();
  void deletePressed();
  void changePressed();
  void addPressed();
  void changed();
  
  void changeSendUAString();
  void changeDefaultUAModifiers( int );

private:
  bool handleDuplicate( const QString&, const QString&, const QString& );

  enum {
    SHOW_OS = 0,
    SHOW_OS_VERSION,
    SHOW_PLATFORM,
    SHOW_MACHINE,
    SHOW_LANGUAGE
  };

  //Send User-agent String
  QCheckBox*      cb_sendUAString;

  // Default User-agent settings
  QLabel*         lb_default;
  QButtonGroup*   bg_default;
  QCheckBox*      cb_showPlatform;
  QCheckBox*      cb_showLanguage;
  QCheckBox*      cb_showMachine;
  QCheckBox*      cb_showOSV;
  QCheckBox*      cb_showOS;

  // Site specific settings
  KListView*      lv_siteUABindings;
  QGroupBox*      gb_siteSpecific;
  QPushButton*    pb_deleteAll;
  QPushButton*    pb_delete;
  QPushButton*    pb_change;
  QPushButton*    pb_import;
  QPushButton*    pb_export;
  QPushButton*    pb_add;

  // Useragent modifiers...
  QString m_ua_keys;

  // Fake user-agent modifiers...
  FakeUASProvider* m_provider;

  //
  int d_itemsSelected;

  KConfig *m_config;
};

#endif
