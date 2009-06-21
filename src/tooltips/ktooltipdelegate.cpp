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

#include "ktooltipdelegate.h"

#include "ktooltip.h"

#include <QApplication>
#include <QPainter>
#include <QIcon>

#ifdef Q_WS_X11
#  include <QX11Info>
#  include <X11/Xlib.h>
#  include <X11/extensions/shape.h>
#endif


// ----------------------------------------------------------------------------


KStyleOptionToolTip::KStyleOptionToolTip()
    : fontMetrics(QApplication::font())
{
}


// ----------------------------------------------------------------------------


KToolTipDelegate::KToolTipDelegate() : QObject()
{
}

KToolTipDelegate::~KToolTipDelegate()
{
}

QSize KToolTipDelegate::sizeHint(const KStyleOptionToolTip &option, const KToolTipItem &item) const
{
    QSize size;
    size.rwidth() = option.fontMetrics.width(item.text());
    size.rheight() = option.fontMetrics.lineSpacing();

    QIcon icon = item.icon();
    if (!icon.isNull()) {
        const QSize iconSize = icon.actualSize(option.decorationSize);
        size.rwidth() += iconSize.width() + 4;
        size.rheight() = qMax(size.height(), iconSize.height());
    }

    return size + QSize(20, 20);

}

void KToolTipDelegate::paint(QPainter *painter,
                             const KStyleOptionToolTip &option,
                             const KToolTipItem &item) const
{
    bool haveAlpha = haveAlphaChannel();
    painter->setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    if (haveAlpha)
        path.addRoundRect(option.rect.adjusted(0, 0, -1, -1), 25);
    else
        path.addRect(option.rect.adjusted(0, 0, -1, -1));

    QColor color = option.palette.color(QPalette::ToolTipBase);
    QColor from = color.lighter(105);
    QColor to   = color.darker(120);

    QLinearGradient gradient(0, 0, 0, 1);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setColorAt(0, from);
    gradient.setColorAt(1, to);

    painter->translate(.5, .5);
    painter->setPen(QPen(Qt::black, 1));
    painter->setBrush(gradient);
    painter->drawPath(path);
    painter->translate(-.5, -.5);

    if (haveAlpha) {
        QLinearGradient mask(0, 0, 0, 1);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        gradient.setColorAt(0, QColor(0, 0, 0, 192));
        gradient.setColorAt(1, QColor(0, 0, 0, 72));
        painter->setCompositionMode(QPainter::CompositionMode_DestinationIn);
        painter->fillRect(option.rect, gradient);
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    QRect textRect = option.rect.adjusted(10, 10, -10, -10);

    QIcon icon = item.icon();
    if (!icon.isNull()) {
        const QSize iconSize = icon.actualSize(option.decorationSize);
        painter->drawPixmap(textRect.topLeft(), icon.pixmap(iconSize));
        textRect.adjust(iconSize.width() + 4, 0, 0, 0);
    }
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, item.text());
}

QRegion KToolTipDelegate::inputShape(const KStyleOptionToolTip &option) const
{
    return QRegion(option.rect);
}

QRegion KToolTipDelegate::shapeMask(const KStyleOptionToolTip &option) const
{
    return QRegion(option.rect);
}

bool KToolTipDelegate::haveAlphaChannel() const
{
#ifdef Q_WS_X11
    return QX11Info::isCompositingManagerRunning();
#else
	return false;
#endif
}
