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
#include "settings/dolphinsettings.h"
#include "dolphinviewcontainer.h"
#include "dolphin_generalsettings.h"

#include <kactioncollection.h>
#include <kdesktopfile.h>
#include <kfileitemlistproperties.h>
#include <kfileplacesmodel.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kmenu.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <knewfilemenu.h>
#include <konqmimedata.h>
#include <konq_operations.h>
#include <kfileitemactions.h>
#include <klocale.h>
#include <kpropertiesdialog.h>
#include <kstandardaction.h>
#include <kstandarddirs.h>

#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtCore/QDir>

#include "views/dolphinview.h"
#include "views/viewmodecontroller.h"

DolphinContextMenu::DolphinContextMenu(DolphinMainWindow* parent,
                                       const KFileItem& fileInfo,
                                       const KUrl& baseUrl) :
    m_mainWindow(parent),
    m_capabilities(0),
    m_fileInfo(fileInfo),
    m_baseUrl(baseUrl),
    m_context(NoContext),
    m_copyToMenu(parent),
    m_customActions(),
    m_popup(new KMenu(m_mainWindow)),
    m_shiftPressed(false),
    m_keyInfo(),
    m_removeAction(0)
{
    // The context menu either accesses the URLs of the selected items
    // or the items itself. To increase the performance both lists are cached.
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();
    m_selectedUrls = view->selectedUrls();
    m_selectedItems = view->selectedItems();

    if (m_keyInfo.isKeyPressed(Qt::Key_Shift) || m_keyInfo.isKeyLatched(Qt::Key_Shift)) {
        m_shiftPressed = true;
    }

    connect(&m_keyInfo, SIGNAL(keyPressed(Qt::Key, bool)),
            this, SLOT(slotKeyModifierPressed(Qt::Key, bool)));

    m_removeAction = new QAction(this);
    connect(m_removeAction, SIGNAL(triggered()), this, SLOT(slotRemoveActionTriggered()));
}

DolphinContextMenu::~DolphinContextMenu()
{
    delete m_capabilities;
    m_capabilities = 0;
}

void DolphinContextMenu::setCustomActions(const QList<QAction*>& actions)
{
    m_customActions = actions;
}

void DolphinContextMenu::open()
{
    // get the context information
    if (m_baseUrl.protocol() == "trash") {
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
}

void DolphinContextMenu::slotKeyModifierPressed(Qt::Key key, bool pressed)
{
    m_shiftPressed = (key == Qt::Key_Shift) && pressed;
    updateRemoveAction();
}

void DolphinContextMenu::slotRemoveActionTriggered()
{
    const KActionCollection* collection = m_mainWindow->actionCollection();
    if (m_shiftPressed) {
        collection->action("delete")->trigger();
    } else {
        collection->action("move_to_trash")->trigger();
    }
}

void DolphinContextMenu::openTrashContextMenu()
{
    Q_ASSERT(m_context & TrashContext);

    addShowMenubarAction();

    QAction* emptyTrashAction = new QAction(KIcon("trash-empty"), i18nc("@action:inmenu", "Empty Trash"), m_popup.data());
    KConfig trashConfig("trashrc", KConfig::SimpleConfig);
    emptyTrashAction->setEnabled(!trashConfig.group("Status").readEntry("Empty", true));
    m_popup->addAction(emptyTrashAction);

    QAction* addToPlacesAction = m_popup->addAction(KIcon("bookmark-new"),
                                                  i18nc("@action:inmenu Add current folder to places", "Add to Places"));

    // Don't show if url is already in places
    if (placeExists(m_mainWindow->activeViewContainer()->url())) {
        addToPlacesAction->setVisible(false);
    }

    addCustomActions();

    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    m_popup->addAction(propertiesAction);

    QAction *action = m_popup->exec(QCursor::pos());
    if (action == emptyTrashAction) {
        const QString text(i18nc("@info", "Do you really want to empty the Trash? All items will be deleted."));
        const bool del = KMessageBox::warningContinueCancel(m_mainWindow,
                                                            text,
                                                            QString(),
                                                            KGuiItem(i18nc("@action:button", "Empty Trash"),
                                                                     KIcon("user-trash"))
                                                           ) == KMessageBox::Continue;
        if (del) {
            KonqOperations::emptyTrash(m_mainWindow);
        }
    } else if (action == addToPlacesAction) {
        const KUrl& url = m_mainWindow->activeViewContainer()->url();
        if (url.isValid()) {
            DolphinSettings::instance().placesModel()->addPlace(i18nc("@label", "Trash"), url);
        }
    }
}

void DolphinContextMenu::openTrashItemContextMenu()
{
    Q_ASSERT(m_context & TrashContext);
    Q_ASSERT(m_context & ItemContext);

    addShowMenubarAction();

    QAction* restoreAction = new QAction(i18nc("@action:inmenu", "Restore"), m_mainWindow);
    m_popup->addAction(restoreAction);

    QAction* deleteAction = m_mainWindow->actionCollection()->action("delete");
    m_popup->addAction(deleteAction);

    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    m_popup->addAction(propertiesAction);

    if (m_popup->exec(QCursor::pos()) == restoreAction) {
        KonqOperations::restoreTrashedItems(m_selectedUrls, m_mainWindow);
    }
}

void DolphinContextMenu::openItemContextMenu()
{
    Q_ASSERT(!m_fileInfo.isNull());

    if (m_fileInfo.isDir() && (m_selectedUrls.count() == 1)) {
        // setup 'Create New' menu
        DolphinNewFileMenu* newFileMenu = new DolphinNewFileMenu(m_popup.data(), m_mainWindow);
        const DolphinView* view = m_mainWindow->activeViewContainer()->view();
        newFileMenu->setViewShowsHiddenFiles(view->showHiddenFiles());
        newFileMenu->checkUpToDate();
        newFileMenu->setPopupFiles(m_fileInfo.url());
        newFileMenu->setEnabled(capabilities().supportsWriting());

        KMenu* menu = newFileMenu->menu();
        menu->setTitle(i18nc("@title:menu Create new folder, file, link, etc.", "Create New"));
        menu->setIcon(KIcon("document-new"));
        m_popup->addMenu(menu);
        m_popup->addSeparator();

        // insert 'Open in new window' and 'Open in new tab' entries
        m_popup->addAction(m_mainWindow->actionCollection()->action("open_in_new_window"));
        m_popup->addAction(m_mainWindow->actionCollection()->action("open_in_new_tab"));
        m_popup->addSeparator();
    }
    addShowMenubarAction();
    insertDefaultItemActions();

    m_popup->addSeparator();

    KFileItemActions fileItemActions;
    fileItemActions.setItemListProperties(capabilities());
    addServiceActions(fileItemActions);

    addVersionControlActions();

    // insert 'Copy To' and 'Move To' sub menus
    if (DolphinSettings::instance().generalSettings()->showCopyMoveMenu()) {
        m_copyToMenu.setItems(m_selectedItems);
        m_copyToMenu.setReadOnly(!capabilities().supportsWriting());
        m_copyToMenu.addActionsTo(m_popup.data());
        m_popup->addSeparator();
    }

    // insert 'Add to Places' entry if exactly one item is selected
    QAction* addToPlacesAction = 0;
    if (m_fileInfo.isDir() && (m_selectedUrls.count() == 1) && !placeExists(m_fileInfo.url())) {
        addToPlacesAction = m_popup->addAction(KIcon("bookmark-new"),
                                             i18nc("@action:inmenu Add selected folder to places", "Add to Places"));
    }

    // insert 'Properties...' entry
    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    m_popup->addAction(propertiesAction);

    QAction* activatedAction = m_popup->exec(QCursor::pos());

    if ((addToPlacesAction != 0) && (activatedAction == addToPlacesAction)) {
        const KUrl selectedUrl(m_fileInfo.url());
        if (selectedUrl.isValid()) {
            DolphinSettings::instance().placesModel()->addPlace(placesName(selectedUrl),
                                                                selectedUrl);
        }
    }
}

void DolphinContextMenu::openViewportContextMenu()
{
    addShowMenubarAction();

    // setup 'Create New' menu
    KNewFileMenu* newFileMenu = m_mainWindow->newFileMenu();
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();
    newFileMenu->setViewShowsHiddenFiles(view->showHiddenFiles());
    newFileMenu->checkUpToDate();
    newFileMenu->setPopupFiles(m_baseUrl);
    m_popup->addMenu(newFileMenu->menu());
    m_popup->addSeparator();

    // insert 'Open in new window' and 'Open in new tab' entries
    m_popup->addAction(m_mainWindow->actionCollection()->action("open_in_new_window"));
    m_popup->addAction(m_mainWindow->actionCollection()->action("open_in_new_tab"));
    m_popup->addSeparator();

    QAction* pasteAction = createPasteAction();
    m_popup->addAction(pasteAction);
    m_popup->addSeparator();

    // insert service actions
    const KFileItem item(KFileItem::Unknown, KFileItem::Unknown, m_baseUrl);
    const KFileItemListProperties baseUrlProperties(KFileItemList() << item);
    KFileItemActions fileItemActions;
    fileItemActions.setItemListProperties(baseUrlProperties);
    addServiceActions(fileItemActions);

    addVersionControlActions();

    // insert 'Add to Places' entry if exactly one item is selected
    QAction* addToPlacesAction = 0;
    if (!placeExists(m_mainWindow->activeViewContainer()->url())) {
        addToPlacesAction = m_popup->addAction(KIcon("bookmark-new"),
                                             i18nc("@action:inmenu Add current folder to places", "Add to Places"));
    }

    addCustomActions();

    QAction* propertiesAction = m_popup->addAction(i18nc("@action:inmenu", "Properties"));
    propertiesAction->setIcon(KIcon("document-properties"));
    QAction* action = m_popup->exec(QCursor::pos());
    if (action == propertiesAction) {
        const KUrl& url = m_mainWindow->activeViewContainer()->url();

        KPropertiesDialog* dialog = new KPropertiesDialog(url, m_mainWindow);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    } else if ((addToPlacesAction != 0) && (action == addToPlacesAction)) {
        const KUrl& url = m_mainWindow->activeViewContainer()->url();
        if (url.isValid()) {
            DolphinSettings::instance().placesModel()->addPlace(placesName(url), url);
        }
    }
}

void DolphinContextMenu::insertDefaultItemActions()
{
    const KActionCollection* collection = m_mainWindow->actionCollection();

    // Insert 'Cut', 'Copy' and 'Paste'
    m_popup->addAction(collection->action(KStandardAction::name(KStandardAction::Cut)));
    m_popup->addAction(collection->action(KStandardAction::name(KStandardAction::Copy)));
    m_popup->addAction(createPasteAction());

    m_popup->addSeparator();

    // Insert 'Rename'
    QAction* renameAction = collection->action("rename");
    m_popup->addAction(renameAction);

    // Insert 'Move to Trash' and/or 'Delete'
    if (KGlobal::config()->group("KDE").readEntry("ShowDeleteCommand", false)) {
        m_popup->addAction(collection->action("move_to_trash"));
        m_popup->addAction(collection->action("delete"));
    } else {
        m_popup->addAction(m_removeAction);
        updateRemoveAction();
    }
}

void DolphinContextMenu::addShowMenubarAction()
{
    KAction* showMenuBar = m_mainWindow->showMenuBarAction();
    if (!m_mainWindow->menuBar()->isVisible()) {
        m_popup->addAction(showMenuBar);
        m_popup->addSeparator();
    }
}

QString DolphinContextMenu::placesName(const KUrl& url) const
{
    QString name = url.fileName();
    if (name.isEmpty()) {
        name = url.host();
    }
    return name;
}

bool DolphinContextMenu::placeExists(const KUrl& url) const
{
    const KFilePlacesModel* placesModel = DolphinSettings::instance().placesModel();
    const int count = placesModel->rowCount();

    for (int i = 0; i < count; ++i) {
        const QModelIndex index = placesModel->index(i, 0);

        if (url.equals(placesModel->url(index), KUrl::CompareWithoutTrailingSlash)) {
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
        action->setEnabled(!pasteData.isEmpty() && capabilities().supportsWriting());
        connect(action, SIGNAL(triggered()), m_mainWindow, SLOT(pasteIntoFolder()));
    } else {
        action = m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::Paste));
    }

    return action;
}

KFileItemListProperties& DolphinContextMenu::capabilities()
{
    if (m_capabilities == 0) {
        m_capabilities = new KFileItemListProperties(m_selectedItems);
    }
    return *m_capabilities;
}

void DolphinContextMenu::addServiceActions(KFileItemActions& fileItemActions)
{
    fileItemActions.setParentWidget(m_mainWindow);

    // insert 'Open With...' action or sub menu
    fileItemActions.addOpenWithActionsTo(m_popup.data(), "DesktopEntryName != 'dolphin'");

    // insert 'Actions' sub menu
    if (fileItemActions.addServiceActionsTo(m_popup.data())) {
        m_popup->addSeparator();
    }
}

void DolphinContextMenu::addVersionControlActions()
{
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();
    const QList<QAction*> versionControlActions = view->versionControlActions(m_selectedItems);
    if (!versionControlActions.isEmpty()) {
        foreach (QAction* action, versionControlActions) {
            m_popup->addAction(action);
        }
        m_popup->addSeparator();
    }
}

void DolphinContextMenu::addCustomActions()
{
    foreach (QAction* action, m_customActions) {
        m_popup->addAction(action);
    }
}

void DolphinContextMenu::updateRemoveAction()
{
    const KActionCollection* collection = m_mainWindow->actionCollection();
    const bool moveToTrash = capabilities().isLocal() && !m_shiftPressed;
    const QAction* action = moveToTrash ? collection->action("move_to_trash") : collection->action("delete");
    m_removeAction->setText(action->text());
    m_removeAction->setIcon(action->icon());
    m_removeAction->setShortcuts(action->shortcuts());
}

#include "dolphincontextmenu.moc"
