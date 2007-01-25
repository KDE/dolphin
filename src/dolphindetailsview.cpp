/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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

#include "dolphindetailsview.h"
#include "dolphinmainwindow.h"
#include "dolphinview.h"

#include <QHeaderView>

DolphinDetailsView::DolphinDetailsView(DolphinView* parent) :
    QTreeView(parent),
    m_parentView(parent)
{
    setAcceptDrops(true);
    setRootIsDecorated(false);
    setSortingEnabled(true);
    setUniformRowHeights(true);
}

DolphinDetailsView::~DolphinDetailsView()
{
}

bool DolphinDetailsView::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        // Assure that by respecting the available width that:
        // - the 'Name' column is stretched as large as possible
        // - the remaining columns are as small as possible
        QHeaderView* headerView = header();
        headerView->setStretchLastSection(false);
        headerView->setResizeMode(QHeaderView::ResizeToContents);
        headerView->setResizeMode(0, QHeaderView::Stretch);
    }

    return QTreeView::event(event);
}
QStyleOptionViewItem DolphinDetailsView::viewOptions() const
{
    return QTreeView::viewOptions();

    // TODO: the view options should been read from the settings;
    // the following code is just for testing...
    //QStyleOptionViewItem options = QTreeView::viewOptions();
    //options.decorationAlignment = Qt::AlignRight;
    //options.decorationPosition = QStyleOptionViewItem::Right;
    //options.decorationSize = QSize(100, 100);
    //options.showDecorationSelected = true;
    //options.state = QStyle::State_MouseOver;
    //return options;
}

void DolphinDetailsView::contextMenuEvent(QContextMenuEvent* event)
{
    QTreeView::contextMenuEvent(event);

    KFileItem* item = 0;

    const QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        item = m_parentView->fileItem(index);
    }

    m_parentView->openContextMenu(item, event->globalPos());
}

void DolphinDetailsView::mouseReleaseEvent(QMouseEvent* event)
{
    QTreeView::mouseReleaseEvent(event);
    m_parentView->declareViewActive();
}

void DolphinDetailsView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void DolphinDetailsView::dropEvent(QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (!urls.isEmpty()) {
        event->acceptProposedAction();

        // TODO: handle dropping above a directory

        const KUrl& destination = m_parentView->url();
        m_parentView->mainWindow()->dropUrls(urls, destination);
    }
}

#include "dolphindetailsview.moc"
