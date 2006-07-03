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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_historymgr.h"
#include "konq_historymgr_adaptor.h"
#include "konqbookmarkmanager.h"

#include <QtDBus/QtDBus>
#include <kapplication.h>
#include <kdebug.h>
#include <ksavefile.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kcompletion.h>

#include <zlib.h> // for crc32


const int KonqHistoryManager::s_historyVersion = 3;

KonqHistoryManager::KonqHistoryManager( QObject *parent )
    : KParts::HistoryProvider( parent )
{
    m_updateTimer = new QTimer( this );

    // defaults
    KConfig *config = KGlobal::config();
    KConfigGroup cs( config, "HistorySettings" );
    m_maxCount = cs.readEntry( "Maximum of History entries", 500 );
    m_maxCount = qMax( 1, m_maxCount );
    m_maxAgeDays = cs.readEntry( "Maximum age of History entries", 90);

    m_filename = locateLocal( "data",
			      QLatin1String("konqueror/konq_history" ));

    // take care of the completion object
    m_pCompletion = new KCompletion;
    m_pCompletion->setOrder( KCompletion::Weighted );

    // and load the history
    loadHistory();

    connect( m_updateTimer, SIGNAL( timeout() ), SLOT( slotEmitUpdated() ));

    (void) new KonqHistoryManagerAdaptor( this );
    const QString dbusPath = "/KonqHistoryManager";
    const QString dbusInterface = "org.kde.libkonq.KonqHistoryManager";

    QDBusConnection dbus = QDBus::sessionBus();
    dbus.registerObject( dbusPath, this );
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyClear", this, SLOT(slotNotifyClear(QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyHistoryEntry", this, SLOT(slotNotifyHistoryEntry(QByteArray,QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyMaxAge", this, SLOT(slotNotifyMaxAge(int,QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyMaxCount", this, SLOT(slotNotifyMaxCount(int,QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyRemove", this, SLOT(slotNotifyRemove(QString,QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyRemoveList", this, SLOT(slotNotifyRemoveList(QStringList,QDBusMessage)));
}


KonqHistoryManager::~KonqHistoryManager()
{
    delete m_pCompletion;
    clearPending();
}

static QString dbusService()
{
    return QDBus::sessionBus().baseService();
}

bool KonqHistoryManager::isSenderOfSignal( const QDBusMessage& msg )
{
    return dbusService() == msg.service();
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
	    kWarning() << "Can't open " << file.fileName() << endl;

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
	quint32 version;
        fileStream >> version;

        QDataStream *stream = &fileStream;

        bool crcChecked = false;
        bool crcOk = false;

        if ( version == 2 || version == 3) {
            quint32 crc;
            crcChecked = true;
            fileStream >> crc >> data;
            crcOk = crc32( 0, reinterpret_cast<unsigned char *>( data.data() ), data.size() ) == crc;
            stream = &crcStream; // pick up the right stream
        }

	if ( version == 3 )
	{
	    //Use KUrl marshalling for V3 format.
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
	    quint32 dummy;
	    *stream >> dummy;
	    *stream >> dummy;

	    //OK.
	    version = 3;
	}

        if ( s_historyVersion != (int)version || ( crcChecked && !crcOk ) ) {
	    kWarning() << "The history version doesn't match, aborting loading" << endl;
	    file.close();
	    emit loadingFinished();
	    return false;
	}


        while ( !stream->atEnd() ) {
	    KonqHistoryEntry entry;
            *stream >> entry;
	    // kDebug(1203) << "## loaded entry: " << entry->url << ",  Title: " << entry->title << endl;
	    m_history.append( entry );
	    QString urlString2 = entry.url.prettyUrl();

	    addToCompletion( urlString2, entry.typedUrl, entry.numberOfTimesVisited );

	    // and fill our baseclass.
            QString urlString = entry.url.url();
	    KParts::HistoryProvider::insert( urlString );
            // DF: also insert the "pretty" version if different
            // This helps getting 'visited' links on websites which don't use fully-escaped urls.

            if ( urlString != urlString2 )
                KParts::HistoryProvider::insert( urlString2 );
	}

	kDebug(1203) << "## loaded: " << m_history.count() << " entries." << endl;

	qSort( m_history.begin(), m_history.end(), lastVisitedOrder );
	adjustSize();
    }


    //This is important - we need to switch to a consistent marshalling format for
    //communicating between different konqueror instances. Since during an upgrade
    //some "old" copies may still running, we use the old format for the DBUS transfers.
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
	kWarning() << "Can't open " << file.name() << endl;
	return false;
    }

    QDataStream *fileStream = file.dataStream();
    *fileStream << s_historyVersion;

    QByteArray data;
    QDataStream stream( &data, QIODevice::WriteOnly );

    //We use KUrl for marshalling URLs in entries in the V3
    //file format
    KonqHistoryEntry::marshalURLAsStrings = false;
    QListIterator<KonqHistoryEntry> it( m_history );
    while ( it.hasNext() ) {
        stream << it.next();
    }

    //For DBUS, transfer strings instead - wire compat.
    KonqHistoryEntry::marshalURLAsStrings = true;

    quint32 crc = crc32( 0, reinterpret_cast<unsigned char *>( data.data() ), data.size() );
    *fileStream << crc << data;

    file.close();

    return true;
}


void KonqHistoryManager::adjustSize()
{
    if (m_history.isEmpty())
        return;

    KonqHistoryEntry entry = m_history.first();
    const QDateTime expirationDate( QDate::currentDate().addDays( -m_maxAgeDays ) );

    while ( m_history.count() > (qint32)m_maxCount ||
            (m_maxAgeDays > 0 && entry.lastVisited.isValid() && entry.lastVisited < expirationDate) ) // i.e. entry is expired
    {
	removeFromCompletion( entry.url.prettyUrl(), entry.typedUrl );

        QString urlString = entry.url.url();
	KParts::HistoryProvider::remove( urlString );

        addToUpdateList( urlString );

	emit entryRemoved( m_history.first() );
	m_history.removeFirst();

        if ( m_history.isEmpty() )
            break;
	entry = m_history.first();
    }
}


void KonqHistoryManager::addPending( const KUrl& url, const QString& typedUrl,
				     const QString& title )
{
    addToHistory( true, url, typedUrl, title );
}

void KonqHistoryManager::confirmPending( const KUrl& url,
					 const QString& typedUrl,
					 const QString& title )
{
    addToHistory( false, url, typedUrl, title );
}


void KonqHistoryManager::addToHistory( bool pending, const KUrl& _url,
				       const QString& typedUrl,
				       const QString& title )
{
    kDebug(1203) << "## addToHistory: " << _url.prettyUrl() << " Typed URL: " << typedUrl << ", Title: " << title << endl;

    if ( filterOut( _url ) ) // we only want remote URLs
	return;

    // http URLs without a path will get redirected immediately to url + '/'
    if ( _url.path().isEmpty() && _url.protocol().startsWith("http") )
	return;

    KUrl url( _url );
    bool hasPass = url.hasPass();
    url.setPass( QString() ); // No password in the history, especially not in the completion!
    url.setHost( url.host().toLower() ); // All host parts lower case
    KonqHistoryEntry entry;
    QString u = url.prettyUrl();
    entry.url = url;
    if ( (u != typedUrl) && !hasPass )
	entry.typedUrl = typedUrl;

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
        delete it.value();
        m_pending.erase( it );
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
        KonqHistoryList::iterator oldEntry = findEntry( url );
	m_pending.insert( u, oldEntry != m_history.end() ?
                          new KonqHistoryEntry( *oldEntry ) : 0 );
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
    KUrl u ( url );
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
    stream << entry << dbusService();
    // Protection against very long urls (like data:)
    if ( data.size() > 4096 )
        return;
    emit notifyHistoryEntry( data );
}


void KonqHistoryManager::removePending( const KUrl& url )
{
    // kDebug(1203) << "## Removing pending... " << url.prettyUrl() << endl;

    if ( url.isLocalFile() )
	return;

    QMap<QString,KonqHistoryEntry*>::iterator it = m_pending.find( url.prettyUrl() );
    if ( it != m_pending.end() ) {
	KonqHistoryEntry *oldEntry = it.value(); // the old entry, may be 0
	emitRemoveFromHistory( url ); // remove the current pending entry

	if ( oldEntry ) // we had an entry before, now use that instead
	    emitAddToHistory( *oldEntry );

	delete oldEntry;
	m_pending.erase( it );
    }
}

// clears the pending list and makes sure the entries get deleted.
void KonqHistoryManager::clearPending()
{
    QMap<QString,KonqHistoryEntry*>::iterator it = m_pending.begin();
    while ( it != m_pending.end() ) {
	delete it.value();
	++it;
    }
    m_pending.clear();
}

void KonqHistoryManager::emitRemoveFromHistory( const KUrl& url )
{
    emit notifyRemove( url.url() );
}

void KonqHistoryManager::emitRemoveListFromHistory( const KUrl::List& urls )
{
    emit notifyRemoveList( urls.toStringList() );
}

void KonqHistoryManager::emitClear()
{
    emit notifyClear();
}

void KonqHistoryManager::emitSetMaxCount( int count )
{
    emit notifyMaxCount( count );
}

void KonqHistoryManager::emitSetMaxAge( int days )
{
    emit notifyMaxAge( days );
}

///////////////////////////////////////////////////////////////////
// DBUS called methods

void KonqHistoryManager::slotNotifyHistoryEntry( const QByteArray & data,
                                                 const QDBusMessage& msg )
{
    KonqHistoryEntry e;
    QDataStream stream( const_cast<QByteArray *>( &data ), QIODevice::ReadOnly );
    stream >> e;
    kDebug(1203) << "Got new entry from Broadcast: " << e.url.prettyUrl() << endl;

    KonqHistoryList::iterator existingEntry = findEntry( e.url );
    QString urlString = e.url.url();
    const bool newEntry = existingEntry == m_history.end();

    KonqHistoryEntry entry;

    if ( !newEntry ) {
        entry = *existingEntry;
    } else { // create a new history entry
	entry.url = e.url;
	entry.firstVisited = e.firstVisited;
	entry.numberOfTimesVisited = 0; // will get set to 1 below
	KParts::HistoryProvider::insert( urlString );
    }

    if ( !e.typedUrl.isEmpty() )
	entry.typedUrl = e.typedUrl;
    if ( !e.title.isEmpty() )
	entry.title = e.title;
    entry.numberOfTimesVisited += e.numberOfTimesVisited;
    entry.lastVisited = e.lastVisited;

    if ( newEntry )
        m_history.append( entry );
    else {
        *existingEntry = entry;
    }

    addToCompletion( entry.url.prettyUrl(), entry.typedUrl );

    // bool pending = (e.numberOfTimesVisited != 0);

    adjustSize();

    // note, no need to do the updateBookmarkMetadata for every
    // history object, only need to for the broadcast sender as
    // the history object itself keeps the data consistant.
    bool updated = KonqBookmarkManager::self()->updateAccessMetadata( urlString );

    if ( isSenderOfSignal( msg ) ) {
	// we are the sender of the broadcast, so we save
	saveHistory();
	// note, bk save does not notify, and we don't want to!
	if (updated)
	    KonqBookmarkManager::self()->save();
    }

    addToUpdateList( urlString );
    emit entryAdded( entry );
}

void KonqHistoryManager::slotNotifyMaxCount( int count, const QDBusMessage& msg )
{
    m_maxCount = count;
    clearPending();
    adjustSize();

    KConfig *config = KGlobal::config();
    KConfigGroup cs( config, "HistorySettings" );
    cs.writeEntry( "Maximum of History entries", m_maxCount );

    if ( isSenderOfSignal( msg ) ) {
	saveHistory();
	cs.sync();
    }
}

void KonqHistoryManager::slotNotifyMaxAge( int days, const QDBusMessage& msg  )
{
    m_maxAgeDays = days;
    clearPending();
    adjustSize();

    KConfig *config = KGlobal::config();
    KConfigGroup cs( config, "HistorySettings" );
    cs.writeEntry( "Maximum age of History entries", m_maxAgeDays );

    if ( isSenderOfSignal( msg ) ) {
	saveHistory();
	cs.sync();
    }
}

void KonqHistoryManager::slotNotifyClear( const QDBusMessage& msg )
{
    clearPending();
    m_history.clear();
    m_pCompletion->clear();

    if ( isSenderOfSignal( msg ) )
	saveHistory();

    KParts::HistoryProvider::clear(); // also emits the cleared() signal
}

void KonqHistoryManager::slotNotifyRemove( const QString& urlStr, const QDBusMessage& msg )
{
    KUrl url( urlStr );
    kDebug(1203) << "#### Broadcast: remove entry:: " << url.prettyUrl() << endl;

    KonqHistoryList::iterator existingEntry = findEntry( url );

    if ( existingEntry != m_history.end() ) {
        const KonqHistoryEntry entry = *existingEntry; // make copy, due to erase call below
	removeFromCompletion( entry.url.prettyUrl(), entry.typedUrl );

        const QString urlString = entry.url.url();
	KParts::HistoryProvider::remove( urlString );

        addToUpdateList( urlString );

	m_history.erase( existingEntry );
	emit entryRemoved( entry );

	if ( isSenderOfSignal( msg ) )
	    saveHistory();
    }
}

void KonqHistoryManager::slotNotifyRemoveList( const QStringList& urls, const QDBusMessage& msg )
{
    kDebug(1203) << "#### Broadcast: removing list!" << endl;

    bool doSave = false;
    QStringList::const_iterator it = urls.begin();
    for ( ; it != urls.end(); ++it ) {
        KUrl url = *it;
        KonqHistoryList::iterator existingEntry = m_history.findEntry( url );
        if ( existingEntry != m_history.end() ) {
            const KonqHistoryEntry entry = *existingEntry; // make copy, due to erase call below
	    removeFromCompletion( entry.url.prettyUrl(), entry.typedUrl );

            const QString urlString = entry.url.url();
	    KParts::HistoryProvider::remove( urlString );

            addToUpdateList( urlString );

            m_history.erase( existingEntry );
            emit entryRemoved( entry );

            doSave = true;
	}
    }

    if ( doSave && isSenderOfSignal( msg ) )
        saveHistory();
}

// compatibility fallback, try to load the old completion history
bool KonqHistoryManager::loadFallback()
{
    QString file = locateLocal( "config", QLatin1String("konq_history"));
    if ( file.isEmpty() )
	return false;

    KSimpleConfig config( file );
    config.setGroup("History");
    QStringList items = config.readEntry( "CompletionItems" , QStringList() );
    QStringList::Iterator it = items.begin();

    while ( it != items.end() ) {
	KonqHistoryEntry entry = createFallbackEntry( *it );
	if ( entry.url.isValid() ) {
	    m_history.append( entry );
	    addToCompletion( entry.url.prettyUrl(), QString(), entry.numberOfTimesVisited );

	    KParts::HistoryProvider::insert( entry.url.url() );
   	}
	++it;
    }

    qSort( m_history.begin(), m_history.end(), lastVisitedOrder );
    adjustSize();
    saveHistory();

    return true;
}

// tries to create a small KonqHistoryEntry out of a string, where the string
// looks like "http://www.bla.com/bla.html:23"
// the attached :23 is the weighting from KCompletion
KonqHistoryEntry KonqHistoryManager::createFallbackEntry(const QString& item) const
{
    // code taken from KCompletion::addItem(), adjusted to use weight = 1
    uint len = item.length();
    uint weight = 1;

    // find out the weighting of this item (appended to the string as ":num")
    int index = item.lastIndexOf(':');
    if ( index > 0 ) {
	bool ok;
	weight = item.mid( index + 1 ).toUInt( &ok );
	if ( !ok )
	    weight = 1;

	len = index; // only insert until the ':'
    }


    KonqHistoryEntry entry;
    KUrl u( item.left( len ));
    // that's the only entries we know about...
    entry.url = u;
    entry.numberOfTimesVisited = weight;
    // to make it not expire immediately...
    entry.lastVisited = QDateTime::currentDateTime();

    return entry;
}

KonqHistoryList::iterator KonqHistoryManager::findEntry( const KUrl& url )
{
    // small optimization (dict lookup) for items _not_ in our history
    if ( !KParts::HistoryProvider::contains( url.url() ) )
        return m_history.end();

    return m_history.findEntry( url );
}

bool KonqHistoryManager::filterOut( const KUrl& url )
{
    return ( url.isLocalFile() || url.host().isEmpty() );
}

void KonqHistoryManager::slotEmitUpdated()
{
    emit KParts::HistoryProvider::updated( m_updateURLs );
    m_updateURLs.clear();
}

#if 0 // unused
QStringList KonqHistoryManager::allURLs() const
{
    QStringList list;
    QListIterator<KonqHistoryEntry> it( m_history );
    while ( it.hasNext() ) {
        list.append( it.next().url.url() );
    }
    return list;
}
#endif

void KonqHistoryManager::addToCompletion( const QString& url, const QString& typedUrl,
                                          int numberOfTimesVisited )
{
    m_pCompletion->addItem( url, numberOfTimesVisited );
    // typed urls have a higher priority
    m_pCompletion->addItem( typedUrl, numberOfTimesVisited +10 );
}

void KonqHistoryManager::removeFromCompletion( const QString& url, const QString& typedUrl )
{
    m_pCompletion->removeItem( url );
    m_pCompletion->removeItem( typedUrl );
}

//////////////////////////////////////////////////////////////////


KonqHistoryList::iterator KonqHistoryList::findEntry( const KUrl& url )
{
    // we search backwards, probably faster to find an entry
    KonqHistoryList::iterator it = end();
    while ( it != begin() ) {
        --it;
	if ( (*it).url == url )
	    return it;
    }
    return end();
}

void KonqHistoryList::removeEntry( const KUrl& url )
{
    iterator it = findEntry( url );
    if ( it != end() )
        erase( it );
}

using namespace KParts; // for IRIX
#include "konq_historymgr.moc"
