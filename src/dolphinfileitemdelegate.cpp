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
#include <kicon.h>
#include <kiconloader.h>

#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QFontMetrics>
#include <QPalette>
#include <QPainter>
#include <QStyleOptionViewItemV4>

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
    const bool isNameColumn = (index.column() == KDirModel::Name);

    if (m_hasMinimizedNameColumn && isNameColumn) {
        QStyleOptionViewItemV4 opt(option);
        adjustOptionWidth(opt, proxyModel, dolphinModel, index);
        KFileItemDelegate::paint(painter, opt, index);
    } else {
        KFileItemDelegate::paint(painter, option, index);
    }

    if (dolphinModel->hasRevisionData() && isNameColumn) {
        // The currently shown items are under revision control. Show the current revision
        // state by adding an emblem.
        const QModelIndex dirIndex = proxyModel->mapToSource(index);
        const QModelIndex revisionIndex = dolphinModel->index(dirIndex.row(), DolphinModel::Revision);
        const QVariant data = dolphinModel->data(revisionIndex, Qt::DecorationRole);
        const DolphinModel::RevisionState state = static_cast<DolphinModel::RevisionState>(data.toInt());

        if (state != DolphinModel::LocalRevision) {
            // TODO: extend KFileItemDelegate to be able to get the icon boundaries
            const QRect iconRect(option.rect.x(), option.rect.y(),
                                 KIconLoader::SizeSmall, KIconLoader::SizeSmall);
            const QPixmap emblem = emblemForState(state, iconRect.size());
            painter->drawPixmap(iconRect.x(), iconRect.y(), emblem);
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

void DolphinFileItemDelegate::adjustOptionWidth(QStyleOptionViewItemV4& option,
                                                const QAbstractProxyModel* proxyModel,
                                                const DolphinModel* dolphinModel,
                                                const QModelIndex& index)
{
    const QModelIndex dirIndex = proxyModel->mapToSource(index);
    const KFileItem item = dolphinModel->itemForIndex(dirIndex);
    if (!item.isNull()) {
        // symbolic links are displayed in an italic font
        if (item.isLink()) {
            option.font.setItalic(true);
        }

        const int width = nameColumnWidth(item.text(), option);
        option.rect.setWidth(width);
    }
}

QPixmap DolphinFileItemDelegate::emblemForState(DolphinModel::RevisionState state, const QSize& size)
{
    // TODO #1: all icons that are use here will be replaced by revision control emblems provided by the
    // Oxygen team before KDE 4.4
    // TODO #2: cache the icons
    switch (state) {
    case DolphinModel::LatestRevision:
        return KIcon("dialog-ok-apply").pixmap(size);
        break;

    case DolphinModel::ConflictingRevision:
        return KIcon("emblem-important").pixmap(size);
        break;

    // ...

    default:
        break;
    }
    return QPixmap();
}

