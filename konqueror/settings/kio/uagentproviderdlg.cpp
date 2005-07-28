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
 *  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <qlabel.h>
#include <qlayout.h>
#include <q3listbox.h>
#include <q3whatsthis.h>
#include <qpushbutton.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QKeyEvent>

#include <kdebug.h>
#include <klocale.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <kurllabel.h>

#include "fakeuaprovider.h"
#include "uagentproviderdlg.h"
#include "uagentproviderdlg_ui.h"

UALineEdit::UALineEdit( QWidget *parent, const char *name )
           :KLineEdit( parent, name )
{
  // For now do not accept any drops since they might contain
  // characters we do not accept.
  // TODO: Re-implement ::dropEvent to allow acceptable formats...
  setAcceptDrops( false );
}

void UALineEdit::keyPressEvent( QKeyEvent* e )
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

UAProviderDlg::UAProviderDlg( const QString& caption, QWidget *parent,
                              FakeUASProvider* provider, const char *name )
              :KDialog(parent, name, true), m_provider(provider)
{
  setCaption ( caption );

  QVBoxLayout* mainLayout = new QVBoxLayout(this, 0, 0);

  dlg = new UAProviderDlgUI (this);
  mainLayout->addWidget(dlg);
  //dlg->leIdentity->setEnableSqueezedText( true );

  if (!m_provider)
  {
    setEnabled( false );
    return;
  }

  init();
}

UAProviderDlg::~UAProviderDlg()
{
}

void UAProviderDlg::init()
{
  connect( dlg->pbOk, SIGNAL(clicked()), SLOT(accept()) );
  connect( dlg->pbCancel, SIGNAL(clicked()), SLOT(reject()) );

  connect( dlg->leSite, SIGNAL(textChanged(const QString&)),
                SLOT(slotTextChanged( const QString&)) );

  connect( dlg->cbAlias, SIGNAL(activated(const QString&)),
                SLOT(slotActivated(const QString&)) );

  dlg->cbAlias->clear();
  dlg->cbAlias->insertStringList( m_provider->userAgentAliasList() );
  dlg->cbAlias->insertItem( "", 0 );
  dlg->cbAlias->model()->sort( 0 );

  dlg->leSite->setFocus();
}

void UAProviderDlg::slotActivated( const QString& text )
{
  if ( text.isEmpty() )
    dlg->leIdentity->setText( "" );
  else
    dlg->leIdentity->setText( m_provider->agentStr(text) );

  dlg->pbOk->setEnabled( (!dlg->leSite->text().isEmpty() && !text.isEmpty()) );
}

void UAProviderDlg::slotTextChanged( const QString& text )
{
  dlg->pbOk->setEnabled( (!text.isEmpty() && !dlg->cbAlias->currentText().isEmpty()) );
}

void UAProviderDlg::setSiteName( const QString& text )
{
  dlg->leSite->setText( text );
}

void UAProviderDlg::setIdentity( const QString& text )
{
  int id = dlg->cbAlias->findText( text );
  if ( id != -1 )
     dlg->cbAlias->setCurrentItem( id );
  slotActivated( dlg->cbAlias->currentText() );
  if ( !dlg->leSite->isEnabled() )
    dlg->cbAlias->setFocus();
}

QString UAProviderDlg::siteName()
{
  QString site_name=dlg->leSite->text().lower();
  site_name = site_name.remove( "https://" );
  site_name = site_name.remove( "http://" );
  return site_name;
}

QString UAProviderDlg::identity()
{
  return dlg->cbAlias->currentText();
}

QString UAProviderDlg::alias()
{
  return dlg->leIdentity->text();
}

#include "uagentproviderdlg.moc"
