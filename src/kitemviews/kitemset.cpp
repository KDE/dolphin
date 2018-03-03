/***************************************************************************
 *   Copyright (C) 2013 by Frank Reininghaus <frank78ac@googlemail.com>    *
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

#include "kitemset.h"


KItemSet::iterator KItemSet::insert(int i)
{
    if (m_itemRanges.empty()) {
        m_itemRanges.push_back(KItemRange(i, 1));
        return iterator(m_itemRanges.begin(), 0);
    }

    KItemRangeList::iterator rangeBegin = m_itemRanges.begin();
    if (i < rangeBegin->index) {
        // The inserted index is smaller than all existing items.
        if (i == rangeBegin->index - 1) {
            // Move the beginning of the first range one item to the front.
            --rangeBegin->index;
            ++rangeBegin->count;
        } else {
            // Insert a new range at the beginning.
            rangeBegin = m_itemRanges.insert(rangeBegin, KItemRange(i, 1));
        }

        return iterator(rangeBegin, 0);
    }

    KItemRangeList::iterator rangeEnd = m_itemRanges.end();
    KItemRangeList::iterator lastRange = rangeEnd - 1;
    if (i >= lastRange->index) {
        // i either belongs to the last range, or it is larger than all existing items.
        const int lastItemPlus1 = lastRange->index + lastRange->count;
        if (i == lastItemPlus1) {
            // Move the end of the last range one item to the back.
            ++lastRange->count;
        } else if (i > lastItemPlus1) {
            // Append a new range.
            lastRange = m_itemRanges.insert(rangeEnd, KItemRange(i, 1));
        }

        return iterator(lastRange, i - lastRange->index);
    }

    // We know that i is between the smallest existing item and the first item
    // of the last range. Find the lowest range whose 'index' is smaller than i.
    KItemRangeList::iterator low = rangeBegin;
    KItemRangeList::iterator high = lastRange;

    while (low + 1 != high) {
        const int span = high - low;
        Q_ASSERT(span >= 2);

        KItemRangeList::iterator mid = low + span / 2;
        if (mid->index > i) {
            high = mid;
        } else {
            low = mid;
        }
    }

    Q_ASSERT(low->index <= i && high->index > i);

    if (i == low->index + low->count) {
        // i is just one item behind the range low.
        if (i == high->index - 1) {
            // i closes the gap between low and high. Merge the two ranges.
            const int newRangeCount = low->count + 1 + high->count;
            KItemRangeList::iterator behindNewRange = m_itemRanges.erase(high);
            KItemRangeList::iterator newRange = behindNewRange - 1;
            newRange->count = newRangeCount;
            return iterator(newRange, i - newRange->index);
        } else {
            // Extend low by one item.
            ++low->count;
            return iterator(low, low->count - 1);
        }
    } else if (i > low->index + low->count) {
        if (i == high->index - 1) {
            // Extend high by one item to the front.
            --high->index;
            ++high->count;
            return iterator(high, 0);
        } else {
            // Insert a new range between low and high.
            KItemRangeList::iterator newRange = m_itemRanges.insert(high, KItemRange(i, 1));
            return iterator(newRange, 0);
        }
    } else {
        // The range low already contains i.
        return iterator(low, i - low->index);
    }
}

KItemSet::iterator KItemSet::erase(iterator it)
{
    KItemRangeList::iterator rangeIt = it.m_rangeIt;

    if (it.m_offset == 0) {
        // Removed index is at the beginning of a range.
        if (rangeIt->count > 1) {
            ++rangeIt->index;
            --rangeIt->count;
        } else {
            // The range only contains the removed index.
            rangeIt = m_itemRanges.erase(rangeIt);
        }
        return iterator(rangeIt, 0);
    } else if (it.m_offset == rangeIt->count - 1) {
        // Removed index is at the end of a range.
        --rangeIt->count;
        ++rangeIt;
        return iterator(rangeIt, 0);
    } else {
        // The removed index is in the middle of a range.
        const int newRangeIndex = *it + 1;
        const int newRangeCount = rangeIt->count - it.m_offset - 1;
        const KItemRange newRange(newRangeIndex, newRangeCount);

        rangeIt->count = it.m_offset;
        ++rangeIt;
        rangeIt = m_itemRanges.insert(rangeIt, newRange);

        return iterator(rangeIt, 0);
    }
}

KItemSet KItemSet::operator+(const KItemSet& other) const
{
    KItemSet sum;

    KItemRangeList::const_iterator it1 = m_itemRanges.constBegin();
    KItemRangeList::const_iterator it2 = other.m_itemRanges.constBegin();

    const KItemRangeList::const_iterator end1 = m_itemRanges.constEnd();
    const KItemRangeList::const_iterator end2 = other.m_itemRanges.constEnd();

    while (it1 != end1 || it2 != end2) {
        if (it1 == end1) {
            // We are past the end of 'this' already. Append all remaining
            // item ranges from 'other'.
            while (it2 != end2) {
                sum.m_itemRanges.append(*it2);
                ++it2;
            }
        } else if (it2 == end2) {
            // We are past the end of 'other' already. Append all remaining
            // item ranges from 'this'.
            while (it1 != end1) {
                sum.m_itemRanges.append(*it1);
                ++it1;
            }
        } else {
            // Find the beginning of the next range.
            int index = qMin(it1->index, it2->index);
            int count = 0;

            do {
                if (it1 != end1 && it1->index <= index + count) {
                    // The next range from 'this' overlaps with the current range in the sum.
                    count = qMax(count, it1->index + it1->count - index);
                    ++it1;
                }

                if (it2 != end2 && it2->index <= index + count) {
                    // The next range from 'other' overlaps with the current range in the sum.
                    count = qMax(count, it2->index + it2->count - index);
                    ++it2;
                }
            } while ((it1 != end1 && it1->index <= index + count)
                    || (it2 != end2 && it2->index <= index + count));

            sum.m_itemRanges.append(KItemRange(index, count));
        }
    }

    return sum;
}

KItemSet KItemSet::operator^(const KItemSet& other) const
{
    // We are looking for all ints which are either in *this or in other,
    // but not in both.
    KItemSet result;

    // When we go through all integers from INT_MIN to INT_MAX and start
    // in the state "do not add to result", every beginning/end of a range
    // of *this and other toggles the "add/do not add to result" state.
    // Therefore, we just have to put ints where any range starts/ends to
    // a sorted array, and then we can calculate the result quite easily.
    QVector<int> rangeBoundaries;
    rangeBoundaries.resize(2 * (m_itemRanges.count() + other.m_itemRanges.count()));
    const QVector<int>::iterator begin = rangeBoundaries.begin();
    const QVector<int>::iterator end = rangeBoundaries.end();
    QVector<int>::iterator it = begin;

    foreach (const KItemRange& range, m_itemRanges) {
        *it++ = range.index;
        *it++ = range.index + range.count;
    }

    const QVector<int>::iterator middle = it;

    foreach (const KItemRange& range, other.m_itemRanges) {
        *it++ = range.index;
        *it++ = range.index + range.count;
    }
    Q_ASSERT(it == end);

    std::inplace_merge(begin, middle, end);

    it = begin;
    while (it != end) {
        const int rangeBegin = *it;
        ++it;

        if (*it == rangeBegin) {
            // It seems that ranges from both *this and other start at
            // rangeBegin. Do not start a new range, but read the next int.
            //
            // Example: Consider the symmetric difference of the sets
            // {1, 2, 3, 4} and {1, 2}. The sorted list of range boundaries is
            // 1 1 3 5. Discarding the duplicate 1 yields the result
            // rangeBegin = 3, rangeEnd = 5, which corresponds to the set {3, 4}.
            ++it;
        } else {
            // The end of the current range is the next *single* int that we
            // find. If an int appears twice in rangeBoundaries, the range does
            // not end.
            //
            // Example: Consider the symmetric difference of the sets
            // {1, 2, 3, 4, 8, 9, 10} and {5, 6, 7}. The sorted list of range
            // boundaries is 1 5 5 8 8 11, and discarding all duplicates yields
            // the result rangeBegin = 1, rangeEnd = 11, which corresponds to
            // the set {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}.
            bool foundEndOfRange = false;
            int rangeEnd;
            do {
                rangeEnd = *it;
                ++it;

                if (it == end || *it != rangeEnd) {
                    foundEndOfRange = true;
                } else {
                    ++it;
                }
            } while (!foundEndOfRange);

            result.m_itemRanges.append(KItemRange(rangeBegin, rangeEnd - rangeBegin));
        }
    }

    return result;
}

bool KItemSet::isValid() const
{
    const KItemRangeList::const_iterator begin = m_itemRanges.constBegin();
    const KItemRangeList::const_iterator end = m_itemRanges.constEnd();

    for (KItemRangeList::const_iterator it = begin; it != end; ++it) {
        if (it->count <= 0) {
            return false;
        }

        if (it != begin) {
            const KItemRangeList::const_iterator previous = it - 1;
            if (previous->index + previous->count >= it->index) {
                return false;
            }
        }
    }

    return true;
}

KItemRangeList::iterator KItemSet::rangeForItem(int i)
{
    const KItemRangeList::iterator end = m_itemRanges.end();
    KItemRangeList::iterator low = m_itemRanges.begin();
    KItemRangeList::iterator high = end;

    if (low == end || low->index > i) {
        return end;
    }

    while (low != high && low + 1 != high) {
        KItemRangeList::iterator mid = low + (high - low) / 2;
        if (mid->index > i) {
            high = mid;
        } else {
            low = mid;
        }
    }

    Q_ASSERT(low->index <= i);
    if (low->index + low->count > i) {
        return low;
    }

    return end;
}

KItemRangeList::const_iterator KItemSet::constRangeForItem(int i) const
{
    const KItemRangeList::const_iterator end = m_itemRanges.constEnd();
    KItemRangeList::const_iterator low = m_itemRanges.constBegin();
    KItemRangeList::const_iterator high = end;

    if (low == end || low->index > i) {
        return end;
    }

    while (low != high && low + 1 != high) {
        KItemRangeList::const_iterator mid = low + (high - low) / 2;
        if (mid->index > i) {
            high = mid;
        } else {
            low = mid;
        }
    }

    Q_ASSERT(low->index <= i);
    if (low->index + low->count > i) {
        return low;
    }

    return end;
}
