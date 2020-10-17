/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTSIZEHINTRESOLVER_H
#define KITEMLISTSIZEHINTRESOLVER_H

#include "dolphin_export.h"
#include "kitemviews/kitemmodelbase.h"

#include <QSizeF>
#include <QVector>

class KItemListView;

/**
 * @brief Calculates and caches the sizehints of items in KItemListView.
 */
class DOLPHIN_EXPORT KItemListSizeHintResolver
{
public:
    explicit KItemListSizeHintResolver(const KItemListView* itemListView);
    virtual ~KItemListSizeHintResolver();
    QSizeF minSizeHint();
    QSizeF sizeHint(int index);

    void itemsInserted(const KItemRangeList& itemRanges);
    void itemsRemoved(const KItemRangeList& itemRanges);
    void itemsMoved(const KItemRange& range, const QList<int>& movedToIndexes);
    void itemsChanged(int index, int count, const QSet<QByteArray>& roles);

    void clearCache();
    void updateCache();

private:
    const KItemListView* m_itemListView;
    mutable QVector<qreal> m_logicalHeightHintCache;
    mutable qreal m_logicalWidthHint;
    mutable qreal m_logicalHeightHint;
    mutable qreal m_minHeightHint;
    bool m_needsResolving;
};

#endif
