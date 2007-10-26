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

#include <QPainter>

DolphinController::DolphinController(DolphinView* dolphinView) :
    QObject(dolphinView),
    m_showPreview(false),
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
                                            const KFileItem& destItem,
                                            QWidget* source)
{
    emit urlsDropped(urls, destPath, destItem, source);
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

void DolphinController::setShowPreview(bool show)
{
    if (m_showPreview != show) {
        m_showPreview = show;
        emit showPreviewChanged(show);
    }
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

void DolphinController::drawHoverIndication(QWidget* widget,
                                            const QRect& bounds,
                                            const QBrush& brush)
{
    QPainter painter(widget);
    painter.save();
    QBrush blendedBrush(brush);
    QColor color = blendedBrush.color();
    color.setAlpha(64);
    blendedBrush.setColor(color);

    const int radius = 10;
    QPainterPath path(QPointF(bounds.left(), bounds.top() + radius));
    path.quadTo(bounds.left(), bounds.top(), bounds.left() + radius, bounds.top());
    path.lineTo(bounds.right() - radius, bounds.top());
    path.quadTo(bounds.right(), bounds.top(), bounds.right(), bounds.top() + radius);
    path.lineTo(bounds.right(), bounds.bottom() - radius);
    path.quadTo(bounds.right(), bounds.bottom(), bounds.right() - radius, bounds.bottom());
    path.lineTo(bounds.left() + radius, bounds.bottom());
    path.quadTo(bounds.left(), bounds.bottom(), bounds.left(), bounds.bottom() - radius);
    path.closeSubpath();

    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillPath(path, blendedBrush);
    painter.restore();
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
