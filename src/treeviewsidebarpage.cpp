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

#include "dolphinmainwindow.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphinview.h"
#include "dolphinsettings.h"
#include "sidebartreeview.h"
#include "treeviewcontextmenu.h"

#include <kfileplacesmodel.h>
#include <kdirlister.h>
#include <kdirmodel.h>
#include <kfileitem.h>

#include <QItemSelection>
#include <QTreeView>
#include <QBoxLayout>
#include <QModelIndex>
#include <QTimer>

TreeViewSidebarPage::TreeViewSidebarPage(QWidget* parent) :
    SidebarPage(parent),
    m_dirLister(0),
    m_dirModel(0),
    m_proxyModel(0),
    m_treeView(0),
    m_leafDir()
{
}

TreeViewSidebarPage::~TreeViewSidebarPage()
{
    delete m_dirLister;
    m_dirLister = 0;
}

QSize TreeViewSidebarPage::sizeHint() const
{
    QSize size = SidebarPage::sizeHint();
    size.setWidth(200);
    return size;
}

void TreeViewSidebarPage::setUrl(const KUrl& url)
{
    if (!url.isValid() || (url == SidebarPage::url())) {
        return;
    }

    SidebarPage::setUrl(url);
    if (m_dirLister != 0) {
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

        Q_ASSERT(m_dirModel == 0);
        m_dirModel = new KDirModel(this);
        m_dirModel->setDirLister(m_dirLister);
        m_dirModel->setDropsAllowed(KDirModel::DropOnDirectory);
        connect(m_dirModel, SIGNAL(expand(const QModelIndex&)),
                this, SLOT(triggerExpanding(const QModelIndex&)));

        Q_ASSERT(m_proxyModel == 0);
        m_proxyModel = new DolphinSortFilterProxyModel(this);
        m_proxyModel->setSourceModel(m_dirModel);

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

    const QModelIndex dirModelIndex = m_proxyModel->mapToSource(index);
    KFileItem item = m_dirModel->itemForIndex(dirModelIndex);

    emit changeSelection(QList<KFileItem>());
    TreeViewContextMenu contextMenu(this, item);
    contextMenu.open();
}

void TreeViewSidebarPage::expandSelectionParent()
{
    disconnect(m_dirLister, SIGNAL(completed()),
               this, SLOT(expandSelectionParent()));

    // expand the parent folder of the selected item
    KUrl parentUrl = url().upUrl();
    if (!m_dirLister->url().isParentOf(parentUrl)) {
        return;
    }

    QModelIndex index = m_dirModel->indexForUrl(parentUrl);
    if (index.isValid()) {
        QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);
        m_treeView->setExpanded(proxyIndex, true);

        // select the item and assure that the item is visible
        index = m_dirModel->indexForUrl(url());
        if (index.isValid()) {
            proxyIndex = m_proxyModel->mapFromSource(index);
            m_treeView->scrollTo(proxyIndex);

            QItemSelectionModel* selModel = m_treeView->selectionModel();
            selModel->setCurrentIndex(proxyIndex, QItemSelectionModel::Select);
        }
    }
}

void TreeViewSidebarPage::updateActiveView(const QModelIndex& index)
{
    const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
    const KFileItem item = m_dirModel->itemForIndex(dirIndex);
    if (!item.isNull()) {
        emit changeUrl(item.url());
    }
}

void TreeViewSidebarPage::dropUrls(const KUrl::List& urls,
                                   const QModelIndex& index)
{
    if (index.isValid()) {
        const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
        KFileItem item = m_dirModel->itemForIndex(dirIndex);
        Q_ASSERT(!item.isNull());
        if (item.isDir()) {
            emit urlsDropped(urls, item.url());
        }
    }
}

void TreeViewSidebarPage::triggerExpanding(const QModelIndex& index)
{
    Q_UNUSED(index);
    // the expanding of the folders may not be done in the context
    // of this slot
    QTimer::singleShot(0, this, SLOT(expandToLeafDir()));
}

void TreeViewSidebarPage::expandToLeafDir()
{
    // expand all directories until the parent directory of m_leafDir
    const KUrl parentUrl = m_leafDir.upUrl();
    QModelIndex dirIndex = m_dirModel->indexForUrl(parentUrl);
    QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
    m_treeView->setExpanded(proxyIndex, true);

    // assure that m_leafDir gets selected
    dirIndex = m_dirModel->indexForUrl(m_leafDir);
    proxyIndex = m_proxyModel->mapFromSource(dirIndex);
    m_treeView->scrollTo(proxyIndex);

    QItemSelectionModel* selModel = m_treeView->selectionModel();
    selModel->setCurrentIndex(proxyIndex, QItemSelectionModel::Select);
}

void TreeViewSidebarPage::loadSubTree()
{
    disconnect(m_dirLister, SIGNAL(completed()),
               this, SLOT(loadSubTree()));

    QItemSelectionModel* selModel = m_treeView->selectionModel();
    selModel->clearSelection();

    if (m_leafDir.isParentOf(m_dirLister->url())) {
        // The leaf directory is not a child of the base URL, hence
        // no sub directory must be loaded or selected.
        return;
    }

    const QModelIndex index = m_dirModel->indexForUrl(m_leafDir);
    if (index.isValid()) {
        // the item with the given URL is already part of the model
        const QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);
        m_treeView->scrollTo(proxyIndex);
        selModel->setCurrentIndex(proxyIndex, QItemSelectionModel::Select);
    } else {
        // Load all sub directories that need to get expanded for making
        // the leaf directory visible. The slot triggerExpanding() will
        // get invoked if the expanding has been finished.
        m_dirModel->expandToUrl(m_leafDir);
    }
}

void TreeViewSidebarPage::loadTree(const KUrl& url)
{
    Q_ASSERT(m_dirLister != 0);
    m_leafDir = url;

    // adjust the root of the tree to the base place
    KFilePlacesModel* placesModel = DolphinSettings::instance().placesModel();
    KUrl baseUrl = placesModel->url(placesModel->closestItem(url));
    if (!baseUrl.isValid()) {
        // it's possible that no closest item is available and hence an
        // empty URL is returned
        baseUrl = url;
    }

    connect(m_dirLister, SIGNAL(completed()),
            this, SLOT(loadSubTree()));
    if (m_dirLister->url() != baseUrl) {
        m_dirLister->stop();
        m_dirLister->openUrl(baseUrl);
    } else {
        loadSubTree();
    }
}

#include "treeviewsidebarpage.moc"
