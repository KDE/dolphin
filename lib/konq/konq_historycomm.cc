#include "konq_historycomm.h"

// QDataStream operators (read and write a KonqHistoryEntry
// from/into a QDataStream)
QDataStream& operator<< (QDataStream& s, const KonqHistoryEntry& e) {
    // TODO (BIC) stream the KURL, not just its .url(). Takes a bit more space,
    // but prevents much reparsing when loading.
    s << e.url.url();
    s << e.typedURL;
    s << e.title;
    s << e.numberOfTimesVisited;
    s << e.firstVisited;
    s << e.lastVisited;

    return s;
}

QDataStream& operator>> (QDataStream& s, KonqHistoryEntry& e) {
    QString url;
    s >> url;
    e.url = url;
    s >> e.typedURL;
    s >> e.title;
    s >> e.numberOfTimesVisited;
    s >> e.firstVisited;
    s >> e.lastVisited;

    return s;
}
