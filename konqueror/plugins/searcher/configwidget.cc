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

#include "configwidget.h"

#include <qlayout.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qlistbox.h>

#include <klocale.h>
#include <kapp.h>

#include <iostream.h>

ConfigWidget::ConfigWidget()
{
  QHBoxLayout *topLayout = new QHBoxLayout( this );

  QVBoxLayout *subLayout = new QVBoxLayout;
  
  topLayout->addLayout( subLayout );

  QHBoxLayout *buttonLayout = new QHBoxLayout;
  
  subLayout->addLayout( buttonLayout );
  
  m_pSaveEntryPushButton = new QPushButton( i18n( "Save Entry" ), this );
  
  buttonLayout->addWidget( m_pSaveEntryPushButton );
  
  m_pRemoveEntryPushButton = new QPushButton( i18n( "Remove Entry" ), this );
  
  buttonLayout->addWidget( m_pRemoveEntryPushButton );

  QGridLayout *topGrid = new QGridLayout( 3, 2 );
  
  subLayout->addLayout( topGrid );
  
  QLabel *label = new QLabel( i18n( "Name:" ), this );

  topGrid->addWidget( label, 0, 0 );
  
  m_pNameLineEdit = new QLineEdit( this );
  
  topGrid->addWidget( m_pNameLineEdit, 0, 1 );

  label = new QLabel( i18n( "Query:" ), this );
  
  topGrid->addWidget( label, 1, 0 );
  
  m_pQueryLineEdit = new QLineEdit( this );
  
  topGrid->addWidget( m_pQueryLineEdit, 1, 1 );

  label = new QLabel( i18n( "Keys:" ), this );
  
  topGrid->addWidget( label, 2, 0, Qt::AlignTop );

  QGridLayout *subGrid = new QGridLayout( 6, 2 );

  topGrid->addLayout( subGrid, 2, 1 );

  m_pKeyListBox = new QListBox( this );

  subGrid->addMultiCellWidget( m_pKeyListBox, 0, 3, 0, 1 );

  m_pKeyLineEdit = new QLineEdit( this );
  
  subGrid->addMultiCellWidget( m_pKeyLineEdit, 4, 4, 0, 1 );

  m_pAddKeyPushButton = new QPushButton( i18n( "Add Key" ), this );
  
  subGrid->addWidget( m_pAddKeyPushButton, 5, 0 );
  
  m_pRemoveKeyPushButton = new QPushButton( i18n( "Remove Key" ), this );

  subGrid->addWidget( m_pRemoveKeyPushButton, 5, 1 );

  m_pListView = new QListView( this );
  m_pListView->addColumn( i18n( "Name" ) );
  m_pListView->addColumn( i18n( "Keys" ) );
  m_pListView->addColumn( i18n( "Query" ) );

  m_pListView->setMinimumSize( m_pListView->sizeHint() );

  topLayout->addSpacing( 10 );
  
  topLayout->addWidget( m_pListView );

  QValueList<EngineCfg::SearchEntry> lstSearchEngines = EngineCfg::self()->searchEngines();
  QValueList<EngineCfg::SearchEntry>::ConstIterator it = lstSearchEngines.begin();
  QValueList<EngineCfg::SearchEntry>::ConstIterator end = lstSearchEngines.end();
  for (; it != end; ++it )
    saveEngine( *it );
  
  topLayout->setMargin( 2 );
  
  resize( topLayout->sizeHint() );
  
  m_pSaveEntryPushButton->setEnabled( false );
  m_pRemoveEntryPushButton->setEnabled( false );
  
  m_pAddKeyPushButton->setEnabled( false );
  m_pRemoveKeyPushButton->setEnabled( false );
  
  connect( m_pListView, SIGNAL( selectionChanged( QListViewItem * ) ),
           this, SLOT( slotSelectionChanged( QListViewItem * ) ) );

  connect( m_pSaveEntryPushButton, SIGNAL( clicked() ),
           this, SLOT( slotSaveEntry() ) );

  connect( m_pRemoveEntryPushButton, SIGNAL( clicked() ),
           this, SLOT( slotRemoveEntry() ) );
	   
  connect( m_pNameLineEdit, SIGNAL( textChanged( const QString & ) ),
           this, SLOT( slotTextChanged( const QString & ) ) );
	   
  connect( m_pKeyListBox, SIGNAL( selected( const QString & ) ),
           this, SLOT( slotKeySelected( const QString & ) ) );

  connect( m_pKeyListBox, SIGNAL( selected( const QString & ) ),
           m_pKeyLineEdit, SLOT( setText( const QString & ) ) );
	   
  connect( m_pKeyLineEdit, SIGNAL( textChanged( const QString & ) ),
           this, SLOT( slotKeyTextChanged( const QString & ) ) );

  connect( m_pAddKeyPushButton, SIGNAL( clicked() ),
           this, SLOT( slotAddKey() ) );
	   
  connect( m_pRemoveKeyPushButton, SIGNAL( clicked() ),
           this, SLOT( slotRemoveKey() ) );
	   
  connect( m_pQueryLineEdit, SIGNAL( textChanged( const QString & ) ),
           this, SLOT( slotQueryTextChanged() ) );
}

ConfigWidget::~ConfigWidget()
{
}

void ConfigWidget::slotSelectionChanged( QListViewItem *item )
{
  m_pRemoveEntryPushButton->setEnabled( true );

  EngineCfg::SearchEntry entry = EngineCfg::self()->searchEntryByName( item->text( 0 ) );

  m_strCurrentName = entry.m_strName;

  m_pNameLineEdit->setText( entry.m_strName );
  
  m_pQueryLineEdit->setText( entry.m_strQuery );
  
  m_pKeyListBox->clear();
  m_pKeyListBox->insertStringList( entry.m_lstKeys );
  
  m_pKeyLineEdit->clear();

  m_pAddKeyPushButton->setEnabled( false );
  m_pRemoveKeyPushButton->setEnabled( false );
}

void ConfigWidget::slotSaveEntry()
{
  EngineCfg::SearchEntry e;
  
  e.m_strName = m_pNameLineEdit->text();
 
  for (uint i = 0; i < m_pKeyListBox->count(); i++)
    e.m_lstKeys.append( m_pKeyListBox->text( i ) );

  e.m_strQuery = m_pQueryLineEdit->text();
  
  saveEngine( e );
  
  EngineCfg::self()->saveSearchEngine( e );
  
  m_pSaveEntryPushButton->setEnabled( false );
}

void ConfigWidget::slotRemoveEntry()
{
  m_pRemoveEntryPushButton->setEnabled( false );
  m_pSaveEntryPushButton->setEnabled( false );

  QListViewItem *current = m_pListView->selectedItem();
  if ( current && current->text( 0 ) == m_pNameLineEdit->text() )
    m_pListView->removeItem( current );

  EngineCfg::self()->removeSearchEngine( m_pNameLineEdit->text() );
  
  m_pNameLineEdit->clear();
  m_pQueryLineEdit->clear();
  m_pKeyListBox->clear();
  m_pKeyLineEdit->clear();
}

void ConfigWidget::slotTextChanged( const QString &text )
{
  m_pSaveEntryPushButton->setEnabled( !text.isEmpty() && 
                                      text != m_strCurrentName &&
				      !m_pQueryLineEdit->text().isEmpty() &&
				      m_pKeyListBox->count() > 0 );
}

void ConfigWidget::slotKeySelected( const QString &text )
{
  m_pAddKeyPushButton->setEnabled( false );
  m_pRemoveKeyPushButton->setEnabled( true );
  m_strCurrentKey = text;
  m_pKeyLineEdit->setText( text );
}

void ConfigWidget::slotKeyTextChanged( const QString &text )
{
  m_pAddKeyPushButton->setEnabled( !text.isEmpty() && text != m_strCurrentKey );

  m_pRemoveKeyPushButton->setEnabled( false );

  for ( uint i = 0; i < m_pKeyListBox->count(); i++ )
    if ( m_pKeyListBox->text( i ) == text )
    {
      m_pRemoveKeyPushButton->setEnabled( true );
      m_pAddKeyPushButton->setEnabled( false );
      break;
    }
}

void ConfigWidget::slotAddKey()
{
  m_pKeyListBox->insertItem( m_pKeyLineEdit->text() );
  slotKeyTextChanged( m_pKeyLineEdit->text() );
  slotTextChanged( m_pNameLineEdit->text() );
}

void ConfigWidget::slotRemoveKey()
{
  m_pRemoveKeyPushButton->setEnabled( false );
  
  QString key = m_pKeyLineEdit->text();
  
  m_pKeyLineEdit->clear();
  
  for ( uint i = 0; i < m_pKeyListBox->count(); i++ )
    if ( m_pKeyListBox->text( i ) == key )
    {
      m_pKeyListBox->removeItem( i );
      break;
    }

  m_strCurrentKey = QString::null;
}

void ConfigWidget::slotQueryTextChanged()
{
  slotTextChanged( m_pNameLineEdit->text() );
}

void ConfigWidget::saveEngine( const EngineCfg::SearchEntry &e )
{
  QListViewItem *item = 0L;

  QListViewItemIterator it( m_pListView );
  for (; it.current(); ++it )
    if ( it.current()->text( 0 ) == e.m_strName )
    {
      item = it.current();
      break;
    }

  if ( !item )
    item = new QListViewItem( m_pListView );

  QString keystr;
    
  QStringList::ConstIterator sIt = e.m_lstKeys.begin();
  QStringList::ConstIterator sEnd = e.m_lstKeys.end();
  for (; sIt != sEnd; ++sIt )
    keystr += *sIt + ',';
    
  item->setText( 0, e.m_strName );
  item->setText( 1, keystr );
  item->setText( 2, e.m_strQuery );
}

#include "configwidget.moc"
