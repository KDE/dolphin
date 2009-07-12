/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "dolphinfileitemdelegate.h"

#include <dolphinmodel.h>
#include <kfileitem.h>

#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QFontMetrics>
#include <QPainter>

DolphinFileItemDelegate::DolphinFileItemDelegate(QObject* parent) :
    KFileItemDelegate(parent),
    m_hasMinimizedNameColumn(false)
{
}

DolphinFileItemDelegate::~DolphinFileItemDelegate()
{
}

void DolphinFileItemDelegate::paint(QPainter* painter,
                                    const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const
{
    const QAbstractProxyModel* proxyModel = static_cast<const QAbstractProxyModel*>(index.model());
    const DolphinModel* dolphinModel = static_cast<const DolphinModel*>(proxyModel->sourceModel());

    if (m_hasMinimizedNameColumn && (index.column() == KDirModel::Name)) {
        QStyleOptionViewItemV4 opt(option);

        const QModelIndex dirIndex = proxyModel->mapToSource(index);
        const KFileItem item = dolphinModel->itemForIndex(dirIndex);
        if (!item.isNull()) {
            // symbolic links are displayed in an italic font
            if (item.isLink()) {
                opt.font.setItalic(true);
            }

            const int width = nameColumnWidth(item.text(), opt);
            opt.rect.setWidth(width);
        }
        KFileItemDelegate::paint(painter, opt, index);
    } else {
        KFileItemDelegate::paint(painter, option, index);
    }

    if (dolphinModel->hasRevisionData()) {
        // The currently shown items are under revision control. Show the current revision
        // state above the decoration.
        const QModelIndex dirIndex = proxyModel->mapToSource(index);
        const QModelIndex revisionIndex = dolphinModel->index(dirIndex.row(), DolphinModel::Revision);
        const QVariant data = dolphinModel->data(revisionIndex, Qt::DecorationRole);
        const DolphinModel::RevisionState state = static_cast<DolphinModel::RevisionState>(data.toInt());

        if (state != DolphinModel::LocalRevision) {
            // TODO: The following code is just a proof of concept. Icons will be used later...
            QColor color(200, 0, 0, 32);
            switch (state) {
            case DolphinModel::LatestRevision: color = QColor(0, 180, 0, 32); break;
            // ...
            default: break;
            }
            painter->fillRect(option.rect, color);
        }
    }
}

int DolphinFileItemDelegate::nameColumnWidth(const QString& name, const QStyleOptionViewItem& option)
{
    QFontMetrics fontMetrics(option.font);
    int width = option.decorationSize.width() + fontMetrics.width(name) + 16;

    const int defaultWidth = option.rect.width();
    if ((defaultWidth > 0) && (defaultWidth < width)) {
        width = defaultWidth;
    }
    return width;
}

