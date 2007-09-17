/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@gmail.com>
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

#include <QPainter>
#include <QFile>
#include <QDir>

#include <kiconloader.h>
#include <kcategorizedsortfilterproxymodel.h>
#include <kpixmapeffect.h>
#include <kuser.h>

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
#include <nepomuk/global.h>
#include <nepomuk/resource.h>
#include <nepomuk/tag.h>
#endif

#include "dolphinview.h"
#include "dolphinmodel.h"

DolphinCategoryDrawer::DolphinCategoryDrawer()
{
}

DolphinCategoryDrawer::~DolphinCategoryDrawer()
{
}

void DolphinCategoryDrawer::drawCategory(const QModelIndex &index, int sortRole,
                                         const QStyleOption &option, QPainter *painter) const
{
    QRect starRect = option.rect;

    int iconSize =  KIconLoader::global()->currentSize(K3Icon::Small);
    QVariant categoryVariant = index.model()->data(index, KCategorizedSortFilterProxyModel::CategoryRole);

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
    opt.palette = option.palette;
    opt.direction = option.direction;
    opt.text = category;

    if (option.state & QStyle::State_Selected)
    {
        QColor selected = option.palette.color(QPalette::Highlight);

        QLinearGradient gradient(option.rect.topLeft(),
                                 option.rect.bottomRight());
        gradient.setColorAt(option.direction == Qt::LeftToRight ? 0
                                                                : 1, selected);
        gradient.setColorAt(option.direction == Qt::LeftToRight ? 1
                                                                : 0, Qt::transparent);

        painter->fillRect(option.rect, gradient);
    }
    else if (option.state & QStyle::State_MouseOver)
    {
        QColor hover = option.palette.color(QPalette::Highlight).light();
        hover.setAlpha(88);

        QLinearGradient gradient(option.rect.topLeft(),
                                 option.rect.bottomRight());
        gradient.setColorAt(option.direction == Qt::LeftToRight ? 0
                                                                : 1, hover);
        gradient.setColorAt(option.direction == Qt::LeftToRight ? 1
                                                                : 0, Qt::transparent);

        painter->fillRect(option.rect, gradient);
    }

    QFont painterFont = painter->font();
    painterFont.setWeight(QFont::Bold);
    QFontMetrics metrics(painterFont);
    painter->setFont(painterFont);

    QPainterPath path;
    path.addRect(option.rect.left(),
                 option.rect.bottom() - 2,
                 option.rect.width(),
                 2);

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
        opt.rect.setLeft(opt.rect.left() + (iconSize / 4));
        starRect.setLeft(starRect.left() + (iconSize / 4));
        starRect.setRight(starRect.right() + (iconSize / 4));
    }
    else
    {
        opt.rect.setRight(opt.rect.width() - (iconSize / 4));
        starRect.setLeft(starRect.width() - iconSize);
        starRect.setRight(starRect.width() - (iconSize / 4));
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
            opt.rect.setTop(option.rect.top() + (iconSize / 4));
            KUser user(category);
            if (QFile::exists(user.homeDir() + QDir::separator() + ".face.icon"))
            {
                icon = QPixmap::fromImage(QImage(user.homeDir() + QDir::separator() + ".face.icon")).scaled(iconSize, iconSize);
            }
            else
            {
                icon = KIconLoader::global()->loadIcon("user", K3Icon::Small);
            }
            break;
        }

        case KDirModel::Group:
            paintIcon = false;
            break;

        case KDirModel::Type: {
            opt.rect.setTop(option.rect.top() + (option.rect.height() / 2) - (iconSize / 2));
            const KCategorizedSortFilterProxyModel *proxyModel = static_cast<const KCategorizedSortFilterProxyModel*>(index.model());
            const DolphinModel *model = static_cast<const DolphinModel*>(proxyModel->sourceModel());
            KFileItem item = model->itemForIndex(proxyModel->mapToSource(index));
            // This is the only way of getting the icon right. Others will fail on corner
            // cases like the item representing this group has been set a different icon,
            // so the group icon drawn is that one particularly. This way assures the drawn
            // icon is the one of the mimetype of the group itself. (ereslibre)
            icon = KIconLoader::global()->loadMimeTypeIcon(item.mimeTypePtr()->iconName(),
                                                           K3Icon::Small);
            break;
        }

#ifdef HAVE_NEPOMUK
        case DolphinModel::Rating: {
            paintText = false;
            paintIcon = false;

            starRect.setTop(option.rect.top() + (option.rect.height() / 2) - (iconSize / 2));
            starRect.setSize(QSize(iconSize, iconSize));

            QPixmap pixmap = KIconLoader::global()->loadIcon("rating", K3Icon::Small);
            QPixmap smallPixmap = KIconLoader::global()->loadIcon("rating", K3Icon::NoGroup, iconSize / 2);
            QPixmap disabledPixmap = KIconLoader::global()->loadIcon("rating", K3Icon::Small);

            KPixmapEffect::toGray(disabledPixmap, false);

            int rating = category.toInt();

            for (int i = 0; i < rating - (rating % 2); i += 2) {
                painter->drawPixmap(starRect, pixmap);

                if (option.direction == Qt::LeftToRight)
                {
                    starRect.setLeft(starRect.left() + iconSize + (iconSize / 4) /* separator between stars */);
                    starRect.setRight(starRect.right() + iconSize + (iconSize / 4) /* separator between stars */);
                }
                else
                {
                    starRect.setLeft(starRect.left() - iconSize - (iconSize / 4) /* separator between stars */);
                    starRect.setRight(starRect.right() - iconSize - (iconSize / 4) /* separator between stars */);
                }
            }

            if (rating && rating % 2) {
                if (option.direction == Qt::RightToLeft)
                {
                    starRect.setLeft(starRect.left() + (iconSize / 2) /* separator between stars */);
                }

                starRect.setTop(option.rect.top() + (option.rect.height() / 2) - (iconSize / 4));
                starRect.setSize(QSize(iconSize / 2, iconSize / 2));
                painter->drawPixmap(starRect, smallPixmap);
                starRect.setTop(opt.rect.top() + (option.rect.height() / 2) - (iconSize / 2));

                if (option.direction == Qt::LeftToRight)
                {
                    starRect.setLeft(starRect.left() + (iconSize / 2) + (iconSize / 4));
                    starRect.setRight(starRect.right() + (iconSize / 2) + (iconSize / 4));
                }
                else
                {
                    starRect.setLeft(starRect.left() - (iconSize / 2) - (iconSize / 4));
                    starRect.setRight(starRect.right() - (iconSize / 2) - (iconSize / 4));
                }

                if (option.direction == Qt::RightToLeft)
                {
                    starRect.setLeft(starRect.left() - (iconSize / 2));
                    starRect.setRight(starRect.right() - (iconSize / 2));
                }

                starRect.setSize(QSize(iconSize, iconSize));
            }

            for (int i = rating; i < 9; i += 2) {
                painter->drawPixmap(starRect, disabledPixmap);

                if (option.direction == Qt::LeftToRight)
                {
                    starRect.setLeft(starRect.left() + iconSize + (iconSize / 4));
                    starRect.setRight(starRect.right() + iconSize + (iconSize / 4));
                }
                else
                {
                    starRect.setLeft(starRect.left() - iconSize - (iconSize / 4));
                    starRect.setRight(starRect.right() - iconSize - (iconSize / 4));
                }
            }

            break;
        }

        case DolphinModel::Tags:
            paintIcon = false;
            break;
#endif
    }

    if (paintIcon) {
        painter->drawPixmap(QRect(option.direction == Qt::LeftToRight ? opt.rect.left()
                                                                      : opt.rect.right() - iconSize + (iconSize / 4), opt.rect.top(), iconSize, iconSize), icon);

        if (option.direction == Qt::LeftToRight)
        {
            opt.rect.setLeft(opt.rect.left() + iconSize + (iconSize / 4));
        }
        else
        {
            opt.rect.setRight(opt.rect.right() + (iconSize / 4));
        }
    }

    if (paintText) {
        opt.rect.setTop(option.rect.top() + (iconSize / 4));
        opt.rect.setBottom(opt.rect.bottom() - 2);
        painter->setPen(color);

        QRect textRect = opt.rect;

        if (option.direction == Qt::RightToLeft)
        {
            textRect.setWidth(textRect.width() - (paintIcon ? iconSize + (iconSize / 2)
                                                            : -(iconSize / 4)));
        }

        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
        metrics.elidedText(category, Qt::ElideRight, textRect.width()));
    }

    painter->restore();
}

int DolphinCategoryDrawer::categoryHeight(const QStyleOption &option) const
{
    int iconSize = KIconLoader::global()->currentSize(K3Icon::Small);

    return qMax(option.fontMetrics.height() + (iconSize / 4) * 2 + 2, iconSize + (iconSize / 4) * 2 + 2) /* 2 gradient */;
}
