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

#include "dolphinmodel.h"
#include "dolphinsortfilterproxymodel.h"

#include <kformattedballoontipdelegate.h>
#include <kicon.h>
#include <ktooltip.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>
#include <QToolTip>

K_GLOBAL_STATIC(KFormattedBalloonTipDelegate, g_delegate)

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
    KToolTipItem* tip = new KToolTipItem(KIcon(m_item.iconName()), m_item.getToolTipText());

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
    const QRect desktop = QApplication::desktop()->availableGeometry();

    // m_itemRect defines the area of the item, where the tooltip should be
    // shown. Per default the tooltip is shown in the bottom right corner.
    // If the tooltip content exceeds the desktop borders, it must be assured that:
    // - the content is fully visible
    // - the content is not drawn inside m_itemRect
    int x = m_itemRect.right();
    int y = m_itemRect.bottom();
    const int xDiff = x + size.width()  - desktop.width();
    const int yDiff = y + size.height() - desktop.height();

    if ((xDiff > 0) && (yDiff > 0)) {
        x = m_itemRect.left() - size.width();
        y = m_itemRect.top() - size.height();
    } else if (xDiff > 0) {
        x -= xDiff;
    } else if (yDiff > 0) {
        y -= yDiff;
    }

    KToolTip::showTip(QPoint(x, y), tip);
}

#include "tooltipmanager.moc"
