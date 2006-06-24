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

#ifndef KONQ_HISTORY_H
#define KONQ_HISTORY_H

#include <QObject>
#include <QMap>
#include <QTimer>

#include <kurl.h>
#include <kparts/historyprovider.h>

#include "konq_historyentry.h"

#include <libkonq_export.h>

class QDBusMessage;
class KCompletion;


class LIBKONQ_EXPORT KonqHistoryList : public QList<KonqHistoryEntry>
{
public:
    /**
     * Finds an entry by URL and return an iterator to it.
     * If no matching entry is found, end() is returned.
     */
    iterator findEntry( const KUrl& url );

    /**
     * Finds an entry by URL and removes it
     */
    void removeEntry( const KUrl& url );
};


///////////////////////////////////////////////////////////////////


/**
 * This class maintains and manages a history of all URLs visited by one
 * Konqueror instance. Additionally it synchronizes the history with other
 * Konqueror instances via DBUS to keep one global and persistant history.
 *
 * It keeps the history in sync with one KCompletion object
 */
class LIBKONQ_EXPORT KonqHistoryManager : public KParts::HistoryProvider
{
    Q_OBJECT

public:
    static KonqHistoryManager *kself() {
	return static_cast<KonqHistoryManager*>( KParts::HistoryProvider::self() );
    }

    KonqHistoryManager( QObject *parent = 0 );
    ~KonqHistoryManager();

    /**
     * Sets a new maximum size of history and truncates the current history
     * if necessary. Notifies all other Konqueror instances via DBUS
     * to do the same.
     *
     * The history is saved after receiving the DBUS call.
     */
    void emitSetMaxCount( int count );

    /**
     * Sets a new maximum age of history entries and removes all entries that
     * are older than @p days. Notifies all other Konqueror instances via DBUS
     * to do the same.
     *
     * An age of 0 means no expiry based on the age.
     *
     * The history is saved after receiving the DBUS call.
     */
    void emitSetMaxAge( int days );

    /**
     * Removes the history entry for @p url, if existant. Tells all other
     * Konqueror instances via DBUS to do the same.
     *
     * The history is saved after receiving the DBUS call.
     */
    void emitRemoveFromHistory( const KUrl& url );

    /**
     * Removes the history entries for the given list of @p urls. Tells all
     * other Konqueror instances via DBUS to do the same.
     *
     * The history is saved after receiving the DBUS call.
     */
    void emitRemoveListFromHistory( const KUrl::List& urls );

    /**
     * @returns the current maximum number of history entries.
     */
    int maxCount() const { return m_maxCount; }

    /**
     * @returns the current maximum age (in days) of history entries.
     */
    int maxAge() const { return m_maxAgeDays; }

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
     * @param typedUrl the string that the user typed, which resulted in url
     *                 Doesn't have to be a valid url, e.g. "slashdot.org".
     * @param title The title of the URL. If you don't know it (yet), you may
                    specify it in @ref confirmPending().
     */
    void addPending( const KUrl& url, const QString& typedUrl = QString(),
		     const QString& title = QString() );

    /**
     * Confirms and updates the entry for @p url.
     */
    void confirmPending( const KUrl& url,
			 const QString& typedUrl = QString(),
			 const QString& title = QString() );

    /**
     * Removes a pending url from the history, e.g. when the url does not
     * exist, or the user aborted loading.
     */
    void removePending( const KUrl& url );

    /**
     * @returns the KCompletion object.
     */
    KCompletion * completionObject() const { return m_pCompletion; }

    /**
     * @returns the list of all history entries, sorted by date
     * (oldest entries first)
     */
    const KonqHistoryList& entries() const { return m_history; }

    // HistoryProvider interface, let konq handle this
    /**
     * Reimplemented in such a way that all URLs that would be filtered
     * out normally (see @ref filterOut()) will still be added to the history.
     * By default, file:/ urls will be filtered out, but if they come thru
     * the HistoryProvider interface, they are added to the history.
     */
    virtual void insert( const QString& );
    virtual void remove( const QString& ) {}
    virtual void clear() {}


public Q_SLOTS:
    /**
     * Loads the history and fills the completion object.
     */
    bool loadHistory();

    /**
     * Saves the entire history.
     */
    bool saveHistory();

    /**
     * Clears the history and tells all other Konqueror instances via DBUS
     * to do the same.
     * The history is saved afterwards, if necessary.
     */
    void emitClear();


Q_SIGNALS:
    /**
     * Emitted after the entire history was loaded from disk.
     */
    void loadingFinished();

    /**
     * Emitted after a new entry was added
     */
    void entryAdded( const KonqHistoryEntry& entry );

    /**
     * Emitted after an entry was removed from the history
     * Note, that this entry will be deleted immediately after you got
     * that signal.
     */
    void entryRemoved( const KonqHistoryEntry& entry );

protected:
    /**
     * Resizes the history list to contain less or equal than m_maxCount
     * entries. The first (oldest) entries are removed.
     */
    void adjustSize();

    /**
     * Notifes all running instances about a new HistoryEntry via DBUS
     */
    void emitAddToHistory( const KonqHistoryEntry& entry );

Q_SIGNALS: // DBUS methods/signals

    /**
     * Every konqueror instance broadcasts new history entries to the other
     * konqueror instances. Those add the entry to their list, but don't
     * save the list, because the sender saves the list.
     *
     * @param e the new history entry
     * @param saveId is the dbus service of the sender so that
     * only the sender saves the new history.
     */
    void notifyHistoryEntry( const QByteArray & historyEntry );

    /**
     * Called when the configuration of the maximum count changed.
     * Called via DBUS by some config-module
     */
    void notifyMaxCount( int count );

    /**
     * Called when the configuration of the maximum age of history-entries
     * changed. Called via DBUS by some config-module
     */
    void notifyMaxAge( int days );

    /**
     * Clears the history completely. Called via DBUS by some config-module
     */
    void notifyClear();

    /**
     * Notifes about a url that has to be removed from the history.
     * The sender instance has to save the history.
     */
    void notifyRemove( const QString& url );

    /**
     * Notifes about a list of urls that has to be removed from the history.
     * The sender instance has to save the history.
     */
    void notifyRemoveList( const QStringList& urls );

private Q_SLOTS: // connected to DBUS signals
    /**
     * @returns a list of all urls in the history.
     */
    //QStringList allURLs() const;

    void slotNotifyHistoryEntry( const QByteArray & historyEntry, const QDBusMessage& msg );
    void slotNotifyMaxCount( int count, const QDBusMessage& msg );
    void slotNotifyMaxAge( int days, const QDBusMessage& msg );
    void slotNotifyClear( const QDBusMessage& msg );
    void slotNotifyRemove( const QString& url, const QDBusMessage& msg );
    void slotNotifyRemoveList( const QStringList& urls, const QDBusMessage& msg );

private:
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
    void addToHistory( bool pending, const KUrl& url,
		       const QString& typedUrl = QString(),
		       const QString& title = QString() );


    /**
     * @returns true if the given @p url should be filtered out and not be
     * added to the history. By default, all local urls (url.isLocalFile())
     * will return true, as well as urls with an empty host.
     */
    virtual bool filterOut( const KUrl& url );

    void addToUpdateList( const QString& url ) {
        m_updateURLs.append( url );
        m_updateTimer->setSingleShot( true );
        m_updateTimer->start( 500 );
    }

    /**
     * The list of urls that is going to be emitted in slotEmitUpdated. Add
     * urls to it whenever you modify the list of history entries and start
     * m_updateTimer.
     */
    QStringList m_updateURLs;

private Q_SLOTS:
    /**
     * Called by the updateTimer to emit the KParts::HistoryProvider::updated()
     * signal so that khtml can repaint the updated links.
     */
    void slotEmitUpdated();

private:
    /**
     * Returns whether the DBUS call we are handling was a call from us self
     */
    bool isSenderOfSignal( const QDBusMessage& msg );

    void clearPending();
    /**
     * a little optimization for KonqHistoryList::findEntry(),
     * checking the dict of KParts::HistoryProvider before traversing the list.
     * Can't be used everywhere, because it always returns end() for "pending"
     * entries, as those are not added to the dict, currently.
     */
    KonqHistoryList::iterator findEntry( const KUrl& url );

    /**
     * Stuff to create a proper history out of KDE 2.0's konq_history for
     * completion.
     */
    bool loadFallback();
    KonqHistoryEntry createFallbackEntry( const QString& ) const;

    void addToCompletion( const QString& url, const QString& typedUrl, int numberOfTimesVisited = 1 );
    void removeFromCompletion( const QString& url, const QString& typedUrl );

    /**
     * Ensures that the items are sorted by the lastVisited date
     * (oldest goes first)
     */
    static bool lastVisitedOrder( const KonqHistoryEntry& lhs, const KonqHistoryEntry& rhs ) {
        return lhs.lastVisited < rhs.lastVisited;
    }

    QString m_filename;
    KonqHistoryList m_history;

    /**
     * List of pending entries, which were added to the history, but not yet
     * confirmed (i.e. not yet added with pending = false).
     * Note: when removing an entry, you have to delete the KonqHistoryEntry
     * of the item you remove.
     */
    QMap<QString,KonqHistoryEntry*> m_pending;

    int m_maxCount;   // maximum of history entries
    int m_maxAgeDays; // maximum age of a history entry

    KCompletion *m_pCompletion; // the completion object we sync with

    /**
     * A timer that will emit the KParts::HistoryProvider::updated() signal
     * thru the slotEmitUpdated slot.
     */
    QTimer *m_updateTimer;

    static const int s_historyVersion;
};


#endif // KONQ_HISTORY_H
