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
#include <QBrush>
#include <QDrag>
#include <QPainter>
#include <QRect>
#include <QWidget>

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
        drag->exec(supportedActions);
    }
}

void DragAndDropHelper::drawHoverIndication(QAbstractItemView* itemView,
                                            const QRect& bounds,
                                            const QBrush& brush)
{
    if (bounds.isEmpty()) {
        return;
    }

    QWidget* widget = itemView->viewport();

    QPainter painter(widget);
    painter.save();
    QBrush blendedBrush(brush);
    QColor color = blendedBrush.color();
    color.setAlpha(64);
    blendedBrush.setColor(color);

    if (dynamic_cast<DolphinIconsView*>(itemView)) {
        const int radius = 10;
        QPainterPath path(QPointF(bounds.left(), bounds.top() + radius));
        path.quadTo(bounds.left(), bounds.top(), bounds.left() + radius, bounds.top());
        path.lineTo(bounds.right() - radius, bounds.top());
        path.quadTo(bounds.right(), bounds.top(), bounds.right(), bounds.top() + radius);
        path.lineTo(bounds.right(), bounds.bottom() - radius);
        path.quadTo(bounds.right(), bounds.bottom(), bounds.right() - radius, bounds.bottom());
        path.lineTo(bounds.left() + radius, bounds.bottom());
        path.quadTo(bounds.left(), bounds.bottom(), bounds.left(), bounds.bottom() - radius);
        path.closeSubpath();

        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillPath(path, blendedBrush);
    } else {
        painter.fillRect(bounds, blendedBrush);
    }
    painter.restore();
}
