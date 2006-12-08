/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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

#ifndef DOLPHINDETAILSVIEW_H
#define DOLPHINDETAILSVIEW_H

#include <QListView>

class DolphinView;

/**
 * @brief Represents the details view which shows the name, size,
 * date, permissions, owner and group of an item.
 *
 * The width of the columns are automatically adjusted in a way
 * that full available width of the view is used by stretching the width
 * of the name column.
 *
 * @author Peter Penz
 */
class DolphinDetailsView : public QListView
{
    Q_OBJECT

public:
    DolphinDetailsView(DolphinView* parent);
    virtual ~DolphinDetailsView();
};

#endif
