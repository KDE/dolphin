/***************************************************************************
 *   Copyright (C) 2007 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2007 by David Faure <faure@kde.org>                     *
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

#include "draganddrophelper.h"
#include "dolphiniconsview.h"
#include "dolphinviewcontroller.h"

#include <kdirmodel.h>
#include <kfileitem.h>
#include <kicon.h>
#include <klocale.h>
#include <konq_operations.h>

#include <QAbstractItemView>
#include <QAbstractProxyModel>
#include <QtDBus>
#include <QDrag>
#include <QPainter>

class DragAndDropHelperSingleton
{
public:
    DragAndDropHelper instance;
};
K_GLOBAL_STATIC(DragAndDropHelperSingleton, s_dragAndDropHelper)

DragAndDropHelper& DragAndDropHelper::instance()
{
    return s_dragAndDropHelper->instance;
}

bool DragAndDropHelper::isMimeDataSupported(const QMimeData* mimeData) const
{
    return mimeData->hasUrls() || mimeData->hasFormat("application/x-kde-dndextract");
}

void DragAndDropHelper::startDrag(QAbstractItemView* itemView,
                                  Qt::DropActions supportedActions,
                                  DolphinViewController* dolphinViewController)
{
    // Do not start a new drag until the previous one has been finished.
    // This is a (possibly temporary) fix for bug #187884.
    static bool isDragging = false;
    if (isDragging) {
        return;
    }
    isDragging = true;

    const QModelIndexList indexes = itemView->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty()) {
        QMimeData *data = itemView->model()->mimeData(indexes);
        if (data == 0) {
            return;
        }

        if (dolphinViewController != 0) {
            dolphinViewController->requestToolTipHiding();
        }

        QDrag* drag = new QDrag(itemView);
        drag->setPixmap(createDragPixmap(itemView));
        drag->setMimeData(data);

        m_dragSource = itemView;
        drag->exec(supportedActions, Qt::IgnoreAction);
        m_dragSource = 0;
    }
    isDragging = false;
}

bool DragAndDropHelper::isDragSource(QAbstractItemView* itemView) const
{
    return (m_dragSource != 0) && (m_dragSource == itemView);
}

void DragAndDropHelper::dropUrls(const KFileItem& destItem,
                                 const KUrl& destPath,
                                 QDropEvent* event,
                                 QWidget* widget)
{
    const bool dropToItem = !destItem.isNull() && (destItem.isDir() || destItem.isDesktopFile());
    const KUrl destination = dropToItem ? destItem.url() : destPath;

    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasFormat("application/x-kde-dndextract")) {
        QString remoteDBusClient = mimeData->data("application/x-kde-dndextract");
        QDBusMessage message = QDBusMessage::createMethodCall(remoteDBusClient, "/DndExtract",
                                                              "org.kde.DndExtract", "extractSelectedFilesTo");
        message.setArguments(QVariantList() << destination.path());
        QDBusConnection::sessionBus().call(message);
    } else {
        const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
        const int urlsCount = urls.count();
        if (urlsCount == 0) {
            // TODO: handle dropping of other data
        } else if ((urlsCount == 1) && (urls.first() == destination)) {
            emit errorMessage(i18nc("@info:status", "A folder cannot be dropped into itself"));
        } else if (dropToItem) {
            KonqOperations::doDrop(destItem, destination, event, widget);
        } else {
            KonqOperations::doDrop(KFileItem(), destination, event, widget);
        }
    }
}

DragAndDropHelper::DragAndDropHelper()
    : m_dragSource(0)
{
}

QPixmap DragAndDropHelper::createDragPixmap(QAbstractItemView* itemView) const
{
    const QModelIndexList selectedIndexes = itemView->selectionModel()->selectedIndexes();    
    Q_ASSERT(!selectedIndexes.isEmpty());
    
    QAbstractProxyModel* proxyModel = static_cast<QAbstractProxyModel*>(itemView->model());
    KDirModel* dirModel = static_cast<KDirModel*>(proxyModel->sourceModel());
    
    const int itemCount = selectedIndexes.count();
    
    // If more than one item is dragged, align the items inside a
    // rectangular grid. The maximum grid size is limited to 5 x 5 items.
    int xCount = 3;
    int size = KIconLoader::SizeMedium;
    if (itemCount > 16) {
        xCount = 5;
        size = KIconLoader::SizeSmall;
    } else if (itemCount > 9) {
        xCount = 4;
        size = KIconLoader::SizeSmallMedium;
    }
    
    if (itemCount < xCount) {
        xCount = itemCount;
    }
    
    int yCount = itemCount / xCount;
    if (itemCount % xCount != 0) {
        ++yCount;
    }
    if (yCount > xCount) {
        yCount = xCount;
    }

    // Draw the selected items into the grid cells    
    QPixmap dragPixmap(xCount * size + xCount - 1, yCount * size + yCount - 1);
    dragPixmap.fill(Qt::transparent);
    
    QPainter painter(&dragPixmap);
    int x = 0;
    int y = 0;
    foreach (const QModelIndex& selectedIndex, selectedIndexes) {
        const QModelIndex index = proxyModel->mapToSource(selectedIndex);
        const KFileItem item = dirModel->itemForIndex(index);
        const QPixmap pixmap = item.pixmap(size, size);
        painter.drawPixmap(x, y, pixmap);
        
        x += size + 1;
        if (x >= dragPixmap.width()) {
            x = 0;
            y += size + 1;
        }
        if (y >= dragPixmap.height()) {
            break;
        }
    }
    
    return dragPixmap;
}

#include "draganddrophelper.moc"
