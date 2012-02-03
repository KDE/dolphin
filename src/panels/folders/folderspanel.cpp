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
#include <QDropEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsView>
#include <QPropertyAnimation>
#include <QTimer>

#include <views/draganddrophelper.h>
#include <views/renamedialog.h>

#include <KDebug>

FoldersPanel::FoldersPanel(QWidget* parent) :
    Panel(parent),
    m_updateCurrentItem(false),
    m_dirLister(0),
    m_controller(0)
{
    setLayoutDirection(Qt::LeftToRight);
}

FoldersPanel::~FoldersPanel()
{
    FoldersPanelSettings::self()->writeConfig();

    if (m_controller) {
        KItemListView* view = m_controller->view();
        m_controller->setView(0);
        delete view;
    }

    delete m_dirLister;
    m_dirLister = 0;
}

void FoldersPanel::setHiddenFilesShown(bool show)
{
    FoldersPanelSettings::setHiddenFilesShown(show);
    fileItemModel()->setShowHiddenFiles(show);
}

bool FoldersPanel::hiddenFilesShown() const
{
    return FoldersPanelSettings::hiddenFilesShown();
}

void FoldersPanel::setAutoScrolling(bool enable)
{
    // TODO: Not supported yet in Dolphin 2.0
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
        m_dirLister->setAutoUpdate(true);
        m_dirLister->setMainWindow(window());
        m_dirLister->setDelayedMimeTypes(true);
        m_dirLister->setAutoErrorHandlingEnabled(false, this);

        KFileItemListView* view  = new KFileItemListView();
        view->setWidgetCreator(new KItemListWidgetCreator<KFileItemListWidget>());

        KItemListStyleOption styleOption = view->styleOption();
        styleOption.margin = 2;
        styleOption.iconSize = KIconLoader::SizeSmall;
        view->setStyleOption(styleOption);

        const qreal itemHeight = qMax(int(KIconLoader::SizeSmall), styleOption.fontMetrics.height());
        view->setItemSize(QSizeF(-1, itemHeight + 2 * styleOption.margin));
        view->setItemLayout(KFileItemListView::DetailsLayout);
        // Set the opacity to 0 initially. The opacity will be increased after the loading of the initial tree
        // has been finished in slotLoadingCompleted(). This prevents an unnecessary animation-mess when
        // opening the folders panel.
        view->setOpacity(0);

        KFileItemModel* model = new KFileItemModel(m_dirLister, this);
        model->setShowFoldersOnly(true);
        model->setShowHiddenFiles(FoldersPanelSettings::hiddenFilesShown());
        // Use a QueuedConnection to give the view the possibility to react first on the
        // finished loading.
        connect(model, SIGNAL(loadingCompleted()), this, SLOT(slotLoadingCompleted()), Qt::QueuedConnection);

        KItemListContainer* container = new KItemListContainer(this);
        m_controller = container->controller();
        m_controller->setView(view);
        m_controller->setModel(model);
        m_controller->setSelectionBehavior(KItemListController::SingleSelection);
        m_controller->setAutoActivationDelay(750);
        m_controller->setSingleClickActivation(true);

        connect(m_controller, SIGNAL(itemActivated(int)), this, SLOT(slotItemActivated(int)));
        connect(m_controller, SIGNAL(itemMiddleClicked(int)), this, SLOT(slotItemMiddleClicked(int)));
        connect(m_controller, SIGNAL(itemContextMenuRequested(int,QPointF)), this, SLOT(slotItemContextMenuRequested(int,QPointF)));
        connect(m_controller, SIGNAL(viewContextMenuRequested(QPointF)), this, SLOT(slotViewContextMenuRequested(QPointF)));
        connect(m_controller, SIGNAL(itemDropEvent(int,QGraphicsSceneDragDropEvent*)), this, SLOT(slotItemDropEvent(int,QGraphicsSceneDragDropEvent*)));

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

void FoldersPanel::keyPressEvent(QKeyEvent* event)
{
    const int key = event->key();
    if ((key == Qt::Key_Enter) || (key == Qt::Key_Return)) {
        event->accept();
    } else {
        Panel::keyPressEvent(event);
    }
}

void FoldersPanel::slotItemActivated(int index)
{
    const KFileItem item = fileItemModel()->fileItem(index);
    if (!item.isNull()) {
        emit changeUrl(item.url(), Qt::LeftButton);
    }
}

void FoldersPanel::slotItemMiddleClicked(int index)
{
    const KFileItem item = fileItemModel()->fileItem(index);
    if (!item.isNull()) {
        emit changeUrl(item.url(), Qt::MiddleButton);
    }
}

void FoldersPanel::slotItemContextMenuRequested(int index, const QPointF& pos)
{
    Q_UNUSED(pos);

    const KFileItem fileItem = fileItemModel()->fileItem(index);

    QWeakPointer<TreeViewContextMenu> contextMenu = new TreeViewContextMenu(this, fileItem);
    contextMenu.data()->open();
    if (contextMenu.data()) {
        delete contextMenu.data();
    }
}

void FoldersPanel::slotViewContextMenuRequested(const QPointF& pos)
{
    Q_UNUSED(pos);

    QWeakPointer<TreeViewContextMenu> contextMenu = new TreeViewContextMenu(this, KFileItem());
    contextMenu.data()->open();
    if (contextMenu.data()) {
        delete contextMenu.data();
    }
}

void FoldersPanel::slotItemDropEvent(int index, QGraphicsSceneDragDropEvent* event)
{
    if (index >= 0) {
        KFileItemModel* model = fileItemModel();
        KFileItem destItem = model->fileItem(index);
        if (destItem.isNull()) {
            return;
        }

        QDropEvent dropEvent(event->pos().toPoint(),
                             event->possibleActions(),
                             event->mimeData(),
                             event->buttons(),
                             event->modifiers());

        DragAndDropHelper::dropUrls(destItem, destItem.url(), &dropEvent);
    }
}

void FoldersPanel::slotLoadingCompleted()
{
    if (m_controller->view()->opacity() == 0) {
        // The loading of the initial tree after opening the Folders panel
        // has been finished. Trigger the increasing of the opacity after
        // a short delay to give the view the chance to finish its internal
        // animations.
        // TODO: Check whether it makes sense to allow accessing the
        // view-internal delay for usecases like this.
        QTimer::singleShot(250, this, SLOT(startFadeInAnimation()));
    }

    if (!m_updateCurrentItem) {
        return;
    }

    const int index = fileItemModel()->index(url());
    updateCurrentItem(index);
    m_updateCurrentItem = false;
}

void FoldersPanel::startFadeInAnimation()
{
    QPropertyAnimation* anim = new QPropertyAnimation(m_controller->view(), "opacity", this);
    anim->setStartValue(0);
    anim->setEndValue(1);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
    anim->setDuration(200);
}

void FoldersPanel::loadTree(const KUrl& url)
{
    Q_ASSERT(m_dirLister);

    m_updateCurrentItem = false;

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
        m_updateCurrentItem = true;
        m_dirLister->stop();
        m_dirLister->openUrl(baseUrl, KDirLister::Reload);
    }

    KFileItemModel* model = fileItemModel();
    const int index = model->index(url);
    if (index >= 0) {
        updateCurrentItem(index);
    } else {
        m_updateCurrentItem = true;
        model->setExpanded(QSet<KUrl>() << url);
        // slotLoadingCompleted() will be invoked after the model has
        // expanded the url
    }
}

void FoldersPanel::updateCurrentItem(int index)
{
    KItemListSelectionManager* selectionManager = m_controller->selectionManager();
    selectionManager->setCurrentItem(index);
    selectionManager->clearSelection();
    selectionManager->setSelected(index);

    m_controller->view()->scrollToItem(index);
}

KFileItemModel* FoldersPanel::fileItemModel() const
{
    return static_cast<KFileItemModel*>(m_controller->model());
}

#include "folderspanel.moc"
