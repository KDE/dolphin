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
#include "editbookmarkdialog.h"

#include <assert.h>

#include <kactioncollection.h>
#include <kbookmarkmanager.h>
#include <kbookmark.h>
#include <kdesktopfile.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kmenu.h>
#include <kmimetypetrader.h>
#include <knewmenu.h>
#include <klocale.h>
#include <kpropertiesdialog.h>
#include <krun.h>
#include <kstandardaction.h>
#include <kstandarddirs.h>

#include <QDir>

DolphinContextMenu::DolphinContextMenu(DolphinView* parent,
                                       KFileItem* fileInfo) :
   m_dolphinView(parent),
   m_fileInfo(fileInfo)
{
}

void DolphinContextMenu::open()
{
    if (m_fileInfo == 0) {
        openViewportContextMenu();
    }
    else {
        openItemContextMenu();
    }
}

DolphinContextMenu::~DolphinContextMenu()
{
}

void DolphinContextMenu::openViewportContextMenu()
{
    assert(m_fileInfo == 0);
    DolphinMainWindow* dolphin = m_dolphinView->mainWindow();
    KMenu* popup = new KMenu(m_dolphinView);

    // setup 'Create New' menu
    KNewMenu* newMenu = dolphin->newMenu();
    newMenu->slotCheckUpToDate();
    newMenu->setPopupFiles(m_dolphinView->url());
    popup->addMenu(newMenu->menu());
    popup->addSeparator();

    QAction* pasteAction = dolphin->actionCollection()->action(KStandardAction::stdName(KStandardAction::Paste));
    popup->addAction(pasteAction);

    // setup 'View Mode' menu
    KMenu* viewModeMenu = new KMenu(i18n("View Mode"));

    QAction* iconsMode = dolphin->actionCollection()->action("icons");
    viewModeMenu->addAction(iconsMode);

    QAction* detailsMode = dolphin->actionCollection()->action("details");
    viewModeMenu->addAction(detailsMode);

    QAction* previewsMode = dolphin->actionCollection()->action("previews");
    viewModeMenu->addAction(previewsMode);

    popup->addMenu(viewModeMenu);
    popup->addSeparator();

    QAction* bookmarkAction = popup->addAction(i18n("Bookmark this folder"));
    popup->addSeparator();

    QAction* propertiesAction = popup->addAction(i18n("Properties..."));

    QAction* activatedAction = popup->exec(QCursor::pos());
    if (activatedAction == propertiesAction) {
        new KPropertiesDialog(dolphin->activeView()->url());
    }
    else if (activatedAction == bookmarkAction) {
        const KUrl& url = dolphin->activeView()->url();
        KBookmark bookmark = EditBookmarkDialog::getBookmark(i18n("Add folder as bookmark"),
                                                             url.fileName(),
                                                             url,
                                                             "bookmark");
        if (!bookmark.isNull()) {
            KBookmarkManager* manager = DolphinSettings::instance().bookmarkManager();
            KBookmarkGroup root = manager->root();
            root.addBookmark(manager, bookmark);
            manager->emitChanged(root);
        }
    }

    popup->deleteLater();
}

void DolphinContextMenu::openItemContextMenu()
{
    assert(m_fileInfo != 0);

    KMenu* popup = new KMenu(m_dolphinView);
    DolphinMainWindow* dolphin = m_dolphinView->mainWindow();
    const KUrl::List urls = m_dolphinView->selectedUrls();

    // insert 'Cut', 'Copy' and 'Paste'
    const KStandardAction::StandardAction actionNames[] = {
        KStandardAction::Cut,
        KStandardAction::Copy,
        KStandardAction::Paste
    };

    const int count = sizeof(actionNames) / sizeof(KStandardAction::StandardAction);
    for (int i = 0; i < count; ++i) {
        QAction* action = dolphin->actionCollection()->action(KStandardAction::stdName(actionNames[i]));
        if (action != 0) {
            popup->addAction(action);
        }
    }
    popup->addSeparator();

    // insert 'Rename'
    QAction* renameAction = dolphin->actionCollection()->action("rename");
    popup->addAction(renameAction);

    // insert 'Move to Trash' for local Urls, otherwise insert 'Delete'
    const KUrl& url = dolphin->activeView()->url();
    if (url.isLocalFile()) {
        QAction* moveToTrashAction = dolphin->actionCollection()->action("move_to_trash");
        popup->addAction(moveToTrashAction);
    }
    else {
        QAction* deleteAction = dolphin->actionCollection()->action("delete");
        popup->addAction(deleteAction);
    }

    // insert 'Bookmark this folder...' entry
    // urls is a list of selected items, so insert boolmark menu if
    // urls contains only one item, i.e. no multiple selection made
    QAction* bookmarkAction = 0;
    if (m_fileInfo->isDir() && (urls.count() == 1)) {
        bookmarkAction = popup->addAction(i18n("Bookmark this folder"));
    }

    // Insert 'Open With...' sub menu
    QVector<KService::Ptr> openWithVector;
    const QList<QAction*> openWithActions = insertOpenWithItems(popup, openWithVector);

    // Insert 'Actions' sub menu
    QVector<KDEDesktopMimeType::Service> actionsVector;
    const QList<QAction*> serviceActions = insertActionItems(popup, actionsVector);
    popup->addSeparator();

    // insert 'Properties...' entry
    QAction* propertiesAction = dolphin->actionCollection()->action("properties");
    popup->addAction(propertiesAction);

    QAction* activatedAction = popup->exec(QCursor::pos());

    if ((bookmarkAction!= 0) && (activatedAction == bookmarkAction)) {
        const KUrl selectedUrl(m_fileInfo->url());
        KBookmark bookmark = EditBookmarkDialog::getBookmark(i18n("Add folder as bookmark"),
                                                             selectedUrl.fileName(),
                                                             selectedUrl,
                                                             "bookmark");
        if (!bookmark.isNull()) {
            KBookmarkManager* manager = DolphinSettings::instance().bookmarkManager();
            KBookmarkGroup root = manager->root();
            root.addBookmark(manager, bookmark);
            manager->emitChanged(root);
        }
    }
    else if (serviceActions.contains(activatedAction)) {
        // one of the 'Actions' items has been selected
        int id = serviceActions.indexOf(activatedAction);
        KDEDesktopMimeType::executeService(urls, actionsVector[id]);
    }
    else if (openWithActions.contains(activatedAction)) {
        // one of the 'Open With' items has been selected
        if (openWithActions.last() == activatedAction) {
            // the item 'Other...' has been selected
            KRun::displayOpenWithDialog(urls, m_dolphinView);
        }
        else {
            int id = openWithActions.indexOf(activatedAction);
            KService::Ptr servicePtr = openWithVector[id];
            KRun::run(*servicePtr, urls, m_dolphinView);
        }
    }

    openWithVector.clear();
    actionsVector.clear();
    popup->deleteLater();
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
    const KFileItemList list = m_dolphinView->selectedItems();

    bool insertOpenWithItems = true;
    const QString contextMimeType(m_fileInfo->mimetype());

    QListIterator<KFileItem*> mimeIt(list);
    while (insertOpenWithItems && mimeIt.hasNext()) {
        KFileItem* item = mimeIt.next();
        insertOpenWithItems = (contextMimeType == item->mimetype());
    }

    QList<QAction*> openWithActions;
    if (insertOpenWithItems) {
        // fill the 'Open with' sub menu with application types
        const KMimeType::Ptr mimePtr = KMimeType::findByUrl(m_fileInfo->url());
        KService::List offers = KMimeTypeTrader::self()->query(mimePtr->name(),
                                                               "Application",
                                                               "Type == 'Application'");
        if (offers.count() > 0) {
            KService::List::Iterator it;
            KMenu* openWithMenu = new KMenu(i18n("Open With"));
            for(it = offers.begin(); it != offers.end(); ++it) {
                // The offer list from the KTrader returns duplicate
                // application entries. Although this seems to be a configuration
                // problem outside the scope of Dolphin, duplicated entries just
                // will be skipped here.
                const QString appName((*it)->name());
                if (!containsEntry(openWithMenu, appName)) {
                    const KIcon icon((*it)->icon());
                    QAction *action = openWithMenu->addAction(icon, appName);
                    openWithVector.append(*it);
                    openWithActions << action;
                }
            }

            openWithMenu->addSeparator();
            QAction* action = openWithMenu->addAction(i18n("&Other..."));

            openWithActions << action;
            popup->addSeparator();
            popup->addMenu(openWithMenu);
        }
        else {
            // No applications are registered, hence just offer
            // a "Open With..." item instead of a sub menu containing
            // only one entry.
            QAction* action = popup->addAction(i18n("Open With..."));
            openWithActions << action;
        }
    }
    else {
        // At least one of the selected items has a different MIME type. In this case
        // just show a disabled "Open With..." entry.
        popup->addSeparator();
        QAction* action = popup->addAction(i18n("Open With..."));
        action->setEnabled(false);
    }

    return openWithActions;
}

QList<QAction*> DolphinContextMenu::insertActionItems(KMenu* popup,
                                                      QVector<KDEDesktopMimeType::Service>& actionsVector)
{
    // Parts of the following code have been taken
    // from the class KonqOperations located in
    // libqonq/konq_operations.h of Konqueror.
    // (Copyright (C) 2000  David Faure <faure@kde.org>)

    KMenu* actionsMenu = new KMenu(i18n("Actions"));

    QList<QAction*> serviceActions;

    QStringList dirs = KGlobal::dirs()->findDirs("data", "dolphin/servicemenus/");

    KMenu* menu = 0;
    for (QStringList::ConstIterator dirIt = dirs.begin(); dirIt != dirs.end(); ++dirIt) {
        QDir dir(*dirIt);
        QStringList filters;
        filters << "*.desktop";
        dir.setNameFilters(filters);
        QStringList entries = dir.entryList(QDir::Files);

        for (QStringList::ConstIterator entryIt = entries.begin(); entryIt != entries.end(); ++entryIt) {
            KConfigGroup cfg(KSharedConfig::openConfig( *dirIt + *entryIt, KConfig::OnlyLocal), "Desktop Entry" );
            if ((cfg.hasKey("Actions") || cfg.hasKey("X-KDE-GetActionMenu")) && cfg.hasKey("ServiceTypes")) {
                //const QStringList types = cfg.readListEntry("ServiceTypes");
                QStringList types;
                types = cfg.readEntry("ServiceTypes", types);
                for (QStringList::ConstIterator it = types.begin(); it != types.end(); ++it) {
                    // check whether the mime type is equal or whether the
                    // mimegroup (e. g. image/*) is supported

                    bool insert = false;
                    if ((*it) == "all/allfiles") {
                        // The service type is valid for all files, but not for directories.
                        // Check whether the selected items only consist of files...
                        const KFileItemList list = m_dolphinView->selectedItems();

                        QListIterator<KFileItem*> mimeIt(list);
                        insert = true;
                        while (insert && mimeIt.hasNext()) {
                            KFileItem* item = mimeIt.next();
                            insert = !item->isDir();
                        }
                    }

                    if (!insert) {
                        // Check whether the MIME types of all selected files match
                        // to the mimetype of the service action. As soon as one MIME
                        // type does not match, no service menu is shown at all.
                        const KFileItemList list = m_dolphinView->selectedItems();

                        QListIterator<KFileItem*> mimeIt(list);
                        insert = true;
                        while (insert && mimeIt.hasNext()) {
                            KFileItem* item = mimeIt.next();
                            const QString mimeType(item->mimetype());
                            const QString mimeGroup(mimeType.left(mimeType.indexOf('/')));

                            insert  = (*it == mimeType) ||
                                      ((*it).right(1) == "*") &&
                                      ((*it).left((*it).indexOf('/')) == mimeGroup);
                        }
                    }

                    if (insert) {
                        menu = actionsMenu;

                        const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );
                        if (!submenuName.isEmpty()) {
                            menu = new KMenu(submenuName);
                            actionsMenu->addMenu(menu);
                        }

                        Q3ValueList<KDEDesktopMimeType::Service> userServices =
                            KDEDesktopMimeType::userDefinedServices(*dirIt + *entryIt, true);

                        Q3ValueList<KDEDesktopMimeType::Service>::Iterator serviceIt;
                        for (serviceIt = userServices.begin(); serviceIt != userServices.end(); ++serviceIt) {
                            KDEDesktopMimeType::Service service = (*serviceIt);
                            if (!service.m_strIcon.isEmpty()) {
                                QAction* action = menu->addAction(SmallIcon(service.m_strIcon),
                                                                  service.m_strName);
                                serviceActions << action;
                            }
                            else {
                                QAction *action = menu->addAction(service.m_strName);
                                serviceActions << action;
                            }
                            actionsVector.append(service);
                        }
                    }
                }
            }
        }
    }

    const int itemsCount = actionsMenu->actions().count();
    if (itemsCount == 0) {
        // no actions are available at all, hence show the "Actions"
        // submenu disabled
        actionsMenu->setEnabled(false);
    }

    if (itemsCount == 1) {
        // Exactly one item is available. Instead of showing a sub menu with
        // only one item, show the item directly in the root menu.
        if (menu == actionsMenu) {
            // The item is an action, hence show the action in the root menu.
            const QList<QAction*> actions = actionsMenu->actions();
            assert(actions.count() == 1);

            const QString text = actions[0]->text();
            const QIcon icon = actions[0]->icon();
            if (icon.isNull()) {
                QAction* action = popup->addAction(text);
                serviceActions.clear();
                serviceActions << action;
            }
            else {
                QAction* action = popup->addAction(icon, text);
                serviceActions.clear();
                serviceActions << action;
            }
        }
        else {
            // The item is a sub menu, hence show the sub menu in the root menu.
            popup->addMenu(menu);
        }
        actionsMenu->deleteLater();
        actionsMenu = 0;
    }
    else {
        popup->addMenu(actionsMenu);
    }

    return serviceActions;
}

bool DolphinContextMenu::containsEntry(const KMenu* menu,
                                       const QString& entryName) const
{
    assert(menu != 0);

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
