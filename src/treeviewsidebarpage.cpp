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

#include "bookmarkselector.h"
#include "dolphinmainwindow.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphinview.h"
#include "sidebartreeview.h"
#include "treeviewcontextmenu.h"

#include <kdirlister.h>
#include <kdirmodel.h>
#include <kfileitem.h>

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QTreeView>
#include <QVBoxLayout>
#include "dolphinsettings.h"

// TODO: currently when using a proxy model the strange effect occurs
// that items get duplicated. Activate the following define to have the proxy
// model:
//#define USE_PROXY_MODEL

TreeViewSidebarPage::TreeViewSidebarPage(QWidget* parent) :
    SidebarPage(parent),
    m_dirLister(0),
    m_dirModel(0),
    m_proxyModel(0),
    m_treeView(0)
{
    m_dirLister = new KDirLister();
    m_dirLister->setDirOnlyMode(true);
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(this);
    m_dirLister->setDelayedMimeTypes(true);
    m_dirLister->setAutoErrorHandlingEnabled(false, this);

    m_dirModel = new KDirModel();
    m_dirModel->setDirLister(m_dirLister);
    m_dirModel->setDropsAllowed(KDirModel::DropOnDirectory);


#if defined(USE_PROXY_MODEL)
    m_proxyModel = new DolphinSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_dirModel);

    m_treeView = new SidebarTreeView(this);
    m_treeView->setModel(m_proxyModel);

    m_proxyModel->setSorting(DolphinView::SortByName);
    m_proxyModel->setSortOrder(Qt::Ascending);
#else
    m_treeView = new SidebarTreeView(this);
    m_treeView->setModel(m_dirModel);
#endif

    connect(m_treeView, SIGNAL(clicked(const QModelIndex&)),
            this, SLOT(updateActiveView(const QModelIndex&)));
    connect(m_treeView, SIGNAL(urlsDropped(const KUrl::List&, const QModelIndex&)),
            this, SLOT(dropUrls(const KUrl::List&, const QModelIndex&)));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_treeView);
}

TreeViewSidebarPage::~TreeViewSidebarPage()
{
    delete m_dirLister;
    m_dirLister = 0;
}

void TreeViewSidebarPage::setUrl(const KUrl& url)
{
    if (!url.isValid() || (url == m_url)) {
        return;
    }

    m_url = url;

    // adjust the root of the tree to the base bookmark
    const KUrl baseUrl = BookmarkSelector::baseBookmark(DolphinSettings::instance().bookmarkManager(), url).url();
    if (m_dirLister->url() != baseUrl) {
        m_dirLister->stop();
        m_dirLister->openUrl(baseUrl);
    }

    // select the folder which contains the given URL
    QItemSelectionModel* selModel = m_treeView->selectionModel();
    selModel->clearSelection();

    const QModelIndex index = m_dirModel->indexForUrl(url);
    if (index.isValid()) {
#if defined(USE_PROXY_MODEL)
        // the item with the given URL is already part of the model
        const QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);
        m_treeView->scrollTo(proxyIndex);
        selModel->setCurrentIndex(proxyIndex, QItemSelectionModel::Select);
#else
        m_treeView->scrollTo(index);
        selModel->setCurrentIndex(index, QItemSelectionModel::Select);
#endif
    }
    else {
        // The item with the given URL is not loaded by the model yet. Iterate
        // backward to the base URL and trigger the loading of the items for
        // each hierarchy level.
        connect(m_dirLister, SIGNAL(completed()),
                this, SLOT(expandSelectionParent()));

        KUrl parentUrl = url.upUrl();
        while (!parentUrl.isParentOf(baseUrl)) {
            m_dirLister->openUrl(parentUrl, true, false);
            parentUrl = parentUrl.upUrl();
        }
    }

}

void TreeViewSidebarPage::showEvent(QShowEvent* event)
{
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

#if defined(USE_PROXY_MODEL)
    const QModelIndex dirModelIndex = m_proxyModel->mapToSource(index);
    KFileItem* item = m_dirModel->itemForIndex(dirModelIndex);
#else
    KFileItem* item = m_dirModel->itemForIndex(index);
#endif

    emit changeSelection(KFileItemList());
    TreeViewContextMenu contextMenu(this, item);
    contextMenu.open();
}

void TreeViewSidebarPage::expandSelectionParent()
{
    disconnect(m_dirLister, SIGNAL(completed()),
               this, SLOT(expandSelectionParent()));

    // expand the parent folder of the selected item
    KUrl parentUrl = m_url.upUrl();
    if (!m_dirLister->url().isParentOf(parentUrl)) {
        return;
    }

    QModelIndex index = m_dirModel->indexForUrl(parentUrl);
    if (index.isValid()) {
#if defined(USE_PROXY_MODEL)
        QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);
        m_treeView->setExpanded(proxyIndex, true);
#else
        m_treeView->setExpanded(index, true);
#endif

        // select the item and assure that the item is visible
        index = m_dirModel->indexForUrl(m_url);
        if (index.isValid()) {
#if defined(USE_PROXY_MODEL)
            proxyIndex = m_proxyModel->mapFromSource(index);
            m_treeView->scrollTo(proxyIndex);

            QItemSelectionModel* selModel = m_treeView->selectionModel();
            selModel->setCurrentIndex(proxyIndex, QItemSelectionModel::Select);
#else
            m_treeView->scrollTo(index);

            QItemSelectionModel* selModel = m_treeView->selectionModel();
            selModel->setCurrentIndex(index, QItemSelectionModel::Select);
#endif
        }
    }
}

void TreeViewSidebarPage::updateActiveView(const QModelIndex& index)
{
#if defined(USE_PROXY_MODEL)
    const QModelIndex& dirIndex = m_proxyModel->mapToSource(index);
    const KFileItem* item = m_dirModel->itemForIndex(dirIndex);
#else
    const KFileItem* item = m_dirModel->itemForIndex(index);
#endif
    if (item != 0) {
        const KUrl& url = item->url();
        emit changeUrl(url);
    }
}

void TreeViewSidebarPage::dropUrls(const KUrl::List& urls,
                                   const QModelIndex& index)
{
    if (index.isValid()) {
#if defined(USE_PROXY_MODEL)
        const QModelIndex& dirIndex = m_proxyModel->mapToSource(index);
        KFileItem* item = m_dirModel->itemForIndex(dirIndex);
#else
        KFileItem* item = m_dirModel->itemForIndex(index);
#endif
        Q_ASSERT(item != 0);
        if (item->isDir()) {
            emit urlsDropped(urls, item->url());
        }
    }
}

#include "treeviewsidebarpage.moc"
