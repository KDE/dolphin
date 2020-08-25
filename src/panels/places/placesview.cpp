/*
 * SPDX-FileCopyrightText: 2012 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "placesview.h"

#include "dolphin_placespanelsettings.h"

PlacesView::PlacesView(QGraphicsWidget* parent) :
    KStandardItemListView(parent)
{
    const int iconSize = PlacesPanelSettings::iconSize();
    if (iconSize >= 0) {
        setIconSize(iconSize);
    }
}

void PlacesView::setIconSize(int size)
{
    if (size != iconSize()) {
        PlacesPanelSettings* settings = PlacesPanelSettings::self();
        settings->setIconSize(size);
        settings->save();

        KItemListStyleOption option = styleOption();
        option.iconSize = size;
        setStyleOption(option);
    }
}

int PlacesView::iconSize() const
{
    const KItemListStyleOption option = styleOption();
    return option.iconSize;
}

