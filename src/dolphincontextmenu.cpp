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
#include "dolphinsettings.h"
#include "dolphinview.h"
#include "dolphinviewcontainer.h"
#include "dolphin_generalsettings.h"

#include <kactioncollection.h>
#include <kfileplacesmodel.h>
#include <kdesktopfile.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kmenu.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <knewmenu.h>
#include <konqmimedata.h>
#include <konq_fileitemcapabilities.h>
#include <konq_operations.h>
#include <konq_menuactions.h>
#include <konq_popupmenuinformation.h>
#include <klocale.h>
#include <kpropertiesdialog.h>
#include <krun.h>
#include <kstandardaction.h>
#include <kstandarddirs.h>

#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtCore/QDir>

DolphinContextMenu::DolphinContextMenu(DolphinMainWindow* parent,
                                       const KFileItem& fileInfo,
                                       const KUrl& baseUrl) :
    m_mainWindow(parent),
    m_capabilities(0),
    m_fileInfo(fileInfo),
    m_baseUrl(baseUrl),
    m_context(NoContext)
{
    // The context menu either accesses the URLs of the selected items
    // or the items itself. To increase the performance both lists are cached.
    DolphinView* view = m_mainWindow->activeViewContainer()->view();
    m_selectedUrls = view->selectedUrls();
    m_selectedItems = view->selectedItems();
}

DolphinContextMenu::~DolphinContextMenu()
{
    delete m_capabilities;
    m_capabilities = 0;
}

void DolphinContextMenu::open()
{
    // get the context information
    if (m_baseUrl.protocol() == "trash") {
        m_context |= TrashContext;
    }

    if (!m_fileInfo.isNull() && (m_selectedItems.count() > 0)) {
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

void DolphinContextMenu::openTrashContextMenu()
{
    Q_ASSERT(m_context & TrashContext);

    KMenu* popup = new KMenu(m_mainWindow);

    addShowMenubarAction(popup);

    QAction* emptyTrashAction = new QAction(KIcon("trash-empty"), i18nc("@action:inmenu", "Empty Trash"), popup);
    KConfig trashConfig("trashrc", KConfig::SimpleConfig);
    emptyTrashAction->setEnabled(!trashConfig.group("Status").readEntry("Empty", true));
    popup->addAction(emptyTrashAction);

    QAction* addToPlacesAction = popup->addAction(KIcon("bookmark-new"),
                                                  i18nc("@action:inmenu Add current folder to places", "Add to Places"));

    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    popup->addAction(propertiesAction);

    QAction *action = popup->exec(QCursor::pos());
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

    popup->deleteLater();
}

void DolphinContextMenu::openTrashItemContextMenu()
{
    Q_ASSERT(m_context & TrashContext);
    Q_ASSERT(m_context & ItemContext);

    KMenu* popup = new KMenu(m_mainWindow);

    addShowMenubarAction(popup);

    QAction* restoreAction = new QAction(i18nc("@action:inmenu", "Restore"), m_mainWindow);
    popup->addAction(restoreAction);

    QAction* deleteAction = m_mainWindow->actionCollection()->action("delete");
    popup->addAction(deleteAction);

    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    popup->addAction(propertiesAction);

    if (popup->exec(QCursor::pos()) == restoreAction) {
        KonqOperations::restoreTrashedItems(m_selectedUrls, m_mainWindow);
    }

    popup->deleteLater();
}

void DolphinContextMenu::openItemContextMenu()
{
    Q_ASSERT(!m_fileInfo.isNull());

    KMenu* popup = new KMenu(m_mainWindow);
    addShowMenubarAction(popup);
    insertDefaultItemActions(popup);

    popup->addSeparator();

    // insert 'Bookmark This Folder' entry if exactly one item is selected
    QAction* addToPlacesAction = 0;
    if (m_fileInfo.isDir() && (m_selectedUrls.count() == 1)) {
        addToPlacesAction = popup->addAction(KIcon("bookmark-new"),
                                             i18nc("@action:inmenu Add selected folder to places", "Add to Places"));
    }

    KonqPopupMenuInformation popupInfo;
    popupInfo.setItems(m_selectedItems);
    popupInfo.setParentWidget(m_mainWindow);
    KonqMenuActions menuActions;
    menuActions.setPopupMenuInfo(popupInfo);

    // Insert 'Open With...' action or sub menu
    menuActions.addOpenWithActionsTo(popup, "DesktopEntryName != 'dolphin'");

    // Insert 'Actions' sub menu
    if (menuActions.addActionsTo(popup)) {
        popup->addSeparator();
    }

    // Insert 'Copy To' and 'Move To' sub menus
    if (DolphinSettings::instance().generalSettings()->showCopyMoveMenu()) {
        m_copyToMenu.setItems(m_selectedItems);
        m_copyToMenu.setReadOnly(!capabilities().supportsWriting());
        m_copyToMenu.addActionsTo(popup);
        popup->addSeparator();
    }

    // insert 'Properties...' entry
    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    popup->addAction(propertiesAction);

    QAction* activatedAction = popup->exec(QCursor::pos());

    if ((addToPlacesAction != 0) && (activatedAction == addToPlacesAction)) {
        const KUrl selectedUrl(m_fileInfo.url());
        if (selectedUrl.isValid()) {
            DolphinSettings::instance().placesModel()->addPlace(placesName(selectedUrl),
                                                                selectedUrl);
        }
    }

    popup->deleteLater();
}

void DolphinContextMenu::openViewportContextMenu()
{
    KMenu* popup = new KMenu(m_mainWindow);

    addShowMenubarAction(popup);

    // setup 'Create New' menu
    KNewMenu* newMenu = m_mainWindow->newMenu();
    newMenu->slotCheckUpToDate();
    newMenu->setPopupFiles(m_baseUrl);
    popup->addMenu(newMenu->menu());
    popup->addSeparator();

    QAction* pasteAction = createPasteAction();
    popup->addAction(pasteAction);

    // setup 'View Mode' menu
    KMenu* viewModeMenu = new KMenu(i18nc("@title:menu", "View Mode"));

    QAction* iconsMode = m_mainWindow->actionCollection()->action("icons");
    viewModeMenu->addAction(iconsMode);

    QAction* detailsMode = m_mainWindow->actionCollection()->action("details");
    viewModeMenu->addAction(detailsMode);

    QAction* columnsMode = m_mainWindow->actionCollection()->action("columns");
    viewModeMenu->addAction(columnsMode);

    popup->addMenu(viewModeMenu);

    popup->addSeparator();

    QAction* addToPlacesAction = popup->addAction(KIcon("bookmark-new"),
                                                  i18nc("@action:inmenu Add current folder to places", "Add to Places"));
    popup->addSeparator();

    QAction* propertiesAction = popup->addAction(i18nc("@action:inmenu", "Properties"));

    QAction* action = popup->exec(QCursor::pos());
    if (action == propertiesAction) {
        const KUrl& url = m_mainWindow->activeViewContainer()->url();
                
        KPropertiesDialog* dialog = new KPropertiesDialog(url, m_mainWindow);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
    } else if (action == addToPlacesAction) {
        const KUrl& url = m_mainWindow->activeViewContainer()->url();
        if (url.isValid()) {
            DolphinSettings::instance().placesModel()->addPlace(placesName(url), url);
        }
    }

    popup->deleteLater();
}

void DolphinContextMenu::insertDefaultItemActions(KMenu* popup)
{
    Q_ASSERT(popup != 0);
    const KActionCollection* collection = m_mainWindow->actionCollection();

    // insert 'Cut', 'Copy' and 'Paste'
    QAction* cutAction = collection->action(KStandardAction::name(KStandardAction::Cut));
    QAction* copyAction  = collection->action(KStandardAction::name(KStandardAction::Copy));
    QAction* pasteAction = createPasteAction();

    popup->addAction(cutAction);
    popup->addAction(copyAction);
    popup->addAction(pasteAction);
    popup->addSeparator();

    // insert 'Rename'
    QAction* renameAction = collection->action("rename");
    popup->addAction(renameAction);

    // insert 'Move to Trash' and (optionally) 'Delete'
    KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::IncludeGlobals);
    KConfigGroup configGroup(globalConfig, "KDE");
    bool showDeleteCommand = configGroup.readEntry("ShowDeleteCommand", false);

    const KUrl& url = m_mainWindow->activeViewContainer()->url();
    if (url.isLocalFile()) {
        QAction* moveToTrashAction = collection->action("move_to_trash");
        popup->addAction(moveToTrashAction);
    } else {
        showDeleteCommand = true;
    }

    if (showDeleteCommand) {
        QAction* deleteAction = collection->action("delete");
        popup->addAction(deleteAction);
    }
}

void DolphinContextMenu::addShowMenubarAction(KMenu* menu)
{
    KAction* showMenuBar = m_mainWindow->showMenuBarAction();
    if (!m_mainWindow->menuBar()->isVisible()) {
        // TODO: it should not be necessary to uncheck the menu
        // bar action, but currently the action states don't get
        // updated if the menu is disabled
        showMenuBar->setChecked(false);
        menu->addAction(showMenuBar);
        menu->addSeparator();
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

KonqFileItemCapabilities& DolphinContextMenu::capabilities()
{
    if (m_capabilities == 0) {
        m_capabilities = new KonqFileItemCapabilities(m_selectedItems);
    }
    return *m_capabilities;
}

#include "dolphincontextmenu.moc"
