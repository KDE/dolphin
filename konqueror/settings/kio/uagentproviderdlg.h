/**
 * Copyright (c) 2001 Dawit Alemayehu <adawit@kde.org>
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

#ifndef __UAPROVIDERDLG_H___
#define __UAPROVIDERDLG_H___

#include <qgroupbox.h>

#include <kdialog.h>
#include <klineedit.h>

class QPushButton;

class KComboBox;
class FakeUASProvider;

class UALineEdit : public KLineEdit
{
  Q_OBJECT

public:
  UALineEdit( QWidget *parent, const char *name=0 );

protected:
  virtual void keyPressEvent( QKeyEvent * );
};

class UAProviderDlg : public KDialog
{
  Q_OBJECT

public:
  UAProviderDlg( const QString& caption,
                 QWidget *parent = 0,
                 const char *name = 0,
                 FakeUASProvider* provider = 0 );
  ~UAProviderDlg();

  void setSiteName( const QString& );
  void setIdentity( const QString& );
  void setEnableHostEdit( bool );

  QString siteName();
  QString identity();
  QString alias();

protected slots:
  void slotActivated( const QString& );
  void slotTextChanged( const QString& );
  void updateInfo();

protected:
  void init();

private:
  FakeUASProvider* m_provider;

  QGroupBox*     m_gbNewBox;
  QPushButton*   m_btnOK;
  UALineEdit*    m_leSite;
  KComboBox*     m_cbIdentity;
  KLineEdit*     m_leAlias;
};
#endif
