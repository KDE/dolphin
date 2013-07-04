/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "kitemlistsizehintresolver.h"

#include <kitemviews/kitemlistview.h>

KItemListSizeHintResolver::KItemListSizeHintResolver(const KItemListView* itemListView) :
    m_itemListView(itemListView),
    m_sizeHintCache()
{
}

KItemListSizeHintResolver::~KItemListSizeHintResolver()
{
}

QSizeF KItemListSizeHintResolver::sizeHint(int index) const
{
    QSizeF size = m_sizeHintCache.at(index);
    if (size.isEmpty()) {
        size = m_itemListView->itemSizeHint(index);
        m_sizeHintCache[index] = size;
    }
    return size;
}

void KItemListSizeHintResolver::itemsInserted(const KItemRangeList& itemRanges)
{
    int insertedCount = 0;
    foreach (const KItemRange& range, itemRanges) {
        insertedCount += range.count;
    }

    const int currentCount = m_sizeHintCache.count();
    m_sizeHintCache.reserve(currentCount + insertedCount);

    // We build the new list from the end to the beginning to mimize the
    // number of moves.
    m_sizeHintCache.insert(m_sizeHintCache.end(), insertedCount, QSizeF());

    int sourceIndex = currentCount - 1;
    int targetIndex = m_sizeHintCache.count() - 1;
    int itemsToInsertBeforeCurrentRange = insertedCount;

    for (int rangeIndex = itemRanges.count() - 1; rangeIndex >= 0; --rangeIndex) {
        const KItemRange& range = itemRanges.at(rangeIndex);
        itemsToInsertBeforeCurrentRange -= range.count;

        // First: move all existing items that must be put behind 'range'.
        while (targetIndex >= itemsToInsertBeforeCurrentRange + range.index + range.count) {
            m_sizeHintCache[targetIndex] = m_sizeHintCache[sourceIndex];
            --sourceIndex;
            --targetIndex;
        }

        // Then: insert QSizeF() for the items which are inserted into 'range'.
        while (targetIndex >= itemsToInsertBeforeCurrentRange + range.index) {
            m_sizeHintCache[targetIndex] = QSizeF();
            --targetIndex;
        }
    }

    Q_ASSERT(m_sizeHintCache.count() == m_itemListView->model()->count());
}

void KItemListSizeHintResolver::itemsRemoved(const KItemRangeList& itemRanges)
{
    const QVector<QSizeF>::iterator begin = m_sizeHintCache.begin();
    const QVector<QSizeF>::iterator end = m_sizeHintCache.end();

    KItemRangeList::const_iterator rangeIt = itemRanges.constBegin();
    const KItemRangeList::const_iterator rangeEnd = itemRanges.constEnd();

    QVector<QSizeF>::iterator destIt = begin + rangeIt->index;
    QVector<QSizeF>::iterator srcIt = destIt + rangeIt->count;

    ++rangeIt;

    while (srcIt != end) {
        *destIt = *srcIt;
        ++destIt;
        ++srcIt;

        if (rangeIt != rangeEnd && srcIt == begin + rangeIt->index) {
            // Skip the items in the next removed range.
            srcIt += rangeIt->count;
            ++rangeIt;
        }
    }

    m_sizeHintCache.erase(destIt, end);

    // Note that the cache size might temporarily not match the model size if
    // this function is called from KItemListView::setModel() to empty the cache.
    if (!m_sizeHintCache.isEmpty() && m_itemListView->model()) {
        Q_ASSERT(m_sizeHintCache.count() == m_itemListView->model()->count());
    }
}

void KItemListSizeHintResolver::itemsMoved(int index, int count)
{
    while (count) {
        m_sizeHintCache[index] = QSizeF();
        ++index;
        --count;
    }
}

void KItemListSizeHintResolver::itemsChanged(int index, int count, const QSet<QByteArray>& roles)
{
    Q_UNUSED(roles);
    while (count) {
        m_sizeHintCache[index] = QSizeF();
        ++index;
        --count;
    }
}

void KItemListSizeHintResolver::clearCache()
{
    const int count = m_sizeHintCache.count();
    for (int i = 0; i < count; ++i) {
        m_sizeHintCache[i] = QSizeF();
    }
}
