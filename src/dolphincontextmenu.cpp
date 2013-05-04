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

#include <KActionCollection>
#include <KDesktopFile>
#include <kfileitemactionplugin.h>
#include <kabstractfileitemactionplugin.h>
#include <KFileItemActions>
#include <KFileItemListProperties>
#include <KGlobal>
#include <KIconLoader>
#include <KIO/NetAccess>
#include <KMenu>
#include <KMenuBar>
#include <KMessageBox>
#include <KMimeTypeTrader>
#include <KNewFileMenu>
#include <konqmimedata.h>
#include <konq_operations.h>
#include <KService>
#include <KLocale>
#include <KPropertiesDialog>
#include <KStandardAction>
#include <KStandardDirs>
#include <KToolBar>

#include <panels/places/placesitem.h>
#include <panels/places/placesitemmodel.h>

#include <QApplication>
#include <QClipboard>
#include <QDir>

#include "views/dolphinview.h"
#include "views/viewmodecontroller.h"

DolphinContextMenu::DolphinContextMenu(DolphinMainWindow* parent,
                                       const QPoint& pos,
                                       const KFileItem& fileInfo,
                                       const KUrl& baseUrl) :
    KMenu(parent),
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
    m_shiftPressed(qApp->keyboardModifiers() & Qt::ShiftModifier),
    m_removeAction(0)
{
    // The context menu either accesses the URLs of the selected items
    // or the items itself. To increase the performance both lists are cached.
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();
    m_selectedItems = view->selectedItems();

    m_removeAction = new QAction(this);
    connect(m_removeAction, SIGNAL(triggered()), this, SLOT(slotRemoveActionTriggered()));
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
    if (m_baseUrl.protocol() == QLatin1String("trash")) {
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
    if (ev->key() == Qt::Key_Shift) {
        m_shiftPressed = true;
        updateRemoveAction();
    }
    KMenu::keyPressEvent(ev);
}

void DolphinContextMenu::keyReleaseEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Shift) {
        // not just "m_shiftPressed = false", the user could be playing with both Shift keys...
        m_shiftPressed = qApp->keyboardModifiers() & Qt::ShiftModifier;
        updateRemoveAction();
    }
    KMenu::keyReleaseEvent(ev);
}

void DolphinContextMenu::slotRemoveActionTriggered()
{
    const KActionCollection* collection = m_mainWindow->actionCollection();
    if (moveToTrash()) {
        collection->action("move_to_trash")->trigger();
    } else {
        collection->action("delete")->trigger();
    }
}

void DolphinContextMenu::openTrashContextMenu()
{
    Q_ASSERT(m_context & TrashContext);

    QAction* emptyTrashAction = new QAction(KIcon("trash-empty"), i18nc("@action:inmenu", "Empty Trash"), this);
    KConfig trashConfig("trashrc", KConfig::SimpleConfig);
    emptyTrashAction->setEnabled(!trashConfig.group("Status").readEntry("Empty", true));
    addAction(emptyTrashAction);

    addCustomActions();

    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    addAction(propertiesAction);

    addShowMenuBarAction();

    if (exec(m_pos) == emptyTrashAction) {
        KonqOperations::emptyTrash(m_mainWindow);
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
        KUrl::List selectedUrls;
        foreach (const KFileItem &item, m_selectedItems) {
            selectedUrls.append(item.url());
        }

        KonqOperations::restoreTrashedItems(selectedUrls, m_mainWindow);
    }
}

void DolphinContextMenu::openItemContextMenu()
{
    Q_ASSERT(!m_fileInfo.isNull());

    QAction* openParentInNewWindowAction = 0;
    QAction* openParentInNewTabAction = 0;
    QAction* addToPlacesAction = 0;
    if (m_selectedItems.count() == 1) {
        if (m_fileInfo.isDir()) {
            // setup 'Create New' menu
            DolphinNewFileMenu* newFileMenu = new DolphinNewFileMenu(m_mainWindow);
            const DolphinView* view = m_mainWindow->activeViewContainer()->view();
            newFileMenu->setViewShowsHiddenFiles(view->hiddenFilesShown());
            newFileMenu->checkUpToDate();
            newFileMenu->setPopupFiles(m_fileInfo.url());
            newFileMenu->setEnabled(selectedItemsProperties().supportsWriting());
            connect(newFileMenu, SIGNAL(fileCreated(KUrl)), newFileMenu, SLOT(deleteLater()));
            connect(newFileMenu, SIGNAL(directoryCreated(KUrl)), newFileMenu, SLOT(deleteLater()));

            KMenu* menu = newFileMenu->menu();
            menu->setTitle(i18nc("@title:menu Create new folder, file, link, etc.", "Create New"));
            menu->setIcon(KIcon("document-new"));
            addMenu(menu);
            addSeparator();

            // insert 'Open in new window' and 'Open in new tab' entries
            addAction(m_mainWindow->actionCollection()->action("open_in_new_window"));
            addAction(m_mainWindow->actionCollection()->action("open_in_new_tab"));

            // insert 'Add to Places' entry
            if (!placeExists(m_fileInfo.url())) {
                addToPlacesAction = addAction(KIcon("bookmark-new"),
                                                       i18nc("@action:inmenu Add selected folder to places",
                                                             "Add to Places"));
            }

            addSeparator();
        } else if (m_baseUrl.protocol().contains("search")) {
            openParentInNewWindowAction = new QAction(KIcon("window-new"),
                                                    i18nc("@action:inmenu",
                                                          "Open Path in New Window"),
                                                    this);
            addAction(openParentInNewWindowAction);

            openParentInNewTabAction = new QAction(KIcon("tab-new"),
                                                   i18nc("@action:inmenu",
                                                         "Open Path in New Tab"),
                                                   this);
            addAction(openParentInNewTabAction);

            addSeparator();
        }
    }

    insertDefaultItemActions();

    addSeparator();

    KFileItemActions fileItemActions;
    fileItemActions.setItemListProperties(selectedItemsProperties());
    addServiceActions(fileItemActions);

    addFileItemPluginActions();

    addVersionControlPluginActions();

    // insert 'Copy To' and 'Move To' sub menus
    if (GeneralSettings::showCopyMoveMenu()) {
        m_copyToMenu.setItems(m_selectedItems);
        m_copyToMenu.setReadOnly(!selectedItemsProperties().supportsWriting());
        m_copyToMenu.addActionsTo(this);
    }

    // insert 'Properties...' entry
    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    addAction(propertiesAction);

    QAction* activatedAction = exec(m_pos);
    if (activatedAction) {
        if (activatedAction == addToPlacesAction) {
            const KUrl selectedUrl(m_fileInfo.url());
            if (selectedUrl.isValid()) {
                PlacesItemModel model;
                const QString text = selectedUrl.fileName();
                PlacesItem* item = model.createPlacesItem(text, selectedUrl);
                model.appendItemToGroup(item);
            }
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
        addToPlacesAction = addAction(KIcon("bookmark-new"),
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

void DolphinContextMenu::insertDefaultItemActions()
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
    if (KGlobal::config()->group("KDE").readEntry("ShowDeleteCommand", false)) {
        addAction(collection->action("move_to_trash"));
        addAction(collection->action("delete"));
    } else {
        addAction(m_removeAction);
        updateRemoveAction();
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

bool DolphinContextMenu::placeExists(const KUrl& url) const
{
    PlacesItemModel model;

    const int count = model.count();
    for (int i = 0; i < count; ++i) {
        const KUrl placeUrl = model.placesItem(i)->url();
        if (placeUrl.equals(url, KUrl::CompareWithoutTrailingSlash)) {
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
        action = new QAction(KIcon("edit-paste"), i18nc("@action:inmenu", "Paste Into Folder"), this);
        const QMimeData* mimeData = QApplication::clipboard()->mimeData();
        const KUrl::List pasteData = KUrl::List::fromMimeData(mimeData);
        action->setEnabled(!pasteData.isEmpty() && selectedItemsProperties().supportsWriting());
        connect(action, SIGNAL(triggered()), m_mainWindow, SLOT(pasteIntoFolder()));
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

    foreach (const KSharedPtr<KService>& service, pluginServices) {
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

void DolphinContextMenu::updateRemoveAction()
{
    const KActionCollection* collection = m_mainWindow->actionCollection();

    // Using m_removeAction->setText(action->text()) does not apply the &-shortcut.
    // This is only done until the original action has been shown at least once. To
    // bypass this issue, the text and &-shortcut is applied manually.
    const QAction* action = 0;
    if (moveToTrash()) {
        action = collection->action("move_to_trash");
        m_removeAction->setText(i18nc("@action:inmenu", "&Move to Trash"));
    } else {
        action = collection->action("delete");
        m_removeAction->setText(i18nc("@action:inmenu", "&Delete"));
    }
    m_removeAction->setIcon(action->icon());
    m_removeAction->setShortcuts(action->shortcuts());
}

bool DolphinContextMenu::moveToTrash() const
{
    return !m_shiftPressed;
}

#include "dolphincontextmenu.moc"
