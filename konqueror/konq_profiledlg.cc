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
#include "konq_mainwindow.h"
#include "konq_factory.h"

#include <qcheckbox.h>
#include <qdir.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include <kglobal.h>
#include <kdebug.h>
#include <klineeditdlg.h>
#include <kio/global.h>
#include <kstddirs.h>
#include <klocale.h>
#include <ksimpleconfig.h>

QMap<QString,QString> KonqProfileDlg::readAllProfiles()
{
  QMap<QString,QString> mapProfiles;

  QStringList profiles = KGlobal::dirs()->findAllResources( "data", "konqueror/profiles/*", false, true );
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

#define N_BUTTONS 4
  m_pGrid = new QGridLayout( this, 10, N_BUTTONS, KDialog::marginHint(), KDialog::spacingHint() );

  m_pGrid->addMultiCellWidget( new QLabel( i18n( "Enter Profile Name :" ), this ), 0, 0, 0, N_BUTTONS-1 );

  m_pProfileNameLineEdit = new QLineEdit( this );
  m_pProfileNameLineEdit->setFocus();

  m_pGrid->addMultiCellWidget( m_pProfileNameLineEdit, 1, 1, 0, N_BUTTONS-1 );

  m_pListBox = new QListBox( this );

  m_pGrid->addMultiCellWidget( m_pListBox, 2, 6, 0, N_BUTTONS-1 );

  QMap<QString,QString>::ConstIterator eIt = m_mapEntries.begin();
  QMap<QString,QString>::ConstIterator eEnd = m_mapEntries.end();
  for (; eIt != eEnd; ++eIt )
    m_pListBox->insertItem( eIt.key() );

  m_pListBox->setMinimumSize( m_pListBox->sizeHint() );

  m_cbSaveURLs = new QCheckBox( i18n("Save URLs in profile"), this );
  m_cbSaveURLs->setChecked( true ); // not saving URLs is tricky, because it means
  // that one shouldn't apply the profile if the current URL can't be opened into it...
  m_pGrid->addMultiCellWidget( m_cbSaveURLs, 7, 7, 0, N_BUTTONS-1 );

  m_cbSaveSize = new QCheckBox( i18n("Save window size in profile"), this );
  m_cbSaveSize->setChecked( false );
  m_pGrid->addMultiCellWidget( m_cbSaveSize, 8, 8, 0, N_BUTTONS-1 );

  m_pSaveButton = new QPushButton( i18n( "Save" ), this );
  m_pSaveButton->setEnabled( false );
  m_pSaveButton->setDefault( true );

  m_pGrid->addWidget( m_pSaveButton, 9, 0 );

  m_pDeleteProfileButton = new QPushButton( i18n( "Delete Selected Profile" ), this );

  m_pGrid->addWidget( m_pDeleteProfileButton, 9, 1 );

  m_pRenameProfileButton = new QPushButton( i18n( "Rename Selected Profile" ), this );

  m_pGrid->addWidget( m_pRenameProfileButton, 9, 2 );

  m_pCloseButton = new QPushButton( i18n( "Close" ), this );

  m_pGrid->addWidget( m_pCloseButton, 9, 3 );

  connect( m_pListBox, SIGNAL( highlighted( const QString & ) ),
           m_pProfileNameLineEdit, SLOT( setText( const QString & ) ) );

  connect( m_pProfileNameLineEdit, SIGNAL( textChanged( const QString & ) ),
           this, SLOT( slotTextChanged( const QString & ) ) );

  connect( m_pSaveButton, SIGNAL( clicked() ),
           this, SLOT( slotSave() ) );

  connect( m_pDeleteProfileButton, SIGNAL( clicked() ),
           this, SLOT( slotDelete() ) );

  connect( m_pRenameProfileButton, SIGNAL( clicked() ),
           this, SLOT( slotRename() ) );

  connect( m_pCloseButton, SIGNAL( clicked() ),
           this, SLOT( accept() ) );

  resize( sizeHint() );
}

KonqProfileDlg::~KonqProfileDlg()
{
}

void KonqProfileDlg::slotSave()
{
  QString name = KIO::encodeFileName( m_pProfileNameLineEdit->text() ); // in case of '/'

  // Reuse filename of existing item, if any
  if ( m_pListBox->currentItem() != -1 )
  {
    QMap<QString, QString>::Iterator it = m_mapEntries.find( m_pListBox->currentText() );
    if ( it != m_mapEntries.end() )
    {
      QFileInfo info( it.data() );
      name = info.baseName();
    }
  }

  m_pViewManager->saveViewProfile( name, m_pProfileNameLineEdit->text(),
                                   m_cbSaveURLs->isChecked(), m_cbSaveSize->isChecked() );

  accept();
}

void KonqProfileDlg::slotDelete()
{
  QMap<QString, QString>::Iterator it = m_mapEntries.find( m_pListBox->currentText() );

  if ( it != m_mapEntries.end() && QFile::remove( it.data() ) )
  {
    m_pListBox->removeItem( m_pListBox->currentItem() );
    m_mapEntries.remove( it );
  }
}

void KonqProfileDlg::slotRename()
{
  QMap<QString, QString>::Iterator it = m_mapEntries.find( m_pListBox->currentText() );

  if ( it != m_mapEntries.end() )
  {
    KLineEditDlg dlg( i18n("Rename profile '%1'").arg(m_pListBox->currentText()), m_pListBox->currentText(), this );
    dlg.setCaption( i18n("Rename profile") );
    if ( dlg.exec() )
    {
      QString newName = dlg.text();
      if (!newName.isEmpty())
      {
        QString fileName = it.data();
        KSimpleConfig cfg( fileName );
        cfg.setGroup( "Profile" );
        cfg.writeEntry( "Name", newName );
        cfg.sync();
        // Didn't find how to change a key...
        m_mapEntries.remove( it );
        m_mapEntries.insert( newName, fileName );
        m_pListBox->changeItem( newName, m_pListBox->currentItem() );
        m_pProfileNameLineEdit->setText( newName );
      }
    }
  }
}

void KonqProfileDlg::slotTextChanged( const QString & text )
{
  m_pSaveButton->setEnabled( !text.isEmpty() );

  // If we type the name of a profile, select it in the list
  QListBoxItem * item = m_pListBox->findItem( text );
  bool itemSelected = true;
  if ( item && item->text() == text /*only full text, not partial*/ )
    m_pListBox->setSelected( item, true );
  else
  {
    // otherwise, clear selection
    m_pListBox->clearSelection();
    itemSelected = false;
  }
  m_pDeleteProfileButton->setEnabled( itemSelected );
  m_pRenameProfileButton->setEnabled( itemSelected );
}

#include "konq_profiledlg.moc"
