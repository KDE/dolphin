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
#include <qlineedit.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qheader.h>

#include <klistview.h>
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
    if ( cfg.hasGroup( "Profile" ) )
    {
      cfg.setGroup( "Profile" );
      if ( cfg.hasKey( "Name" ) )
        profileName = cfg.readEntry( "Name" );

      mapProfiles.insert( profileName, *pIt );
    }
  }

  return mapProfiles;
}

KonqProfileDlg::KonqProfileDlg( KonqViewManager *manager, const QString & preselectProfile, QWidget *parent )
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

  m_pListView = new KListView( this );
  m_pListView->setAllColumnsShowFocus(true);
  m_pListView->header()->hide();
  m_pListView->addColumn("");

  m_pGrid->addMultiCellWidget( m_pListView, 2, 6, 0, N_BUTTONS-1 );

  QMap<QString,QString>::ConstIterator eIt = m_mapEntries.begin();
  QMap<QString,QString>::ConstIterator eEnd = m_mapEntries.end();
  for (; eIt != eEnd; ++eIt )
  {
    QListViewItem *item = new QListViewItem( m_pListView, eIt.key() );
    QString filename = eIt.data().mid( eIt.data().findRev( '/' ) + 1 );
    kdDebug() << filename << endl;
    if ( filename == preselectProfile )
    {
      m_pProfileNameLineEdit->setText( eIt.key() );
      m_pListView->setSelected( item, true );
    }
  }

  m_pListView->setMinimumSize( m_pListView->sizeHint() );

  KGlobal::config()->setGroup("Settings");
  m_cbSaveURLs = new QCheckBox( i18n("Save URLs in profile"), this );
  m_cbSaveURLs->setChecked( KGlobal::config()->readBoolEntry("SaveURLInProfile",true) );
  m_pGrid->addMultiCellWidget( m_cbSaveURLs, 7, 7, 0, N_BUTTONS-1 );

  m_cbSaveSize = new QCheckBox( i18n("Save window size in profile"), this );
  m_cbSaveSize->setChecked( KGlobal::config()->readBoolEntry("SaveWindowSizeInProfile",false) );
  m_pGrid->addMultiCellWidget( m_cbSaveSize, 8, 8, 0, N_BUTTONS-1 );

  m_pSaveButton = new QPushButton( i18n( "Save" ), this );
  m_pSaveButton->setEnabled( !m_pProfileNameLineEdit->text().isEmpty() );
  m_pSaveButton->setDefault( true );

  m_pGrid->addWidget( m_pSaveButton, 9, 0 );

  m_pDeleteProfileButton = new QPushButton( i18n( "Delete Selected Profile" ), this );

  m_pGrid->addWidget( m_pDeleteProfileButton, 9, 1 );

  m_pRenameProfileButton = new QPushButton( i18n( "Rename Selected Profile" ), this );

  m_pGrid->addWidget( m_pRenameProfileButton, 9, 2 );

  m_pCloseButton = new QPushButton( i18n( "Close" ), this );

  m_pGrid->addWidget( m_pCloseButton, 9, 3 );

  connect( m_pListView, SIGNAL( selectionChanged( QListViewItem * ) ),
           this, SLOT( slotSelectionChanged( QListViewItem * ) ) );

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

  m_pDeleteProfileButton->setEnabled( m_pListView->selectedItem ()!=0 );
  m_pRenameProfileButton->setEnabled( m_pListView->selectedItem ()!=0 );

  resize( sizeHint() );
}

KonqProfileDlg::~KonqProfileDlg()
{
  KConfig * config = KGlobal::config();
  config->setGroup("Settings");
  config->writeEntry("SaveURLInProfile", m_cbSaveURLs->isChecked());
  config->writeEntry("SaveWindowSizeInProfile", m_cbSaveSize->isChecked());
}

void KonqProfileDlg::slotSave()
{
  QString name = KIO::encodeFileName( m_pProfileNameLineEdit->text() ); // in case of '/'

  // Reuse filename of existing item, if any
  if ( m_pListView->selectedItem() )
  {
    QMap<QString, QString>::Iterator it = m_mapEntries.find( m_pListView->selectedItem()->text(0) );
    if ( it != m_mapEntries.end() )
    {
      QFileInfo info( it.data() );
      name = info.baseName();
    }
  }

  kdDebug(1202) << "Saving as " << name << endl;
  m_pViewManager->saveViewProfile( name, m_pProfileNameLineEdit->text(),
                                   m_cbSaveURLs->isChecked(), m_cbSaveSize->isChecked() );

  accept();
}

void KonqProfileDlg::slotDelete()
{
    if(!m_pListView->selectedItem())
        return;
  QMap<QString, QString>::Iterator it = m_mapEntries.find( m_pListView->selectedItem()->text(0) );

  if ( it != m_mapEntries.end() && QFile::remove( it.data() ) )
  {
    m_pListView->removeItem( m_pListView->selectedItem() );
    m_mapEntries.remove( it );
  }
  m_pDeleteProfileButton->setEnabled( m_pListView->selectedItem ()!=0 );
  m_pRenameProfileButton->setEnabled( m_pListView->selectedItem ()!=0 );
}

void KonqProfileDlg::slotRename()
{
 if(!m_pListView->selectedItem())
        return;
    QString currentText = m_pListView->selectedItem()->text(0);
  QMap<QString, QString>::Iterator it = m_mapEntries.find( currentText );

  if ( it != m_mapEntries.end() )
  {
    KLineEditDlg dlg( i18n("Rename profile '%1'").arg(currentText), currentText, this );
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
        m_pListView->selectedItem()->setText(0, newName);
        m_pProfileNameLineEdit->setText( newName );
      }
    }
  }
}

void KonqProfileDlg::slotSelectionChanged( QListViewItem * item )
{
    m_pProfileNameLineEdit->setText( item ? item->text(0) : QString::null );
}

void KonqProfileDlg::slotTextChanged( const QString & text )
{
  m_pSaveButton->setEnabled( !text.isEmpty() );

  // If we type the name of a profile, select it in the list

  bool itemSelected = false;
  for ( QListViewItem * item = m_pListView->firstChild() ; item ; item = item->nextSibling() )
      if ( item->text(0) == text /*only full text, not partial*/ )
      {
          itemSelected = true;
          m_pListView->setSelected( item, true );
          break;
      }

  if ( !itemSelected ) // otherwise, clear selection
    m_pListView->clearSelection();

  m_pDeleteProfileButton->setEnabled( itemSelected );
  m_pRenameProfileButton->setEnabled( itemSelected );
}

#include "konq_profiledlg.moc"
