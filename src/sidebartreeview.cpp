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

#include <kdirmodel.h>
#include <kfileitemdelegate.h>

#include <QKeyEvent>
#include <QPainter>
#include <QHeaderView>

SidebarTreeView::SidebarTreeView(QWidget* parent) :
    QTreeView(parent),
    m_dragging(false)
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

    viewport()->setAttribute(Qt::WA_Hover);

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
        hideColumn(KDirModel::Size);
        hideColumn(KDirModel::ModifiedTime);
        hideColumn(KDirModel::Permissions);
        hideColumn(KDirModel::Owner);
        hideColumn(KDirModel::Group);
        hideColumn(KDirModel::Type);
        header()->hide();
    }

    return QTreeView::event(event);
}

void SidebarTreeView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
    QTreeView::dragEnterEvent(event);
    m_dragging = true;
}

void SidebarTreeView::dragMoveEvent(QDragMoveEvent* event)
{
    QTreeView::dragMoveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    const QModelIndex index = indexAt(event->pos());
    setDirtyRegion(m_dropRect);
    m_dropRect = visualRect(index);
    setDirtyRegion(m_dropRect);
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
    m_dragging = false;
}

void SidebarTreeView::paintEvent(QPaintEvent* event)
{
    QTreeView::paintEvent(event);

    if (m_dragging) {
        // TODO: remove this code when the issue #160611 is solved in Qt 4.4
        QPainter painter(viewport());
        painter.save();
        QBrush brush(palette().brush(QPalette::Normal, QPalette::Highlight));
        QColor color = brush.color();
        color.setAlpha(64);
        brush.setColor(color);
        painter.fillRect(m_dropRect, brush);
        painter.restore();
    }
}

#include "sidebartreeview.moc"
