/*
   kenvvarproxydlg.h - Base dialog box for proxy configuration

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
#ifndef KENVVAR_PROXY_DIALOG_H
#define KENVVAR_PROXY_DIALOG_H

#include <qstringlist.h>

#include "kproxydlgbase.h"

class QCheckBox;
class QGroupBox;
class QPushButton;

class KLineEdit;
class KExceptionBox;

class KEnvVarProxyDlg : public KProxyDialogBase
{
    Q_OBJECT

public:
    KEnvVarProxyDlg( QWidget* parent = 0, const char* name = 0 );
    ~KEnvVarProxyDlg();

    virtual void setProxyData( const KProxyData &data );
    virtual const KProxyData data() const;

protected slots:
    void showValue( bool );
    void sameProxy( bool );
    void setChecked( bool );

    void verifyPressed();
    void autoDetectPressed();

    virtual void slotOk();

protected:
    void init();
    bool validate();

private:
    QCheckBox *m_cbEnvFtp;
    QCheckBox *m_cbEnvHttp;
    QCheckBox *m_cbEnvHttps;
    QCheckBox *m_cbEnvGopher;
    QCheckBox *m_cbSameProxy;

    KLineEdit *m_leEnvHttp;
    KLineEdit *m_leEnvHttps;
    KLineEdit *m_leEnvFtp;

    QPushButton *m_pbVerify;
    QPushButton *m_pbDetect;
    QPushButton *m_pbShowValue;

    QGroupBox *m_gbHostnames;
    KExceptionBox *m_gbExceptions;

    QStringList m_lstEnvVars;
};
#endif
