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

#include <QTimer>

K_GLOBAL_STATIC(KFormattedBalloonTipDelegate, g_delegate);

ToolTipManager::ToolTipManager(QAbstractItemView* parent,
                               DolphinSortFilterProxyModel* model) :
    QObject(parent),
    m_view(parent),
    m_dolphinModel(0),
    m_proxyModel(model),
    m_timer(0),
    m_item(),
    m_pos()
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
    KToolTip::hideTip();

    const QRect rect = m_view->visualRect(index);
    m_pos = m_view->viewport()->mapToGlobal(rect.bottomRight());

    const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
    m_item = m_dolphinModel->itemForIndex(dirIndex);

    m_timer->start(500);
}

void ToolTipManager::hideToolTip()
{
    m_timer->stop();
    KToolTip::hideTip();
}

void ToolTipManager::showToolTip()
{
    KToolTipItem* tip = new KToolTipItem(KIcon(m_item.iconName()), m_item.getToolTipText());
    KToolTip::showTip(m_pos, tip);
}

#include "tooltipmanager.moc"
