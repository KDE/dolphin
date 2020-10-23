/*
 * SPDX-FileCopyrightText: 2013 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */


#ifndef KITEMSET_H
#define KITEMSET_H

#include "dolphin_export.h"
#include "kitemviews/kitemrange.h"

/**
 * @brief Stores a set of integer numbers in a space-efficient way.
 *
 * This class is similar to QSet<int>, but it has the following advantages:
 *
 * 1. It uses less memory than a QSet<int> if many consecutive numbers are
 *    stored. This is achieved by not storing each number separately, but
 *    "ranges" of numbers.
 *
 *    Example: The set {1, 2, 3, 4, 5} is represented by a single range which
 *    starts at 1 and has the length 5.
 *
 * 2. When iterating through a KItemSet using KItemSet::iterator or
 *    KItemSet::const_iterator, the numbers are traversed in ascending order.
 *
 * The complexity of most operations depends on the number of ranges.
 */

class DOLPHIN_EXPORT KItemSet
{
public:
    KItemSet();
    KItemSet(const KItemSet& other);
    ~KItemSet();
    KItemSet& operator=(const KItemSet& other);

    /**
     * Returns the number of items in the set.
     * Complexity: O(log(number of ranges)).
     */
    int count() const;

    bool isEmpty() const;
    void clear();

    bool operator==(const KItemSet& other) const;
    bool operator!=(const KItemSet& other) const;

    class iterator
    {
        iterator(const KItemRangeList::iterator& rangeIt, int offset) :
            m_rangeIt(rangeIt),
            m_offset(offset)
        {
        }

    public:
        iterator(const iterator& other) :
            m_rangeIt(other.m_rangeIt),
            m_offset(other.m_offset)
        {
        }

        iterator& operator=(const iterator& other)
        {
            m_rangeIt = other.m_rangeIt;
            m_offset = other.m_offset;
            return *this;
        }

        ~iterator() = default;

        int operator*() const
        {
            return m_rangeIt->index + m_offset;
        }

        inline bool operator==(const iterator& other) const
        {
            return m_rangeIt == other.m_rangeIt && m_offset == other.m_offset;
        }

        inline bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }

        inline iterator& operator++()
        {
            ++m_offset;

            if (m_offset == m_rangeIt->count) {
                ++m_rangeIt;
                m_offset = 0;
            }

            return *this;
        }

        inline iterator operator++(int)
        {
            iterator r = *this;
            ++(*this);
            return r;
        }

        inline iterator& operator--()
        {
            if (m_offset == 0) {
                --m_rangeIt;
                m_offset = m_rangeIt->count - 1;
            } else {
                --m_offset;
            }

            return *this;
        }

        inline iterator operator--(int)
        {
            iterator r = *this;
            --(*this);
            return r;
        }

    private:
        KItemRangeList::iterator m_rangeIt;
        int m_offset;

        friend class const_iterator;
        friend class KItemSet;
    };


    class const_iterator
    {
        const_iterator(KItemRangeList::const_iterator rangeIt, int offset) :
            m_rangeIt(rangeIt),
            m_offset(offset)
        {
        }

    public:
        const_iterator(const const_iterator& other) :
            m_rangeIt(other.m_rangeIt),
            m_offset(other.m_offset)
        {
        }

        explicit const_iterator(const iterator& other) :
            m_rangeIt(other.m_rangeIt),
            m_offset(other.m_offset)
        {
        }

        const_iterator& operator=(const const_iterator& other)
        {
            m_rangeIt = other.m_rangeIt;
            m_offset = other.m_offset;
            return *this;
        }

        ~const_iterator() = default;

        int operator*() const
        {
            return m_rangeIt->index + m_offset;
        }

        inline bool operator==(const const_iterator& other) const
        {
            return m_rangeIt == other.m_rangeIt && m_offset == other.m_offset;
        }

        inline bool operator!=(const const_iterator& other) const
        {
            return !(*this == other);
        }

        inline const_iterator& operator++()
        {
            ++m_offset;

            if (m_offset == m_rangeIt->count) {
                ++m_rangeIt;
                m_offset = 0;
            }

            return *this;
        }

        inline const_iterator operator++(int)
        {
            const_iterator r = *this;
            ++(*this);
            return r;
        }

        inline const_iterator& operator--()
        {
            if (m_offset == 0) {
                --m_rangeIt;
                m_offset = m_rangeIt->count - 1;
            } else {
                --m_offset;
            }

            return *this;
        }

        inline const_iterator operator--(int)
        {
            const_iterator r = *this;
            --(*this);
            return r;
        }

    private:
        KItemRangeList::const_iterator m_rangeIt;
        int m_offset;

        friend class KItemSet;
    };

    iterator begin();
    const_iterator begin() const;
    const_iterator constBegin() const;
    iterator end();
    const_iterator end() const;
    const_iterator constEnd() const;

    int first() const;
    int last() const;

    bool contains(int i) const;
    iterator insert(int i);
    iterator find(int i);
    const_iterator constFind(int i) const;
    bool remove(int i);
    iterator erase(iterator it);

    /**
     * Returns a new set which contains all items that are contained in this
     * KItemSet, in \a other, or in both.
     */
    KItemSet operator+(const KItemSet& other) const;

    /**
     * Returns a new set which contains all items that are contained either in
     * this KItemSet, or in \a other, but not in both (the symmetric difference
     * of both KItemSets).
     */
    KItemSet operator^(const KItemSet& other) const;

    KItemSet& operator<<(int i);

private:
    /**
     * Returns true if the KItemSet is valid, and false otherwise.
     * A valid KItemSet must store the item ranges in ascending order, and
     * the ranges must not overlap.
     */
    bool isValid() const;

    /**
     * This function returns an iterator that points to the KItemRange which
     * contains i, or m_itemRanges.end() if no such range exists.
     */
    KItemRangeList::iterator rangeForItem(int i);

    /**
     * This function returns an iterator that points to the KItemRange which
     * contains i, or m_itemRanges.constEnd() if no such range exists.
     */
    KItemRangeList::const_iterator constRangeForItem(int i) const;

    KItemRangeList m_itemRanges;

    friend class KItemSetTest;
};

inline KItemSet::KItemSet() :
    m_itemRanges()
{
}

inline KItemSet::KItemSet(const KItemSet& other) :
    m_itemRanges(other.m_itemRanges)
{
}

inline KItemSet::~KItemSet() = default;

inline KItemSet& KItemSet::operator=(const KItemSet& other)
{
    m_itemRanges=other.m_itemRanges;
    return *this;
}

inline int KItemSet::count() const
{
    int result = 0;
    for (const KItemRange& range : qAsConst(m_itemRanges)) {
        result += range.count;
    }
    return result;
}

inline bool KItemSet::isEmpty() const
{
    return m_itemRanges.isEmpty();
}

inline void KItemSet::clear()
{
    m_itemRanges.clear();
}

inline bool KItemSet::operator==(const KItemSet& other) const
{
    return m_itemRanges == other.m_itemRanges;
}

inline bool KItemSet::operator!=(const KItemSet& other) const
{
    return m_itemRanges != other.m_itemRanges;
}

inline bool KItemSet::contains(int i) const
{
    const KItemRangeList::const_iterator it = constRangeForItem(i);
    return it != m_itemRanges.end();
}

inline KItemSet::iterator KItemSet::find(int i)
{
    const KItemRangeList::iterator it = rangeForItem(i);
    if (it != m_itemRanges.end()) {
        return iterator(it, i - it->index);
    } else {
        return end();
    }
}

inline KItemSet::const_iterator KItemSet::constFind(int i) const
{
    const KItemRangeList::const_iterator it = constRangeForItem(i);
    if (it != m_itemRanges.constEnd()) {
        return const_iterator(it, i - it->index);
    } else {
        return constEnd();
    }
}

inline bool KItemSet::remove(int i)
{
    iterator it = find(i);
    if (it != end()) {
        erase(it);
        return true;
    } else {
        return false;
    }
}

inline KItemSet::iterator KItemSet::begin()
{
    return iterator(m_itemRanges.begin(), 0);
}

inline KItemSet::const_iterator KItemSet::begin() const
{
    return const_iterator(m_itemRanges.begin(), 0);
}

inline KItemSet::const_iterator KItemSet::constBegin() const
{
    return const_iterator(m_itemRanges.constBegin(), 0);
}

inline KItemSet::iterator KItemSet::end()
{
    return iterator(m_itemRanges.end(), 0);
}

inline KItemSet::const_iterator KItemSet::end() const
{
    return const_iterator(m_itemRanges.end(), 0);
}

inline KItemSet::const_iterator KItemSet::constEnd() const
{
    return const_iterator(m_itemRanges.constEnd(), 0);
}

inline int KItemSet::first() const
{
    return m_itemRanges.first().index;
}

inline int KItemSet::last() const
{
    const KItemRange& lastRange = m_itemRanges.last();
    return lastRange.index + lastRange.count - 1;
}

inline KItemSet& KItemSet::operator<<(int i)
{
    insert(i);
    return *this;
}

#endif
