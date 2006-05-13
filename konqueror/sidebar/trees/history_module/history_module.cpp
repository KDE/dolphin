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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <QApplication>
#include <QMenu>

#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kcursor.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kstaticdeleter.h>
#include <ktoggleaction.h>

#include <konq_faviconmgr.h>

#include "history_module.h"
#include "history_settings.h"

static KStaticDeleter<KonqSidebarHistorySettings> sd;
KonqSidebarHistorySettings * KonqSidebarHistoryModule::s_settings = 0L;

KonqSidebarHistoryModule::KonqSidebarHistoryModule( KonqSidebarTree * parentTree, const char *name )
    : QObject( 0L ), KonqSidebarTreeModule( parentTree ),
      m_dict( 349 ),
      m_topLevelItem( 0L ),
      m_dlg( 0L ),
      m_initialized( false )
{
    setObjectName( name );

    if ( !s_settings ) {
	sd.setObject( s_settings,
                      new KonqSidebarHistorySettings( 0, "history settings" ));
	s_settings->readSettings( true );
    }

    connect( s_settings, SIGNAL( settingsChanged() ), SLOT( slotSettingsChanged() ));

    m_dict.setAutoDelete( true );
    m_currentTime = QDateTime::currentDateTime();

    KConfig *kc = KGlobal::config();
    KConfigGroup cs( kc, "HistorySettings" );
    m_sortsByName = cs.readEntry( "SortHistory", "byDate" ) == "byName";


    KonqHistoryManager *manager = KonqHistoryManager::kself();

    connect( manager, SIGNAL( loadingFinished() ), SLOT( slotCreateItems() ));
    connect( manager, SIGNAL( cleared() ), SLOT( clear() ));

    connect( manager, SIGNAL( entryAdded( const KonqHistoryEntry & ) ),
	     SLOT( slotEntryAdded( const KonqHistoryEntry & ) ));
    connect( manager, SIGNAL( entryRemoved( const KonqHistoryEntry &) ),
	     SLOT( slotEntryRemoved( const KonqHistoryEntry &) ));

    connect( parentTree, SIGNAL( expanded( QListViewItem * )),
	     SLOT( slotItemExpanded( QListViewItem * )));

    m_collection = new KActionCollection( this );
    KAction *action = new KAction(KIcon("window_new"),  i18n("New &Window"), m_collection, "open_new");
    connect(action, SIGNAL(triggered(bool)), SLOT( slotNewWindow() ));
    action = new KAction(KIcon("editdelete"),  i18n("&Remove Entry"), m_collection, "remove");
    connect(action, SIGNAL(triggered(bool)), SLOT( slotRemoveEntry() ));
    action = new KAction(KIcon("history_clear"),  i18n("C&lear History"), m_collection, "clear");
    connect(action, SIGNAL(triggered(bool)), SLOT( slotClearHistory() ));
    action = new KAction(KIcon("configure"),  i18n("&Preferences..."), m_collection, "preferences");
    connect(action, SIGNAL(triggered(bool)), SLOT( slotPreferences()));

    QActionGroup* sortGroup = new QActionGroup(this);
    sortGroup->setExclusive(true);

    KToggleAction *sort;
    sort = new KToggleAction( i18n("By &Name"), m_collection, "byName");
    connect(sort, SIGNAL(triggered(bool) ), SLOT( slotSortByName() ));
    sort->setActionGroup(sortGroup);
    sort->setChecked( m_sortsByName );

    sort = new KToggleAction( i18n("By &Date"), m_collection, "byDate");
    connect(sort, SIGNAL(triggered(bool) ), SLOT( slotSortByDate() ));
    sort->setActionGroup(sortGroup);
    sort->setChecked( !m_sortsByName );

    m_folderClosed = SmallIcon( "folder" );
    m_folderOpen = SmallIcon( "folder_open" );

    slotSettingsChanged(); // read the settings
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
    KConfigGroup cs( kc, "HistorySettings" );
    cs.writeEntry("OpenGroups", openGroups);
    kc->sync();
}

void KonqSidebarHistoryModule::slotSettingsChanged()
{
    KonqSidebarHistoryItem::setSettings( s_settings );
    tree()->triggerUpdate();
}

void KonqSidebarHistoryModule::slotCreateItems()
{
    QApplication::setOverrideCursor( KCursor::waitCursor() );
    clear();

    KonqSidebarHistoryItem *item;
    KonqHistoryList entries( KonqHistoryManager::kself()->entries() );
    m_currentTime = QDateTime::currentDateTime();

    // the group item and the item of the serverroot '/' get a fav-icon
    // if available. All others get the protocol icon.
    KonqHistoryList::const_iterator it = entries.begin();
    const KonqHistoryList::const_iterator end = entries.end();
    for ( ; it != end ; ++it ) {
	KonqSidebarHistoryGroupItem *group = getGroupItem( (*it).url );
	item = new KonqSidebarHistoryItem( (*it), group, m_topLevelItem );
    }

    KConfig *kc = KGlobal::config();
    KConfigGroup cs( kc, "HistorySettings" );
    QStringList openGroups = cs.readEntry("OpenGroups",QStringList());
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

void KonqSidebarHistoryModule::slotEntryAdded( const KonqHistoryEntry& entry )
{
    if ( !m_initialized )
	return;

    m_currentTime = QDateTime::currentDateTime();
    KonqSidebarHistoryGroupItem *group = getGroupItem( entry.url );
    KonqSidebarHistoryItem *item = group->findChild( entry );
    if ( !item )
	item = new KonqSidebarHistoryItem( entry, group, m_topLevelItem );
    else
	item->update( entry );

    // QListView scrolls when calling sort(), so we have to hack around that
    // (we don't want no scrolling every time an entry is added)
    KonqSidebarTree *t = tree();
    t->lockScrolling( true );
    group->sort();
    m_topLevelItem->sort();
    qApp->processOneEvent(); // #####
    t->lockScrolling( false );
}

void KonqSidebarHistoryModule::slotEntryRemoved( const KonqHistoryEntry& entry )
{
    if ( !m_initialized )
	return;

    QString groupKey = groupForURL( entry.url );
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

bool KonqSidebarHistoryModule::handleTopLevelContextMenu( KonqSidebarTreeTopLevelItem *,
                                                          const QPoint& pos )
{
    showPopupMenu( ModuleContextMenu, pos );
    return true;
}

void KonqSidebarHistoryModule::showPopupMenu()
{
    showPopupMenu( EntryContextMenu | ModuleContextMenu, QCursor::pos() );
}

void KonqSidebarHistoryModule::showPopupMenu( int which, const QPoint& pos )
{
    QMenu *sortMenu = new QMenu;
    sortMenu->addAction( m_collection->action("byName") );
    sortMenu->addAction( m_collection->action("byDate") );

    QMenu *menu = new QMenu;

    if ( which & EntryContextMenu )
    {
        menu->addAction( m_collection->action("open_new") );
        menu->insertSeparator();
        menu->addAction( m_collection->action("remove") );
    }

    menu->addAction( m_collection->action("clear") );
    menu->insertSeparator();
    menu->insertItem( i18n("Sort"), sortMenu );
    menu->insertSeparator();
    menu->addAction( m_collection->action("preferences") );

    menu->exec( pos );
    delete menu;
    delete sortMenu;
}

void KonqSidebarHistoryModule::slotNewWindow()
{
    kDebug(1201)<<"void KonqSidebarHistoryModule::slotNewWindow()"<<endl;

    Q3ListViewItem *item = tree()->selectedItem();
    KonqSidebarHistoryItem *hi = dynamic_cast<KonqSidebarHistoryItem*>( item );
    if ( hi )
       {
          kDebug(1201)<<"void KonqSidebarHistoryModule::slotNewWindow(): emitting createNewWindow"<<endl;
   	  emit tree()->createNewWindow( hi->url() );
       }
}

void KonqSidebarHistoryModule::slotRemoveEntry()
{
    Q3ListViewItem *item = tree()->selectedItem();
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
    // Run the history sidebar settings.
    KRun::run( "kcmshell kcmhistory", KUrl::List() );
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
    KConfigGroup cs( kc, "HistorySettings" );
    cs.writeEntry( "SortHistory", m_sortsByName ? "byName" : "byDate" );
    kc->sync();
}

void KonqSidebarHistoryModule::slotItemExpanded( Q3ListViewItem *item )
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


KonqSidebarHistoryGroupItem * KonqSidebarHistoryModule::getGroupItem( const KUrl& url )
{
    const QString& groupKey = groupForURL( url );
    KonqSidebarHistoryGroupItem *group = m_dict.find( groupKey );
    if ( !group ) {
	group = new KonqSidebarHistoryGroupItem( url, m_topLevelItem );

	QString icon = KonqFavIconMgr::iconForURL( url );
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
    KGuiItem guiitem = KStdGuiItem::clear();
    guiitem.setIcon( SmallIconSet("history_clear"));

    if ( KMessageBox::warningContinueCancel( tree(),
				     i18n("Do you really want to clear "
					  "the entire history?"),
				     i18n("Clear History?"), guiitem )
	 == KMessageBox::Continue )
	KonqHistoryManager::kself()->emitClear();
}


extern "C"
{
	KDE_EXPORT KonqSidebarTreeModule* create_konq_sidebartree_history(KonqSidebarTree* par, const bool)
	{
		return new KonqSidebarHistoryModule(par);
	}
}



#include "history_module.moc"
