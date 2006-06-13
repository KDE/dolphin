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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQ_HISTORYENTRY_H
#define KONQ_HISTORYENTRY_H

#include <QDateTime>
#include <QStringList>

#include <kurl.h>

#include <libkonq_export.h>

class LIBKONQ_EXPORT KonqHistoryEntry
{
public:
    //Should URLs be marshaled as strings (V2 history format)?
    static bool marshalURLAsStrings;

    KonqHistoryEntry()
	: numberOfTimesVisited(1) {}

    KUrl url;
    QString typedURL;
    QString title;
    quint32 numberOfTimesVisited;
    QDateTime firstVisited;
    QDateTime lastVisited;

    // Necessary for QList (on Windows)
    bool operator==( const KonqHistoryEntry& entry ) const {
        return url == entry.url &&
            typedURL == entry.typedURL &&
            title == entry.title &&
            numberOfTimesVisited == entry.numberOfTimesVisited &&
            firstVisited == entry.firstVisited &&
            lastVisited == entry.lastVisited;
    }
};


// QDataStream operators (read and write a KonqHistoryEntry
// from/into a QDataStream
QDataStream& operator<< (QDataStream& s, const KonqHistoryEntry& e);
QDataStream& operator>> (QDataStream& s, KonqHistoryEntry& e);

///////////////////////////////////////////////////////////////////


/**
 * DCOP Methods for KonqHistoryManager. Has to be in a separate file, because
 * dcopidl2cpp barfs on every second construct ;(
 * Implementations of the pure virtual methods are in KonqHistoryManager
class KonqHistoryComm : public DCOPObject
{
    K_DCOP

protected:
    KonqHistoryComm( DCOPCString objId ) : DCOPObject( objId ) {}

k_dcop:
    virtual ASYNC notifyHistoryEntry( KonqHistoryEntry e, QByteArray saveId) = 0;
    virtual ASYNC notifyMaxCount( quint32 count, QByteArray saveId ) = 0;
    virtual ASYNC notifyMaxAge( quint32 days, QByteArray saveId ) = 0;
    virtual ASYNC notifyClear( QByteArray saveId ) = 0;
    virtual ASYNC notifyRemove( KUrl url, QByteArray saveId ) = 0;
    virtual ASYNC notifyRemove( KUrl::List url, QByteArray saveId ) = 0;
    virtual QStringList allURLs() const = 0;

};
 */

#endif // KONQ_HISTORYCOMM_H
