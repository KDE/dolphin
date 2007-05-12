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
    QObject(parent),
    m_showPreview(false),
    m_showAdditionalInfo(false),
    m_zoomInPossible(false),
    m_zoomOutPossible(false)
{
}

DolphinController::~DolphinController()
{
}

void DolphinController::triggerContextMenuRequest(const QPoint& pos)
{
    emit activated();
    emit requestContextMenu(pos);
}

void DolphinController::triggerActivation()
{
    emit activated();
}

void DolphinController::indicateDroppedUrls(const KUrl::List& urls,
        const QModelIndex& index,
        QWidget* source)
{
    emit urlsDropped(urls, index, source);
}


void DolphinController::indicateSortingChange(DolphinView::Sorting sorting)
{
    emit sortingChanged(sorting);
}

void DolphinController::indicateSortOrderChange(Qt::SortOrder order)
{
    emit sortOrderChanged(order);
}

void DolphinController::setShowPreview(bool show)
{
    if (m_showPreview != show) {
        m_showPreview = show;
        emit showPreviewChanged(show);
    }
}

void DolphinController::setShowAdditionalInfo(bool show)
{
    if (m_showAdditionalInfo != show) {
        m_showAdditionalInfo = show;
        emit showAdditionalInfoChanged(show);
    }
}

void DolphinController::triggerZoomIn()
{
    emit zoomIn();
}

void DolphinController::triggerZoomOut()
{
    emit zoomOut();
}

void DolphinController::triggerItem(const QModelIndex& index)
{
    emit itemTriggered(index);
}

void DolphinController::emitItemEntered(const QModelIndex& index)
{
    emit itemEntered(index);
}

void DolphinController::emitViewportEntered()
{
    emit viewportEntered();
}

void DolphinController::indicateSelectionChange()
{
    emit selectionChanged();
}

#include "dolphincontroller.moc"
