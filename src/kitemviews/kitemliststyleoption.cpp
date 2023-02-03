/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemliststyleoption.h"

KItemListStyleOption::KItemListStyleOption()
    : rect()
    , font()
    , fontMetrics(QFont())
    , palette()
    , padding(-1)
    , horizontalMargin(-1)
    , verticalMargin(-1)
    , iconSize(-1)
    , extendedSelectionRegion(false)
    , maxTextLines(0)
    , maxTextWidth(0)
{
}

KItemListStyleOption::~KItemListStyleOption()
{
}

bool KItemListStyleOption::operator==(const KItemListStyleOption &other) const
{
    return rect == other.rect && font == other.font && fontMetrics == other.fontMetrics && palette == other.palette && padding == other.padding
        && horizontalMargin == other.horizontalMargin && verticalMargin == other.verticalMargin && iconSize == other.iconSize
        && extendedSelectionRegion == other.extendedSelectionRegion && maxTextLines == other.maxTextLines && maxTextWidth == other.maxTextWidth;
}

bool KItemListStyleOption::operator!=(const KItemListStyleOption &other) const
{
    return !(*this == other);
}
