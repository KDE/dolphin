/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistviewlayouter.h"
#include "dolphindebug.h"
#include "kitemlistsizehintresolver.h"
#include "kitemviews/kitemmodelbase.h"

#include <QScopeGuard>

// #define KITEMLISTVIEWLAYOUTER_DEBUG

KItemListViewLayouter::KItemListViewLayouter(KItemListSizeHintResolver* sizeHintResolver, QObject* parent) :
    QObject(parent),
    m_dirty(true),
    m_visibleIndexesDirty(true),
    m_scrollOrientation(Qt::Vertical),
    m_size(),
    m_itemSize(128, 128),
    m_itemMargin(),
    m_headerHeight(0),
    m_model(nullptr),
    m_sizeHintResolver(sizeHintResolver),
    m_scrollOffset(0),
    m_maximumScrollOffset(0),
    m_itemOffset(0),
    m_maximumItemOffset(0),
    m_firstVisibleIndex(-1),
    m_lastVisibleIndex(-1),
    m_columnWidth(0),
    m_xPosInc(0),
    m_columnCount(0),
    m_rowOffsets(),
    m_columnOffsets(),
    m_groupItemIndexes(),
    m_groupHeaderHeight(0),
    m_groupHeaderMargin(0),
    m_itemInfos()
{
    Q_ASSERT(m_sizeHintResolver);
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
        if (m_scrollOrientation == Qt::Vertical) {
            if (m_size.width() != size.width()) {
                m_dirty = true;
            }
        } else if (m_size.height() != size.height()) {
            m_dirty = true;
        }

        m_size = size;
        m_visibleIndexesDirty = true;
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

void KItemListViewLayouter::setItemMargin(const QSizeF& margin)
{
    if (m_itemMargin != margin) {
        m_itemMargin = margin;
        m_dirty = true;
    }
}

QSizeF KItemListViewLayouter::itemMargin() const
{
    return m_itemMargin;
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

void KItemListViewLayouter::setGroupHeaderMargin(qreal margin)
{
    if (m_groupHeaderMargin != margin) {
        m_groupHeaderMargin = margin;
        m_dirty = true;
    }
}

qreal KItemListViewLayouter::groupHeaderMargin() const
{
    return m_groupHeaderMargin;
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
    if (index < 0 || index >= m_itemInfos.count()) {
        return QRectF();
    }

    QSizeF sizeHint = m_sizeHintResolver->sizeHint(index);

    const qreal x = m_columnOffsets.at(m_itemInfos.at(index).column);
    const qreal y = m_rowOffsets.at(m_itemInfos.at(index).row);

    if (m_scrollOrientation == Qt::Horizontal) {
        // Rotate the logical direction which is always vertical by 90°
        // to get the physical horizontal direction
        QPointF pos(y, x);
        pos.rx() -= m_scrollOffset;
        sizeHint.transpose();
        return QRectF(pos, sizeHint);
    }

    if (sizeHint.width() <= 0) {
        // In Details View, a size hint with negative width is used internally.
        sizeHint.rwidth() = m_itemSize.width();
    }

    const QPointF pos(x - m_itemOffset, y - m_scrollOffset);
    return QRectF(pos, sizeHint);
}

QRectF KItemListViewLayouter::groupHeaderRect(int index) const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();

    const QRectF firstItemRect = itemRect(index);
    QPointF pos = firstItemRect.topLeft();
    if (pos.isNull()) {
        return QRectF();
    }

    QSizeF size;
    if (m_scrollOrientation == Qt::Vertical) {
        pos.rx() = 0;
        pos.ry() -= m_groupHeaderHeight;
        size = QSizeF(m_size.width(), m_groupHeaderHeight);
    } else {
        pos.rx() -= m_itemMargin.width();
        pos.ry() = 0;

        // Determine the maximum width used in the current column. As the
        // scroll-direction is Qt::Horizontal and m_itemRects is accessed
        // directly, the logical height represents the visual width, and
        // the logical row represents the column.
        qreal headerWidth = minimumGroupHeaderWidth();
        const int row = m_itemInfos[index].row;
        const int maxIndex = m_itemInfos.count() - 1;
        while (index <= maxIndex) {
            if (m_itemInfos[index].row != row) {
                break;
            }

            const qreal itemWidth = (m_scrollOrientation == Qt::Vertical)
                                     ? m_sizeHintResolver->sizeHint(index).width()
                                     : m_sizeHintResolver->sizeHint(index).height();

            if (itemWidth > headerWidth) {
                headerWidth = itemWidth;
            }

            ++index;
        }

        size = QSizeF(headerWidth, m_size.height());
    }
    return QRectF(pos, size);
}

int KItemListViewLayouter::itemColumn(int index) const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    if (index < 0 || index >= m_itemInfos.count()) {
        return -1;
    }

    return (m_scrollOrientation == Qt::Vertical)
            ? m_itemInfos[index].column
            : m_itemInfos[index].row;
}

int KItemListViewLayouter::itemRow(int index) const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    if (index < 0 || index >= m_itemInfos.count()) {
        return -1;
    }

    return (m_scrollOrientation == Qt::Vertical)
            ? m_itemInfos[index].row
            : m_itemInfos[index].column;
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

bool KItemListViewLayouter::isFirstGroupItem(int itemIndex) const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    return m_groupItemIndexes.contains(itemIndex);
}

void KItemListViewLayouter::markAsDirty()
{
    m_dirty = true;
}


#ifndef QT_NO_DEBUG
    bool KItemListViewLayouter::isDirty()
    {
        return m_dirty;
    }
#endif

void KItemListViewLayouter::doLayout()
{
    // we always want to update visible indexes after performing a layout
    auto qsg = qScopeGuard([this] { updateVisibleIndexes(); });

    if (!m_dirty) {
        return;
    }

#ifdef KITEMLISTVIEWLAYOUTER_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif
    m_visibleIndexesDirty = true;

    QSizeF itemSize = m_itemSize;
    QSizeF itemMargin = m_itemMargin;
    QSizeF size = m_size;

    const bool grouped = createGroupHeaders();

    const bool horizontalScrolling = (m_scrollOrientation == Qt::Horizontal);
    if (horizontalScrolling) {
        // Flip everything so that the layout logically can work like having
        // a vertical scrolling
        itemSize.transpose();
        itemMargin.transpose();
        size.transpose();

        if (grouped) {
            // In the horizontal scrolling case all groups are aligned
            // at the top, which decreases the available height. For the
            // flipped data this means that the width must be decreased.
            size.rwidth() -= m_groupHeaderHeight;
        }
    }

    m_columnWidth = itemSize.width() + itemMargin.width();
    const qreal widthForColumns = size.width() - itemMargin.width();
    m_columnCount = qMax(1, int(widthForColumns / m_columnWidth));
    m_xPosInc = itemMargin.width();

    const int itemCount = m_model->count();
    if (itemCount > m_columnCount && m_columnWidth >= 32) {
        // Apply the unused width equally to each column
        const qreal unusedWidth = widthForColumns - m_columnCount * m_columnWidth;
        if (unusedWidth > 0) {
            const qreal columnInc = unusedWidth / (m_columnCount + 1);
            m_columnWidth += columnInc;
            m_xPosInc += columnInc;
        }
    }

    m_itemInfos.resize(itemCount);

    // Calculate the offset of each column, i.e., the x-coordinate where the column starts.
    m_columnOffsets.resize(m_columnCount);
    qreal currentOffset = m_xPosInc;

    if (grouped && horizontalScrolling) {
        // All group headers will always be aligned on the top and not
        // flipped like the other properties.
        currentOffset += m_groupHeaderHeight;
    }

    for (int column = 0; column < m_columnCount; ++column) {
        m_columnOffsets[column] = currentOffset;
        currentOffset += m_columnWidth;
    }

    // Prepare the QVector which stores the y-coordinate for each new row.
    int numberOfRows = (itemCount + m_columnCount - 1) / m_columnCount;
    if (grouped && m_columnCount > 1) {
        // In the worst case, a new row will be started for every group.
        // We could calculate the exact number of rows now to prevent that we reserve
        // too much memory, but the code required to do that might need much more
        // memory than it would save in the average case.
        numberOfRows += m_groupItemIndexes.count();
    }
    m_rowOffsets.resize(numberOfRows);

    qreal y = m_headerHeight + itemMargin.height();
    int row = 0;

    int index = 0;
    while (index < itemCount) {
        qreal maxItemHeight = itemSize.height();

        if (grouped) {
            if (m_groupItemIndexes.contains(index)) {
                // The item is the first item of a group.
                // Increase the y-position to provide space
                // for the group header.
                if (index > 0) {
                    // Only add a margin if there has been added another
                    // group already before
                    y += m_groupHeaderMargin;
                } else if (!horizontalScrolling) {
                    // The first group header should be aligned on top
                    y -= itemMargin.height();
                }

                if (!horizontalScrolling) {
                    y += m_groupHeaderHeight;
                }
            }
        }

        m_rowOffsets[row] = y;

        int column = 0;
        while (index < itemCount && column < m_columnCount) {
            qreal requiredItemHeight = itemSize.height();
            const QSizeF sizeHint = m_sizeHintResolver->sizeHint(index);
            const qreal sizeHintHeight = sizeHint.height();
            if (sizeHintHeight > requiredItemHeight) {
                requiredItemHeight = sizeHintHeight;
            }

            ItemInfo& itemInfo = m_itemInfos[index];
            itemInfo.column = column;
            itemInfo.row = row;

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
                const qreal headerWidth = minimumGroupHeaderWidth();
                if (requiredItemHeight < headerWidth) {
                    requiredItemHeight = headerWidth;
                }
            }

            maxItemHeight = qMax(maxItemHeight, requiredItemHeight);
            ++index;
            ++column;

            if (grouped && m_groupItemIndexes.contains(index)) {
                // The item represents the first index of a group
                // and must aligned in the first column
                break;
            }
        }

        y += maxItemHeight + itemMargin.height();
        ++row;
    }

    if (itemCount > 0) {
        m_maximumScrollOffset = y;
        m_maximumItemOffset = m_columnCount * m_columnWidth;
    } else {
        m_maximumScrollOffset = 0;
        m_maximumItemOffset = 0;
    }

#ifdef KITEMLISTVIEWLAYOUTER_DEBUG
    qCDebug(DolphinDebug) << "[TIME] doLayout() for " << m_model->count() << "items:" << timer.elapsed();
#endif
    m_dirty = false;
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

    // Calculate the first visible index that is fully visible
    int min = 0;
    int max = maxIndex;
    int mid = 0;
    do {
        mid = (min + max) / 2;
        if (m_rowOffsets.at(m_itemInfos[mid].row) < m_scrollOffset) {
            min = mid + 1;
        } else {
            max = mid - 1;
        }
    } while (min <= max);

    if (mid > 0) {
        // Include the row before the first fully visible index, as it might
        // be partly visible
        if (m_rowOffsets.at(m_itemInfos[mid].row) >= m_scrollOffset) {
            --mid;
            Q_ASSERT(m_rowOffsets.at(m_itemInfos[mid].row) < m_scrollOffset);
        }

        const int firstVisibleRow = m_itemInfos[mid].row;
        while (mid > 0 && m_itemInfos[mid - 1].row == firstVisibleRow) {
            --mid;
        }
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
        if (m_rowOffsets.at(m_itemInfos[mid].row) <= bottom) {
            min = mid + 1;
        } else {
            max = mid - 1;
        }
    } while (min <= max);

    while (mid > 0 && m_rowOffsets.at(m_itemInfos[mid].row) > bottom) {
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

qreal KItemListViewLayouter::minimumGroupHeaderWidth() const
{
    return 100;
}

