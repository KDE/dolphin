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

#include "sidebartreeview.h"

#include "dolphincontroller.h"
#include "dolphinmodel.h"
#include "draganddrophelper.h"

#include <kfileitemdelegate.h>
#include <QKeyEvent>
#include <QPainter>
#include <QHeaderView>
#include <QScrollBar>

SidebarTreeView::SidebarTreeView(QWidget* parent) :
    QTreeView(parent)
{
    setAcceptDrops(true);
    setUniformRowHeights(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSortingEnabled(true);
    setFrameStyle(QFrame::NoFrame);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);
    setAutoExpandDelay(300);

    setVerticalScrollMode(QListView::ScrollPerPixel);
    setHorizontalScrollMode(QListView::ScrollPerPixel);

    viewport()->setAttribute(Qt::WA_Hover);

    QPalette palette = viewport()->palette();
    palette.setColor(viewport()->backgroundRole(), Qt::transparent);
    viewport()->setPalette(palette);

    KFileItemDelegate* delegate = new KFileItemDelegate(this);
    setItemDelegate(delegate);
}

SidebarTreeView::~SidebarTreeView()
{
}

bool SidebarTreeView::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        // hide all columns except of the 'Name' column
        hideColumn(DolphinModel::Size);
        hideColumn(DolphinModel::ModifiedTime);
        hideColumn(DolphinModel::Permissions);
        hideColumn(DolphinModel::Owner);
        hideColumn(DolphinModel::Group);
        hideColumn(DolphinModel::Type);
        hideColumn(DolphinModel::Rating);
        hideColumn(DolphinModel::Tags);
        header()->hide();
    }
    else if (event->type() == QEvent::UpdateRequest) {
        // a wheel movement will scroll 1 item
        if (model()->rowCount() > 0) {
            verticalScrollBar()->setSingleStep(sizeHintForRow(0) / 3);
        }
    }
    else if (event->type() == QEvent::MetaCall) {
        resizeColumnToContents(DolphinModel::Name);
    }

    return QTreeView::event(event);
}

void SidebarTreeView::startDrag(Qt::DropActions supportedActions)
{
    DragAndDropHelper::startDrag(this, supportedActions);
}

void SidebarTreeView::dragEnterEvent(QDragEnterEvent* event)
{
    QTreeView::dragEnterEvent(event);
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void SidebarTreeView::dragLeaveEvent(QDragLeaveEvent* event)
{
    QTreeView::dragLeaveEvent(event);
    setDirtyRegion(m_dropRect);
}

void SidebarTreeView::dragMoveEvent(QDragMoveEvent* event)
{
    QTreeView::dragMoveEvent(event);

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

void SidebarTreeView::dropEvent(QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (urls.isEmpty()) {
        QTreeView::dropEvent(event);
    } else {
        event->acceptProposedAction();
        const QModelIndex index = indexAt(event->pos());
        if (index.isValid()) {
            emit urlsDropped(urls, index);
        }
    }
}

#include "sidebartreeview.moc"
