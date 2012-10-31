/***************************************************************************
 *   Copyright (C) 2012 by Frank Reininghaus <frank78ac@googlemail.com>    *
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
        settings->writeConfig();

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

#include "placesview.moc"
