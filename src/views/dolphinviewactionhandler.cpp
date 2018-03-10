/***************************************************************************
 *   Copyright (C) 2008 by David Faure <faure@kde.org>                     *
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "dolphindebug.h"
#include "kitemviews/kfileitemmodel.h"
#include "settings/viewpropertiesdialog.h"
#include "views/zoomlevelinfo.h"

#ifdef HAVE_BALOO
#include <Baloo/IndexerConfig>
#endif
#include <KActionCollection>
#include <KActionMenu>
#include <KLocalizedString>
#include <KNewFileMenu>
#include <KPropertiesDialog>
#include <KProtocolManager>

#include <QMenu>
#include <QPointer>

DolphinViewActionHandler::DolphinViewActionHandler(KActionCollection* collection, QObject* parent) :
    QObject(parent),
    m_actionCollection(collection),
    m_currentView(nullptr),
    m_sortByActions(),
    m_visibleRoles()
{
    Q_ASSERT(m_actionCollection);
    createActions();
}

void DolphinViewActionHandler::setCurrentView(DolphinView* view)
{
    Q_ASSERT(view);

    if (m_currentView) {
        disconnect(m_currentView, nullptr, this, nullptr);
    }

    m_currentView = view;

    connect(view, &DolphinView::modeChanged,
            this, &DolphinViewActionHandler::updateViewActions);
    connect(view, &DolphinView::previewsShownChanged,
            this, &DolphinViewActionHandler::slotPreviewsShownChanged);
    connect(view, &DolphinView::sortOrderChanged,
            this, &DolphinViewActionHandler::slotSortOrderChanged);
    connect(view, &DolphinView::sortFoldersFirstChanged,
            this, &DolphinViewActionHandler::slotSortFoldersFirstChanged);
    connect(view, &DolphinView::visibleRolesChanged,
            this, &DolphinViewActionHandler::slotVisibleRolesChanged);
    connect(view, &DolphinView::groupedSortingChanged,
            this, &DolphinViewActionHandler::slotGroupedSortingChanged);
    connect(view, &DolphinView::hiddenFilesShownChanged,
            this, &DolphinViewActionHandler::slotHiddenFilesShownChanged);
    connect(view, &DolphinView::sortRoleChanged,
            this, &DolphinViewActionHandler::slotSortRoleChanged);
    connect(view, &DolphinView::zoomLevelChanged,
            this, &DolphinViewActionHandler::slotZoomLevelChanged);
    connect(view, &DolphinView::writeStateChanged,
            this, &DolphinViewActionHandler::slotWriteStateChanged);
}

DolphinView* DolphinViewActionHandler::currentView()
{
    return m_currentView;
}

void DolphinViewActionHandler::createActions()
{
    // This action doesn't appear in the GUI, it's for the shortcut only.
    // KNewFileMenu takes care of the GUI stuff.
    QAction* newDirAction = m_actionCollection->addAction(QStringLiteral("create_dir"));
    newDirAction->setText(i18nc("@action", "Create Folder..."));
    m_actionCollection->setDefaultShortcut(newDirAction, Qt::Key_F10);
    newDirAction->setIcon(QIcon::fromTheme(QStringLiteral("folder-new")));
    newDirAction->setEnabled(false);    // Will be enabled in slotWriteStateChanged(bool) if the current URL is writable
    connect(newDirAction, &QAction::triggered, this, &DolphinViewActionHandler::createDirectory);

    // File menu

    KStandardAction::renameFile(this, &DolphinViewActionHandler::slotRename, m_actionCollection);

    auto trashAction = KStandardAction::moveToTrash(this, &DolphinViewActionHandler::slotTrashActivated, m_actionCollection);
    auto trashShortcuts = trashAction->shortcuts();
    if (!trashShortcuts.contains(QKeySequence::Delete)) {
        trashShortcuts.append(QKeySequence::Delete);
        m_actionCollection->setDefaultShortcuts(trashAction, trashShortcuts);
    }

    auto deleteAction = KStandardAction::deleteFile(this, &DolphinViewActionHandler::slotDeleteItems, m_actionCollection);
    auto deleteShortcuts = deleteAction->shortcuts();
    if (!deleteShortcuts.contains(Qt::SHIFT | Qt::Key_Delete)) {
        deleteShortcuts.append(Qt::SHIFT | Qt::Key_Delete);
        m_actionCollection->setDefaultShortcuts(deleteAction, deleteShortcuts);
    }

    // This action is useful for being enabled when KStandardAction::MoveToTrash should be
    // disabled and KStandardAction::DeleteFile is enabled (e.g. non-local files), so that Key_Del
    // can be used for deleting the file (#76016). It needs to be a separate action
    // so that the Edit menu isn't affected.
    QAction* deleteWithTrashShortcut = m_actionCollection->addAction(QStringLiteral("delete_shortcut"));
    // The descriptive text is just for the shortcuts editor.
    deleteWithTrashShortcut->setText(i18nc("@action \"Move to Trash\" for non-local files, etc.", "Delete (using shortcut for Trash)"));
    m_actionCollection->setDefaultShortcuts(deleteWithTrashShortcut, KStandardShortcut::moveToTrash());
    deleteWithTrashShortcut->setEnabled(false);
    connect(deleteWithTrashShortcut, &QAction::triggered, this, &DolphinViewActionHandler::slotDeleteItems);

    QAction *propertiesAction = m_actionCollection->addAction( QStringLiteral("properties") );
    // Well, it's the File menu in dolphinmainwindow and the Edit menu in dolphinpart... :)
    propertiesAction->setText( i18nc("@action:inmenu File", "Properties") );
    propertiesAction->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
    m_actionCollection->setDefaultShortcuts(propertiesAction, {Qt::ALT + Qt::Key_Return, Qt::ALT + Qt::Key_Enter});
    connect(propertiesAction, &QAction::triggered, this, &DolphinViewActionHandler::slotProperties);

    // View menu
    KToggleAction* iconsAction = iconsModeAction();
    KToggleAction* compactAction = compactModeAction();
    KToggleAction* detailsAction = detailsModeAction();

    KSelectAction* viewModeActions = m_actionCollection->add<KSelectAction>(QStringLiteral("view_mode"));
    viewModeActions->setText(i18nc("@action:intoolbar", "View Mode"));
    viewModeActions->addAction(iconsAction);
    viewModeActions->addAction(compactAction);
    viewModeActions->addAction(detailsAction);
    viewModeActions->setToolBarMode(KSelectAction::MenuMode);
    connect(viewModeActions, static_cast<void(KSelectAction::*)(QAction*)>(&KSelectAction::triggered), this, &DolphinViewActionHandler::slotViewModeActionTriggered);

    KStandardAction::zoomIn(this,
                            &DolphinViewActionHandler::zoomIn,
                            m_actionCollection);

    KStandardAction::zoomOut(this,
                             &DolphinViewActionHandler::zoomOut,
                             m_actionCollection);

    KToggleAction* showPreview = m_actionCollection->add<KToggleAction>(QStringLiteral("show_preview"));
    showPreview->setText(i18nc("@action:intoolbar", "Preview"));
    showPreview->setToolTip(i18nc("@info", "Show preview of files and folders"));
    showPreview->setIcon(QIcon::fromTheme(QStringLiteral("view-preview")));
    connect(showPreview, &KToggleAction::triggered, this, &DolphinViewActionHandler::togglePreview);

    KToggleAction* sortDescending = m_actionCollection->add<KToggleAction>(QStringLiteral("descending"));
    sortDescending->setText(i18nc("@action:inmenu Sort", "Descending"));
    connect(sortDescending, &KToggleAction::triggered, this, &DolphinViewActionHandler::toggleSortOrder);

    KToggleAction* sortFoldersFirst = m_actionCollection->add<KToggleAction>(QStringLiteral("folders_first"));
    sortFoldersFirst->setText(i18nc("@action:inmenu Sort", "Folders First"));
    connect(sortFoldersFirst, &KToggleAction::triggered, this, &DolphinViewActionHandler::toggleSortFoldersFirst);

    // View -> Sort By
    QActionGroup* sortByActionGroup = createFileItemRolesActionGroup(QStringLiteral("sort_by_"));

    KActionMenu* sortByActionMenu = m_actionCollection->add<KActionMenu>(QStringLiteral("sort"));
    sortByActionMenu->setText(i18nc("@action:inmenu View", "Sort By"));
    sortByActionMenu->setDelayed(false);

    foreach (QAction* action, sortByActionGroup->actions()) {
        sortByActionMenu->addAction(action);
    }
    sortByActionMenu->addSeparator();
    sortByActionMenu->addAction(sortDescending);
    sortByActionMenu->addAction(sortFoldersFirst);

    // View -> Additional Information
    QActionGroup* visibleRolesGroup = createFileItemRolesActionGroup(QStringLiteral("show_"));

    KActionMenu* visibleRolesMenu = m_actionCollection->add<KActionMenu>(QStringLiteral("additional_info"));
    visibleRolesMenu->setText(i18nc("@action:inmenu View", "Additional Information"));
    visibleRolesMenu->setDelayed(false);

    foreach (QAction* action, visibleRolesGroup->actions()) {
        visibleRolesMenu->addAction(action);
    }

    KToggleAction* showInGroups = m_actionCollection->add<KToggleAction>(QStringLiteral("show_in_groups"));
    showInGroups->setIcon(QIcon::fromTheme(QStringLiteral("view-group")));
    showInGroups->setText(i18nc("@action:inmenu View", "Show in Groups"));
    connect(showInGroups, &KToggleAction::triggered, this, &DolphinViewActionHandler::toggleGroupedSorting);

    KToggleAction* showHiddenFiles = m_actionCollection->add<KToggleAction>(QStringLiteral("show_hidden_files"));
    showHiddenFiles->setText(i18nc("@action:inmenu View", "Hidden Files"));
    showHiddenFiles->setToolTip(i18nc("@info", "Visibility of hidden files and folders"));
    m_actionCollection->setDefaultShortcuts(showHiddenFiles, {Qt::ALT + Qt::Key_Period, Qt::CTRL + Qt::Key_H, Qt::Key_F8});
    connect(showHiddenFiles, &KToggleAction::triggered, this, &DolphinViewActionHandler::toggleShowHiddenFiles);

    QAction* adjustViewProps = m_actionCollection->addAction(QStringLiteral("view_properties"));
    adjustViewProps->setText(i18nc("@action:inmenu View", "Adjust View Properties..."));
    connect(adjustViewProps, &QAction::triggered, this, &DolphinViewActionHandler::slotAdjustViewProperties);
}

QActionGroup* DolphinViewActionHandler::createFileItemRolesActionGroup(const QString& groupPrefix)
{
    const bool isSortGroup = (groupPrefix == QLatin1String("sort_by_"));
    Q_ASSERT(isSortGroup || (!isSortGroup && groupPrefix == QLatin1String("show_")));

    QActionGroup* rolesActionGroup = new QActionGroup(m_actionCollection);
    rolesActionGroup->setExclusive(isSortGroup);
    if (isSortGroup) {
        connect(rolesActionGroup, &QActionGroup::triggered,
                this, &DolphinViewActionHandler::slotSortTriggered);
    } else {
        connect(rolesActionGroup, &QActionGroup::triggered,
                this, &DolphinViewActionHandler::toggleVisibleRole);
    }

    QString groupName;
    KActionMenu* groupMenu = nullptr;
    QActionGroup* groupMenuGroup = nullptr;

    bool indexingEnabled = false;
#ifdef HAVE_BALOO
    Baloo::IndexerConfig config;
    indexingEnabled = config.fileIndexingEnabled();
#endif

    const QList<KFileItemModel::RoleInfo> rolesInfo = KFileItemModel::rolesInformation();
    foreach (const KFileItemModel::RoleInfo& info, rolesInfo) {
        if (!isSortGroup && info.role == "text") {
            // It should not be possible to hide the "text" role
            continue;
        }

        KToggleAction* action = nullptr;
        const QString name = groupPrefix + info.role;
        if (info.group.isEmpty()) {
            action = m_actionCollection->add<KToggleAction>(name);
            action->setActionGroup(rolesActionGroup);
        } else {
            if (!groupMenu || info.group != groupName) {
                groupName = info.group;
                groupMenu = m_actionCollection->add<KActionMenu>(groupName);
                groupMenu->setText(groupName);
                groupMenu->setActionGroup(rolesActionGroup);

                groupMenuGroup = new QActionGroup(groupMenu);
                groupMenuGroup->setExclusive(isSortGroup);
                if (isSortGroup) {
                    connect(groupMenuGroup, &QActionGroup::triggered,
                            this, &DolphinViewActionHandler::slotSortTriggered);
                } else {
                    connect(groupMenuGroup, &QActionGroup::triggered,
                            this, &DolphinViewActionHandler::toggleVisibleRole);
                }
            }

            action = new KToggleAction(groupMenu);
            action->setActionGroup(groupMenuGroup);
            groupMenu->addAction(action);
        }
        action->setText(info.translation);
        action->setData(info.role);

        const bool enable = (!info.requiresBaloo && !info.requiresIndexer) ||
                            (info.requiresBaloo) ||
                            (info.requiresIndexer && indexingEnabled);
        action->setEnabled(enable);

        if (isSortGroup) {
            m_sortByActions.insert(info.role, action);
        } else {
            m_visibleRoles.insert(info.role, action);
        }
    }

    return rolesActionGroup;
}

void DolphinViewActionHandler::slotViewModeActionTriggered(QAction* action)
{
    const DolphinView::Mode mode = action->data().value<DolphinView::Mode>();
    m_currentView->setMode(mode);

    QAction* viewModeMenu = m_actionCollection->action(QStringLiteral("view_mode"));
    viewModeMenu->setIcon(action->icon());
}

void DolphinViewActionHandler::slotRename()
{
    emit actionBeingHandled();
    m_currentView->renameSelectedItems();
}

void DolphinViewActionHandler::slotTrashActivated()
{
    emit actionBeingHandled();
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
        return QStringLiteral("icons");
    case DolphinView::DetailsView:
        return QStringLiteral("details");
    case DolphinView::CompactView:
        return QStringLiteral("compact");
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

        QAction* viewModeMenu = m_actionCollection->action(QStringLiteral("view_mode"));
        viewModeMenu->setIcon(viewModeAction->icon());
    }

    QAction* showPreviewAction = m_actionCollection->action(QStringLiteral("show_preview"));
    showPreviewAction->setChecked(m_currentView->previewsShown());

    slotSortOrderChanged(m_currentView->sortOrder());
    slotSortFoldersFirstChanged(m_currentView->sortFoldersFirst());
    slotVisibleRolesChanged(m_currentView->visibleRoles(), QList<QByteArray>());
    slotGroupedSortingChanged(m_currentView->groupedSorting());
    slotSortRoleChanged(m_currentView->sortRole());
    slotZoomLevelChanged(m_currentView->zoomLevel(), -1);

    // Updates the "show_hidden_files" action state and icon
    slotHiddenFilesShownChanged(m_currentView->hiddenFilesShown());
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
    QAction* descending = m_actionCollection->action(QStringLiteral("descending"));
    const bool sortDescending = (order == Qt::DescendingOrder);
    descending->setChecked(sortDescending);
}

void DolphinViewActionHandler::slotSortFoldersFirstChanged(bool foldersFirst)
{
    m_actionCollection->action(QStringLiteral("folders_first"))->setChecked(foldersFirst);
}

void DolphinViewActionHandler::toggleVisibleRole(QAction* action)
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

    const QSet<QByteArray> checkedRoles = current.toSet();
    QHashIterator<QByteArray, KToggleAction*> it(m_visibleRoles);
    while (it.hasNext()) {
        it.next();
        const QByteArray& role = it.key();
        KToggleAction* action = it.value();
        action->setChecked(checkedRoles.contains(role));
    }
}

void DolphinViewActionHandler::toggleGroupedSorting(bool grouped)
{
    m_currentView->setGroupedSorting(grouped);
}

void DolphinViewActionHandler::slotGroupedSortingChanged(bool groupedSorting)
{
    QAction* showInGroupsAction = m_actionCollection->action(QStringLiteral("show_in_groups"));
    showInGroupsAction->setChecked(groupedSorting);
}

void DolphinViewActionHandler::toggleShowHiddenFiles(bool show)
{
    emit actionBeingHandled();
    m_currentView->setHiddenFilesShown(show);
}

void DolphinViewActionHandler::slotHiddenFilesShownChanged(bool shown)
{
    QAction* showHiddenFilesAction = m_actionCollection->action(QStringLiteral("show_hidden_files"));
    showHiddenFilesAction->setChecked(shown);

    // #374508: don't overwrite custom icons.
    const QString iconName = showHiddenFilesAction->icon().name();
    if (!iconName.isEmpty() && iconName != QLatin1String("visibility") && iconName != QLatin1String("hint")) {
        return;
    }

    showHiddenFilesAction->setIcon(QIcon::fromTheme(shown ? QStringLiteral("visibility") : QStringLiteral("hint")));
}

void DolphinViewActionHandler::slotWriteStateChanged(bool isFolderWritable)
{
    m_actionCollection->action(QStringLiteral("create_dir"))->setEnabled(isFolderWritable &&
                                                                         KProtocolManager::supportsMakeDir(currentView()->url()));
}

KToggleAction* DolphinViewActionHandler::iconsModeAction()
{
    KToggleAction* iconsView = m_actionCollection->add<KToggleAction>(QStringLiteral("icons"));
    iconsView->setText(i18nc("@action:inmenu View Mode", "Icons"));
    iconsView->setToolTip(i18nc("@info", "Icons view mode"));
    m_actionCollection->setDefaultShortcut(iconsView, Qt::CTRL + Qt::Key_1);
    iconsView->setIcon(QIcon::fromTheme(QStringLiteral("view-list-icons")));
    iconsView->setData(QVariant::fromValue(DolphinView::IconsView));
    return iconsView;
}

KToggleAction* DolphinViewActionHandler::compactModeAction()
{
    KToggleAction* iconsView = m_actionCollection->add<KToggleAction>(QStringLiteral("compact"));
    iconsView->setText(i18nc("@action:inmenu View Mode", "Compact"));
    iconsView->setToolTip(i18nc("@info", "Compact view mode"));
    m_actionCollection->setDefaultShortcut(iconsView, Qt::CTRL + Qt::Key_2);
    iconsView->setIcon(QIcon::fromTheme(QStringLiteral("view-list-details"))); // TODO: discuss with Oxygen-team the wrong (?) name
    iconsView->setData(QVariant::fromValue(DolphinView::CompactView));
    return iconsView;
}

KToggleAction* DolphinViewActionHandler::detailsModeAction()
{
    KToggleAction* detailsView = m_actionCollection->add<KToggleAction>(QStringLiteral("details"));
    detailsView->setText(i18nc("@action:inmenu View Mode", "Details"));
    detailsView->setToolTip(i18nc("@info", "Details view mode"));
    m_actionCollection->setDefaultShortcut(detailsView, Qt::CTRL + Qt::Key_3);
    detailsView->setIcon(QIcon::fromTheme(QStringLiteral("view-list-tree")));
    detailsView->setData(QVariant::fromValue(DolphinView::DetailsView));
    return detailsView;
}

void DolphinViewActionHandler::slotSortRoleChanged(const QByteArray& role)
{
    KToggleAction* action = m_sortByActions.value(role);
    if (action) {
        action->setChecked(true);

        if (!action->icon().isNull()) {
            QAction* sortByMenu = m_actionCollection->action(QStringLiteral("sort"));
            sortByMenu->setIcon(action->icon());
        }
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
    // The radiobuttons of the "Sort By"-menu are split between the main-menu
    // and several sub-menus. Because of this they don't have a common
    // action-group that assures an exclusive toggle-state between the main-menu
    // actions and the sub-menu-actions. If an action gets checked, it must
    // be assured that all other actions get unchecked.
    QAction* sortByMenu =  m_actionCollection->action(QStringLiteral("sort"));
    foreach (QAction* groupAction, sortByMenu->menu()->actions()) {
        KActionMenu* actionMenu = qobject_cast<KActionMenu*>(groupAction);
        if (actionMenu) {
            foreach (QAction* subAction, actionMenu->menu()->actions()) {
                subAction->setChecked(false);
            }
        } else if (groupAction->actionGroup()) {
            groupAction->setChecked(false);
        }
    }
    action->setChecked(true);

    // Apply the activated sort-role to the view
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
    KPropertiesDialog* dialog = nullptr;
    const KFileItemList list = m_currentView->selectedItems();
    if (list.isEmpty()) {
        const QUrl url = m_currentView->url();
        dialog = new KPropertiesDialog(url, m_currentView);
    } else {
        dialog = new KPropertiesDialog(list, m_currentView);
    }

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}
