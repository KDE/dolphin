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

#ifndef KITEMLISTVIEWLAYOUTER_H
#define KITEMLISTVIEWLAYOUTER_H

#include <libdolphin_export.h>

#include <QObject>
#include <QRectF>
#include <QSet>
#include <QSizeF>

class KItemModelBase;
class KItemListSizeHintResolver;

class LIBDOLPHINPRIVATE_EXPORT KItemListViewLayouter : public QObject
{
    Q_OBJECT

public:
    KItemListViewLayouter(QObject* parent = 0);
    virtual ~KItemListViewLayouter();

    void setScrollOrientation(Qt::Orientation orientation);
    Qt::Orientation scrollOrientation() const;

    void setSize(const QSizeF& size);
    QSizeF size() const;

    void setItemSize(const QSizeF& size);
    QSizeF itemSize() const;

    /**
     * Sets the height of the header that is always aligned
     * at the top. A height of <= 0.0 means that no header is
     * used.
     */
    void setHeaderHeight(qreal height);
    qreal headerHeight() const;

    // TODO: add note that offset can be < 0 or > maximumOffset!
    void setScrollOffset(qreal scrollOffset);
    qreal scrollOffset() const;

    qreal maximumScrollOffset() const;

    void setItemOffset(qreal scrollOffset);
    qreal itemOffset() const;

    qreal maximumItemOffset() const;

    void setModel(const KItemModelBase* model);
    const KItemModelBase* model() const;

    void setSizeHintResolver(const KItemListSizeHintResolver* sizeHintResolver);
    const KItemListSizeHintResolver* sizeHintResolver() const;


    // TODO: mention that return value is -1 if count == 0
    int firstVisibleIndex() const;

    // TODO: mention that return value is -1 if count == 0
    int lastVisibleIndex() const;

    QRectF itemBoundingRect(int index) const;

    int maximumVisibleItems() const;

    int itemsPerOffset() const;

    /**
     * @return True if the item with the index \p itemIndex
     *         is the first item within a group.
     */
    bool isFirstGroupItem(int itemIndex) const;

    void markAsDirty();

private:
    void doLayout();
    void updateVisibleIndexes();
    bool createGroupHeaders();

private:
    bool m_dirty;
    bool m_visibleIndexesDirty;

    Qt::Orientation m_scrollOrientation;
    QSizeF m_size;

    QSizeF m_itemSize;
    qreal m_headerHeight;
    const KItemModelBase* m_model;
    const KItemListSizeHintResolver* m_sizeHintResolver;

    qreal m_scrollOffset;
    qreal m_maximumScrollOffset;

    qreal m_itemOffset;
    qreal m_maximumItemOffset;

    int m_firstVisibleIndex;
    int m_lastVisibleIndex;

    qreal m_columnWidth;
    qreal m_xPosInc;
    int m_columnCount;

    QList<int> m_groups;

    // Stores all item indexes that are the first item of a group.
    // Assures fast access for KItemListViewLayouter::isFirstGroupItem().
    QSet<int> m_groupIndexes;

    QList<QRectF> m_itemBoundingRects;
};

#endif


