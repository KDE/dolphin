/***************************************************************************
 *   Copyright (C) 2008 by <haraldhv (at) stud.ntnu.no>                    *
 *   Copyright (C) 2008 by <peter.penz@gmx.at>                             *
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

#include "ktreeview.h"
#include "ktreeview_p.h"

#include <KGlobalSettings>

#include <QEvent>
#include <QItemSelectionModel>
#include <QScrollBar>
#include <QTimer>
#include <QTimeLine>

KTreeView::KTreeViewPrivate::KTreeViewPrivate(KTreeView *parent) :
    parent(parent),
    autoHorizontalScroll(false),
    timeLine(0),
    startScrollTimer(0)
{
    startScrollTimer = new QTimer(this);
    startScrollTimer->setSingleShot(true);
    startScrollTimer->setInterval(300);

    timeLine = new QTimeLine(300, this);
}

void KTreeView::KTreeViewPrivate::connectScrollTimers()
{
    connect(startScrollTimer, SIGNAL(timeout()),
            this, SLOT(startScrolling()));

    connect(timeLine, SIGNAL(frameChanged(int)),
            this, SLOT(updateVerticalScrollBar(int)));

    connect(parent->verticalScrollBar(), SIGNAL(rangeChanged(int, int)),
            startScrollTimer, SLOT(start()));
    connect(parent->verticalScrollBar(), SIGNAL(valueChanged(int)),
            startScrollTimer, SLOT(start()));
    connect(parent, SIGNAL(collapsed(const QModelIndex&)),
            startScrollTimer, SLOT(start()));
    connect(parent, SIGNAL(expanded(const QModelIndex&)),
            startScrollTimer, SLOT(start()));
}

void KTreeView::KTreeViewPrivate::startScrolling()
{
    QModelIndex index;

    const int viewportHeight = parent->viewport()->height();

    // check whether there is a selected index which is partly visible
    const QModelIndexList selectedIndexes = parent->selectionModel()->selectedIndexes();
    if (selectedIndexes.count() == 1) {
        QModelIndex selectedIndex = selectedIndexes.first();
        const QRect rect = parent->visualRect(selectedIndex);
        if ((rect.bottom() >= 0) && (rect.top() <= viewportHeight)) {
            // the selected index is (at least partly) visible, use it as
            // scroll target
            index = selectedIndex;
        }
    }

    if (!index.isValid()) {
        // no partly selected index is visible, determine the most left visual index
        QModelIndex visibleIndex = parent->indexAt(QPoint(0, 0));
        if (!visibleIndex.isValid()) {
            return;
        }

        index = visibleIndex;
        int minimum = parent->width();
        do {
            const QRect rect = parent->visualRect(visibleIndex);
            if (rect.top() > viewportHeight) {
                // the current index and all successors are not visible anymore
                break;
            }
            if (rect.left() < minimum) {
                minimum = rect.left();
                index = visibleIndex;
            }
            visibleIndex = parent->indexBelow(visibleIndex);
        } while (visibleIndex.isValid());
    }

    // start the horizontal scrolling to assure that the item indicated by 'index' gets fully visible
    Q_ASSERT(index.isValid());
    const QRect rect = parent->visualRect(index);

    QScrollBar *scrollBar = parent->horizontalScrollBar();
    const int oldScrollBarPos = scrollBar->value();

    const int itemRight = oldScrollBarPos + rect.left() + rect.width() - 1;
    const int availableWidth = parent->viewport()->width();
    int scrollBarPos = itemRight - availableWidth;
    const int scrollBarPosMax = oldScrollBarPos + rect.left() - parent->indentation();
    if (scrollBarPos > scrollBarPosMax) {
        scrollBarPos = scrollBarPosMax;
    }

    if (scrollBarPos != oldScrollBarPos) {
        timeLine->setFrameRange(oldScrollBarPos, scrollBarPos);
        timeLine->start();
    }
}

void KTreeView::KTreeViewPrivate::updateVerticalScrollBar(int value)
{
    QScrollBar *scrollBar = parent->horizontalScrollBar();
    scrollBar->setValue(value);
    startScrollTimer->stop();
}

// ************************************************

KTreeView::KTreeView(QWidget *parent) :
    QTreeView(parent),
    d(new KTreeViewPrivate(this))
{
    if (KGlobalSettings::graphicEffectsLevel() >= KGlobalSettings::SimpleAnimationEffects) {
        setAutoHorizontalScroll(true);
    }
}

KTreeView::~KTreeView()
{
}

void KTreeView::setAutoHorizontalScroll(bool value)
{
	d->autoHorizontalScroll = value;
}

bool KTreeView::autoHorizontalScroll() const
{
	return d->autoHorizontalScroll;
}

void KTreeView::setSelectionModel(QItemSelectionModel *selectionModel)
{
    QTreeView::setSelectionModel(selectionModel);
    connect(selectionModel,
            SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            d->startScrollTimer, SLOT(start()));
}

void KTreeView::scrollTo(const QModelIndex& index, ScrollHint hint)
{
    if (d->autoHorizontalScroll) {
        // assure that the value of the horizontal scrollbar stays on its current value,
        // KTreeView will adjust the value manually
        const int value = horizontalScrollBar()->value();
        QTreeView::scrollTo(index, hint);
        horizontalScrollBar()->setValue(value);
    } else {
        QTreeView::scrollTo(index, hint);
    }
}

bool KTreeView::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        d->connectScrollTimers();
    }
    return QTreeView::event(event);
}

#include "ktreeview.moc"
#include "ktreeview_p.moc"
