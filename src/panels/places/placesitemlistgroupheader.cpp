/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "placesitemlistgroupheader.h"

PlacesItemListGroupHeader::PlacesItemListGroupHeader(QGraphicsWidget* parent) :
    KStandardItemListGroupHeader(parent)
{
}

PlacesItemListGroupHeader::~PlacesItemListGroupHeader()
{
}

void PlacesItemListGroupHeader::paintSeparator(QPainter* painter, const QColor& color)
{
    Q_UNUSED(painter)
    Q_UNUSED(color)
}

QPalette::ColorRole PlacesItemListGroupHeader::normalTextColorRole() const
{
    return QPalette::WindowText;
}

