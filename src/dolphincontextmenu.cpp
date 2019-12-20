  /***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at) and              *
 *   Cvetoslav Ludmiloff                                                   *
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

#include "dolphincontextmenu.h"

#include "dolphin_generalsettings.h"
#include "dolphinmainwindow.h"
#include "dolphinnewfilemenu.h"
#include "dolphinplacesmodelsingleton.h"
#include "dolphinremoveaction.h"
#include "dolphinviewcontainer.h"
#include "panels/places/placesitem.h"
#include "panels/places/placesitemmodel.h"
#include "trash/dolphintrash.h"
#include "views/dolphinview.h"
#include "views/viewmodecontroller.h"

#include <KActionCollection>
#include <KFileItemActions>
#include <KFileItemListProperties>
#include <KIO/EmptyTrashJob>
#include <KIO/JobUiDelegate>
#include <KIO/Paste>
#include <KIO/RestoreJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KNewFileMenu>
#include <KPluginMetaData>
#include <KService>
#include <KStandardAction>
#include <KToolBar>

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QMimeDatabase>

DolphinContextMenu::DolphinContextMenu(DolphinMainWindow* parent,
                                       const QPoint& pos,
                                       const KFileItem& fileInfo,
                                       const QUrl& baseUrl) :
    QMenu(parent),
    m_pos(pos),
    m_mainWindow(parent),
    m_fileInfo(fileInfo),
    m_baseUrl(baseUrl),
    m_baseFileItem(nullptr),
    m_selectedItems(),
    m_selectedItemsProperties(nullptr),
    m_context(NoContext),
    m_copyToMenu(parent),
    m_customActions(),
    m_command(None),
    m_removeAction(nullptr)
{
    // The context menu either accesses the URLs of the selected items
    // or the items itself. To increase the performance both lists are cached.
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();
    m_selectedItems = view->selectedItems();
}

DolphinContextMenu::~DolphinContextMenu()
{
    delete m_baseFileItem;
    m_baseFileItem = nullptr;
    delete m_selectedItemsProperties;
    m_selectedItemsProperties = nullptr;
}

void DolphinContextMenu::setCustomActions(const QList<QAction*>& actions)
{
    m_customActions = actions;
}

DolphinContextMenu::Command DolphinContextMenu::open()
{
    // get the context information
    const auto scheme = m_baseUrl.scheme();
    if (scheme == QLatin1String("trash")) {
        m_context |= TrashContext;
    } else if (scheme.contains(QLatin1String("search"))) {
        m_context |= SearchContext;
    } else if (scheme.contains(QLatin1String("timeline"))) {
        m_context |= TimelineContext;
    }

    if (!m_fileInfo.isNull() && !m_selectedItems.isEmpty()) {
        m_context |= ItemContext;
        // TODO: handle other use cases like devices + desktop files
    }

    // open the corresponding popup for the context
    if (m_context & TrashContext) {
        if (m_context & ItemContext) {
            openTrashItemContextMenu();
        } else {
            openTrashContextMenu();
        }
    } else if (m_context & ItemContext) {
        openItemContextMenu();
    } else {
        Q_ASSERT(m_context == NoContext);
        openViewportContextMenu();
    }

    return m_command;
}

void DolphinContextMenu::keyPressEvent(QKeyEvent *ev)
{
    if (m_removeAction && ev->key() == Qt::Key_Shift) {
        m_removeAction->update(DolphinRemoveAction::ShiftState::Pressed);
    }
    QMenu::keyPressEvent(ev);
}

void DolphinContextMenu::keyReleaseEvent(QKeyEvent *ev)
{
    if (m_removeAction && ev->key() == Qt::Key_Shift) {
        m_removeAction->update(DolphinRemoveAction::ShiftState::Released);
    }
    QMenu::keyReleaseEvent(ev);
}

void DolphinContextMenu::openTrashContextMenu()
{
    Q_ASSERT(m_context & TrashContext);

    QAction* emptyTrashAction = new QAction(QIcon::fromTheme(QStringLiteral("trash-empty")), i18nc("@action:inmenu", "Empty Trash"), this);
    emptyTrashAction->setEnabled(!Trash::isEmpty());
    addAction(emptyTrashAction);

    addCustomActions();

    QAction* propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);

    addShowMenuBarAction();

    if (exec(m_pos) == emptyTrashAction) {
        Trash::empty(m_mainWindow);
    }
}

void DolphinContextMenu::openTrashItemContextMenu()
{
    Q_ASSERT(m_context & TrashContext);
    Q_ASSERT(m_context & ItemContext);

    QAction* restoreAction = new QAction(QIcon::fromTheme("restoration"), i18nc("@action:inmenu", "Restore"), m_mainWindow);
    addAction(restoreAction);

    QAction* deleteAction = m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::DeleteFile));
    addAction(deleteAction);

    QAction* propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);

    if (exec(m_pos) == restoreAction) {
        QList<QUrl> selectedUrls;
        selectedUrls.reserve(m_selectedItems.count());
        foreach (const KFileItem &item, m_selectedItems) {
            selectedUrls.append(item.url());
        }

        KIO::RestoreJob *job = KIO::restoreFromTrash(selectedUrls);
        KJobWidgets::setWindow(job, m_mainWindow);
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
}

void DolphinContextMenu::addDirectoryItemContextMenu(KFileItemActions &fileItemActions)
{
    // insert 'Open in new window' and 'Open in new tab' entries

    const KFileItemListProperties& selectedItemsProps = selectedItemsProperties();

    addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_new_tab")));
    addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_new_window")));

    // Insert 'Open With' entries
    addOpenWithActions(fileItemActions);

    // set up 'Create New' menu
     DolphinNewFileMenu* newFileMenu = new DolphinNewFileMenu(m_mainWindow->actionCollection(), m_mainWindow);
     const DolphinView* view = m_mainWindow->activeViewContainer()->view();
     newFileMenu->setViewShowsHiddenFiles(view->hiddenFilesShown());
     newFileMenu->checkUpToDate();
     newFileMenu->setPopupFiles(QList<QUrl>() << m_fileInfo.url());
     newFileMenu->setEnabled(selectedItemsProps.supportsWriting());
     connect(newFileMenu, &DolphinNewFileMenu::fileCreated, newFileMenu, &DolphinNewFileMenu::deleteLater);
     connect(newFileMenu, &DolphinNewFileMenu::directoryCreated, newFileMenu, &DolphinNewFileMenu::deleteLater);

     QMenu* menu = newFileMenu->menu();
     menu->setTitle(i18nc("@title:menu Create new folder, file, link, etc.", "Create New"));
     menu->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
     addMenu(menu);

     addSeparator();
}

void DolphinContextMenu::openItemContextMenu()
{
    Q_ASSERT(!m_fileInfo.isNull());

    QAction* openParentAction = nullptr;
    QAction* openParentInNewWindowAction = nullptr;
    QAction* openParentInNewTabAction = nullptr;
    const KFileItemListProperties& selectedItemsProps = selectedItemsProperties();

    KFileItemActions fileItemActions;
    fileItemActions.setParentWidget(m_mainWindow);
    fileItemActions.setItemListProperties(selectedItemsProps);

    if (m_selectedItems.count() == 1) {
        // single files
        if (m_fileInfo.isDir()) {
            addDirectoryItemContextMenu(fileItemActions);
        } else if (m_context & TimelineContext || m_context & SearchContext) {
            addOpenWithActions(fileItemActions);

            openParentAction = new QAction(QIcon::fromTheme(QStringLiteral("document-open-folder")),
                                           i18nc("@action:inmenu",
                                                 "Open Path"),
                                           this);
            addAction(openParentAction);

            openParentInNewWindowAction = new QAction(QIcon::fromTheme(QStringLiteral("window-new")),
                                                    i18nc("@action:inmenu",
                                                          "Open Path in New Window"),
                                                    this);
            addAction(openParentInNewWindowAction);

            openParentInNewTabAction = new QAction(QIcon::fromTheme(QStringLiteral("tab-new")),
                                                   i18nc("@action:inmenu",
                                                         "Open Path in New Tab"),
                                                   this);
            addAction(openParentInNewTabAction);

            addSeparator();
        } else {
            // Insert 'Open With" entries
            addOpenWithActions(fileItemActions);
        }
        if (m_fileInfo.isLink()) {
            addAction(m_mainWindow->actionCollection()->action(QStringLiteral("show_target")));
            addSeparator();
        }
    } else {
        // multiple files
        bool selectionHasOnlyDirs = true;
        for (const auto &item : qAsConst(m_selectedItems)) {
            const QUrl& url = DolphinView::openItemAsFolderUrl(item);
            if (url.isEmpty()) {
                selectionHasOnlyDirs = false;
                break;
            }
        }

        if (selectionHasOnlyDirs) {
            // insert 'Open in new tab' entry
            addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_new_tabs")));
        }
        // Insert 'Open With" entries
        addOpenWithActions(fileItemActions);
    }

    insertDefaultItemActions(selectedItemsProps);

    // insert 'Add to Places' entry if appropriate
    if (m_selectedItems.count() == 1) {
        if (m_fileInfo.isDir()) {
            if (!placeExists(m_fileInfo.url())) {
                addAction(m_mainWindow->actionCollection()->action(QStringLiteral("add_to_places")));
            }
        }
    }

    addSeparator();

    fileItemActions.addServiceActionsTo(this);
    fileItemActions.addPluginActionsTo(this);

    addVersionControlPluginActions();

    // insert 'Copy To' and 'Move To' sub menus
    if (GeneralSettings::showCopyMoveMenu()) {
        m_copyToMenu.setUrls(m_selectedItems.urlList());
        m_copyToMenu.setReadOnly(!selectedItemsProps.supportsWriting());
        m_copyToMenu.setAutoErrorHandlingEnabled(true);
        m_copyToMenu.addActionsTo(this);
    }

    // insert 'Properties...' entry
    addSeparator();
    QAction* propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);

    QAction* activatedAction = exec(m_pos);
    if (activatedAction) {
        if (activatedAction == openParentAction) {
            m_command = OpenParentFolder;
        } else if (activatedAction == openParentInNewWindowAction) {
            m_command = OpenParentFolderInNewWindow;
        } else if (activatedAction == openParentInNewTabAction) {
            m_command = OpenParentFolderInNewTab;
        }
    }
}

void DolphinContextMenu::openViewportContextMenu()
{
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();

    // Insert 'Open With' entries
    KFileItem baseItem = view->rootItem();
    if (baseItem.isNull() || baseItem.url() != m_baseUrl) {
        baseItem = baseFileItem();
    }

    const KFileItemListProperties baseUrlProperties(KFileItemList() << baseItem);
    KFileItemActions fileItemActions;
    fileItemActions.setParentWidget(m_mainWindow);
    fileItemActions.setItemListProperties(baseUrlProperties);

    // Set up and insert 'Create New' menu
    KNewFileMenu* newFileMenu = m_mainWindow->newFileMenu();
    newFileMenu->setViewShowsHiddenFiles(view->hiddenFilesShown());
    newFileMenu->checkUpToDate();
    newFileMenu->setPopupFiles(QList<QUrl>() << m_baseUrl);
    addMenu(newFileMenu->menu());

    // Don't show "Open With" menu items if the current dir is empty, because there's
    // generally no app that can do anything interesting with an empty directory
    if (view->itemsCount() != 0) {
        addOpenWithActions(fileItemActions);
    }

    QAction* pasteAction = createPasteAction();
    addAction(pasteAction);

    // Insert 'Add to Places' entry if it's not already in the places panel
    if (!placeExists(m_mainWindow->activeViewContainer()->url())) {
        addAction(m_mainWindow->actionCollection()->action(QStringLiteral("add_to_places")));
    }
    addSeparator();

    // Insert 'Sort By' and 'View Mode'
    addAction(m_mainWindow->actionCollection()->action(QStringLiteral("sort")));
    addAction(m_mainWindow->actionCollection()->action(QStringLiteral("view_mode")));

    addSeparator();

    // Insert service actions
    fileItemActions.addServiceActionsTo(this);
    fileItemActions.addPluginActionsTo(this);

    addVersionControlPluginActions();

    addCustomActions();

    addSeparator();

    QAction* propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);

    addShowMenuBarAction();

    exec(m_pos);
}

void DolphinContextMenu::insertDefaultItemActions(const KFileItemListProperties& properties)
{
    const KActionCollection* collection = m_mainWindow->actionCollection();

    // Insert 'Cut', 'Copy' and 'Paste'
    addAction(collection->action(KStandardAction::name(KStandardAction::Cut)));
    addAction(collection->action(KStandardAction::name(KStandardAction::Copy)));
    addAction(createPasteAction());
    addAction(m_mainWindow->actionCollection()->action(QStringLiteral("duplicate")));

    addSeparator();

    // Insert 'Rename'
    addAction(collection->action(KStandardAction::name(KStandardAction::RenameFile)));

    // Insert 'Move to Trash' and/or 'Delete'
    const bool showDeleteAction = (KSharedConfig::openConfig()->group("KDE").readEntry("ShowDeleteCommand", false) ||
                                    !properties.isLocal());
    const bool showMoveToTrashAction = (properties.isLocal() &&
                                        properties.supportsMoving());

    if (showDeleteAction && showMoveToTrashAction) {
        delete m_removeAction;
        m_removeAction = nullptr;
        addAction(m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::MoveToTrash)));
        addAction(m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::DeleteFile)));
    } else if (showDeleteAction && !showMoveToTrashAction) {
        addAction(m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::DeleteFile)));
    } else {
        if (!m_removeAction) {
            m_removeAction = new DolphinRemoveAction(this, m_mainWindow->actionCollection());
        }
        addAction(m_removeAction);
        m_removeAction->update();
    }
}

void DolphinContextMenu::addShowMenuBarAction()
{
    const KActionCollection* ac = m_mainWindow->actionCollection();
    QAction* showMenuBar = ac->action(KStandardAction::name(KStandardAction::ShowMenubar));
    if (!m_mainWindow->menuBar()->isVisible() && !m_mainWindow->toolBar()->isVisible()) {
        addSeparator();
        addAction(showMenuBar);
    }
}

bool DolphinContextMenu::placeExists(const QUrl& url) const
{
    const KFilePlacesModel* placesModel = DolphinPlacesModelSingleton::instance().placesModel();

    const auto& matchedPlaces = placesModel->match(placesModel->index(0,0), KFilePlacesModel::UrlRole, url, 1, Qt::MatchExactly);

    return !matchedPlaces.isEmpty();
}

QAction* DolphinContextMenu::createPasteAction()
{
    QAction* action = nullptr;
    const bool isDir = !m_fileInfo.isNull() && m_fileInfo.isDir();
    if (isDir && (m_selectedItems.count() == 1)) {
        const QMimeData *mimeData = QApplication::clipboard()->mimeData();
        bool canPaste;
        const QString text = KIO::pasteActionText(mimeData, &canPaste, m_fileInfo);
        action = new QAction(QIcon::fromTheme(QStringLiteral("edit-paste")), text, this);
        action->setEnabled(canPaste);
        connect(action, &QAction::triggered, m_mainWindow, &DolphinMainWindow::pasteIntoFolder);
    } else {
        action = m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::Paste));
    }

    return action;
}

KFileItemListProperties& DolphinContextMenu::selectedItemsProperties() const
{
    if (!m_selectedItemsProperties) {
        m_selectedItemsProperties = new KFileItemListProperties(m_selectedItems);
    }
    return *m_selectedItemsProperties;
}

KFileItem DolphinContextMenu::baseFileItem()
{
    if (!m_baseFileItem) {
        m_baseFileItem = new KFileItem(m_baseUrl);
    }
    return *m_baseFileItem;
}

void DolphinContextMenu::addOpenWithActions(KFileItemActions& fileItemActions)
{
    // insert 'Open With...' action or sub menu
    fileItemActions.addOpenWithActionsTo(this, QStringLiteral("DesktopEntryName != '%1'").arg(qApp->desktopFileName()));
}

void DolphinContextMenu::addVersionControlPluginActions()
{
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();
    const QList<QAction*> versionControlActions = view->versionControlActions(m_selectedItems);
    if (!versionControlActions.isEmpty()) {
        addActions(versionControlActions);
        addSeparator();
    }
}

void DolphinContextMenu::addCustomActions()
{
    addActions(m_customActions);
}

