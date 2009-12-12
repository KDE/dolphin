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
    m_hasMinimizedNameColumn(false),
    m_cachedSize(),
    m_cachedEmblems()
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

    if (dolphinModel->hasVersionData() && isNameColumn) {
        // The currently shown items are under revision control. Show the current revision
        // state by adding an emblem.
        const QModelIndex dirIndex = proxyModel->mapToSource(index);
        const QModelIndex revisionIndex = dolphinModel->index(dirIndex.row(), DolphinModel::Version, dirIndex.parent());
        const QVariant data = dolphinModel->data(revisionIndex, Qt::DecorationRole);
        const KVersionControlPlugin::VersionState state = static_cast<KVersionControlPlugin::VersionState>(data.toInt());

        if (state != KVersionControlPlugin::UnversionedVersion) {
            const QRect rect = iconRect(option, index);
            const QPixmap emblem = emblemForState(state, rect.size());
            painter->drawPixmap(rect.x(), rect.y() + rect.height() - emblem.height(), emblem);
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

QPixmap DolphinFileItemDelegate::emblemForState(KVersionControlPlugin::VersionState state, const QSize& size) const
{
    // TODO: all icons that are use here will be replaced by revision control emblems provided by the
    // Oxygen team before KDE 4.4
    Q_ASSERT(state <= KVersionControlPlugin::ConflictingVersion);
    if ((m_cachedSize != size) || m_cachedEmblems[state].isNull()) {
        m_cachedSize = size;

        const int iconHeight = size.height();
        int emblemHeight = KIconLoader::SizeSmall;
        if (iconHeight >= KIconLoader::SizeEnormous) {
            emblemHeight = KIconLoader::SizeMedium;
        } else if (iconHeight >= KIconLoader::SizeLarge) {
            emblemHeight = KIconLoader::SizeSmallMedium;
        } else if (iconHeight >= KIconLoader::SizeMedium) {
            emblemHeight = KIconLoader::SizeSmall;
        } else {
            emblemHeight = KIconLoader::SizeSmall / 2;
        }

        const QSize emblemSize(emblemHeight, emblemHeight);
        for (int i = 0; i <= KVersionControlPlugin::ConflictingVersion; ++i) {
            QString iconName;
            switch (state) {
            case KVersionControlPlugin::NormalVersion:          iconName = "vcs-normal"; break;
            case KVersionControlPlugin::UpdateRequiredVersion:  iconName = "vcs-update-required"; break;
            case KVersionControlPlugin::LocallyModifiedVersion: iconName = "vcs-locally-modified"; break;
            case KVersionControlPlugin::AddedVersion:           iconName = "vcs-added"; break;
            case KVersionControlPlugin::ConflictingVersion:     iconName = "vcs-conflicting"; break;
            default: Q_ASSERT(false); break;
            }

            m_cachedEmblems[i] = KIcon(iconName).pixmap(emblemSize);
        }
    }
    return m_cachedEmblems[state];
}

