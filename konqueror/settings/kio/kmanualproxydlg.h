/*
   kmanualproxydlg.h - Base dialog box for proxy configuration

   Copyright (C) 2001, 2002,2003 - Dawit Alemayehu <adawit@kde.org>

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

#ifndef KMANUAL_PROXY_DIALOG_H
#define KMANUAL_PROXY_DIALOG_H

#include "kproxydlgbase.h"

class QSpinBox;
class KLineEdit;
class ManualProxyDlgUI;

class KManualProxyDlg : public KProxyDialogBase
{
  Q_OBJECT
  
public:
  KManualProxyDlg( QWidget* parent = 0, const char* name = 0 );
  ~KManualProxyDlg() {};
  
  virtual void setProxyData( const KProxyData &data );
  virtual const KProxyData data() const;
  
protected:
  void init();
  bool validate();
  
protected slots:
  virtual void slotOk();
  
  void copyDown();
  void sameProxy( bool );
  void valueChanged (int value);
  void textChanged (const QString&);
  
  void newPressed();
  void updateButtons();
  void changePressed();
  void deletePressed();
  void deleteAllPressed();
  
private:
  QString urlFromInput( const KLineEdit* edit, const QSpinBox* spin ) const;
  bool isValidURL( const QString&, KURL* = 0 ) const;
  bool handleDuplicate( const QString& );
  bool getException ( QString&, const QString&,
                      const QString& value = QString::null );
  void showErrorMsg( const QString& caption = QString::null,
                     const QString& message = QString::null );
  
private:
  ManualProxyDlgUI* dlg;

  int m_oldFtpPort;
  int m_oldHttpsPort;
  QString m_oldFtpText;
  QString m_oldHttpsText;
};
#endif
