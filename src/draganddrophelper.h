/***************************************************************************
 *   Copyright (C) 2007 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef DRAGANDDROPHELPER_H
#define DRAGANDDROPHELPER_H

#include <QtCore/Qt>

class QAbstractItemView;
class QBrush;
class QRect;
class QWidget;

/**
 * @brief Helper class to bypass some drag & drop limitations in Qt.
 *
 * The class is used by DolphinIconsView, DolphinDetailsView,
 * DolphinColumnView and SidebarTreeView to have a consistent
 * drag and drop behavior between all views.
 */
class DragAndDropHelper
{

public:
    /**
     * Creates a drag object for the view \a itemView for all selected items.
     */
    static void startDrag(QAbstractItemView* itemView, Qt::DropActions supportedActions);

    // TODO: remove this method when the issue #160611 is solved in Qt 4.4
    static void drawHoverIndication(QWidget* widget,
                                    const QRect& bounds,
                                    const QBrush& brush);
};

#endif
