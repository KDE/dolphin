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

#include "rolesaccessor.h"
#include "settings/viewpropertiesdialog.h"
#include "views/dolphinview.h"
#include "views/zoomlevelinfo.h"
#include <konq_operations.h>

#include <KAction>
#include <KActionCollection>
#include <KActionMenu>
#include <KFileItemDelegate>
#include <KLocale>
#include <KNewFileMenu>
#include <KSelectAction>
#include <KToggleAction>
#include <KRun>
#include <KPropertiesDialog>

#include <KDebug>

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

    if (m_currentView) {
        disconnect(m_currentView, 0, this, 0);
    }

    m_currentView = view;

    connect(view, SIGNAL(modeChanged(DolphinView::Mode,DolphinView::Mode)),
            this, SLOT(updateViewActions()));
    connect(view, SIGNAL(previewsShownChanged(bool)),
            this, SLOT(slotPreviewsShownChanged(bool)));
    connect(view, SIGNAL(sortOrderChanged(Qt::SortOrder)),
            this, SLOT(slotSortOrderChanged(Qt::SortOrder)));
    connect(view, SIGNAL(sortFoldersFirstChanged(bool)),
            this, SLOT(slotSortFoldersFirstChanged(bool)));
    connect(view, SIGNAL(visibleRolesChanged(QList<QByteArray>,QList<QByteArray>)),
            this, SLOT(slotVisibleRolesChanged(QList<QByteArray>,QList<QByteArray>)));
    connect(view, SIGNAL(groupedSortingChanged(bool)),
            this, SLOT(slotGroupedSortingChanged(bool)));
    connect(view, SIGNAL(hiddenFilesShownChanged(bool)),
            this, SLOT(slotHiddenFilesShownChanged(bool)));
    connect(view, SIGNAL(sortRoleChanged(QByteArray)),
            this, SLOT(slotSortRoleChanged(QByteArray)));
    connect(view, SIGNAL(zoomLevelChanged(int,int)),
            this, SLOT(slotZoomLevelChanged(int,int)));
}

DolphinView* DolphinViewActionHandler::currentView()
{
    return m_currentView;
}

void DolphinViewActionHandler::createActions()
{
    // This action doesn't appear in the GUI, it's for the shortcut only.
    // KNewFileMenu takes care of the GUI stuff.
    KAction* newDirAction = m_actionCollection->addAction("create_dir");
    newDirAction->setText(i18nc("@action", "Create Folder..."));
    newDirAction->setShortcut(Qt::Key_F10);
    newDirAction->setIcon(KIcon("folder-new"));
    connect(newDirAction, SIGNAL(triggered()), this, SIGNAL(createDirectory()));

    // File menu

    KAction* rename = m_actionCollection->addAction("rename");
    rename->setText(i18nc("@action:inmenu File", "Rename..."));
    rename->setShortcut(Qt::Key_F2);
    rename->setIcon(KIcon("edit-rename"));
    connect(rename, SIGNAL(triggered()), this, SLOT(slotRename()));

    KAction* moveToTrash = m_actionCollection->addAction("move_to_trash");
    moveToTrash->setText(i18nc("@action:inmenu File", "Move to Trash"));
    moveToTrash->setIcon(KIcon("user-trash"));
    moveToTrash->setShortcut(QKeySequence::Delete);
    connect(moveToTrash, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)),
            this, SLOT(slotTrashActivated(Qt::MouseButtons,Qt::KeyboardModifiers)));

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
    deleteWithTrashShortcut->setText(i18nc("@action \"Move to Trash\" for non-local files, etc.", "Delete (using shortcut for Trash)"));
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
    KToggleAction* iconsAction = iconsModeAction();
    KToggleAction* compactAction = compactModeAction();
    KToggleAction* detailsAction = detailsModeAction();

    KSelectAction* viewModeActions = m_actionCollection->add<KSelectAction>("view_mode");
    viewModeActions->setText(i18nc("@action:intoolbar", "View Mode"));
    viewModeActions->addAction(iconsAction);
    viewModeActions->addAction(compactAction);
    viewModeActions->addAction(detailsAction);
    viewModeActions->setToolBarMode(KSelectAction::MenuMode);
    connect(viewModeActions, SIGNAL(triggered(QAction*)), this, SLOT(slotViewModeActionTriggered(QAction*)));

    KStandardAction::zoomIn(this,
                            SLOT(zoomIn()),
                            m_actionCollection);

    KStandardAction::zoomOut(this,
                             SLOT(zoomOut()),
                             m_actionCollection);

    KToggleAction* showPreview = m_actionCollection->add<KToggleAction>("show_preview");
    showPreview->setText(i18nc("@action:intoolbar", "Preview"));
    showPreview->setToolTip(i18nc("@info", "Show preview of files and folders"));
    showPreview->setIcon(KIcon("view-preview"));
    connect(showPreview, SIGNAL(triggered(bool)), this, SLOT(togglePreview(bool)));

    KToggleAction* sortDescending = m_actionCollection->add<KToggleAction>("descending");
    sortDescending->setText(i18nc("@action:inmenu Sort", "Descending"));
    connect(sortDescending, SIGNAL(triggered()), this, SLOT(toggleSortOrder()));

    KToggleAction* sortFoldersFirst = m_actionCollection->add<KToggleAction>("folders_first");
    sortFoldersFirst->setText(i18nc("@action:inmenu Sort", "Folders First"));
    connect(sortFoldersFirst, SIGNAL(triggered()), this, SLOT(toggleSortFoldersFirst()));

    // View -> Sort By
    QActionGroup* sortByActionGroup = createSortByActionGroup();
    connect(sortByActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(slotSortTriggered(QAction*)));

    KActionMenu* sortByActionMenu = m_actionCollection->add<KActionMenu>("sort");
    sortByActionMenu->setText(i18nc("@action:inmenu View", "Sort By"));
    sortByActionMenu->setDelayed(false);

    foreach (QAction* action, sortByActionGroup->actions()) {
        sortByActionMenu->addAction(action);
    }
    sortByActionMenu->addSeparator();
    sortByActionMenu->addAction(sortDescending);
    sortByActionMenu->addAction(sortFoldersFirst);

    // View -> Additional Information
    QActionGroup* additionalInfoGroup = createAdditionalInformationActionGroup();
    connect(additionalInfoGroup, SIGNAL(triggered(QAction*)), this, SLOT(toggleAdditionalInfo(QAction*)));

    KActionMenu* additionalInfoMenu = m_actionCollection->add<KActionMenu>("additional_info");
    additionalInfoMenu->setText(i18nc("@action:inmenu View", "Additional Information"));
    additionalInfoMenu->setDelayed(false);
    foreach (QAction* action, additionalInfoGroup->actions()) {
        additionalInfoMenu->addAction(action);
    }

    KToggleAction* showInGroups = m_actionCollection->add<KToggleAction>("show_in_groups");
    showInGroups->setIcon(KIcon("view-group"));
    showInGroups->setText(i18nc("@action:inmenu View", "Show in Groups"));
    connect(showInGroups, SIGNAL(triggered(bool)), this, SLOT(toggleGroupedSorting(bool)));

    KToggleAction* showHiddenFiles = m_actionCollection->add<KToggleAction>("show_hidden_files");
    showHiddenFiles->setText(i18nc("@action:inmenu View", "Show Hidden Files"));
    showHiddenFiles->setShortcuts(QList<QKeySequence>() << Qt::ALT + Qt::Key_Period << Qt::Key_F8);
    connect(showHiddenFiles, SIGNAL(triggered(bool)), this, SLOT(toggleShowHiddenFiles(bool)));

    KAction* adjustViewProps = m_actionCollection->addAction("view_properties");
    adjustViewProps->setText(i18nc("@action:inmenu View", "Adjust View Properties..."));
    connect(adjustViewProps, SIGNAL(triggered()), this, SLOT(slotAdjustViewProperties()));
}

QActionGroup* DolphinViewActionHandler::createAdditionalInformationActionGroup()
{
    QActionGroup* additionalInfoGroup = new QActionGroup(m_actionCollection);
    additionalInfoGroup->setExclusive(false);

    KActionMenu* showInformationMenu = m_actionCollection->add<KActionMenu>("additional_info");
    showInformationMenu->setText(i18nc("@action:inmenu View", "Additional Information"));
    showInformationMenu->setDelayed(false);

    const RolesAccessor& rolesAccessor = RolesAccessor::instance();

    const QList<QByteArray> roles = rolesAccessor.roles();
    foreach (const QByteArray& role, roles) {
        if (role == "name") {
            // It should not be possible to hide the "name" role
            continue;
        }

        const QString name = QLatin1String("show_") + role;
        KToggleAction* action = m_actionCollection->add<KToggleAction>(name);
        action->setText(rolesAccessor.translation(role));
        action->setData(role);
        action->setActionGroup(additionalInfoGroup);
    }

    return additionalInfoGroup;
}

QActionGroup* DolphinViewActionHandler::createSortByActionGroup()
{
    QActionGroup* sortByActionGroup = new QActionGroup(m_actionCollection);
    sortByActionGroup->setExclusive(true);

    const RolesAccessor& rolesAccessor = RolesAccessor::instance();
    const QList<QByteArray> roles = rolesAccessor.roles();
    foreach (const QByteArray& role, roles) {
        const QString name = QLatin1String("sort_by_") + role;
        KToggleAction* action = m_actionCollection->add<KToggleAction>(name);
        action->setText(rolesAccessor.translation(role));
        action->setData(role);
        sortByActionGroup->addAction(action);
    }

    return sortByActionGroup;
}

void DolphinViewActionHandler::slotViewModeActionTriggered(QAction* action)
{
    const DolphinView::Mode mode = action->data().value<DolphinView::Mode>();
    m_currentView->setMode(mode);

    QAction* viewModeMenu = m_actionCollection->action("view_mode");
    viewModeMenu->setIcon(KIcon(action->icon()));
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
    if (modifiers & Qt::ShiftModifier) {
        m_currentView->deleteSelectedItems();
    } else {
        m_currentView->trashSelectedItems();
    }
}

void DolphinViewActionHandler::slotDeleteItems()
{
    emit actionBeingHandled();
    m_currentView->deleteSelectedItems();
}

void DolphinViewActionHandler::togglePreview(bool show)
{
    emit actionBeingHandled();
    m_currentView->setPreviewsShown(show);
}

void DolphinViewActionHandler::slotPreviewsShownChanged(bool shown)
{
    Q_UNUSED(shown);
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
    case DolphinView::CompactView:
        return "compact";
    default:
        Q_ASSERT(false);
        break;
    }
    return QString(); // can't happen
}

KActionCollection* DolphinViewActionHandler::actionCollection()
{
    return m_actionCollection;
}

void DolphinViewActionHandler::updateViewActions()
{
    QAction* viewModeAction = m_actionCollection->action(currentViewModeActionName());
    if (viewModeAction) {
        viewModeAction->setChecked(true);

        QAction* viewModeMenu = m_actionCollection->action("view_mode");
        viewModeMenu->setIcon(KIcon(viewModeAction->icon()));
    }

    QAction* showPreviewAction = m_actionCollection->action("show_preview");
    showPreviewAction->setChecked(m_currentView->previewsShown());

    slotSortOrderChanged(m_currentView->sortOrder());
    slotSortFoldersFirstChanged(m_currentView->sortFoldersFirst());
    slotVisibleRolesChanged(m_currentView->visibleRoles(), QList<QByteArray>());
    slotGroupedSortingChanged(m_currentView->groupedSorting());
    slotSortRoleChanged(m_currentView->sortRole());
    slotZoomLevelChanged(m_currentView->zoomLevel(), -1);

    QAction* showHiddenFilesAction = m_actionCollection->action("show_hidden_files");
    showHiddenFilesAction->setChecked(m_currentView->hiddenFilesShown());
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
    const Qt::SortOrder order = (m_currentView->sortOrder() == Qt::AscendingOrder) ?
                                Qt::DescendingOrder :
                                Qt::AscendingOrder;
    m_currentView->setSortOrder(order);
}

void DolphinViewActionHandler::toggleSortFoldersFirst()
{
    const bool sortFirst = m_currentView->sortFoldersFirst();
    m_currentView->setSortFoldersFirst(!sortFirst);
}

void DolphinViewActionHandler::slotSortOrderChanged(Qt::SortOrder order)
{
    QAction* descending = m_actionCollection->action("descending");
    const bool sortDescending = (order == Qt::DescendingOrder);
    descending->setChecked(sortDescending);
}

void DolphinViewActionHandler::slotSortFoldersFirstChanged(bool foldersFirst)
{
    m_actionCollection->action("folders_first")->setChecked(foldersFirst);
}

void DolphinViewActionHandler::toggleAdditionalInfo(QAction* action)
{
    emit actionBeingHandled();

    const QByteArray toggledRole = action->data().toByteArray();

    QList<QByteArray> roles = m_currentView->visibleRoles();

    const bool show = action->isChecked();

    const int index = roles.indexOf(toggledRole);
    const bool containsInfo = (index >= 0);
    if (show && !containsInfo) {
        roles.append(toggledRole);
        m_currentView->setVisibleRoles(roles);
    } else if (!show && containsInfo) {
        roles.removeAt(index);
        m_currentView->setVisibleRoles(roles);
        Q_ASSERT(roles.indexOf(toggledRole) < 0);
    }
}

void DolphinViewActionHandler::slotVisibleRolesChanged(const QList<QByteArray>& current,
                                                       const QList<QByteArray>& previous)
{
    Q_UNUSED(previous);

    const RolesAccessor& rolesAccessor = RolesAccessor::instance();

    const QSet<QByteArray> checkedRoles = current.toSet();
    const QList<QByteArray> roles = rolesAccessor.roles();

    foreach (const QByteArray& role, roles) {
        const QString name = QLatin1String("show_") + role;
        QAction* action = m_actionCollection->action(name);
        if (action) {
            action->setChecked(checkedRoles.contains(role));
        }
    }
}

void DolphinViewActionHandler::toggleGroupedSorting(bool grouped)
{
    m_currentView->setGroupedSorting(grouped);
}

void DolphinViewActionHandler::slotGroupedSortingChanged(bool groupedSorting)
{
    QAction* showInGroupsAction = m_actionCollection->action("show_in_groups");
    showInGroupsAction->setChecked(groupedSorting);
}

void DolphinViewActionHandler::toggleShowHiddenFiles(bool show)
{
    emit actionBeingHandled();
    m_currentView->setHiddenFilesShown(show);
}

void DolphinViewActionHandler::slotHiddenFilesShownChanged(bool shown)
{
    QAction* showHiddenFilesAction = m_actionCollection->action("show_hidden_files");
    showHiddenFilesAction->setChecked(shown);
}

KToggleAction* DolphinViewActionHandler::iconsModeAction()
{
    KToggleAction* iconsView = m_actionCollection->add<KToggleAction>("icons");
    iconsView->setText(i18nc("@action:inmenu View Mode", "Icons"));
    iconsView->setToolTip(i18nc("@info", "Icons view mode"));
    iconsView->setShortcut(Qt::CTRL | Qt::Key_1);
    iconsView->setIcon(KIcon("view-list-icons"));
    iconsView->setData(QVariant::fromValue(DolphinView::IconsView));
    return iconsView;
}

KToggleAction* DolphinViewActionHandler::compactModeAction()
{
    KToggleAction* iconsView = m_actionCollection->add<KToggleAction>("compact");
    iconsView->setText(i18nc("@action:inmenu View Mode", "Compact"));
    iconsView->setToolTip(i18nc("@info", "Compact view mode"));
    iconsView->setShortcut(Qt::CTRL | Qt::Key_2);
    iconsView->setIcon(KIcon("view-list-details")); // TODO: discuss with Oxygen-team the wrong (?) name
    iconsView->setData(QVariant::fromValue(DolphinView::CompactView));
    return iconsView;
}

KToggleAction* DolphinViewActionHandler::detailsModeAction()
{
    KToggleAction* detailsView = m_actionCollection->add<KToggleAction>("details");
    detailsView->setText(i18nc("@action:inmenu View Mode", "Details"));
    detailsView->setToolTip(i18nc("@info", "Details view mode"));
    detailsView->setShortcut(Qt::CTRL | Qt::Key_3);
    detailsView->setIcon(KIcon("view-list-tree"));
    detailsView->setData(QVariant::fromValue(DolphinView::DetailsView));
    return detailsView;
}

void DolphinViewActionHandler::slotSortRoleChanged(const QByteArray& role)
{
    const QString name = QLatin1String("sort_by_") + role;
    QAction* action = m_actionCollection->action(name);
    if (action) {
        action->setChecked(true);

        QAction* sortByMenu =  m_actionCollection->action("sort");
        sortByMenu->setIcon(KIcon(action->icon()));
    }
}

void DolphinViewActionHandler::slotZoomLevelChanged(int current, int previous)
{
    Q_UNUSED(previous);

    QAction* zoomInAction = m_actionCollection->action(KStandardAction::name(KStandardAction::ZoomIn));
    if (zoomInAction) {
        zoomInAction->setEnabled(current < ZoomLevelInfo::maximumLevel());
    }

    QAction* zoomOutAction = m_actionCollection->action(KStandardAction::name(KStandardAction::ZoomOut));
    if (zoomOutAction) {
        zoomOutAction->setEnabled(current > ZoomLevelInfo::minimumLevel());
    }
}

void DolphinViewActionHandler::slotSortTriggered(QAction* action)
{
    const QByteArray role = action->data().toByteArray();
    m_currentView->setSortRole(role);
}

void DolphinViewActionHandler::slotAdjustViewProperties()
{
    emit actionBeingHandled();
    QPointer<ViewPropertiesDialog> dialog = new ViewPropertiesDialog(m_currentView);
    dialog->exec();
    delete dialog;
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
