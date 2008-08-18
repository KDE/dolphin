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
#include <QTextDocument>
#include <kdebug.h>

QSize KFormattedBalloonTipDelegate::sizeHint(const KStyleOptionToolTip *option, const KToolTipItem *item) const
{
    QTextDocument doc;
    doc.setHtml(item->text());
    QIcon icon = item->icon();
        
    QSize iconSize = (icon.isNull()) ? QSize(0,0) : icon.actualSize(option->decorationSize);
    QSize docSize = doc.size().toSize();
    QSize contentSize = iconSize + docSize;
    
    // Re-adjust contentSize's height so that it is the maximum of the icon
    // and doc sizes.
    contentSize.setHeight( iconSize.height() > doc.size().height() ? iconSize.height() : doc.size().height());
    return contentSize + QSize(20+5,20+1);
}

void KFormattedBalloonTipDelegate::paint(QPainter *painter, const KStyleOptionToolTip *option, const KToolTipItem *item) const
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

    QIcon icon = item->icon();
    if (!icon.isNull()) {
        const QSize iconSize = icon.actualSize(option->decorationSize);
        painter->drawPixmap(contents.topLeft(), icon.pixmap(iconSize));
        contents.adjust(iconSize.width() + 4, 0, 0, 0);
    }

    QTextDocument doc;
    doc.setHtml(item->text());
    QPixmap bitmap(doc.size().toSize());
    bitmap.fill(Qt::transparent);
    QPainter p(&bitmap);
    doc.drawContents(&p);
    
    // Ensure doc text is not stretched when icon is larger.
    contents.setSize(doc.size().toSize());

    painter->drawPixmap(contents, bitmap);
}

QRegion KFormattedBalloonTipDelegate::inputShape(const KStyleOptionToolTip *option) const
{
    QBitmap bitmap(option->rect.size()+QSize(20,20));
    bitmap.fill(Qt::color0);

    QPainter p(&bitmap);
    p.setPen(QPen(Qt::color1, 1));
    p.setBrush(Qt::color1);
    p.drawPath(createPath(option, 0));

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

QPainterPath KFormattedBalloonTipDelegate::createPath(const KStyleOptionToolTip *option, QRect *contents) const
{
    QPainterPath path;
    QRect rect = option->rect.adjusted(0, 0, -1, -1);
    qreal radius = 10;

    path.moveTo(rect.left(), rect.top() + radius);
    arc(path, rect.left() + radius, rect.top() + radius, radius, 180, -90);
    arc(path, rect.right() - radius, rect.top() + radius, radius, 90, -90);
    arc(path, rect.right() - radius, rect.bottom() - radius, radius, 0, -90);
    arc(path, rect.left() + radius, rect.bottom() - radius, radius, 270, -90);
    path.closeSubpath();

    if (contents)
        *contents = rect.adjusted(10, 10, -10, -10);

    return path;
}
