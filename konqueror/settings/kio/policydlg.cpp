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

#include <qstringlist.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qhbox.h>

#include <kglobal.h>
#include <klocale.h>
#include <kcombobox.h>
#include <kiconloader.h>
#include <kmessagebox.h>

#include "policydlg.h"

DomainLineEdit::DomainLineEdit( QWidget *parent, const char *name )
           :KLineEdit( parent, name )
{
  // For now do not accept any drops since they might contain
  // characters we do not accept.
  // TODO: Re-implement ::dropEvent to allow acceptable formats...
  setAcceptDrops( false );
}

void DomainLineEdit::keyPressEvent( QKeyEvent* e )
{
  int key = e->key();
  QString keycode = e->text();
  if ( (key >= Qt::Key_Escape && key <= Qt::Key_Help) || key == Qt::Key_Period ||
       (cursorPosition() > 0 && key == Qt::Key_Minus) ||
       (!keycode.isEmpty() && keycode.unicode()->isLetterOrNumber()) )
  {
    KLineEdit::keyPressEvent(e);
    return;
  }
  e->accept();
}


PolicyDialog::PolicyDialog( const QString& caption, QWidget *parent,
                            const char *name )
             :KDialog(parent, name, true)
{

  setCaption( caption );
  QVBoxLayout *topl = new QVBoxLayout(this, marginHint(), spacingHint());
  topl->setAutoAdd( true );

  QLabel *l = new QLabel(i18n("Site/domain name:"), this);
  m_leDomain = new DomainLineEdit(this);
  connect(m_leDomain, SIGNAL(textChanged(const QString&)), SLOT(slotTextChanged(const QString&)));
  QString wstr = i18n("Enter the name of the site or domain to "
                      "which this policy applies.");
  QWhatsThis::add( l, wstr );
  QWhatsThis::add( m_leDomain, wstr );

  l = new QLabel(i18n("Policy:"), this);
  m_cbPolicy = new KComboBox(this);
  m_cbPolicy->setMinimumWidth( m_cbPolicy->fontMetrics().width('W') * 25 );
  QStringList policies;
  policies << i18n( "Accept" ) << i18n( "Reject" ) << i18n( "Ask" );
  m_cbPolicy->insertStringList( policies );
  wstr = i18n("Select the desired policy:"
              "<ul><li><b>Accept</b> - Allows this site to set "
              "cookie</li><li><b>Reject</b> - Refuse all cookies "
              "sent from this site</li><li><b>Ask</b> - Prompt "
              "when cookies are received from this site</li></ul>");
  QWhatsThis::add( l, wstr );
  QWhatsThis::add( m_cbPolicy, wstr );

  QWidget* bbox = new QWidget( this );
  QBoxLayout* blay = new QHBoxLayout( bbox );
  blay->setSpacing( KDialog::spacingHint() );
  blay->addStretch( 1 );

  m_btnOK = new QPushButton( i18n("&OK"), bbox );
  connect(m_btnOK, SIGNAL(clicked()), this, SLOT(accept()));
  m_btnOK->setDefault( true );
  m_btnOK->setEnabled( false );
  blay->addWidget( m_btnOK );

  m_btnCancel = new QPushButton( i18n("&Cancel"), bbox );
  connect(m_btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
  blay->addWidget( m_btnCancel );

  setFixedSize( sizeHint() );
  m_leDomain->setFocus();
}

void PolicyDialog::setEnableHostEdit( bool state, const QString& hname )
{
  if ( !hname.isEmpty() )
    m_leDomain->setText( hname );
  m_leDomain->setEnabled( state );
}

void PolicyDialog::setDefaultPolicy( int value )
{
  if( value > -1 && value < (int) m_cbPolicy->count() )
    m_cbPolicy->setCurrentItem( value );

  if ( !m_leDomain->isEnabled() )
    m_cbPolicy->setFocus();
}

void PolicyDialog::keyPressEvent( QKeyEvent* e )
{
  int key = e->key();
  if ( key == Qt::Key_Escape )
  {
    e->accept();
    m_btnCancel->animateClick();
  }
  KDialog::keyPressEvent( e );
}

void PolicyDialog::slotTextChanged( const QString& text )
{
  m_btnOK->setEnabled( !text.isEmpty() );
}
#include "policydlg.moc"
