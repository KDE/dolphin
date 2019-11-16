/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2013 by Frank Reininghaus <frank78ac@googlemail.com>    *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

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
