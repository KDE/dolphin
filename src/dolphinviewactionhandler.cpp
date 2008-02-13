/***************************************************************************
 *   Copyright (C) 2008 by David Faure <faure@kde.org>                     *
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

#include "dolphinviewactionhandler.h"
#include <kdebug.h>

#include "dolphinview.h"

#include <konq_operations.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <ktoggleaction.h>

DolphinViewActionHandler::DolphinViewActionHandler(KActionCollection* collection, QObject* parent)
    : QObject(parent),
      m_actionCollection(collection),
      m_currentView(0)
{
    Q_ASSERT(m_actionCollection);
    createActions();
}

void DolphinViewActionHandler::setCurrentView(DolphinView* view)
{
    Q_ASSERT(view);

    if (m_currentView)
        disconnect(m_currentView, 0, this, 0);

    m_currentView = view;

    connect(view, SIGNAL(modeChanged()),
            this, SLOT(updateViewActions()));
    connect(view, SIGNAL(showPreviewChanged()),
            this, SLOT(slotShowPreviewChanged()));
    connect(view, SIGNAL(sortOrderChanged(Qt::SortOrder)),
            this, SLOT(slotSortOrderChanged(Qt::SortOrder)));
    connect(view, SIGNAL(additionalInfoChanged()),
            this, SLOT(slotAdditionalInfoChanged()));
    connect(view, SIGNAL(categorizedSortingChanged()),
            this, SLOT(slotCategorizedSortingChanged()));
    connect(view, SIGNAL(showHiddenFilesChanged()),
            this, SLOT(slotShowHiddenFilesChanged()));
}

void DolphinViewActionHandler::createActions()
{
    // This action doesn't appear in the GUI, it's for the shortcut only.
    // KNewMenu takes care of the GUI stuff.
    KAction* newDirAction = m_actionCollection->addAction("create_dir");
    newDirAction->setText(i18n("Create Folder..."));
    newDirAction->setShortcut(Qt::Key_F10);
    connect(newDirAction, SIGNAL(triggered()), SLOT(slotCreateDir()));

    // Edit menu

    KAction* rename = m_actionCollection->addAction("rename");
    rename->setText(i18nc("@action:inmenu File", "Rename..."));
    rename->setShortcut(Qt::Key_F2);
    connect(rename, SIGNAL(triggered()), this, SLOT(slotRename()));

    KAction* moveToTrash = m_actionCollection->addAction("move_to_trash");
    moveToTrash->setText(i18nc("@action:inmenu File", "Move to Trash"));
    moveToTrash->setIcon(KIcon("user-trash"));
    moveToTrash->setShortcut(QKeySequence::Delete);
    connect(moveToTrash, SIGNAL(triggered(Qt::MouseButtons, Qt::KeyboardModifiers)),
            this, SLOT(slotTrashActivated(Qt::MouseButtons, Qt::KeyboardModifiers)));

    KAction* deleteAction = m_actionCollection->addAction("delete");
    deleteAction->setIcon(KIcon("edit-delete"));
    deleteAction->setText(i18nc("@action:inmenu File", "Delete"));
    deleteAction->setShortcut(Qt::SHIFT | Qt::Key_Delete);
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(slotDeleteItems()));

    // View menu

    QActionGroup* viewModeActions = new QActionGroup(this);
    viewModeActions->addAction(iconsModeAction());
    viewModeActions->addAction(detailsModeAction());
    viewModeActions->addAction(columnsModeAction());
    connect(viewModeActions, SIGNAL(triggered(QAction*)), this, SLOT(slotViewModeActionTriggered(QAction*)));

    KStandardAction::zoomIn(this,
                            SLOT(zoomIn()),
                            m_actionCollection);

    KStandardAction::zoomOut(this,
                             SLOT(zoomOut()),
                             m_actionCollection);

    KToggleAction* showPreview = m_actionCollection->add<KToggleAction>("show_preview");
    showPreview->setText(i18nc("@action:intoolbar", "Preview"));
    showPreview->setIcon(KIcon("view-preview"));
    connect(showPreview, SIGNAL(triggered(bool)), this, SLOT(togglePreview(bool)));

    KToggleAction* sortDescending = m_actionCollection->add<KToggleAction>("descending");
    sortDescending->setText(i18nc("@action:inmenu Sort", "Descending"));
    connect(sortDescending, SIGNAL(triggered()), this, SLOT(toggleSortOrder()));

    QActionGroup* showInformationActionGroup = createAdditionalInformationActionGroup();
    connect(showInformationActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(toggleAdditionalInfo(QAction*)));

    KToggleAction* showInGroups = m_actionCollection->add<KToggleAction>("show_in_groups");
    showInGroups->setText(i18nc("@action:inmenu View", "Show in Groups"));
    connect(showInGroups, SIGNAL(triggered(bool)), this, SLOT(toggleSortCategorization(bool)));

    KToggleAction* showHiddenFiles = m_actionCollection->add<KToggleAction>("show_hidden_files");
    showHiddenFiles->setText(i18nc("@action:inmenu View", "Show Hidden Files"));
    showHiddenFiles->setShortcut(Qt::ALT | Qt::Key_Period);
    connect(showHiddenFiles, SIGNAL(triggered(bool)), this, SLOT(toggleShowHiddenFiles(bool)));

}

QActionGroup* DolphinViewActionHandler::createAdditionalInformationActionGroup()
{
    QActionGroup* showInformationGroup = new QActionGroup(m_actionCollection);
    showInformationGroup->setExclusive(false);

    KToggleAction* showSizeInfo = m_actionCollection->add<KToggleAction>("show_size_info");
    showSizeInfo->setText(i18nc("@action:inmenu Additional information", "Size"));
    showSizeInfo->setData(KFileItemDelegate::Size);
    showSizeInfo->setActionGroup(showInformationGroup);

    KToggleAction* showDateInfo = m_actionCollection->add<KToggleAction>("show_date_info");
    showDateInfo->setText(i18nc("@action:inmenu Additional information", "Date"));
    showDateInfo->setData(KFileItemDelegate::ModificationTime);
    showDateInfo->setActionGroup(showInformationGroup);

    KToggleAction* showPermissionsInfo = m_actionCollection->add<KToggleAction>("show_permissions_info");
    showPermissionsInfo->setText(i18nc("@action:inmenu Additional information", "Permissions"));
    showPermissionsInfo->setData(KFileItemDelegate::Permissions);
    showPermissionsInfo->setActionGroup(showInformationGroup);

    KToggleAction* showOwnerInfo = m_actionCollection->add<KToggleAction>("show_owner_info");
    showOwnerInfo->setText(i18nc("@action:inmenu Additional information", "Owner"));
    showOwnerInfo->setData(KFileItemDelegate::Owner);
    showOwnerInfo->setActionGroup(showInformationGroup);

    KToggleAction* showGroupInfo = m_actionCollection->add<KToggleAction>("show_group_info");
    showGroupInfo->setText(i18nc("@action:inmenu Additional information", "Group"));
    showGroupInfo->setData(KFileItemDelegate::OwnerAndGroup);
    showGroupInfo->setActionGroup(showInformationGroup);

    KToggleAction* showMimeInfo = m_actionCollection->add<KToggleAction>("show_mime_info");
    showMimeInfo->setText(i18nc("@action:inmenu Additional information", "Type"));
    showMimeInfo->setData(KFileItemDelegate::FriendlyMimeType);
    showMimeInfo->setActionGroup(showInformationGroup);

    return showInformationGroup;
}

void DolphinViewActionHandler::slotCreateDir()
{
    Q_ASSERT(m_currentView);
    KonqOperations::newDir(m_currentView, m_currentView->url());
}

void DolphinViewActionHandler::slotViewModeActionTriggered(QAction* action)
{
    const DolphinView::Mode mode = action->data().value<DolphinView::Mode>();
    m_currentView->setMode(mode);
}

void DolphinViewActionHandler::slotRename()
{
    emit actionBeingHandled();
    m_currentView->renameSelectedItems();
}

void DolphinViewActionHandler::slotTrashActivated(Qt::MouseButtons, Qt::KeyboardModifiers modifiers)
{
    emit actionBeingHandled();
    // Note: kde3's konq_mainwindow.cpp used to check
    // reason == KAction::PopupMenuActivation && ...
    // but this isn't supported anymore
    if (modifiers & Qt::ShiftModifier)
        m_currentView->deleteSelectedItems();
    else
        m_currentView->trashSelectedItems();
}

void DolphinViewActionHandler::slotDeleteItems()
{
    emit actionBeingHandled();
    m_currentView->deleteSelectedItems();
}

void DolphinViewActionHandler::togglePreview(bool show)
{
    emit actionBeingHandled();
    m_currentView->setShowPreview(show);
}

void DolphinViewActionHandler::slotShowPreviewChanged()
{
    // It is not enough to update the 'Show Preview' action, also
    // the 'Zoom In' and 'Zoom Out' actions must be adapted.
    updateViewActions();
}

QString DolphinViewActionHandler::currentViewModeActionName() const
{
    switch (m_currentView->mode()) {
    case DolphinView::IconsView:
        return "icons";
    case DolphinView::DetailsView:
        return "details";
    case DolphinView::ColumnView:
        return "columns";
    }
    return QString(); // can't happen
}

void DolphinViewActionHandler::updateViewActions()
{
    QAction* viewModeAction = m_actionCollection->action(currentViewModeActionName());
    if (viewModeAction != 0) {
        viewModeAction->setChecked(true);
    }

    QAction* zoomInAction = m_actionCollection->action(KStandardAction::stdName(KStandardAction::ZoomIn));
    if (zoomInAction != 0) {
        zoomInAction->setEnabled(m_currentView->isZoomInPossible());
    }

    QAction* zoomOutAction = m_actionCollection->action(KStandardAction::stdName(KStandardAction::ZoomOut));
    if (zoomOutAction != 0) {
        zoomOutAction->setEnabled(m_currentView->isZoomOutPossible());
    }

    QAction* showPreviewAction = m_actionCollection->action("show_preview");
    showPreviewAction->setChecked(m_currentView->showPreview());

    slotSortOrderChanged(m_currentView->sortOrder());
    slotAdditionalInfoChanged();
    slotCategorizedSortingChanged();

    QAction* showHiddenFilesAction = m_actionCollection->action("show_hidden_files");
    showHiddenFilesAction->setChecked(m_currentView->showHiddenFiles());

}

void DolphinViewActionHandler::zoomIn()
{
    m_currentView->zoomIn();
    updateViewActions();
}

void DolphinViewActionHandler::zoomOut()
{
    m_currentView->zoomOut();
    updateViewActions();
}

void DolphinViewActionHandler::toggleSortOrder()
{
    m_currentView->toggleSortOrder();
}

void DolphinViewActionHandler::slotSortOrderChanged(Qt::SortOrder order)
{
    QAction* descending = m_actionCollection->action("descending");
    const bool sortDescending = (order == Qt::DescendingOrder);
    descending->setChecked(sortDescending);
}

void DolphinViewActionHandler::toggleAdditionalInfo(QAction* action)
{
    emit actionBeingHandled();
    m_currentView->toggleAdditionalInfo(action);
}

void DolphinViewActionHandler::slotAdditionalInfoChanged()
{
    m_currentView->updateAdditionalInfoActions(m_actionCollection);
}

void DolphinViewActionHandler::toggleSortCategorization(bool categorizedSorting)
{
    m_currentView->setCategorizedSorting(categorizedSorting);
}

void DolphinViewActionHandler::slotCategorizedSortingChanged()
{
    QAction* showInGroupsAction = m_actionCollection->action("show_in_groups");
    showInGroupsAction->setChecked(m_currentView->categorizedSorting());
    showInGroupsAction->setEnabled(m_currentView->supportsCategorizedSorting());
}

void DolphinViewActionHandler::toggleShowHiddenFiles(bool show)
{
    emit actionBeingHandled();
    m_currentView->setShowHiddenFiles(show);
}

void DolphinViewActionHandler::slotShowHiddenFilesChanged()
{
    QAction* showHiddenFilesAction = m_actionCollection->action("show_hidden_files");
    showHiddenFilesAction->setChecked(m_currentView->showHiddenFiles());
}


KToggleAction* DolphinViewActionHandler::iconsModeAction()
{
    KToggleAction* iconsView = m_actionCollection->add<KToggleAction>("icons");
    iconsView->setText(i18nc("@action:inmenu View Mode", "Icons"));
    iconsView->setShortcut(Qt::CTRL | Qt::Key_1);
    iconsView->setIcon(KIcon("view-list-icons"));
    iconsView->setData(QVariant::fromValue(DolphinView::IconsView));
    return iconsView;
}

KToggleAction* DolphinViewActionHandler::detailsModeAction()
{
    KToggleAction* detailsView = m_actionCollection->add<KToggleAction>("details");
    detailsView->setText(i18nc("@action:inmenu View Mode", "Details"));
    detailsView->setShortcut(Qt::CTRL | Qt::Key_2);
    detailsView->setIcon(KIcon("view-list-details"));
    detailsView->setData(QVariant::fromValue(DolphinView::DetailsView));
    return detailsView;
}

KToggleAction* DolphinViewActionHandler::columnsModeAction()
{
    KToggleAction* columnView = m_actionCollection->add<KToggleAction>("columns");
    columnView->setText(i18nc("@action:inmenu View Mode", "Columns"));
    columnView->setShortcut(Qt::CTRL | Qt::Key_3);
    columnView->setIcon(KIcon("view-file-columns"));
    columnView->setData(QVariant::fromValue(DolphinView::ColumnView));
    return columnView;
}
