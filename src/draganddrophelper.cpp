/***************************************************************************
 *   Copyright (C) 2007 by Peter Penz <peter.penz@gmx.at>                  *
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

#include <kdirmodel.h>
#include <kicon.h>

#include <QAbstractItemView>
#include <QAbstractProxyModel>
#include <QDrag>

void DragAndDropHelper::startDrag(QAbstractItemView* itemView, Qt::DropActions supportedActions)
{
    QModelIndexList indexes = itemView->selectionModel()->selectedIndexes();
    if (indexes.count() > 0) {
        QMimeData *data = itemView->model()->mimeData(indexes);
        if (data == 0) {
            return;
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
