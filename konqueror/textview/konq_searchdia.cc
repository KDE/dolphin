/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/ 

#include "konq_searchdia.h"

#include <qlabel.h>

#include <kbuttonbox.h>
#include <kseparator.h>
#include <klocale.h>

KonqSearchDialog::KonqSearchDialog( QWidget *parent )
: QDialog( parent, 0L, true )
{
  setCaption( i18n( "Konqueror: Search" ) );
  
  m_bFirstSearch = true;
  
  m_pVBox = new QVBox( this );
  
  QHBox *hBox = new QHBox( m_pVBox );
  
  (void) new QLabel( i18n( "Find :"), hBox );
  
  m_pLineEdit = new QLineEdit( hBox );
  connect( m_pLineEdit, SIGNAL( textChanged( const QString & ) ),
           this, SLOT( slotTextChanged() ) );
  
  hBox->setSpacing( 5 );

  hBox = new QHBox( m_pVBox );
  
  m_pCaseSensitiveCheckBox = new QCheckBox( i18n( "Case Sensitive" ), hBox );
  
  m_pBackwardsCheckBox = new QCheckBox( i18n( "Find Backwards" ), hBox );

  hBox->setSpacing( 5 );
  
  m_pCaseSensitiveCheckBox->setChecked( false );
  m_pBackwardsCheckBox->setChecked( false );
  
  (void) new KSeparator( QFrame::HLine, m_pVBox );
  
  KButtonBox *buttonBox = new KButtonBox( m_pVBox );
  
  m_pFindButton = buttonBox->addButton( i18n( "Find" ) );
  buttonBox->addStretch( 1 );
  m_pClearButton = buttonBox->addButton( i18n( "Clear" ) );
  buttonBox->addStretch( 1 );
  m_pCloseButton = buttonBox->addButton( i18n( "Close" ) );
  buttonBox->addStretch( 1 );
  
  buttonBox->layout();
  
  connect( m_pFindButton, SIGNAL( clicked() ), this, SLOT( slotFind() ) );
  connect( m_pClearButton, SIGNAL( clicked() ), this, SLOT( slotClear() ) );
  connect( m_pCloseButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
 
  m_pVBox->setSpacing( 10 );
  m_pVBox->setMargin( 5 );
  
  resize( 300, 150 );
}

void KonqSearchDialog::slotFind()
{
  bool backwards = m_pBackwardsCheckBox->isChecked();
  bool caseSensitive = m_pCaseSensitiveCheckBox->isChecked();
  
  if ( m_bFirstSearch )
  {
    emit findFirst( m_pLineEdit->text(), backwards, caseSensitive );
    m_bFirstSearch = false;
  }
  else
    emit findNext( backwards, caseSensitive );
}

void KonqSearchDialog::slotClear()
{
  m_bFirstSearch = true;
  m_pLineEdit->clear();
}

void KonqSearchDialog::slotTextChanged()
{
  m_bFirstSearch = true;
}

void KonqSearchDialog::resizeEvent( QResizeEvent * )
{
  m_pVBox->setGeometry( 0, 0, width(), height() );
}

#include "konq_searchdia.moc"
