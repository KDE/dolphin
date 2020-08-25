/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PLACESITEMLISTGROUPHEADER_H
#define PLACESITEMLISTGROUPHEADER_H

#include "kitemviews/kstandarditemlistgroupheader.h"

class PlacesItemListGroupHeader : public KStandardItemListGroupHeader
{
    Q_OBJECT

public:
    explicit PlacesItemListGroupHeader(QGraphicsWidget* parent = nullptr);
    ~PlacesItemListGroupHeader() override;

protected:
    void paintSeparator(QPainter* painter, const QColor& color) override;

    QPalette::ColorRole normalTextColorRole() const override;
};
#endif


