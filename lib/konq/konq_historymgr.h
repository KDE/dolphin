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

#ifndef KONQ_HISTORY_H
#define KONQ_HISTORY_H

#include <qdatastream.h>
#include <qfile.h>
#include <qlist.h>
#include <qobject.h>
#include <qmap.h>

#include <dcopobject.h>

#include <kcompletion.h>
#include <kurl.h>
#include <kparts/historyprovider.h>

#include "konq_historycomm.h"

class KCompletion;


typedef QList<KonqHistoryEntry> KonqBaseHistoryList;
typedef QListIterator<KonqHistoryEntry> KonqHistoryIterator;

class KonqHistoryList : public KonqBaseHistoryList
{
public:
    /**
     * Finds an entry by URL. The found item will also be current().
     * If no matching entry is found, 0L is returned and current() will be
     * the first item in the list.
     */
    KonqHistoryEntry * findEntry( const KURL& url );

protected:
    /**
     * Ensures that the items are sorted by the lastVisited date
     */
    virtual int compareItems( QCollection::Item, QCollection::Item );
};


///////////////////////////////////////////////////////////////////


/**
 * This class maintains and manages a history of all URLs visited by one
 * Konqueror instance. Additionally it synchronizes the history with other
 * Konqueror instances via DCOP to keep one global and persistant history.
 *
 * It keeps the history in sync with one KCompletion object
 */
class KonqHistoryManager : public KParts::HistoryProvider,
			   public KonqHistoryComm
{
    Q_OBJECT

public:
    static KonqHistoryManager *kself() {
	return static_cast<KonqHistoryManager*>( KParts::HistoryProvider::self() );
    }

    KonqHistoryManager( QObject *parent, const char *name );
    ~KonqHistoryManager();

    /**
     * Sets a new maximum size of history and truncates the current history
     * if necessary. Notifies all other Konqueror instances via DCOP
     * to do the same.
     *
     * The history is saved after receiving the DCOP call.
     */
    void emitSetMaxCount( Q_UINT32 count );

    /**
     * Sets a new maximum age of history entries and removes all entries that
     * are older than @p days. Notifies all other Konqueror instances via DCOP
     * to do the same.
     *
     * An age of 0 means no expiry based on the age.
     *
     * The history is saved after receiving the DCOP call.
     */
    void emitSetMaxAge( Q_UINT32 days );

    /**
     * Removes the history entry for @p url, if existant. Tells all other
     * Konqueror instances via DCOP to do the same.
     *
     * The history is saved after receiving the DCOP call.
     */
    void emitRemoveFromHistory( const KURL& url );

    /**
     * Removes the history entries for the given list of @p urls. Tells all
     * other Konqueror instances via DCOP to do the same.
     *
     * The history is saved after receiving the DCOP call.
     */
    void emitRemoveFromHistory( const KURL::List& urls );

    /**
     * @returns the current maximum number of history entries.
     */
    Q_UINT32 maxCount() const { return m_maxCount; }

    /**
     * @returns the current maximum age (in days) of history entries.
     */
    Q_UINT32 maxAge() const { return m_maxAgeDays; }

    /**
     * Adds a pending entry to the history. Pending means, that the entry is
     * not verified yet, i.e. it is not sure @p url does exist at all. You
     * probably don't know the title of the url in that case either.
     * Call @ref confirmPending() as soon you know the entry is good and should
     * be updated.
     *
     * If an entry with @p url already exists,
     * it will be updated (lastVisited date will become the current time
     * and the number of visits will be incremented).
     *
     * @param url The url of the history entry
     * @param typedURL the string that the user typed, which resulted in url
     *                 Doesn't have to be a valid url, e.g. "slashdot.org".
     * @param title The title of the URL. If you don't know it (yet), you may
                    specify it in @ref confirmPending().
     */
    void addPending( const KURL& url, const QString& typedURL = QString::null,
		     const QString& title = QString::null );

    /**
     * Confirms and updates the entry for @p url.
     */
    void confirmPending( const KURL& url,
			 const QString& typedURL = QString::null,
			 const QString& title = QString::null );

    /**
     * Removes a pending url from the history, e.g. when the url does not
     * exist, or the user aborted loading.
     */
    void removePending( const KURL& url );

    /**
     * @returns the KCompletion object.
     */
    KCompletion * completionObject() const { return m_pCompletion; }

    /**
     * @returns the list of all history entries, sorted by date
     * (oldest entries first)
     */
    const KonqHistoryList& entries() const { return m_history; }

    // HistoryProvider interfae, let konq handle this
    /**
     * Reimplemented in such a way that all URLs that would be filtered
     * out normally (see @ref filterOut()) will still be added to the history.
     * By default, file:/ urls will be filtered out, but if they come thru
     * the HistoryProvider interface, they are added to the history.
     */
    virtual void insert( const QString& );
    virtual void remove( const QString& ) {}
    virtual void clear() {}


public slots:
    /**
     * Loads the history and fills the completion object.
     */
    bool loadHistory();

    /**
     * Saves the entire history.
     */
    bool saveHistory();

    /**
     * Clears the history and tells all other Konqueror instances via DCOP
     * to do the same.
     * The history is saved afterwards, if necessary.
     */
    void emitClear();


signals:
    /**
     * Emitted after the entire history was loaded from disk.
     */
    void loadingFinished();

    /**
     * Emitted after a new entry was added
     */
    void entryAdded( const KonqHistoryEntry *entry );

    /**
     * Emitted after an entry was removed from the history
     * Note, that this entry will be deleted immediately after you got
     * that signal.
     */
    void entryRemoved( const KonqHistoryEntry *entry );

protected:
    /**
     * Resizes the history list to contain less or equal than m_maxCount
     * entries. The first (oldest) entries are removed.
     */
    void adjustSize();

    /**
     * @returns true if @p entry is older than the given maximum age,
     * otherwise false.
     */
    inline bool isExpired( KonqHistoryEntry *entry ) {
	return (entry && m_maxAgeDays > 0 && entry->lastVisited <
		QDate::currentDate().addDays( -m_maxAgeDays ));
    }

    /**
     * Notifes all running instances about a new HistoryEntry via DCOP
     */
    void emitAddToHistory( const KonqHistoryEntry& entry );

    /**
     * Every konqueror instance broadcasts new history entries to the other
     * konqueror instances. Those add the entry to their list, but don't
     * save the list, because the sender saves the list.
     *
     * @p saveId is the @ref DCOPObject::objId() of the sender so that
     * only the sender saves the new history.
     */
    virtual void notifyHistoryEntry( KonqHistoryEntry e, QCString saveId );

    /**
     * Called when the configuration of the maximum count changed.
     * Called via DCOP by some config-module
     */
    virtual void notifyMaxCount( Q_UINT32 count, QCString saveId );

    /**
     * Called when the configuration of the maximum age of history-entries
     * changed. Called via DCOP by some config-module
     */
    virtual void notifyMaxAge( Q_UINT32 days, QCString saveId );

    /**
     * Clears the history completely. Called via DCOP by some config-module
     */
    virtual void notifyClear( QCString saveId );

    /**
     * Notifes about a url that has to be removed from the history.
     * The instance where saveId == objId() has to save the history.
     */
    virtual void notifyRemove( KURL url, QCString saveId );

    /**
     * Notifes about a list of urls that has to be removed from the history.
     * The instance where saveId == objId() has to save the history.
     */
    virtual void notifyRemove( KURL::List urls, QCString saveId );

    /**
     * Does the work for @ref addPending() and @ref confirmPending().
     *
     * Adds an entry to the history. If an entry with @p url already exists,
     * it will be updated (lastVisited date will become the current time
     * and the number of visits will be incremented).
     * @p pending means, the entry has not been "verified", it's been added
     * right after typing the url.
     * If @p pending is false, @p url will be removed from the pending urls
     * (if available) and NOT be added again in that case.
     */
    void addToHistory( bool pending, const KURL& url,
		       const QString& typedURL = QString::null,
		       const QString& title = QString::null );


    /**
     * @returns true if the given @p url should be filtered out and not be
     * added to the history. By default, all local urls (url.isLocalFile())
     * will return true, as well as urls with an empty host.
     */
    virtual bool filterOut( const KURL& url );

private:
    void clearPending();
    /**
     * a little optimization for KonqHistoryList::findEntry(),
     * checking the dict of KParts::HistoryProvider before traversing the list.
     * Can't be used everywhere, because it always returns 0L for "pending"
     * entries, as those are not added to the dict, currently.
     */
    KonqHistoryEntry * findEntry( const KURL& url );

    /**
     * Stuff to create a proper history out of KDE 2.0's konq_history for
     * completion.
     */
    bool loadFallback();
    KonqHistoryEntry * createFallbackEntry( const QString& ) const;

    QString m_filename;
    KonqHistoryList m_history;

    /**
     * List of pending entries, which were added to the history, but not yet
     * confirmed (i.e. not yet added with pending = false).
     * Note: when removing an entry, you have to delete the KonqHistoryEntry
     * of the item you remove.
     */
    QMap<QString,KonqHistoryEntry*> m_pending;

    Q_UINT32 m_maxCount;   // maximum of history entries
    Q_UINT32 m_maxAgeDays; // maximum age of a history entry

    KCompletion *m_pCompletion; // the completion object we sync with

    static const Q_UINT32 s_historyVersion = 2;
};


#endif // KONQ_HISTORY_H
