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
 * @file UserInfo's Interface to chfn
 * @author Braden MacDonald
 */
#include <qstring.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <kdialogbase.h>
#include <klocale.h>

#include "userinfo_chfn.h"

KUserInfoChFnDlg::KUserInfoChFnDlg(QString *userName, QString *userFullName,
                                   QWidget *parent, const char *name, bool modal)
  : KDialogBase( parent, name, modal, i18n("Change your Real Name"), Ok|Cancel, Ok )
{
  QWidget *page = new QWidget(this);
  setMainWidget( page );

  QVBoxLayout *top = new QVBoxLayout(page, 10);
  top->setAlignment( Qt::AlignTop );

  QLabel *header = new QLabel( i18n("Changing real name for user %1:\n").arg( *userName ), page );
  top->addWidget( header );

  // Name input
  m_UNEdit = new QLineEdit( page );
  m_UNEdit->setText( *userFullName );
  QLabel *unlabel = new QLabel( m_UNEdit, i18n("Your new &name:"), page );
  top->addWidget( unlabel );
  top->addWidget( m_UNEdit );
  m_UNEdit->setFocus();

  // Password input
  m_PsEdit = new QLineEdit( page );
  m_PsEdit->setEchoMode( QLineEdit::Password );
  QLabel *pslabel = new QLabel( m_PsEdit, i18n("Your &password:"), page );
  top->addWidget( pslabel );

  connect( m_PsEdit, SIGNAL( textChanged( const QString & ) ), SLOT( slotTextChanged( const QString & ) ) );

  top->addWidget( m_PsEdit );

  this->enableButtonOK( false );
}

void KUserInfoChFnDlg::slotTextChanged( const QString &newps )
{
  enableButtonOK( !newps.isEmpty() );
}

QString KUserInfoChFnDlg::getname()
{
  return m_UNEdit->text();
}

QString KUserInfoChFnDlg::getpass()
{
  return m_PsEdit->text();
}
