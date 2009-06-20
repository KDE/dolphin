/***************************************************************************
 *   Copyright (C) 2008 by Simon St James <kdedevel@etotheipiplusone.com>  *
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

#include "dolphintooltip.h"

#include <kicon.h>
#include <kio/previewjob.h>
#include <kfileitem.h>

#include <QtGui/QPixmap>

DolphinBalloonTooltipDelegate::DolphinBalloonTooltipDelegate()
{
}

DolphinBalloonTooltipDelegate::~DolphinBalloonTooltipDelegate()
{
}

// Delegate everything to the base class, after re-setting the decorationSize
// to the preview size.
QSize DolphinBalloonTooltipDelegate::sizeHint(const KStyleOptionToolTip& option, const KToolTipItem& item) const
{
    KStyleOptionToolTip updatedStyleOption = option;
    updatedStyleOption.decorationSize = QSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);
    return KFormattedBalloonTipDelegate::sizeHint(updatedStyleOption, item);
}

void DolphinBalloonTooltipDelegate::paint(QPainter* painter,
                                          const KStyleOptionToolTip& option,
                                          const KToolTipItem& item) const
{
    KStyleOptionToolTip updatedStyleOption = option;
    updatedStyleOption.decorationSize = QSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);
    return KFormattedBalloonTipDelegate::paint(painter, updatedStyleOption, item);
}
