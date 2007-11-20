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

DolphinController::DolphinController(DolphinView* dolphinView) :
    QObject(dolphinView),
    m_zoomInPossible(false),
    m_zoomOutPossible(false),
    m_url(),
    m_dolphinView(dolphinView)
{
}

DolphinController::~DolphinController()
{
}

void DolphinController::setUrl(const KUrl& url)
{
    if (m_url != url) {
        m_url = url;
        emit urlChanged(url);
    }
}

void DolphinController::triggerUrlChangeRequest(const KUrl& url)
{
    if (m_url != url) {
        emit requestUrlChange(url);
    }
}

void DolphinController::triggerContextMenuRequest(const QPoint& pos)
{
    emit activated();
    emit requestContextMenu(pos);
}

void DolphinController::requestActivation()
{
    emit activated();
}

void DolphinController::indicateDroppedUrls(const KUrl::List& urls,
                                            const KUrl& destPath,
                                            const KFileItem& destItem)
{
    emit urlsDropped(urls, destPath, destItem);
}


void DolphinController::indicateSortingChange(DolphinView::Sorting sorting)
{
    emit sortingChanged(sorting);
}

void DolphinController::indicateSortOrderChange(Qt::SortOrder order)
{
    emit sortOrderChanged(order);
}

void DolphinController::indicateAdditionalInfoChange(const KFileItemDelegate::InformationList& info)
{
    emit additionalInfoChanged(info);
}

void DolphinController::indicateActivationChange(bool active)
{
    emit activationChanged(active);
}

void DolphinController::triggerZoomIn()
{
    emit zoomIn();
}

void DolphinController::triggerZoomOut()
{
    emit zoomOut();
}

void DolphinController::triggerItem(const KFileItem& item)
{
    emit itemTriggered(item);
}

void DolphinController::emitItemEntered(const KFileItem& item)
{
    emit itemEntered(item);
}

void DolphinController::emitViewportEntered()
{
    emit viewportEntered();
}

#include "dolphincontroller.moc"
