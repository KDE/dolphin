/*
   kproxyexceptiondlg.h - Proxy exception configuration dialog

   Copyright (C) 2002- Dawit Alemayehu <adawit@kde.org>

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

#ifndef KPROXY_EXCEPTION_DLG_H
#define KPROXY_EXCEPTION_DLG_H

#include <qstring.h>
#include <qgroupbox.h>
#include <qstringlist.h>

#include <kdialogbase.h>

class KLineEdit;
class KListView;

class QCheckBox;
class QPushButton;


class KExceptionBox : public QGroupBox
{
    Q_OBJECT

public:
    KExceptionBox( QWidget* parent = 0, const char* name = 0 );
    ~KExceptionBox() {};

    QStringList exceptions() const;
    void fillExceptions( const QStringList& items );
    bool isReverseProxyChecked() const;
    void setCheckReverseProxy( bool check );

protected slots:
    void newPressed();
    void updateButtons();
    void changePressed();
    void deletePressed();
    void deleteAllPressed();

private:
    bool handleDuplicate( const QString& );

private:
    QPushButton* m_pbNew;
    QPushButton* m_pbChange;
    QPushButton* m_pbDelete;
    QPushButton* m_pbDeleteAll;

    QCheckBox* m_cbReverseproxy;

    KListView* m_lvExceptions;
};

class KProxyExceptionDlg : public KDialogBase
{
  Q_OBJECT

public:
    KProxyExceptionDlg( QWidget* parent = 0, const char* name = 0, 
                        bool modal = true, const QString &caption = QString::null,
                        const QString &msg = QString::null );
    ~KProxyExceptionDlg();

    QString exception() const;
    void setException( const QString& );

protected slots:
    void slotTextChanged( const QString& );

private:
    KLineEdit* m_leExceptions;
};
#endif
