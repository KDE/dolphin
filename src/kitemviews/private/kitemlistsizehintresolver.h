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

#ifndef KITEMLISTSIZEHINTRESOLVER_H
#define KITEMLISTSIZEHINTRESOLVER_H

#include "dolphin_export.h"

#include <kitemviews/kitemmodelbase.h>
#include <QSizeF>
#include <QVector>

class KItemListView;

/**
 * @brief Calculates and caches the sizehints of items in KItemListView.
 */
class DOLPHIN_EXPORT KItemListSizeHintResolver
{
public:
    KItemListSizeHintResolver(const KItemListView* itemListView);
    virtual ~KItemListSizeHintResolver();
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
    bool m_needsResolving;
};

#endif
