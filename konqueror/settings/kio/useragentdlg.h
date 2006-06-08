/*
   Original Authors:
   Copyright (c) Kalle Dalheimer <kalle@kde.org> 1997
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _USERAGENTDLG_H
#define _USERAGENTDLG_H

#include <kcmodule.h>

class KConfig;
class FakeUASProvider;
class UserAgentDlgUI;

class UserAgentDlg : public KCModule
{
  Q_OBJECT

public:
  UserAgentDlg ( KInstance *inst, QWidget * parent) ;
  ~UserAgentDlg();

  virtual void load();
  virtual void save();
  virtual void defaults();
  QString quickHelp() const;

private Q_SLOTS:
  void updateButtons();
  void selectionChanged();

  void addPressed();
  void changePressed();
  void deletePressed();
  void deleteAllPressed();

  void configChanged();
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

  // Useragent modifiers...
  QString m_ua_keys;

  // Fake user-agent modifiers...
  FakeUASProvider* m_provider;

  //
  int d_itemsSelected;

  KConfig *m_config;
  UserAgentDlgUI* dlg;
};

#endif
