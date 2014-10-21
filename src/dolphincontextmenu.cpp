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

#include "dolphinmainwindow.h"
#include "dolphinnewfilemenu.h"
#include "dolphinviewcontainer.h"
#include "dolphin_generalsettings.h"
#include "dolphinremoveaction.h"

#include <KActionCollection>
#include <kfileitemactionplugin.h>
#include <kabstractfileitemactionplugin.h>
#include <KFileItemActions>
#include <KFileItemListProperties>
#include <KIO/RestoreJob>
#include <KIO/EmptyTrashJob>
#include <KIO/JobUiDelegate>
#include <KIO/Paste>
#include <KJobWidgets>
#include <KMimeTypeTrader>
#include <KNewFileMenu>
#include <konq_operations.h>
#include <KService>
#include <KLocalizedString>
#include <KStandardAction>
#include <KToolBar>

#include <QApplication>
#include <QClipboard>
#include <QMenuBar>
#include <QMenu>

#include <panels/places/placesitem.h>
#include <panels/places/placesitemmodel.h>


#include "views/dolphinview.h"
#include "views/viewmodecontroller.h"

DolphinContextMenu::DolphinContextMenu(DolphinMainWindow* parent,
                                       const QPoint& pos,
                                       const KFileItem& fileInfo,
                                       const QUrl& baseUrl) :
    QMenu(parent),
    m_pos(pos),
    m_mainWindow(parent),
    m_fileInfo(fileInfo),
    m_baseUrl(baseUrl),
    m_baseFileItem(0),
    m_selectedItems(),
    m_selectedItemsProperties(0),
    m_context(NoContext),
    m_copyToMenu(parent),
    m_customActions(),
    m_command(None),
    m_removeAction(0)
{
    // The context menu either accesses the URLs of the selected items
    // or the items itself. To increase the performance both lists are cached.
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();
    m_selectedItems = view->selectedItems();
}

DolphinContextMenu::~DolphinContextMenu()
{
    delete m_selectedItemsProperties;
    m_selectedItemsProperties = 0;
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
        m_removeAction->update();
    }
    QMenu::keyPressEvent(ev);
}

void DolphinContextMenu::keyReleaseEvent(QKeyEvent *ev)
{
    if (m_removeAction && ev->key() == Qt::Key_Shift) {
        m_removeAction->update();
    }
    QMenu::keyReleaseEvent(ev);
}

void DolphinContextMenu::openTrashContextMenu()
{
    Q_ASSERT(m_context & TrashContext);

    QAction* emptyTrashAction = new QAction(QIcon::fromTheme("trash-empty"), i18nc("@action:inmenu", "Empty Trash"), this);
    KConfig trashConfig("trashrc", KConfig::SimpleConfig);
    emptyTrashAction->setEnabled(!trashConfig.group("Status").readEntry("Empty", true));
    addAction(emptyTrashAction);

    addCustomActions();

    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    addAction(propertiesAction);

    addShowMenuBarAction();

    if (exec(m_pos) == emptyTrashAction) {
        KIO::JobUiDelegate uiDelegate;
        uiDelegate.setWindow(m_mainWindow);
        if (uiDelegate.askDeleteConfirmation(QList<QUrl>(), KIO::JobUiDelegate::EmptyTrash, KIO::JobUiDelegate::DefaultConfirmation)) {
            KIO::Job* job = KIO::emptyTrash();
            KJobWidgets::setWindow(job, m_mainWindow);
            job->ui()->setAutoErrorHandlingEnabled(true);
        }
    }
}

void DolphinContextMenu::openTrashItemContextMenu()
{
    Q_ASSERT(m_context & TrashContext);
    Q_ASSERT(m_context & ItemContext);

    QAction* restoreAction = new QAction(i18nc("@action:inmenu", "Restore"), m_mainWindow);
    addAction(restoreAction);

    QAction* deleteAction = m_mainWindow->actionCollection()->action("delete");
    addAction(deleteAction);

    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    addAction(propertiesAction);

    if (exec(m_pos) == restoreAction) {
        QList<QUrl> selectedUrls;
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

    QAction* openParentAction = 0;
    QAction* openParentInNewWindowAction = 0;
    QAction* openParentInNewTabAction = 0;
    QAction* addToPlacesAction = 0;
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
            menu->setIcon(QIcon::fromTheme("document-new"));
            addMenu(menu);
            addSeparator();

            // insert 'Open in new window' and 'Open in new tab' entries
            addAction(m_mainWindow->actionCollection()->action("open_in_new_window"));
            addAction(m_mainWindow->actionCollection()->action("open_in_new_tab"));

            // insert 'Add to Places' entry
            if (!placeExists(m_fileInfo.url())) {
                addToPlacesAction = addAction(QIcon::fromTheme("bookmark-new"),
                                                       i18nc("@action:inmenu Add selected folder to places",
                                                             "Add to Places"));
            }

            addSeparator();
        } else if (m_baseUrl.scheme().contains("search") || m_baseUrl.scheme().contains("timeline")) {
            openParentAction = new QAction(QIcon::fromTheme("document-open-folder"),
                                           i18nc("@action:inmenu",
                                                 "Open Path"),
                                           this);
            addAction(openParentAction);

            openParentInNewWindowAction = new QAction(QIcon::fromTheme("window-new"),
                                                    i18nc("@action:inmenu",
                                                          "Open Path in New Window"),
                                                    this);
            addAction(openParentInNewWindowAction);

            openParentInNewTabAction = new QAction(QIcon::fromTheme("tab-new"),
                                                   i18nc("@action:inmenu",
                                                         "Open Path in New Tab"),
                                                   this);
            addAction(openParentInNewTabAction);

            addSeparator();
        } else if (!DolphinView::openItemAsFolderUrl(m_fileInfo).isEmpty()) {
            // insert 'Open in new window' and 'Open in new tab' entries
            addAction(m_mainWindow->actionCollection()->action("open_in_new_window"));
            addAction(m_mainWindow->actionCollection()->action("open_in_new_tab"));

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
            addAction(m_mainWindow->actionCollection()->action("open_in_new_tabs"));
            addSeparator();
        }
    }

    insertDefaultItemActions(selectedItemsProps);

    addSeparator();

    KFileItemActions fileItemActions;
    fileItemActions.setItemListProperties(selectedItemsProps);
    addServiceActions(fileItemActions);

    addFileItemPluginActions();

    addVersionControlPluginActions();

    // insert 'Copy To' and 'Move To' sub menus
    if (GeneralSettings::showCopyMoveMenu()) {
        m_copyToMenu.setItems(m_selectedItems);
        m_copyToMenu.setReadOnly(!selectedItemsProps.supportsWriting());
        m_copyToMenu.addActionsTo(this);
    }

    // insert 'Properties...' entry
    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    addAction(propertiesAction);

    QAction* activatedAction = exec(m_pos);
    if (activatedAction) {
        if (activatedAction == addToPlacesAction) {
            const QUrl selectedUrl(m_fileInfo.url());
            if (selectedUrl.isValid()) {
                PlacesItemModel model;
                const QString text = selectedUrl.fileName();
                PlacesItem* item = model.createPlacesItem(text, selectedUrl);
                model.appendItemToGroup(item);
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
    addAction(m_mainWindow->actionCollection()->action("new_window"));
    addAction(m_mainWindow->actionCollection()->action("new_tab"));

    // Insert 'Add to Places' entry if exactly one item is selected
    QAction* addToPlacesAction = 0;
    if (!placeExists(m_mainWindow->activeViewContainer()->url())) {
        addToPlacesAction = addAction(QIcon::fromTheme("bookmark-new"),
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

    addFileItemPluginActions();

    addVersionControlPluginActions();

    addCustomActions();

    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    addAction(propertiesAction);

    addShowMenuBarAction();

    QAction* action = exec(m_pos);
    if (addToPlacesAction && (action == addToPlacesAction)) {
        const DolphinViewContainer* container =  m_mainWindow->activeViewContainer();
        if (container->url().isValid()) {
            PlacesItemModel model;
            PlacesItem* item = model.createPlacesItem(container->placesText(),
                                                      container->url());
            model.appendItemToGroup(item);
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
    QAction* renameAction = collection->action("rename");
    addAction(renameAction);

    // Insert 'Move to Trash' and/or 'Delete'
    if (properties.supportsDeleting()) {
        const bool showDeleteAction = (KSharedConfig::openConfig()->group("KDE").readEntry("ShowDeleteCommand", false) ||
                                       !properties.isLocal());
        const bool showMoveToTrashAction = (properties.isLocal() &&
                                            properties.supportsMoving());

        if (showDeleteAction && showMoveToTrashAction) {
            delete m_removeAction;
            m_removeAction = 0;
            addAction(m_mainWindow->actionCollection()->action("move_to_trash"));
            addAction(m_mainWindow->actionCollection()->action("delete"));
        } else if (showDeleteAction && !showMoveToTrashAction) {
            addAction(m_mainWindow->actionCollection()->action("delete"));
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
    PlacesItemModel model;

    const int count = model.count();
    for (int i = 0; i < count; ++i) {
        const QUrl placeUrl = model.placesItem(i)->url();
        if (placeUrl.matches(url, QUrl::StripTrailingSlash)) {
            return true;
        }
    }

    return false;
}

QAction* DolphinContextMenu::createPasteAction()
{
    QAction* action = 0;
    const bool isDir = !m_fileInfo.isNull() && m_fileInfo.isDir();
    if (isDir && (m_selectedItems.count() == 1)) {
        const QMimeData *mimeData = QApplication::clipboard()->mimeData();
        bool canPaste;
        const QString text = KIO::pasteActionText(mimeData, &canPaste, m_fileInfo);
        action = new QAction(QIcon::fromTheme("edit-paste"), text, this);
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
        m_baseFileItem = new KFileItem(KFileItem::Unknown, KFileItem::Unknown, m_baseUrl);
    }
    return *m_baseFileItem;
}

void DolphinContextMenu::addServiceActions(KFileItemActions& fileItemActions)
{
    fileItemActions.setParentWidget(m_mainWindow);

    // insert 'Open With...' action or sub menu
    fileItemActions.addOpenWithActionsTo(this, "DesktopEntryName != 'dolphin'");

    // insert 'Actions' sub menu
    fileItemActions.addServiceActionsTo(this);
}

void DolphinContextMenu::addFileItemPluginActions()
{
    KFileItemListProperties props;
    if (m_selectedItems.isEmpty()) {
        props.setItems(KFileItemList() << baseFileItem());
    } else {
        props = selectedItemsProperties();
    }

    QString commonMimeType = props.mimeType();
    if (commonMimeType.isEmpty()) {
        commonMimeType = QLatin1String("application/octet-stream");
    }

    const KService::List pluginServices = KMimeTypeTrader::self()->query(commonMimeType, "KFileItemAction/Plugin", "exist Library");
    if (pluginServices.isEmpty()) {
        return;
    }

    const KConfig config("kservicemenurc", KConfig::NoGlobals);
    const KConfigGroup showGroup = config.group("Show");

    foreach (const KService::Ptr& service, pluginServices) {
        if (!showGroup.readEntry(service->desktopEntryName(), true)) {
            // The plugin has been disabled
            continue;
        }

        // Old API (kdelibs-4.6.0 only)
        KFileItemActionPlugin* plugin = service->createInstance<KFileItemActionPlugin>();
        if (plugin) {
            plugin->setParent(this);
            addActions(plugin->actions(props, m_mainWindow));
        }
        // New API (kdelibs >= 4.6.1)
        KAbstractFileItemActionPlugin* abstractPlugin = service->createInstance<KAbstractFileItemActionPlugin>();
        if (abstractPlugin) {
            abstractPlugin->setParent(this);
            addActions(abstractPlugin->actions(props, m_mainWindow));
        }
    }
}

void DolphinContextMenu::addVersionControlPluginActions()
{
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();
    const QList<QAction*> versionControlActions = view->versionControlActions(m_selectedItems);
    if (!versionControlActions.isEmpty()) {
        foreach (QAction* action, versionControlActions) {
            addAction(action);
        }
        addSeparator();
    }
}

void DolphinContextMenu::addCustomActions()
{
    foreach (QAction* action, m_customActions) {
        addAction(action);
    }
}

