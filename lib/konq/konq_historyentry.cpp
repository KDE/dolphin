#include "konq_historyentry.h"

bool KonqHistoryEntry::marshalURLAsStrings;

// QDataStream operators (read and write a KonqHistoryEntry
// from/into a QDataStream)
QDataStream& operator<< (QDataStream& s, const KonqHistoryEntry& e) {
    if (KonqHistoryEntry::marshalURLAsStrings)
	s << e.url.url();
    else
	s << e.url;

    s << e.typedURL;
    s << e.title;
    s << e.numberOfTimesVisited;
    s << e.firstVisited;
    s << e.lastVisited;

    return s;
}

QDataStream& operator>> (QDataStream& s, KonqHistoryEntry& e) {
    if (KonqHistoryEntry::marshalURLAsStrings)
    {
	QString url;
	s >> url;
	e.url = url;
    }
    else
    {
	s>>e.url;
    }

    s >> e.typedURL;
    s >> e.title;
    s >> e.numberOfTimesVisited;
    s >> e.firstVisited;
    s >> e.lastVisited;

    return s;
}
