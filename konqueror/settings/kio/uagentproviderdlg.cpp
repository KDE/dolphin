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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qvbox.h>
#include <qaccel.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <kurllabel.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kprotocolmanager.h>

#include "fakeuaprovider.h"
#include "uagentproviderdlg.h"

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
  setIcon( SmallIcon("agent") );
  QVBoxLayout *lay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );
  lay->setAutoAdd( true );

  setCaption( caption );
  QLabel* label = new QLabel( i18n( "When connecting &to the following site:" ), this );
  m_leSite = new UALineEdit( this );
  label->setBuddy( m_leSite );
  connect( m_leSite, SIGNAL( textChanged(const QString&)), SLOT(slotTextChanged(const QString&)) );
  QString wtstr = i18n( "Enter the site or domain name where a fake identity should be used.  "
                        "<p><u>NOTE:</u> Wildcard syntax such as \"*,?\" is NOT allowed.  Instead "
                        "enter the top level address of a site to make generic matches for all "
                        "locations found under it.  For example, if you want all sites at "
                        "<code>http://www.acme.com</code> to receive fake browser-identification, "
                        "simply enter <code>acme.com</code> here.");
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_leSite, wtstr );

  label = new QLabel( i18n("Use the following &identity:"), this );
  m_cbIdentity = new KComboBox( false, this );
  m_cbIdentity->setInsertionPolicy( QComboBox::AtBottom );
  label->setBuddy( m_cbIdentity );
  m_cbIdentity->setMinimumWidth( m_cbIdentity->fontMetrics().width('W') * 30 );
  connect ( m_cbIdentity, SIGNAL(activated(const QString&)), SLOT(slotActivated(const QString&)) );
  wtstr = i18n( "<qt>Select the browser-identification to use whenever "
                "contacting the site or domain given above."
                "<P>Upon selection, a straight forward description, if "
                "available, will be displayed in the box below." );
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_cbIdentity, wtstr );

  label = new QLabel( i18n("Alias (description):"), this );
  m_leAlias = new KLineEdit( this );
  m_leAlias->setReadOnly( true );
  label->setBuddy( m_leAlias );
  wtstr = i18n( "A plain (friendlier) description of the above "
                "browser-identification string." );
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_leAlias, wtstr );

  // Organize the buttons in a button box...
  QWidget* btnBox = new QWidget( this );
  QBoxLayout *btnLay = new QHBoxLayout( btnBox );
  btnLay->setSpacing( KDialog::spacingHint() );

  // Update Button
  QPushButton* btn = new QPushButton( i18n("&Update List"), btnBox );
  wtstr = i18n( "Updates the browser identification list shown above."
                "<p>\n<u>NOTE:</u> There is no need to press this button "
                "unless a new browser-identfication description "
                "file was added after this configuration box was already "
                "displayed!" );
  QWhatsThis::add( btn, wtstr );
  connect( btn, SIGNAL(clicked()), SLOT(updateInfo()) );
  btnLay->addWidget( btn );
  btnLay->addStretch( 1 );

  // OK button
  m_btnOK = new QPushButton( i18n("&OK"), btnBox );
  btnLay->addWidget( m_btnOK );
  m_btnOK->setDefault( true );
  m_btnOK->setEnabled( false );
  connect( m_btnOK, SIGNAL(clicked()), SLOT(accept()) );

  // Cancel Button
  btn = new QPushButton( i18n("&Cancel"), btnBox );
  btnLay->addWidget( btn );
  connect( btn, SIGNAL(clicked()), SLOT(reject()) );
  QAccel* a = new QAccel( this );
  a->connectItem( a->insertItem(Qt::Key_Escape), btn, SLOT(animateClick()) );
  setBaseSize( minimumSizeHint() );
  init();
  m_leSite->setFocus();
}

UAProviderDlg::~UAProviderDlg()
{
}

void UAProviderDlg::slotActivated( const QString& text )
{
  if ( text.isEmpty() )
  {
    m_leAlias->setText( "" );
    m_btnOK->setEnabled( false );
  }
  else
  {
    m_leAlias->setText( m_provider->aliasFor(text) );
    if ( !m_leSite->text().isEmpty() && !text.isEmpty()  )
      m_btnOK->setEnabled( true );
  }
}

void UAProviderDlg::slotTextChanged( const QString& text )
{
  m_btnOK->setEnabled( !(text.isEmpty() || m_cbIdentity->currentText().isEmpty()) );
}

void UAProviderDlg::init()
{
  if ( !m_provider )
    m_provider = new FakeUASProvider();
  m_cbIdentity->clear();
  m_cbIdentity->insertStringList( m_provider->userAgentStringList() );
  m_cbIdentity->insertItem( "", 0 );
}

void UAProviderDlg::updateInfo()
{
  QString citem = m_cbIdentity->currentText();
  m_provider->setListDirty(true);
  init();
  setIdentity(citem);
}

void UAProviderDlg::setEnableHostEdit( bool state )
{
  m_leSite->setEnabled( state );
}

void UAProviderDlg::setSiteName( const QString& text )
{
  m_leSite->setText( text );
}

void UAProviderDlg::setIdentity( const QString& text )
{
  int id = m_cbIdentity->listBox()->index( m_cbIdentity->listBox()->findItem(text) );
  m_cbIdentity->setCurrentItem( id );
  slotActivated( m_cbIdentity->currentText() );
  if ( !m_leSite->isEnabled() )
    m_cbIdentity->setFocus();
}

QString UAProviderDlg::siteName()
{
  return m_leSite->text();
}

QString UAProviderDlg::identity()
{
  return m_cbIdentity->currentText();
}

QString UAProviderDlg::alias()
{
  return m_leAlias->text();
}

#include "uagentproviderdlg.moc"
