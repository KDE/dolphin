/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "treeviewsidebarpage.h"

#include "dolphinmodel.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphinview.h"
#include "dolphinsettings.h"
#include "sidebartreeview.h"
#include "treeviewcontextmenu.h"

#include <kfileplacesmodel.h>
#include <kdirlister.h>
#include <kfileitem.h>

#include <QItemSelection>
#include <QTreeView>
#include <QBoxLayout>
#include <QModelIndex>
#include <QScrollBar>
#include <QTimer>

TreeViewSidebarPage::TreeViewSidebarPage(QWidget* parent) :
    SidebarPage(parent),
    m_setLeafVisible(false),
    m_horizontalPos(0),
    m_dirLister(0),
    m_dolphinModel(0),
    m_proxyModel(0),
    m_treeView(0),
    m_leafDir()
{
}

TreeViewSidebarPage::~TreeViewSidebarPage()
{
    delete m_proxyModel;
    m_proxyModel = 0;
    delete m_dolphinModel;
    m_dolphinModel = 0;
    m_dirLister = 0; // deleted by m_dolphinModel
}

QSize TreeViewSidebarPage::sizeHint() const
{
    return QSize(200, 400);
}

void TreeViewSidebarPage::setUrl(const KUrl& url)
{
    if (!url.isValid() || (url == SidebarPage::url())) {
        return;
    }

    SidebarPage::setUrl(url);
    if (m_dirLister != 0) {
        m_setLeafVisible = true;
        loadTree(url);
    }
}

void TreeViewSidebarPage::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) {
        SidebarPage::showEvent(event);
        return;
    }

    if (m_dirLister == 0) {
        // Postpone the creating of the dir lister to the first show event.
        // This assures that no performance and memory overhead is given when the TreeView is not
        // used at all (see TreeViewSidebarPage::setUrl()).
        m_dirLister = new KDirLister();
        m_dirLister->setDirOnlyMode(true);
        m_dirLister->setAutoUpdate(true);
        m_dirLister->setMainWindow(this);
        m_dirLister->setDelayedMimeTypes(true);
        m_dirLister->setAutoErrorHandlingEnabled(false, this);

        connect(m_dirLister, SIGNAL(completed()),
                this, SLOT(triggerLoadSubTree()));

        Q_ASSERT(m_dolphinModel == 0);
        m_dolphinModel = new DolphinModel(this);
        m_dolphinModel->setDirLister(m_dirLister);
        m_dolphinModel->setDropsAllowed(DolphinModel::DropOnDirectory);
        connect(m_dolphinModel, SIGNAL(expand(const QModelIndex&)),
                this, SLOT(triggerExpanding()));

        Q_ASSERT(m_proxyModel == 0);
        m_proxyModel = new DolphinSortFilterProxyModel(this);
        m_proxyModel->setSourceModel(m_dolphinModel);

        Q_ASSERT(m_treeView == 0);
        m_treeView = new SidebarTreeView(this);
        m_treeView->setModel(m_proxyModel);
        m_proxyModel->setSorting(DolphinView::SortByName);
        m_proxyModel->setSortOrder(Qt::AscendingOrder);

        connect(m_treeView, SIGNAL(clicked(const QModelIndex&)),
                this, SLOT(updateActiveView(const QModelIndex&)));
        connect(m_treeView, SIGNAL(urlsDropped(const KUrl::List&, const QModelIndex&)),
                this, SLOT(dropUrls(const KUrl::List&, const QModelIndex&)));

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->addWidget(m_treeView);
    }

    loadTree(url());
    SidebarPage::showEvent(event);
}

void TreeViewSidebarPage::contextMenuEvent(QContextMenuEvent* event)
{
    SidebarPage::contextMenuEvent(event);

    const QModelIndex index = m_treeView->indexAt(event->pos());
    if (!index.isValid()) {
        // only open a context menu above a directory item
        return;
    }

    const QModelIndex dolphinModelIndex = m_proxyModel->mapToSource(index);
    KFileItem item = m_dolphinModel->itemForIndex(dolphinModelIndex);

    emit changeSelection(KFileItemList());
    TreeViewContextMenu contextMenu(this, item);
    contextMenu.open();
}

void TreeViewSidebarPage::updateActiveView(const QModelIndex& index)
{
    const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
    const KFileItem item = m_dolphinModel->itemForIndex(dirIndex);
    if (!item.isNull()) {
        emit changeUrl(item.url());
    }
}

void TreeViewSidebarPage::dropUrls(const KUrl::List& urls,
                                   const QModelIndex& index)
{
    if (index.isValid()) {
        const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
        KFileItem item = m_dolphinModel->itemForIndex(dirIndex);
        Q_ASSERT(!item.isNull());
        if (item.isDir()) {
            emit urlsDropped(urls, item.url());
        }
    }
}

void TreeViewSidebarPage::triggerExpanding()
{
    // the expanding of the folders may not be done in the context
    // of this slot
    QMetaObject::invokeMethod(this, "expandToLeafDir", Qt::QueuedConnection);
}

void TreeViewSidebarPage::triggerLoadSubTree()
{
    // the loading of the sub tree may not be done in the context
    // of this slot
    QMetaObject::invokeMethod(this, "loadSubTree", Qt::QueuedConnection);
}

void TreeViewSidebarPage::expandToLeafDir()
{
    // expand all directories until the parent directory of m_leafDir
    const KUrl parentUrl = m_leafDir.upUrl();
    QModelIndex dirIndex = m_dolphinModel->indexForUrl(parentUrl);
    QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
    m_treeView->setExpanded(proxyIndex, true);
    selectLeafDirectory();
}


void TreeViewSidebarPage::loadSubTree()
{
    m_treeView->selectionModel()->clearSelection();

    if (m_leafDir.isParentOf(m_dirLister->url())) {
        // The leaf directory is not a child of the base URL, hence
        // no sub directory must be loaded or selected.
        return;
    }

    const QModelIndex index = m_dolphinModel->indexForUrl(m_leafDir);
    if (index.isValid()) {
        selectLeafDirectory();
    } else {
        // Load all sub directories that need to get expanded for making
        // the leaf directory visible. The slot triggerExpanding() will
        // get invoked if the expanding has been finished.
        m_dolphinModel->expandToUrl(m_leafDir);
    }
}

void TreeViewSidebarPage::scrollToLeaf()
{
    const QModelIndex dirIndex = m_dolphinModel->indexForUrl(m_leafDir);
    const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
    if (proxyIndex.isValid()) {
        m_treeView->scrollTo(proxyIndex);
    }
}

void TreeViewSidebarPage::loadTree(const KUrl& url)
{
    Q_ASSERT(m_dirLister != 0);
    m_leafDir = url;

    m_horizontalPos = m_treeView->horizontalScrollBar()->value();

    KUrl baseUrl = url;
    if (url.isLocalFile()) {
        // use the root directory as base for local URLs
        baseUrl = KUrl("file:///");
    } else {
        // clear the path for non-local URLs and use it as base
        baseUrl = url;
        baseUrl.setPath(QString());
    }

    if (m_dirLister->url() != baseUrl) {
        m_dirLister->stop();
        m_dirLister->openUrl(baseUrl, KDirLister::Reload);
    } else {
        loadSubTree();
    }
}

void TreeViewSidebarPage::selectLeafDirectory()
{
    const QModelIndex dirIndex = m_dolphinModel->indexForUrl(m_leafDir);
    const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
    if (!proxyIndex.isValid()) {
        return;
    }

    if (m_setLeafVisible) {
        // Invoke m_treeView->scrollTo(proxyIndex) asynchronously by
        // scrollToLeaf(). This assures that the scrolling is done after
        // the horizontal scrollbar gets visible (otherwise the scrollbar
        // might hide the leaf).
        QTimer::singleShot(100, this, SLOT(scrollToLeaf()));
        m_setLeafVisible = false;
    }

    QItemSelectionModel* selModel = m_treeView->selectionModel();
    selModel->setCurrentIndex(proxyIndex, QItemSelectionModel::Select);

    m_treeView->horizontalScrollBar()->setValue(m_horizontalPos);
}

#include "treeviewsidebarpage.moc"
