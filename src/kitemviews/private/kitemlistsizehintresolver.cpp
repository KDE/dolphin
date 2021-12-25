/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistsizehintresolver.h"
#include "kitemviews/kitemlistview.h"

KItemListSizeHintResolver::KItemListSizeHintResolver(const KItemListView* itemListView) :
    m_itemListView(itemListView),
    m_logicalHeightHintCache(),
    m_logicalWidthHint(0.0),
    m_minHeightHint(0.0),
    m_needsResolving(false)
{
}

KItemListSizeHintResolver::~KItemListSizeHintResolver()
{
}

QSizeF KItemListSizeHintResolver::minSizeHint()
{
    updateCache();
    return QSizeF(m_logicalWidthHint, m_minHeightHint);
}

QSizeF KItemListSizeHintResolver::sizeHint(int index)
{
    updateCache();
    return QSizeF(m_logicalWidthHint, m_logicalHeightHintCache.at(index).first);
}

bool KItemListSizeHintResolver::isElided(int index)
{
    return m_logicalHeightHintCache.at(index).second;
}

void KItemListSizeHintResolver::itemsInserted(const KItemRangeList& itemRanges)
{
    int insertedCount = 0;
    for (const KItemRange& range : itemRanges) {
        insertedCount += range.count;
    }

    const int currentCount = m_logicalHeightHintCache.count();
    m_logicalHeightHintCache.reserve(currentCount + insertedCount);

    // We build the new list from the end to the beginning to mimize the
    // number of moves.
    m_logicalHeightHintCache.insert(m_logicalHeightHintCache.end(), insertedCount, std::make_pair(0.0, false));

    int sourceIndex = currentCount - 1;
    int targetIndex = m_logicalHeightHintCache.count() - 1;
    int itemsToInsertBeforeCurrentRange = insertedCount;

    for (int rangeIndex = itemRanges.count() - 1; rangeIndex >= 0; --rangeIndex) {
        const KItemRange& range = itemRanges.at(rangeIndex);
        itemsToInsertBeforeCurrentRange -= range.count;

        // First: move all existing items that must be put behind 'range'.
        while (targetIndex >= itemsToInsertBeforeCurrentRange + range.index + range.count) {
            m_logicalHeightHintCache[targetIndex] = m_logicalHeightHintCache[sourceIndex];
            --sourceIndex;
            --targetIndex;
        }

        // Then: insert QSizeF() for the items which are inserted into 'range'.
        while (targetIndex >= itemsToInsertBeforeCurrentRange + range.index) {
            m_logicalHeightHintCache[targetIndex] = std::make_pair(0.0, false);
            --targetIndex;
        }
    }

    m_needsResolving = true;

    Q_ASSERT(m_logicalHeightHintCache.count() == m_itemListView->model()->count());
}

void KItemListSizeHintResolver::itemsRemoved(const KItemRangeList& itemRanges)
{
    const QVector<std::pair<qreal, bool>>::iterator begin = m_logicalHeightHintCache.begin();
    const QVector<std::pair<qreal, bool>>::iterator end = m_logicalHeightHintCache.end();

    KItemRangeList::const_iterator rangeIt = itemRanges.constBegin();
    const KItemRangeList::const_iterator rangeEnd = itemRanges.constEnd();

    QVector<std::pair<qreal, bool>>::iterator destIt = begin + rangeIt->index;
    QVector<std::pair<qreal, bool>>::iterator srcIt = destIt + rangeIt->count;

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

    m_logicalHeightHintCache.erase(destIt, end);

    // Note that the cache size might temporarily not match the model size if
    // this function is called from KItemListView::setModel() to empty the cache.
    if (!m_logicalHeightHintCache.isEmpty() && m_itemListView->model()) {
        Q_ASSERT(m_logicalHeightHintCache.count() == m_itemListView->model()->count());
    }
}

void KItemListSizeHintResolver::itemsMoved(const KItemRange& range, const QList<int>& movedToIndexes)
{
    QVector<std::pair<qreal, bool>> newLogicalHeightHintCache(m_logicalHeightHintCache);

    const int movedRangeEnd = range.index + range.count;
    for (int i = range.index; i < movedRangeEnd; ++i) {
        const int newIndex = movedToIndexes.at(i - range.index);
        newLogicalHeightHintCache[newIndex] = m_logicalHeightHintCache.at(i);
    }

    m_logicalHeightHintCache = newLogicalHeightHintCache;
}

void KItemListSizeHintResolver::itemsChanged(int index, int count, const QSet<QByteArray>& roles)
{
    Q_UNUSED(roles)
    while (count) {
        m_logicalHeightHintCache[index] = std::make_pair(0.0, false);
        ++index;
        --count;
    }

    m_needsResolving = true;
}

void KItemListSizeHintResolver::clearCache()
{
    m_logicalHeightHintCache.fill(std::make_pair(0.0, false));
    m_needsResolving = true;
}

void KItemListSizeHintResolver::updateCache()
{
    if (m_needsResolving) {
        m_itemListView->calculateItemSizeHints(m_logicalHeightHintCache, m_logicalWidthHint);
        m_needsResolving = false;
    }
}
