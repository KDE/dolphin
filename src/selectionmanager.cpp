/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "selectionmanager.h"

#include "dolphinmodel.h"
#include "selectiontoggle.h"
#include <kdirmodel.h>
#include <kiconeffect.h>

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractProxyModel>
#include <QModelIndex>
#include <QPainter>
#include <QPaintEvent>
#include <QRect>

SelectionManager::SelectionManager(QAbstractItemView* parent) :
    QObject(parent),
    m_view(parent),
    m_button(0),
    m_item()
{
    connect(parent, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(slotEntered(const QModelIndex&)));
    connect(parent, SIGNAL(viewportEntered()),
            this, SLOT(slotViewportEntered()));
    m_button = new SelectionToggle(m_view->viewport());
    m_button->setCheckable(true);
    m_button->hide();
    connect(m_button, SIGNAL(clicked(bool)),
            this, SLOT(setItemSelected(bool)));
}

SelectionManager::~SelectionManager()
{
}

void SelectionManager::reset()
{
    m_button->hide();
    m_item = KFileItem();
}

void SelectionManager::slotEntered(const QModelIndex& index)
{
    m_button->hide();
    if (index.isValid() && (index.column() == DolphinModel::Name)) {
        m_item = itemForIndex(index);

        const QRect rect = m_view->visualRect(index);
        const int gap = 2;
        const int x = rect.right() - m_button->width() - gap;
        int y = rect.top();
        if (rect.height() <= m_button->height() * 2) {
            // center the button vertically
            y += (rect.height() - m_button->height()) / 2;
        } else {
            y += gap;
        }

        m_button->move(QPoint(x, y));

        QItemSelectionModel* selModel = m_view->selectionModel();
        m_button->setChecked(selModel->isSelected(index));
        m_button->show();
    } else {
        m_item = KFileItem();
    }
}

void SelectionManager::slotViewportEntered()
{
    m_button->hide();
    m_item = KFileItem();
}

void SelectionManager::setItemSelected(bool selected)
{
    emit selectionChanged();
    Q_ASSERT(!m_item.isNull());

    const QModelIndex index = indexForItem(m_item);
    if (index.isValid()) {
        QItemSelectionModel* selModel = m_view->selectionModel();
        if (selected) {
            selModel->select(index, QItemSelectionModel::Select);
        } else {
            selModel->select(index, QItemSelectionModel::Deselect);
        }
    }
}

KFileItem SelectionManager::itemForIndex(const QModelIndex& index) const
{
    QAbstractProxyModel* proxyModel = static_cast<QAbstractProxyModel*>(m_view->model());
    KDirModel* dirModel = static_cast<KDirModel*>(proxyModel->sourceModel());
    const QModelIndex dirIndex = proxyModel->mapToSource(index);
    return dirModel->itemForIndex(dirIndex);
}

const QModelIndex SelectionManager::indexForItem(const KFileItem& item) const
{
    QAbstractProxyModel* proxyModel = static_cast<QAbstractProxyModel*>(m_view->model());
    KDirModel* dirModel = static_cast<KDirModel*>(proxyModel->sourceModel());
    const QModelIndex dirIndex = dirModel->indexForItem(item);
    return proxyModel->mapFromSource(dirIndex);
}

#include "selectionmanager.moc"
