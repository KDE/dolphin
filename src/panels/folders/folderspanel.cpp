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

#include "dolphin_folderspanelsettings.h"
#include "dolphin_generalsettings.h"
#include "treeviewcontextmenu.h"

#include <kitemviews/kitemlistselectionmanager.h>
#include <kitemviews/kfileitemlistview.h>
#include <kitemviews/kfileitemlistwidget.h>
#include <kitemviews/kitemlistcontainer.h>
#include <kitemviews/kitemlistcontroller.h>
#include <kitemviews/kfileitemmodel.h>

#include <KDirLister>
#include <KFileItem>
#include <konq_operations.h>

#include <QApplication>
#include <QBoxLayout>
#include <QGraphicsView>
#include <QTimer>

#include <views/renamedialog.h>

#include <KDebug>

FoldersPanel::FoldersPanel(QWidget* parent) :
    Panel(parent),
    m_setLeafVisible(false),
    m_mouseButtons(Qt::NoButton),
    m_dirLister(0),
    m_controller(0),
    m_leafDir()
{
    setLayoutDirection(Qt::LeftToRight);
}

FoldersPanel::~FoldersPanel()
{
    FoldersPanelSettings::self()->writeConfig();

    KItemListView* view = m_controller->view();
    m_controller->setView(0);
    delete view;

    delete m_dirLister;
    m_dirLister = 0;
}

void FoldersPanel::setHiddenFilesShown(bool show)
{
    FoldersPanelSettings::setHiddenFilesShown(show);
    if (m_dirLister) {
        KFileItemModel* model = fileItemModel();
        const QSet<KUrl> expandedUrls = model->expandedUrls();
        m_dirLister->setShowingDotFiles(show);
        m_dirLister->openUrl(m_dirLister->url(), KDirLister::Reload);
        model->setExpanded(expandedUrls);
    }
}

bool FoldersPanel::hiddenFilesShown() const
{
    return FoldersPanelSettings::hiddenFilesShown();
}

void FoldersPanel::setAutoScrolling(bool enable)
{
    //m_treeView->setAutoHorizontalScroll(enable);
    FoldersPanelSettings::setAutoScrolling(enable);
}

bool FoldersPanel::autoScrolling() const
{
    return FoldersPanelSettings::autoScrolling();
}

void FoldersPanel::rename(const KFileItem& item)
{
    // TODO: Inline renaming is not supported anymore in Dolphin 2.0
    if (false /* GeneralSettings::renameInline() */) {
        //const QModelIndex dirIndex = m_dolphinModel->indexForItem(item);
        //const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
        //m_treeView->edit(proxyIndex);
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

    if (m_dirLister) {
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

    if (!m_dirLister) {
        // Postpone the creating of the dir lister to the first show event.
        // This assures that no performance and memory overhead is given when the TreeView is not
        // used at all (see FoldersPanel::setUrl()).
        m_dirLister = new KDirLister();
        m_dirLister->setDirOnlyMode(true);
        m_dirLister->setAutoUpdate(true);
        m_dirLister->setMainWindow(window());
        m_dirLister->setDelayedMimeTypes(true);
        m_dirLister->setAutoErrorHandlingEnabled(false, this);
        m_dirLister->setShowingDotFiles(FoldersPanelSettings::hiddenFilesShown());

        KFileItemListView* view  = new KFileItemListView();
        view->setWidgetCreator(new KItemListWidgetCreator<KFileItemListWidget>());

        KItemListStyleOption styleOption = view->styleOption();
        styleOption.margin = 2;
        styleOption.iconSize = KIconLoader::SizeSmall;
        view->setStyleOption(styleOption);

        const qreal itemHeight = qMax(int(KIconLoader::SizeSmall), styleOption.fontMetrics.height());
        view->setItemSize(QSizeF(-1, itemHeight + 2 * styleOption.margin));
        view->setItemLayout(KFileItemListView::DetailsLayout);

        KFileItemModel* model = new KFileItemModel(m_dirLister, this);
        // Use a QueuedConnection to give the view the possibility to react first on the
        // finished loading.
        connect(model, SIGNAL(loadingCompleted()), this, SLOT(slotLoadingCompleted()), Qt::QueuedConnection);

        KItemListContainer* container = new KItemListContainer(this);
        m_controller = container->controller();
        m_controller->setView(view);
        m_controller->setModel(model);

        // TODO: Check whether it makes sense to make an explicit API for KItemListContainer
        // to make the background transparent.
        container->setFrameShape(QFrame::NoFrame);
        QGraphicsView* graphicsView = qobject_cast<QGraphicsView*>(container->viewport());
        if (graphicsView) {
            // Make the background of the container transparent and apply the window-text color
            // to the text color, so that enough contrast is given for all color
            // schemes
            QPalette p = graphicsView->palette();
            p.setColor(QPalette::Active,   QPalette::Text, p.color(QPalette::Active,   QPalette::WindowText));
            p.setColor(QPalette::Inactive, QPalette::Text, p.color(QPalette::Inactive, QPalette::WindowText));
            p.setColor(QPalette::Disabled, QPalette::Text, p.color(QPalette::Disabled, QPalette::WindowText));
            graphicsView->setPalette(p);
            graphicsView->viewport()->setAutoFillBackground(false);
        }

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->addWidget(container);
    }

    loadTree(url());
    Panel::showEvent(event);
}

void FoldersPanel::contextMenuEvent(QContextMenuEvent* event)
{
    Panel::contextMenuEvent(event);

    KFileItem item;
    /*const QModelIndex index = m_treeView->indexAt(event->pos());
    if (index.isValid()) {
        const QModelIndex dolphinModelIndex = m_proxyModel->mapToSource(index);
        item = m_dolphinModel->itemForIndex(dolphinModelIndex);
    }*/

    QPointer<TreeViewContextMenu> contextMenu = new TreeViewContextMenu(this, item);
    contextMenu->open();
    delete contextMenu;
}

void FoldersPanel::keyPressEvent(QKeyEvent* event)
{
    const int key = event->key();
    if ((key == Qt::Key_Enter) || (key == Qt::Key_Return)) {
        event->accept();
        //updateActiveView(m_treeView->currentIndex());
    } else {
        Panel::keyPressEvent(event);
    }
}

void FoldersPanel::updateMouseButtons()
{
    m_mouseButtons = QApplication::mouseButtons();
}

void FoldersPanel::slotLoadingCompleted()
{
    const int index = fileItemModel()->index(url());
    if (index >= 0) {
        m_controller->selectionManager()->setCurrentItem(index);
    }
}

void FoldersPanel::slotHorizontalScrollBarMoved(int value)
{
    Q_UNUSED(value);
    // Disable the auto-scrolling until the vertical scrollbar has
    // been moved by the user.
    //m_treeView->setAutoHorizontalScroll(false);
}

void FoldersPanel::slotVerticalScrollBarMoved(int value)
{
    Q_UNUSED(value);
    // Enable the auto-scrolling again (it might have been disabled by
    // moving the horizontal scrollbar).
    //m_treeView->setAutoHorizontalScroll(FoldersPanelSettings::autoScrolling());
}

void FoldersPanel::loadTree(const KUrl& url)
{
    Q_ASSERT(m_dirLister);
    m_leafDir = url;

    KUrl baseUrl;
    if (url.isLocalFile()) {
        // Use the root directory as base for local URLs (#150941)
        baseUrl = QDir::rootPath();
    } else {
        // Clear the path for non-local URLs and use it as base
        baseUrl = url;
        baseUrl.setPath(QString('/'));
    }

    if (m_dirLister->url() != baseUrl) {
        m_dirLister->stop();
        m_dirLister->openUrl(baseUrl, KDirLister::Reload);
    }

    KFileItemModel* model = fileItemModel();
    const int index = model->index(url);
    if (index >= 0) {
        m_controller->selectionManager()->setCurrentItem(index);
    } else {
        model->setExpanded(QSet<KUrl>() << url);
    }
}

void FoldersPanel::selectLeafDirectory()
{
    /*const QModelIndex dirIndex = m_dolphinModel->indexForUrl(m_leafDir);
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
    }*/
}

KFileItemModel* FoldersPanel::fileItemModel() const
{
    return static_cast<KFileItemModel*>(m_controller->model());
}

#include "folderspanel.moc"
