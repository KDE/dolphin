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
#include <konq_sidebartree.h>

#include "history_dlg.h"
#include "history_module.h"
#include "history_settings.h"

static KStaticDeleter<KonqSidebarHistorySettings> sd;
KonqSidebarHistorySettings * KonqSidebarHistoryModule::s_settings = 0L;

KonqSidebarHistoryModule::KonqSidebarHistoryModule( KonqSidebarTree * parentTree, const char *name )
    : QObject( 0L, name ), KonqSidebarTreeModule( parentTree ),
      m_dict( 349 ),
      m_topLevelItem( 0L ),
      m_dlg( 0L ),
      m_initialized( false )
{
    if ( !s_settings ) {
	s_settings = sd.setObject(
			 new KonqSidebarHistorySettings( 0, "history settings" ));
	s_settings->readSettings();
    }

    connect( s_settings, SIGNAL( settingsChanged(const KonqSidebarHistorySettings *)),
	     SLOT( slotSettingsChanged( const KonqSidebarHistorySettings *) ));

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

KonqSidebarHistoryModule::~KonqSidebarHistoryModule()
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
void KonqSidebarHistoryModule::slotSettingsChanged(const KonqSidebarHistorySettings */*old*/)
{
    KonqSidebarHistoryItem::setSettings( s_settings );
    tree()->triggerUpdate();
}

void KonqSidebarHistoryModule::slotCreateItems()
{
    QApplication::setOverrideCursor( KCursor::waitCursor() );
    clear();

    KonqSidebarHistoryItem *item;
    KonqHistoryEntry *entry;
    KonqHistoryList entries( KonqHistoryManager::kself()->entries() );
    KonqHistoryIterator it( entries );
    m_currentTime = QDateTime::currentDateTime();

    // the group item and the item of the serverroot '/' get a fav-icon
    // if available. All others get the protocol icon.
    while ( (entry = it.current()) ) {
	KonqSidebarHistoryGroupItem *group = getGroupItem( entry->url );
	item = new KonqSidebarHistoryItem( entry, group, m_topLevelItem );

	++it;
    }

    KConfig *kc = KGlobal::config();
    KConfigGroupSaver cs( kc, "HistorySettings" );
    QStringList openGroups = kc->readListEntry("OpenGroups");
    QStringList::Iterator it2 = openGroups.begin();
    KonqSidebarHistoryGroupItem *group;
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
void KonqSidebarHistoryModule::clear()
{
    m_dict.clear();
}

void KonqSidebarHistoryModule::slotEntryAdded( const KonqHistoryEntry *entry )
{
    if ( !m_initialized )
	return;

    m_currentTime = QDateTime::currentDateTime();
    KonqSidebarHistoryGroupItem *group = getGroupItem( entry->url );
    KonqSidebarHistoryItem *item = group->findChild( entry );
    if ( !item )
	item = new KonqSidebarHistoryItem( entry, group, m_topLevelItem );
    else
	item->update( entry );

    // QListView scrolls when calling sort(), so we have to hack around that
    // (we don't want no scrolling everytime an entry is added)
    KonqSidebarTree *t = tree();
    t->lockScrolling( true );
    group->sort();
    m_topLevelItem->sort();
    qApp->processOneEvent();
    t->lockScrolling( false );
}

void KonqSidebarHistoryModule::slotEntryRemoved( const KonqHistoryEntry *entry )
{
    if ( !m_initialized )
	return;

    QString groupKey = groupForURL( entry->url );
    KonqSidebarHistoryGroupItem *group = m_dict.find( groupKey );
    if ( !group )
	return;

    delete group->findChild( entry );

    if ( group->childCount() == 0 )
	m_dict.remove( groupKey );
}

void KonqSidebarHistoryModule::addTopLevelItem( KonqSidebarTreeTopLevelItem * item )
{
    m_topLevelItem = item;
}

void KonqSidebarHistoryModule::showPopupMenu()
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

void KonqSidebarHistoryModule::slotNewWindow()
{
    QListViewItem *item = tree()->selectedItem();
    KonqSidebarHistoryItem *hi = dynamic_cast<KonqSidebarHistoryItem*>( item );
    if ( hi )
	emit tree()->part()->getInterfaces()->getExtension()->createNewWindow( hi->url() );
}

void KonqSidebarHistoryModule::slotRemoveEntry()
{
    QListViewItem *item = tree()->selectedItem();
    KonqSidebarHistoryItem *hi = dynamic_cast<KonqSidebarHistoryItem*>( item );
    if ( hi ) // remove a single entry
	KonqHistoryManager::kself()->emitRemoveFromHistory( hi->externalURL());

    else { // remove a group of entries
	KonqSidebarHistoryGroupItem *gi = dynamic_cast<KonqSidebarHistoryGroupItem*>( item );
	if ( gi )
	    gi->remove();
    }
}

void KonqSidebarHistoryModule::slotPreferences()
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
    m_settingsDlg = new KonqSidebarHistoryDialog( s_settings, page );
    connect( m_dlg, SIGNAL( finished() ), SLOT( slotDialogFinished() ));
    connect( m_dlg, SIGNAL( okClicked() ),
	     m_settingsDlg, SLOT( applySettings() ));

    m_dlg->show();
}

void KonqSidebarHistoryModule::slotDialogFinished()
{
    if ( !m_dlg )
	return;

    m_dlg->delayedDestruct();
    m_dlg = 0L;
}

void KonqSidebarHistoryModule::slotSortByName()
{
    m_sortsByName = true;
    sortingChanged();
}

void KonqSidebarHistoryModule::slotSortByDate()
{
    m_sortsByName = false;
    sortingChanged();
}

void KonqSidebarHistoryModule::sortingChanged()
{
    m_topLevelItem->sort();

    KConfig *kc = KGlobal::config();
    KConfigGroupSaver cs( kc, "HistorySettings" );
    kc->writeEntry( "SortHistory", m_sortsByName ? "byName" : "byDate" );
    kc->sync();
}

void KonqSidebarHistoryModule::slotItemExpanded( QListViewItem *item )
{
    if ( item == m_topLevelItem && !m_initialized )
	slotCreateItems();
}

void KonqSidebarHistoryModule::groupOpened( KonqSidebarHistoryGroupItem *item, bool open )
{
    if ( item->hasFavIcon() )
	return;

    if ( open )
	item->setPixmap( 0, m_folderOpen );
    else
	item->setPixmap( 0, m_folderClosed );
}


KonqSidebarHistoryGroupItem * KonqSidebarHistoryModule::getGroupItem( const KURL& url )
{
    const QString& groupKey = groupForURL( url );
    KonqSidebarHistoryGroupItem *group = m_dict.find( groupKey );
    if ( !group ) {
	group = new KonqSidebarHistoryGroupItem( url, m_topLevelItem );

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

void KonqSidebarHistoryModule::slotClearHistory()
{
    if ( KMessageBox::questionYesNo( tree(),
				     i18n("Do you really want to clear\n"
					  "the entire history?"),
				     i18n("Clear History?") )
	 == KMessageBox::Yes )
	KonqHistoryManager::kself()->emitClear();
}


///////////////////////////////////////////////////////////////////


KonqSidebarHistoryDialog::KonqSidebarHistoryDialog( KonqSidebarHistorySettings *settings,
				      QWidget *parent, const char *name )
    : KonqSidebarHistoryDlg( parent, name ),
      m_settings( settings )
{
    m_settings->m_activeDialog = this;

    spinEntries->setRange( 1, INT_MAX, 1, false );
    spinExpire->setRange(  1, INT_MAX, 1, false );

    spinNewer->setRange( 0, INT_MAX, 1, false );
    spinOlder->setRange( 0, INT_MAX, 1, false );

    comboNewer->insertItem( i18n("minutes"), KonqSidebarHistorySettings::MINUTES );
    comboNewer->insertItem( i18n("days"), KonqSidebarHistorySettings::DAYS );

    comboOlder->insertItem( i18n("minutes"), KonqSidebarHistorySettings::MINUTES );
    comboOlder->insertItem( i18n("days"), KonqSidebarHistorySettings::DAYS );

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

KonqSidebarHistoryDialog::~KonqSidebarHistoryDialog()
{
    if ( m_settings->m_activeDialog == this )
	m_settings->m_activeDialog = 0L;
}

void KonqSidebarHistoryDialog::initFromSettings()
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

void KonqSidebarHistoryDialog::applySettings()
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

void KonqSidebarHistoryDialog::slotExpireChanged( int value )
{
    if ( value == 1 )
        spinExpire->setSuffix( i18n(" day") );
    else
        spinExpire->setSuffix( i18n(" days") );
}

// change hour to days, minute to minutes and the other way round,
// depending on the value of the spinbox, and synchronize the two spinBoxes
// to enfore newer <= older.
void KonqSidebarHistoryDialog::slotNewerChanged( int value )
{
    const QString& days = i18n("days");
    const QString& minutes = i18n("minutes");

    if ( value == 1 ) {
	comboNewer->changeItem( i18n("day"), KonqSidebarHistorySettings::DAYS );
	comboNewer->changeItem( i18n("minute"), KonqSidebarHistorySettings::MINUTES );
    }
    else {
	comboNewer->changeItem( days, KonqSidebarHistorySettings::DAYS );
	comboNewer->changeItem( minutes, KonqSidebarHistorySettings::MINUTES);
    }

    if ( spinNewer->value() > spinOlder->value() )
	spinOlder->setValue( spinNewer->value() );
}

void KonqSidebarHistoryDialog::slotOlderChanged( int value )
{
    const QString& days = i18n("days");
    const QString& minutes = i18n("minutes");

    if ( value == 1 ) {
	comboOlder->changeItem( i18n("day"), KonqSidebarHistorySettings::DAYS );
	comboOlder->changeItem( i18n("minute"), KonqSidebarHistorySettings::MINUTES );
    }
    else {
	comboOlder->changeItem( days, KonqSidebarHistorySettings::DAYS );
	comboOlder->changeItem( minutes, KonqSidebarHistorySettings::MINUTES);
    }

    if ( spinNewer->value() > spinOlder->value() )
	spinNewer->setValue( spinOlder->value() );
}

void KonqSidebarHistoryDialog::slotGetFontNewer()
{
    (void) KFontDialog::getFont( m_fontNewer, false, this );
}

void KonqSidebarHistoryDialog::slotGetFontOlder()
{
    (void) KFontDialog::getFont( m_fontOlder, false, this );
}

extern "C"
{
	KonqSidebarTreeModule* create_konqsidebartree_history(KonqSidebarTree* par)
	{
		return new KonqSidebarHistoryModule(par);
	}   
}



#include "history_module.moc"
