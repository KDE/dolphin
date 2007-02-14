/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
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

#include "dolphiniconsview.h"

#include "dolphincontroller.h"

#include <assert.h>
#include <kdirmodel.h>
#include <kfileitem.h>

#include <QAbstractProxyModel>

DolphinIconsView::DolphinIconsView(QWidget* parent, DolphinController* controller) :
    QListView(parent),
    m_controller(controller)
{
    assert(controller != 0);

    setResizeMode(QListView::Adjust);

    // TODO: read out settings
    setViewMode(QListView::IconMode);
    setSpacing(32);

    connect(this, SIGNAL(clicked(const QModelIndex&)),
            controller, SLOT(triggerItem(const QModelIndex&)));
}

DolphinIconsView::~DolphinIconsView()
{
}

QStyleOptionViewItem DolphinIconsView::viewOptions() const
{
    return QListView::viewOptions();

    // TODO: the view options should been read from the settings;
    // the following code is just for testing...
    //QStyleOptionViewItem options = QListView::viewOptions();
    //options.decorationAlignment = Qt::AlignRight;
    //options.decorationPosition = QStyleOptionViewItem::Right;
    //options.decorationSize = QSize(100, 100);
    //options.showDecorationSelected = true;
    //options.state = QStyle::State_MouseOver;
    //return options;
}

void DolphinIconsView::contextMenuEvent(QContextMenuEvent* event)
{
    QListView::contextMenuEvent(event);
    m_controller->triggerContextMenuRequest(event->pos(),
                                            event->globalPos());
}

void DolphinIconsView::mouseReleaseEvent(QMouseEvent* event)
{
    QListView::mouseReleaseEvent(event);
    m_controller->triggerActivation();
}

void DolphinIconsView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void DolphinIconsView::dropEvent(QDropEvent* event)
{
    QListView::dropEvent(event);
    // TODO: temporary deactivated until DolphinController will support this

    /*    KFileItem* directory = 0;
    bool dropIntoDirectory = false;
    const QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        KFileItem* item = m_dolphinView->fileItem(index);
        assert(item != 0);
        dropIntoDirectory = item->isDir();
        if (dropIntoDirectory) {
            directory = item;
        }
    }

    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (urls.isEmpty() || (event->source() == this) && !dropIntoDirectory) {
        QListView::dropEvent(event);
    }
    else {
        event->acceptProposedAction();
        const KUrl& destination = (directory == 0) ? m_dolphinView->url() :
                                                     directory->url();
        m_dolphinView->mainWindow()->dropUrls(urls, destination);
    }*/
}

#include "dolphiniconsview.moc"
