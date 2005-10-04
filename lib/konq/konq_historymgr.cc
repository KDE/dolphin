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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_historymgr.h"

#include <dcopclient.h>

#include <kapplication.h>
#include <kdebug.h>
#include <ksavefile.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>

#include <zlib.h>

#include "konqbookmarkmanager.h"
//Added by qt3to4:
#include <Q3CString>

const Q_UINT32 KonqHistoryManager::s_historyVersion = 3;

KonqHistoryManager::KonqHistoryManager( QObject *parent, const char *name )
    : KParts::HistoryProvider( parent ),
              KonqHistoryComm( "KonqHistoryManager" )
{
	setObjectName(name);
    m_updateTimer = new QTimer( this );

    // defaults
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "HistorySettings" );
    m_maxCount = config->readNumEntry( "Maximum of History entries", 500 );
    m_maxCount = QMAX( (Q_UINT32)1, m_maxCount );
    m_maxAgeDays = config->readNumEntry( "Maximum age of History entries", 90);

    m_history.setAutoDelete( true );
    m_filename = locateLocal( "data",
			      QLatin1String("konqueror/konq_history" ));

    if ( !kapp->dcopClient()->isAttached() )
	kapp->dcopClient()->attach();


    // take care of the completion object
    m_pCompletion = new KCompletion;
    m_pCompletion->setOrder( KCompletion::Weighted );

    // and load the history
    loadHistory();

    connect( m_updateTimer, SIGNAL( timeout() ), SLOT( slotEmitUpdated() ));
}


KonqHistoryManager::~KonqHistoryManager()
{
    delete m_pCompletion;
    clearPending();
}

bool KonqHistoryManager::isSenderOfBroadcast()
{
    DCOPClient *dc = callingDcopClient();
    return !dc || (dc->senderId() == dc->appId());
}

// loads the entire history
bool KonqHistoryManager::loadHistory()
{
    clearPending();
    m_history.clear();
    m_pCompletion->clear();

    QFile file( m_filename );
    if ( !file.open( QIODevice::ReadOnly ) ) {
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
    QDataStream crcStream( data );

    if ( !fileStream.atEnd() ) {
	Q_UINT32 version;
        fileStream >> version;

        QDataStream *stream = &fileStream;

        bool crcChecked = false;
        bool crcOk = false;

        if ( version == 2 || version == 3) {
            Q_UINT32 crc;
            crcChecked = true;
            fileStream >> crc >> data;
            crcOk = crc32( 0, reinterpret_cast<unsigned char *>( data.data() ), data.size() ) == crc;
            stream = &crcStream; // pick up the right stream
        }

	if ( version == 3 )
	{
	    //Use KURL marshalling for V3 format.
	    KonqHistoryEntry::marshalURLAsStrings = false;
	}

	if ( version != 0 && version < 3 ) //Versions 1,2 (but not 0) are also valid
	{
	    //Turn on backwards compatibility mode..
	    KonqHistoryEntry::marshalURLAsStrings = true;
	    // it doesn't make sense to save to save maxAge and maxCount  in the
	    // binary file, this would make backups impossible (they would clear
	    // themselves on startup, because all entries expire).
	    // [But V1 and V2 formats did it, so we do a dummy read]
	    Q_UINT32 dummy;
	    *stream >> dummy;
	    *stream >> dummy;

	    //OK.
	    version = 3;
	}

        if ( s_historyVersion != version || ( crcChecked && !crcOk ) ) {
	    kdWarning() << "The history version doesn't match, aborting loading" << endl;
	    file.close();
	    emit loadingFinished();
	    return false;
	}


        while ( !stream->atEnd() ) {
	    KonqHistoryEntry *entry = new KonqHistoryEntry;
	    Q_CHECK_PTR( entry );
            *stream >> *entry;
	    // kdDebug(1203) << "## loaded entry: " << entry->url << ",  Title: " << entry->title << endl;
	    m_history.append( entry );
	    QString urlString2 = entry->url.prettyURL();    

	    addToCompletion( urlString2, entry->typedURL, entry->numberOfTimesVisited );

	    // and fill our baseclass.
            QString urlString = entry->url.url();
	    KParts::HistoryProvider::insert( urlString );
            // DF: also insert the "pretty" version if different
            // This helps getting 'visited' links on websites which don't use fully-escaped urls.
        
            if ( urlString != urlString2 )
                KParts::HistoryProvider::insert( urlString2 );
	}

	kdDebug(1203) << "## loaded: " << m_history.count() << " entries." << endl;

	m_history.sort();
	adjustSize();
    }
    
    
    //This is important - we need to switch to a consistent marshalling format for
    //communicating between different konqueror instances. Since during an upgrade
    //some "old" copies may still running, we use the old format for the DCOP transfers.
    //This doesn't make that much difference performance-wise for single entries anyway.
    KonqHistoryEntry::marshalURLAsStrings = true;


    // Theoretically, we should emit update() here, but as we only ever
    // load items on startup up to now, this doesn't make much sense. Same
    // thing for the above loadFallback().
    // emit KParts::HistoryProvider::update( some list );



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
    QDataStream stream( &data, QIODevice::WriteOnly );

    //We use KURL for marshalling URLs in entries in the V3
    //file format
    KonqHistoryEntry::marshalURLAsStrings = false;
    Q3PtrListIterator<KonqHistoryEntry> it( m_history );
    KonqHistoryEntry *entry;
    while ( (entry = it.current()) ) {
        stream << *entry;
	++it;
    }

    //For DCOP, transfer strings instead - wire compat.
    KonqHistoryEntry::marshalURLAsStrings = true;

    Q_UINT32 crc = crc32( 0, reinterpret_cast<unsigned char *>( data.data() ), data.size() );
    *fileStream << crc << data;

    file.close();

    return true;
}


void KonqHistoryManager::adjustSize()
{
    KonqHistoryEntry *entry = m_history.getFirst();

    while ( m_history.count() > m_maxCount || isExpired( entry ) ) {
	removeFromCompletion( entry->url.prettyURL(), entry->typedURL );

        QString urlString = entry->url.url();
	KParts::HistoryProvider::remove( urlString );

        addToUpdateList( urlString );

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
    kdDebug(1203) << "## addToHistory: " << _url.prettyURL() << " Typed URL: " << typedURL << ", Title: " << title << endl;

    if ( filterOut( _url ) ) // we only want remote URLs
	return;

    // http URLs without a path will get redirected immediately to url + '/'
    if ( _url.path().isEmpty() && _url.protocol().startsWith("http") )
	return;

    KURL url( _url );
    bool hasPass = url.hasPass();
    url.setPass( QString::null ); // No password in the history, especially not in the completion!
    url.setHost( url.host().lower() ); // All host parts lower case
    KonqHistoryEntry entry;
    QString u = url.prettyURL();
    entry.url = url;
    if ( (u != typedURL) && !hasPass )
	entry.typedURL = typedURL;

    // we only keep the title if we are confirming an entry. Otherwise,
    // we might get bogus titles from the previous url (actually it's just
    // konqueror's window caption).
    if ( !pending && u != title )
	entry.title = title;
    entry.firstVisited = QDateTime::currentDateTime();
    entry.lastVisited = entry.firstVisited;

    // always remove from pending if available, otherwise the else branch leaks
    // if the map already contains an entry for this key.
    QMap<QString,KonqHistoryEntry*>::iterator it = m_pending.find( u );
    if ( it != m_pending.end() ) {
        delete it.data();
        m_pending.remove( it );
    }

    if ( !pending ) {
	if ( it != m_pending.end() ) {
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
// returns false). But when using the HistoryProvider interface, we record
// exactly those filtered-out urls.
// Moreover, we  don't get any pending/confirming entries, just one insert()
void KonqHistoryManager::insert( const QString& url )
{
    KURL u ( url );
    if ( !filterOut( u ) || u.protocol() == "about" ) { // remote URL
	return;
    }
    // Local URL -> add to history
    KonqHistoryEntry entry;
    entry.url = u;
    entry.firstVisited = QDateTime::currentDateTime();
    entry.lastVisited = entry.firstVisited;
    emitAddToHistory( entry );
}

void KonqHistoryManager::emitAddToHistory( const KonqHistoryEntry& entry )
{
    QByteArray data;
    QDataStream stream( &data, QIODevice::WriteOnly );
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

    QMap<QString,KonqHistoryEntry*>::iterator it = m_pending.find( url.prettyURL() );
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
    QMap<QString,KonqHistoryEntry*>::iterator it = m_pending.begin();
    while ( it != m_pending.end() ) {
	delete it.data();
	++it;
    }
    m_pending.clear();
}

void KonqHistoryManager::emitRemoveFromHistory( const KURL& url )
{
    QByteArray data;
    QDataStream stream( &data, QIODevice::WriteOnly );
    stream << url << objId();
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyRemove(KURL, QCString)", data );
}

void KonqHistoryManager::emitRemoveFromHistory( const KURL::List& urls )
{
    QByteArray data;
    QDataStream stream( &data, QIODevice::WriteOnly );
    stream << urls << objId();
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyRemove(KURL::List, QCString)", data );
}

void KonqHistoryManager::emitClear()
{
    QByteArray data;
    QDataStream stream( &data, QIODevice::WriteOnly );
    stream << objId();
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyClear(QCString)", data );
}

void KonqHistoryManager::emitSetMaxCount( Q_UINT32 count )
{
    QByteArray data;
    QDataStream stream( &data, QIODevice::WriteOnly );
    stream << count << objId();
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyMaxCount(Q_UINT32, QCString)", data );
}

void KonqHistoryManager::emitSetMaxAge( Q_UINT32 days )
{
    QByteArray data;
    QDataStream stream( &data, QIODevice::WriteOnly );
    stream << days << objId();
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyMaxAge(Q_UINT32, QCString)", data );
}

///////////////////////////////////////////////////////////////////
// DCOP called methods

void KonqHistoryManager::notifyHistoryEntry( KonqHistoryEntry e,
					     Q3CString  )
{
    //kdDebug(1203) << "Got new entry from Broadcast: " << e.url.prettyURL() << endl;

    KonqHistoryEntry *entry = findEntry( e.url );
    QString urlString = e.url.url();

    if ( !entry ) { // create a new history entry
	entry = new KonqHistoryEntry;
	entry->url = e.url;
	entry->firstVisited = e.firstVisited;
	entry->numberOfTimesVisited = 0; // will get set to 1 below
	m_history.append( entry );
	KParts::HistoryProvider::insert( urlString );
    }

    if ( !e.typedURL.isEmpty() )
	entry->typedURL = e.typedURL;
    if ( !e.title.isEmpty() )
	entry->title = e.title;
    entry->numberOfTimesVisited += e.numberOfTimesVisited;
    entry->lastVisited = e.lastVisited;

    addToCompletion( entry->url.prettyURL(), entry->typedURL );

    // bool pending = (e.numberOfTimesVisited != 0);

    adjustSize();

    // note, no need to do the updateBookmarkMetadata for every
    // history object, only need to for the broadcast sender as
    // the history object itself keeps the data consistant.
    bool updated = KonqBookmarkManager::self()->updateAccessMetadata( urlString );

    if ( isSenderOfBroadcast() ) {
	// we are the sender of the broadcast, so we save
	saveHistory();
	// note, bk save does not notify, and we don't want to!
	if (updated) 
	    KonqBookmarkManager::self()->save();
    }

    addToUpdateList( urlString );
    emit entryAdded( entry );
}

void KonqHistoryManager::notifyMaxCount( Q_UINT32 count, Q3CString )
{
    m_maxCount = count;
    clearPending();
    adjustSize();

    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "HistorySettings" );
    config->writeEntry( "Maximum of History entries", m_maxCount );

    if ( isSenderOfBroadcast() ) { 
	saveHistory();
	config->sync();
    }
}

void KonqHistoryManager::notifyMaxAge( Q_UINT32 days, Q3CString  )
{
    m_maxAgeDays = days;
    clearPending();
    adjustSize();

    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "HistorySettings" );
    config->writeEntry( "Maximum age of History entries", m_maxAgeDays );

    if ( isSenderOfBroadcast() ) { 
	saveHistory();
	config->sync();
    }
}

void KonqHistoryManager::notifyClear( Q3CString )
{
    clearPending();
    m_history.clear();
    m_pCompletion->clear();

    if ( isSenderOfBroadcast() )
	saveHistory();

    KParts::HistoryProvider::clear(); // also emits the cleared() signal
}

void KonqHistoryManager::notifyRemove( KURL url, Q3CString )
{
    kdDebug(1203) << "#### Broadcast: remove entry:: " << url.prettyURL() << endl;
    

    KonqHistoryEntry *entry = m_history.findEntry( url );
    
    if ( entry ) { // entry is now the current item
	removeFromCompletion( entry->url.prettyURL(), entry->typedURL );

        QString urlString = entry->url.url();
	KParts::HistoryProvider::remove( urlString );

        addToUpdateList( urlString );

	m_history.take(); // does not delete
	emit entryRemoved( entry );
	delete entry;

	if ( isSenderOfBroadcast() )
	    saveHistory();
    }
}

void KonqHistoryManager::notifyRemove( KURL::List urls, Q3CString )
{
    kdDebug(1203) << "#### Broadcast: removing list!" << endl;

    bool doSave = false;
    KURL::List::Iterator it = urls.begin();
    while ( it != urls.end() ) {
	KonqHistoryEntry *entry = m_history.findEntry( *it );
	
	if ( entry ) { // entry is now the current item
	    removeFromCompletion( entry->url.prettyURL(), entry->typedURL );

            QString urlString = entry->url.url();
	    KParts::HistoryProvider::remove( urlString );

            addToUpdateList( urlString );

	    m_history.take(); // does not delete
	    emit entryRemoved( entry );
	    delete entry;
	    doSave = true;
	}

	++it;
    }

    if (doSave && isSenderOfBroadcast())
        saveHistory();
}


// compatibility fallback, try to load the old completion history
bool KonqHistoryManager::loadFallback()
{
    QString file = locateLocal( "config", QLatin1String("konq_history"));
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
	    addToCompletion( entry->url.prettyURL(), QString::null, entry->numberOfTimesVisited );

	    KParts::HistoryProvider::insert( entry->url.url() );
   	}
	++it;
    }

    m_history.sort();
    adjustSize();
    saveHistory();

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
    if ( u.isValid() ) {
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

void KonqHistoryManager::slotEmitUpdated()
{
    emit KParts::HistoryProvider::updated( m_updateURLs );
    m_updateURLs.clear();
}

QStringList KonqHistoryManager::allURLs() const
{
    QStringList list;
    KonqHistoryIterator it ( m_history );
    for ( ; it.current(); ++it )
        list.append( it.current()->url.url() );
    
    return list;
}

void KonqHistoryManager::addToCompletion( const QString& url, const QString& typedURL, 
                                          int numberOfTimesVisited )
{
    m_pCompletion->addItem( url, numberOfTimesVisited );
    // typed urls have a higher priority
    m_pCompletion->addItem( typedURL, numberOfTimesVisited +10 );
}

void KonqHistoryManager::removeFromCompletion( const QString& url, const QString& typedURL )
{
    m_pCompletion->removeItem( url );
    m_pCompletion->removeItem( typedURL );
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
int KonqHistoryList::compareItems( Q3PtrCollection::Item item1,
				   Q3PtrCollection::Item item2 )
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

using namespace KParts; // for IRIX

#include "konq_historymgr.moc"
