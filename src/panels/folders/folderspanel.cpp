/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz <peter.penz19@gmail.com>        *
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

#include "folderspanel.h"

#include "settings/dolphinsettings.h"
#include "dolphin_folderspanelsettings.h"
#include "dolphin_generalsettings.h"
#include "paneltreeview.h"
#include "treeviewcontextmenu.h"

#include <kfileplacesmodel.h>
#include <kdirlister.h>
#include <kfileitem.h>
#include <konq_operations.h>

#include <QApplication>
#include <QBoxLayout>
#include <QItemSelection>
#include <QModelIndex>
#include <QPointer>
#include <QTreeView>
#include <QScrollBar>
#include <QTimer>

#include <views/draganddrophelper.h>
#include <views/dolphinmodel.h>
#include <views/dolphinsortfilterproxymodel.h>
#include <views/dolphinview.h>
#include <views/folderexpander.h>
#include <views/renamedialog.h>

FoldersPanel::FoldersPanel(QWidget* parent) :
    Panel(parent),
    m_setLeafVisible(false),
    m_mouseButtons(Qt::NoButton),
    m_dirLister(0),
    m_dolphinModel(0),
    m_proxyModel(0),
    m_treeView(0),
    m_leafDir()
{
    setLayoutDirection(Qt::LeftToRight);
}

FoldersPanel::~FoldersPanel()
{
    FoldersPanelSettings::self()->writeConfig();

    delete m_proxyModel;
    m_proxyModel = 0;
    delete m_dolphinModel;
    m_dolphinModel = 0;
    m_dirLister = 0; // deleted by m_dolphinModel
}

QSize FoldersPanel::sizeHint() const
{
    return QSize(200, 400);
}

void FoldersPanel::setShowHiddenFiles(bool show)
{
    FoldersPanelSettings::setShowHiddenFiles(show);
    if (m_dirLister != 0) {
        m_dirLister->setShowingDotFiles(show);
        m_dirLister->openUrl(m_dirLister->url(), KDirLister::Reload);
    }
}

bool FoldersPanel::showHiddenFiles() const
{
    return FoldersPanelSettings::showHiddenFiles();
}

void FoldersPanel::setAutoScrolling(bool enable)
{
    m_treeView->setAutoHorizontalScroll(enable);
    FoldersPanelSettings::setAutoScrolling(enable);
}

bool FoldersPanel::autoScrolling() const
{
    return FoldersPanelSettings::autoScrolling();
}

void FoldersPanel::rename(const KFileItem& item)
{
    if (DolphinSettings::instance().generalSettings()->renameInline()) {
        const QModelIndex dirIndex = m_dolphinModel->indexForItem(item);
        const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
        m_treeView->edit(proxyIndex);
    } else {
        RenameDialog* dialog = new RenameDialog(this, KFileItemList() << item);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
    }
}

bool FoldersPanel::urlChanged()
{
    if (!url().isValid() || url().protocol().contains("search")) {
        // Skip results shown by a search, as possible identical
        // directory names are useless without parent-path information.
        return false;
    }

    if (m_dirLister != 0) {
        m_setLeafVisible = true;
        loadTree(url());
    }

    return true;
}

void FoldersPanel::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) {
        Panel::showEvent(event);
        return;
    }

    if (m_dirLister == 0) {
        // Postpone the creating of the dir lister to the first show event.
        // This assures that no performance and memory overhead is given when the TreeView is not
        // used at all (see FoldersPanel::setUrl()).
        m_dirLister = new KDirLister();
        m_dirLister->setDirOnlyMode(true);
        m_dirLister->setAutoUpdate(true);
        m_dirLister->setMainWindow(window());
        m_dirLister->setDelayedMimeTypes(true);
        m_dirLister->setAutoErrorHandlingEnabled(false, this);
        m_dirLister->setShowingDotFiles(FoldersPanelSettings::showHiddenFiles());
        connect(m_dirLister, SIGNAL(completed()), this, SLOT(slotDirListerCompleted()));

        Q_ASSERT(m_dolphinModel == 0);
        m_dolphinModel = new DolphinModel(this);
        m_dolphinModel->setDirLister(m_dirLister);
        m_dolphinModel->setDropsAllowed(DolphinModel::DropOnDirectory);
        connect(m_dolphinModel, SIGNAL(expand(const QModelIndex&)),
                this, SLOT(expandToDir(const QModelIndex&)));

        Q_ASSERT(m_proxyModel == 0);
        m_proxyModel = new DolphinSortFilterProxyModel(this);
        m_proxyModel->setSourceModel(m_dolphinModel);

        Q_ASSERT(m_treeView == 0);
        m_treeView = new PanelTreeView(this);
        m_treeView->setModel(m_proxyModel);
        m_proxyModel->setSorting(DolphinView::SortByName);
        m_proxyModel->setSortOrder(Qt::AscendingOrder);
        m_treeView->setAutoHorizontalScroll(FoldersPanelSettings::autoScrolling());

        new FolderExpander(m_treeView, m_proxyModel);

        connect(m_treeView, SIGNAL(clicked(const QModelIndex&)),
                this, SLOT(updateActiveView(const QModelIndex&)));
        connect(m_treeView, SIGNAL(urlsDropped(const QModelIndex&, QDropEvent*)),
                this, SLOT(dropUrls(const QModelIndex&, QDropEvent*)));
        connect(m_treeView, SIGNAL(pressed(const QModelIndex&)),
                this, SLOT(updateMouseButtons()));

        connect(m_treeView->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
                this, SLOT(slotHorizontalScrollBarMoved(int)));
        connect(m_treeView->verticalScrollBar(), SIGNAL(valueChanged(int)),
                this, SLOT(slotVerticalScrollBarMoved(int)));

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->addWidget(m_treeView);
    }

    loadTree(url());
    Panel::showEvent(event);
}

void FoldersPanel::contextMenuEvent(QContextMenuEvent* event)
{
    Panel::contextMenuEvent(event);

    KFileItem item;
    const QModelIndex index = m_treeView->indexAt(event->pos());
    if (index.isValid()) {
        const QModelIndex dolphinModelIndex = m_proxyModel->mapToSource(index);
        item = m_dolphinModel->itemForIndex(dolphinModelIndex);
    }

    QPointer<TreeViewContextMenu> contextMenu = new TreeViewContextMenu(this, item);
    contextMenu->open();
    delete contextMenu;
}

void FoldersPanel::keyPressEvent(QKeyEvent* event)
{
    const int key = event->key();
    if ((key == Qt::Key_Enter) || (key == Qt::Key_Return)) {
        event->accept();
        updateActiveView(m_treeView->currentIndex());
    } else {
        Panel::keyPressEvent(event);
    }
}

void FoldersPanel::updateActiveView(const QModelIndex& index)
{
    const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
    const KFileItem item = m_dolphinModel->itemForIndex(dirIndex);
    if (!item.isNull()) {
        emit changeUrl(item.url(), m_mouseButtons);
    }
}

void FoldersPanel::dropUrls(const QModelIndex& index, QDropEvent* event)
{
    if (index.isValid()) {
        const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
        KFileItem item = m_dolphinModel->itemForIndex(dirIndex);
        Q_ASSERT(!item.isNull());
        if (item.isDir()) {
            DragAndDropHelper::instance().dropUrls(item, item.url(), event, this);
        }
    }
}

void FoldersPanel::expandToDir(const QModelIndex& index)
{
    m_treeView->setExpanded(index, true);
    selectLeafDirectory();
}

void FoldersPanel::scrollToLeaf()
{
    const QModelIndex dirIndex = m_dolphinModel->indexForUrl(m_leafDir);
    const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
    if (proxyIndex.isValid()) {
        m_treeView->scrollTo(proxyIndex);
    }
}

void FoldersPanel::updateMouseButtons()
{
    m_mouseButtons = QApplication::mouseButtons();
}

void FoldersPanel::slotDirListerCompleted()
{
    m_treeView->resizeColumnToContents(DolphinModel::Name);
}

void FoldersPanel::slotHorizontalScrollBarMoved(int value)
{
    Q_UNUSED(value);
    // Disable the auto-scrolling until the vertical scrollbar has
    // been moved by the user.
    m_treeView->setAutoHorizontalScroll(false);
}

void FoldersPanel::slotVerticalScrollBarMoved(int value)
{
    Q_UNUSED(value);
    // Enable the auto-scrolling again (it might have been disabled by
    // moving the horizontal scrollbar).
    m_treeView->setAutoHorizontalScroll(FoldersPanelSettings::autoScrolling());
}

void FoldersPanel::loadTree(const KUrl& url)
{
    Q_ASSERT(m_dirLister != 0);
    m_leafDir = url;

    KUrl baseUrl;
    if (url.isLocalFile()) {
        // use the root directory as base for local URLs (#150941)
        baseUrl = QDir::rootPath();
    } else {
        // clear the path for non-local URLs and use it as base
        baseUrl = url;
        baseUrl.setPath(QString('/'));
    }

    if (m_dirLister->url() != baseUrl) {
        m_dirLister->stop();
        m_dirLister->openUrl(baseUrl, KDirLister::Reload);
    }
    m_dolphinModel->expandToUrl(m_leafDir);
}

void FoldersPanel::selectLeafDirectory()
{
    const QModelIndex dirIndex = m_dolphinModel->indexForUrl(m_leafDir);
    const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);

    if (proxyIndex.isValid()) {
        QItemSelectionModel* selModel = m_treeView->selectionModel();
        selModel->setCurrentIndex(proxyIndex, QItemSelectionModel::ClearAndSelect);

        if (m_setLeafVisible) {
            // Invoke scrollToLeaf() asynchronously. This assures that
            // the horizontal scrollbar is shown after resizing the column
            // (otherwise the scrollbar might hide the leaf).
            QTimer::singleShot(0, this, SLOT(scrollToLeaf()));
            m_setLeafVisible = false;
        }
    }
}

#include "folderspanel.moc"
