/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2013 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMRANGE_H
#define KITEMRANGE_H

#include <QList>

struct KItemRange
{
    KItemRange(int index = 0, int count = 0);
    int index;
    int count;

    bool operator == (const KItemRange& other) const;
};

inline KItemRange::KItemRange(int index, int count) :
    index(index),
    count(count)
{
}

inline bool KItemRange::operator == (const KItemRange& other) const
{
    return index == other.index && count == other.count;
}


class KItemRangeList : public QList<KItemRange>
{
public:
    KItemRangeList() : QList<KItemRange>() {}
    explicit KItemRangeList(const QList<KItemRange>& list) : QList<KItemRange>(list) {}

    template<class Container>
    static KItemRangeList fromSortedContainer(const Container& container);

    KItemRangeList& operator<<(const KItemRange& range)
    {
        append(range);
        return *this;
    }
};

template<class Container>
KItemRangeList KItemRangeList::fromSortedContainer(const Container& container)
{
    typename Container::const_iterator it = container.constBegin();
    const typename Container::const_iterator end = container.constEnd();

    if (it == end) {
        return KItemRangeList();
    }

    KItemRangeList result;

    int index = *it;
    int count = 1;

    // Remove duplicates, see https://bugs.kde.org/show_bug.cgi?id=335672
    while (it != end && *it == index) {
        ++it;
    }

    while (it != end) {
        if (*it == index + count) {
            ++count;
        } else {
            result << KItemRange(index, count);
            index = *it;
            count = 1;
        }
        ++it;

        // Remove duplicates, see https://bugs.kde.org/show_bug.cgi?id=335672
        while (it != end && *it == *(it - 1)) {
            ++it;
        }
    }

    result << KItemRange(index, count);
    return result;
}

#endif
