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

#include <qspinbox.h>
#include <qstringlist.h>
#include <qbuttongroup.h>

#include <kdialog.h>
#include <kcmodule.h>
#include <klineedit.h>

class QLabel;
class QGroupBox;
class QCheckBox;
class QStringList;
class QPushButton;
class QRadioButton;
class QButtonGroup;
class QListViewItem;

class KListView;
class KURLRequester;

struct ProxyData
{
    bool changed;
    bool envBased;
    bool useReverseProxy;

    QString httpProxy;
    QString secureProxy;
    QString ftpProxy;
    QString gopherProxy;
    QString scriptProxy;
    QStringList noProxyFor;

    ProxyData() {
        init();
    }

    void reset() {
        init();
        httpProxy = QString::null;
        secureProxy = QString::null;
        ftpProxy = QString::null;
        gopherProxy = QString::null;
        scriptProxy = QString::null;
        noProxyFor.clear();
    }

private:
    void init() {
        changed = false;
        envBased = false;
        useReverseProxy = false;
    }
};

class KProxyExceptionDlg : public KDialog
{
    Q_OBJECT

public:
    KProxyExceptionDlg( QWidget* parent = 0, const char* name = 0 );
    ~KProxyExceptionDlg();

    QString exception() const;
    void setException( const QString& );

protected slots:
    virtual void accept();
    virtual void reject();
    void slotTextChanged( const QString& );

private:
    QPushButton* pb_ok;
    QPushButton* pb_cancel;
    QLineEdit* le_exceptions;
};

class KExceptionBox : public QGroupBox
{
    Q_OBJECT

public:
    KExceptionBox( QWidget* parent = 0, const char* name = 0 );
    ~KExceptionBox() {};

    QStringList exceptions() const;
    void fillExceptions( const ProxyData* d );
    bool isReverseProxyChecked() const;

protected slots:
    void newPressed();
    void updateButtons();
    void changePressed();
    void deletePressed();
    void deleteAllPressed();

private:
    bool handleDuplicate( const QString& );

    QCheckBox* cb_reverseproxy;
    KListView* lv_exceptions;

    QPushButton* pb_new;
    QPushButton* pb_change;
    QPushButton* pb_delete;
    QPushButton* pb_deleteAll;
};

class KCommonProxyDlg : public KDialog
{
public:
    KCommonProxyDlg( QWidget* parent = 0, const char* name = 0, bool modal = false )
    :KDialog( parent, name, modal ) {};

    ~KCommonProxyDlg() { d = 0L; }

    virtual void setProxyData( const ProxyData* ) {};
    virtual ProxyData data() const=0;

protected:
    const ProxyData* d;
};

class KEnvVarProxyDlg : public KCommonProxyDlg
{
    Q_OBJECT

public:
    KEnvVarProxyDlg( QWidget* parent = 0, const char* name = 0 );
    ~KEnvVarProxyDlg(){};

    virtual void setProxyData( const ProxyData* data );
    virtual ProxyData data() const;

protected slots:
    void verifyPressed();
    void showValue( bool );
    void setChecked( bool );
    void autoDetectPressed();

    virtual void accept();
    virtual void reject();

protected:
    void init();
    bool validate( unsigned short& );

private:
    // Servers box
    QGroupBox* gb_servers;
    QCheckBox* cb_envHttp;
    QCheckBox* cb_envSecure;
    QCheckBox* cb_envFtp;
    QCheckBox* cb_envGopher;
    QCheckBox* cb_showValue;

    KLineEdit* le_envHttp;
    KLineEdit* le_envSecure;
    KLineEdit* le_envFtp;
    KLineEdit* le_envGopher;

    QPushButton* pb_verify;
    QPushButton* pb_detect;

    // Exception dialog
    KExceptionBox* gb_exceptions;

    // Dialog buttons
    QPushButton* pb_ok;
    QPushButton* pb_cancel;

    QStringList lst_envVars;
};


class KManualProxyDlg : public KCommonProxyDlg
{
    Q_OBJECT

public:
    KManualProxyDlg( QWidget* parent = 0, const char* name = 0 );
    ~KManualProxyDlg(){};

    virtual void setProxyData( const ProxyData* data );
    virtual ProxyData data() const;

protected slots:
    void copyDown();
    void setChecked( bool );

    virtual void accept();
    virtual void reject();

protected:
    void init();
    bool validate();

private:
    QGroupBox* gb_servers;
    QPushButton* pb_copyDown;

    QSpinBox* sb_httpproxy;
    QSpinBox* sb_secproxy;
    QSpinBox* sb_ftpproxy;
    QSpinBox* sb_gopherproxy;

    QCheckBox* cb_httpproxy;
    QCheckBox* cb_secproxy;
    QCheckBox* cb_ftpproxy;
    QCheckBox* cb_gopherproxy;

    KLineEdit* le_httpproxy;
    KLineEdit* le_secproxy;
    KLineEdit* le_ftpproxy;
    KLineEdit* le_gopherproxy;

    // Exception dialog
    KExceptionBox* gb_exceptions;

    QPushButton* pb_ok;
    QPushButton* pb_cancel;
};

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
    QCheckBox* cb_useProxy;
    QButtonGroup* gb_configure;
    QRadioButton* rb_autoDiscover;
    QRadioButton* rb_autoScript;
    KURLRequester* ur_location;
    QRadioButton* rb_envVar;
    QRadioButton* rb_manual;
    QPushButton* pb_envSetup;
    QPushButton* pb_manSetup;

    // Authorization
    QButtonGroup* gb_auth;
    QRadioButton* rb_prompt;
    QRadioButton* rb_autoLogin;

    ProxyData* d;
};

#endif // KPROXYDIALOG_H
