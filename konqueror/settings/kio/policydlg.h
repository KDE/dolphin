/**
 * Copyright (c) 2000-2001 Dawit Alemayehu <adawit@kde.org>
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

#ifndef _POLICYDLG_H
#define _POLICYDLG_H

#include <qstring.h>

#include <klineedit.h>
#include <kcombobox.h>
#include <kdialog.h>

class QPushButton;

class DomainLineEdit : public KLineEdit
{
  Q_OBJECT

public:
  DomainLineEdit( QWidget *parent, const char *name=0 );

protected:
  virtual void keyPressEvent( QKeyEvent * );
};

class PolicyDialog : public KDialog
{
    Q_OBJECT

public:
    PolicyDialog( const QString& caption, QWidget *parent = 0, const char *name = 0 );
    ~PolicyDialog() {};

    /*
    * @return 1 for "Accept", 2 for "Reject" and 3 for "Ask"
    */
    int policyAdvice() const { return m_cbPolicy->currentItem() + 1; }
    QString domain() const { return m_leDomain->text(); }

    void setEnableHostEdit( bool, const QString& hname = QString::null );
    void setDefaultPolicy( int /*value*/ );

protected slots:
    void slotTextChanged( const QString& );

protected:
    virtual void keyPressEvent( QKeyEvent* );

private:
    DomainLineEdit*  m_leDomain;
    KComboBox*       m_cbPolicy;

    QPushButton*     m_btnOK;
    QPushButton*     m_btnCancel;
};

#endif
