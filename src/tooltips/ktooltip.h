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

#ifndef KTOOLTIP_H
#define KTOOLTIP_H

#include <tooltips/ktooltipitem.h>

#include <QStyle>
#include <QFontMetrics>
class KToolTipDelegate;

/**
 * KToolTip provides customizable tooltips that can have animations as well as an alpha
 * channel, allowing for dynamic transparency effects.
 *
 * ARGB tooltips work on X11 even when the application isn't using the ARGB visual.
 */
namespace KToolTip
{
    void showText(const QPoint &pos, const QString &text, QWidget *widget, const QRect &rect);
    void showText(const QPoint &pos, const QString &text, QWidget *widget = 0);

    /**
     * Shows the tip @p item at the global position indicated by @p pos.
     *
     * Ownership of the item is transferred to KToolTip. The item will be deleted
     * automatically when it is hidden.
     *
     * The tip is shown immediately when this function is called.
     */
    void showTip(const QPoint &pos, KToolTipItem *item);
    void hideTip();

    void setToolTipDelegate(KToolTipDelegate *delegate);
}

#endif
