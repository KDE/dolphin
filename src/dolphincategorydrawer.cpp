/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "dolphincategorydrawer.h"

#include <config-nepomuk.h>

#include <QPainter>
#include <QFile>
#include <QDir>
#include <QApplication>
#include <QStyleOption>

#ifdef HAVE_NEPOMUK
#include <nepomuk/kratingpainter.h>
#endif

#include <kiconloader.h>
#include <kcategorizedsortfilterproxymodel.h>
#include <qimageblitz.h>
#include <kuser.h>

#include "dolphinview.h"
#include "dolphinmodel.h"

#define HORIZONTAL_HINT 3

DolphinCategoryDrawer::DolphinCategoryDrawer()
{
}

DolphinCategoryDrawer::~DolphinCategoryDrawer()
{
}

void DolphinCategoryDrawer::drawCategory(const QModelIndex &index, int sortRole,
                                         const QStyleOption &option, QPainter *painter) const
{
    Q_UNUSED(sortRole);

    QRect starRect = option.rect;

    int iconSize = KIconLoader::global()->currentSize(KIconLoader::Small);
    QVariant categoryVariant = index.model()->data(index, KCategorizedSortFilterProxyModel::CategoryDisplayRole);

    if (!categoryVariant.isValid())
    {
        return;
    }

    const QString category = categoryVariant.toString();

    QColor color;

    if (option.state & QStyle::State_Selected)
    {
        color = option.palette.color(QPalette::HighlightedText);
    }
    else
    {
        color = option.palette.color(QPalette::Text);
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QStyleOptionButton opt;

    opt.rect = option.rect;
    opt.rect.setLeft(opt.rect.left() + HORIZONTAL_HINT);
    opt.rect.setRight(opt.rect.right() - HORIZONTAL_HINT);
    opt.palette = option.palette;
    opt.direction = option.direction;
    opt.text = category;

    QStyleOptionViewItemV4 viewOptions;
    viewOptions.rect = option.rect;
    viewOptions.palette = option.palette;
    viewOptions.direction = option.direction;
    viewOptions.state = option.state;
    viewOptions.viewItemPosition = QStyleOptionViewItemV4::OnlyOne;
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &viewOptions, painter, 0);

    QFont painterFont = painter->font();
    painterFont.setWeight(QFont::Bold);
    QFontMetrics metrics(painterFont);
    painter->setFont(painterFont);

    QPainterPath path;
    path.addRect(option.rect.left(),
                 option.rect.bottom() - 1,
                 option.rect.width(),
                 1);

    QLinearGradient gradient(option.rect.topLeft(),
                             option.rect.bottomRight());
    gradient.setColorAt(option.direction == Qt::LeftToRight ? 0
                                                            : 1, color);
    gradient.setColorAt(option.direction == Qt::LeftToRight ? 1
                                                            : 0, Qt::transparent);

    painter->setBrush(gradient);
    painter->fillPath(path, gradient);

    if (option.direction == Qt::LeftToRight)
    {
        opt.rect.setLeft(opt.rect.left());
        starRect.setLeft(starRect.left());
        starRect.setRight(starRect.right());
    }
    else
    {
        opt.rect.setRight(opt.rect.width());
        starRect.setLeft(starRect.width());
        starRect.setRight(starRect.width());
    }

    bool paintIcon = true;
    bool paintText = true;

    QPixmap icon;
    switch (index.column()) {
        case KDirModel::Name:
            paintIcon = false;
            break;

        case KDirModel::Size:
            paintIcon = false;
            break;

        case KDirModel::ModifiedTime:
            paintIcon = false;
            break;

        case KDirModel::Permissions:
            paintIcon = false; // TODO: let's think about how to represent permissions
            break;

        case KDirModel::Owner: {
            opt.rect.setTop(option.rect.bottom() - (iconSize / 4));
            KUser user(category);
            QString faceIconPath = user.faceIconPath();

            if (!faceIconPath.isEmpty())
            {
                icon = QPixmap::fromImage(QImage(faceIconPath).scaledToHeight(option.fontMetrics.height(), Qt::SmoothTransformation));
            }
            else
            {
                icon = KIconLoader::global()->loadIcon("user-identity", KIconLoader::NoGroup, option.fontMetrics.height());
            }

            opt.rect.setTop(opt.rect.top() - icon.height());

            break;
        }

        case KDirModel::Group:
            paintIcon = false;
            break;

        case KDirModel::Type: {
            opt.rect.setTop(option.rect.bottom() - (iconSize / 4));
            const KCategorizedSortFilterProxyModel *proxyModel = static_cast<const KCategorizedSortFilterProxyModel*>(index.model());
            const DolphinModel *model = static_cast<const DolphinModel*>(proxyModel->sourceModel());
            KFileItem item = model->itemForIndex(proxyModel->mapToSource(index));
            // This is the only way of getting the icon right. Others will fail on corner
            // cases like the item representing this group has been set a different icon,
            // so the group icon drawn is that one particularly. This way assures the drawn
            // icon is the one of the mimetype of the group itself. (ereslibre)
            icon = KIconLoader::global()->loadMimeTypeIcon(item.mimeTypePtr()->iconName(),
                                                           KIconLoader::NoGroup, option.fontMetrics.height());

            opt.rect.setTop(opt.rect.top() - icon.height());

            break;
        }

#ifdef HAVE_NEPOMUK
        case DolphinModel::Rating: {
            paintText = false;
            paintIcon = false;

            painter->setLayoutDirection( option.direction );
            QRect ratingRect( option.rect );
            ratingRect.setTop(option.rect.top() + (option.rect.height() / 2) - (iconSize / 2));
            ratingRect.setHeight( iconSize );
            KRatingPainter::paintRating( painter, ratingRect, Qt::AlignLeft, category.toInt() );
            break;
        }

        case DolphinModel::Tags:
            paintIcon = false;
            break;
#endif
    }

    if (paintIcon) {
        painter->drawPixmap(QRect(option.direction == Qt::LeftToRight ? opt.rect.left()
                                                                      : opt.rect.right() - icon.width() + (iconSize / 4), opt.rect.top(), icon.width(), icon.height()), icon);

        if (option.direction == Qt::LeftToRight)
        {
            opt.rect.setLeft(opt.rect.left() + icon.width() + (iconSize / 4));
        }
        else
        {
            opt.rect.setRight(opt.rect.right() + (iconSize / 4));
        }
    }

    if (paintText) {
        opt.rect.setTop(option.rect.top() + (iconSize / 4));
        opt.rect.setBottom(opt.rect.bottom() - 1);
        painter->setPen(color);

        QRect textRect = opt.rect;

        if (option.direction == Qt::RightToLeft)
        {
            textRect.setWidth(textRect.width() - (paintIcon ? icon.width() + (iconSize / 4)
                                                            : -(iconSize / 4)));
        }

        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
        metrics.elidedText(category, Qt::ElideRight, textRect.width()));
    }

    painter->restore();
}

int DolphinCategoryDrawer::categoryHeight(const QModelIndex &index, const QStyleOption &option) const
{
    int iconSize = KIconLoader::global()->currentSize(KIconLoader::Small);
    int heightWithoutIcon = option.fontMetrics.height() + (iconSize / 4) * 2 + 1; /* 1 pixel-width gradient */
    bool paintIcon;

    switch (index.column()) {
        case KDirModel::Owner:
        case KDirModel::Type:
            paintIcon = true;
            break;
        default:
            paintIcon = false;
    }

    if (paintIcon)
        return qMax(heightWithoutIcon, iconSize + (iconSize / 4) * 2 + 1) /* 1 pixel-width gradient */;

    return heightWithoutIcon;
}
