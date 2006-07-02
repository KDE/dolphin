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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include "konq_profiledlg.h"
#include "konq_viewmgr.h"
#include "konq_settingsxt.h"

#include <QCheckBox>
#include <QDir>
#include <kvbox.h>
#include <QLabel>
#include <q3header.h>
#include <QLineEdit>

#include <k3listview.h>
#include <kdebug.h>
#include <kstdguiitem.h>
#include <kio/global.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <ksimpleconfig.h>
#include <kseparator.h>
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

KonqProfileItem::KonqProfileItem( K3ListView *parent, const QString & text )
    : Q3ListViewItem( parent, text ), m_profileName( text )
{
}

#define BTN_RENAME KDialog::User1
#define BTN_DELETE KDialog::User2
#define BTN_SAVE   KDialog::User3

KonqProfileDlg::KonqProfileDlg( KonqViewManager *manager, const QString & preselectProfile, QWidget *parent )
: KDialog( parent )
{
  setObjectName( "konq_profile_dialog" );
  setModal( true );
  setCaption( i18n( "Profile Management" ) );
  setButtons( Close | BTN_RENAME | BTN_DELETE | BTN_SAVE );
  setDefaultButton( BTN_SAVE );
  showButtonSeparator( true );
  setButtonGuiItem( BTN_RENAME, KGuiItem( i18n( "&Rename Profile" ) ) );
  setButtonGuiItem( BTN_DELETE, KGuiItem( i18n( "&Delete Profile" ), "editdelete" ) );
  setButtonGuiItem( BTN_SAVE, KStdGuiItem::save() );

  m_pViewManager = manager;

  KVBox* box = new KVBox( this );
  box->setSpacing( KDialog::spacingHint() );
  setMainWidget( box );

  QLabel *lblName = new QLabel( i18n(  "&Profile name:" ), box );

  m_pProfileNameLineEdit = new QLineEdit( box );
  m_pProfileNameLineEdit->setFocus();

  lblName->setBuddy( m_pProfileNameLineEdit );

  m_pListView = new K3ListView( box );
  m_pListView->setAllColumnsShowFocus(true);
  m_pListView->header()->hide();
  m_pListView->addColumn("");
  m_pListView->setRenameable( 0 );

  box->setStretchFactor( m_pListView, 1 );

  connect( m_pListView, SIGNAL( itemRenamed( Q3ListViewItem * ) ),
            SLOT( slotItemRenamed( Q3ListViewItem * ) ) );

  loadAllProfiles( preselectProfile );
  m_pListView->setMinimumSize( m_pListView->sizeHint() );

  m_cbSaveURLs = new QCheckBox( i18n("Save &URLs in profile"), box );
  m_cbSaveURLs->setChecked( KonqSettings::saveURLInProfile() );

  m_cbSaveSize = new QCheckBox( i18n("Save &window size in profile"), box );
  m_cbSaveSize->setChecked( KonqSettings::saveWindowSizeInProfile() );

  connect( m_pListView, SIGNAL( selectionChanged( Q3ListViewItem * ) ),
           this, SLOT( slotSelectionChanged( Q3ListViewItem * ) ) );

  connect( m_pProfileNameLineEdit, SIGNAL( textChanged( const QString & ) ),
           this, SLOT( slotTextChanged( const QString & ) ) );

  enableButton( BTN_RENAME, m_pListView->selectedItem ()!=0 );
  enableButton( BTN_DELETE, m_pListView->selectedItem ()!=0 );

  resize( sizeHint() );
}

KonqProfileDlg::~KonqProfileDlg()
{
  KonqSettings::setSaveURLInProfile( m_cbSaveURLs->isChecked() );
  KonqSettings::setSaveWindowSizeInProfile( m_cbSaveSize->isChecked() );
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
        Q3ListViewItem *item = new KonqProfileItem( m_pListView, eIt.key() );
        QString filename = eIt.value().mid( eIt.value().lastIndexOf( '/' ) + 1 );
        kDebug(1202) << filename << endl;
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

void KonqProfileDlg::slotUser3() // Save button
{
  QString name = KIO::encodeFileName( m_pProfileNameLineEdit->text() ); // in case of '/'

  // Reuse filename of existing item, if any
  if ( m_pListView->selectedItem() )
  {
    KonqProfileMap::Iterator it = m_mapEntries.find( m_pListView->selectedItem()->text(0) );
    if ( it != m_mapEntries.end() )
    {
      QFileInfo info( it.value() );
      name = info.baseName();
    }
  }

  kDebug(1202) << "Saving as " << name << endl;
  m_pViewManager->saveViewProfile( name, m_pProfileNameLineEdit->text(),
            m_cbSaveURLs->isChecked(), m_cbSaveSize->isChecked() );

  accept();
}

void KonqProfileDlg::slotUser2() // Delete button
{
    if(!m_pListView->selectedItem())
        return;
  KonqProfileMap::Iterator it = m_mapEntries.find( m_pListView->selectedItem()->text(0) );

  if ( it != m_mapEntries.end() && QFile::remove( it.value() ) )
      loadAllProfiles();

  enableButton( BTN_RENAME, m_pListView->selectedItem() != 0 );
  enableButton( BTN_DELETE, m_pListView->selectedItem() != 0 );
}

void KonqProfileDlg::slotUser1() // Rename button
{
  Q3ListViewItem *item = m_pListView->selectedItem();

  if ( item )
    m_pListView->rename( item, 0 );
}

void KonqProfileDlg::slotItemRenamed( Q3ListViewItem * item )
{
  KonqProfileItem * profileItem = static_cast<KonqProfileItem *>( item );

  QString newName = profileItem->text(0);
  QString oldName = profileItem->m_profileName;

  if (!newName.isEmpty())
  {
    KonqProfileMap::ConstIterator it = m_mapEntries.find( oldName );

    if ( it != m_mapEntries.end() )
    {
      QString fileName = it.value();
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

void KonqProfileDlg::slotSelectionChanged( Q3ListViewItem * item )
{
    m_pProfileNameLineEdit->setText( item ? item->text(0) : QString() );
}

void KonqProfileDlg::slotTextChanged( const QString & text )
{
  enableButton( KDialog::User3, !text.isEmpty() );

  // If we type the name of a profile, select it in the list

  bool itemSelected = false;
  Q3ListViewItem * item;

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

  enableButton( BTN_RENAME, itemSelected );
  enableButton( BTN_DELETE, itemSelected );
}

#undef BTN_RENAME
#undef BTN_DELETE
#undef BTN_SAVE

#include "konq_profiledlg.moc"
