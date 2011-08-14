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

#include "kitemlistviewlayouter_p.h"

#include "kitemmodelbase.h"
#include "kitemlistsizehintresolver_p.h"

#include <KDebug>

#define KITEMLISTVIEWLAYOUTER_DEBUG

namespace {
    // TODO
    const int HeaderHeight = 50;
};

KItemListViewLayouter::KItemListViewLayouter(QObject* parent) :
    QObject(parent),
    m_dirty(true),
    m_visibleIndexesDirty(true),
    m_grouped(false),
    m_scrollOrientation(Qt::Vertical),
    m_size(),
    m_itemSize(128, 128),
    m_model(0),
    m_sizeHintResolver(0),
    m_offset(0),
    m_maximumOffset(0),
    m_firstVisibleIndex(-1),
    m_lastVisibleIndex(-1),
    m_firstVisibleGroupIndex(-1),
    m_columnWidth(0),
    m_xPosInc(0),
    m_columnCount(0),
    m_groups(),
    m_groupIndexes(),
    m_itemBoundingRects()
{
}

KItemListViewLayouter::~KItemListViewLayouter()
{
}

void KItemListViewLayouter::setScrollOrientation(Qt::Orientation orientation)
{
    if (m_scrollOrientation != orientation) {
        m_scrollOrientation = orientation;
        m_dirty = true;
    }
}

Qt::Orientation KItemListViewLayouter::scrollOrientation() const
{
    return m_scrollOrientation;
}

void KItemListViewLayouter::setSize(const QSizeF& size)
{
    if (m_size != size) {
        m_size = size;
        m_dirty = true;
    }
}

QSizeF KItemListViewLayouter::size() const
{
    return m_size;
}

void KItemListViewLayouter::setItemSize(const QSizeF& size)
{
    if (m_itemSize != size) {
        m_itemSize = size;
        m_dirty = true;
    }
}

QSizeF KItemListViewLayouter::itemSize() const
{
    return m_itemSize;
}

void KItemListViewLayouter::setOffset(qreal offset)
{
    if (m_offset != offset) {
        m_offset = offset;
        m_visibleIndexesDirty = true;
    }
}

qreal KItemListViewLayouter::offset() const
{
    return m_offset;
}

void KItemListViewLayouter::setModel(const KItemModelBase* model)
{
    if (m_model != model) {
        m_model = model;
        m_dirty = true;
    }
}

const KItemModelBase* KItemListViewLayouter::model() const
{
    return m_model;
}

void KItemListViewLayouter::setSizeHintResolver(const KItemListSizeHintResolver* sizeHintResolver)
{
    if (m_sizeHintResolver != sizeHintResolver) {
        m_sizeHintResolver = sizeHintResolver;
        m_dirty = true;
    }
}

const KItemListSizeHintResolver* KItemListViewLayouter::sizeHintResolver() const
{
    return m_sizeHintResolver;
}

qreal KItemListViewLayouter::maximumOffset() const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    return m_maximumOffset;
}

int KItemListViewLayouter::firstVisibleIndex() const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    return m_firstVisibleIndex;
}

int KItemListViewLayouter::lastVisibleIndex() const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    return m_lastVisibleIndex;
}

QRectF KItemListViewLayouter::itemBoundingRect(int index) const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    if (index < 0 || index >= m_itemBoundingRects.count()) {
        return QRectF();
    }

    if (m_scrollOrientation == Qt::Horizontal) {
        // Rotate the logical direction which is always vertical by 90°
        // to get the physical horizontal direction
        const QRectF& b = m_itemBoundingRects[index];
        QRectF bounds(b.y(), b.x(), b.height(), b.width());
        QPointF pos = bounds.topLeft();
        pos.rx() -= m_offset;
        bounds.moveTo(pos);
        return bounds;
    }

    QRectF bounds = m_itemBoundingRects[index];
    QPointF pos = bounds.topLeft();
    pos.ry() -= m_offset;
    bounds.moveTo(pos);
    return bounds;
}

int KItemListViewLayouter::maximumVisibleItems() const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();

    const int height = static_cast<int>(m_size.height());
    const int rowHeight = static_cast<int>(m_itemSize.height());
    int rows = height / rowHeight;
    if (height % rowHeight != 0) {
        ++rows;
    }

    return rows * m_columnCount;
}

int KItemListViewLayouter::itemsPerOffset() const
{
    return m_columnCount;
}

bool KItemListViewLayouter::isFirstGroupItem(int itemIndex) const
{
    return m_groupIndexes.contains(itemIndex);
}

void KItemListViewLayouter::markAsDirty()
{
    m_dirty = true;
}

void KItemListViewLayouter::doLayout()
{
    if (m_dirty) {
#ifdef KITEMLISTVIEWLAYOUTER_DEBUG
        QElapsedTimer timer;
        timer.start();
#endif

        m_visibleIndexesDirty = true;

        QSizeF itemSize = m_itemSize;
        QSizeF size = m_size;

        const bool horizontalScrolling = (m_scrollOrientation == Qt::Horizontal);
        if (horizontalScrolling) {
            itemSize.setWidth(m_itemSize.height());
            itemSize.setHeight(m_itemSize.width());
            size.setWidth(m_size.height());
            size.setHeight(m_size.width());
        }

        m_columnWidth = itemSize.width();
        m_columnCount = qMax(1, int(size.width() / m_columnWidth));
        m_xPosInc = 0;

        const int itemCount = m_model->count();
        if (itemCount > m_columnCount) {
            // Apply the unused width equally to each column
            const qreal unusedWidth = size.width() - m_columnCount * m_columnWidth;
            const qreal columnInc = unusedWidth / (m_columnCount + 1);
            m_columnWidth += columnInc;
            m_xPosInc += columnInc;
        }

        int rowCount = itemCount / m_columnCount;
        if (itemCount % m_columnCount != 0) {
            ++rowCount;
        }

        m_itemBoundingRects.reserve(itemCount);

        qreal y = 0;
        int rowIndex = 0;

        int index = 0;
        while (index < itemCount) {
            qreal x = m_xPosInc;
            qreal maxItemHeight = itemSize.height();

            int column = 0;
            while (index < itemCount && column < m_columnCount) {
                qreal requiredItemHeight = itemSize.height();
                if (m_sizeHintResolver) {
                    const QSizeF sizeHint = m_sizeHintResolver->sizeHint(index);
                    const qreal sizeHintHeight = horizontalScrolling ? sizeHint.width() : sizeHint.height();
                    if (sizeHintHeight > requiredItemHeight) {
                        requiredItemHeight = sizeHintHeight;
                    }
                }

                const QRectF bounds(x, y, itemSize.width(), requiredItemHeight);
                if (index < m_itemBoundingRects.count()) {
                    m_itemBoundingRects[index] = bounds;
                } else {
                    m_itemBoundingRects.append(bounds);
                }

                maxItemHeight = qMax(maxItemHeight, requiredItemHeight);
                x += m_columnWidth;
                ++index;
                ++column;
            }

            y += maxItemHeight;
            ++rowIndex;
        }
        if (m_itemBoundingRects.count() > itemCount) {
            m_itemBoundingRects.erase(m_itemBoundingRects.begin() + itemCount,
                                      m_itemBoundingRects.end());
        }

        m_maximumOffset = (itemCount > 0) ? m_itemBoundingRects.last().bottom() : 0;

        m_grouped = !m_model->groupRole().isEmpty();
        /*if (m_grouped) {
            createGroupHeaders();

            const int lastGroupItemCount = m_model->count() - m_groups.last().firstItemIndex;
            m_maximumOffset = m_groups.last().y + (lastGroupItemCount / m_columnCount) * m_rowHeight;
            if (lastGroupItemCount % m_columnCount != 0) {
                m_maximumOffset += m_rowHeight;
            }
        } else {*/
         //   m_maximumOffset = m_minimumRowHeight * rowCount;
        //}

#ifdef KITEMLISTVIEWLAYOUTER_DEBUG
        kDebug() << "[TIME] doLayout() for " << m_model->count() << "items:" << timer.elapsed();
#endif
        m_dirty = false;
    }

    if (m_grouped) {
        updateGroupedVisibleIndexes();
    } else {
        updateVisibleIndexes();
    }
}

void KItemListViewLayouter::updateVisibleIndexes()
{
    if (!m_visibleIndexesDirty) {
        return;
    }

    Q_ASSERT(!m_grouped);
    Q_ASSERT(!m_dirty);

    if (m_model->count() <= 0) {
        m_firstVisibleIndex = -1;
        m_lastVisibleIndex = -1;
        m_visibleIndexesDirty = false;
        return;
    }

    const bool horizontalScrolling = (m_scrollOrientation == Qt::Horizontal);
    const int minimumHeight =  horizontalScrolling ? m_itemSize.width()
                                                   : m_itemSize.height();

    // Calculate the first visible index:
    // 1. Guess the index by using the minimum row height
    const int maxIndex = m_model->count() - 1;
    m_firstVisibleIndex = int(m_offset / minimumHeight) * m_columnCount;

    // 2. Decrease the index by checking the real row heights
    int prevRowIndex = m_firstVisibleIndex - m_columnCount;
    while (prevRowIndex > maxIndex) {
        prevRowIndex -= m_columnCount;
    }

    while (prevRowIndex >= 0 && m_itemBoundingRects[prevRowIndex].bottom() >= m_offset) {
        m_firstVisibleIndex = prevRowIndex;
        prevRowIndex -= m_columnCount;
    }
    m_firstVisibleIndex = qBound(0, m_firstVisibleIndex, maxIndex);

    // Calculate the last visible index
    const int visibleHeight = horizontalScrolling ? m_size.width() : m_size.height();
    const qreal bottom = m_offset + visibleHeight;
    m_lastVisibleIndex = m_firstVisibleIndex; // first visible row, first column
    int nextRowIndex = m_lastVisibleIndex + m_columnCount;
    while (nextRowIndex <= maxIndex && m_itemBoundingRects[nextRowIndex].y() <= bottom) {
        m_lastVisibleIndex = nextRowIndex;
        nextRowIndex += m_columnCount;
    }
    m_lastVisibleIndex += m_columnCount - 1; // move it to the last column
    m_lastVisibleIndex = qBound(0, m_lastVisibleIndex, maxIndex);

    m_visibleIndexesDirty = false;
}

void KItemListViewLayouter::updateGroupedVisibleIndexes()
{
    if (!m_visibleIndexesDirty) {
        return;
    }

    Q_ASSERT(m_grouped);
    Q_ASSERT(!m_dirty);

    if (m_model->count() <= 0) {
        m_firstVisibleIndex = -1;
        m_lastVisibleIndex = -1;
        m_visibleIndexesDirty = false;
        return;
    }

    // Find the first visible group
    const int lastGroupIndex = m_groups.count() - 1;
    int groupIndex = lastGroupIndex;
    for (int i = 1; i < m_groups.count(); ++i) {
        if (m_groups[i].y >= m_offset) {
            groupIndex = i - 1;
            break;
        }
    }

    // Calculate the first visible index
    qreal groupY = m_groups[groupIndex].y;
    m_firstVisibleIndex = m_groups[groupIndex].firstItemIndex;
    const int invisibleRowCount = int(m_offset - groupY) / int(m_itemSize.height());
    m_firstVisibleIndex += invisibleRowCount * m_columnCount;
    if (groupIndex + 1 <= lastGroupIndex) {
        // Check whether the calculated first visible index remains inside the current
        // group. If this is not the case let the first element of the next group be the first
        // visible index.
        const int nextGroupIndex = m_groups[groupIndex + 1].firstItemIndex;
        if (m_firstVisibleIndex > nextGroupIndex) {
            m_firstVisibleIndex = nextGroupIndex;
        }
    }

    m_firstVisibleGroupIndex = groupIndex;

    const int maxIndex = m_model->count() - 1;
    m_firstVisibleIndex = qBound(0, m_firstVisibleIndex, maxIndex);

    // Calculate the last visible index: Find group where the last visible item is shown.
    const qreal visibleBottom = m_offset + m_size.height(); // TODO: respect Qt::Horizontal alignment
    while ((groupIndex < lastGroupIndex) && (m_groups[groupIndex + 1].y < visibleBottom)) {
        ++groupIndex;
    }

    groupY = m_groups[groupIndex].y;
    m_lastVisibleIndex = m_groups[groupIndex].firstItemIndex;
    const int availableHeight = static_cast<int>(visibleBottom - groupY);
    int visibleRowCount = availableHeight / int(m_itemSize.height());
    if (availableHeight % int(m_itemSize.height()) != 0) {
        ++visibleRowCount;
    }
    m_lastVisibleIndex += visibleRowCount * m_columnCount - 1;

    if (groupIndex + 1 <= lastGroupIndex) {
        // Check whether the calculate last visible index remains inside the current group.
        // If this is not the case let the last element of this group be the last visible index.
        const int nextGroupIndex = m_groups[groupIndex + 1].firstItemIndex;
        if (m_lastVisibleIndex >= nextGroupIndex) {
            m_lastVisibleIndex = nextGroupIndex - 1;
        }
    }
    //Q_ASSERT(m_lastVisibleIndex < m_model->count());
    m_lastVisibleIndex = qBound(0, m_lastVisibleIndex, maxIndex);

    m_visibleIndexesDirty = false;
}

void KItemListViewLayouter::createGroupHeaders()
{
    m_groups.clear();
    m_groupIndexes.clear();

    // TODO:
    QList<int> numbers;
    numbers << 0 << 5 << 6 << 13 << 20 << 25 << 30 << 35 << 50;

    qreal y = 0;
    for (int i = 0; i < numbers.count(); ++i) {
        if (i > 0) {
            const int previousGroupItemCount = numbers[i] - m_groups.last().firstItemIndex;
            int previousGroupRowCount = previousGroupItemCount / m_columnCount;
            if (previousGroupItemCount % m_columnCount != 0) {
                ++previousGroupRowCount;
            }
            const qreal previousGroupHeight = previousGroupRowCount * m_itemSize.height();
            y += previousGroupHeight;
        }
        y += HeaderHeight;

        ItemGroup itemGroup;
        itemGroup.firstItemIndex = numbers[i];
        itemGroup.y = y;

        m_groups.append(itemGroup);
        m_groupIndexes.insert(itemGroup.firstItemIndex);
    }
}

#include "kitemlistviewlayouter_p.moc"
