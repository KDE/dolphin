#include "konq_historycomm.h"

// QDataStream operators (read and write a KonqHistoryEntry
// from/into a QDataStream
QDataStream& operator<< (QDataStream& s, const KonqHistoryEntry& e) {
    s << e.url;
    s << e.typedURL;
    s << e.title;
    s << e.numberOfTimesVisited;
    s << e.firstVisited;
    s << e.lastVisited;

    return s;
}

QDataStream& operator>> (QDataStream& s, KonqHistoryEntry& e) {
    s >> e.url;
    s >> e.typedURL;
    s >> e.title;
    s >> e.numberOfTimesVisited;
    s >> e.firstVisited;
    s >> e.lastVisited;

    return s;
}
