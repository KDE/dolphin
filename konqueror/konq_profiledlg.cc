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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "konq_profiledlg.h"
#include "konq_viewmgr.h"

#include <qcheckbox.h>
#include <qdir.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qheader.h>
#include <qlineedit.h>

#include <klistview.h>
#include <kdebug.h>
#include <kstdguiitem.h>
#include <kio/global.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <ksimpleconfig.h>
#include <kpushbutton.h>

KonqProfileMap KonqProfileDlg::readAllProfiles()
{
  KonqProfileMap mapProfiles;

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

KonqProfileItem::KonqProfileItem( KListView *parent, const QString & text )
    : QListViewItem( parent, text ), m_profileName( text )
{
}

KonqProfileDlg::KonqProfileDlg( KonqViewManager *manager, const QString & preselectProfile, QWidget *parent )
: KDialog( parent, 0L, true )
{
  m_pViewManager = manager;

  setCaption( i18n( "Profile Management" ) );

#define N_BUTTONS 4
  m_pGrid = new QGridLayout( this, 10, N_BUTTONS, KDialog::marginHint(), KDialog::spacingHint() );

  QLabel *lblName = new QLabel( i18n(  "&Profile name:" ), this );
  m_pGrid->addMultiCellWidget( lblName, 0, 0, 0, N_BUTTONS-1 );

  m_pProfileNameLineEdit = new QLineEdit( this );
  m_pProfileNameLineEdit->setFocus();

  lblName->setBuddy( m_pProfileNameLineEdit );

  m_pGrid->addMultiCellWidget( m_pProfileNameLineEdit, 1, 1, 0, N_BUTTONS-1 );

  m_pListView = new KListView( this );
  m_pListView->setAllColumnsShowFocus(true);
  m_pListView->header()->hide();
  m_pListView->addColumn("");
  m_pListView->setRenameable( 0 );

  m_pGrid->addMultiCellWidget( m_pListView, 2, 6, 0, N_BUTTONS-1 );

  connect( m_pListView, SIGNAL( itemRenamed( QListViewItem * ) ),
            SLOT( slotItemRenamed( QListViewItem * ) ) );

  loadAllProfiles(preselectProfile);
  m_pListView->setMinimumSize( m_pListView->sizeHint() );

  KGlobal::config()->setGroup("Settings");
  m_cbSaveURLs = new QCheckBox( i18n("Save &URLs in profile"), this );
  m_cbSaveURLs->setChecked( KGlobal::config()->readBoolEntry("SaveURLInProfile",true) );
  m_pGrid->addMultiCellWidget( m_cbSaveURLs, 7, 7, 0, N_BUTTONS-1 );

  m_cbSaveSize = new QCheckBox( i18n("Save &window size in profile"), this );
  m_cbSaveSize->setChecked( KGlobal::config()->readBoolEntry("SaveWindowSizeInProfile",false) );
  m_pGrid->addMultiCellWidget( m_cbSaveSize, 8, 8, 0, N_BUTTONS-1 );

  m_pSaveButton = new KPushButton( KStdGuiItem::save(), this );
  m_pSaveButton->setEnabled( !m_pProfileNameLineEdit->text().isEmpty() );
  m_pSaveButton->setDefault( true );

  m_pGrid->addWidget( m_pSaveButton, 9, 0 );

  m_pDeleteProfileButton = new KPushButton( KGuiItem( i18n( "&Delete Profile" ), "editdelete"), this );

  m_pGrid->addWidget( m_pDeleteProfileButton, 9, 1 );

  m_pRenameProfileButton = new KPushButton( i18n( "&Rename Profile" ), this );

  m_pGrid->addWidget( m_pRenameProfileButton, 9, 2 );

  m_pCloseButton = new KPushButton( KStdGuiItem::close(), this );

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

void KonqProfileDlg::loadAllProfiles(const QString & preselectProfile)
{
    bool profileFound = false;
    m_mapEntries.clear();
    m_pListView->clear();
    m_mapEntries = readAllProfiles();
    KonqProfileMap::ConstIterator eIt = m_mapEntries.begin();
    KonqProfileMap::ConstIterator eEnd = m_mapEntries.end();
    for (; eIt != eEnd; ++eIt )
    {
        QListViewItem *item = new KonqProfileItem( m_pListView, eIt.key() );
        QString filename = eIt.data().mid( eIt.data().findRev( '/' ) + 1 );
        kdDebug(1202) << filename << endl;
        if ( filename == preselectProfile )
        {
            profileFound = true;
            m_pProfileNameLineEdit->setText( eIt.key() );
            m_pListView->setSelected( item, true );
        }
    }
    if (!profileFound)
        m_pProfileNameLineEdit->setText( preselectProfile);
}

void KonqProfileDlg::slotSave()
{
  QString name = KIO::encodeFileName( m_pProfileNameLineEdit->text() ); // in case of '/'

  // Reuse filename of existing item, if any
  if ( m_pListView->selectedItem() )
  {
    KonqProfileMap::Iterator it = m_mapEntries.find( m_pListView->selectedItem()->text(0) );
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
  KonqProfileMap::Iterator it = m_mapEntries.find( m_pListView->selectedItem()->text(0) );

  if ( it != m_mapEntries.end() && QFile::remove( it.data() ) )
      loadAllProfiles();
  m_pDeleteProfileButton->setEnabled( m_pListView->selectedItem ()!=0 );
  m_pRenameProfileButton->setEnabled( m_pListView->selectedItem ()!=0 );
}

void KonqProfileDlg::slotRename()
{
  QListViewItem *item = m_pListView->selectedItem();

  if ( item )
    m_pListView->rename( item, 0 );
}

void KonqProfileDlg::slotItemRenamed( QListViewItem * item )
{
  KonqProfileItem * profileItem = static_cast<KonqProfileItem *>( item );

  QString newName = profileItem->text(0);
  QString oldName = profileItem->m_profileName;

  if (!newName.isEmpty())
  {
    KonqProfileMap::ConstIterator it = m_mapEntries.find( oldName );

    if ( it != m_mapEntries.end() )
    {
      QString fileName = it.data();
      KSimpleConfig cfg( fileName );
      cfg.setGroup( "Profile" );
      cfg.writeEntry( "Name", newName );
      cfg.sync();
      // Didn't find how to change a key...
      m_mapEntries.remove( oldName );
      m_mapEntries.insert( newName, fileName );
      m_pProfileNameLineEdit->setText( newName );
      profileItem->m_profileName = newName;
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
  QListViewItem * item;

  for ( item = m_pListView->firstChild() ; item ; item = item->nextSibling() )
      if ( item->text(0) == text /*only full text, not partial*/ )
      {
          itemSelected = true;
          m_pListView->setSelected( item, true );
          break;
      }

  if ( !itemSelected ) // otherwise, clear selection
    m_pListView->clearSelection();

  if ( itemSelected )
  {
    QFileInfo fi( m_mapEntries[ item->text( 0 ) ] );
    itemSelected = itemSelected && fi.isWritable();
  }

  m_pDeleteProfileButton->setEnabled( itemSelected );
  m_pRenameProfileButton->setEnabled( itemSelected );
}

#include "konq_profiledlg.moc"
