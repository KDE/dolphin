/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMVIEWSUTILS_H
#define KITEMVIEWSUTILS_H

#include <qtypes.h>

class QGraphicsItem;

class KItemViewsUtils
{
public:
    static qreal devicePixelRatio(const QGraphicsItem *item);
};

#endif // KITEMVIEWSUTILS_H
