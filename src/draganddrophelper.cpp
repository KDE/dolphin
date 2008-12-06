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
#include "dolphincontroller.h"

#include <kdirmodel.h>
#include <kfileitem.h>
#include <kicon.h>
#include <klocale.h>
#include <kdebug.h>
#include <konq_operations.h>

#include <QAbstractItemView>
#include <QAbstractProxyModel>
#include <QtDBus>
#include <QDrag>

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
                                  DolphinController* controller)
{
    QModelIndexList indexes = itemView->selectionModel()->selectedIndexes();
    if (indexes.count() > 0) {
        QMimeData *data = itemView->model()->mimeData(indexes);
        if (data == 0) {
            return;
        }
        
        if (controller != 0) {
            controller->emitHideToolTip();
        }

        QDrag* drag = new QDrag(itemView);
        QPixmap pixmap;
        if (indexes.count() == 1) {
            QAbstractProxyModel* proxyModel = static_cast<QAbstractProxyModel*>(itemView->model());
            KDirModel* dirModel = static_cast<KDirModel*>(proxyModel->sourceModel());
            const QModelIndex index = proxyModel->mapToSource(indexes.first());

            const KFileItem item = dirModel->itemForIndex(index);
            pixmap = item.pixmap(KIconLoader::SizeMedium, KIconLoader::SizeMedium);
        } else {
            pixmap = KIcon("document-multiple").pixmap(KIconLoader::SizeMedium, KIconLoader::SizeMedium);
        }
        drag->setPixmap(pixmap);
        drag->setMimeData(data);
        drag->exec(supportedActions, Qt::IgnoreAction);
    }
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
                                                              "org.kde.DndExtract", "extractFilesTo");
        message.setArguments(QVariantList() << destination.path());
        QDBusConnection::sessionBus().call(message);
    } else {                                
        const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
        if ((urls.count() == 1) && (urls.first() == destination)) {
            emit errorMessage(i18nc("@info:status", "A folder cannot be dropped into itself"));
        } else if (dropToItem) {
            KonqOperations::doDrop(destItem, destination, event, widget);
        } else {
            KonqOperations::doDrop(KFileItem(), destination, event, widget);
        }
    }
}

DragAndDropHelper::DragAndDropHelper()
{
}

#include "draganddrophelper.moc"
