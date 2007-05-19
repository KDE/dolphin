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

// NOTE: rectForIndex() not virtual on QListView !! relevant ?
#include "klistview.h"
#include "klistview_p.h"

#include <QtGui/QPainter>
#include <QtGui/QScrollBar>
#include <QtGui/QKeyEvent>
#include <QtGui/QSortFilterProxyModel>

#include <kdebug.h>

#include "kitemcategorizer.h"

KListView::Private::Private(KListView *listView)
    : listView(listView)
    , modelSortCapable(false)
    , itemCategorizer(0)
    , numCategories(0)
    , proxyModel(0)
{
}

KListView::Private::~Private()
{
}

QModelIndexList KListView::Private::intersectionSet(const QRect &rect) const
{
    // FIXME: boost me, I suck (ereslibre)

    QModelIndexList modelIndexList;

    QModelIndex index;
    for (int i = 0; i < listView->model()->rowCount(); i++)
    {
        index = listView->model()->index(i, 0);

        if (rect.intersects(listView->visualRect(index)))
            modelIndexList.append(index);
    }

    return modelIndexList;
}

KListView::KListView(QWidget *parent)
    : QListView(parent)
    , d(new Private(this))
{
}

KListView::~KListView()
{
    if (d->proxyModel)
    {
        QObject::disconnect(this->model(), SIGNAL(layoutChanged()),
                            this         , SLOT(itemsLayoutChanged()));
    }

    delete d;
}

void KListView::setModel(QAbstractItemModel *model)
{
    QSortFilterProxyModel *proxyModel =
                                    qobject_cast<QSortFilterProxyModel*>(model);

    if (this->model() && this->model()->rowCount())
    {
        QObject::disconnect(this->model(), SIGNAL(layoutChanged()),
                            this         , SLOT(itemsLayoutChanged()));

        rowsAboutToBeRemovedArtifficial(QModelIndex(), 0,
                                        this->model()->rowCount() - 1);
    }

    d->modelSortCapable = (proxyModel != 0);
    d->proxyModel = proxyModel;

    // If the model was initialized before applying to the view, we update
    // internal data structure of the view with the model information
    if (model->rowCount())
    {
        rowsInsertedArtifficial(QModelIndex(), 0, model->rowCount() - 1);
    }

    QListView::setModel(model);

    QObject::connect(model, SIGNAL(layoutChanged()),
                     this , SLOT(itemsLayoutChanged()));
}

QRect KListView::visualRect(const QModelIndex &index) const
{
    // FIXME: right to left languages (ereslibre)
    // FIXME: drag & drop support (ereslibre)
    // FIXME: do not forget to remove itemWidth's hard-coded values that were
    //        only for testing purposes. We might like to calculate the best
    //        width, but what we would really like for sure is that all items
    //        have the same width, as well as the same height (ereslibre)

    if ((viewMode() == KListView::ListMode) || !d->modelSortCapable ||
        !d->itemCategorizer)
    {
        return QListView::visualRect(index);
    }

    QRect retRect(spacing(), spacing(), 0, 0);
    int viewportWidth = viewport()->width() - spacing();
    int dx = -horizontalOffset();
    int dy = -verticalOffset();

    if (verticalScrollBar() && !verticalScrollBar()->isHidden())
        viewportWidth -= verticalScrollBar()->width();

    int itemHeight = sizeHintForIndex(index).height();
    int itemWidth = 130; // NOTE: ghosts in here !
    int itemWidthPlusSeparation = spacing() + itemWidth;
    int elementsPerRow = viewportWidth / itemWidthPlusSeparation;
    if (!elementsPerRow)
        elementsPerRow++;
    QModelIndex currentIndex = d->proxyModel->index(index.row(), 0);
    QString itemCategory = d->itemCategorizer->categoryForItem(currentIndex,
                                                     d->proxyModel->sortRole());
    int naturalRow = index.row() / elementsPerRow;
    int naturalTop = naturalRow * itemHeight + naturalRow * spacing();

    int rowsForCategory;
    int lastIndexShown = -1;
    foreach (QString category, d->categories)
    {
        retRect.setTop(retRect.top() + spacing());

        if (category == itemCategory)
        {
            break;
        }

        rowsForCategory = (d->elementsPerCategory[category] / elementsPerRow);

        if ((d->elementsPerCategory[category] % elementsPerRow) ||
            !rowsForCategory)
        {
            rowsForCategory++;
        }

        lastIndexShown += d->elementsPerCategory[category];

        retRect.setTop(retRect.top() + categoryHeight(viewOptions()) +
                       (rowsForCategory * spacing() * 2) +
                       (rowsForCategory * itemHeight));
    }

    int rowToPosition = (index.row() - (lastIndexShown + 1)) / elementsPerRow;
    int columnToPosition = (index.row() - (lastIndexShown + 1)) %
                                                                 elementsPerRow;

    retRect.setTop(retRect.top() + (rowToPosition * spacing() * 2) +
                   (rowToPosition * itemHeight));

    retRect.setLeft(retRect.left() + (columnToPosition * spacing()) +
                    (columnToPosition * itemWidth));

    retRect.setWidth(130); // NOTE: ghosts in here !
    retRect.setHeight(itemHeight);

    retRect.adjust(dx, dy, dx, dy);

    return retRect;
}

KItemCategorizer *KListView::itemCategorizer() const
{
    return d->itemCategorizer;
}

void KListView::setItemCategorizer(KItemCategorizer *itemCategorizer)
{
    d->itemCategorizer = itemCategorizer;

    if (itemCategorizer)
        itemsLayoutChanged();
}

QModelIndex KListView::indexAt(const QPoint &point) const
{
    QModelIndex index;

    if ((viewMode() == KListView::ListMode) || !d->modelSortCapable ||
        !d->itemCategorizer)
    {
        return QListView::indexAt(point);
    }

    QModelIndexList item = d->intersectionSet(QRect(point, point));

    if (item.count() == 1)
        index = item[0];

    d->hovered = index;

    return index;
}

int KListView::sizeHintForRow(int row) const
{
    if ((viewMode() == KListView::ListMode) || !d->modelSortCapable ||
        !d->itemCategorizer)
    {
        return QListView::sizeHintForRow(row);
    }

    QModelIndex index = d->proxyModel->index(0, 0);

    if (!index.isValid())
        return 0;

    return sizeHintForIndex(index).height() + categoryHeight(viewOptions()) +
           spacing();
}

void KListView::drawNewCategory(const QString &category,
                                const QStyleOptionViewItem &option,
                                QPainter *painter)
{
    painter->drawText(option.rect.topLeft(), category);
}

int KListView::categoryHeight(const QStyleOptionViewItem &option) const
{
    return option.fontMetrics.height();
}

void KListView::paintEvent(QPaintEvent *event)
{
    if ((viewMode() == KListView::ListMode) || !d->modelSortCapable ||
        !d->itemCategorizer)
    {
        QListView::paintEvent(event);
        return;
    }

    if (!itemDelegate())
        return;

    QStyleOptionViewItemV3 option = viewOptions();
    QPainter painter(viewport());
    QRect area = event->rect();
    const bool focus = (hasFocus() || viewport()->hasFocus()) &&
                        currentIndex().isValid();
    const QStyle::State state = option.state;
    const bool enabled = (state & QStyle::State_Enabled) != 0;

    int totalHeight = 0;
    QModelIndex index;
    QString prevCategory;
    QModelIndexList dirtyIndexes = d->intersectionSet(area);
    foreach (const QModelIndex &index, dirtyIndexes)
    {
        option.state = state;
        option.rect = visualRect(index);
        if (selectionModel() && selectionModel()->isSelected(index))
            option.state |= QStyle::State_Selected;
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

        if (index == d->hovered)
            option.state |= QStyle::State_MouseOver;
        else
            option.state &= ~QStyle::State_MouseOver;

        if (prevCategory != d->itemCategorizer->categoryForItem(index,
                                                     d->proxyModel->sortRole()))
        {
            prevCategory = d->itemCategorizer->categoryForItem(index,
                                                     d->proxyModel->sortRole());
            drawNewCategory(prevCategory, option, &painter);
        }
        itemDelegate(index)->paint(&painter, option, index);
    }
}

void KListView::setSelection(const QRect &rect,
                             QItemSelectionModel::SelectionFlags flags)
{
    // TODO: implement me

    QListView::setSelection(rect, flags);

    /*if ((viewMode() == KListView::ListMode) || !d->modelSortCapable ||
        !d->itemCategorizer)
    {
        QListView::setSelection(rect, flags);
        return;
    }

    QModelIndex index;
    for (int i = 0; i < d->proxyModel->rowCount(); i++)
    {
        index = d->proxyModel->index(i, 0);
        if (rect.intersects(visualRect(index)))
        {
            selectionModel()->select(index, QItemSelectionModel::Select);
        }
        else
        {
            selectionModel()->select(index, QItemSelectionModel::Deselect);
        }
    }*/

    //selectionModel()->select(selection, flags);
}

void KListView::timerEvent(QTimerEvent *event)
{
    QListView::timerEvent(event);

    if ((viewMode() == KListView::ListMode) || !d->modelSortCapable ||
        !d->itemCategorizer)
    {
        return;
    }
}

void KListView::rowsInserted(const QModelIndex &parent,
                             int start,
                             int end)
{
    QListView::rowsInserted(parent, start, end);
    rowsInsertedArtifficial(parent, start, end);
}

void KListView::rowsAboutToBeRemoved(const QModelIndex &parent,
                                     int start,
                                     int end)
{
    QListView::rowsAboutToBeRemoved(parent, start, end);
    rowsAboutToBeRemovedArtifficial(parent, start, end);
}

void KListView::rowsInsertedArtifficial(const QModelIndex &parent,
                                        int start,
                                        int end)
{
    if ((viewMode() == KListView::ListMode) || !d->modelSortCapable ||
        !d->itemCategorizer)
    {
        return;
    }

    QString category;
    QModelIndex index;
    for (int i = start; i <= end; i++)
    {
        index = d->proxyModel->index(i, 0, parent);
        category = d->itemCategorizer->categoryForItem(index,
                                                     d->proxyModel->sortRole());

        if (d->elementsPerCategory.contains(category))
            d->elementsPerCategory[category]++;
        else
        {
            d->elementsPerCategory.insert(category, 1);
            d->categories.append(category);
        }
    }
}

void KListView::rowsAboutToBeRemovedArtifficial(const QModelIndex &parent,
                                                int start,
                                                int end)
{
    if ((viewMode() == KListView::ListMode) || !d->modelSortCapable ||
        !d->itemCategorizer)
    {
        return;
    }

    d->hovered = QModelIndex();

    QString category;
    QModelIndex index;
    for (int i = start; i <= end; i++)
    {
        index = d->proxyModel->index(i, 0, parent);
        category = d->itemCategorizer->categoryForItem(index,
                                                     d->proxyModel->sortRole());

        if (d->elementsPerCategory.contains(category))
        {
            d->elementsPerCategory[category]--;

            if (!d->elementsPerCategory[category])
            {
                d->elementsPerCategory.remove(category);
                d->categories.removeAll(category);
            }
        }
    }
}

void KListView::itemsLayoutChanged()
{
    d->elementsPerCategory.clear();
    d->categories.clear();

    if (d->proxyModel && d->proxyModel->rowCount())
        rowsInsertedArtifficial(QModelIndex(), 0,
                                                 d->proxyModel->rowCount() - 1);
}

#include "klistview.moc"
