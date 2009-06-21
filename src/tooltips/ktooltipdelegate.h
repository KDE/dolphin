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

#ifndef KTOOLTIPDELEGATE_H
#define KTOOLTIPDELEGATE_H

#include <QFontMetrics>
#include <QFont>
#include <QObject>
#include <QPalette>
#include <QRect>
#include <QStyle>
#include <QSize>

class KStyleOptionToolTip;
class KToolTipItem;
class QPainter;
class QRegion;

class KStyleOptionToolTip
{
public:
    KStyleOptionToolTip();
    enum Corner { TopLeftCorner, TopRightCorner, BottomLeftCorner, BottomRightCorner, NoCorner };

    Qt::LayoutDirection direction;
    QFontMetrics fontMetrics;
    QPalette palette;
    QRect rect;
    QStyle::State state;
    QFont font;
    QSize decorationSize;
    Corner activeCorner;
};

/**
 * KToolTipDelegate is responsible for providing the size hint and
 * painting the tooltips.
 */
class KToolTipDelegate : public QObject
{
    Q_OBJECT
public:
     KToolTipDelegate();
     virtual ~KToolTipDelegate();

     virtual QSize sizeHint(const KStyleOptionToolTip &option, const KToolTipItem &item) const;

     /**
      * If haveAlphaChannel() returns true, the paint device will be filled with
      * Qt::transparent when this function is called, otherwise the content is
      * undefined.
      */
     virtual void paint(QPainter *painter,
                        const KStyleOptionToolTip &option,
                        const KToolTipItem &item) const;

     /**
      * Reimplement this function to specify the region of the tooltip
      * that accepts input. Any mouse events that occur outside this
      * region will be sent to the widget below the tooltip.
      *
      * The default implementation returns a region containing the
      * bounding rect of the tooltip.
      *
      * This function will only be called if haveAlphaChannel()
      * returns true.
      */
     virtual QRegion inputShape(const KStyleOptionToolTip &option) const;

     /**
      * Reimplement this function to specify a shape mask for the tooltip.
      *
      * The default implementation returns a region containing the
      * bounding rect of the tooltip.
      *
      * This function will only be called if haveAlphaChannel()
      * returns false.
      */
     virtual QRegion shapeMask(const KStyleOptionToolTip &option) const;

protected:
     /**
      * Returns true if the tooltip has an alpha channel, and false
      * otherwise.
      *
      * Implementors should assume that this condition may change at
      * any time during the runtime of the application, as compositing
      * can be enabled or disabled in the window manager.
      */
     bool haveAlphaChannel() const;
};

#endif
