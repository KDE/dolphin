/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "dolphinviewcontroller.h"
#include "zoomlevelinfo.h"

#include <KDirModel>
#include <KFileItemActions>
#include <QAbstractProxyModel>
#include <QApplication>
#include <QClipboard>
#include <QDir>

Qt::MouseButtons DolphinViewController::m_mouseButtons = Qt::NoButton;

DolphinViewController::DolphinViewController(DolphinView* dolphinView) :
    QObject(dolphinView),
    m_dolphinView(dolphinView),
    m_itemView(0),
    m_versionControlActions()
{
}

DolphinViewController::~DolphinViewController()
{
}

const DolphinView* DolphinViewController::view() const
{
    return m_dolphinView;
}

void DolphinViewController::requestUrlChange(const KUrl& url)
{
    emit urlChangeRequested(url);
}

void DolphinViewController::setItemView(QAbstractItemView* view)
{
    if (m_itemView != 0) {
        disconnect(m_itemView, SIGNAL(pressed(const QModelIndex&)),
                   this, SLOT(updateMouseButtonState()));
    }

    m_itemView = view;

    if (m_itemView != 0) {
        // TODO: this is a workaround until  Qt-issue 176832 has been fixed
        connect(m_itemView, SIGNAL(pressed(const QModelIndex&)),
                this, SLOT(updateMouseButtonState()));
    }
}

QAbstractItemView* DolphinViewController::itemView() const
{
    return m_itemView;
}

void DolphinViewController::triggerContextMenuRequest(const QPoint& pos,
                                                  const QList<QAction*>& customActions)
{
    emit activated();
    emit requestContextMenu(pos, customActions);
}

void DolphinViewController::requestActivation()
{
    emit activated();
}

void DolphinViewController::indicateDroppedUrls(const KFileItem& destItem, QDropEvent* event)
{
    emit urlsDropped(destItem, m_dolphinView->url(), event);
}


void DolphinViewController::indicateSortingChange(DolphinView::Sorting sorting)
{
    emit sortingChanged(sorting);
}

void DolphinViewController::indicateSortOrderChange(Qt::SortOrder order)
{
    emit sortOrderChanged(order);
}

void DolphinViewController::indicateSortFoldersFirstChange(bool foldersFirst)
{
    emit sortFoldersFirstChanged(foldersFirst);
}

void DolphinViewController::indicateAdditionalInfoChange(const KFileItemDelegate::InformationList& info)
{
    emit additionalInfoChanged(info);
}

void DolphinViewController::setVersionControlActions(QList<QAction*> actions)
{
    m_versionControlActions = actions;
}

QList<QAction*> DolphinViewController::versionControlActions(const KFileItemList& items)
{
    emit requestVersionControlActions(items);
    // All view implementations are connected with the signal requestVersionControlActions()
    // (see ViewExtensionFactory) and will invoke DolphinViewController::setVersionControlActions(),
    // so that the context dependent actions can be returned.
    return m_versionControlActions;
}

void DolphinViewController::handleKeyPressEvent(QKeyEvent* event)
{
    if (m_itemView == 0) {
        return;
    }

    const QItemSelectionModel* selModel = m_itemView->selectionModel();
    const QModelIndex currentIndex = selModel->currentIndex();
    const bool trigger = currentIndex.isValid()
                         && ((event->key() == Qt::Key_Return) || (event->key() == Qt::Key_Enter))
                         && !selModel->selectedIndexes().isEmpty();
    if (!trigger) {
        return;
    }

    // Collect the non-directory files into a list and
    // call runPreferredApplications for that list.
    // Several selected directories are opened in separate tabs,
    // one selected directory will get opened in the view.
    QModelIndexList dirQueue;
    const QModelIndexList indexList = selModel->selectedIndexes();
    KFileItemList fileOpenList;
    foreach (const QModelIndex& index, indexList) {
        if (itemForIndex(index).isDir()) {
            dirQueue << index;
        } else {
            fileOpenList << itemForIndex(index);
        }
    }

    KFileItemActions fileItemActions;
    fileItemActions.runPreferredApplications(fileOpenList, "DesktopEntryName != 'dolphin'");

    if (dirQueue.length() == 1) {
        // open directory in the view
        emit itemTriggered(itemForIndex(dirQueue[0]));
    } else {
        // open directories in separate tabs
        foreach(const QModelIndex& dir, dirQueue) {
            emit tabRequested(itemForIndex(dir).url());
        }
    }
}

void DolphinViewController::replaceUrlByClipboard()
{
    const QClipboard* clipboard = QApplication::clipboard();
    QString text;
    if (clipboard->mimeData(QClipboard::Selection)->hasText()) {
        text = clipboard->mimeData(QClipboard::Selection)->text();
    } else if (clipboard->mimeData(QClipboard::Clipboard)->hasText()) {
        text = clipboard->mimeData(QClipboard::Clipboard)->text();
    }
    if (!text.isEmpty() && QDir::isAbsolutePath(text)) {
        m_dolphinView->setUrl(KUrl(text));
    }
}

void DolphinViewController::requestToolTipHiding()
{
    emit hideToolTip();
}

void DolphinViewController::emitItemTriggered(const KFileItem& item)
{
    emit itemTriggered(item);
}

KFileItem DolphinViewController::itemForIndex(const QModelIndex& index) const
{
    if (m_itemView != 0) {
        QAbstractProxyModel* proxyModel = static_cast<QAbstractProxyModel*>(m_itemView->model());
        if (proxyModel != 0) {
            KDirModel* dirModel = static_cast<KDirModel*>(proxyModel->sourceModel());
            const QModelIndex dirIndex = proxyModel->mapToSource(index);
            return dirModel->itemForIndex(dirIndex);
        }
    }

    return KFileItem();
}

void DolphinViewController::triggerItem(const QModelIndex& index)
{
    if (m_mouseButtons & Qt::LeftButton) {
        const KFileItem item = itemForIndex(index);
        if (index.isValid() && (index.column() == KDirModel::Name)) {
            emit itemTriggered(item);
        } else if (m_itemView != 0) {
            m_itemView->clearSelection();
            emit itemEntered(KFileItem());
        }
    }
}

void DolphinViewController::requestTab(const QModelIndex& index)
{
    if (m_mouseButtons & Qt::MidButton) {
        const KFileItem item = itemForIndex(index);
        const bool validRequest = index.isValid() &&
                                  (index.column() == KDirModel::Name) &&
                                  (item.isDir() || m_dolphinView->isTabsForFilesEnabled());
        if (validRequest) {
            emit tabRequested(item.url());
        }
    }
}

void DolphinViewController::emitItemEntered(const QModelIndex& index)
{
    KFileItem item = itemForIndex(index);
    if (!item.isNull()) {
        emit itemEntered(item);
    }
}

void DolphinViewController::emitViewportEntered()
{
    emit viewportEntered();
}

void DolphinViewController::updateMouseButtonState()
{
    m_mouseButtons = QApplication::mouseButtons();
}

#include "dolphinviewcontroller.moc"
