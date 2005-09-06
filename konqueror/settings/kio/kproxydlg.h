/*
   kproxydlg.h - Proxy configuration dialog

   Copyright (C) 2001- Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License (GPL) version 2 as published by the Free Software
   Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KPROXYDIALOG_H
#define _KPROXYDIALOG_H

#include <qstring.h>

#include <kcmodule.h>

class QTabWidget;

class KProxyData;
class KProxyDialogUI;

class KProxyOptions : public KCModule
{
  Q_OBJECT

public:
  KProxyOptions( QWidget* parent = 0 );
  ~KProxyOptions();

  virtual void load();
  virtual void save();
  virtual void defaults();
  virtual QString quickHelp() const;

private:
  KCModule* mProxy;
  KCModule* mSocks;
  QTabWidget* mTab;
};

class KProxyDialog : public KCModule
{
  Q_OBJECT

public:
  KProxyDialog( QWidget* parent = 0 );
  ~KProxyDialog();

  virtual void load();
  virtual void save();
  virtual void defaults();
  QString quickHelp() const;

private slots:
  void slotChanged();
  void slotUseProxyChanged();

  void setupManProxy();
  void setupEnvProxy();

private:
  void showInvalidMessage( const QString& _msg = QString::null );

private:
  KProxyDialogUI* mDlg;
  KProxyData* mData;
  bool mDefaultData;
};

#endif
