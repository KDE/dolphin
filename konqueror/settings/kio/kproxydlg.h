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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KPROXYDIALOG_H
#define _KPROXYDIALOG_H

#include <qstring.h>

#include <kcmodule.h>

class QCheckBox;
class QPushButton;
class QRadioButton;
class QButtonGroup;

class KProxyData;
class KURLRequester;


class KProxyDialog : public KCModule
{
  Q_OBJECT

public:
  KProxyDialog( QWidget* parent = 0, const char* name = 0 );
  ~KProxyDialog();

  virtual void load();
  virtual void save();
  virtual void defaults();
  QString quickHelp() const;

protected slots:
  void autoDiscoverChecked();
  void autoScriptChecked( bool );
  void manualChecked();
  void envVarChecked();
  void promptChecked();
  void autoChecked();

  void useProxyChecked( bool );
  void autoScriptChanged( const QString& );
  void setupManProxy();
  void setupEnvProxy();

private:
  QCheckBox* m_cbUseProxy;

  QRadioButton* m_rbAutoDiscover;
  QRadioButton* m_rbAutoScript;
  QRadioButton* m_rbAutoLogin;
  QRadioButton* m_rbEnvVar;
  
  QRadioButton* m_rbManual;
  QRadioButton* m_rbPrompt;

  QPushButton* m_pbEnvSetup;
  QPushButton* m_pbManSetup;

  QButtonGroup* m_gbAuth;
  QButtonGroup* m_gbConfigure;

  KURLRequester* m_location;

  KProxyData* m_data;
};

#endif
