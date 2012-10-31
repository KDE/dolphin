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

#ifndef PLACESVIEW_H
#define PLACESVIEW_H

#include <kitemviews/kstandarditemlistview.h>

/**
 * @brief View class for the Places Panel.
 *
 * Reads the icon size from GeneralSettings::placesPanelIconSize().
 */
class PlacesView : public KStandardItemListView
{
    Q_OBJECT

public:
    explicit PlacesView(QGraphicsWidget* parent = 0);

    void setIconSize(int size);
    int iconSize() const;
};

#endif
