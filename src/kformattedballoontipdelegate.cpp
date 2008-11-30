/*******************************************************************************
 *   Copyright (C) 2008 by Fredrik Höglund <fredrik@kde.org>                   *
 *   Copyright (C) 2008 by Konstantin Heil <konst.heil@stud.uni-heidelberg.de> *
 *                                                                             *
 *   This program is free software; you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by      *
 *   the Free Software Foundation; either version 2 of the License, or         *
 *   (at your option) any later version.                                       *
 *                                                                             *
 *   This program is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *   GNU General Public License for more details.                              *
 *                                                                             *
 *   You should have received a copy of the GNU General Public License         *
 *   along with this program; if not, write to the                             *
 *   Free Software Foundation, Inc.,                                           *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA                *
 *******************************************************************************/

#include "kformattedballoontipdelegate.h"
#include <QBitmap>
#include <QLinearGradient>
#include <QTextDocument>
#include <kcolorscheme.h>

KFormattedBalloonTipDelegate::KFormattedBalloonTipDelegate()
{
}

KFormattedBalloonTipDelegate::~KFormattedBalloonTipDelegate()
{
}

QSize KFormattedBalloonTipDelegate::sizeHint(const KStyleOptionToolTip *option, const KToolTipItem *item) const
{
    QTextDocument doc;
    doc.setHtml(item->text());
    const QIcon icon = item->icon();
        
    const QSize iconSize = icon.isNull() ? QSize(0, 0) : icon.actualSize(option->decorationSize);
    const QSize docSize = doc.size().toSize();
    QSize contentSize = iconSize + docSize;
    
    // assure that the content height is large enough for the icon and the document
    contentSize.setHeight(iconSize.height() > doc.size().height() ? iconSize.height() : doc.size().height());
    return contentSize + QSize(Border * 3, Border * 2);
}

void KFormattedBalloonTipDelegate::paint(QPainter *painter,
                                         const KStyleOptionToolTip *option,
                                         const KToolTipItem *item) const
{
    QPainterPath path = createPath(*option);
    if (haveAlphaChannel()) {
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(.5, .5);
    }

    const QColor toColor = option->palette.brush(QPalette::ToolTipBase).color();
    const QColor fromColor = KColorScheme::shade(toColor, KColorScheme::LightShade, 0.2);
    
    QLinearGradient gradient(option->rect.topLeft(), option->rect.bottomLeft());
    gradient.setColorAt(0.0, fromColor);
    gradient.setColorAt(1.0, toColor);
    painter->setPen(Qt::NoPen);
    painter->setBrush(gradient);

    painter->drawPath(path);

    const QIcon icon = item->icon();
    int x = Border;
    const int y = Border;
    if (!icon.isNull()) {
        const QSize iconSize = icon.actualSize(option->decorationSize);
        const QPoint pos(x + option->rect.x(), y + option->rect.y());
        painter->drawPixmap(pos, icon.pixmap(iconSize));
        x += iconSize.width() + Border;
    }

    QTextDocument doc;
    doc.setHtml(item->text());
    QPixmap bitmap(doc.size().toSize());
    bitmap.fill(Qt::transparent);
    QPainter p(&bitmap);
    doc.drawContents(&p);
 
    const QRect docRect(QPoint(x, y), doc.size().toSize());
    painter->drawPixmap(docRect, bitmap);
}

QRegion KFormattedBalloonTipDelegate::inputShape(const KStyleOptionToolTip *option) const
{
    QBitmap bitmap(option->rect.size());
    bitmap.fill(Qt::color0);

    QPainter p(&bitmap);
    p.setPen(QPen(Qt::color1, 1));
    p.setBrush(Qt::color1);
    p.drawPath(createPath(*option));

    return QRegion(bitmap);
}

QRegion KFormattedBalloonTipDelegate::shapeMask(const KStyleOptionToolTip *option) const
{
    return inputShape(option);
}

static inline void arc(QPainterPath &path, qreal cx, qreal cy, qreal radius, qreal angle, qreal sweeplength)
{
    path.arcTo(cx-radius, cy-radius, radius * 2, radius * 2, angle, sweeplength);
}

QPainterPath KFormattedBalloonTipDelegate::createPath(const KStyleOptionToolTip& option) const
{
    const QRect rect = option.rect;
    const qreal radius = 5;

    QPainterPath path;
    path.moveTo(rect.left(), rect.top() + radius);
    arc(path, rect.left()  + radius, rect.top()    + radius, radius, 180, -90);
    arc(path, rect.right() - radius, rect.top()    + radius, radius,  90, -90);
    arc(path, rect.right() - radius, rect.bottom() - radius, radius,   0, -90);
    arc(path, rect.left()  + radius, rect.bottom() - radius, radius, 270, -90);
    path.closeSubpath();

    return path;
}
