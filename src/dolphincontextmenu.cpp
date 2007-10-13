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

#include <kactioncollection.h>
#include <kfileplacesmodel.h>
#include <kdesktopfile.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kmenu.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <knewmenu.h>
#include <konqmimedata.h>
#include <konq_operations.h>
#include <konq_menuactions.h>
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
}

void DolphinContextMenu::open()
{
    // get the context information
    if (m_baseUrl.protocol() == "trash") {
        m_context |= TrashContext;
    }

    if (!m_fileInfo.isNull()) {
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

    QAction* emptyTrashAction = new QAction(KIcon("emptytrash"), i18nc("@action:inmenu", "Empty Trash"), popup);
    KConfig trashConfig("trashrc", KConfig::OnlyLocal);
    emptyTrashAction->setEnabled(!trashConfig.group("Status").readEntry("Empty", true));
    popup->addAction(emptyTrashAction);

    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    popup->addAction(propertiesAction);

    if (popup->exec(QCursor::pos()) == emptyTrashAction) {
        const QString text(i18nc("@info", "Do you really want to empty the Trash? All items will get deleted."));
        const bool del = KMessageBox::warningContinueCancel(m_mainWindow,
                                                            text,
                                                            QString(),
                                                            KGuiItem(i18nc("@action:button", "Empty Trash"),
                                                                     KIcon("user-trash"))
                                                           ) == KMessageBox::Continue;
        if (del) {
            KonqOperations::emptyTrash(m_mainWindow);
        }
    }

    popup->deleteLater();
}

void DolphinContextMenu::openTrashItemContextMenu()
{
    Q_ASSERT(m_context & TrashContext);
    Q_ASSERT(m_context & ItemContext);

    KMenu* popup = new KMenu(m_mainWindow);

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
    insertDefaultItemActions(popup);

    popup->addSeparator();

    // insert 'Bookmark This Folder' entry if exactly one item is selected
    QAction* addToPlacesAction = 0;
    if (m_fileInfo.isDir() && (m_selectedUrls.count() == 1)) {
        addToPlacesAction = popup->addAction(KIcon("bookmark-folder"),
                                             i18nc("@action:inmenu Add selected folder to places", "Add to Places"));
    }

    // Insert 'Open With...' sub menu
    QVector<KService::Ptr> openWithVector;
    const QList<QAction*> openWithActions = insertOpenWithItems(popup, openWithVector);

    // Insert 'Actions' sub menu
    KonqMenuActions menuActions;
    menuActions.setItems(m_selectedItems);
    if (menuActions.addActionsTo(popup))
        popup->addSeparator();

    // insert 'Properties...' entry
    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    popup->addAction(propertiesAction);

    QAction* activatedAction = popup->exec(QCursor::pos());

    if ((addToPlacesAction != 0) && (activatedAction == addToPlacesAction)) {
        const KUrl selectedUrl(m_fileInfo.url());
        if (selectedUrl.isValid()) {
            DolphinSettings::instance().placesModel()->addPlace(selectedUrl.fileName(),
                                                                selectedUrl);
        }
    } else if (openWithActions.contains(activatedAction)) {
        // one of the 'Open With' items has been selected
        if (openWithActions.last() == activatedAction) {
            // the item 'Other...' has been selected
            KRun::displayOpenWithDialog(m_selectedUrls, m_mainWindow);
        } else {
            int id = openWithActions.indexOf(activatedAction);
            KService::Ptr servicePtr = openWithVector[id];
            KRun::run(*servicePtr, m_selectedUrls, m_mainWindow);
        }
    }

    openWithVector.clear();
    popup->deleteLater();
}

void DolphinContextMenu::openViewportContextMenu()
{
    Q_ASSERT(m_fileInfo.isNull());
    KMenu* popup = new KMenu(m_mainWindow);

    // setup 'Create New' menu
    KNewMenu* newMenu = m_mainWindow->newMenu();
    newMenu->slotCheckUpToDate();
    newMenu->setPopupFiles(m_baseUrl);
    popup->addMenu(newMenu->menu());
    popup->addSeparator();

    QAction* pasteAction = m_mainWindow->actionCollection()->action(KStandardAction::stdName(KStandardAction::Paste));
    popup->addAction(pasteAction);

    // setup 'View Mode' menu
    KMenu* viewModeMenu = new KMenu(i18nc("@title:menu", "View Mode"));

    QAction* iconsMode = m_mainWindow->actionCollection()->action("icons");
    viewModeMenu->addAction(iconsMode);

    QAction* detailsMode = m_mainWindow->actionCollection()->action("details");
    viewModeMenu->addAction(detailsMode);

    QAction* columnsMode = m_mainWindow->actionCollection()->action("columns");
    viewModeMenu->addAction(columnsMode);

    QAction* previewsMode = m_mainWindow->actionCollection()->action("previews");
    viewModeMenu->addAction(previewsMode);

    popup->addMenu(viewModeMenu);

    popup->addSeparator();

    QAction* addToPlacesAction = popup->addAction(KIcon("bookmark-folder"),
                                                  i18nc("@action:inmenu Add current folder to places", "Add to Places"));
    popup->addSeparator();

    QAction* propertiesAction = popup->addAction(i18nc("@action:inmenu", "Properties"));

    QAction* action = popup->exec(QCursor::pos());
    if (action == propertiesAction) {
        const KUrl& url = m_mainWindow->activeViewContainer()->url();
        KPropertiesDialog dialog(url);
        dialog.exec();
    } else if (action == addToPlacesAction) {
        const KUrl& url = m_mainWindow->activeViewContainer()->url();
        if (url.isValid()) {
            DolphinSettings::instance().placesModel()->addPlace(url.fileName(), url);
        }
    }

    popup->deleteLater();
}

void DolphinContextMenu::insertDefaultItemActions(KMenu* popup)
{
    Q_ASSERT(popup != 0);
    const KActionCollection* collection = m_mainWindow->actionCollection();

    // insert 'Cut', 'Copy' and 'Paste'
    QAction* cutAction = collection->action(KStandardAction::stdName(KStandardAction::Cut));
    QAction* copyAction  = collection->action(KStandardAction::stdName(KStandardAction::Copy));
    QAction* pasteAction = collection->action(KStandardAction::stdName(KStandardAction::Paste));

    popup->addAction(cutAction);
    popup->addAction(copyAction);
    popup->addAction(pasteAction);
    popup->addSeparator();

    // insert 'Rename'
    QAction* renameAction = collection->action("rename");
    popup->addAction(renameAction);

    // insert 'Move to Trash' and (optionally) 'Delete'
    const KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals");
    const KConfigGroup kdeConfig(globalConfig, "KDE");
    bool showDeleteCommand = kdeConfig.readEntry("ShowDeleteCommand", false);
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

QList<QAction*> DolphinContextMenu::insertOpenWithItems(KMenu* popup,
        QVector<KService::Ptr>& openWithVector)
{
    // Parts of the following code have been taken
    // from the class KonqOperations located in
    // libqonq/konq_operations.h of Konqueror.
    // (Copyright (C) 2000  David Faure <faure@kde.org>)

    // Prepare 'Open With' sub menu. Usually a sub menu is created, where all applications
    // are listed which are registered to open the item. As last entry "Other..." will be
    // attached which allows to select a custom application. If no applications are registered
    // no sub menu is created at all, only "Open With..." will be offered.
    bool insertOpenWithItems = true;
    const QString contextMimeType(m_fileInfo.mimetype());

    QListIterator<KFileItem> mimeIt(m_selectedItems);
    while (insertOpenWithItems && mimeIt.hasNext()) {
        KFileItem item = mimeIt.next();
        insertOpenWithItems = (contextMimeType == item.mimetype());
    }

    QList<QAction*> openWithActions;
    if (insertOpenWithItems) {
        // fill the 'Open with' sub menu with application types
        const KMimeType::Ptr mimePtr = KMimeType::findByUrl(m_fileInfo.url());
        KService::List offers = KMimeTypeTrader::self()->query(mimePtr->name(),
                                "Application",
                                "Type == 'Application'");
        if (offers.count() > 0) {
            KService::List::Iterator it;
            KMenu* openWithMenu = new KMenu(i18nc("@title:menu", "Open With"));
            for (it = offers.begin(); it != offers.end(); ++it) {
                // The offer list from the KTrader returns duplicate
                // application entries. Although this seems to be a configuration
                // problem outside the scope of Dolphin, duplicated entries just
                // will be skipped here.
                const QString appName((*it)->name());
                if (!containsEntry(openWithMenu, appName)) {
                    const KIcon icon((*it)->icon());
                    QAction* action = openWithMenu->addAction(icon, appName);
                    openWithVector.append(*it);
                    openWithActions << action;
                }
            }

            openWithMenu->addSeparator();
            QAction* action = openWithMenu->addAction(i18nc("@action:inmenu Open With", "&Other..."));

            openWithActions << action;
            popup->addMenu(openWithMenu);
        } else {
            // No applications are registered, hence just offer
            // a "Open With..." item instead of a sub menu containing
            // only one entry.
            QAction* action = popup->addAction(i18nc("@title:menu", "Open With..."));
            openWithActions << action;
        }
    } else {
        // At least one of the selected items has a different MIME type. In this case
        // just show a disabled "Open With..." entry.
        QAction* action = popup->addAction(i18nc("@title:menu", "Open With..."));
        action->setEnabled(false);
    }

    return openWithActions;
}

bool DolphinContextMenu::containsEntry(const KMenu* menu,
                                       const QString& entryName) const
{
    Q_ASSERT(menu != 0);

    const QList<QAction*> list = menu->actions();
    const uint count = list.count();
    for (uint i = 0; i < count; ++i) {
        const QAction* action = list.at(i);
        if (action->text() == entryName) {
            return true;
        }
    }

    return false;
}

#include "dolphincontextmenu.moc"
