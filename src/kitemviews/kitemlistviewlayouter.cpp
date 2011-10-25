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

KItemListViewLayouter::KItemListViewLayouter(QObject* parent) :
    QObject(parent),
    m_dirty(true),
    m_visibleIndexesDirty(true),
    m_scrollOrientation(Qt::Vertical),
    m_size(),
    m_itemSize(128, 128),
    m_headerHeight(0),
    m_model(0),
    m_sizeHintResolver(0),
    m_scrollOffset(0),
    m_maximumScrollOffset(0),
    m_itemOffset(0),
    m_maximumItemOffset(0),
    m_firstVisibleIndex(-1),
    m_lastVisibleIndex(-1),
    m_columnWidth(0),
    m_xPosInc(0),
    m_columnCount(0),
    m_groupItemIndexes(),
    m_groupHeaderHeight(0),
    m_itemRects()
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

void KItemListViewLayouter::setHeaderHeight(qreal height)
{
    if (m_headerHeight != height) {
        m_headerHeight = height;
        m_dirty = true;
    }
}

qreal KItemListViewLayouter::headerHeight() const
{
    return m_headerHeight;
}

void KItemListViewLayouter::setGroupHeaderHeight(qreal height)
{
    if (m_groupHeaderHeight != height) {
        m_groupHeaderHeight = height;
        m_dirty = true;
    }
}

qreal KItemListViewLayouter::groupHeaderHeight() const
{
    return m_groupHeaderHeight;
}

void KItemListViewLayouter::setScrollOffset(qreal offset)
{
    if (m_scrollOffset != offset) {
        m_scrollOffset = offset;
        m_visibleIndexesDirty = true;
    }
}

qreal KItemListViewLayouter::scrollOffset() const
{
    return m_scrollOffset;
}

qreal KItemListViewLayouter::maximumScrollOffset() const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    return m_maximumScrollOffset;
}

void KItemListViewLayouter::setItemOffset(qreal offset)
{
    if (m_itemOffset != offset) {
        m_itemOffset = offset;
        m_visibleIndexesDirty = true;
    }
}

qreal KItemListViewLayouter::itemOffset() const
{
    return m_itemOffset;
}

qreal KItemListViewLayouter::maximumItemOffset() const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    return m_maximumItemOffset;
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

QRectF KItemListViewLayouter::itemRect(int index) const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    if (index < 0 || index >= m_itemRects.count()) {
        return QRectF();
    }

    if (m_scrollOrientation == Qt::Horizontal) {
        // Rotate the logical direction which is always vertical by 90°
        // to get the physical horizontal direction
        const QRectF& b = m_itemRects[index];
        QRectF bounds(b.y(), b.x(), b.height(), b.width());
        QPointF pos = bounds.topLeft();
        pos.rx() -= m_scrollOffset;
        bounds.moveTo(pos);
        return bounds;
    }

    QRectF bounds = m_itemRects[index];
    bounds.moveTo(bounds.topLeft() - QPointF(m_itemOffset, m_scrollOffset));
    return bounds;
}

QRectF KItemListViewLayouter::groupHeaderRect(int index) const
{
    const QRectF firstItemRect = itemRect(index);
    QPointF pos = firstItemRect.topLeft();
    if (pos.isNull()) {
        return QRectF();
    }

    pos.ry() -= m_groupHeaderHeight;

    QSizeF size;
    if (m_scrollOrientation == Qt::Vertical) {
        pos.rx() = 0;
        size = QSizeF(m_size.width(), m_groupHeaderHeight);
    } else {
        size = QSizeF(firstItemRect.width(), m_groupHeaderHeight);
    }
    return QRectF(pos, size);
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
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    return m_groupItemIndexes.contains(itemIndex);
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
            if (unusedWidth > 0) {
                // [Comment #1] A cast to int is done on purpose to prevent rounding issues when
                // drawing pixmaps and drawing text or other graphic primitives: Qt uses a different
                // rastering algorithm for the upper/left of pixmaps
                const qreal columnInc = int(unusedWidth / (m_columnCount + 1));
                m_columnWidth += columnInc;
                m_xPosInc += columnInc;
            }
        }

        int rowCount = itemCount / m_columnCount;
        if (itemCount % m_columnCount != 0) {
            ++rowCount;
        }

        m_itemRects.reserve(itemCount);

        qreal y = m_headerHeight;
        int rowIndex = 0;

        const bool grouped = createGroupHeaders();

        int index = 0;
        while (index < itemCount) {
            qreal x = m_xPosInc;
            qreal maxItemHeight = itemSize.height();

            if (grouped) {
                if (horizontalScrolling) {
                    // All group headers will always be aligned on the top and not
                    // flipped like the other properties
                    x += m_groupHeaderHeight;
                }

                if (m_groupItemIndexes.contains(index)) {
                    if (!horizontalScrolling) {
                        // The item is the first item of a group.
                        // Increase the y-position to provide space
                        // for the group header.
                        y += m_groupHeaderHeight;
                    }
                }
            }

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
                if (index < m_itemRects.count()) {
                    m_itemRects[index] = bounds;
                } else {
                    m_itemRects.append(bounds);
                }

                if (grouped && horizontalScrolling) {
                    // When grouping is enabled in the horizontal mode, the header alignment
                    // looks like this:
                    //   Header-1 Header-2 Header-3
                    //   Item 1   Item 4   Item 7
                    //   Item 2   Item 5   Item 8
                    //   Item 3   Item 6   Item 9
                    // In this case 'requiredItemHeight' represents the column-width. We don't
                    // check the content of the header in the layouter to determine the required
                    // width, hence assure that at least a minimal width of 15 characters is given
                    // (in average a character requires the halve width of the font height).
                    //
                    // TODO: Let the group headers provide a minimum width and respect this width here
                    const qreal minimumGroupHeaderWidth = int(m_groupHeaderHeight * 15 / 2); // See [Comment #1]
                    if (requiredItemHeight < minimumGroupHeaderWidth) {
                        requiredItemHeight = minimumGroupHeaderWidth;
                    }
                }

                maxItemHeight = qMax(maxItemHeight, requiredItemHeight);
                x += m_columnWidth;
                ++index;
                ++column;

                if (grouped && m_groupItemIndexes.contains(index)) {
                    // The item represents the first index of a group
                    // and must aligned in the first column
                    break;
                }
            }

            y += maxItemHeight;
            ++rowIndex;
        }
        if (m_itemRects.count() > itemCount) {
            m_itemRects.erase(m_itemRects.begin() + itemCount,
                                      m_itemRects.end());
        }

        if (itemCount > 0) {
            m_maximumScrollOffset = m_itemRects.last().bottom();
            m_maximumItemOffset = m_columnCount * m_columnWidth;
        } else {
            m_maximumScrollOffset = 0;
            m_maximumItemOffset = 0;
        }

#ifdef KITEMLISTVIEWLAYOUTER_DEBUG
        kDebug() << "[TIME] doLayout() for " << m_model->count() << "items:" << timer.elapsed();
#endif
        m_dirty = false;
    }

    updateVisibleIndexes();
}

void KItemListViewLayouter::updateVisibleIndexes()
{
    if (!m_visibleIndexesDirty) {
        return;
    }

    Q_ASSERT(!m_dirty);

    if (m_model->count() <= 0) {
        m_firstVisibleIndex = -1;
        m_lastVisibleIndex = -1;
        m_visibleIndexesDirty = false;
        return;
    }

    const int maxIndex = m_model->count() - 1;

    // Calculate the first visible index that is (at least partly) visible
    int min = 0;
    int max = maxIndex;
    int mid = 0;
    do {
        mid = (min + max) / 2;
        if (m_itemRects[mid].bottom() < m_scrollOffset) {
            min = mid + 1;
        } else {
            max = mid - 1;
        }
    } while (min <= max);

    while (mid < maxIndex && m_itemRects[mid].bottom() < m_scrollOffset) {
        ++mid;
    }
    m_firstVisibleIndex = mid;

    // Calculate the last visible index that is (at least partly) visible
    const int visibleHeight = (m_scrollOrientation == Qt::Horizontal) ? m_size.width() : m_size.height();
    qreal bottom = m_scrollOffset + visibleHeight;
    if (m_model->groupedSorting()) {
        bottom += m_groupHeaderHeight;
    }

    min = m_firstVisibleIndex;
    max = maxIndex;
    do {
        mid = (min + max) / 2;
        if (m_itemRects[mid].y() <= bottom) {
            min = mid + 1;
        } else {
            max = mid - 1;
        }
    } while (min <= max);

    while (mid > 0 && m_itemRects[mid].y() > bottom) {
        --mid;
    }
    m_lastVisibleIndex = mid;

    m_visibleIndexesDirty = false;
}

bool KItemListViewLayouter::createGroupHeaders()
{
    if (!m_model->groupedSorting()) {
        return false;
    }

    m_groupItemIndexes.clear();

    const QList<QPair<int, QVariant> > groups = m_model->groups();
    if (groups.isEmpty()) {
        return false;
    }

    for (int i = 0; i < groups.count(); ++i) {
        const int firstItemIndex = groups.at(i).first;
        m_groupItemIndexes.insert(firstItemIndex);
    }

    return true;
}

#include "kitemlistviewlayouter_p.moc"
