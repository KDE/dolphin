/*******************************************************************************
 *   Copyright (C) 2008 by Konstantin Heil <konst.heil@stud.uni-heidelberg.de> *
 *                                                                             *
 *   This program is free software; you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by      *
 *   the Free Software Foundation; either version 2 of the License, or         *
 *   (at your option) any later version.                                       *
 *                                                                             *
 *   This program is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *   GNU General Public License for more details.                              *
 *                                                                             *
 *   You should have received a copy of the GNU General Public License         *
 *   along with this program; if not, write to the                             *
 *   Free Software Foundation, Inc.,                                           *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA                *
 *******************************************************************************/

#include "tooltipmanager.h"

#include "dolphintooltip.h"
#include "dolphinmodel.h"
#include "dolphinsortfilterproxymodel.h"

#include <kicon.h>
#include <ktooltip.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>
#include <QToolTip>

K_GLOBAL_STATIC(DolphinBalloonTooltipDelegate, g_delegate)

ToolTipManager::ToolTipManager(QAbstractItemView* parent,
                               DolphinSortFilterProxyModel* model) :
    QObject(parent),
    m_view(parent),
    m_dolphinModel(0),
    m_proxyModel(model),
    m_timer(0),
    m_item(),
    m_itemRect()
{
    KToolTip::setToolTipDelegate(g_delegate);

    m_dolphinModel = static_cast<DolphinModel*>(m_proxyModel->sourceModel());
    connect(parent, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(requestToolTip(const QModelIndex&)));
    connect(parent, SIGNAL(viewportEntered()),
            this, SLOT(hideToolTip()));

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(showToolTip()));

    m_view->viewport()->installEventFilter(this);
}

ToolTipManager::~ToolTipManager()
{
}

void ToolTipManager::hideTip()
{
    hideToolTip();
}

bool ToolTipManager::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == m_view->viewport()) && (event->type() == QEvent::Leave)) {
        hideToolTip();
    }

    return QObject::eventFilter(watched, event);
}

void ToolTipManager::requestToolTip(const QModelIndex& index)
{
    if (index.column() == DolphinModel::Name) {
        KToolTip::hideTip();

        m_itemRect = m_view->visualRect(index);
        const QPoint pos = m_view->viewport()->mapToGlobal(m_itemRect.topLeft());
        m_itemRect.moveTo(pos);

        const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
        m_item = m_dolphinModel->itemForIndex(dirIndex);

        m_timer->start(500);
    } else {
        hideToolTip();
    }
}

void ToolTipManager::hideToolTip()
{
    m_timer->stop();
    KToolTip::hideTip();
}

void ToolTipManager::showToolTip()
{
    // TODO - create tip during requestTip(...) - this makes it more likely that the previews
    // job will have completed by the time the tooltip is shown, resulting in less flicker.
    // The memory management will be more intricate, though.
    DolphinToolTipItem *tip = new DolphinToolTipItem(m_item);

    KStyleOptionToolTip option;
    // TODO: get option content from KToolTip or add KToolTip::sizeHint() method
    option.direction      = QApplication::layoutDirection();
    option.fontMetrics    = QFontMetrics(QToolTip::font());
    option.activeCorner   = KStyleOptionToolTip::TopLeftCorner;
    option.palette        = QToolTip::palette();
    option.font           = QToolTip::font();
    option.rect           = QRect();
    option.state          = QStyle::State_None;
    option.decorationSize = QSize(32, 32);

    const QSize size = g_delegate->sizeHint(&option, tip);
    const QRect desktop = QApplication::desktop()->screenGeometry(m_itemRect.bottomRight());

    // m_itemRect defines the area of the item, where the tooltip should be
    // shown. Per default the tooltip is shown in the bottom right corner.
    // If the tooltip content exceeds the desktop borders, it must be assured that:
    // - the content is fully visible
    // - the content is not drawn inside m_itemRect
    const bool hasRoomToLeft  = (m_itemRect.left()   - size.width()  >= desktop.left());
    const bool hasRoomToRight = (m_itemRect.right()  + size.width()  <= desktop.right());
    const bool hasRoomAbove   = (m_itemRect.top()    - size.height() >= desktop.top());
    const bool hasRoomBelow   = (m_itemRect.bottom() + size.height() <= desktop.bottom());    
    if (!hasRoomAbove && !hasRoomBelow && !hasRoomToLeft && !hasRoomToRight) {
        delete tip;
        tip = 0;
        return;
    }

    int x = 0;
    if (hasRoomToLeft || hasRoomToRight) {
        x = hasRoomToRight ? m_itemRect.right() : m_itemRect.left() - size.width();
    } else {
        // Put the tooltip at the far right of the screen. The item will be overlapped
        // horizontally, but the y-coordinate will be adjusted afterwards so that no overlapping
        // occurs vertically.
        x = desktop.right() - size.width();
    }
    
    int y = 0;
    if (hasRoomBelow || hasRoomAbove) {
        y = hasRoomBelow ? m_itemRect.bottom() : m_itemRect.top() - size.height();
    } else {
        // Put the tooltip at the bottom of the screen. The x-coordinate has already
        // been adjusted, so that no overlapping with m_itemRect occurs.
        y = desktop.bottom() - size.height();
    }

    KToolTip::showTip(QPoint(x, y), tip);
}

#include "tooltipmanager.moc"
