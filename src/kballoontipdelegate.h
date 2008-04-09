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

#ifndef KBALLOONTIPDELEGATE_H
#define KBALLOONTIPDELEGATE_H

#include "ktooltip.h"

class KBalloonTipDelegate : public KToolTipDelegate
{
public:
    KBalloonTipDelegate() {}
    ~KBalloonTipDelegate() {}

    QSize sizeHint(const KStyleOptionToolTip *option, const KToolTipItem *item) const;
    void paint(QPainter *painter, const KStyleOptionToolTip *option, const KToolTipItem *item) const;
    QRegion inputShape(const KStyleOptionToolTip *option) const;
    QRegion shapeMask(const KStyleOptionToolTip *option) const;

private:
    QPainterPath createPath(const KStyleOptionToolTip *option, QRect *contentRect = 0) const;
};

#endif
