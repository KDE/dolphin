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

#include "settings/viewpropertiesdialog.h"
#include "dolphinview.h"
#include "zoomlevelinfo.h"

#include <konq_operations.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <ktoggleaction.h>
#include <krun.h>
#include <kpropertiesdialog.h>

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
    connect(view, SIGNAL(sortingChanged(DolphinView::Sorting)),
            this, SLOT(slotSortingChanged(DolphinView::Sorting)));
    connect(view, SIGNAL(zoomLevelChanged(int)),
            this, SLOT(slotZoomLevelChanged(int)));
}

void DolphinViewActionHandler::createActions()
{
    // This action doesn't appear in the GUI, it's for the shortcut only.
    // KNewMenu takes care of the GUI stuff.
    KAction* newDirAction = m_actionCollection->addAction("create_dir");
    newDirAction->setText(i18nc("@action", "Create Folder..."));
    newDirAction->setShortcut(Qt::Key_F10);
    newDirAction->setIcon(KIcon("folder-new"));
    connect(newDirAction, SIGNAL(triggered()), SLOT(slotCreateDir()));

    // Edit menu

    KAction* rename = m_actionCollection->addAction("rename");
    rename->setText(i18nc("@action:inmenu File", "Rename..."));
    rename->setShortcut(Qt::Key_F2);
    rename->setIcon(KIcon("edit-rename"));
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

    // This action is useful for being enabled when "move_to_trash" should be
    // disabled and "delete" is enabled (e.g. non-local files), so that Key_Del
    // can be used for deleting the file (#76016). It needs to be a separate action
    // so that the Edit menu isn't affected.
    KAction* deleteWithTrashShortcut = m_actionCollection->addAction("delete_shortcut");
    // The descriptive text is just for the shortcuts editor.
    deleteWithTrashShortcut->setText(i18nc("@action:inmenu File", "Delete (using shortcut for Trash)"));
    deleteWithTrashShortcut->setShortcut(QKeySequence::Delete);
    deleteWithTrashShortcut->setEnabled(false);
    connect(deleteWithTrashShortcut, SIGNAL(triggered()), this, SLOT(slotDeleteItems()));

    KAction *propertiesAction = m_actionCollection->addAction( "properties" );
    // Well, it's the File menu in dolphinmainwindow and the Edit menu in dolphinpart... :)
    propertiesAction->setText( i18nc("@action:inmenu File", "Properties") );
    propertiesAction->setIcon(KIcon("document-properties"));
    propertiesAction->setShortcut(Qt::ALT | Qt::Key_Return);
    connect(propertiesAction, SIGNAL(triggered()), SLOT(slotProperties()));

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

    QActionGroup* sortByActionGroup = createSortByActionGroup();
    connect(sortByActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(slotSortTriggered(QAction*)));

    QActionGroup* showInformationActionGroup = createAdditionalInformationActionGroup();
    connect(showInformationActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(toggleAdditionalInfo(QAction*)));

    KToggleAction* showInGroups = m_actionCollection->add<KToggleAction>("show_in_groups");
    showInGroups->setText(i18nc("@action:inmenu View", "Show in Groups"));
    connect(showInGroups, SIGNAL(triggered(bool)), this, SLOT(toggleSortCategorization(bool)));

    KToggleAction* showHiddenFiles = m_actionCollection->add<KToggleAction>("show_hidden_files");
    showHiddenFiles->setText(i18nc("@action:inmenu View", "Show Hidden Files"));
    showHiddenFiles->setShortcut(Qt::ALT | Qt::Key_Period);
    connect(showHiddenFiles, SIGNAL(triggered(bool)), this, SLOT(toggleShowHiddenFiles(bool)));

    KAction* adjustViewProps = m_actionCollection->addAction("view_properties");
    adjustViewProps->setText(i18nc("@action:inmenu View", "Adjust View Properties..."));
    connect(adjustViewProps, SIGNAL(triggered()), this, SLOT(slotAdjustViewProperties()));

    // Tools menu

    KAction* findFile = m_actionCollection->addAction("find_file");
    findFile->setText(i18nc("@action:inmenu Tools", "Find File..."));
    findFile->setShortcut(Qt::CTRL | Qt::Key_F);
    findFile->setIcon(KIcon("edit-find"));
    connect(findFile, SIGNAL(triggered()), this, SLOT(slotFindFile()));
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

Q_DECLARE_METATYPE(DolphinView::Sorting)

QActionGroup* DolphinViewActionHandler::createSortByActionGroup()
{
    QActionGroup* sortByActionGroup = new QActionGroup(m_actionCollection);
    sortByActionGroup->setExclusive(true);

    KToggleAction* sortByName = m_actionCollection->add<KToggleAction>("sort_by_name");
    sortByName->setText(i18nc("@action:inmenu Sort By", "Name"));
    sortByName->setData(QVariant::fromValue(DolphinView::SortByName));
    sortByActionGroup->addAction(sortByName);

    KToggleAction* sortBySize = m_actionCollection->add<KToggleAction>("sort_by_size");
    sortBySize->setText(i18nc("@action:inmenu Sort By", "Size"));
    sortBySize->setData(QVariant::fromValue(DolphinView::SortBySize));
    sortByActionGroup->addAction(sortBySize);

    KToggleAction* sortByDate = m_actionCollection->add<KToggleAction>("sort_by_date");
    sortByDate->setText(i18nc("@action:inmenu Sort By", "Date"));
    sortByDate->setData(QVariant::fromValue(DolphinView::SortByDate));
    sortByActionGroup->addAction(sortByDate);

    KToggleAction* sortByPermissions = m_actionCollection->add<KToggleAction>("sort_by_permissions");
    sortByPermissions->setText(i18nc("@action:inmenu Sort By", "Permissions"));
    sortByPermissions->setData(QVariant::fromValue(DolphinView::SortByPermissions));
    sortByActionGroup->addAction(sortByPermissions);

    KToggleAction* sortByOwner = m_actionCollection->add<KToggleAction>("sort_by_owner");
    sortByOwner->setText(i18nc("@action:inmenu Sort By", "Owner"));
    sortByOwner->setData(QVariant::fromValue(DolphinView::SortByOwner));
    sortByActionGroup->addAction(sortByOwner);

    KToggleAction* sortByGroup = m_actionCollection->add<KToggleAction>("sort_by_group");
    sortByGroup->setText(i18nc("@action:inmenu Sort By", "Group"));
    sortByGroup->setData(QVariant::fromValue(DolphinView::SortByGroup));
    sortByActionGroup->addAction(sortByGroup);

    KToggleAction* sortByType = m_actionCollection->add<KToggleAction>("sort_by_type");
    sortByType->setText(i18nc("@action:inmenu Sort By", "Type"));
    sortByType->setData(QVariant::fromValue(DolphinView::SortByType));
    sortByActionGroup->addAction(sortByType);

    // TODO: Hid "sort by rating" and "sort by tags" as without caching the performance
    // is too slow currently (Nepomuk will support caching in future releases).
    //
    // KToggleAction* sortByRating = m_actionCollection->add<KToggleAction>("sort_by_rating");
    // sortByRating->setData(QVariant::fromValue(DolphinView::SortByRating));
    // sortByRating->setText(i18nc("@action:inmenu Sort By", "Rating"));
    // sortByActionGroup->addAction(sortByRating);
    //
    // KToggleAction* sortByTags = m_actionCollection->add<KToggleAction>("sort_by_tags");
    // sortByTags->setData(QVariant::fromValue(DolphinView::SortByTags));
    // sortByTags->setText(i18nc("@action:inmenu Sort By", "Tags"));
    // sortByActionGroup->addAction(sortByTags);
    //
#ifdef HAVE_NEPOMUK
    // if (!MetaDataWidget::metaDataAvailable()) {
    //     sortByRating->setEnabled(false);
    //     sortByTags->setEnabled(false);
    // }
#else
    // sortByRating->setEnabled(false);
    // sortByTags->setEnabled(false);
#endif



    return sortByActionGroup;
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

    QAction* showPreviewAction = m_actionCollection->action("show_preview");
    showPreviewAction->setChecked(m_currentView->showPreview());

    slotSortOrderChanged(m_currentView->sortOrder());
    slotAdditionalInfoChanged();
    slotCategorizedSortingChanged();
    slotSortingChanged(m_currentView->sorting());
    slotZoomLevelChanged(m_currentView->zoomLevel());

    QAction* showHiddenFilesAction = m_actionCollection->action("show_hidden_files");
    showHiddenFilesAction->setChecked(m_currentView->showHiddenFiles());

}

void DolphinViewActionHandler::zoomIn()
{
    const int level = m_currentView->zoomLevel();
    m_currentView->setZoomLevel(level + 1);
    updateViewActions();
}

void DolphinViewActionHandler::zoomOut()
{
    const int level = m_currentView->zoomLevel();
    m_currentView->setZoomLevel(level - 1);
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

void DolphinViewActionHandler::slotSortingChanged(DolphinView::Sorting sorting)
{
    QAction* action = 0;
    switch (sorting) {
    case DolphinView::SortByName:
        action = m_actionCollection->action("sort_by_name");
        break;
    case DolphinView::SortBySize:
        action = m_actionCollection->action("sort_by_size");
        break;
    case DolphinView::SortByDate:
        action = m_actionCollection->action("sort_by_date");
        break;
    case DolphinView::SortByPermissions:
        action = m_actionCollection->action("sort_by_permissions");
        break;
    case DolphinView::SortByOwner:
        action = m_actionCollection->action("sort_by_owner");
        break;
    case DolphinView::SortByGroup:
        action = m_actionCollection->action("sort_by_group");
        break;
    case DolphinView::SortByType:
        action = m_actionCollection->action("sort_by_type");
        break;
#ifdef HAVE_NEPOMUK
    case DolphinView::SortByRating:
        action = m_actionCollection->action("sort_by_rating");
        break;
    case DolphinView::SortByTags:
        action = m_actionCollection->action("sort_by_tags");
        break;
#endif
    default:
        break;
    }

    if (action != 0) {
        action->setChecked(true);
    }
}

void DolphinViewActionHandler::slotZoomLevelChanged(int level)
{
    QAction* zoomInAction = m_actionCollection->action(KStandardAction::name(KStandardAction::ZoomIn));
    if (zoomInAction != 0) {
        zoomInAction->setEnabled(level < ZoomLevelInfo::maximumLevel());
    }

    QAction* zoomOutAction = m_actionCollection->action(KStandardAction::name(KStandardAction::ZoomOut));
    if (zoomOutAction != 0) {
        zoomOutAction->setEnabled(level > ZoomLevelInfo::minimumLevel());
    }
}

void DolphinViewActionHandler::slotSortTriggered(QAction* action)
{
    const DolphinView::Sorting sorting = action->data().value<DolphinView::Sorting>();
    m_currentView->setSorting(sorting);
}

void DolphinViewActionHandler::slotAdjustViewProperties()
{
    emit actionBeingHandled();
    ViewPropertiesDialog dlg(m_currentView);
    dlg.exec();
}

void DolphinViewActionHandler::slotFindFile()
{
    KRun::run("kfind %u", m_currentView->url(), m_currentView->window());
}

void DolphinViewActionHandler::slotProperties()
{
    KPropertiesDialog* dialog = 0;
    const KFileItemList list = m_currentView->selectedItems();
    if (list.isEmpty()) {
        const KUrl url = m_currentView->url();
        dialog = new KPropertiesDialog(url, m_currentView);
    } else {
        dialog = new KPropertiesDialog(list, m_currentView);
    }

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}
