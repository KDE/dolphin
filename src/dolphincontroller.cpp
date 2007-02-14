/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "dolphincontroller.h"

DolphinController::DolphinController(QObject* parent) :
    QObject(parent)
{
}

DolphinController::~DolphinController()
{
}

void DolphinController::triggerContextMenuRequest(const QPoint& pos,
                                                  const QPoint& globalPos)
{
    emit activated();
    emit requestContextMenu(pos, globalPos);
}

void DolphinController::triggerActivation()
{
    emit activated();
}

void DolphinController::indicateSortingChange(DolphinView::Sorting sorting)
{
    emit sortingChanged(sorting);
}

void DolphinController::indicateSortOrderChange(Qt::SortOrder order)
{
    emit sortOrderChanged(order);
}

void DolphinController::triggerItem(const QModelIndex& index)
{
    emit itemTriggered(index);
}

void DolphinController::indicateSelectionChange()
{
    emit selectionChanged();
}

#include "dolphincontroller.moc"
