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
#include "konq_factory.h"

#include <qcheckbox.h>
#include <qdir.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include <kglobal.h>
#include <kio/global.h>
#include <kstddirs.h>
#include <klocale.h>
#include <ksimpleconfig.h>

QMap<QString,QString> KonqProfileDlg::readAllProfiles()
{
  QMap<QString,QString> mapProfiles;

  QStringList profiles = KonqFactory::instance()->dirs()->findAllResources( "data", "konqueror/profiles/*", false, true );
  QStringList::ConstIterator pIt = profiles.begin();
  QStringList::ConstIterator pEnd = profiles.end();
  for (; pIt != pEnd; ++pIt )
  {
    QFileInfo info( *pIt );
    QString profileName = KIO::decodeFileName( info.baseName() );
    KSimpleConfig cfg( *pIt, true );
    cfg.setGroup( "Profile" );
    if ( cfg.hasKey( "Name" ) )
      profileName = cfg.readEntry( "Name" );

    mapProfiles.insert( profileName, *pIt );
  }

  return mapProfiles;
}

KonqProfileDlg::KonqProfileDlg( KonqViewManager *manager, QWidget *parent )
: KDialog( parent, 0L, true )
{
  m_pViewManager = manager;

  setCaption( i18n( "Profile Management" ) );

  m_mapEntries = readAllProfiles();

  m_pGrid = new QGridLayout( this, 9, 3, KDialog::marginHint(), KDialog::spacingHint() );

  m_pGrid->addMultiCellWidget( new QLabel( i18n( "Enter Profile Name :" ), this ), 0, 0, 0, 2 );

  m_pProfileNameLineEdit = new QLineEdit( this );
  m_pProfileNameLineEdit->setFocus();

  m_pGrid->addMultiCellWidget( m_pProfileNameLineEdit, 1, 1, 0, 2 );

  m_pListBox = new QListBox( this );

  m_pGrid->addMultiCellWidget( m_pListBox, 2, 6, 0, 2 );

  QMap<QString,QString>::ConstIterator eIt = m_mapEntries.begin();
  QMap<QString,QString>::ConstIterator eEnd = m_mapEntries.end();
  for (; eIt != eEnd; ++eIt )
    m_pListBox->insertItem( eIt.key() );

  m_pListBox->setMinimumSize( m_pListBox->sizeHint() );

  m_cbSaveURLs = new QCheckBox( i18n("Save URLs in profile"), this );
  m_cbSaveURLs->setChecked( true ); // not saving URLs is tricky, because it means
  // that one shouldn't apply the profile if the current URL can't be opened into it...
  m_pGrid->addMultiCellWidget( m_cbSaveURLs, 7, 7, 0, 2 );

  m_pSaveButton = new QPushButton( i18n( "Save" ), this );
  m_pSaveButton->setEnabled( false );
  m_pSaveButton->setDefault( true );

  m_pGrid->addWidget( m_pSaveButton, 8, 0 );

  m_pDeleteProfileButton = new QPushButton( i18n( "Delete Selected Profile" ), this );

  m_pGrid->addWidget( m_pDeleteProfileButton, 8, 1 );

  m_pCloseButton = new QPushButton( i18n( "Close" ), this );

  m_pGrid->addWidget( m_pCloseButton, 8, 2 );

  connect( m_pListBox, SIGNAL( selected( const QString & ) ),
           m_pProfileNameLineEdit, SLOT( setText( const QString & ) ) );

  connect( m_pProfileNameLineEdit, SIGNAL( textChanged( const QString & ) ),
           this, SLOT( slotEnableSave( const QString & ) ) );
	
  connect( m_pSaveButton, SIGNAL( clicked() ),
           this, SLOT( slotSave() ) );
	
  connect( m_pDeleteProfileButton, SIGNAL( clicked() ),
           this, SLOT( slotDelete() ) );
	
  connect( m_pCloseButton, SIGNAL( clicked() ),
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
  QString name = KIO::encodeFileName( m_pProfileNameLineEdit->text() ); // in case of '/'
  QString fileName = locateLocal( "data", QString::fromLatin1( "konqueror/profiles/" ) +
                                          name, KonqFactory::instance() );

  if ( QFile::exists( fileName ) )
    QFile::remove( fileName );

  KSimpleConfig cfg( fileName );
  cfg.setGroup( "Profile" );
  m_pViewManager->saveViewProfile( cfg, m_cbSaveURLs->isChecked() );

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
