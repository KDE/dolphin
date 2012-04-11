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

#include <libdolphin_export.h>

#include <QByteArray>
#include <QList>
#include <QSizeF>

class KItemListView;

/**
 * @brief Calculates and caches the sizehints of items in KItemListView.
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListSizeHintResolver
{
public:
    KItemListSizeHintResolver(const KItemListView* itemListView);
    virtual ~KItemListSizeHintResolver();
    QSizeF sizeHint(int index) const;

    void itemsInserted(int index, int count);
    void itemsRemoved(int index, int count);
    void itemsMoved(int index, int count);
    void itemsChanged(int index, int count, const QSet<QByteArray>& roles);

    void clearCache();

private:
    const KItemListView* m_itemListView;
    mutable QList<QSizeF> m_sizeHintCache;
};

#endif
