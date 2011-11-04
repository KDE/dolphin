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

#include "kitemlistsizehintresolver_p.h"

#include <kitemviews/kitemlistview.h>
#include <KDebug>

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

void KItemListSizeHintResolver::itemsInserted(int index, int count)
{
    const int currentCount = m_sizeHintCache.count();
    m_sizeHintCache.reserve(currentCount + count);
    while (count > 0) {
        m_sizeHintCache.insert(index, QSizeF());
        ++index;
        --count;
    }
}

void KItemListSizeHintResolver::itemsRemoved(int index, int count)
{
    const QList<QSizeF>::iterator begin = m_sizeHintCache.begin() + index;
    const QList<QSizeF>::iterator end = begin + count;
    m_sizeHintCache.erase(begin, end);
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
