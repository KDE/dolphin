/* This file is part of the KDE project
   Copyright (C) 2000,2001 Carsten Pfeiffer <pfeiffer@kde.org>
                 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qapplication.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>

#include <kaction.h>
#include <kapp.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kdialogbase.h>
#include <kfontdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <kprotocolinfo.h>
#include <kstaticdeleter.h>
#include <kwin.h>

#include <konq_drag.h>
#include <konq_faviconmgr.h>
#include <konq_historymgr.h>
#include <konq_tree.h>
#include <konq_treepart.h>

#include "history_dlg.h"
#include "history_module.h"
#include "history_settings.h"

static KStaticDeleter<KonqHistorySettings> sd;
KonqHistorySettings * KonqHistoryModule::s_settings = 0L;

KonqHistoryModule::KonqHistoryModule( KonqTree * parentTree, const char *name )
    : QObject( 0L, name ), KonqTreeModule( parentTree ),
      m_dict( 349 ),
      m_topLevelItem( 0L ),
      m_dlg( 0L ),
      m_initialized( false )
{
    if ( !s_settings ) {
	s_settings = sd.setObject(
			 new KonqHistorySettings( 0, "history settings" ));
	s_settings->readSettings();
    }

    connect( s_settings, SIGNAL( settingsChanged(const KonqHistorySettings *)),
	     SLOT( slotSettingsChanged( const KonqHistorySettings *) ));

    m_dict.setAutoDelete( true );
    m_currentTime = QDateTime::currentDateTime();

    KConfig *kc = KGlobal::config();
    KConfigGroupSaver cs( kc, "HistorySettings" );
    m_sortsByName = kc->readEntry( "SortHistory", "byName" ) == "byName";


    KonqHistoryManager *manager = KonqHistoryManager::kself();

    connect( manager, SIGNAL( loadingFinished() ), SLOT( slotCreateItems() ));
    connect( manager, SIGNAL( cleared() ), SLOT( clear() ));

    connect( manager, SIGNAL( entryAdded( const KonqHistoryEntry * ) ),
	     SLOT( slotEntryAdded( const KonqHistoryEntry * ) ));
    connect( manager, SIGNAL( entryRemoved( const KonqHistoryEntry *) ),
	     SLOT( slotEntryRemoved( const KonqHistoryEntry *) ));

    connect( parentTree, SIGNAL( expanded( QListViewItem * )),
	     SLOT( slotItemExpanded( QListViewItem * )));

    m_collection = new KActionCollection( this, "history actions" );
    (void) new KAction( i18n("New &window"), "window_new", 0, this,
 			SLOT( slotNewWindow() ), m_collection, "open_new");
    (void) new KAction( i18n("&Remove entry"), 0, this,
			SLOT( slotRemoveEntry() ), m_collection, "remove");
    (void) new KAction( i18n("C&lear History"), "history_clear", 0, this,
			SLOT( slotClearHistory() ), m_collection, "clear");
    (void) new KAction( i18n("&Preferences..."), 0, this,
			SLOT( slotPreferences()), m_collection, "preferences");

    KRadioAction *sort;
    sort = new KRadioAction( i18n("By &Name"), 0, this,
			     SLOT( slotSortByName() ), m_collection, "byName");
    sort->setExclusiveGroup("SortGroup");
    sort->setChecked( m_sortsByName );

    sort = new KRadioAction( i18n("By &Date"), 0, this,
			     SLOT( slotSortByDate() ), m_collection, "byDate");
    sort->setExclusiveGroup("SortGroup");
    sort->setChecked( !m_sortsByName );

    m_folderClosed = SmallIcon( "folder" );
    m_folderOpen = SmallIcon( "folder_open" );

    slotSettingsChanged( 0L ); // read the settings
}

KonqHistoryModule::~KonqHistoryModule()
{
    HistoryItemIterator it( m_dict );
    QStringList openGroups;
    while ( it.current() ) {
	if ( it.current()->isOpen() )
	    openGroups.append( it.currentKey() );
	++it;
    }

    KConfig *kc = KGlobal::config();
    KConfigGroupSaver cs( kc, "HistorySettings" );
    kc->writeEntry("OpenGroups", openGroups);
    kc->sync();
}

// old may be 0L!
void KonqHistoryModule::slotSettingsChanged(const KonqHistorySettings */*old*/)
{
    KonqHistoryItem::setSettings( s_settings );
    tree()->triggerUpdate();
}

void KonqHistoryModule::slotCreateItems()
{
    QApplication::setOverrideCursor( KCursor::waitCursor() );
    clear();

    KonqHistoryItem *item;
    KonqHistoryEntry *entry;
    KonqHistoryList entries( KonqHistoryManager::kself()->entries() );
    KonqHistoryIterator it( entries );
    m_currentTime = QDateTime::currentDateTime();

    // the group item and the item of the serverroot '/' get a fav-icon
    // if available. All others get the protocol icon.
    while ( (entry = it.current()) ) {
	KonqHistoryGroupItem *group = getGroupItem( entry->url );
	item = new KonqHistoryItem( entry, group, m_topLevelItem );

	++it;
    }

    KConfig *kc = KGlobal::config();
    KConfigGroupSaver cs( kc, "HistorySettings" );
    QStringList openGroups = kc->readListEntry("OpenGroups");
    QStringList::Iterator it2 = openGroups.begin();
    KonqHistoryGroupItem *group;
    while ( it2 != openGroups.end() ) {
	group = m_dict.find( *it2 );
	if ( group )
	    group->setOpen( true );

	++it2;
    }

    QApplication::restoreOverrideCursor();
    m_initialized = true;
}

// deletes the listview items but does not affect the history backend
void KonqHistoryModule::clear()
{
    m_dict.clear();
}

void KonqHistoryModule::slotEntryAdded( const KonqHistoryEntry *entry )
{
    if ( !m_initialized )
	return;

    m_currentTime = QDateTime::currentDateTime();
    KonqHistoryGroupItem *group = getGroupItem( entry->url );
    KonqHistoryItem *item = group->findChild( entry );
    if ( !item )
	item = new KonqHistoryItem( entry, group, m_topLevelItem );
    else
	item->update( entry );

    // QListView scrolls when calling sort(), so we have to hack around that
    // (we don't want no scrolling everytime an entry is added)
    KonqTree *t = tree();
    t->lockScrolling( true );
    group->sort();
    m_topLevelItem->sort();
    qApp->processOneEvent();
    t->lockScrolling( false );
}

void KonqHistoryModule::slotEntryRemoved( const KonqHistoryEntry *entry )
{
    if ( !m_initialized )
	return;

    QString groupKey = groupForURL( entry->url );
    KonqHistoryGroupItem *group = m_dict.find( groupKey );
    if ( !group )
	return;

    delete group->findChild( entry );

    if ( group->childCount() == 0 )
	m_dict.remove( groupKey );
}

void KonqHistoryModule::addTopLevelItem( KonqTreeTopLevelItem * item )
{
    m_topLevelItem = item;
}

void KonqHistoryModule::showPopupMenu()
{
    QPopupMenu *sortMenu = new QPopupMenu;
    m_collection->action("byName")->plug( sortMenu );
    m_collection->action("byDate")->plug( sortMenu );

    QPopupMenu *menu = new QPopupMenu;
    m_collection->action("open_new")->plug( menu );
    menu->insertSeparator();
    m_collection->action("remove")->plug( menu );
    m_collection->action("clear")->plug( menu );
    menu->insertSeparator();
    menu->insertItem( i18n("Sort"), sortMenu );
    menu->insertSeparator();
    m_collection->action("preferences")->plug( menu );

    menu->exec( QCursor::pos() );
    delete menu;
    delete sortMenu;
}

void KonqHistoryModule::slotNewWindow()
{
    QListViewItem *item = tree()->selectedItem();
    KonqHistoryItem *hi = dynamic_cast<KonqHistoryItem*>( item );
    if ( hi )
	emit tree()->part()->extension()->createNewWindow( hi->url() );
}

void KonqHistoryModule::slotRemoveEntry()
{
    QListViewItem *item = tree()->selectedItem();
    KonqHistoryItem *hi = dynamic_cast<KonqHistoryItem*>( item );
    if ( hi ) // remove a single entry
	KonqHistoryManager::kself()->emitRemoveFromHistory( hi->externalURL());

    else { // remove a group of entries
	KonqHistoryGroupItem *gi = dynamic_cast<KonqHistoryGroupItem*>( item );
	if ( gi )
	    gi->remove();
    }
}

void KonqHistoryModule::slotPreferences()
{
    QWidget *dlg = s_settings->m_activeDialog;
    if ( dlg ) {
	// kwin bug? we have to use the toplevel widget, instead of the widget
	dlg = dlg->topLevelWidget() ? dlg->topLevelWidget() : dlg;
	KWin::setOnDesktop( dlg->winId(), KWin::currentDesktop() );
	dlg->show();
	dlg->raise();
	KWin::setActiveWindow( dlg->winId() );
	return;
    }

    m_dlg = new KDialogBase( KDialogBase::Plain, i18n("History settings"),
			     KDialogBase::Ok | KDialogBase::Cancel,
			     KDialogBase::Ok,
			     tree(), "history dialog", false );
    QWidget *page = m_dlg->plainPage();
    QVBoxLayout *layout = new QVBoxLayout( page );
    layout->setAutoAdd( true );
    layout->setSpacing( 0 );
    layout->setMargin( 0 );
    m_settingsDlg = new KonqHistoryDialog( s_settings, page );
    connect( m_dlg, SIGNAL( finished() ), SLOT( slotDialogFinished() ));
    connect( m_dlg, SIGNAL( okClicked() ),
	     m_settingsDlg, SLOT( applySettings() ));

    m_dlg->show();
}

void KonqHistoryModule::slotDialogFinished()
{
    if ( !m_dlg )
	return;

    m_dlg->delayedDestruct();
    m_dlg = 0L;
}

void KonqHistoryModule::slotSortByName()
{
    m_sortsByName = true;
    sortingChanged();
}

void KonqHistoryModule::slotSortByDate()
{
    m_sortsByName = false;
    sortingChanged();
}

void KonqHistoryModule::sortingChanged()
{
    m_topLevelItem->sort();

    KConfig *kc = KGlobal::config();
    KConfigGroupSaver cs( kc, "HistorySettings" );
    kc->writeEntry( "SortHistory", m_sortsByName ? "byName" : "byDate" );
    kc->sync();
}

void KonqHistoryModule::slotItemExpanded( QListViewItem *item )
{
    if ( item == m_topLevelItem && !m_initialized )
	slotCreateItems();
}

void KonqHistoryModule::groupOpened( KonqHistoryGroupItem *item, bool open )
{
    if ( item->hasFavIcon() )
	return;

    if ( open )
	item->setPixmap( 0, m_folderOpen );
    else
	item->setPixmap( 0, m_folderClosed );
}


KonqHistoryGroupItem * KonqHistoryModule::getGroupItem( const KURL& url )
{
    const QString& groupKey = groupForURL( url );
    KonqHistoryGroupItem *group = m_dict.find( groupKey );
    if ( !group ) {
	group = new KonqHistoryGroupItem( url, m_topLevelItem );

	QString icon = KonqFavIconMgr::iconForURL( url.url() );
	if ( icon.isEmpty() )
	    group->setPixmap( 0, m_folderClosed );
	else
	    group->setFavIcon( SmallIcon( icon ) );

	group->setText( 0, groupKey );

	m_dict.insert( groupKey, group );
    }

    return group;
}

void KonqHistoryModule::slotClearHistory()
{
    if ( KMessageBox::questionYesNo( tree(),
				     i18n("Do you really want to clear\n"
					  "the entire history?"),
				     i18n("Clear History?") )
	 == KMessageBox::Yes )
	KonqHistoryManager::kself()->emitClear();
}


///////////////////////////////////////////////////////////////////


KonqHistoryDialog::KonqHistoryDialog( KonqHistorySettings *settings,
				      QWidget *parent, const char *name )
    : KonqHistoryDlg( parent, name ),
      m_settings( settings )
{
    m_settings->m_activeDialog = this;

    spinEntries->setRange( 1, INT_MAX, 1, false );
    spinExpire->setRange(  1, INT_MAX, 1, false );

    spinNewer->setRange( 0, INT_MAX, 1, false );
    spinOlder->setRange( 0, INT_MAX, 1, false );

    comboNewer->insertItem( i18n("minutes"), KonqHistorySettings::MINUTES );
    comboNewer->insertItem( i18n("days"), KonqHistorySettings::DAYS );

    comboOlder->insertItem( i18n("minutes"), KonqHistorySettings::MINUTES );
    comboOlder->insertItem( i18n("days"), KonqHistorySettings::DAYS );

    initFromSettings();

    connect( cbExpire, SIGNAL( toggled( bool )),
	     spinExpire, SLOT( setEnabled( bool )));
    connect( spinExpire, SIGNAL( valueChanged( int )),
	     this, SLOT( slotExpireChanged( int )));

    connect( spinNewer, SIGNAL( valueChanged( int )),
	     SLOT( slotNewerChanged( int )));
    connect( spinOlder, SIGNAL( valueChanged( int )),
	     SLOT( slotOlderChanged( int )));

    connect( btnFontNewer, SIGNAL( clicked() ), SLOT( slotGetFontNewer() ));
    connect( btnFontOlder, SIGNAL( clicked() ), SLOT( slotGetFontOlder() ));
}

KonqHistoryDialog::~KonqHistoryDialog()
{
    if ( m_settings->m_activeDialog == this )
	m_settings->m_activeDialog = 0L;
}

void KonqHistoryDialog::initFromSettings()
{
    KonqHistoryManager *manager = KonqHistoryManager::kself();
    spinEntries->setValue( manager->maxCount() );
    Q_UINT32 maxAge = manager->maxAge();
    cbExpire->setChecked( maxAge > 0 );
    spinExpire->setValue( maxAge );

    spinNewer->setValue( m_settings->m_valueYoungerThan );
    spinOlder->setValue( m_settings->m_valueOlderThan );

    comboNewer->setCurrentItem( m_settings->m_metricYoungerThan );
    comboOlder->setCurrentItem( m_settings->m_metricOlderThan );

    cbDetailedTips->setChecked( m_settings->m_detailedTips );

    m_fontNewer = m_settings->m_fontYoungerThan;
    m_fontOlder = m_settings->m_fontOlderThan;

    // enable/disable widgets
    spinExpire->setEnabled( cbExpire->isChecked() );

    slotExpireChanged( spinExpire->value() );
    slotNewerChanged( spinNewer->value() );
    slotOlderChanged( spinOlder->value() );
}

void KonqHistoryDialog::applySettings()
{
    KonqHistoryManager *manager = KonqHistoryManager::kself();
    manager->emitSetMaxCount( spinEntries->value() );
    manager->emitSetMaxAge( cbExpire->isChecked() ? spinExpire->value() : 0 );

    m_settings->m_valueYoungerThan = spinNewer->value();
    m_settings->m_valueOlderThan   = spinOlder->value();

    m_settings->m_metricYoungerThan = comboNewer->currentItem();
    m_settings->m_metricOlderThan   = comboOlder->currentItem();

    m_settings->m_detailedTips = cbDetailedTips->isChecked();

    m_settings->m_fontYoungerThan = m_fontNewer;
    m_settings->m_fontOlderThan   = m_fontOlder;

    m_settings->applySettings();
}

void KonqHistoryDialog::slotExpireChanged( int value )
{
    if ( value == 1 )
        spinExpire->setSuffix( i18n(" day") );
    else
        spinExpire->setSuffix( i18n(" days") );
}

// change hour to days, minute to minutes and the other way round,
// depending on the value of the spinbox, and synchronize the two spinBoxes
// to enfore newer <= older.
void KonqHistoryDialog::slotNewerChanged( int value )
{
    const QString& days = i18n("days");
    const QString& minutes = i18n("minutes");

    if ( value == 1 ) {
	comboNewer->changeItem( i18n("day"), KonqHistorySettings::DAYS );
	comboNewer->changeItem( i18n("minute"), KonqHistorySettings::MINUTES );
    }
    else {
	comboNewer->changeItem( days, KonqHistorySettings::DAYS );
	comboNewer->changeItem( minutes, KonqHistorySettings::MINUTES);
    }

    if ( spinNewer->value() > spinOlder->value() )
	spinOlder->setValue( spinNewer->value() );
}

void KonqHistoryDialog::slotOlderChanged( int value )
{
    const QString& days = i18n("days");
    const QString& minutes = i18n("minutes");

    if ( value == 1 ) {
	comboOlder->changeItem( i18n("day"), KonqHistorySettings::DAYS );
	comboOlder->changeItem( i18n("minute"), KonqHistorySettings::MINUTES );
    }
    else {
	comboOlder->changeItem( days, KonqHistorySettings::DAYS );
	comboOlder->changeItem( minutes, KonqHistorySettings::MINUTES);
    }

    if ( spinNewer->value() > spinOlder->value() )
	spinNewer->setValue( spinOlder->value() );
}

void KonqHistoryDialog::slotGetFontNewer()
{
    (void) KFontDialog::getFont( m_fontNewer, false, this );
}

void KonqHistoryDialog::slotGetFontOlder()
{
    (void) KFontDialog::getFont( m_fontOlder, false, this );
}

#include "history_module.moc"
