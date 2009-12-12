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

    QStyleOptionViewItemV4 opt(option);
    if (m_hasMinimizedNameColumn && isNameColumn) {
        adjustOptionWidth(opt, proxyModel, dolphinModel, index);
    }

    if (dolphinModel->hasVersionData() && isNameColumn) {
        // The currently shown items are under revision control. Show the current revision
        // state by adding an emblem and changing the text tintColor.
        const QModelIndex dirIndex = proxyModel->mapToSource(index);
        const QModelIndex revisionIndex = dolphinModel->index(dirIndex.row(), DolphinModel::Version, dirIndex.parent());
        const QVariant data = dolphinModel->data(revisionIndex, Qt::DecorationRole);
        const KVersionControlPlugin::VersionState state = static_cast<KVersionControlPlugin::VersionState>(data.toInt());

        adjustOptionTextColor(opt, state);

        KFileItemDelegate::paint(painter, opt, index);

        if (state != KVersionControlPlugin::UnversionedVersion) {
            const QRect rect = iconRect(option, index);
            const QPixmap emblem = emblemForState(state, rect.size());
            painter->drawPixmap(rect.x(), rect.y() + rect.height() - emblem.height(), emblem);
        }
    } else {
        KFileItemDelegate::paint(painter, opt, index);
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

void DolphinFileItemDelegate::adjustOptionTextColor(QStyleOptionViewItemV4& option,
                                                    KVersionControlPlugin::VersionState state)
{
    QColor tintColor;

    // Using hardcoded colors is generally a bad idea. In this case the colors just act
    // as tint colors and are mixed with the current set text color. The tint colors
    // have been optimized for the base colors of the corresponding Oxygen emblems.
    switch (state) {
    case KVersionControlPlugin::UpdateRequiredVersion:  tintColor = Qt::yellow; break;
    case KVersionControlPlugin::LocallyModifiedVersion: tintColor = Qt::green; break;
    case KVersionControlPlugin::AddedVersion:           tintColor = Qt::darkGreen; break;
    case KVersionControlPlugin::RemovedVersion:         tintColor = Qt::darkRed; break;
    case KVersionControlPlugin::ConflictingVersion:     tintColor = Qt::red; break;
    case KVersionControlPlugin::UnversionedVersion:
    case KVersionControlPlugin::NormalVersion:
    default:
        // use the default text color
        return;
    }

    QPalette palette = option.palette;
    const QColor textColor = palette.color(QPalette::Text);
    tintColor = QColor((tintColor.red()   + textColor.red())   / 2,
                       (tintColor.green() + textColor.green()) / 2,
                       (tintColor.blue()  + textColor.blue())  / 2,
                       (tintColor.alpha() + textColor.alpha()) / 2);
    palette.setColor(QPalette::Text, tintColor);
    option.palette = palette;
}

QPixmap DolphinFileItemDelegate::emblemForState(KVersionControlPlugin::VersionState state, const QSize& size) const
{
    Q_ASSERT(state <= KVersionControlPlugin::ConflictingVersion);
    if (m_cachedSize != size) {
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
        for (int i = KVersionControlPlugin::NormalVersion; i <= KVersionControlPlugin::ConflictingVersion; ++i) {
            QString iconName;
            switch (i) {
            case KVersionControlPlugin::NormalVersion:          iconName = "vcs-normal"; break;
            case KVersionControlPlugin::UpdateRequiredVersion:  iconName = "vcs-update-required"; break;
            case KVersionControlPlugin::LocallyModifiedVersion: iconName = "vcs-locally-modified"; break;
            case KVersionControlPlugin::AddedVersion:           iconName = "vcs-added"; break;
            case KVersionControlPlugin::RemovedVersion:         iconName = "vcs-removed"; break;
            case KVersionControlPlugin::ConflictingVersion:     iconName = "vcs-conflicting"; break;
            case KVersionControlPlugin::UnversionedVersion:
            default:                                            Q_ASSERT(false); break;
            }

            m_cachedEmblems[i] = KIcon(iconName).pixmap(emblemSize);
        }
    }
    return m_cachedEmblems[state];
}

