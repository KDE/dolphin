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

#include "konq_profiledlg.h"
#include "konq_viewmgr.h"

#include <qdir.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <ksimpleconfig.h>

KonqProfileDlg::KonqProfileDlg( KonqViewManager *manager, QWidget *parent )
: QDialog( parent, 0L, true )
{
  m_pViewManager = manager;

  setCaption( i18n( "Konqueror: Profile Management" ) );
  
  QStringList dirs = KGlobal::dirs()->findDirs( "data", "konqueror/profiles/" );
  QStringList::ConstIterator dirIt = dirs.begin();
  QStringList::ConstIterator dirEnd = dirs.end();
  for (; dirIt != dirEnd; ++dirIt )
  {
    QDir dir( *dirIt );
    const QFileInfoList *entries = dir.entryInfoList( QDir::Files );
    
    QFileInfoListIterator eIt( *entries );
    for (; eIt.current(); ++eIt )
      if ( eIt.current()->isWritable() )
        m_mapEntries.insert( eIt.current()->baseName(), eIt.current()->absFilePath() );
  }
  
  m_pGrid = new QGridLayout( this, 8, 3 );
  
  m_pGrid->addMultiCellWidget( new QLabel( i18n( "Enter Profile Name :" ), this ), 0, 0, 0, 2 );
  
  m_pProfileNameLineEdit = new QLineEdit( this );
  
  m_pGrid->addMultiCellWidget( m_pProfileNameLineEdit, 1, 1, 0, 2 );
  
  m_pListBox = new QListBox( this );
  
  m_pGrid->addMultiCellWidget( m_pListBox, 2, 6, 0, 2 );
  
  QMap<QString,QString>::ConstIterator eIt = m_mapEntries.begin();
  QMap<QString,QString>::ConstIterator eEnd = m_mapEntries.end();
  for (; eIt != eEnd; ++eIt )
    m_pListBox->insertItem( eIt.key() );

  m_pListBox->setMinimumSize( m_pListBox->sizeHint() );
  
  m_pSaveButton = new QPushButton( i18n( "Save" ), this );
  m_pSaveButton->setEnabled( false );
  
  m_pGrid->addWidget( m_pSaveButton, 7, 0 );
  
  m_pDeleteProfileButton = new QPushButton( i18n( "Delete Selected Profile" ), this );
  
  m_pGrid->addWidget( m_pDeleteProfileButton, 7, 1 );
  
  m_pCancelButton = new QPushButton( i18n( "Cancel" ), this );
  
  m_pGrid->addWidget( m_pCancelButton, 7, 2 );

  connect( m_pListBox, SIGNAL( selected( const QString & ) ),
           m_pProfileNameLineEdit, SLOT( setText( const QString & ) ) );  

  connect( m_pProfileNameLineEdit, SIGNAL( textChanged( const QString & ) ),
           this, SLOT( slotEnableSave( const QString & ) ) );
	   
  connect( m_pSaveButton, SIGNAL( clicked() ),
           this, SLOT( slotSave() ) );
	   
  connect( m_pDeleteProfileButton, SIGNAL( clicked() ),
           this, SLOT( slotDelete() ) );
	   
  connect( m_pCancelButton, SIGNAL( clicked() ),
           this, SLOT( accept() ) );

  resize( sizeHint() );
}

KonqProfileDlg::~KonqProfileDlg()
{
}

void KonqProfileDlg::slotEnableSave( const QString &text )
{
  m_pSaveButton->setEnabled( !text.isEmpty() );
}

void KonqProfileDlg::slotSave()
{
  QString fileName = locateLocal( "data", QString::fromLatin1( "konqueror/profiles/" ) +
                                          m_pProfileNameLineEdit->text() );
					  
  if ( QFile::exists( fileName ) )
    QFile::remove( fileName );

  KSimpleConfig cfg( fileName );
  cfg.setGroup( "Profile" );
  m_pViewManager->saveViewProfile( cfg );

  accept();
}

void KonqProfileDlg::slotDelete()
{
  QMap<QString, QString>::Iterator it = m_mapEntries.find( m_pListBox->text( m_pListBox->currentItem() ) );
  
  if ( it != m_mapEntries.end() && QFile::remove( it.data() ) )
  {
    m_pListBox->removeItem( m_pListBox->currentItem() );
    m_mapEntries.remove( it );
  }
  
}

#include "konq_profiledlg.moc"
