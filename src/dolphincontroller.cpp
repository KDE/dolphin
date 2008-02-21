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

#include <kdirmodel.h>
#include <QAbstractProxyModel>

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

void DolphinController::handleKeyPressEvent(QKeyEvent* event, QAbstractItemView* view)
{
    const QItemSelectionModel* selModel = view->selectionModel();
    const QModelIndex currentIndex = selModel->currentIndex();
    const bool trigger = currentIndex.isValid()
                         && (event->key() == Qt::Key_Return)
                         && (selModel->selectedIndexes().count() > 0);
    if (trigger) {
        const QModelIndexList indexList = selModel->selectedIndexes();
        foreach (const QModelIndex& index, indexList) {
            triggerItem(index, view);
        }
    }
}

KFileItem DolphinController::itemForIndex(const QModelIndex& index, QAbstractItemView* view) const
{
    QAbstractProxyModel* proxyModel = static_cast<QAbstractProxyModel*>(view->model());
    KDirModel* dirModel = static_cast<KDirModel*>(proxyModel->sourceModel());
    const QModelIndex dirIndex = proxyModel->mapToSource(index);
    return dirModel->itemForIndex(dirIndex);
}

void DolphinController::triggerItem(const QModelIndex& index, QAbstractItemView* view)
{
    const KFileItem item = itemForIndex(index, view);
    if (index.isValid() && (index.column() == KDirModel::Name)) {
        emit itemTriggered(item);
    } else {
        view->clearSelection();
        emit itemEntered(item);
    }
}

void DolphinController::emitItemEntered(const QModelIndex& index, QAbstractItemView* view)
{
    KFileItem item = itemForIndex(index, view);
    if (!item.isNull()) {
        emit itemEntered(item);
    }
}

void DolphinController::emitViewportEntered()
{
    emit viewportEntered();
}

#include "dolphincontroller.moc"
