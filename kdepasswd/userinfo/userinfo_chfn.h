/***************************************************************************
 *   Copyright 2003 Braden MacDonald <bradenm_k@shaw.ca>                   *
 *   Copyright 2003 Ravikiran Rajagopal <ravi@ee.eng.ohio-state.edu>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License (version 2) as   *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/
/**
 * @file UserInfo-chfn: Dialog for calling chfn to change user's full name
 * @author Braden MacDonald
 */

#ifndef USERINFO_CHFN_H
#define USERINFO_CHFN_H

#include <kdialogbase.h>
#include <qlineedit.h>

class KUserInfoChFnDlg : public KDialogBase
{
  Q_OBJECT
public:
  KUserInfoChFnDlg(QString *userName, QString *userFullName, QWidget *parent=0, const char *name=0, bool modal=true);

  QString getname();
  QString getpass();

private slots:
  void slotTextChanged( const QString & );

private:
  QLineEdit *m_UNEdit;
  QLineEdit *m_PsEdit;

  QString m_FullName;
  QString m_Password;
};

#endif
