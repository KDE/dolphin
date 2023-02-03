/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTSTYLEOPTION_H
#define KITEMLISTSTYLEOPTION_H

#include "dolphin_export.h"

#include <QFont>
#include <QFontMetrics>
#include <QPalette>
#include <QRect>

class DOLPHIN_EXPORT KItemListStyleOption
{
public:
    KItemListStyleOption();
    virtual ~KItemListStyleOption();

    QRect rect;
    QFont font;
    QFontMetrics fontMetrics;
    QPalette palette;
    int padding;
    int horizontalMargin;
    int verticalMargin;
    int iconSize;
    bool extendedSelectionRegion;
    int maxTextLines;
    int maxTextWidth;

    bool operator==(const KItemListStyleOption &other) const;
    bool operator!=(const KItemListStyleOption &other) const;
};
#endif
