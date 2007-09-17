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

#include "kcategorydrawer.h"

#include <QPainter>
#include <QStyleOption>

#include <kcategorizedsortfilterproxymodel.h>

KCategoryDrawer::KCategoryDrawer()
{
}

KCategoryDrawer::~KCategoryDrawer()
{
}

void KCategoryDrawer::drawCategory(const QModelIndex &index,
                                   int sortRole,
                                   const QStyleOption &option,
                                   QPainter *painter) const
{
    const QString category = index.model()->data(index, KCategorizedSortFilterProxyModel::CategoryRole).toString();

    QColor color = option.palette.color(QPalette::Text);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QStyleOptionButton opt;

    opt.rect = option.rect;
    opt.palette = option.palette;
    opt.direction = option.direction;
    opt.text = category;

    if (option.state & QStyle::State_MouseOver)
    {
        const QPalette::ColorGroup group =
                                          option.state & QStyle::State_Enabled ?
                                          QPalette::Normal : QPalette::Disabled;

        QLinearGradient gradient(option.rect.topLeft(),
                                 option.rect.bottomRight());
        gradient.setColorAt(0,
                            option.palette.color(group,
                                                 QPalette::Highlight).light());
        gradient.setColorAt(1, Qt::transparent);

        painter->fillRect(option.rect, gradient);
    }

    QFont painterFont = painter->font();
    painterFont.setWeight(QFont::Bold);
    painterFont.setPointSize(painterFont.pointSize() + 2);
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

    painter->setPen(color);

    painter->drawText(option.rect, Qt::AlignVCenter | Qt::AlignLeft,
    metrics.elidedText(category, Qt::ElideRight, option.rect.width()));

    painter->restore();
}

int KCategoryDrawer::categoryHeight(const QStyleOption &option) const
{
    return option.fontMetrics.height() + 6 /* 4 separator; 2 gradient */;
}
