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

#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QFontMetrics>

#include <kdirmodel.h>
#include <kfileitem.h>

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
    if (m_hasMinimizedNameColumn && (index.column() == KDirModel::Name)) {
        QStyleOptionViewItemV4 opt(option);

        const QAbstractProxyModel* proxyModel = static_cast<const QAbstractProxyModel*>(index.model());
        const KDirModel* dirModel = static_cast<const KDirModel*>(proxyModel->sourceModel());
        const QModelIndex dirIndex = proxyModel->mapToSource(index);
        const KFileItem item = dirModel->itemForIndex(dirIndex);
        if (!item.isNull()) {
            const int width = nameColumnWidth(item.text(), opt);
            opt.rect.setWidth(width);
        }
        KFileItemDelegate::paint(painter, opt, index);
    } else {
        KFileItemDelegate::paint(painter, option, index);
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

