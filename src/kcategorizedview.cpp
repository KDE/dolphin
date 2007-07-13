/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@gmail.com>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "kcategorizedview.h"
#include "kcategorizedview_p.h"

#include <math.h> // trunc on C99 compliant systems
#include <kdefakes.h> // trunc for not C99 compliant systems

#include <QApplication>
#include <QPainter>
#include <QScrollBar>
#include <QPaintEvent>

#include <kdebug.h>
#include <kstyle.h>

#include "kitemcategorizer.h"
#include "ksortfilterproxymodel.h"

class LessThan
{
public:
    enum Purpose
    {
        GeneralPurpose = 0,
        CategoryPurpose
    };

    inline LessThan(const KSortFilterProxyModel *proxyModel,
                    Purpose purpose)
        : proxyModel(proxyModel)
        , purpose(purpose)
    {
    }

    inline bool operator()(const QModelIndex &left,
                           const QModelIndex &right) const
    {
        if (purpose == GeneralPurpose)
        {
            return proxyModel->sortOrder() == Qt::AscendingOrder ?
                   proxyModel->lessThanGeneralPurpose(left, right) :
                   !proxyModel->lessThanGeneralPurpose(left, right);
        }

        return proxyModel->sortOrder() == Qt::AscendingOrder ?
               proxyModel->lessThanCategoryPurpose(left, right) :
               !proxyModel->lessThanCategoryPurpose(left, right);
    }

private:
    const KSortFilterProxyModel *proxyModel;
    const Purpose purpose;
};


//==============================================================================


KCategorizedView::Private::Private(KCategorizedView *listView)
    : listView(listView)
    , itemCategorizer(0)
    , biggestItemSize(QSize(0, 0))
    , mouseButtonPressed(false)
    , isDragging(false)
    , dragLeftViewport(false)
    , proxyModel(0)
    , lastIndex(QModelIndex())
{
}

KCategorizedView::Private::~Private()
{
}

const QModelIndexList &KCategorizedView::Private::intersectionSet(const QRect &rect)
{
    QModelIndex index;
    QRect indexVisualRect;

    intersectedIndexes.clear();

    // Lets find out where we should start
    int top = proxyModel->rowCount() - 1;
    int bottom = 0;
    int middle = (top + bottom) / 2;
    while (bottom <= top)
    {
        middle = (top + bottom) / 2;

        index = elementDictionary[proxyModel->index(middle, 0)];
        indexVisualRect = visualRect(index);

        if (qMax(indexVisualRect.topLeft().y(),
                 indexVisualRect.bottomRight().y()) < qMin(rect.topLeft().y(),
                                                        rect.bottomRight().y()))
        {
            bottom = middle + 1;
        }
        else
        {
            top = middle - 1;
        }
    }

    for (int i = middle; i < proxyModel->rowCount(); i++)
    {
        index = elementDictionary[proxyModel->index(i, 0)];
        indexVisualRect = visualRect(index);

        if (rect.intersects(indexVisualRect))
            intersectedIndexes.append(index);

        // If we passed next item, stop searching for hits
        if (qMax(rect.bottomRight().y(), rect.topLeft().y()) <
                                                  qMin(indexVisualRect.topLeft().y(),
                                                       indexVisualRect.bottomRight().y()))
            break;
    }

    return intersectedIndexes;
}

QRect KCategorizedView::Private::visualRectInViewport(const QModelIndex &index) const
{
    if (!index.isValid())
        return QRect();

    QString curCategory = elementsInfo[index].category;

    QRect retRect(listView->spacing(), listView->spacing() * 2 +
                  itemCategorizer->categoryHeight(listView->viewOptions()), 0, 0);

    int viewportWidth = listView->viewport()->width() - listView->spacing();

    int itemHeight = biggestItemSize.height();
    int itemWidth = biggestItemSize.width();
    int itemWidthPlusSeparation = listView->spacing() + itemWidth;
    int elementsPerRow = viewportWidth / itemWidthPlusSeparation;
    if (!elementsPerRow)
        elementsPerRow++;

    int column = elementsInfo[index].relativeOffsetToCategory % elementsPerRow;
    int row = elementsInfo[index].relativeOffsetToCategory / elementsPerRow;

    retRect.setLeft(retRect.left() + column * listView->spacing() +
                    column * itemWidth);

    foreach (const QString &category, categories)
    {
        if (category == curCategory)
            break;

        float rows = (float) ((float) categoriesIndexes[category].count() /
                              (float) elementsPerRow);
        int rowsInt = categoriesIndexes[category].count() / elementsPerRow;

        if (rows - trunc(rows)) rowsInt++;

        retRect.setTop(retRect.top() +
                       (rowsInt * listView->spacing()) +
                       (rowsInt * itemHeight) +
                       itemCategorizer->categoryHeight(listView->viewOptions()) +
                       listView->spacing() * 2);
    }

    retRect.setTop(retRect.top() + row * listView->spacing() +
                   row * itemHeight);

    retRect.setWidth(itemWidth);
    retRect.setHeight(itemHeight);

    return retRect;
}

QRect KCategorizedView::Private::visualCategoryRectInViewport(const QString &category)
                                                                           const
{
    QRect retRect(listView->spacing(),
                  listView->spacing(),
                  listView->viewport()->width() - listView->spacing() * 2,
                  0);

    if (!proxyModel->rowCount() || !categories.contains(category))
        return QRect();

    QModelIndex index = proxyModel->index(0, 0, QModelIndex());

    int viewportWidth = listView->viewport()->width() - listView->spacing();

    int itemHeight = biggestItemSize.height();
    int itemWidth = biggestItemSize.width();
    int itemWidthPlusSeparation = listView->spacing() + itemWidth;
    int elementsPerRow = viewportWidth / itemWidthPlusSeparation;

    if (!elementsPerRow)
        elementsPerRow++;

    foreach (const QString &itCategory, categories)
    {
        if (itCategory == category)
            break;

        float rows = (float) ((float) categoriesIndexes[itCategory].count() /
                              (float) elementsPerRow);
        int rowsInt = categoriesIndexes[itCategory].count() / elementsPerRow;

        if (rows - trunc(rows)) rowsInt++;

        retRect.setTop(retRect.top() +
                       (rowsInt * listView->spacing()) +
                       (rowsInt * itemHeight) +
                       itemCategorizer->categoryHeight(listView->viewOptions()) +
                       listView->spacing() * 2);
    }

    retRect.setHeight(itemCategorizer->categoryHeight(listView->viewOptions()));

    return retRect;
}

// We're sure elementsPosition doesn't contain index
const QRect &KCategorizedView::Private::cacheIndex(const QModelIndex &index)
{
    QRect rect = visualRectInViewport(index);
    elementsPosition[index] = rect;

    return elementsPosition[index];
}

// We're sure categoriesPosition doesn't contain category
const QRect &KCategorizedView::Private::cacheCategory(const QString &category)
{
    QRect rect = visualCategoryRectInViewport(category);
    categoriesPosition[category] = rect;

    return categoriesPosition[category];
}

const QRect &KCategorizedView::Private::cachedRectIndex(const QModelIndex &index)
{
    if (elementsPosition.contains(index)) // If we have it cached
    {                                        // return it
        return elementsPosition[index];
    }
    else                                     // Otherwise, cache it
    {                                        // and return it
        return cacheIndex(index);
    }
}

const QRect &KCategorizedView::Private::cachedRectCategory(const QString &category)
{
    if (categoriesPosition.contains(category)) // If we have it cached
    {                                                // return it
        return categoriesPosition[category];
    }
    else                                            // Otherwise, cache it and
    {                                               // return it
        return cacheCategory(category);
    }
}

QRect KCategorizedView::Private::visualRect(const QModelIndex &index)
{
    QModelIndex mappedIndex = proxyModel->mapToSource(index);

    QRect retRect = cachedRectIndex(mappedIndex);
    int dx = -listView->horizontalOffset();
    int dy = -listView->verticalOffset();
    retRect.adjust(dx, dy, dx, dy);

    return retRect;
}

QRect KCategorizedView::Private::categoryVisualRect(const QString &category)
{
    QRect retRect = cachedRectCategory(category);
    int dx = -listView->horizontalOffset();
    int dy = -listView->verticalOffset();
    retRect.adjust(dx, dy, dx, dy);

    return retRect;
}

void KCategorizedView::Private::drawNewCategory(const QModelIndex &index,
                                         int sortRole,
                                         const QStyleOption &option,
                                         QPainter *painter)
{
    QStyleOption optionCopy = option;
    const QString category = itemCategorizer->categoryForItem(index, sortRole);

    if ((category == hoveredCategory) && !mouseButtonPressed)
    {
        optionCopy.state |= QStyle::State_MouseOver;
    }

    itemCategorizer->drawCategory(index,
                                  sortRole,
                                  optionCopy,
                                  painter);
}


void KCategorizedView::Private::updateScrollbars()
{
    int lastItemBottom = cachedRectIndex(lastIndex).bottom() +
                           listView->spacing() - listView->viewport()->height();

    listView->verticalScrollBar()->setSingleStep(listView->viewport()->height() / 10);
    listView->verticalScrollBar()->setPageStep(listView->viewport()->height());
    listView->verticalScrollBar()->setRange(0, lastItemBottom);
}

void KCategorizedView::Private::drawDraggedItems(QPainter *painter)
{
    QStyleOptionViewItemV3 option = listView->viewOptions();
    option.state &= ~QStyle::State_MouseOver;
    foreach (const QModelIndex &index, listView->selectionModel()->selectedIndexes())
    {
        const int dx = mousePosition.x() - initialPressPosition.x() + listView->horizontalOffset();
        const int dy = mousePosition.y() - initialPressPosition.y() + listView->verticalOffset();

        option.rect = visualRect(index);
        option.rect.adjust(dx, dy, dx, dy);

        if (option.rect.intersects(listView->viewport()->rect()))
        {
            listView->itemDelegate(index)->paint(painter, option, index);
        }
    }
}

void KCategorizedView::Private::drawDraggedItems()
{
    QRect rectToUpdate;
    QRect currentRect;
    foreach (const QModelIndex &index, listView->selectionModel()->selectedIndexes())
    {
        int dx = mousePosition.x() - initialPressPosition.x() + listView->horizontalOffset();
        int dy = mousePosition.y() - initialPressPosition.y() + listView->verticalOffset();

        currentRect = visualRect(index);
        currentRect.adjust(dx, dy, dx, dy);

        if (currentRect.intersects(listView->viewport()->rect()))
        {
            rectToUpdate = rectToUpdate.united(currentRect);
        }
    }

    listView->viewport()->update(lastDraggedItemsRect.united(rectToUpdate));

    lastDraggedItemsRect = rectToUpdate;
}


//==============================================================================


KCategorizedView::KCategorizedView(QWidget *parent)
    : QListView(parent)
    , d(new Private(this))
{
}

KCategorizedView::~KCategorizedView()
{
    delete d;
}

void KCategorizedView::setModel(QAbstractItemModel *model)
{
    d->lastSelection = QItemSelection();
    d->currentViewIndex = QModelIndex();
    d->forcedSelectionPosition = 0;
    d->elementsInfo.clear();
    d->elementsPosition.clear();
    d->elementDictionary.clear();
    d->invertedElementDictionary.clear();
    d->categoriesIndexes.clear();
    d->categoriesPosition.clear();
    d->categories.clear();
    d->intersectedIndexes.clear();
    d->sourceModelIndexList.clear();
    d->hovered = QModelIndex();
    d->mouseButtonPressed = false;

    if (d->proxyModel)
    {
        QObject::disconnect(d->proxyModel,
                            SIGNAL(rowsRemoved(QModelIndex,int,int)),
                            this, SLOT(rowsRemoved(QModelIndex,int,int)));

        QObject::disconnect(d->proxyModel,
                            SIGNAL(sortingRoleChanged()),
                            this, SLOT(slotSortingRoleChanged()));
    }

    QListView::setModel(model);

    d->proxyModel = dynamic_cast<KSortFilterProxyModel*>(model);

    if (d->proxyModel)
    {
        QObject::connect(d->proxyModel,
                         SIGNAL(rowsRemoved(QModelIndex,int,int)),
                         this, SLOT(rowsRemoved(QModelIndex,int,int)));

        QObject::connect(d->proxyModel,
                         SIGNAL(sortingRoleChanged()),
                         this, SLOT(slotSortingRoleChanged()));
    }
}

QRect KCategorizedView::visualRect(const QModelIndex &index) const
{
    if ((viewMode() != KCategorizedView::IconMode) || !d->proxyModel ||
        !d->itemCategorizer)
    {
        return QListView::visualRect(index);
    }

    if (!qobject_cast<const QSortFilterProxyModel*>(index.model()))
    {
        return d->visualRect(d->proxyModel->mapFromSource(index));
    }

    return d->visualRect(index);
}

KItemCategorizer *KCategorizedView::itemCategorizer() const
{
    return d->itemCategorizer;
}

void KCategorizedView::setItemCategorizer(KItemCategorizer *itemCategorizer)
{
    d->lastSelection = QItemSelection();
    d->currentViewIndex = QModelIndex();
    d->forcedSelectionPosition = 0;
    d->elementsInfo.clear();
    d->elementsPosition.clear();
    d->elementDictionary.clear();
    d->invertedElementDictionary.clear();
    d->categoriesIndexes.clear();
    d->categoriesPosition.clear();
    d->categories.clear();
    d->intersectedIndexes.clear();
    d->sourceModelIndexList.clear();
    d->hovered = QModelIndex();
    d->mouseButtonPressed = false;

    if (!itemCategorizer && d->proxyModel)
    {
        QObject::disconnect(d->proxyModel,
                            SIGNAL(rowsRemoved(QModelIndex,int,int)),
                            this, SLOT(rowsRemoved(QModelIndex,int,int)));

        QObject::disconnect(d->proxyModel,
                            SIGNAL(sortingRoleChanged()),
                            this, SLOT(slotSortingRoleChanged()));
    }
    else if (itemCategorizer && d->proxyModel)
    {
        QObject::connect(d->proxyModel,
                         SIGNAL(rowsRemoved(QModelIndex,int,int)),
                         this, SLOT(rowsRemoved(QModelIndex,int,int)));

        QObject::connect(d->proxyModel,
                         SIGNAL(sortingRoleChanged()),
                         this, SLOT(slotSortingRoleChanged()));
    }

    d->itemCategorizer = itemCategorizer;

    if (itemCategorizer)
    {
        rowsInserted(QModelIndex(), 0, d->proxyModel->rowCount() - 1);
    }
    else
    {
        updateGeometries();
    }
}

QModelIndex KCategorizedView::indexAt(const QPoint &point) const
{
    if ((viewMode() != KCategorizedView::IconMode) || !d->proxyModel ||
        !d->itemCategorizer)
    {
        return QListView::indexAt(point);
    }

    QModelIndex index;

    QModelIndexList item = d->intersectionSet(QRect(point, point));

    if (item.count() == 1)
    {
        index = item[0];
    }

    d->hovered = index;

    return index;
}

void KCategorizedView::reset()
{
    QListView::reset();

    d->lastSelection = QItemSelection();
    d->currentViewIndex = QModelIndex();
    d->forcedSelectionPosition = 0;
    d->elementsInfo.clear();
    d->elementsPosition.clear();
    d->elementDictionary.clear();
    d->invertedElementDictionary.clear();
    d->categoriesIndexes.clear();
    d->categoriesPosition.clear();
    d->categories.clear();
    d->intersectedIndexes.clear();
    d->sourceModelIndexList.clear();
    d->hovered = QModelIndex();
    d->biggestItemSize = QSize(0, 0);
    d->mouseButtonPressed = false;
}

void KCategorizedView::paintEvent(QPaintEvent *event)
{
    if ((viewMode() != KCategorizedView::IconMode) || !d->proxyModel ||
        !d->itemCategorizer)
    {
        QListView::paintEvent(event);
        return;
    }

    QStyleOptionViewItemV3 option = viewOptions();
    option.widget = this;
    QPainter painter(viewport());
    QRect area = event->rect();
    const bool focus = (hasFocus() || viewport()->hasFocus()) &&
                        currentIndex().isValid();
    const QStyle::State state = option.state;
    const bool enabled = (state & QStyle::State_Enabled) != 0;

    painter.save();

    QModelIndexList dirtyIndexes = d->intersectionSet(area);
    foreach (const QModelIndex &index, dirtyIndexes)
    {
        option.state = state;
        option.rect = d->visualRect(index);

        if (selectionModel() && selectionModel()->isSelected(index))
        {
            option.state |= QStyle::State_Selected;
        }

        if (enabled)
        {
            QPalette::ColorGroup cg;
            if ((d->proxyModel->flags(index) & Qt::ItemIsEnabled) == 0)
            {
                option.state &= ~QStyle::State_Enabled;
                cg = QPalette::Disabled;
            }
            else
            {
                cg = QPalette::Normal;
            }
            option.palette.setCurrentColorGroup(cg);
        }

        if (focus && currentIndex() == index)
        {
            option.state |= QStyle::State_HasFocus;
            if (this->state() == EditingState)
                option.state |= QStyle::State_Editing;
        }

        if ((index == d->hovered) && !d->mouseButtonPressed)
            option.state |= QStyle::State_MouseOver;
        else
            option.state &= ~QStyle::State_MouseOver;

        itemDelegate(index)->paint(&painter, option, index);
    }

    // Redraw categories
    QStyleOptionViewItem otherOption;
    foreach (const QString &category, d->categories)
    {
        otherOption = option;
        otherOption.rect = d->categoryVisualRect(category);
        otherOption.state &= ~QStyle::State_MouseOver;

        if (otherOption.rect.intersects(area))
        {
            d->drawNewCategory(d->categoriesIndexes[category][0],
                               d->proxyModel->sortRole(), otherOption, &painter);
        }
    }

    if (d->mouseButtonPressed && !d->isDragging)
    {
        QPoint start, end, initialPressPosition;

        initialPressPosition = d->initialPressPosition;

        initialPressPosition.setY(initialPressPosition.y() - verticalOffset());
        initialPressPosition.setX(initialPressPosition.x() - horizontalOffset());

        if (d->initialPressPosition.x() > d->mousePosition.x() ||
            d->initialPressPosition.y() > d->mousePosition.y())
        {
            start = d->mousePosition;
            end = initialPressPosition;
        }
        else
        {
            start = initialPressPosition;
            end = d->mousePosition;
        }

        QStyleOptionRubberBand yetAnotherOption;
        yetAnotherOption.initFrom(this);
        yetAnotherOption.shape = QRubberBand::Rectangle;
        yetAnotherOption.opaque = false;
        yetAnotherOption.rect = QRect(start, end).intersected(viewport()->rect().adjusted(-16, -16, 16, 16));
        painter.save();
        style()->drawControl(QStyle::CE_RubberBand, &yetAnotherOption, &painter);
        painter.restore();
    }

    if (d->isDragging && !d->dragLeftViewport)
    {
        painter.setOpacity(0.5);
        d->drawDraggedItems(&painter);
    }

    painter.restore();
}

void KCategorizedView::resizeEvent(QResizeEvent *event)
{
    QListView::resizeEvent(event);

    // Clear the items positions cache
    d->elementsPosition.clear();
    d->categoriesPosition.clear();
    d->forcedSelectionPosition = 0;

    if ((viewMode() != KCategorizedView::IconMode) || !d->proxyModel ||
        !d->itemCategorizer)
    {
        return;
    }

    d->updateScrollbars();
}

void KCategorizedView::setSelection(const QRect &rect,
                                    QItemSelectionModel::SelectionFlags flags)
{
    if ((viewMode() != KCategorizedView::IconMode) || !d->proxyModel ||
        !d->itemCategorizer)
    {
        QListView::setSelection(rect, flags);
        return;
    }

    if (!flags)
        return;

    selectionModel()->clear();

    if (flags & QItemSelectionModel::Clear)
    {
        d->lastSelection = QItemSelection();
    }

    QModelIndexList dirtyIndexes = d->intersectionSet(rect);

    QItemSelection selection;

    if (!dirtyIndexes.count())
    {
        if (d->lastSelection.count())
        {
            selectionModel()->select(d->lastSelection, flags);
        }

        return;
    }

    if (!d->mouseButtonPressed)
    {
        selection = QItemSelection(dirtyIndexes[0], dirtyIndexes[0]);
        d->currentViewIndex = dirtyIndexes[0];
    }
    else
    {
        QModelIndex first = dirtyIndexes[0];
        QModelIndex last;
        foreach (const QModelIndex &index, dirtyIndexes)
        {
            if (last.isValid() && last.row() + 1 != index.row())
            {
                QItemSelectionRange range(first, last);

                selection << range;

                first = index;
            }

            last = index;
        }

        if (last.isValid())
            selection << QItemSelectionRange(first, last);
    }

    if (d->lastSelection.count() && !d->mouseButtonPressed)
    {
        selection.merge(d->lastSelection, flags);
    }
    else if (d->lastSelection.count())
    {
        selection.merge(d->lastSelection, QItemSelectionModel::Select);
    }

    selectionModel()->select(selection, flags);
}

void KCategorizedView::mouseMoveEvent(QMouseEvent *event)
{
    QListView::mouseMoveEvent(event);

    if ((viewMode() != KCategorizedView::IconMode) || !d->proxyModel ||
        !d->itemCategorizer)
    {
        return;
    }

    const QString previousHoveredCategory = d->hoveredCategory;

    d->mousePosition = event->pos();
    d->hoveredCategory = QString();

    // Redraw categories
    foreach (const QString &category, d->categories)
    {
        if (d->categoryVisualRect(category).intersects(QRect(event->pos(), event->pos())))
        {
            d->hoveredCategory = category;
            viewport()->update(d->categoryVisualRect(category));
        }
        else if ((category == previousHoveredCategory) &&
                 (!d->categoryVisualRect(previousHoveredCategory).intersects(QRect(event->pos(), event->pos()))))
        {
            viewport()->update(d->categoryVisualRect(category));
        }
    }

    QRect rect;
    if (d->mouseButtonPressed && !d->isDragging)
    {
        QPoint start, end, initialPressPosition;

        initialPressPosition = d->initialPressPosition;

        initialPressPosition.setY(initialPressPosition.y() - verticalOffset());
        initialPressPosition.setX(initialPressPosition.x() - horizontalOffset());

        if (d->initialPressPosition.x() > d->mousePosition.x() ||
            d->initialPressPosition.y() > d->mousePosition.y())
        {
            start = d->mousePosition;
            end = initialPressPosition;
        }
        else
        {
            start = initialPressPosition;
            end = d->mousePosition;
        }

        rect = QRect(start, end).intersected(viewport()->rect().adjusted(-16, -16, 16, 16));

        //viewport()->update(rect.united(d->lastSelectionRect));

        d->lastSelectionRect = rect;
    }
}

void KCategorizedView::mousePressEvent(QMouseEvent *event)
{
    d->dragLeftViewport = false;

    if (event->button() == Qt::LeftButton)
    {
        d->mouseButtonPressed = true;

        d->initialPressPosition = event->pos();
        d->initialPressPosition.setY(d->initialPressPosition.y() +
                                                              verticalOffset());
        d->initialPressPosition.setX(d->initialPressPosition.x() +
                                                            horizontalOffset());
    }

    QListView::mousePressEvent(event);
}

void KCategorizedView::mouseReleaseEvent(QMouseEvent *event)
{
    d->mouseButtonPressed = false;

    QListView::mouseReleaseEvent(event);

    if ((viewMode() != KCategorizedView::IconMode) || !d->proxyModel ||
        !d->itemCategorizer)
    {
        return;
    }

    QPoint initialPressPosition = viewport()->mapFromGlobal(QCursor::pos());
    initialPressPosition.setY(initialPressPosition.y() + verticalOffset());
    initialPressPosition.setX(initialPressPosition.x() + horizontalOffset());

    QItemSelection selection;

    if (initialPressPosition == d->initialPressPosition)
    {
        foreach(const QString &category, d->categories)
        {
            if (d->categoryVisualRect(category).contains(event->pos()))
            {
                foreach (const QModelIndex &index, d->categoriesIndexes[category])
                {
                    selection << QItemSelectionRange(d->proxyModel->mapFromSource(index));
                }

                selectionModel()->select(selection, QItemSelectionModel::Select);

                break;
            }
        }
    }

    d->lastSelection = selectionModel()->selection();

    if (d->hovered.isValid())
        viewport()->update(d->visualRect(d->hovered));
    else if (!d->hoveredCategory.isEmpty())
        viewport()->update(d->categoryVisualRect(d->hoveredCategory));
}

void KCategorizedView::leaveEvent(QEvent *event)
{
    d->hovered = QModelIndex();
    d->hoveredCategory = QString();

    QListView::leaveEvent(event);
}

void KCategorizedView::startDrag(Qt::DropActions supportedActions)
{
    QListView::startDrag(supportedActions);

    d->isDragging = false;
    d->mouseButtonPressed = false;

    viewport()->update(d->lastDraggedItemsRect);
}

void KCategorizedView::dragMoveEvent(QDragMoveEvent *event)
{
    d->mousePosition = event->pos();

    if (d->mouseButtonPressed)
    {
        d->isDragging = true;
    }
    else
    {
        d->isDragging = false;
    }

    d->dragLeftViewport = false;

    if ((viewMode() != KCategorizedView::IconMode) || !d->proxyModel ||
        !d->itemCategorizer)
    {
        QListView::dragMoveEvent(event);
        return;
    }

    d->drawDraggedItems();
}

void KCategorizedView::dragLeaveEvent(QDragLeaveEvent *event)
{
    d->dragLeftViewport = true;

    QListView::dragLeaveEvent(event);
}

QModelIndex KCategorizedView::moveCursor(CursorAction cursorAction,
                                  Qt::KeyboardModifiers modifiers)
{
    if ((viewMode() != KCategorizedView::IconMode) || !d->proxyModel ||
        !d->itemCategorizer)
    {
        return QListView::moveCursor(cursorAction, modifiers);
    }

    const QModelIndex current = selectionModel()->currentIndex();

    int viewportWidth = viewport()->width() - spacing();
    int itemWidth = d->biggestItemSize.width();
    int itemWidthPlusSeparation = spacing() + itemWidth;
    int elementsPerRow = viewportWidth / itemWidthPlusSeparation;

    QString lastCategory = d->categories[0];
    QString theCategory = d->categories[0];
    QString afterCategory = d->categories[0];
    bool hasToBreak = false;
    foreach (const QString &category, d->categories)
    {
        if (hasToBreak)
        {
            afterCategory = category;

            break;
        }

        if (category == d->elementsInfo[d->proxyModel->mapToSource(current)].category)
        {
            theCategory = category;

            hasToBreak = true;
        }

        if (!hasToBreak)
        {
            lastCategory = category;
        }
    }

    switch (cursorAction)
    {
        case QAbstractItemView::MoveUp: {
            if (d->elementsInfo[d->proxyModel->mapToSource(current)].relativeOffsetToCategory >= elementsPerRow)
            {
                int indexToMove = d->invertedElementDictionary[current].row();
                indexToMove -= qMin(((d->elementsInfo[d->proxyModel->mapToSource(current)].relativeOffsetToCategory) + d->forcedSelectionPosition), elementsPerRow - d->forcedSelectionPosition + (d->elementsInfo[d->proxyModel->mapToSource(current)].relativeOffsetToCategory % elementsPerRow));

                return d->elementDictionary[d->proxyModel->index(indexToMove, 0)];
            }
            else
            {
                int lastCategoryLastRow = (d->categoriesIndexes[lastCategory].count() - 1) % elementsPerRow;
                int indexToMove = d->invertedElementDictionary[current].row() - d->elementsInfo[d->proxyModel->mapToSource(current)].relativeOffsetToCategory;

                if (d->forcedSelectionPosition >= lastCategoryLastRow)
                {
                    indexToMove -= 1;
                }
                else
                {
                    indexToMove -= qMin((lastCategoryLastRow - d->forcedSelectionPosition + 1), d->forcedSelectionPosition + elementsPerRow + 1);
                }

                return d->elementDictionary[d->proxyModel->index(indexToMove, 0)];
            }
        }

        case QAbstractItemView::MoveDown: {
            if (d->elementsInfo[d->proxyModel->mapToSource(current)].relativeOffsetToCategory < (d->categoriesIndexes[theCategory].count() - 1 - ((d->categoriesIndexes[theCategory].count() - 1) % elementsPerRow)))
            {
                int indexToMove = d->invertedElementDictionary[current].row();
                indexToMove += qMin(elementsPerRow, d->categoriesIndexes[theCategory].count() - 1 - d->elementsInfo[d->proxyModel->mapToSource(current)].relativeOffsetToCategory);

                return d->elementDictionary[d->proxyModel->index(indexToMove, 0)];
            }
            else
            {
                int afterCategoryLastRow = qMin(elementsPerRow, d->categoriesIndexes[afterCategory].count());
                int indexToMove = d->invertedElementDictionary[current].row() + (d->categoriesIndexes[theCategory].count() - d->elementsInfo[d->proxyModel->mapToSource(current)].relativeOffsetToCategory);

                if (d->forcedSelectionPosition >= afterCategoryLastRow)
                {
                    indexToMove += afterCategoryLastRow - 1;
                }
                else
                {
                    indexToMove += qMin(d->forcedSelectionPosition, elementsPerRow);
                }

                return d->elementDictionary[d->proxyModel->index(indexToMove, 0)];
            }
        }

        case QAbstractItemView::MoveLeft:
            d->forcedSelectionPosition = d->elementsInfo[d->proxyModel->mapToSource(d->elementDictionary[d->proxyModel->index(d->invertedElementDictionary[current].row() - 1, 0)])].relativeOffsetToCategory % elementsPerRow;

            if (d->forcedSelectionPosition < 0)
                d->forcedSelectionPosition = (d->categoriesIndexes[theCategory].count() - 1) % elementsPerRow;

            return d->elementDictionary[d->proxyModel->index(d->invertedElementDictionary[current].row() - 1, 0)];

        case QAbstractItemView::MoveRight:
            d->forcedSelectionPosition = d->elementsInfo[d->proxyModel->mapToSource(d->elementDictionary[d->proxyModel->index(d->invertedElementDictionary[current].row() + 1, 0)])].relativeOffsetToCategory % elementsPerRow;

            if (d->forcedSelectionPosition < 0)
                d->forcedSelectionPosition = (d->categoriesIndexes[theCategory].count() - 1) % elementsPerRow;

            return d->elementDictionary[d->proxyModel->index(d->invertedElementDictionary[current].row() + 1, 0)];

        default:
            break;
    }

    return QListView::moveCursor(cursorAction, modifiers);
}

void KCategorizedView::rowsInserted(const QModelIndex &parent,
                             int start,
                             int end)
{
    QListView::rowsInserted(parent, start, end);

    if ((viewMode() != KCategorizedView::IconMode) || !d->proxyModel ||
        !d->itemCategorizer)
    {
        d->lastSelection = QItemSelection();
        d->currentViewIndex = QModelIndex();
        d->forcedSelectionPosition = 0;
        d->elementsInfo.clear();
        d->elementsPosition.clear();
        d->elementDictionary.clear();
        d->invertedElementDictionary.clear();
        d->categoriesIndexes.clear();
        d->categoriesPosition.clear();
        d->categories.clear();
        d->intersectedIndexes.clear();
        d->sourceModelIndexList.clear();
        d->hovered = QModelIndex();
        d->biggestItemSize = QSize(0, 0);
        d->mouseButtonPressed = false;

        return;
    }

    rowsInsertedArtifficial(parent, start, end);
}

void KCategorizedView::rowsInsertedArtifficial(const QModelIndex &parent,
                                        int start,
                                        int end)
{
    Q_UNUSED(parent);

    d->lastSelection = QItemSelection();
    d->currentViewIndex = QModelIndex();
    d->forcedSelectionPosition = 0;
    d->elementsInfo.clear();
    d->elementsPosition.clear();
    d->elementDictionary.clear();
    d->invertedElementDictionary.clear();
    d->categoriesIndexes.clear();
    d->categoriesPosition.clear();
    d->categories.clear();
    d->intersectedIndexes.clear();
    d->sourceModelIndexList.clear();
    d->hovered = QModelIndex();
    d->biggestItemSize = QSize(0, 0);
    d->mouseButtonPressed = false;

    if (start > end || end < 0 || start < 0 || !d->proxyModel->rowCount())
    {
        return;
    }

    // Add all elements mapped to the source model
    for (int k = 0; k < d->proxyModel->rowCount(); k++)
    {
        d->biggestItemSize = QSize(qMax(sizeHintForIndex(d->proxyModel->index(k, 0)).width(),
                                        d->biggestItemSize.width()),
                                   qMax(sizeHintForIndex(d->proxyModel->index(k, 0)).height(),
                                        d->biggestItemSize.height()));

        d->sourceModelIndexList <<
                         d->proxyModel->mapToSource(d->proxyModel->index(k, 0));
    }

    // Sort them with the general purpose lessThan method
    LessThan generalLessThan(d->proxyModel,
                             LessThan::GeneralPurpose);

    qStableSort(d->sourceModelIndexList.begin(), d->sourceModelIndexList.end(),
                generalLessThan);

    // Explore categories
    QString prevCategory =
                 d->itemCategorizer->categoryForItem(d->sourceModelIndexList[0],
                                                     d->proxyModel->sortRole());
    QString lastCategory = prevCategory;
    QModelIndexList modelIndexList;
    struct Private::ElementInfo elementInfo;
    foreach (const QModelIndex &index, d->sourceModelIndexList)
    {
        lastCategory = d->itemCategorizer->categoryForItem(index,
                                                     d->proxyModel->sortRole());

        elementInfo.category = lastCategory;

        if (prevCategory != lastCategory)
        {
            d->categoriesIndexes.insert(prevCategory, modelIndexList);
            d->categories << prevCategory;
            modelIndexList.clear();
        }

        modelIndexList << index;
        prevCategory = lastCategory;

        d->elementsInfo.insert(index, elementInfo);
    }

    d->categoriesIndexes.insert(prevCategory, modelIndexList);
    d->categories << prevCategory;

    // Sort items locally in their respective categories with the category
    // purpose lessThan
    LessThan categoryLessThan(d->proxyModel,
                              LessThan::CategoryPurpose);

    foreach (const QString &key, d->categories)
    {
        QModelIndexList &indexList = d->categoriesIndexes[key];

        qStableSort(indexList.begin(), indexList.end(), categoryLessThan);
    }

    d->lastIndex = d->categoriesIndexes[d->categories[d->categories.count() - 1]][d->categoriesIndexes[d->categories[d->categories.count() - 1]].count() - 1];

    // Finally, fill data information of items situation. This will help when
    // trying to compute an item place in the viewport
    int i = 0; // position relative to the category beginning
    int j = 0; // number of elements before current
    foreach (const QString &key, d->categories)
    {
        foreach (const QModelIndex &index, d->categoriesIndexes[key])
        {
            struct Private::ElementInfo &elementInfo = d->elementsInfo[index];

            elementInfo.relativeOffsetToCategory = i;

            d->elementDictionary.insert(d->proxyModel->index(j, 0),
                                        d->proxyModel->mapFromSource(index));

            d->invertedElementDictionary.insert(d->proxyModel->mapFromSource(index),
                                                d->proxyModel->index(j, 0));

            i++;
            j++;
        }

        i = 0;
    }

    d->updateScrollbars();
}

void KCategorizedView::rowsRemoved(const QModelIndex &parent,
                            int start,
                            int end)
{
    if ((viewMode() == KCategorizedView::IconMode) && d->proxyModel &&
        d->itemCategorizer)
    {
        // Force the view to update all elements
        rowsInsertedArtifficial(parent, start, end);
    }
}

void KCategorizedView::updateGeometries()
{
    if ((viewMode() != KCategorizedView::IconMode) || !d->proxyModel ||
        !d->itemCategorizer)
    {
        QListView::updateGeometries();
        return;
    }

    // Avoid QListView::updateGeometries(), since it will try to set another
    // range to our scroll bars, what we don't want (ereslibre)
    QAbstractItemView::updateGeometries();
}

void KCategorizedView::slotSortingRoleChanged()
{
    if ((viewMode() == KCategorizedView::IconMode) && d->proxyModel &&
        d->itemCategorizer)
    {
        // Force the view to update all elements
        rowsInsertedArtifficial(QModelIndex(), 0, d->proxyModel->rowCount() - 1);
    }
}

#include "kcategorizedview.moc"
