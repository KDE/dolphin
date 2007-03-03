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
#include "dolphinview.h"

#include "kdirlister.h"
#include "kdirmodel.h"

#include <QTreeView>
#include <QVBoxLayout>

TreeViewSidebarPage::TreeViewSidebarPage(DolphinMainWindow* mainWindow,
                                         QWidget* parent) :
    SidebarPage(mainWindow, parent),
    m_dirLister(0),
    m_dirModel(0),
    m_treeView(0)
{
    Q_ASSERT(mainWindow != 0);

    m_dirLister = new KDirLister();
    m_dirLister->setDirOnlyMode(true);
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(this);
    m_dirLister->setDelayedMimeTypes(true);
    m_dirLister->setAutoErrorHandlingEnabled(false, this);

    m_dirModel = new KDirModel();
    m_dirModel->setDirLister(m_dirLister);

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_dirModel);

    // hide all columns except of the 'Name' column
    m_treeView->hideColumn(KDirModel::Size);
    m_treeView->hideColumn(KDirModel::ModifiedTime);
    m_treeView->hideColumn(KDirModel::Permissions);
    m_treeView->hideColumn(KDirModel::Owner);
    m_treeView->hideColumn(KDirModel::Group);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_treeView);

    connectToActiveView();
}

TreeViewSidebarPage::~TreeViewSidebarPage()
{
    delete m_dirLister;
    m_dirLister = 0;
}

void TreeViewSidebarPage::activeViewChanged()
{
    connectToActiveView();
}

void TreeViewSidebarPage::updatePosition(const KUrl& url)
{
    KUrl baseUrl = BookmarkSelector::baseBookmark(url).url();
    if (m_dirLister->url() != baseUrl) {
        m_dirLister->stop();
        m_dirLister->openUrl(baseUrl);
    }

    // TODO: open sub folders to be synchronous to 'url'
}

void TreeViewSidebarPage::connectToActiveView()
{
    DolphinView* view = mainWindow()->activeView();
    m_dirLister->stop();
    m_dirLister->openUrl(view->url());
    connect(view, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(updatePosition(const KUrl&)));
}

#include "treeviewsidebarpage.moc"
