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
#include "foldersitemlistwidget.h"

#include <views/renamedialog.h>
#include <kitemviews/kitemlistselectionmanager.h>
#include <kitemviews/kfileitemlistview.h>
#include <kitemviews/kfileitemlistwidget.h>
#include <kitemviews/kitemlistcontainer.h>
#include <kitemviews/kitemlistcontroller.h>
#include <kitemviews/kfileitemmodel.h>

#include <KFileItem>
#include <KJobWidgets>
#include <KJobUiDelegate>
#include <KIO/CopyJob>
#include <KIO/DropJob>
#include <KIO/FileUndoManager>

#include <QApplication>
#include <QBoxLayout>
#include <QDropEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsView>
#include <QPropertyAnimation>
#include <QTimer>

#include <views/draganddrophelper.h>

#include "dolphindebug.h"
#include "global.h"

FoldersPanel::FoldersPanel(QWidget* parent) :
    Panel(parent),
    m_updateCurrentItem(false),
    m_controller(nullptr),
    m_model(nullptr)
{
    setLayoutDirection(Qt::LeftToRight);
}

FoldersPanel::~FoldersPanel()
{
    FoldersPanelSettings::self()->save();

    if (m_controller) {
        KItemListView* view = m_controller->view();
        m_controller->setView(nullptr);
        delete view;
    }
}

void FoldersPanel::setShowHiddenFiles(bool show)
{
    FoldersPanelSettings::setHiddenFilesShown(show);
    m_model->setShowHiddenFiles(show);
}

bool FoldersPanel::showHiddenFiles() const
{
    return FoldersPanelSettings::hiddenFilesShown();
}

void FoldersPanel::setLimitFoldersPanelToHome(bool enable)
{
    FoldersPanelSettings::setLimitFoldersPanelToHome(enable);
    reloadTree();
}

bool FoldersPanel::limitFoldersPanelToHome() const
{
    return FoldersPanelSettings::limitFoldersPanelToHome();
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
    if (GeneralSettings::renameInline()) {
        const int index = m_model->index(item);
        m_controller->view()->editRole(index, "text");
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
    if (!url().isValid() || url().scheme().contains(QStringLiteral("search"))) {
        // Skip results shown by a search, as possible identical
        // directory names are useless without parent-path information.
        return false;
    }

    if (m_controller) {
        loadTree(url());
    }

    return true;
}

void FoldersPanel::reloadTree()
{
    if (m_controller) {
        loadTree(url(), true);
    }
}


void FoldersPanel::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) {
        Panel::showEvent(event);
        return;
    }

    if (!m_controller) {
        // Postpone the creating of the controller to the first show event.
        // This assures that no performance and memory overhead is given when the folders panel is not
        // used at all and stays invisible.
        KFileItemListView* view  = new KFileItemListView();
        view->setWidgetCreator(new KItemListWidgetCreator<FoldersItemListWidget>());
        view->setSupportsItemExpanding(true);
        // Set the opacity to 0 initially. The opacity will be increased after the loading of the initial tree
        // has been finished in slotLoadingCompleted(). This prevents an unnecessary animation-mess when
        // opening the folders panel.
        view->setOpacity(0);

        connect(view, &KFileItemListView::roleEditingFinished,
                this, &FoldersPanel::slotRoleEditingFinished);

        m_model = new KFileItemModel(this);
        m_model->setShowDirectoriesOnly(true);
        m_model->setShowHiddenFiles(FoldersPanelSettings::hiddenFilesShown());
        // Use a QueuedConnection to give the view the possibility to react first on the
        // finished loading.
        connect(m_model, &KFileItemModel::directoryLoadingCompleted, this, &FoldersPanel::slotLoadingCompleted, Qt::QueuedConnection);

        m_controller = new KItemListController(m_model, view, this);
        m_controller->setSelectionBehavior(KItemListController::SingleSelection);
        m_controller->setAutoActivationBehavior(KItemListController::ExpansionOnly);
        m_controller->setMouseDoubleClickAction(KItemListController::ActivateAndExpandItem);
        m_controller->setAutoActivationDelay(750);
        m_controller->setSingleClickActivationEnforced(true);

        connect(m_controller, &KItemListController::itemActivated, this, &FoldersPanel::slotItemActivated);
        connect(m_controller, &KItemListController::itemMiddleClicked, this, &FoldersPanel::slotItemMiddleClicked);
        connect(m_controller, &KItemListController::itemContextMenuRequested, this, &FoldersPanel::slotItemContextMenuRequested);
        connect(m_controller, &KItemListController::viewContextMenuRequested, this, &FoldersPanel::slotViewContextMenuRequested);
        connect(m_controller, &KItemListController::itemDropEvent, this, &FoldersPanel::slotItemDropEvent);

        KItemListContainer* container = new KItemListContainer(m_controller, this);
        container->setEnabledFrame(false);

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
    const KFileItem item = m_model->fileItem(index);
    if (!item.isNull()) {
        emit folderActivated(item.url());
    }
}

void FoldersPanel::slotItemMiddleClicked(int index)
{
    const KFileItem item = m_model->fileItem(index);
    if (!item.isNull()) {
        emit folderMiddleClicked(item.url());
    }
}

void FoldersPanel::slotItemContextMenuRequested(int index, const QPointF& pos)
{
    Q_UNUSED(pos);

    const KFileItem fileItem = m_model->fileItem(index);

    QPointer<TreeViewContextMenu> contextMenu = new TreeViewContextMenu(this, fileItem);
    contextMenu.data()->open();
    if (contextMenu.data()) {
        delete contextMenu.data();
    }
}

void FoldersPanel::slotViewContextMenuRequested(const QPointF& pos)
{
    Q_UNUSED(pos);

    QPointer<TreeViewContextMenu> contextMenu = new TreeViewContextMenu(this, KFileItem());
    contextMenu.data()->open();
    if (contextMenu.data()) {
        delete contextMenu.data();
    }
}

void FoldersPanel::slotItemDropEvent(int index, QGraphicsSceneDragDropEvent* event)
{
    if (index >= 0) {
        KFileItem destItem = m_model->fileItem(index);
        if (destItem.isNull()) {
            return;
        }

        QDropEvent dropEvent(event->pos().toPoint(),
                             event->possibleActions(),
                             event->mimeData(),
                             event->buttons(),
                             event->modifiers());

        KIO::DropJob *job = DragAndDropHelper::dropUrls(destItem.mostLocalUrl(), &dropEvent, this);
        if (job) {
            connect(job, &KIO::DropJob::result, this, [this](KJob *job) { if (job->error()) emit errorMessage(job->errorString()); });
        }
    }
}

void FoldersPanel::slotRoleEditingFinished(int index, const QByteArray& role, const QVariant& value)
{
    if (role == "text") {
        const KFileItem item = m_model->fileItem(index);
        const QString newName = value.toString();
        if (!newName.isEmpty() && newName != item.text() && newName != QLatin1String(".") && newName != QLatin1String("..")) {
            const QUrl oldUrl = item.url();
            QUrl newUrl = oldUrl.adjusted(QUrl::RemoveFilename);
            newUrl.setPath(newUrl.path() + KIO::encodeFileName(newName));

            KIO::Job* job = KIO::moveAs(oldUrl, newUrl);
            KJobWidgets::setWindow(job, this);
            KIO::FileUndoManager::self()->recordJob(KIO::FileUndoManager::Rename, {oldUrl}, newUrl, job);
            job->uiDelegate()->setAutoErrorHandlingEnabled(true);
        }
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
        QTimer::singleShot(250, this, &FoldersPanel::startFadeInAnimation);
    }

    if (!m_updateCurrentItem) {
        return;
    }

    const int index = m_model->index(url());
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

void FoldersPanel::loadTree(const QUrl& url, bool allowJumpHome)
{
    Q_ASSERT(m_controller);

    m_updateCurrentItem = false;
    bool jumpHome = false;

    QUrl baseUrl;
    if (!url.isLocalFile()) {
        // Clear the path for non-local URLs and use it as base
        baseUrl = url;
        baseUrl.setPath(QString('/'));
    } else if (Dolphin::homeUrl().isParentOf(url) || (Dolphin::homeUrl() == url)) {
        if (FoldersPanelSettings::limitFoldersPanelToHome() ) {
            baseUrl = Dolphin::homeUrl();
        } else {
            // Use the root directory as base for local URLs (#150941)
            baseUrl = QUrl::fromLocalFile(QDir::rootPath());
        }
    } else if (FoldersPanelSettings::limitFoldersPanelToHome() && allowJumpHome) {
        baseUrl = Dolphin::homeUrl();
        jumpHome = true;
    } else {
        // Use the root directory as base for local URLs (#150941)
        baseUrl = QUrl::fromLocalFile(QDir::rootPath());
    }

    if (m_model->directory() != baseUrl && !jumpHome) {
        m_updateCurrentItem = true;
        m_model->refreshDirectory(baseUrl);
    }

    const int index = m_model->index(url);
    if (jumpHome == true) {
      emit folderActivated(baseUrl);
    } else if (index >= 0) {
        updateCurrentItem(index);
    } else if (url == baseUrl) {
        // clear the selection when visiting the base url
        updateCurrentItem(-1);
    } else {
        m_updateCurrentItem = true;
        m_model->expandParentDirectories(url);

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

