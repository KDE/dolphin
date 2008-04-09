/***************************************************************************
 *   Copyright (C) 2008 by Fredrik Höglund <fredrik@kde.org>               *
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

#include "kballoontipdelegate.h"
#include <QPainterPath>
#include <QPainter>
#include <QBitmap>

QSize KBalloonTipDelegate::sizeHint(const KStyleOptionToolTip *option, const KToolTipItem *item) const
{
    QSize size;
    size.rwidth() = option->fontMetrics.width(item->text());
    size.rheight() = option->fontMetrics.lineSpacing();

    QIcon icon = item->icon();
    if (!icon.isNull()) {
        const QSize iconSize = icon.actualSize(option->decorationSize);
        size.rwidth() += iconSize.width() + 4;
        size.rheight() = qMax(size.height(), iconSize.height());
    }

    int margin = 2 * 10 + (option->activeCorner != KStyleOptionToolTip::NoCorner ? 10 : 0);
    return size + QSize(margin, margin);
}

static inline void arc(QPainterPath &path, qreal cx, qreal cy, qreal radius, qreal angle, qreal sweeplength)
{
    path.arcTo(cx-radius, cy-radius, radius * 2, radius * 2, angle, sweeplength);
}

QPainterPath KBalloonTipDelegate::createPath(const KStyleOptionToolTip *option, QRect *contents) const
{
    QPainterPath path;
    QRect rect = option->rect.adjusted(0, 0, -1, -1);
    qreal radius = 10;

    switch (option->activeCorner)
    {
    case KStyleOptionToolTip::TopLeftCorner:
        rect.adjust(10, 10, 0, 0);
        path.moveTo(option->rect.topLeft());
        path.lineTo(rect.left() + radius, rect.top());
        arc(path, rect.right() - radius, rect.top() + radius, radius, 90, -90);
        arc(path, rect.right() - radius, rect.bottom() - radius, radius, 0, -90);
        arc(path, rect.left() + radius, rect.bottom() - radius, radius, 270, -90);
        path.lineTo(rect.left(), rect.top() + radius);
        path.closeSubpath();
        break;

    case KStyleOptionToolTip::TopRightCorner:
        rect.adjust(0, 10, -10, 0);
        path.moveTo(option->rect.topRight());
        path.lineTo(rect.right(), rect.top() + radius);
        arc(path, rect.right() - radius, rect.bottom() - radius, radius, 0, -90);
        arc(path, rect.left() + radius, rect.bottom() - radius, radius, 270, -90);
        arc(path, rect.left() + radius, rect.top() + radius, radius, 180, -90);
        path.lineTo(rect.right() - radius, rect.top());
        path.closeSubpath();
        break;

    case KStyleOptionToolTip::BottomLeftCorner:
        rect.adjust(10, 0, 0, -10);
        path.moveTo(option->rect.bottomLeft());
        path.lineTo(rect.left(), rect.bottom() - radius);
        arc(path, rect.left() + radius, rect.top() + radius, radius, 180, -90);
        arc(path, rect.right() - radius, rect.top() + radius, radius, 90, -90);
        arc(path, rect.right() - radius, rect.bottom() - radius, radius, 0, -90);
        path.lineTo(rect.left() + radius, rect.bottom());
        path.closeSubpath();
        break;

    case KStyleOptionToolTip::BottomRightCorner:
        rect.adjust(0, 0, -10, -10);
        path.moveTo(option->rect.bottomRight());
        path.lineTo(rect.right() - radius, rect.bottom());
        arc(path, rect.left() + radius, rect.bottom() - radius, radius, 270, -90);
        arc(path, rect.left() + radius, rect.top() + radius, radius, 180, -90);
        arc(path, rect.right() - radius, rect.top() + radius, radius, 90, -90);
        path.lineTo(rect.right(), rect.bottom() - radius);
        path.closeSubpath();
        break;

    default:
        path.moveTo(rect.left(), rect.top() + radius);
        arc(path, rect.left() + radius, rect.top() + radius, radius, 180, -90);
	arc(path, rect.right() - radius, rect.top() + radius, radius, 90, -90);
        arc(path, rect.right() - radius, rect.bottom() - radius, radius, 0, -90);
        arc(path, rect.left() + radius, rect.bottom() - radius, radius, 270, -90);
        path.closeSubpath();
        break;
    }

    if (contents)
        *contents = rect.adjusted(10, 10, -10, -10);

    return path;
}

void KBalloonTipDelegate::paint(QPainter *painter, const KStyleOptionToolTip *option, const KToolTipItem *item) const
{
    QRect contents;
    QPainterPath path = createPath(option, &contents);
    bool alpha = haveAlphaChannel();

    if (alpha) {
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(.5, .5);
    }

#if QT_VERSION >= 0x040400
    painter->setBrush(option->palette.brush(QPalette::ToolTipBase));
#else
    painter->setBrush(option->palette.brush(QPalette::Base));
#endif
    painter->drawPath(path);

    if (alpha)
        painter->translate(-.5, -.5);

    QIcon icon = item->icon();
    if (!icon.isNull()) {
        const QSize iconSize = icon.actualSize(option->decorationSize);
        painter->drawPixmap(contents.topLeft(), icon.pixmap(iconSize));
        contents.adjust(iconSize.width() + 4, 0, 0, 0);
    }

    painter->drawText(contents, Qt::AlignLeft | Qt::AlignVCenter, item->text());
}


QRegion KBalloonTipDelegate::inputShape(const KStyleOptionToolTip *option) const
{
    QBitmap bitmap(option->rect.size());
    bitmap.fill(Qt::color0);

    QPainter p(&bitmap);
    p.setPen(QPen(Qt::color1, 1));
    p.setBrush(Qt::color1);
    p.drawPath(createPath(option));

    return QRegion(bitmap);
}

QRegion KBalloonTipDelegate::shapeMask(const KStyleOptionToolTip *option) const
{
    return inputShape(option);
}

