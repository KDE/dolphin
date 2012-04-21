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

#include <KDebug>

FoldersPanel::FoldersPanel(QWidget* parent) :
    Panel(parent),
    m_updateCurrentItem(false),
    m_controller(0),
    m_model(0)
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
    const int index = m_model->index(item);
    m_controller->view()->editRole(index, "text");
}

bool FoldersPanel::urlChanged()
{
    if (!url().isValid() || url().protocol().contains("search")) {
        // Skip results shown by a search, as possible identical
        // directory names are useless without parent-path information.
        return false;
    }

    if (m_controller) {
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

    if (!m_controller) {
        // Postpone the creating of the controller to the first show event.
        // This assures that no performance and memory overhead is given when the folders panel is not
        // used at all and stays invisible.
        KFileItemListView* view  = new KFileItemListView();
        view->setSupportsItemExpanding(true);
        // Set the opacity to 0 initially. The opacity will be increased after the loading of the initial tree
        // has been finished in slotLoadingCompleted(). This prevents an unnecessary animation-mess when
        // opening the folders panel.
        view->setOpacity(0);

        connect(view, SIGNAL(roleEditingFinished(int,QByteArray,QVariant)),
                this, SLOT(slotRoleEditingFinished(int,QByteArray,QVariant)));

        m_model = new KFileItemModel(this);
        m_model->setShowDirectoriesOnly(true);
        m_model->setShowHiddenFiles(FoldersPanelSettings::hiddenFilesShown());
        // Use a QueuedConnection to give the view the possibility to react first on the
        // finished loading.
        connect(m_model, SIGNAL(directoryLoadingCompleted()), this, SLOT(slotLoadingCompleted()), Qt::QueuedConnection);

        m_controller = new KItemListController(m_model, view, this);
        m_controller->setSelectionBehavior(KItemListController::SingleSelection);
        m_controller->setAutoActivationDelay(750);
        m_controller->setSingleClickActivation(true);

        connect(m_controller, SIGNAL(itemActivated(int)), this, SLOT(slotItemActivated(int)));
        connect(m_controller, SIGNAL(itemMiddleClicked(int)), this, SLOT(slotItemMiddleClicked(int)));
        connect(m_controller, SIGNAL(itemContextMenuRequested(int,QPointF)), this, SLOT(slotItemContextMenuRequested(int,QPointF)));
        connect(m_controller, SIGNAL(viewContextMenuRequested(QPointF)), this, SLOT(slotViewContextMenuRequested(QPointF)));
        connect(m_controller, SIGNAL(itemDropEvent(int,QGraphicsSceneDragDropEvent*)), this, SLOT(slotItemDropEvent(int,QGraphicsSceneDragDropEvent*)));

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
        emit changeUrl(item.url(), Qt::LeftButton);
    }
}

void FoldersPanel::slotItemMiddleClicked(int index)
{
    const KFileItem item = m_model->fileItem(index);
    if (!item.isNull()) {
        emit changeUrl(item.url(), Qt::MiddleButton);
    }
}

void FoldersPanel::slotItemContextMenuRequested(int index, const QPointF& pos)
{
    Q_UNUSED(pos);

    const KFileItem fileItem = m_model->fileItem(index);

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
        KFileItem destItem = m_model->fileItem(index);
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

void FoldersPanel::slotRoleEditingFinished(int index, const QByteArray& role, const QVariant& value)
{
    if (role == "text") {
        const KFileItem item = m_model->fileItem(index);
        const QString newName = value.toString();
        if (!newName.isEmpty() && newName != item.text() && newName != QLatin1String(".") && newName != QLatin1String("..")) {
            KonqOperations::rename(this, item.url(), newName);
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
        QTimer::singleShot(250, this, SLOT(startFadeInAnimation()));
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

void FoldersPanel::loadTree(const KUrl& url)
{
    Q_ASSERT(m_controller);

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

    if (m_model->directory() != baseUrl) {
        m_updateCurrentItem = true;
        m_model->refreshDirectory(baseUrl);
    }

    const int index = m_model->index(url);
    if (index >= 0) {
        updateCurrentItem(index);
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

#include "folderspanel.moc"
