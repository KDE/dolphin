/***************************************************************************
 *   Copyright (C) 2008 by <haraldhv (at) stud.ntnu.no>                    *
 *   Copyright (C) 2008 by <peter.penz19@gmail.com>                        *
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

#include <QItemSelectionModel>
#include <QScrollBar>
#include <QTimer>
#include <QTimeLine>

KTreeView::KTreeViewPrivate::KTreeViewPrivate(KTreeView *parent) :
    parent(parent),
    autoHorizontalScroll(false),
    timeLine(0)
{
    timeLine = new QTimeLine(500, this);
    connect(timeLine, SIGNAL(frameChanged(int)),
            this, SLOT(updateVerticalScrollBar(int)));

    connect(parent->verticalScrollBar(), SIGNAL(rangeChanged(int, int)),
            this, SLOT(startScrolling()));
    connect(parent->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(startScrolling()));
    connect(parent, SIGNAL(collapsed(const QModelIndex&)),
            this, SLOT(startScrolling()));
    connect(parent, SIGNAL(expanded(const QModelIndex&)),
            this, SLOT(startScrolling()));
}

void KTreeView::KTreeViewPrivate::startScrolling()
{
    if (!autoHorizontalScroll) {
        return;
    }

    // Determine the most left visual index
    QModelIndex visibleIndex = parent->indexAt(QPoint(0, 0));
    if (!visibleIndex.isValid()) {
        return;
    }

    QModelIndex index = visibleIndex;
    int minimum = parent->width();
    do {
        const QRect rect = parent->visualRect(visibleIndex);
        if (rect.top() > parent->viewport()->height()) {
            // the current index and all successors are not visible anymore
            break;
        }
        if (rect.left() < minimum) {
            minimum = rect.left();
            index = visibleIndex;
        }
        visibleIndex = parent->indexBelow(visibleIndex);
    } while (visibleIndex.isValid());

    // Start the horizontal scrolling to assure that the item indicated by 'index' gets fully visible
    Q_ASSERT(index.isValid());
    const QRect rect = parent->visualRect(index);

    QScrollBar* scrollBar = parent->horizontalScrollBar();
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
        if (timeLine->state() == QTimeLine::Running) {
            timeLine->stop();
        }
        timeLine->start();
    }
}

void KTreeView::KTreeViewPrivate::updateVerticalScrollBar(int value)
{
    QScrollBar *scrollBar = parent->horizontalScrollBar();
    scrollBar->setValue(value);
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

void KTreeView::setAutoHorizontalScroll(bool enable)
{
    d->autoHorizontalScroll = enable;
    if (!enable) {
        d->timeLine->stop();
    }
}

bool KTreeView::autoHorizontalScroll() const
{
    return d->autoHorizontalScroll;
}

void KTreeView::scrollTo(const QModelIndex& index, ScrollHint hint)
{
    const int value = horizontalScrollBar()->value();
    QTreeView::scrollTo(index, hint);
    horizontalScrollBar()->setValue(value);
}

void KTreeView::hideEvent(QHideEvent *event)
{
    d->timeLine->stop();
    QTreeView::hideEvent(event);
}

#include "ktreeview.moc"
#include "ktreeview_p.moc"
