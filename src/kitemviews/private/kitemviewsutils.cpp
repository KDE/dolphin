/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemviewsutils.h"

#include <QApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>

qreal KItemViewsUtils::devicePixelRatio(const QGraphicsItem *item)
{
    qreal dpr = qApp->devicePixelRatio();
    if (item->scene()) {
        if (const auto views = item->scene()->views(); !views.isEmpty()) {
            dpr = views.first()->devicePixelRatioF();
        }
    }
    return dpr;
}
