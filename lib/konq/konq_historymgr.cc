/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_historymgr.h"

#include <qcstring.h>
#include <qtimer.h>

#include <dcopclient.h>

#include <kapp.h>
#include <kcompletion.h>
#include <kconfig.h>
#include <kdebug.h>
#include <ksavefile.h>
#include <ksimpleconfig.h>
#include <kstddirs.h>

KonqHistoryManager * KonqHistoryManager::s_self = 0L;
template class QMap<QString,KonqHistoryEntry*>;

KonqHistoryManager * KonqHistoryManager::self()
{
    if ( !s_self )
	s_self = new KonqHistoryManager( kapp, "KonqHistoryManager" );

    return s_self;
}

KonqHistoryManager::KonqHistoryManager( QObject *parent, const char *name )
    : QObject( parent, name ),
      KonqHistoryComm( "KonqHistoryManager" )
{
    // defaults
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "HistorySettings" );
    m_maxCount = config->readNumEntry( "Maximum of History entries", 300 );
    m_maxAgeDays = config->readNumEntry( "Maximum age of History entries", 90);

    m_history.setAutoDelete( true );
    m_filename = locateLocal( "data",
			      QString::fromLatin1("konqueror/konq_history" ));

    if ( !kapp->dcopClient()->isAttached() )
	kapp->dcopClient()->attach();


    // take care of the completion object
    m_pCompletion = new KCompletion;
    m_pCompletion->setOrder( KCompletion::Weighted );

    // and load the history
    loadHistory();

    // we need to notice when the day changes. We can't check the current
    // time and install a timer from current -> midnight, because we might
    // get SIGSTOP'ed in the middle.

// not yet
//     m_lastDate = QDate::currentDate();
//     m_timer = new QTimer( this, SLOT( slotCheckDayChanged() ));
//     m_timer->start( 60 * 1000 ); // once a minute is enough
}


KonqHistoryManager::~KonqHistoryManager()
{
    delete m_pCompletion;
    clearPending();
}


void KonqHistoryManager::slotCheckDayChanged()
{
    if ( m_lastDate < QDate::currentDate() ) {
	m_lastDate = QDate::currentDate();
	// FIXME
    }
}

// loads the entire history
bool KonqHistoryManager::loadHistory()
{
    clearPending();
    m_history.clear();
    m_pCompletion->clear();

    QFile file( m_filename );
    if ( !file.open( IO_ReadOnly ) ) {
	kdWarning() << "Can't open " << file.name() << endl;

	// try to load the old completion history
	bool ret = loadFallback();
	emit loadingFinished();
	return ret;
    }
	
    QDataStream stream( &file );

    if ( !stream.atEnd() ) {
	Q_UINT32 version;
	stream >> version;
	if ( s_historyVersion != version ) { // simple version check for now
	    kdWarning() << "The history version doesn't match, aborting loading" << endl;
	    file.close();
	    emit loadingFinished();
	    return false;
	}
	
	stream >> m_maxCount;
	stream >> m_maxAgeDays;

	while ( !stream.atEnd() ) {
	    KonqHistoryEntry *entry = new KonqHistoryEntry;
	    CHECK_PTR( entry );
	    stream >> *entry;
	
	    // kdDebug(1203) << "## loaded entry: " << entry->url << ",  Title: " << entry->title << endl;
	    m_history.append( entry );

	    // insert the completion item weighted
 	    m_pCompletion->addItem( entry->url.url(),
				    entry->numberOfTimesVisited );
 	    m_pCompletion->addItem( entry->typedURL,
				    entry->numberOfTimesVisited );
	}
	
	kdDebug(1203) << "## loaded: " << m_history.count() << " entries." << endl;
	
	m_history.sort();
	adjustSize();
    }

    file.close();
    emit loadingFinished();

    return true;
}


// saves the entire history
bool KonqHistoryManager::saveHistory()
{
    // file not open yet
    KSaveFile file( m_filename );
    if ( file.status() != 0 ) {
	kdWarning() << "Can't open " << file.name() << endl;
	return false;
    }

    QDataStream *stream = file.dataStream();
    *stream << s_historyVersion;
    *stream << m_maxCount;
    *stream << m_maxAgeDays;

    QListIterator<KonqHistoryEntry> it( m_history );
    KonqHistoryEntry *entry;
    while ( (entry = it.current()) ) {
	*stream << *entry;
	++it;
    }
    file.close();

    return true;
}


void KonqHistoryManager::adjustSize()
{
    KonqHistoryEntry *entry = m_history.getFirst();
    while ( m_history.count() > m_maxCount || isExpired( entry ) ) {
	m_pCompletion->removeItem( entry->url.url() );
	m_pCompletion->removeItem( entry->typedURL );

	emit entryRemoved( m_history.getFirst() );
	m_history.removeFirst(); // deletes the entry
	
	entry = m_history.getFirst();
    }
}


void KonqHistoryManager::addPending( const KURL& url, const QString& typedURL,
				     const QString& title )
{
    addToHistory( true, url, typedURL, title );
}

void KonqHistoryManager::confirmPending( const KURL& url,
					 const QString& typedURL,
					 const QString& title )
{
    addToHistory( false, url, typedURL, title );
}


void KonqHistoryManager::addToHistory( bool pending, const KURL& url,
				       const QString& typedURL,
				       const QString& title )
{
    kdDebug(1203) << "## addToHistory: " << url.url() << "Typed URL: " << typedURL << ", Title: " << title << endl;

    // we only want remote URLs
    if ( url.isLocalFile() || url.host().isEmpty() )
	return;

    KonqHistoryEntry entry;
    QString u = url.url();
    entry.url = url;
    if ( u != typedURL )
	entry.typedURL = typedURL;

    // we only get keep the title if we are confirming an entry. Otherwise,
    // we might get bogus titles from the previous url (actually it's just
    // konqueror's window caption).
    if ( !pending && u != title )
	entry.title = title;
    entry.firstVisited = QDateTime::currentDateTime();
    entry.lastVisited = entry.firstVisited;

    if ( !pending ) { // remove from pending if available.
	QMapIterator<QString,KonqHistoryEntry*> it = m_pending.find( url.url());
	if ( it != m_pending.end() ) {
	    delete it.data();
	    m_pending.remove( it );
	
	    // we make a pending entry official, so we just have to update
	    // and not increment the counter. No need to care about
	    // firstVisited, as this is not taken into account on update.
	    entry.numberOfTimesVisited = 0;
	}
    }

    if ( pending ) {
	// We add a copy of the current history entry of the url to the
	// pending list, so that we can restore it if the user canceled.
	// If there is no entry for the url yet, we just store the url.
	KonqHistoryEntry *oldEntry = m_history.findEntry( url );
	m_pending.insert( url.url(), oldEntry ?
			             new KonqHistoryEntry( *oldEntry ) : 0L );
    }

    // notify all konqueror instances about the entry
    emitAddToHistory( entry );
}


void KonqHistoryManager::emitAddToHistory( const KonqHistoryEntry& entry )
{
    QByteArray data;
    QDataStream stream( data, IO_WriteOnly );
    stream << entry << objId();
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyHistoryEntry(KonqHistoryEntry, QCString)",
			      data );
}


void KonqHistoryManager::removePending( const KURL& url )
{
    kdDebug(1203) << "## Removing pending... " << url.url() << endl;

    if ( url.isLocalFile() )
	return;

    QMapIterator<QString,KonqHistoryEntry*> it = m_pending.find( url.url() );
    if ( it != m_pending.end() ) {
	KonqHistoryEntry *oldEntry = it.data(); // the old entry, may be 0L
	emitRemoveFromHistory( url ); // remove the current pending entry

	if ( oldEntry ) // we had an entry before, now use that instead
	    emitAddToHistory( *oldEntry );
	
	delete oldEntry;
	m_pending.remove( it );
    }
}

// clears the pending list and makes sure the entries get deleted.
void KonqHistoryManager::clearPending()
{
    QMapIterator<QString,KonqHistoryEntry*> it = m_pending.begin();
    while ( it != m_pending.end() ) {
	delete it.data();
	++it;
    }
    m_pending.clear();
}

void KonqHistoryManager::emitRemoveFromHistory( const KURL& url )
{
    QByteArray data;
    QDataStream stream( data, IO_WriteOnly );
    stream << url << objId();
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyRemove(KURL, QCString)", data );
}

void KonqHistoryManager::emitClear()
{
    QByteArray data;
    QDataStream stream( data, IO_WriteOnly );
    stream << objId();
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyClear(QCString)", data );
}

void KonqHistoryManager::emitSetMaxCount( Q_UINT32 count )
{
    QByteArray data;
    QDataStream stream( data, IO_WriteOnly );
    stream << count << objId();
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyMaxCount(Q_UINT32, QCString)", data );
}

void KonqHistoryManager::emitSetMaxAge( Q_UINT32 days )
{
    QByteArray data;
    QDataStream stream( data, IO_WriteOnly );
    stream << days << objId();
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyMaxAge(Q_UINT32, QCString)", data );
}

///////////////////////////////////////////////////////////////////
// DCOP called methods

void KonqHistoryManager::notifyHistoryEntry( KonqHistoryEntry e,
					     QCString saveId )
{
    kdDebug(1203) << "## Got new entry from Broadcast: " << e.url.url() << endl;

    KonqHistoryEntry *entry = m_history.findEntry( e.url );
    if ( !entry ) { // create a new history entry
	entry = new KonqHistoryEntry;
	entry->url = e.url;
	entry->firstVisited = e.firstVisited;
	m_history.append( entry );
    }

    if ( !e.typedURL.isEmpty() )
	entry->typedURL = e.typedURL;
    if ( !e.title.isEmpty() )
	entry->title = e.title;
    entry->numberOfTimesVisited += e.numberOfTimesVisited;
    entry->lastVisited = e.lastVisited;

    m_pCompletion->addItem( entry->url.url() );
    m_pCompletion->addItem( entry->typedURL );

    adjustSize();

    if ( saveId == objId() ) // we are the sender of the broadcast, so we save
	saveHistory();

    emit entryAdded( entry );
}

void KonqHistoryManager::notifyMaxCount( Q_UINT32 count, QCString saveId )
{
    m_maxCount = count;
    clearPending();
    adjustSize();

    if ( saveId == objId() ) // we are the sender of the broadcast
	saveHistory();

    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "HistorySettings" );
    config->writeEntry( "Maximum of History entries", m_maxCount );
}

void KonqHistoryManager::notifyMaxAge( Q_UINT32 days, QCString saveId )
{
    m_maxAgeDays = days;
    clearPending();
    adjustSize();

    if ( saveId == objId() ) // we are the sender of the broadcast
	saveHistory();

    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "HistorySettings" );
    config->writeEntry( "Maximum age of History entries", m_maxAgeDays );
}

void KonqHistoryManager::notifyClear( QCString saveId )
{
    clearPending();
    m_history.clear();
    m_pCompletion->clear();

    if ( saveId == objId() ) // we are the sender of the broadcast
	saveHistory();

    emit cleared();
}

void KonqHistoryManager::notifyRemove( KURL url, QCString saveId )
{
    kdDebug(1203) << "#### Broadcast: remove entry:: " << url.url() << endl;

    KonqHistoryEntry *entry = m_history.findEntry( url );
    if ( entry ) { // entry is now the current item
	m_pCompletion->removeItem( entry->url.url() );
	m_pCompletion->removeItem( entry->typedURL );
	m_history.take(); // does not delete
	emit entryRemoved( entry );
	delete entry;
	
	if ( saveId == objId() )
	    saveHistory();
    }
}


// compatibility fallback, try to load the old completion history
bool KonqHistoryManager::loadFallback()
{
    QString file = locateLocal( "config", QString::fromLatin1("konq_history"));
    if ( file.isEmpty() )
	return false;

    KonqHistoryEntry *entry;
    KSimpleConfig config( file );
    config.setGroup("History");
    QStringList items = config.readListEntry( "CompletionItems" );
    QStringList::Iterator it = items.begin();
    while ( it != items.end() ) {
	entry = createFallbackEntry( *it );
	if ( entry ) {
	    m_history.append( entry );
	    m_pCompletion->addItem( entry->url.url(),
				    entry->numberOfTimesVisited );
   	}
	++it;
    }

    m_history.sort();
    adjustSize();

    return true;
}

// tries to create a small KonqHistoryEntry out of a string, where the string
// looks like "http://www.bla.com/bla.html:23"
// the attached :23 is the weighting from KCompletion
KonqHistoryEntry * KonqHistoryManager::createFallbackEntry(const QString& item) const
{
    // code taken from KCompletion::addItem(), adjusted to use weight = 1
    uint len = item.length();
    uint weight = 1;

    // find out the weighting of this item (appended to the string as ":num")
    int index = item.findRev(':');
    if ( index > 0 ) {
	bool ok;
	weight = item.mid( index + 1 ).toUInt( &ok );
	if ( !ok )
	    weight = 1;

	len = index; // only insert until the ':'
    }


    KonqHistoryEntry *entry = 0L;
    KURL u( item.left( len ));
    if ( !u.isMalformed() ) {
	entry = new KonqHistoryEntry;
	// that's the only entries we know about...
	entry->url = u;
	entry->numberOfTimesVisited = weight;
    }
    return entry;
}


//////////////////////////////////////////////////////////////////


KonqHistoryEntry * KonqHistoryList::findEntry( const KURL& url )
{
    // we search backwards, probably faster to find an entry
    KonqHistoryEntry *entry = last();
    while ( entry ) {
	if ( entry->url == url )
	    return entry;

	entry = prev();
    }

    return 0L;
}

// sort by lastVisited date (oldest go first)
int KonqHistoryList::compareItems( QCollection::Item item1,
				   QCollection::Item item2 )
{
    KonqHistoryEntry *entry1 = static_cast<KonqHistoryEntry *>( item1 );
    KonqHistoryEntry *entry2 = static_cast<KonqHistoryEntry *>( item2 );

    if ( entry1->lastVisited > entry2->lastVisited )
	return 1;
    else if ( entry1->lastVisited < entry2->lastVisited )
	return -1;
    else
	return 0;
}

#include "konq_historymgr.moc"
