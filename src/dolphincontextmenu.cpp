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
#include "dolphinremoveaction.h"
#include "dolphinviewcontainer.h"
#include "panels/places/placesitem.h"
#include "panels/places/placesitemmodel.h"
#include "trash/dolphintrash.h"
#include "views/dolphinview.h"
#include "views/viewmodecontroller.h"

#include <KAbstractFileItemActionPlugin>
#include <KActionCollection>
#include <KFileItemActions>
#include <KFileItemListProperties>
#include <KIO/EmptyTrashJob>
#include <KIO/JobUiDelegate>
#include <KIO/Paste>
#include <KIO/RestoreJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMimeTypeTrader>
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
    if (m_baseUrl.scheme() == QLatin1String("trash")) {
        m_context |= TrashContext;
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

    QAction* restoreAction = new QAction(i18nc("@action:inmenu", "Restore"), m_mainWindow);
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

void DolphinContextMenu::openItemContextMenu()
{
    Q_ASSERT(!m_fileInfo.isNull());

    QAction* openParentAction = nullptr;
    QAction* openParentInNewWindowAction = nullptr;
    QAction* openParentInNewTabAction = nullptr;
    QAction* addToPlacesAction = nullptr;
    const KFileItemListProperties& selectedItemsProps = selectedItemsProperties();

    if (m_selectedItems.count() == 1) {
        if (m_fileInfo.isDir()) {
            // setup 'Create New' menu
            DolphinNewFileMenu* newFileMenu = new DolphinNewFileMenu(m_mainWindow->actionCollection(), m_mainWindow);
            const DolphinView* view = m_mainWindow->activeViewContainer()->view();
            newFileMenu->setViewShowsHiddenFiles(view->hiddenFilesShown());
            newFileMenu->checkUpToDate();
            newFileMenu->setPopupFiles(m_fileInfo.url());
            newFileMenu->setEnabled(selectedItemsProps.supportsWriting());
            connect(newFileMenu, &DolphinNewFileMenu::fileCreated, newFileMenu, &DolphinNewFileMenu::deleteLater);
            connect(newFileMenu, &DolphinNewFileMenu::directoryCreated, newFileMenu, &DolphinNewFileMenu::deleteLater);

            QMenu* menu = newFileMenu->menu();
            menu->setTitle(i18nc("@title:menu Create new folder, file, link, etc.", "Create New"));
            menu->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
            addMenu(menu);
            addSeparator();

            // insert 'Open in new window' and 'Open in new tab' entries
            addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_new_window")));
            addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_new_tab")));

            // insert 'Add to Places' entry
            if (!placeExists(m_fileInfo.url())) {
                addToPlacesAction = addAction(QIcon::fromTheme(QStringLiteral("bookmark-new")),
                                                       i18nc("@action:inmenu Add selected folder to places",
                                                             "Add to Places"));
            }

            addSeparator();
        } else if (m_baseUrl.scheme().contains(QStringLiteral("search")) || m_baseUrl.scheme().contains(QStringLiteral("timeline"))) {
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
        } else if (!DolphinView::openItemAsFolderUrl(m_fileInfo).isEmpty()) {
            // insert 'Open in new window' and 'Open in new tab' entries
            addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_new_window")));
            addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_new_tab")));

            addSeparator();
        }
    } else {
        bool selectionHasOnlyDirs = true;
        foreach (const KFileItem& item, m_selectedItems) {
            const QUrl& url = DolphinView::openItemAsFolderUrl(item);
            if (url.isEmpty()) {
                selectionHasOnlyDirs = false;
                break;
            }
        }

        if (selectionHasOnlyDirs) {
            // insert 'Open in new tab' entry
            addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_new_tabs")));
            addSeparator();
        }
    }

    insertDefaultItemActions(selectedItemsProps);

    addSeparator();

    KFileItemActions fileItemActions;
    fileItemActions.setItemListProperties(selectedItemsProps);
    addServiceActions(fileItemActions);

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
    QAction* propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);

    QAction* activatedAction = exec(m_pos);
    if (activatedAction) {
        if (activatedAction == addToPlacesAction) {
            const QUrl selectedUrl(m_fileInfo.url());
            if (selectedUrl.isValid()) {
                PlacesItemModel model;
                const QString text = selectedUrl.fileName();
                model.createPlacesItem(text, selectedUrl, KIO::iconNameForUrl(selectedUrl));
            }
        } else if (activatedAction == openParentAction) {
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
    // setup 'Create New' menu
    KNewFileMenu* newFileMenu = m_mainWindow->newFileMenu();
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();
    newFileMenu->setViewShowsHiddenFiles(view->hiddenFilesShown());
    newFileMenu->checkUpToDate();
    newFileMenu->setPopupFiles(m_baseUrl);
    addMenu(newFileMenu->menu());
    addSeparator();

    // Insert 'New Window' and 'New Tab' entries. Don't use "open_in_new_window" and
    // "open_in_new_tab" here, as the current selection should get ignored.
    addAction(m_mainWindow->actionCollection()->action(QStringLiteral("new_window")));
    addAction(m_mainWindow->actionCollection()->action(QStringLiteral("new_tab")));

    // Insert 'Add to Places' entry if exactly one item is selected
    QAction* addToPlacesAction = nullptr;
    if (!placeExists(m_mainWindow->activeViewContainer()->url())) {
        addToPlacesAction = addAction(QIcon::fromTheme(QStringLiteral("bookmark-new")),
                                             i18nc("@action:inmenu Add current folder to places", "Add to Places"));
    }

    addSeparator();

    QAction* pasteAction = createPasteAction();
    addAction(pasteAction);
    addSeparator();

    // Insert service actions
    const KFileItemListProperties baseUrlProperties(KFileItemList() << baseFileItem());
    KFileItemActions fileItemActions;
    fileItemActions.setItemListProperties(baseUrlProperties);
    addServiceActions(fileItemActions);

    fileItemActions.addPluginActionsTo(this);

    addVersionControlPluginActions();

    addCustomActions();

    QAction* propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);

    addShowMenuBarAction();

    QAction* action = exec(m_pos);
    if (addToPlacesAction && (action == addToPlacesAction)) {
        const DolphinViewContainer* container =  m_mainWindow->activeViewContainer();
        const QUrl url = container->url();
        if (url.isValid()) {
            PlacesItemModel model;
            QString icon;
            if (container->isSearchModeEnabled()) {
                icon = QStringLiteral("folder-saved-search-symbolic");
            } else {
                icon = KIO::iconNameForUrl(url);
            }
            model.createPlacesItem(container->placesText(), url, icon);
        }
    }
}

void DolphinContextMenu::insertDefaultItemActions(const KFileItemListProperties& properties)
{
    const KActionCollection* collection = m_mainWindow->actionCollection();

    // Insert 'Cut', 'Copy' and 'Paste'
    addAction(collection->action(KStandardAction::name(KStandardAction::Cut)));
    addAction(collection->action(KStandardAction::name(KStandardAction::Copy)));
    addAction(createPasteAction());

    addSeparator();

    // Insert 'Rename'
    addAction(collection->action(KStandardAction::name(KStandardAction::RenameFile)));

    // Insert 'Move to Trash' and/or 'Delete'
    if (properties.supportsDeleting()) {
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
    Q_UNUSED(url)
    // Creating up a PlacesItemModel to find out if 'url' is one of the Places
    // can be expensive because the model asks Solid for the devices which are
    // available, which can take some time.
    // TODO: Consider restoring this check if the handling of Places and devices
    // will be decoupled in the future.
    return false;
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

void DolphinContextMenu::addServiceActions(KFileItemActions& fileItemActions)
{
    fileItemActions.setParentWidget(m_mainWindow);

    // insert 'Open With...' action or sub menu
    fileItemActions.addOpenWithActionsTo(this, QStringLiteral("DesktopEntryName != 'dolphin'"));

    // insert 'Actions' sub menu
    fileItemActions.addServiceActionsTo(this);
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

