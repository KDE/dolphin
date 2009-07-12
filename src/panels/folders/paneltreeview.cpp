/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "paneltreeview.h"

#include "dolphincontroller.h"
#include "dolphinmodel.h"
#include "draganddrophelper.h"

#include <kfileitemdelegate.h>
#include <QKeyEvent>
#include <QPainter>
#include <QHeaderView>
#include <QScrollBar>

PanelTreeView::PanelTreeView(QWidget* parent) :
    KTreeView(parent)
{
    setAcceptDrops(true);
    setUniformRowHeights(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSortingEnabled(true);
    setFrameStyle(QFrame::NoFrame);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);

    setVerticalScrollMode(QListView::ScrollPerPixel);
    setHorizontalScrollMode(QListView::ScrollPerPixel);

    viewport()->setAttribute(Qt::WA_Hover);

    // make the background transparent and apply the window-text color
    // to the text color, so that enough contrast is given for all color
    // schemes
    QPalette p = palette();
    p.setColor(QPalette::Active,   QPalette::Text, p.color(QPalette::Active,   QPalette::WindowText));
    p.setColor(QPalette::Inactive, QPalette::Text, p.color(QPalette::Inactive, QPalette::WindowText));
    p.setColor(QPalette::Disabled, QPalette::Text, p.color(QPalette::Disabled, QPalette::WindowText));
    setPalette(p);
    viewport()->setAutoFillBackground(false);

    KFileItemDelegate* delegate = new KFileItemDelegate(this);
    setItemDelegate(delegate);
}

PanelTreeView::~PanelTreeView()
{
}

bool PanelTreeView::event(QEvent* event)
{
    switch (event->type()) {
    case QEvent::Polish:
        // hide all columns except of the 'Name' column
        hideColumn(DolphinModel::Size);
        hideColumn(DolphinModel::ModifiedTime);
        hideColumn(DolphinModel::Permissions);
        hideColumn(DolphinModel::Owner);
        hideColumn(DolphinModel::Group);
        hideColumn(DolphinModel::Type);
        hideColumn(DolphinModel::Revision);
        header()->hide();
        break;

    case QEvent::Show:
        // TODO: The opening/closing animation of subtrees flickers in combination with the
        // panel when using the Oxygen style. As workaround the animation is turned off:
        setAnimated(false);
        break;

    case QEvent::UpdateRequest:
        // a wheel movement will scroll 1 item
        if (model()->rowCount() > 0) {
            verticalScrollBar()->setSingleStep(sizeHintForRow(0) / 3);
        }
        break;

    default:
        break;
    }

    return KTreeView::event(event);
}

void PanelTreeView::startDrag(Qt::DropActions supportedActions)
{
    DragAndDropHelper::instance().startDrag(this, supportedActions);
}

void PanelTreeView::dragEnterEvent(QDragEnterEvent* event)
{
    KTreeView::dragEnterEvent(event);
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void PanelTreeView::dragLeaveEvent(QDragLeaveEvent* event)
{
    KTreeView::dragLeaveEvent(event);
    setDirtyRegion(m_dropRect);
}

void PanelTreeView::dragMoveEvent(QDragMoveEvent* event)
{
    KTreeView::dragMoveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    const QModelIndex index = indexAt(event->pos());
    setDirtyRegion(m_dropRect);
    m_dropRect = visualRect(index);
    setDirtyRegion(m_dropRect);

    if (event->mimeData()->hasUrls()) {
        // accept url drops, independently from the destination item
        event->acceptProposedAction();
    }
}

void PanelTreeView::dropEvent(QDropEvent* event)
{
    const QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        emit urlsDropped(index, event);
    }
    KTreeView::dropEvent(event);
}

#include "paneltreeview.moc"
