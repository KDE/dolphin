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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <qlabel.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

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
                              const char *name, FakeUASProvider* provider )
              :KDialog(parent, name, true), m_provider(provider)
{
  setCaption ( caption );

  QVBoxLayout* mainLayout = new QVBoxLayout(this, 0, 0);

  dlg = new UAProviderDlgUI (this);
  mainLayout->addWidget(dlg);

  QString wtstr = i18n( "Enter the site or domain where a fake browser "
                        "identification should be used. <p><u>NOTE:</u> "
                        "Wildcard syntaxes such as \"*,?\" are NOT allowed. "
                        "Instead use the top level address of a site to "
                        "make generic matches. For example, if you want all "
                        "KDE sites to receive a fake browser identification, "
                        "you would enter <code>.kde.org</code>. The fake "
                        "identity would then be sent to any KDE site that "
                        "ends with <code>.kde.org</code>." );
  QWhatsThis::add( dlg->lbSite, wtstr );
  QWhatsThis::add( dlg->leSite, wtstr );

  wtstr = i18n( "<qt>Select the browser identification to use whenever "
                "contacting the site you specified above.");
  QWhatsThis::add( dlg->lbAlias, wtstr );
  QWhatsThis::add( dlg->cbAlias, wtstr );


  wtstr = i18n( "The actual browser identification text that will be sent "
                "to the remote machine." );
  QWhatsThis::add( dlg->lbIdentity, wtstr );
  QWhatsThis::add( dlg->leIdentity, wtstr );

  // Update button
  wtstr = i18n( "Updates the browser identification list.\n"
                "<p><u>NOTE:</u> There is no need to press this button "
                "unless a new description file was added while this "
                "configuration box is displayed!" );
  QWhatsThis::add( dlg->pbUpdateList, wtstr );

  connect( dlg->pbOk, SIGNAL(clicked()), SLOT(accept()) );
  connect( dlg->pbCancel, SIGNAL(clicked()), SLOT(reject()) );

  connect( dlg->leSite, SIGNAL(textChanged(const QString&)),
           SLOT(slotTextChanged( const QString&)) );

  connect( dlg->cbAlias, SIGNAL(activated(const QString&)),
           SLOT(slotActivated(const QString&)) );

  connect( dlg->pbUpdateList, SIGNAL(clicked()), SLOT(updateInfo()) );

  init();
  dlg->leSite->setFocus();
}

UAProviderDlg::~UAProviderDlg()
{
}


void UAProviderDlg::init()
{
  if ( !m_provider )
    m_provider = new FakeUASProvider();

  dlg->cbAlias->clear();
  dlg->cbAlias->insertStringList( m_provider->userAgentAliasList() );
  dlg->cbAlias->insertItem( "", 0 );
  dlg->cbAlias->listBox()->sort();
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

void UAProviderDlg::updateInfo()
{
  QString citem = dlg->cbAlias->currentText();
  m_provider->setListDirty(true);
  init();
  setIdentity(citem);
}

void UAProviderDlg::setSiteName( const QString& text )
{
  dlg->leSite->setText( text );
}

void UAProviderDlg::setIdentity( const QString& text )
{
  int id = dlg->cbAlias->listBox()->index( dlg->cbAlias->listBox()->findItem(text) );
  dlg->cbAlias->setCurrentItem( id );
  slotActivated( dlg->cbAlias->currentText() );
  if ( !dlg->leSite->isEnabled() )
    dlg->cbAlias->setFocus();
}

QString UAProviderDlg::siteName()
{
  return dlg->leSite->text().lower();
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
