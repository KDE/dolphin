/* This file is part of the KDE project
   Copyright (C) 2000,2001 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include <dcopclient.h>

#include <kapp.h>
#include <kcompletion.h>
#include <kconfig.h>
#include <kdebug.h>
#include <ksavefile.h>
#include <ksimpleconfig.h>
#include <kstddirs.h>

#include <zlib.h>

template class QMap<QString,KonqHistoryEntry*>;

KonqHistoryManager::KonqHistoryManager( QObject *parent, const char *name )
    : KParts::HistoryProvider( parent, name ),
              KonqHistoryComm( "KonqHistoryManager" )
{
    // defaults
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "HistorySettings" );
    m_maxCount = config->readNumEntry( "Maximum of History entries", 500 );
    m_maxCount = QMAX( 1, m_maxCount );
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
}


KonqHistoryManager::~KonqHistoryManager()
{
    delete m_pCompletion;
    clearPending();
}


// loads the entire history
bool KonqHistoryManager::loadHistory()
{
    clearPending();
    m_history.clear();
    m_pCompletion->clear();

    QFile file( m_filename );
    if ( !file.open( IO_ReadOnly ) ) {
	if ( file.exists() )
	    kdWarning() << "Can't open " << file.name() << endl;

	// try to load the old completion history
	bool ret = loadFallback();
	emit loadingFinished();
	return ret;
    }

    QDataStream fileStream( &file );
    QByteArray data; // only used for version == 2
    // we construct the stream object now but fill in the data later.
    // thanks to QBA's explicit sharing this works :)
    QDataStream crcStream( data, IO_ReadOnly );

    if ( !fileStream.atEnd() ) {
	Q_UINT32 version;
        fileStream >> version;

        QDataStream *stream = &fileStream;

        bool crcChecked = false;
        bool crcOk = false;

        if ( version == 2 ) {
            Q_UINT32 crc;

            crcChecked = true;

            fileStream >> crc >> data;

            crcOk = crc32( 0, reinterpret_cast<unsigned char *>( data.data() ), data.size() ) == crc;

            stream = &crcStream; // pick up the right stream
        }
        else if ( version == 1 ) // fake, as we still support v1
            version = 2;

        if ( s_historyVersion != version || ( crcChecked && !crcOk ) ) {
	    kdWarning() << "The history version doesn't match, aborting loading" << endl;
	    file.close();
	    emit loadingFinished();
	    return false;
	}

	// it doesn't make sense to save to save maxAge and maxCount  in the
	// binary file, this would make backups impossible (they would clear
	// themselves on startup, because all entries expire).
	Q_UINT32 dummy;
        *stream >> dummy;
        *stream >> dummy;

        while ( !stream->atEnd() ) {
	    KonqHistoryEntry *entry = new KonqHistoryEntry;
	    CHECK_PTR( entry );
            *stream >> *entry;

	    // kdDebug(1203) << "## loaded entry: " << entry->url << ",  Title: " << entry->title << endl;
	    m_history.append( entry );

	    // insert the completion item weighted
 	    m_pCompletion->addItem( entry->url.prettyURL(),
				    entry->numberOfTimesVisited );
 	    m_pCompletion->addItem( entry->typedURL,
				    entry->numberOfTimesVisited );

	    // and fill our baseclass.
	    KParts::HistoryProvider::insert( entry->url.url() );
	}

	kdDebug(1203) << "## loaded: " << m_history.count() << " entries." << endl;

	m_history.sort();
	adjustSize();
    }

    // ### KParts::HistoryProvider::update();
    file.close();
    emit loadingFinished();

    return true;
}


// saves the entire history
bool KonqHistoryManager::saveHistory()
{
    KSaveFile file( m_filename );
    if ( file.status() != 0 ) {
	kdWarning() << "Can't open " << file.name() << endl;
	return false;
    }

    QDataStream *fileStream = file.dataStream();
    *fileStream << s_historyVersion;

    QByteArray data;
    QDataStream stream( data, IO_WriteOnly );

    stream << m_maxCount;   // not saved in history anymore, just a dummy here
    stream << m_maxAgeDays; // not saved in history anymore, just a dummy here

    QListIterator<KonqHistoryEntry> it( m_history );
    KonqHistoryEntry *entry;
    while ( (entry = it.current()) ) {
        stream << *entry;
	++it;
    }

    Q_UINT32 crc = crc32( 0, reinterpret_cast<unsigned char *>( data.data() ), data.size() );
    *fileStream << crc << data;

    file.close();

    return true;
}


void KonqHistoryManager::adjustSize()
{
    KonqHistoryEntry *entry = m_history.getFirst();

    while ( m_history.count() > m_maxCount || isExpired( entry ) ) {
	m_pCompletion->removeItem( entry->url.prettyURL() );
	m_pCompletion->removeItem( entry->typedURL );

	KParts::HistoryProvider::remove( entry->url.url() );

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


void KonqHistoryManager::addToHistory( bool pending, const KURL& _url,
				       const QString& typedURL,
				       const QString& title )
{
    kdDebug(1203) << "## addToHistory: " << _url.prettyURL() << "Typed URL: " << typedURL << ", Title: " << title << endl;

    if ( filterOut( _url ) ) // we only want remote URLs
	return;
    KURL url( _url );
    url.setPass( "" ); // No password in the history, especially not in the completion !
    KonqHistoryEntry entry;
    QString u = url.prettyURL();
    entry.url = url;
    if ( u != typedURL )
	entry.typedURL = typedURL;

    // we only keep the title if we are confirming an entry. Otherwise,
    // we might get bogus titles from the previous url (actually it's just
    // konqueror's window caption).
    if ( !pending && u != title )
	entry.title = title;
    entry.firstVisited = QDateTime::currentDateTime();
    entry.lastVisited = entry.firstVisited;

    if ( !pending ) { // remove from pending if available.
	QMapIterator<QString,KonqHistoryEntry*> it = m_pending.find( u );

	if ( it != m_pending.end() ) {
	    delete it.data();
	    m_pending.remove( it );

	    // we make a pending entry official, so we just have to update
	    // and not increment the counter. No need to care about
	    // firstVisited, as this is not taken into account on update.
	    entry.numberOfTimesVisited = 0;
	}
    }

    else {
	// We add a copy of the current history entry of the url to the
	// pending list, so that we can restore it if the user canceled.
	// If there is no entry for the url yet, we just store the url.
	KonqHistoryEntry *oldEntry = findEntry( url );
	m_pending.insert( u, oldEntry ?
                          new KonqHistoryEntry( *oldEntry ) : 0L );
    }

    // notify all konqueror instances about the entry
    emitAddToHistory( entry );
}

// interface of KParts::HistoryManager
// Usually, we only record the history for non-local URLs (i.e. filterOut()
// returns true. But when using the HistoryProvider interface, we record
// exactly those filtered-out urls.
// Moreover, we  don't get any pending/confirming entries, just one insert()
void KonqHistoryManager::insert( const QString& url )
{
    KURL u = url;
    if ( !filterOut( url ) )
	return;

    KonqHistoryEntry entry;
    entry.url = u;
    entry.firstVisited = QDateTime::currentDateTime();
    entry.lastVisited = entry.firstVisited;
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
    // kdDebug(1203) << "## Removing pending... " << url.prettyURL() << endl;

    if ( url.isLocalFile() )
	return;

    QMapIterator<QString,KonqHistoryEntry*> it = m_pending.find( url.prettyURL() );
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

void KonqHistoryManager::emitRemoveFromHistory( const KURL::List& urls )
{
    QByteArray data;
    QDataStream stream( data, IO_WriteOnly );
    stream << urls << objId();
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyRemove(KURL::List, QCString)", data );
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
    //kdDebug(1203) << "Got new entry from Broadcast: " << e.url.prettyURL() << endl;

    KonqHistoryEntry *entry = findEntry( e.url );

    if ( !entry ) { // create a new history entry
	entry = new KonqHistoryEntry;
	entry->url = e.url;
	entry->firstVisited = e.firstVisited;
	entry->numberOfTimesVisited = 0; // will get set to 1 below
	m_history.append( entry );
	KParts::HistoryProvider::insert( entry->url.url() );
    }

    if ( !e.typedURL.isEmpty() )
	entry->typedURL = e.typedURL;
    if ( !e.title.isEmpty() )
	entry->title = e.title;
    entry->numberOfTimesVisited += e.numberOfTimesVisited;
    entry->lastVisited = e.lastVisited;

    m_pCompletion->addItem( entry->url.prettyURL() );
    m_pCompletion->addItem( entry->typedURL );

    // bool pending = (e.numberOfTimesVisited != 0);

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

    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "HistorySettings" );
    config->writeEntry( "Maximum of History entries", m_maxCount );

    if ( saveId == objId() ) { // we are the sender of the broadcast
	saveHistory();
	config->sync();
    }
}

void KonqHistoryManager::notifyMaxAge( Q_UINT32 days, QCString saveId )
{
    m_maxAgeDays = days;
    clearPending();
    adjustSize();

    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "HistorySettings" );
    config->writeEntry( "Maximum age of History entries", m_maxAgeDays );

    if ( saveId == objId() ) { // we are the sender of the broadcast
	saveHistory();
	config->sync();
    }
}

void KonqHistoryManager::notifyClear( QCString saveId )
{
    clearPending();
    m_history.clear();
    m_pCompletion->clear();

    if ( saveId == objId() ) // we are the sender of the broadcast
	saveHistory();

    KParts::HistoryProvider::clear(); // also emits the cleared() signal
}

void KonqHistoryManager::notifyRemove( KURL url, QCString saveId )
{
    kdDebug(1203) << "#### Broadcast: remove entry:: " << url.prettyURL() << endl;

    KonqHistoryEntry *entry = m_history.findEntry( url );
    if ( entry ) { // entry is now the current item
	m_pCompletion->removeItem( entry->url.prettyURL() );
	m_pCompletion->removeItem( entry->typedURL );

	KParts::HistoryProvider::remove( entry->url.url() );

	m_history.take(); // does not delete
	emit entryRemoved( entry );
	delete entry;

	if ( saveId == objId() )
	    saveHistory();
    }
}

void KonqHistoryManager::notifyRemove( KURL::List urls, QCString saveId )
{
    kdDebug(1203) << "#### Broadcast: removing list!" << endl;

    bool doSave = false;
    KURL::List::Iterator it = urls.begin();
    while ( it != urls.end() ) {
	KonqHistoryEntry *entry = m_history.findEntry( *it );
	if ( entry ) { // entry is now the current item
	    m_pCompletion->removeItem( entry->url.prettyURL() );
	    m_pCompletion->removeItem( entry->typedURL );

	    KParts::HistoryProvider::remove( entry->url.url() );

	    m_history.take(); // does not delete
	    emit entryRemoved( entry );
	    delete entry;
	    doSave = true;
	}

	++it;
    }

    if ( saveId == objId() && doSave )
	saveHistory();
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
	    m_pCompletion->addItem( entry->url.prettyURL(),
				    entry->numberOfTimesVisited );

	    KParts::HistoryProvider::insert( entry->url.url() );
   	}
	++it;
    }

    m_history.sort();
    adjustSize();
    saveHistory();

    // ### KParts::HistoryProvider::update();

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
	// to make it not expire immediately...
	entry->lastVisited = QDateTime::currentDateTime();
    }

    return entry;
}

KonqHistoryEntry * KonqHistoryManager::findEntry( const KURL& url )
{
    // small optimization (dict lookup) for items _not_ in our history
    if ( !KParts::HistoryProvider::contains( url.url() ) )
        return 0L;

    return m_history.findEntry( url );
}

bool KonqHistoryManager::filterOut( const KURL& url )
{
    return ( url.isLocalFile() || url.host().isEmpty() );
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
