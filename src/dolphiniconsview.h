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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef DOLPHINICONSVIEW_H
#define DOLPHINICONSVIEW_H

#include <QListView>

class DolphinView;

/**
 * @brief Represents the view, where each item is shown as an icon.
 *
 * It is also possible that instead of the icon a preview of the item
 * content is shown.
 *
 * @author Peter Penz
 */
class DolphinIconsView : public QListView
{
    Q_OBJECT

public:
    DolphinIconsView(DolphinView* parent);
    virtual ~DolphinIconsView();

protected:
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);

private:
    DolphinView* m_parentView;
};

#endif
